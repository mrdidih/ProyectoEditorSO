# Validación en Linux - Parcial 2

Se validó la implementación de compresión Huffman concurrente en Linux.

## Entorno

- Compilación con GCC
- Estándar C11
- pthreads
- Compilación limpia con `make clean && make`

## Pruebas realizadas

### Compilación
El proyecto compiló correctamente en Linux sin errores.

### Archivo Huffman inválido
Se intentó descomprimir un archivo `.huff` inválido.

Resultado:
- El archivo fue rechazado.
- No se generó una salida falsa.
- No hubo crash.

### Archivo Huffman truncado
Se probó un archivo `.huff` incompleto.

Resultado:
- El archivo fue rechazado correctamente.
- No se generó una salida inválida.

### Modificación externa durante compresión
Se modificó externamente el archivo de entrada mientras Huffman realizaba la compresión.

Resultado:
- La modificación fue detectada.
- La operación terminó en error de forma controlada.
- No ocurrió crash.
- No ocurrió deadlock.

### Integridad de archivo grande
Se comprimió y posteriormente se descomprimió un archivo grande.

Verificación:
- `cmp` no reportó diferencias.
- Los SHA-256 del archivo original y recuperado fueron idénticos.

## Conclusión

La implementación fue validada satisfactoriamente en Linux.
No se encontraron errores que requirieran modificaciones de código.
