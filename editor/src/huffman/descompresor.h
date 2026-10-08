
#ifndef HUFFMAN_DESCOMPRESOR_H
#define HUFFMAN_DESCOMPRESOR_H

/*
 * Descomprime un archivo HUF1.
 *
 * entrada: archivo .huff
 * salida: archivo recuperado
 *
 * Devuelve:
 *   0 si termina correctamente
 *  -1 si ocurre un error
 *
 * No sobrescribe archivos existentes.
 */
int huffman_descomprimir(
    const char *entrada,
    const char *salida
);

#endif
