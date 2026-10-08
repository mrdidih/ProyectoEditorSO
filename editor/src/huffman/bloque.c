
#include "bloque.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

void huffman_liberar_bloque(BloqueComprimido *bloque)
{
    if (bloque == NULL)
        return;

    free(bloque->datos);
    bloque->datos = NULL;
    bloque->bytes = 0;
    bloque->bits_validos = 0;
}

int huffman_codificar_bloque(
    const unsigned char *entrada,
    size_t longitud,
    const CodigoHuffman codigos[HUFFMAN_SIMBOLOS],
    BloqueComprimido *salida)
{
    if (salida == NULL || codigos == NULL ||
        (longitud > 0 && entrada == NULL))
    {
        return -1;
    }

    salida->datos = NULL;
    salida->bytes = 0;
    salida->bits_validos = 0;

    uint64_t total_bits = 0;

    /* Calcular el espacio necesario. */
    for (size_t i = 0; i < longitud; i++)
    {
        size_t n = codigos[entrada[i]].longitud;

        if (n == 0 || n > HUFFMAN_MAX_BITS)
            return -1;

        if (total_bits > UINT64_MAX - (uint64_t)n)
            return -1;

        total_bits += (uint64_t)n;
    }

    uint64_t bytes64 = total_bits / 8 +
                       (total_bits % 8 != 0);

    if (bytes64 > SIZE_MAX)
        return -1;

    size_t total_bytes = (size_t)bytes64;

    unsigned char *datos = NULL;

    if (total_bytes > 0)
    {
        datos = calloc(total_bytes, 1);

        if (datos == NULL)
            return -1;
    }

    uint64_t posicion = 0;

    /* Escribir los codigos bit a bit. */
    for (size_t i = 0; i < longitud; i++)
    {
        const CodigoHuffman *codigo =
            &codigos[entrada[i]];

        for (size_t j = 0; j < codigo->longitud; j++)
        {
            unsigned char bit = codigo->bits[j];

            if (bit > 1)
            {
                free(datos);
                return -1;
            }

            if (bit == 1)
            {
                size_t indice = (size_t)(posicion / 8);
                unsigned desplazamiento =
                    7U - (unsigned)(posicion % 8);

                datos[indice] |=
                    (unsigned char)(1U << desplazamiento);
            }

            posicion++;
        }
    }

    salida->datos = datos;
    salida->bytes = total_bytes;
    salida->bits_validos = total_bits;

    return 0;
}
