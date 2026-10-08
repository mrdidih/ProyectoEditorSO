
#ifndef HUFFMAN_POOL_H
#define HUFFMAN_POOL_H

#include <stdint.h>
#include "arbol.h"

typedef int (*HuffmanGuardarBloque)(
    uint64_t indice,
    const unsigned char *datos,
    uint64_t bits_validos,
    void *contexto
);

/*
 * Codifica en paralelo un archivo de entrada.
 *
 * El callback guardar_bloque se invoca
 * secuencialmente, en orden de indice.
 *
 * Retorna 0 si todos los bloques fueron
 * procesados correctamente o -1 en error.
 */
int huffman_pool_codificar(
    int fd,
    uint64_t tam_archivo,
    int num_hilos,
    const CodigoHuffman codigos[HUFFMAN_SIMBOLOS],
    HuffmanGuardarBloque guardar_bloque,
    void *contexto
);

#endif
