
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

typedef struct {
    uint64_t tamano;
    uint64_t bloques;
    uint64_t frecuencias[HUFF_SYMBOLS];
} CabeceraHuffman;

static int leer_cabecera(
    int fd,
    CabeceraHuffman *cab)
{
    unsigned char firma[4];
    uint32_t version, bloque;

    if (archivo_leer_exacto(fd, firma, 4) != 0 ||
        memcmp(firma, HUFF_MAGIC, 4) != 0 ||
        archivo_leer_u32(fd, &version) != 0 ||
        archivo_leer_u64(fd, &cab->tamano) != 0 ||
        archivo_leer_u32(fd, &bloque) != 0 ||
        archivo_leer_u64(fd, &cab->bloques) != 0)
        return -1;

    if (version != HUFF_VERSION ||
        bloque != HUFF_BLOCK_SIZE ||
        cab->tamano > (uint64_t)LLONG_MAX)
        return -1;

    uint64_t esperados =
        cab->tamano / HUFF_BLOCK_SIZE +
        (cab->tamano % HUFF_BLOCK_SIZE != 0);

    if (cab->bloques != esperados)
        return -1;

    uint64_t suma = 0;

    for (int i = 0; i < HUFF_SYMBOLS; i++)
    {
        if (archivo_leer_u64(
                fd, &cab->frecuencias[i]) != 0)
            return -1;

        if (cab->frecuencias[i] > UINT64_MAX - suma)
            return -1;

        suma += cab->frecuencias[i];
    }

    return suma == cab->tamano ? 0 : -1;
}

static int decodificar_bloque(
    int fd,
    const NodoHuffman *arbol,
    uint64_t bits_validos,
    uint64_t bytes_esperados,
    uint64_t frecuencias[HUFF_SYMBOLS],
    int salida)
{
    uint64_t bytes_codificados =
        bits_validos / 8 + (bits_validos % 8 != 0);

    if (bytes_codificados > HUFF_BLOCK_SIZE *
                             HUFFMAN_MAX_BITS / 8 + 1)
        return -1;

    unsigned char *datos =
        malloc((size_t)(bytes_codificados ?
                        bytes_codificados : 1));

    if (datos == NULL)
        return -1;

    if (archivo_leer_exacto(
            fd, datos, (size_t)bytes_codificados) != 0)
    {
        free(datos);
        return -1;
    }

    unsigned char salida_buffer[HUFF_BLOCK_SIZE];
    uint64_t producidos = 0;
    const NodoHuffman *actual = arbol;

    if (arbol == NULL || bits_validos == 0)
    {
        free(datos);
        return -1;
    }

    /*
     * Caso especial: arbol de un solo simbolo.
     * Su codigo tiene longitud de un bit.
     */
    if (arbol->izquierda == NULL &&
        arbol->derecha == NULL)
    {
        if (bits_validos != bytes_esperados ||
            frecuencias[arbol->simbolo] <
                bytes_esperados)
        {
            free(datos);
            return -1;
        }

        for (uint64_t i = 0; i < bits_validos; i++)
        {
            unsigned char bit =
                (datos[i / 8] >> (7 - (i % 8))) & 1U;

            if (bit != 0)
            {
                free(datos);
                return -1;
            }
        }

        memset(salida_buffer,
               arbol->simbolo,
               (size_t)bytes_esperados);

        producidos = bytes_esperados;
        frecuencias[arbol->simbolo] -= producidos;
    }
    else
    {
        for (uint64_t i = 0; i < bits_validos; i++)
        {
            unsigned char bit =
                (datos[i / 8] >> (7 - (i % 8))) & 1U;

            actual = bit ?
                actual->derecha : actual->izquierda;

            if (actual == NULL)
            {
                free(datos);
                return -1;
            }

            if (actual->izquierda == NULL &&
                actual->derecha == NULL)
            {
                if (producidos >= bytes_esperados ||
                    frecuencias[actual->simbolo] == 0)
                {
                    free(datos);
                    return -1;
                }

                salida_buffer[producidos++] =
                    (unsigned char)actual->simbolo;

                frecuencias[actual->simbolo]--;
                actual = arbol;
            }
        }

        if (actual != arbol ||
            producidos != bytes_esperados)
        {
            free(datos);
            return -1;
        }
    }

    free(datos);

    return archivo_escribir_exacto(
        salida, salida_buffer, (size_t)producidos);
}

int huffman_descomprimir(
    const char *entrada,
    const char *salida)
{
    if (entrada == NULL || salida == NULL ||
        entrada[0] == '\0' || salida[0] == '\0')
        return -1;

    int fd = -1;
    int fd_salida = -1;
    int resultado = -1;
    char *temporal = NULL;
    char *prefijo = NULL;
    NodoHuffman *arbol = NULL;

    struct stat info;

    if (lstat(salida, &info) == 0 || errno != ENOENT)
        return -1;

    fd = open(entrada, O_RDONLY);

    if (fd < 0)
        goto fin;

    if (fstat(fd, &info) != 0 ||
        !S_ISREG(info.st_mode))
        goto fin;

    CabeceraHuffman cab = {0};

    if (leer_cabecera(fd, &cab) != 0)
        goto fin;

    arbol = huffman_crear_arbol(cab.frecuencias);

    if (cab.tamano != 0 && arbol == NULL)
        goto fin;

    uint64_t frecuencias[HUFF_SYMBOLS];
    memcpy(frecuencias, cab.frecuencias,
           sizeof(frecuencias));

    if (cab.bloques >
        ((uint64_t)LLONG_MAX - HUFF_HEADER_SIZE) / 8)
        goto fin;

    uint64_t inicio_datos =
        HUFF_HEADER_SIZE + cab.bloques * 8;

    /*
     * Leer la tabla completa por adelantado.
     */
    uint64_t *longitudes = NULL;

    if (cab.bloques > 0)
    {
        if (cab.bloques > SIZE_MAX /
                          sizeof(*longitudes))
            goto fin;

        longitudes = calloc(
            (size_t)cab.bloques,
            sizeof(*longitudes));

        if (longitudes == NULL)
            goto fin;
    }

    for (uint64_t i = 0; i < cab.bloques; i++)
    {
        if (archivo_leer_u64(
                fd, &longitudes[i]) != 0)
        {
            free(longitudes);
            goto fin;
        }
    }

    if (lseek(fd, (off_t)inicio_datos,
              SEEK_SET) == (off_t)-1)
    {
        free(longitudes);
        goto fin;
    }

    size_t n = strlen(salida);

    if (n > SIZE_MAX - 5)
    {
        free(longitudes);
        goto fin;
    }

    prefijo = malloc(n + 5);

    if (prefijo == NULL)
    {
        free(longitudes);
        goto fin;
    }

    memcpy(prefijo, salida, n);
    memcpy(prefijo + n, ".tmp", 5);

    fd_salida = archivo_crear_temporal(
        prefijo, &temporal);

    if (fd_salida < 0)
    {
        free(longitudes);
        goto fin;
    }

    uint64_t producidos = 0;

    for (uint64_t i = 0; i < cab.bloques; i++)
    {
        uint64_t restante = cab.tamano - producidos;

        uint64_t esperados =
            restante < HUFF_BLOCK_SIZE ?
            restante : HUFF_BLOCK_SIZE;

        if (decodificar_bloque(
                fd, arbol, longitudes[i],
                esperados, frecuencias,
                fd_salida) != 0)
        {
            free(longitudes);
            goto fin;
        }

        producidos += esperados;
    }

    free(longitudes);

    if (producidos != cab.tamano)
        goto fin;

    for (int i = 0; i < HUFF_SYMBOLS; i++)
    {
        if (frecuencias[i] != 0)
            goto fin;
    }

    /*
     * Rechazar datos sobrantes al final.
     */
    unsigned char extra;

    if (read(fd, &extra, 1) != 0)
        goto fin;

    if (fsync(fd_salida) != 0)
        goto fin;

    if (close(fd_salida) != 0)
    {
        fd_salida = -1;
        goto fin;
    }

    fd_salida = -1;

    if (link(temporal, salida) != 0)
        goto fin;

    resultado = 0;

fin:
    if (fd >= 0)
        close(fd);

    if (fd_salida >= 0)
        close(fd_salida);

    huffman_liberar_arbol(arbol);

    if (temporal != NULL)
    {
        unlink(temporal);
        free(temporal);
    }

    free(prefijo);

    return resultado;
}
