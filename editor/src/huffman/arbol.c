
#include "arbol.h"

#include <stdlib.h>
#include <string.h>
#include <limits.h>

#define MAX_NODOS (HUFFMAN_SIMBOLOS * 2 - 1)

static NodoHuffman *crear_nodo(
    uint64_t frecuencia,
    int simbolo,
    NodoHuffman *izquierda,
    NodoHuffman *derecha)
{
    NodoHuffman *nodo = malloc(sizeof(*nodo));

    if (nodo == NULL)
        return NULL;

    nodo->frecuencia = frecuencia;
    nodo->simbolo = simbolo;
    nodo->izquierda = izquierda;
    nodo->derecha = derecha;

    return nodo;
}

void huffman_liberar_arbol(NodoHuffman *raiz)
{
    if (raiz == NULL)
        return;

    huffman_liberar_arbol(raiz->izquierda);
    huffman_liberar_arbol(raiz->derecha);
    free(raiz);
}

static int comparar_nodos(
    const NodoHuffman *a,
    const NodoHuffman *b)
{
    if (a->frecuencia < b->frecuencia)
        return -1;

    if (a->frecuencia > b->frecuencia)
        return 1;

    return a->simbolo < b->simbolo ? -1 :
           a->simbolo > b->simbolo ? 1 : 0;
}

static int extraer_minimo(
    NodoHuffman **nodos,
    int *cantidad)
{
    int minimo = 0;

    for (int i = 1; i < *cantidad; i++)
    {
        if (comparar_nodos(nodos[i],
                          nodos[minimo]) < 0)
        {
            minimo = i;
        }
    }

    return minimo;
}

NodoHuffman *huffman_crear_arbol(
    const uint64_t frecuencias[HUFFMAN_SIMBOLOS])
{
    if (frecuencias == NULL)
        return NULL;

    NodoHuffman *nodos[MAX_NODOS];
    int cantidad = 0;

    for (int i = 0; i < HUFFMAN_SIMBOLOS; i++)
    {
        if (frecuencias[i] == 0)
            continue;

        nodos[cantidad] = crear_nodo(
            frecuencias[i], i, NULL, NULL
        );

        if (nodos[cantidad] == NULL)
        {
            for (int j = 0; j < cantidad; j++)
                huffman_liberar_arbol(nodos[j]);

            return NULL;
        }

        cantidad++;
    }

    if (cantidad == 0)
        return NULL;

    while (cantidad > 1)
    {
        int indice_a = extraer_minimo(
            nodos, &cantidad
        );

        NodoHuffman *a = nodos[indice_a];

        nodos[indice_a] = nodos[--cantidad];

        int indice_b = extraer_minimo(
            nodos, &cantidad
        );

        NodoHuffman *b = nodos[indice_b];

        nodos[indice_b] = nodos[--cantidad];

        if (UINT64_MAX - a->frecuencia < b->frecuencia)
        {
            huffman_liberar_arbol(a);
            huffman_liberar_arbol(b);

            for (int i = 0; i < cantidad; i++)
                huffman_liberar_arbol(nodos[i]);

            return NULL;
        }

        NodoHuffman *padre = crear_nodo(
            a->frecuencia + b->frecuencia,
            -1,
            a,
            b
        );

        if (padre == NULL)
        {
            huffman_liberar_arbol(a);
            huffman_liberar_arbol(b);

            for (int i = 0; i < cantidad; i++)
                huffman_liberar_arbol(nodos[i]);

            return NULL;
        }

        nodos[cantidad++] = padre;
    }

    return nodos[0];
}

static int generar_recursivo(
    const NodoHuffman *nodo,
    CodigoHuffman codigos[HUFFMAN_SIMBOLOS],
    unsigned char camino[HUFFMAN_MAX_BITS],
    size_t profundidad)
{
    if (nodo == NULL)
        return -1;

    if (nodo->izquierda == NULL &&
        nodo->derecha == NULL)
    {
        CodigoHuffman *codigo = &codigos[nodo->simbolo];

        if (profundidad == 0)
        {
            codigo->bits[0] = 0;
            codigo->longitud = 1;
        }
        else
        {
            memcpy(codigo->bits, camino, profundidad);
            codigo->longitud = profundidad;
        }

        return 0;
    }

    if (profundidad >= HUFFMAN_MAX_BITS)
        return -1;

    camino[profundidad] = 0;

    if (generar_recursivo(
            nodo->izquierda,
            codigos,
            camino,
            profundidad + 1) != 0)
    {
        return -1;
    }

    camino[profundidad] = 1;

    return generar_recursivo(
        nodo->derecha,
        codigos,
        camino,
        profundidad + 1
    );
}

int huffman_generar_codigos(
    const NodoHuffman *raiz,
    CodigoHuffman codigos[HUFFMAN_SIMBOLOS])
{
    if (codigos == NULL || raiz == NULL)
        return -1;

    memset(codigos, 0,
           HUFFMAN_SIMBOLOS * sizeof(*codigos));

    unsigned char camino[HUFFMAN_MAX_BITS];

    return generar_recursivo(
        raiz, codigos, camino, 0
    );
}
