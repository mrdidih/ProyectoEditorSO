
#define _POSIX_C_SOURCE 200809L

#include "descompresor.h"
#include "archivo.h"
#include "arbol.h"
#include "formato.h"

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

/*
 * Informacion del archivo HUF1.
 */
typedef struct {
    uint64_t tamano;
    uint64_t bloques;
    uint64_t frecuencias[HUFF_SYMBOLS];
} CabeceraHuffman;

/*
 * Notifica el avance cuando hay un callback.
 */
static void informar_progreso(
    HuffmanProgresoDescompresion progreso,
    void *contexto,
    int porcentaje)
{
    if (progreso != NULL)
        progreso(porcentaje, contexto);
}

/*
 * Lee y valida la cabecera HUF1.
 */
static int leer_cabecera(
    int fd,
    CabeceraHuffman *cab)
{
    unsigned char firma[4];
    uint32_t version;
    uint32_t tamano_bloque;

    if (archivo_leer_exacto(fd, firma, 4) != 0 ||
        memcmp(firma, HUFF_MAGIC, 4) != 0 ||
        archivo_leer_u32(fd, &version) != 0 ||
        archivo_leer_u64(fd, &cab->tamano) != 0 ||
        archivo_leer_u32(fd, &tamano_bloque) != 0 ||
        archivo_leer_u64(fd, &cab->bloques) != 0)
    {
        return -1;
    }

    if (version != HUFF_VERSION ||
        tamano_bloque != HUFF_BLOCK_SIZE ||
        cab->tamano > (uint64_t)LLONG_MAX)
    {
        return -1;
    }

    uint64_t bloques_esperados =
        cab->tamano / HUFF_BLOCK_SIZE +
        (cab->tamano % HUFF_BLOCK_SIZE != 0);

    if (cab->bloques != bloques_esperados)
        return -1;

    uint64_t suma = 0;

    for (int i = 0; i < HUFF_SYMBOLS; i++)
    {
        if (archivo_leer_u64(
                fd, &cab->frecuencias[i]) != 0)
        {
            return -1;
        }

        if (cab->frecuencias[i] > UINT64_MAX - suma)
            return -1;

        suma += cab->frecuencias[i];
    }

    return suma == cab->tamano ? 0 : -1;
}

/*
 * Decodifica un bloque Huffman.
 *
 * Comprueba:
 * - Cantidad de bits.
 * - Cantidad de bytes recuperados.
 * - Recorrido valido del arbol.
 * - Frecuencias de los simbolos.
 */
static int decodificar_bloque(
    int fd,
    const NodoHuffman *arbol,
    uint64_t bits_validos,
    uint64_t bytes_esperados,
    uint64_t frecuencias[HUFF_SYMBOLS],
    int fd_salida)
{
    if (arbol == NULL ||
        bytes_esperados == 0 ||
        bytes_esperados > HUFF_BLOCK_SIZE)
    {
        return -1;
    }

    /*
     * Cada simbolo puede ocupar entre
     * 1 y HUFFMAN_MAX_BITS bits.
     */
    if (bits_validos < bytes_esperados ||
        bits_validos >
            bytes_esperados * HUFFMAN_MAX_BITS)
    {
        return -1;
    }

    uint64_t bytes_codificados =
        bits_validos / 8 +
        (bits_validos % 8 != 0);

    if (bytes_codificados == 0 ||
        bytes_codificados > SIZE_MAX)
    {
        return -1;
    }

    unsigned char *datos =
        malloc((size_t)bytes_codificados);

    if (datos == NULL)
        return -1;

    if (archivo_leer_exacto(
            fd,
            datos,
            (size_t)bytes_codificados) != 0)
    {
        free(datos);
        return -1;
    }

    /*
     * Verificar que los bits de relleno
     * del ultimo byte sean cero.
     */
    if (bits_validos % 8 != 0)
    {
        unsigned int sobrantes =
            8U - (unsigned int)(bits_validos % 8);

        unsigned char mascara =
            (unsigned char)((1U << sobrantes) - 1U);

        if ((datos[bytes_codificados - 1] &
             mascara) != 0)
        {
            free(datos);
            return -1;
        }
    }

    unsigned char salida_buffer[HUFF_BLOCK_SIZE];

    uint64_t producidos = 0;

    const NodoHuffman *actual = arbol;

    /*
     * Caso especial: un unico simbolo.
     *
     * El compresor representa cada
     * aparicion mediante un bit cero.
     */
    if (arbol->izquierda == NULL &&
        arbol->derecha == NULL)
    {
        int simbolo = arbol->simbolo;

        if (simbolo < 0 ||
            simbolo >= HUFF_SYMBOLS ||
            bits_validos != bytes_esperados ||
            frecuencias[simbolo] < bytes_esperados)
        {
            free(datos);
            return -1;
        }

        for (uint64_t i = 0; i < bits_validos; i++)
        {
            unsigned int bit =
                (datos[i / 8] >>
                 (7U - (unsigned int)(i % 8))) & 1U;

            if (bit != 0)
            {
                free(datos);
                return -1;
            }
        }

        memset(
            salida_buffer,
            simbolo,
            (size_t)bytes_esperados
        );

        producidos = bytes_esperados;
        frecuencias[simbolo] -= producidos;
    }
    else
    {
        /*
         * Recorrer el arbol bit por bit.
         */
        for (uint64_t i = 0; i < bits_validos; i++)
        {
            unsigned int bit =
                (datos[i / 8] >>
                 (7U - (unsigned int)(i % 8))) & 1U;

            actual = bit ?
                actual->derecha :
                actual->izquierda;

            if (actual == NULL)
            {
                free(datos);
                return -1;
            }

            /*
             * Al llegar a una hoja,
             * recuperamos un simbolo.
             */
            if (actual->izquierda == NULL &&
                actual->derecha == NULL)
            {
                int simbolo = actual->simbolo;

                if (simbolo < 0 ||
                    simbolo >= HUFF_SYMBOLS ||
                    producidos >= bytes_esperados ||
                    frecuencias[simbolo] == 0)
                {
                    free(datos);
                    return -1;
                }

                salida_buffer[producidos++] =
                    (unsigned char)simbolo;

                frecuencias[simbolo]--;

                actual = arbol;
            }
        }

        /*
         * Debemos terminar exactamente
         * en la raiz, tras completar
         * todos los simbolos esperados.
         */
        if (actual != arbol ||
            producidos != bytes_esperados)
        {
            free(datos);
            return -1;
        }
    }

    free(datos);

    return archivo_escribir_exacto(
        fd_salida,
        salida_buffer,
        (size_t)producidos
    );
}

/*
 * Descompresion con seguimiento de progreso.
 *
 * La salida solo se publica si toda
 * la operacion se completa correctamente.
 *
 * Devuelve:
 *   0: exito
 *  -1: error
 */
int huffman_descomprimir_con_progreso(
    const char *entrada,
    const char *salida,
    HuffmanProgresoDescompresion progreso,
    void *contexto)
{
    if (entrada == NULL ||
        salida == NULL ||
        entrada[0] == '\0' ||
        salida[0] == '\0')
    {
        return -1;
    }

    int fd = -1;
    int fd_salida = -1;
    int resultado = -1;

    char *temporal = NULL;
    char *prefijo = NULL;

    uint64_t *longitudes = NULL;

    NodoHuffman *arbol = NULL;

    struct stat info;

    /*
     * Nunca sobrescribir una salida existente.
     */
    if (lstat(salida, &info) == 0 ||
        errno != ENOENT)
    {
        return -1;
    }

    informar_progreso(progreso, contexto, 0);

    fd = open(entrada, O_RDONLY);

    if (fd < 0)
        goto fin;

    if (fstat(fd, &info) != 0 ||
        !S_ISREG(info.st_mode) ||
        info.st_size < 0)
    {
        goto fin;
    }

    CabeceraHuffman cab = {0};

    if (leer_cabecera(fd, &cab) != 0)
        goto fin;

    /*
     * Verificar que la tabla de longitudes
     * quepa en el desplazamiento soportado.
     */
    if (cab.bloques >
        ((uint64_t)LLONG_MAX - HUFF_HEADER_SIZE) / 8)
    {
        goto fin;
    }

    uint64_t inicio_datos =
        HUFF_HEADER_SIZE + cab.bloques * 8;

    /*
     * Un archivo truncado no puede contener
     * la tabla completa.
     */
    if ((uint64_t)info.st_size < inicio_datos)
        goto fin;

    arbol = huffman_crear_arbol(cab.frecuencias);

    if (cab.tamano != 0 && arbol == NULL)
        goto fin;

    if (cab.tamano == 0 && arbol != NULL)
        goto fin;

    uint64_t frecuencias[HUFF_SYMBOLS];

    memcpy(
        frecuencias,
        cab.frecuencias,
        sizeof(frecuencias)
    );

    /*
     * Reservar memoria para las longitudes
     * de los bloques.
     */
    if (cab.bloques > 0)
    {
        if (cab.bloques >
            SIZE_MAX / sizeof(*longitudes))
        {
            goto fin;
        }

        longitudes = calloc(
            (size_t)cab.bloques,
            sizeof(*longitudes)
        );

        if (longitudes == NULL)
            goto fin;
    }

    for (uint64_t i = 0; i < cab.bloques; i++)
    {
        if (archivo_leer_u64(
                fd,
                &longitudes[i]) != 0)
        {
            goto fin;
        }
    }

    /*
     * Pasar del final de la tabla
     * al inicio de los datos Huffman.
     */
    if (lseek(
            fd,
            (off_t)inicio_datos,
            SEEK_SET) == (off_t)-1)
    {
        goto fin;
    }

    /*
     * Crear archivo temporal junto a
     * la salida final.
     */
    size_t n = strlen(salida);

    if (n > SIZE_MAX - 5)
        goto fin;

    prefijo = malloc(n + 5);

    if (prefijo == NULL)
        goto fin;

    memcpy(prefijo, salida, n);
    memcpy(prefijo + n, ".tmp", 5);

    fd_salida = archivo_crear_temporal(
        prefijo,
        &temporal
    );

    if (fd_salida < 0)
        goto fin;

    uint64_t producidos = 0;

    informar_progreso(progreso, contexto, 5);

    /*
     * Decodificar los bloques en orden.
     */
    for (uint64_t i = 0; i < cab.bloques; i++)
    {
        uint64_t restante =
            cab.tamano - producidos;

        uint64_t esperados =
            restante < HUFF_BLOCK_SIZE ?
            restante :
            HUFF_BLOCK_SIZE;

        if (decodificar_bloque(
                fd,
                arbol,
                longitudes[i],
                esperados,
                frecuencias,
                fd_salida) != 0)
        {
            goto fin;
        }

        producidos += esperados;

        /*
         * Calcular avance sin utilizar
         * operaciones de punto flotante.
         *
         * Los datos se procesan en
         * bloques HUF_BLOCK_SIZE.
         *
         * 5%: preparacion completada
         * 99%: todos los bloques escritos
         * 100%: salida publicada
         */
        if (progreso != NULL)
        {
            int porcentaje = 5 + (int)(
                (94.0 * (double)(i + 1)) /
                (double)cab.bloques
            );

            if (porcentaje > 99)
                porcentaje = 99;

            informar_progreso(
                progreso,
                contexto,
                porcentaje
            );
        }
    }

    /*
     * Verificacion del tamano total.
     */
    if (producidos != cab.tamano)
        goto fin;

    /*
     * Todas las frecuencias deben
     * haberse consumido exactamente.
     */
    for (int i = 0; i < HUFF_SYMBOLS; i++)
    {
        if (frecuencias[i] != 0)
            goto fin;
    }

    /*
     * Rechazar datos extra despues
     * del ultimo bloque.
     */
    unsigned char extra;

    ssize_t leidos;

    do {
        leidos = read(fd, &extra, 1);
    } while (leidos < 0 && errno == EINTR);

    if (leidos != 0)
        goto fin;

    /*
     * Persistir y cerrar el archivo
     * temporal antes de publicarlo.
     */
    if (fsync(fd_salida) != 0)
        goto fin;

    if (close(fd_salida) != 0)
    {
        fd_salida = -1;
        goto fin;
    }

    fd_salida = -1;

    /*
     * link() publica la salida sin
     * sobrescribir un archivo existente.
     */
    if (link(temporal, salida) != 0)
        goto fin;

    resultado = 0;

    informar_progreso(progreso, contexto, 100);

fin:
    if (fd >= 0)
        close(fd);

    if (fd_salida >= 0)
        close(fd_salida);

    huffman_liberar_arbol(arbol);

    free(longitudes);

    if (temporal != NULL)
    {
        unlink(temporal);
        free(temporal);
    }

    free(prefijo);

    return resultado;
}

/*
 * API original.
 *
 * Conserva compatibilidad con
 * test_integridad.c y otros modulos.
 */
int huffman_descomprimir(
    const char *entrada,
    const char *salida)
{
    return huffman_descomprimir_con_progreso(
        entrada,
        salida,
        NULL,
        NULL
    );
}
