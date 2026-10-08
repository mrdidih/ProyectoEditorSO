
#ifndef HUFFMAN_DESCOMPRESOR_H
#define HUFFMAN_DESCOMPRESOR_H

typedef void (*HuffmanProgresoDescompresion)(
    int porcentaje,
    void *contexto
);

/*
 * Descompresion tradicional.
 */
int huffman_descomprimir(
    const char *entrada,
    const char *salida
);

/*
 * Descompresion con progreso.
 */
int huffman_descomprimir_con_progreso(
    const char *entrada,
    const char *salida,
    HuffmanProgresoDescompresion progreso,
    void *contexto
);

#endif
