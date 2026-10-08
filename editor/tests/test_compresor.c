
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/stat.h>

#include "../src/huffman/compresor.h"

static void mostrar_progreso(
    int porcentaje,
    void *contexto)
{
    (void)contexto;
    printf("Progreso: %d%%\n", porcentaje);
}

int main(void)
{
    const char *entrada =
        "editor/tests/prueba_huffman.txt";

    const char *salida =
        "editor/tests/prueba_huffman.huff";

    struct stat info;

    if (stat(entrada, &info) != 0)
    {
        fprintf(stderr,
                "ERROR: archivo de entrada no existe.\n");
        return 1;
    }

    /*
     * Eliminar solo el resultado de prueba anterior.
     * Nunca eliminar el archivo original.
     */
    remove(salida);

    printf("Iniciando compresion Huffman...\n");

    int resultado = huffman_comprimir(
        entrada,
        salida,
        4,
        mostrar_progreso,
        NULL
    );

    if (resultado != 0)
    {
        printf("PRUEBA FALLIDA: compresion.\n");
        return 1;
    }

    if (stat(salida, &info) != 0)
    {
        printf("PRUEBA FALLIDA: archivo no generado.\n");
        return 1;
    }

    printf("Archivo comprimido generado.\n");
    printf("Tamano comprimido: %lld bytes\n",
           (long long)info.st_size);

    printf("PRUEBA SUPERADA.\n");
    return 0;
}
