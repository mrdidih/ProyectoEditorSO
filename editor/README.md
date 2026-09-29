# Editor de Texto CLI - Sistemas Operativos

## 1. Descripción

Este proyecto implementa un editor de texto interactivo desarrollado en lenguaje C para un entorno Linux/Unix.

El editor funciona completamente desde línea de comandos y permite abrir, leer, modificar y buscar contenido dentro de archivos de texto.

La manipulación de archivos se realiza utilizando llamadas al sistema POSIX de bajo nivel como `open()`, `read()`, `write()`, `lseek()`, `ftruncate()` y `close()`, sin utilizar funciones de alto nivel como `fopen()`, `fread()`, `fwrite()` o `fclose()`.

El proyecto fue desarrollado para un equipo de dos integrantes e incluye los comandos base y los requisitos adicionales correspondientes a parejas.

El editor también fue integrado con el Shell utilizado en clase mediante el comando `d_editor`.

---

## 2. Comandos implementados

### Abrir o crear un archivo

```text
o [archivo]
```

Abre un archivo existente o crea uno nuevo si no existe.

Ejemplo:

```text
o notas.txt
```

### Imprimir contenido

```text
p
```

Imprime todo el contenido del archivo.

```text
p [n]
```

Imprime únicamente la línea indicada.

Ejemplo:

```text
p 2
```

### Añadir una línea

```text
a [texto]
```

Añade una nueva línea al final del archivo.

Ejemplo:

```text
a Sistemas Operativos
```

### Eliminar una línea

```text
d [n]
```

Elimina la línea indicada y desplaza el contenido posterior.

Ejemplo:

```text
d 2
```

### Insertar una línea

```text
i [n] [texto]
```

Inserta una nueva línea en la posición indicada.

Ejemplo:

```text
i 2 Nueva linea
```

### Buscar una palabra

```text
s [palabra]
```

Busca una palabra dentro del archivo y muestra las líneas donde aparece.

Ejemplo:

```text
s Linux
```

### Salir

```text
q
```

Cierra correctamente el archivo activo y termina el editor.
