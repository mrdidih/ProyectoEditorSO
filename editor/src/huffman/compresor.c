
#include "compresor.h"
#include "arbol.h"
#include "frecuencias.h"
#include "formato.h"

#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>

#define HUFF_MAX_HILOS 32

/*
 * Verifica el archivo original y evita usar
 * la misma ruta de entrada como salida.
 *
 * Retorna el descriptor abierto o -1.
 */
static int preparar_entrada(
    const char *entrada,
    const char *salida)
{
    if (entrada == NULL || salida == NULL ||
        entrada[0] == '\0' || salida[0] == '\0')
    {
        fprintf(stderr, "Rutas invalidas.\n");
        return -1;
    }

    int fd = open(entrada, O_RDONLY);

    if (fd == -1)
    {
        perror("Error al abrir entrada");
        return -1;
    }

    struct stat origen;

    if (fstat(fd, &origen) != 0 ||
        !S_ISREG(origen.st_mode))
    {
        fprintf(stderr, "Entrada no es archivo regular.\n");
        close(fd);
        return -1;
    }

    struct stat destino;

    if (stat(salida, &destino) == 0)
    {
        if (origen.st_dev == destino.st_dev &&
            origen.st_ino == destino.st_ino)
        {
            fprintf(stderr,
                    "Entrada y salida son el mismo archivo.\n");
            close(fd);
            return -1;
        }
    }

    return fd;
}

/*
 * Primera fase del motor Huffman:
 * validacion, conteo y construccion del arbol.
 *
 * La escritura del archivo comprimido se
 * implementara en la siguiente etapa.
 */
int huffman_comprimir(
    const char *entrada,
    const char *salida,
    int num_hilos,
    HuffmanProgreso progreso,
    void *contexto)
{
    if (num_hilos < 1 || num_hilos > HUFF_MAX_HILOS)
    {
        fprintf(stderr, "Numero de hilos invalido.\n");
        return -1;
    }

    int fd = preparar_entrada(entrada, salida);

    if (fd == -1)
        return -1;

    if (progreso != NULL)
        progreso(0, contexto);

    uint64_t frecuencias[NUM_SIMBOLOS] = {0};

    if (contar_frecuencias_paralelo(
            entrada, num_hilos, frecuencias) != 0)
    {
        fprintf(stderr, "Error contando frecuencias.\n");
        close(fd);
        return -1;
    }

    if (progreso != NULL)
        progreso(25, contexto);

    NodoHuffman *raiz = huffman_crear_arbol(frecuencias);

    /*
     * Un archivo vacio no necesita arbol.
     * Para entradas no vacias, NULL es error.
     */
    struct stat info;

    if (fstat(fd, &info) != 0)
    {
        close(fd);
        huffman_liberar_arbol(raiz);
        return -1;
    }

    if (info.st_size > 0 && raiz == NULL)
    {
        fprintf(stderr, "Error construyendo arbol.\n");
        close(fd);
        return -1;
    }

    if (progreso != NULL)
        progreso(40, contexto);

    CodigoHuffman codigos[HUFFMAN_SIMBOLOS];

    if (raiz != NULL &&
        huffman_generar_codigos(raiz, codigos) != 0)
    {
        fprintf(stderr, "Error generando codigos.\n");
        huffman_liberar_arbol(raiz);
        close(fd);
        return -1;
    }

    /*
     * La codificacion y escritura estan pendientes.
     * Todavia NO se genera un archivo .huff.
     */
    huffman_liberar_arbol(raiz);

    if (close(fd) != 0)
        return -1;

    return -1;
}
