
#include <stdio.h>
#include <stdlib.h>
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
                "No existe el archivo de prueba.\n");
        return 1;
    }

    int resultado = huffman_comprimir(
        entrada,
        salida,
        4,
        mostrar_progreso,
        NULL
    );

    /*
     * El compresor aun no escribe los bloques.
     * Por eso esperamos un retorno -1.
     */
    if (resultado != -1)
    {
        printf("ERROR: resultado inesperado.\n");
        return 1;
    }

    printf("Fase de preparacion ejecutada.\n");
    printf("Compresion completa aun pendiente.\n");

    return 0;
}
