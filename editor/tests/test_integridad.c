
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include "../src/huffman/compresor.h"
#include "../src/huffman/descompresor.h"

static int comparar_archivos(
    const char *ruta_a,
    const char *ruta_b)
{
    FILE *a = fopen(ruta_a, "rb");
    FILE *b = fopen(ruta_b, "rb");

    if (a == NULL || b == NULL)
    {
        if (a) fclose(a);
        if (b) fclose(b);
        return -1;
    }

    unsigned char datos_a[4096];
    unsigned char datos_b[4096];

    int iguales = 1;

    while (1)
    {
        size_t n_a = fread(datos_a, 1, sizeof(datos_a), a);
        size_t n_b = fread(datos_b, 1, sizeof(datos_b), b);

        if (n_a != n_b ||
            memcmp(datos_a, datos_b, n_a) != 0)
        {
            iguales = 0;
            break;
        }

        if (n_a == 0)
        {
            if (ferror(a) || ferror(b))
                iguales = 0;
            break;
        }
    }

    fclose(a);
    fclose(b);

    return iguales ? 0 : -1;
}

static int ejecutar_prueba(
    const char *nombre,
    const char *entrada,
    const char *comprimido,
    const char *recuperado,
    int hilos)
{
    unlink(comprimido);
    unlink(recuperado);

    printf("Probando: %s (%d hilos)\n",
           nombre, hilos);

    if (huffman_comprimir(
            entrada, comprimido,
            hilos, NULL, NULL) != 0)
    {
        printf("FALLIDA: compresion\n");
        return 1;
    }

    if (huffman_descomprimir(
            comprimido, recuperado) != 0)
    {
        printf("FALLIDA: descompresion\n");
        return 1;
    }

    if (comparar_archivos(entrada, recuperado) != 0)
    {
        printf("FALLIDA: archivos diferentes\n");
        return 1;
    }

    printf("SUPERADA: archivos identicos\n\n");
    return 0;
}

int main(void)
{
    const char *base = "editor/tests/";

    char entrada[256];
    char comprimido[256];
    char recuperado[256];

    const char *casos[] = {
        "prueba_huffman.txt",
        "prueba_grande.bin",
        "prueba_pool.bin"
    };

    int hilos[] = {1, 4, 8};
    int errores = 0;

    for (int i = 0; i < 3; i++)
    {
        snprintf(entrada, sizeof(entrada),
                 "%s%s", base, casos[i]);

        snprintf(comprimido, sizeof(comprimido),
                 "%ssalida_%d.huff", base, i);

        snprintf(recuperado, sizeof(recuperado),
                 "%srecuperado_%d.bin", base, i);

        errores += ejecutar_prueba(
            casos[i],
            entrada,
            comprimido,
            recuperado,
            hilos[i]
        );
    }

    if (errores == 0)
    {
        printf("TODAS LAS PRUEBAS DE INTEGRIDAD SUPERADAS.\n");
        return 0;
    }

    printf("TOTAL DE ERRORES: %d\n", errores);
    return 1;
}
