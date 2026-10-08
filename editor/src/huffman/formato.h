
#ifndef FORMATO_H
#define FORMATO_H

#include <stdint.h>

#define HUFF_MAGIC "HUF1"
#define HUFF_VERSION 1

#define HUFF_BLOCK_SIZE 65536U
#define HUFF_SYMBOLS 256

/*
 * Formato HUF1 - Version 1
 *
 * Todos los enteros multibyte se almacenan
 * en little-endian, independientemente
 * de la arquitectura del equipo.
 *
 * Cabecera:
 *   4 bytes: identificador "HUF1"
 *   4 bytes: version (uint32)
 *   8 bytes: tamano original (uint64)
 *   4 bytes: tamano de bloque (uint32)
 *   8 bytes: numero de bloques (uint64)
 *   256 * 8 bytes: frecuencias (uint64)
 *
 * Tabla de bloques:
 *   Por cada bloque:
 *   8 bytes: numero de bits validos (uint64)
 *
 * Datos:
 *   Bloques comprimidos consecutivos.
 *   Cada bloque ocupa ceil(bits_validos / 8) bytes.
 *   Los bits se almacenan del mas significativo
 *   al menos significativo dentro de cada byte.
 *
 * Las frecuencias permiten reconstruir
 * el arbol Huffman global.
 */

#define HUFF_HEADER_SIZE \
    (4U + 4U + 8U + 4U + 8U + HUFF_SYMBOLS * 8U)

/* Escribir enteros en little-endian. */
static inline void huff_escribir_u32(
    unsigned char destino[4],
    uint32_t valor)
{
    for (int i = 0; i < 4; i++)
    {
        destino[i] = (unsigned char)
            ((valor >> (8 * i)) & 0xFFU);
    }
}

static inline void huff_escribir_u64(
    unsigned char destino[8],
    uint64_t valor)
{
    for (int i = 0; i < 8; i++)
    {
        destino[i] = (unsigned char)
            ((valor >> (8 * i)) & 0xFFU);
    }
}

/* Leer enteros en little-endian. */
static inline uint32_t huff_leer_u32(
    const unsigned char origen[4])
{
    uint32_t valor = 0;

    for (int i = 0; i < 4; i++)
    {
        valor |= (uint32_t)origen[i] << (8 * i);
    }

    return valor;
}

static inline uint64_t huff_leer_u64(
    const unsigned char origen[8])
{
    uint64_t valor = 0;

    for (int i = 0; i < 8; i++)
    {
        valor |= (uint64_t)origen[i] << (8 * i);
    }

    return valor;
}

#endif
