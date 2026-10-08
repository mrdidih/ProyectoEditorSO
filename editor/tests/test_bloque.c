
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "../src/huffman/arbol.h"
#include "../src/huffman/bloque.h"

int main(void)
{
    const unsigned char texto[] = "AAABBC";

    uint64_t frecuencias[HUFFMAN_SIMBOLOS] = {0};

    frecuencias['A'] = 3;
    frecuencias['B'] = 2;
    frecuencias['C'] = 1;

    NodoHuffman *arbol =
        huffman_crear_arbol(frecuencias);

    if (arbol == NULL)
    {
        printf("ERROR: no se pudo construir el arbol.\n");
        return 1;
    }

    CodigoHuffman codigos[HUFFMAN_SIMBOLOS];

    if (huffman_generar_codigos(arbol, codigos) != 0)
    {
        printf("ERROR: no se pudieron generar codigos.\n");
        huffman_liberar_arbol(arbol);
        return 1;
    }

    BloqueComprimido bloque = {0};

    if (huffman_codificar_bloque(
            texto,
            strlen((const char *)texto),
            codigos,
            &bloque) != 0)
    {
        printf("ERROR: no se pudo codificar el bloque.\n");
        huffman_liberar_arbol(arbol);
        return 1;
    }

    printf("Bits validos: %llu\n",
           (unsigned long long)bloque.bits_validos);

    printf("Bytes utilizados: %zu\n", bloque.bytes);

    printf("Datos hexadecimales:");

    for (size_t i = 0; i < bloque.bytes; i++)
        printf(" %02X", bloque.datos[i]);

    printf("\n");

    int correcto =
        bloque.bits_validos == 9 &&
        bloque.bytes == 2 &&
        bloque.datos[0] == 0xEA &&
        bloque.datos[1] == 0x00;

    /*
     * El arbol usa la asignacion:
     * C=00, B=01, A=1
     *
     * AAABBC = 111010100
     * Bits almacenados: 11101010 00000000
     */

    huffman_liberar_bloque(&bloque);
    huffman_liberar_arbol(arbol);

    if (correcto)
    {
        printf("PRUEBA SUPERADA.\n");
        return 0;
    }

    printf("PRUEBA FALLIDA.\n");
    return 1;
}
