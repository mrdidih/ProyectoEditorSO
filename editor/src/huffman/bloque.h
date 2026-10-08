
#ifndef BLOQUE_H
#define BLOQUE_H

#include <stddef.h>
#include <stdint.h>
#include "arbol.h"

typedef struct {
    unsigned char *datos;
    size_t bytes;
    uint64_t bits_validos;
} BloqueComprimido;

/*
 * Codifica un bloque de bytes usando los codigos
 * generados por el arbol Huffman global.
 *
 * Devuelve 0 en caso de exito y -1 en error.
 * El llamador debe liberar el resultado.
 */
int huffman_codificar_bloque(
    const unsigned char *entrada,
    size_t longitud,
    const CodigoHuffman codigos[HUFFMAN_SIMBOLOS],
    BloqueComprimido *salida
);

void huffman_liberar_bloque(BloqueComprimido *bloque);

#endif
