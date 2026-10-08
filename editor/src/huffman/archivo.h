
#ifndef HUFFMAN_ARCHIVO_H
#define HUFFMAN_ARCHIVO_H

#include <stddef.h>
#include <stdint.h>

/* Lee exactamente la cantidad solicitada. */
int archivo_leer_exacto(
    int fd,
    void *buffer,
    size_t cantidad
);

/* Escribe todos los bytes, incluso con escrituras parciales. */
int archivo_escribir_exacto(
    int fd,
    const void *buffer,
    size_t cantidad
);

/* Escribe un numero de 32 bits en little-endian. */
int archivo_escribir_u32(
    int fd,
    uint32_t valor
);

/* Escribe un numero de 64 bits en little-endian. */
int archivo_escribir_u64(
    int fd,
    uint64_t valor
);

/* Lee un numero de 32 bits en little-endian. */
int archivo_leer_u32(
    int fd,
    uint32_t *valor
);

/* Lee un numero de 64 bits en little-endian. */
int archivo_leer_u64(
    int fd,
    uint64_t *valor
);

/*
 * Crea un archivo temporal exclusivo.
 * El llamador debe cerrar fd y eliminar la
 * ruta temporal cuando ya no se necesite.
 */
int archivo_crear_temporal(
    const char *prefijo,
    char **ruta_temporal
);

#endif
