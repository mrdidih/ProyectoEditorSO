# Editor de Texto CLI - Proyecto de Sistemas Operativos

## 1. Descripción

Este proyecto implementa un editor de texto interactivo en lenguaje C para un ambiente Linux/Unix.

El editor funciona completamente desde línea de comandos y permite abrir, leer, modificar y buscar contenido dentro de archivos de texto.

La manipulación de archivos se realiza mediante llamadas al sistema POSIX de bajo nivel. Para el acceso al archivo no se utilizan funciones de alto nivel como `fopen()`, `fread()`, `fwrite()` o `fclose()`.

El editor fue desarrollado para un equipo de dos integrantes y cumple los requisitos acumulativos correspondientes a esta modalidad.

---

## 2. Comandos implementados

### Abrir o crear un archivo

```text
o [archivo]
