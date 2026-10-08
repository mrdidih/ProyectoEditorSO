
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include "../src/huffman/archivo.h"

int main(void)
{
    char *ruta = NULL;

    int fd = archivo_crear_temporal(
        "editor/tests/prueba_archivo",
        &ruta
    );

    if (fd == -1)
    {
        printf("ERROR: no se pudo crear temporal.\n");
        return 1;
    }

    int correcto = 1;

    const char mensaje[] = "Prueba Huffman";
    char recuperado[sizeof(mensaje)] = {0};

    if (archivo_escribir_u32(fd, 0x12345678U) != 0 ||
        archivo_escribir_u64(fd, UINT64_C(0x123456789ABCDEF0)) != 0 ||
        archivo_escribir_exacto(fd, mensaje,
                                sizeof(mensaje)) != 0)
    {
        correcto = 0;
    }

    if (correcto && lseek(fd, 0, SEEK_SET) == -1)
        correcto = 0;

    uint32_t numero32 = 0;
    uint64_t numero64 = 0;

    if (correcto &&
        (archivo_leer_u32(fd, &numero32) != 0 ||
         archivo_leer_u64(fd, &numero64) != 0 ||
         archivo_leer_exacto(fd, recuperado,
                             sizeof(recuperado)) != 0))
    {
        correcto = 0;
    }

    if (correcto &&
        (numero32 != 0x12345678U ||
         numero64 != UINT64_C(0x123456789ABCDEF0) ||
         memcmp(mensaje, recuperado,
                sizeof(mensaje)) != 0))
    {
        correcto = 0;
    }

    if (close(fd) != 0)
        correcto = 0;

    if (unlink(ruta) != 0)
        correcto = 0;

    free(ruta);

    if (!correcto)
    {
        printf("PRUEBA FALLIDA.\n");
        return 1;
    }

    printf("Lectura y escritura de 32 bits: OK\n");
    printf("Lectura y escritura de 64 bits: OK\n");
    printf("Lectura y escritura de texto: OK\n");
    printf("Creacion y eliminacion temporal: OK\n");
    printf("PRUEBA SUPERADA.\n");

    return 0;
}
