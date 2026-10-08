
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "../src/huffman/frecuencias.h"

#define TAM_ARCHIVO (1024 * 1024)

int main(void)
{
    const char *ruta = "editor/tests/prueba_grande.bin";

    FILE *archivo = fopen(ruta, "wb");

    if (archivo == NULL)
    {
        perror("fopen");
        return 1;
    }

    uint64_t esperado[NUM_SIMBOLOS] = {0};

    for (size_t i = 0; i < TAM_ARCHIVO; i++)
    {
        unsigned char byte = (unsigned char)(i % 256);

        if (fwrite(&byte, 1, 1, archivo) != 1)
        {
            perror("fwrite");
            fclose(archivo);
            return 1;
        }

        esperado[byte]++;
    }

    if (fclose(archivo) != 0)
    {
        perror("fclose");
        return 1;
    }

    int cantidades_hilos[] = {1, 2, 4, 8};

    for (int prueba = 0; prueba < 4; prueba++)
    {
        int num_hilos = cantidades_hilos[prueba];
        uint64_t obtenido[NUM_SIMBOLOS] = {0};

        if (contar_frecuencias_paralelo(
                ruta, num_hilos, obtenido) != 0)
        {
            printf("ERROR con %d hilos.\n", num_hilos);
            return 1;
        }

        if (memcmp(esperado, obtenido,
                   sizeof(esperado)) != 0)
        {
            printf("PRUEBA FALLIDA con %d hilos.\n",
                   num_hilos);
            return 1;
        }

        printf("PRUEBA SUPERADA con %d hilos.\n",
               num_hilos);
    }

    printf("TODAS LAS PRUEBAS SUPERADAS.\n");

    return 0;
}
