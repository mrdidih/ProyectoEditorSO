
#ifndef FRECUENCIAS_H
#define FRECUENCIAS_H

#include <stdint.h>

#define NUM_SIMBOLOS 256

/*
 * Cuenta las apariciones de cada byte en un archivo
 * mediante varios hilos POSIX.
 *
 * Retorna:
 *  0 si termina correctamente.
 * -1 si ocurre un error.
 */
int contar_frecuencias_paralelo(
    const char *ruta,
    int num_hilos,
    uint64_t frecuencias[NUM_SIMBOLOS]
);

#endif
