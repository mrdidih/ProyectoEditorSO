
#ifndef COMPRESOR_H
#define COMPRESOR_H

#include <stddef.h>

/*
 * Funcion que permite informar el progreso
 * de una operacion de compresion.
 *
 * progreso: porcentaje entre 0 y 100.
 * contexto: informacion proporcionada por
 * el administrador de tareas.
 */
typedef void (*HuffmanProgreso)(
    int progreso,
    void *contexto
);

/*
 * Comprime un archivo utilizando Huffman
 * y un pool de hilos POSIX.
 *
 * entrada: ruta del archivo original.
 * salida: ruta del archivo .huff.
 * num_hilos: cantidad de hilos trabajadores.
 * progreso: callback opcional.
 * contexto: datos para el callback.
 *
 * Retorna:
 *  0 si la compresion termina correctamente.
 * -1 si ocurre un error.
 */
int huffman_comprimir(
    const char *entrada,
    const char *salida,
    int num_hilos,
    HuffmanProgreso progreso,
    void *contexto
);

#endif
