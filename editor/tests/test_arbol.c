
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "../src/huffman/arbol.h"

static int verificar_prefijo(
    const CodigoHuffman *a,
    const CodigoHuffman *b)
{
    if (a->longitud > b->longitud)
        return 0;

    return memcmp(a->bits, b->bits,
                  a->longitud) == 0;
}

static int ejecutar_prueba(
    const char *nombre,
    const uint64_t frecuencias[HUFFMAN_SIMBOLOS])
{
    NodoHuffman *raiz = huffman_crear_arbol(frecuencias);

    if (raiz == NULL)
    {
        printf("FALLIDA: %s (arbol nulo)\n", nombre);
        return 1;
    }

    CodigoHuffman codigos[HUFFMAN_SIMBOLOS];

    if (huffman_generar_codigos(raiz, codigos) != 0)
    {
        printf("FALLIDA: %s (codigos)\n", nombre);
        huffman_liberar_arbol(raiz);
        return 1;
    }

    uint64_t total = 0;

    for (int i = 0; i < HUFFMAN_SIMBOLOS; i++)
    {
        total += frecuencias[i];

        if (frecuencias[i] > 0 &&
            codigos[i].longitud == 0)
        {
            printf("FALLIDA: %s (simbolo sin codigo)\n",
                   nombre);
            huffman_liberar_arbol(raiz);
            return 1;
        }

        if (frecuencias[i] == 0 &&
            codigos[i].longitud != 0)
        {
            printf("FALLIDA: %s (codigo inesperado)\n",
                   nombre);
            huffman_liberar_arbol(raiz);
            return 1;
        }
    }

    if (raiz->frecuencia != total)
    {
        printf("FALLIDA: %s (frecuencia raiz)\n", nombre);
        huffman_liberar_arbol(raiz);
        return 1;
    }

    for (int i = 0; i < HUFFMAN_SIMBOLOS; i++)
    {
        if (frecuencias[i] == 0)
            continue;

        for (int j = 0; j < HUFFMAN_SIMBOLOS; j++)
        {
            if (i == j || frecuencias[j] == 0)
                continue;

            if (verificar_prefijo(&codigos[i],
                                 &codigos[j]))
            {
                printf("FALLIDA: %s (conflicto prefijos)\n",
                       nombre);
                huffman_liberar_arbol(raiz);
                return 1;
            }
        }
    }

    printf("SUPERADA: %s\n", nombre);

    huffman_liberar_arbol(raiz);
    return 0;
}

int main(void)
{
    uint64_t normal[HUFFMAN_SIMBOLOS] = {0};
    uint64_t unico[HUFFMAN_SIMBOLOS] = {0};
    uint64_t empatados[HUFFMAN_SIMBOLOS] = {0};
    uint64_t completo[HUFFMAN_SIMBOLOS] = {0};

    normal['A'] = 3;
    normal['B'] = 2;
    normal['C'] = 1;

    unico['X'] = 100;

    for (int i = 0; i < 10; i++)
        empatados[i] = 5;

    for (int i = 0; i < HUFFMAN_SIMBOLOS; i++)
        completo[i] = 1;

    int errores = 0;

    errores += ejecutar_prueba(
        "Frecuencias A=3 B=2 C=1", normal);

    errores += ejecutar_prueba(
        "Un unico simbolo", unico);

    errores += ejecutar_prueba(
        "Frecuencias empatadas", empatados);

    errores += ejecutar_prueba(
        "Los 256 simbolos", completo);

    NodoHuffman *vacio = huffman_crear_arbol(
        (uint64_t[HUFFMAN_SIMBOLOS]){0}
    );

    if (vacio == NULL)
        printf("SUPERADA: Archivo vacio\n");
    else
    {
        printf("FALLIDA: Archivo vacio\n");
        huffman_liberar_arbol(vacio);
        errores++;
    }

    if (errores == 0)
    {
        printf("TODAS LAS PRUEBAS SUPERADAS.\n");
        return 0;
    }

    printf("TOTAL DE ERRORES: %d\n", errores);
    return 1;
}
