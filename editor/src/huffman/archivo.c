
#define _POSIX_C_SOURCE 200809L

#include "archivo.h"
#include "formato.h"

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int archivo_leer_exacto(
    int fd,
    void *buffer,
    size_t cantidad)
{
    unsigned char *p = buffer;
    size_t total = 0;

    if (fd < 0 || (cantidad > 0 && buffer == NULL))
        return -1;

    while (total < cantidad)
    {
        ssize_t n = read(fd, p + total, cantidad - total);

        if (n < 0 && errno == EINTR)
            continue;

        if (n <= 0)
            return -1;

        total += (size_t)n;
    }

    return 0;
}

int archivo_escribir_exacto(
    int fd,
    const void *buffer,
    size_t cantidad)
{
    const unsigned char *p = buffer;
    size_t total = 0;

    if (fd < 0 || (cantidad > 0 && buffer == NULL))
        return -1;

    while (total < cantidad)
    {
        ssize_t n = write(fd, p + total, cantidad - total);

        if (n < 0 && errno == EINTR)
            continue;

        if (n <= 0)
            return -1;

        total += (size_t)n;
    }

    return 0;
}

int archivo_escribir_u32(int fd, uint32_t valor)
{
    unsigned char datos[4];
    huff_escribir_u32(datos, valor);
    return archivo_escribir_exacto(fd, datos, sizeof(datos));
}

int archivo_escribir_u64(int fd, uint64_t valor)
{
    unsigned char datos[8];
    huff_escribir_u64(datos, valor);
    return archivo_escribir_exacto(fd, datos, sizeof(datos));
}

int archivo_leer_u32(int fd, uint32_t *valor)
{
    unsigned char datos[4];

    if (valor == NULL ||
        archivo_leer_exacto(fd, datos, sizeof(datos)) != 0)
        return -1;

    *valor = huff_leer_u32(datos);
    return 0;
}

int archivo_leer_u64(int fd, uint64_t *valor)
{
    unsigned char datos[8];

    if (valor == NULL ||
        archivo_leer_exacto(fd, datos, sizeof(datos)) != 0)
        return -1;

    *valor = huff_leer_u64(datos);
    return 0;
}

int archivo_crear_temporal(
    const char *prefijo,
    char **ruta_temporal)
{
    if (prefijo == NULL || ruta_temporal == NULL)
        return -1;

    *ruta_temporal = NULL;

    const char sufijo[] = ".XXXXXX";
    size_t longitud = strlen(prefijo);

    if (longitud > SIZE_MAX - sizeof(sufijo))
        return -1;

    char *ruta = malloc(longitud + sizeof(sufijo));

    if (ruta == NULL)
        return -1;

    memcpy(ruta, prefijo, longitud);
    memcpy(ruta + longitud, sufijo, sizeof(sufijo));

    int fd = mkstemp(ruta);

    if (fd == -1)
    {
        free(ruta);
        return -1;
    }

    *ruta_temporal = ruta;
    return fd;
}
