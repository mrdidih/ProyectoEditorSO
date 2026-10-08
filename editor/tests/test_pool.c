
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#include "../src/huffman/arbol.h"
#include "../src/huffman/pool.h"
#include "../src/huffman/formato.h"

#define TAM_PRUEBA (1024U * 1024U)

typedef struct {
    uint64_t siguiente;
    uint64_t total;
    int error;
} ContextoPrueba;

static int guardar_bloque(
    uint64_t indice,
    const unsigned char *datos,
    uint64_t bits_validos,
    void *argumento)
{
    ContextoPrueba *ctx = argumento;

    if (indice != ctx->siguiente ||
        datos == NULL ||
        bits_validos == 0)
    {
        ctx->error = 1;
        return -1;
    }

    ctx->siguiente++;
    return 0;
}

int main(void)
{
    const char *ruta = "editor/tests/prueba_pool.bin";

    int fd = open(ruta, O_CREAT | O_TRUNC | O_RDWR, 0644);

    if (fd < 0)
    {
        perror("open");
        return 1;
    }

    unsigned char buffer[HUFF_BLOCK_SIZE];
    uint64_t frecuencias[HUFFMAN_SIMBOLOS] = {0};

    /*
     * Crear un archivo de 1 MiB con un
     * patron conocido de todos los bytes.
     */
    for (size_t i = 0; i < sizeof(buffer); i++)
    {
        buffer[i] = (unsigned char)(i % 256);
    }

    for (size_t i = 0; i < TAM_PRUEBA; i++)
    {
        frecuencias[i % 256]++;
    }

    for (size_t i = 0;
         i < TAM_PRUEBA / sizeof(buffer);
         i++)
    {
        size_t escritos = 0;

        while (escritos < sizeof(buffer))
        {
            ssize_t n = write(
                fd,
                buffer + escritos,
                sizeof(buffer) - escritos
            );

            if (n <= 0)
            {
                perror("write");
                close(fd);
                return 1;
            }

            escritos += (size_t)n;
        }
    }

    NodoHuffman *arbol =
        huffman_crear_arbol(frecuencias);

    if (arbol == NULL)
    {
        close(fd);
        return 1;
    }

    CodigoHuffman codigos[HUFFMAN_SIMBOLOS];

    if (huffman_generar_codigos(arbol, codigos) != 0)
    {
        huffman_liberar_arbol(arbol);
        close(fd);
        return 1;
    }

    int cantidades[] = {1, 2, 4, 8};

    uint64_t total_bloques =
        TAM_PRUEBA / HUFF_BLOCK_SIZE;

    for (int i = 0; i < 4; i++)
    {
        ContextoPrueba ctx = {
            .siguiente = 0,
            .total = total_bloques,
            .error = 0
        };

        int hilos = cantidades[i];

        int rc = huffman_pool_codificar(
            fd,
            TAM_PRUEBA,
            hilos,
            codigos,
            guardar_bloque,
            &ctx
        );

        if (rc != 0 ||
            ctx.error ||
            ctx.siguiente != ctx.total)
        {
            printf("PRUEBA FALLIDA con %d hilos.\n",
                   hilos);

            huffman_liberar_arbol(arbol);
            close(fd);
            return 1;
        }

        printf("PRUEBA SUPERADA con %d hilos: "
               "%llu bloques ordenados.\n",
               hilos,
               (unsigned long long)ctx.siguiente);
    }

    huffman_liberar_arbol(arbol);
    close(fd);

    printf("TODAS LAS PRUEBAS SUPERADAS.\n");
    return 0;
}
