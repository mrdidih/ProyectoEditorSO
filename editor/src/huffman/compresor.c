
#define _POSIX_C_SOURCE 200809L

#include "compresor.h"
#include "archivo.h"
#include "arbol.h"
#include "frecuencias.h"
#include "formato.h"
#include "pool.h"

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define HUFF_MAX_HILOS 32

typedef struct {
    int fd;
    uint64_t tabla_inicio;
    uint64_t completados;
    uint64_t total;
    HuffmanProgreso progreso;
    void *usuario;
} EscritorHuffman;

static int copiar_entrada(
    const char *entrada,
    const char *salida,
    uint64_t *tamano)
{
    int origen = -1;
    int destino = -1;
    int rc = -1;
    char *temporal = NULL;
    struct stat antes, despues, final;
    unsigned char buffer[HUFF_BLOCK_SIZE];

    origen = open(entrada, O_RDONLY);
    if (origen < 0)
        goto fin;

    if (fstat(origen, &antes) != 0 ||
        !S_ISREG(antes.st_mode) ||
        antes.st_size < 0)
        goto fin;

    destino = archivo_crear_temporal(salida, &temporal);
    if (destino < 0)
        goto fin;

    uint64_t total = 0;

    while (1)
    {
        ssize_t n = read(origen, buffer, sizeof(buffer));

        if (n < 0 && errno == EINTR)
            continue;

        if (n < 0)
            goto fin;

        if (n == 0)
            break;

        if (archivo_escribir_exacto(
                destino, buffer, (size_t)n) != 0)
            goto fin;

        total += (uint64_t)n;
    }

    if (fstat(origen, &despues) != 0 ||
        fstat(destino, &final) != 0)
        goto fin;

    /*
     * Detecta algunos cambios externos durante
     * la copia. La sincronizacion completa con
     * el editor se implementara posteriormente.
     */
    if (antes.st_size != despues.st_size ||
        antes.st_mtime != despues.st_mtime ||
        total != (uint64_t)final.st_size ||
        total > (uint64_t)LLONG_MAX)
        goto fin;

    if (close(destino) != 0)
    {
        destino = -1;
        goto fin;
    }

    destino = -1;

    if (rename(temporal, salida) != 0)
        goto fin;

    *tamano = total;
    rc = 0;

fin:
    if (origen >= 0)
        close(origen);

    if (destino >= 0)
        close(destino);

    if (temporal != NULL)
    {
        if (rc != 0)
            unlink(temporal);
        free(temporal);
    }

    return rc;
}

static int escribir_en_posicion(
    int fd,
    const unsigned char *datos,
    size_t cantidad,
    uint64_t posicion)
{
    size_t escritos = 0;

    while (escritos < cantidad)
    {
        if (posicion > (uint64_t)LLONG_MAX -
                       (uint64_t)escritos)
            return -1;

        ssize_t n = pwrite(
            fd,
            datos + escritos,
            cantidad - escritos,
            (off_t)(posicion + escritos)
        );

        if (n < 0 && errno == EINTR)
            continue;

        if (n <= 0)
            return -1;

        escritos += (size_t)n;
    }

    return 0;
}

static int escribir_cabecera(
    int fd,
    uint64_t tamano,
    uint64_t bloques,
    const uint64_t frecuencias[HUFF_SYMBOLS])
{
    if (archivo_escribir_exacto(
            fd, HUFF_MAGIC, 4) != 0 ||
        archivo_escribir_u32(fd, HUFF_VERSION) != 0 ||
        archivo_escribir_u64(fd, tamano) != 0 ||
        archivo_escribir_u32(fd, HUFF_BLOCK_SIZE) != 0 ||
        archivo_escribir_u64(fd, bloques) != 0)
        return -1;

    for (int i = 0; i < HUFF_SYMBOLS; i++)
    {
        if (archivo_escribir_u64(
                fd, frecuencias[i]) != 0)
            return -1;
    }

    return 0;
}

static int guardar_bloque(
    uint64_t indice,
    const unsigned char *datos,
    uint64_t bits_validos,
    void *argumento)
{
    EscritorHuffman *e = argumento;

    if (indice != e->completados ||
        bits_validos == 0 ||
        datos == NULL)
        return -1;

    uint64_t bytes =
        bits_validos / 8 +
        (bits_validos % 8 != 0);

    if (bytes > SIZE_MAX ||
        indice > (UINT64_MAX - e->tabla_inicio) / 8)
        return -1;

    unsigned char longitud[8];
    huff_escribir_u64(longitud, bits_validos);

    uint64_t posicion =
        e->tabla_inicio + indice * 8;

    /*
     * pwrite no modifica el offset compartido.
     * Por eso podemos escribir la tabla mientras
     * los datos se escriben secuencialmente.
     */
    if (escribir_en_posicion(
            e->fd, longitud, sizeof(longitud),
            posicion) != 0)
        return -1;

    if (archivo_escribir_exacto(
            e->fd, datos, (size_t)bytes) != 0)
        return -1;

    e->completados++;

    if (e->progreso != NULL && e->total != 0)
    {
        int porcentaje = 40 +
            (int)((60.0 * e->completados) / e->total);

        e->progreso(porcentaje, e->usuario);
    }

    return 0;
}

int huffman_comprimir(
    const char *entrada,
    const char *salida,
    int num_hilos,
    HuffmanProgreso progreso,
    void *contexto)
{
    int resultado = -1;
    int fd_entrada = -1;
    int fd_salida = -1;
    char *ruta_copia = NULL;
    char *ruta_salida_temporal = NULL;
    char *prefijo_salida = NULL;
    NodoHuffman *arbol = NULL;

    if (entrada == NULL || salida == NULL ||
        entrada[0] == '\0' || salida[0] == '\0' ||
        num_hilos < 1 || num_hilos > HUFF_MAX_HILOS)
        return -1;

    /*
     * Por seguridad no reemplazamos archivos
     * de salida existentes en esta version.
     */
    struct stat info;

    if (lstat(salida, &info) == 0 || errno != ENOENT)
    {
        fprintf(stderr,
                "La salida ya existe o no se puede consultar.\n");
        return -1;
    }

    if (progreso != NULL)
        progreso(0, contexto);

    uint64_t tamano = 0;

    fd_entrada = archivo_crear_temporal(
        "/tmp/huff_entrada", &ruta_copia);

    if (fd_entrada < 0)
        goto fin;

    if (close(fd_entrada) != 0)
    {
        fd_entrada = -1;
        goto fin;
    }
    fd_entrada = -1;

    /*
     * La copia original se publica sobre
     * el nombre temporal reservado.
     */
    if (copiar_entrada(
            entrada, ruta_copia, &tamano) != 0)
        goto fin;

    if (progreso != NULL)
        progreso(10, contexto);

    uint64_t frecuencias[HUFF_SYMBOLS] = {0};

    if (contar_frecuencias_paralelo(
            ruta_copia, num_hilos, frecuencias) != 0)
        goto fin;

    uint64_t suma = 0;

    for (int i = 0; i < HUFF_SYMBOLS; i++)
    {
        if (frecuencias[i] > UINT64_MAX - suma)
            goto fin;

        suma += frecuencias[i];
    }

    if (suma != tamano)
        goto fin;

    if (progreso != NULL)
        progreso(25, contexto);

    arbol = huffman_crear_arbol(frecuencias);

    if (tamano > 0 && arbol == NULL)
        goto fin;

    CodigoHuffman *codigos =
        calloc(HUFF_SYMBOLS, sizeof(*codigos));

    if (codigos == NULL)
        goto fin_codigos;

    if (arbol != NULL &&
        huffman_generar_codigos(arbol, codigos) != 0)
        goto fin_codigos;

    if (progreso != NULL)
        progreso(40, contexto);

    uint64_t bloques =
        tamano / HUFF_BLOCK_SIZE +
        (tamano % HUFF_BLOCK_SIZE != 0);

    if (bloques >
        ((uint64_t)LLONG_MAX - HUFF_HEADER_SIZE) / 8)
        goto fin_codigos;

    /*
     * Crear temporal en el mismo directorio
     * de salida para publicar mediante link().
     */
    size_t longitud = strlen(salida);

    if (longitud > SIZE_MAX - 5)
        goto fin_codigos;

    prefijo_salida = malloc(longitud + 5);
    if (prefijo_salida == NULL)
        goto fin_codigos;

    memcpy(prefijo_salida, salida, longitud);
    memcpy(prefijo_salida + longitud, ".tmp", 5);

    fd_salida = archivo_crear_temporal(
        prefijo_salida, &ruta_salida_temporal);

    if (fd_salida < 0)
        goto fin_codigos;

    if (escribir_cabecera(
            fd_salida, tamano, bloques,
            frecuencias) != 0)
        goto fin_codigos;

    uint64_t inicio_datos =
        HUFF_HEADER_SIZE + bloques * 8;

    if (lseek(
            fd_salida, (off_t)inicio_datos,
            SEEK_SET) == (off_t)-1)
        goto fin_codigos;

    fd_entrada = open(ruta_copia, O_RDONLY);
    if (fd_entrada < 0)
        goto fin_codigos;

    EscritorHuffman escritor = {
        .fd = fd_salida,
        .tabla_inicio = HUFF_HEADER_SIZE,
        .completados = 0,
        .total = bloques,
        .progreso = progreso,
        .usuario = contexto
    };

    if (huffman_pool_codificar(
            fd_entrada,
            tamano,
            num_hilos,
            codigos,
            guardar_bloque,
            &escritor) != 0)
        goto fin_codigos;

    if (escritor.completados != bloques)
        goto fin_codigos;

    if (fsync(fd_salida) != 0)
        goto fin_codigos;

    if (close(fd_salida) != 0)
    {
        fd_salida = -1;
        goto fin_codigos;
    }
    fd_salida = -1;

    /*
     * link() publica el resultado completo
     * sin sobrescribir un destino existente.
     */
    if (link(ruta_salida_temporal, salida) != 0)
        goto fin_codigos;

    resultado = 0;

    if (progreso != NULL)
        progreso(100, contexto);

fin_codigos:
    free(codigos);

fin:
    if (fd_entrada >= 0)
        close(fd_entrada);

    if (fd_salida >= 0)
        close(fd_salida);

    huffman_liberar_arbol(arbol);

    if (ruta_copia != NULL)
    {
        unlink(ruta_copia);
        free(ruta_copia);
    }

    if (ruta_salida_temporal != NULL)
    {
        unlink(ruta_salida_temporal);
        free(ruta_salida_temporal);
    }

    free(prefijo_salida);

    if (resultado != 0)
        fprintf(stderr, "Compresion Huffman fallida.\n");

    return resultado;
}
