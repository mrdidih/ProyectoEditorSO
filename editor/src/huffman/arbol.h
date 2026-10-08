
#ifndef ARBOL_H
#define ARBOL_H

#include <stdint.h>
#include <stddef.h>

#define HUFFMAN_SIMBOLOS 256
#define HUFFMAN_MAX_BITS 255

typedef struct NodoHuffman {
    uint64_t frecuencia;
    int simbolo;
    struct NodoHuffman *izquierda;
    struct NodoHuffman *derecha;
} NodoHuffman;

typedef struct {
    unsigned char bits[HUFFMAN_MAX_BITS];
    size_t longitud;
} CodigoHuffman;

/*
 * Construye el arbol a partir de las frecuencias.
 * Retorna NULL si esta vacio o si ocurre un error.
 */
NodoHuffman *huffman_crear_arbol(
    const uint64_t frecuencias[HUFFMAN_SIMBOLOS]
);

/* Genera los codigos para cada simbolo. */
int huffman_generar_codigos(
    const NodoHuffman *raiz,
    CodigoHuffman codigos[HUFFMAN_SIMBOLOS]
);

/* Libera todos los nodos del arbol. */
void huffman_liberar_arbol(NodoHuffman *raiz);

#endif
