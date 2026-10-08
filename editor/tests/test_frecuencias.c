
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include "../src/huffman/frecuencias.h"

int main(void)
{
    const char *ruta = "editor/tests/prueba_huffman.txt";
    const char *contenido = "AAABBC";

    int fd = open(ruta, O_WRONLY | O_CREAT | O_TRUNC, 0644);

    if (fd == -1)
    {
        perror("open");
        return 1;
    }

    size_t longitud = strlen(contenido);

    if (write(fd, contenido, longitud) != (ssize_t)longitud)
    {
        perror("write");
        close(fd);
        return 1;
    }

    if (close(fd) == -1)
    {
        perror("close");
        return 1;
    }

    uint64_t frecuencias[NUM_SIMBOLOS] = {0};

    if (contar_frecuencias_paralelo(ruta, 3, frecuencias) != 0)
    {
        fprintf(stderr, "Error contando frecuencias.\n");
        return 1;
    }

    printf("Frecuencia de A: %llu\n",
           (unsigned long long)frecuencias['A']);

    printf("Frecuencia de B: %llu\n",
           (unsigned long long)frecuencias['B']);

    printf("Frecuencia de C: %llu\n",
           (unsigned long long)frecuencias['C']);

    uint64_t total = 0;

    for (int i = 0; i < NUM_SIMBOLOS; i++)
    {
        total += frecuencias[i];
    }

    printf("Total de bytes: %llu\n",
           (unsigned long long)total);

    if (frecuencias['A'] == 3 &&
        frecuencias['B'] == 2 &&
        frecuencias['C'] == 1 &&
        total == longitud)
    {
        printf("PRUEBA SUPERADA.\n");
        return 0;
    }

    printf("PRUEBA FALLIDA.\n");
    return 1;
}
