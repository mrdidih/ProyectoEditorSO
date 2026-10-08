
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "editor.h"
#include "tareas.h"

#define COMMAND_SIZE 256

static const char *nombre_estado(EstadoTarea estado)
{
    switch (estado)
    {
        case TAREA_PENDIENTE:
            return "Pendiente";

        case TAREA_EJECUTANDO:
            return "Ejecutando";

        case TAREA_COMPLETADA:
            return "Completada";

        case TAREA_ERROR:
            return "Error";

        default:
            return "Desconocido";
    }
}

/*
 * Comprueba si existe una tarea activa.
 * Solo se llama desde el hilo principal.
 */
static int tarea_ocupada(Tarea *tarea)
{
    EstadoTarea estado;
    int progreso;

    tarea_consultar(tarea, &estado, &progreso);

   return tarea->hilo_creado &&
       (estado == TAREA_PENDIENTE ||
        estado == TAREA_EJECUTANDO);
}

static int verificar_edicion(Tarea *tarea)
{
    if (tarea_ocupada(tarea))
    {
        printf("Hay una tarea Huffman activa.\n");
        printf("La edicion esta temporalmente bloqueada.\n");
        printf("Consulta el progreso con: estado\n");
        return -1;
    }

    return 0;
}

/*
 * Recibe dos rutas separadas por espacios.
 * En esta primera version no admite rutas
 * que contengan espacios.
 */
static int leer_rutas(
    const char *argumentos,
    char entrada[COMMAND_SIZE],
    char salida[COMMAND_SIZE])
{
    char extra;

    return sscanf(
        argumentos,
        "%255s %255s %c",
        entrada,
        salida,
        &extra
    ) == 2 ? 0 : -1;
}

int main(void)
{
    char command[COMMAND_SIZE];
    int fd = -1;
    int resultado = 0;

    Tarea tarea;

    if (tarea_inicializar(&tarea) != 0)
    {
        fprintf(stderr,
                "Error al inicializar las tareas.\n");
        return 1;
    }

    printf("Editor SO iniciado correctamente.\n");
    printf("Parcial 2 - Huffman concurrente\n");
    printf("Comandos: o, a, p, d, i, s, q\n");
    printf("Huffman: comprimir, descomprimir, estado\n");

    while (1)
    {
        printf("editor> ");
        fflush(stdout);

        if (fgets(command, sizeof(command), stdin) == NULL)
        {
            printf("\nFin de entrada.\n");
            break;
        }

        command[strcspn(command, "\n")] = '\0';

        /*
         * ====================================
         * COMANDOS HUFFMAN - PARCIAL 2
         * ====================================
         */

        if (strncmp(command, "comprimir ", 9) == 0)
        {
            char entrada[COMMAND_SIZE];
            char salida[COMMAND_SIZE];

            if (leer_rutas(
                    command + 9,
                    entrada,
                    salida) != 0)
            {
                printf("Uso: comprimir <entrada> <salida.huff>\n");
                continue;
            }

            if (tarea_iniciar(
                    &tarea,
                    OPERACION_COMPRIMIR,
                    entrada,
                    salida,
                    4) == 0)
            {
                printf("Compresion iniciada en segundo plano.\n");
                printf("Entrada: %s\n", entrada);
                printf("Salida: %s\n", salida);
                printf("Hilos de trabajo: 4\n");
            }
            else
            {
                printf("No se pudo iniciar la compresion.\n");
                printf("Verifica que no haya otra tarea activa.\n");
            }

            continue;
        }

        if (strcmp(command, "comprimir") == 0)
        {
            printf("Uso: comprimir <entrada> <salida.huff>\n");
            continue;
        }

        if (strncmp(command, "descomprimir ", 12) == 0)
        {
            char entrada[COMMAND_SIZE];
            char salida[COMMAND_SIZE];

            if (leer_rutas(
                    command + 12,
                    entrada,
                    salida) != 0)
            {
                printf("Uso: descomprimir <entrada.huff> <salida>\n");
                continue;
            }

            if (tarea_iniciar(
                    &tarea,
                    OPERACION_DESCOMPRIMIR,
                    entrada,
                    salida,
                    1) == 0)
            {
                printf("Descompresion iniciada en segundo plano.\n");
                printf("Entrada: %s\n", entrada);
                printf("Salida: %s\n", salida);
            }
            else
            {
                printf("No se pudo iniciar la descompresion.\n");
                printf("Verifica que no haya otra tarea activa.\n");
            }

            continue;
        }

        if (strcmp(command, "descomprimir") == 0)
        {
            printf("Uso: descomprimir <entrada.huff> <salida>\n");
            continue;
        }

        if (strcmp(command, "estado") == 0)
        {
            EstadoTarea estado;
            int progreso;

            tarea_consultar(
                &tarea,
                &estado,
                &progreso
            );

            printf(
                "Estado: %s\n",
                nombre_estado(estado)
            );

            printf("Progreso: %d%%\n", progreso);

            if (estado == TAREA_COMPLETADA)
                printf("Operacion finalizada correctamente.\n");

            if (estado == TAREA_ERROR)
                printf("La operacion finalizo con un error.\n");

            continue;
        }

        /*
         * ====================================
         * COMANDOS DEL EDITOR - PARCIAL 1
         * ====================================
         */

        if (strcmp(command, "q") == 0)
        {
            break;
        }

        if (command[0] == 'o' && command[1] == ' ')
        {
            char *filename = command + 2;

            while (*filename == ' ')
                filename++;

            if (*filename == '\0')
            {
                printf("Uso: o [archivo]\n");
                continue;
            }

            if (verificar_edicion(&tarea) != 0)
                continue;

            int new_fd = editor_open_file(filename);

            if (new_fd == -1)
                continue;

            if (fd != -1)
            {
                if (editor_close_file(fd) == -1)
                {
                    editor_close_file(new_fd);
                    resultado = 1;
                    break;
                }
            }

            fd = new_fd;

            printf(
                "Archivo abierto correctamente: %s (fd=%d)\n",
                filename,
                fd
            );

            continue;
        }

        if (strcmp(command, "o") == 0)
        {
            printf("Uso: o [archivo]\n");
            continue;
        }

        if (command[0] == 'a' && command[1] == ' ')
        {
            char *text = command + 2;

            if (*text == '\0')
            {
                printf("Uso: a [texto]\n");
                continue;
            }

            if (fd == -1)
            {
                printf("Primero debe abrir un archivo con: o [archivo]\n");
                continue;
            }

            if (verificar_edicion(&tarea) != 0)
                continue;

            if (editor_append_line(fd, text) == -1)
                continue;

            printf("Linea agregada correctamente.\n");
            continue;
        }

        if (strcmp(command, "a") == 0)
        {
            printf("Uso: a [texto]\n");
            continue;
        }

        if (strcmp(command, "p") == 0)
        {
            if (fd == -1)
            {
                printf("Primero debe abrir un archivo con: o [archivo]\n");
                continue;
            }

            if (editor_print_file(fd) == -1)
                continue;

            continue;
        }

        if (command[0] == 'p' && command[1] == ' ')
        {
            int line_number;
            char extra;

            if (sscanf(
                    command + 2,
                    "%d %c",
                    &line_number,
                    &extra) != 1 ||
                line_number < 1)
            {
                printf("Uso: p [n]\n");
                continue;
            }

            if (fd == -1)
            {
                printf("Primero debe abrir un archivo con: o [archivo]\n");
                continue;
            }

            if (editor_print_line(fd, line_number) == -1)
                continue;

            continue;
        }

        if (command[0] == 'd' && command[1] == ' ')
        {
            int line_number;
            char extra;

            if (sscanf(
                    command + 2,
                    "%d %c",
                    &line_number,
                    &extra) != 1 ||
                line_number < 1)
            {
                printf("Uso: d [n]\n");
                continue;
            }

            if (fd == -1)
            {
                printf("Primero debe abrir un archivo con: o [archivo]\n");
                continue;
            }

            if (verificar_edicion(&tarea) != 0)
                continue;

            int result = editor_delete_line(
                fd,
                line_number
            );

            if (result == -1)
                continue;

            if (result == 1)
            {
                printf(
                    "La linea %d no existe.\n",
                    line_number
                );
                continue;
            }

            printf(
                "Linea %d eliminada correctamente.\n",
                line_number
            );

            continue;
        }

        if (strcmp(command, "d") == 0)
        {
            printf("Uso: d [n]\n");
            continue;
        }

        if (command[0] == 'i' && command[1] == ' ')
        {
            char *args = command + 2;
            char *endptr;
            long parsed_line;

            while (*args == ' ')
                args++;

            parsed_line = strtol(args, &endptr, 10);

            if (args == endptr ||
                parsed_line < 1 ||
                parsed_line > INT_MAX ||
                *endptr != ' ')
            {
                printf("Uso: i [n] [texto]\n");
                continue;
            }

            while (*endptr == ' ')
                endptr++;

            if (*endptr == '\0')
            {
                printf("Uso: i [n] [texto]\n");
                continue;
            }

            if (fd == -1)
            {
                printf("Primero debe abrir un archivo con: o [archivo]\n");
                continue;
            }

            if (verificar_edicion(&tarea) != 0)
                continue;

            int result = editor_insert_line(
                fd,
                (int)parsed_line,
                endptr
            );

            if (result == -1)
                continue;

            if (result == 1)
            {
                printf(
                    "No se puede insertar en la linea %ld.\n",
                    parsed_line
                );
                continue;
            }

            printf(
                "Linea insertada correctamente en la posicion %ld.\n",
                parsed_line
            );

            continue;
        }

        if (strcmp(command, "i") == 0)
        {
            printf("Uso: i [n] [texto]\n");
            continue;
        }

        if (command[0] == 's' && command[1] == ' ')
        {
            char *word = command + 2;

            while (*word == ' ')
                word++;

            if (*word == '\0' ||
                strchr(word, ' ') != NULL)
            {
                printf("Uso: s [palabra]\n");
                continue;
            }

            if (fd == -1)
            {
                printf("Primero debe abrir un archivo con: o [archivo]\n");
                continue;
            }

            int result = editor_search_word(fd, word);

            if (result == -1)
                continue;

            if (result == 0)
            {
                printf(
                    "No se encontraron coincidencias para: %s\n",
                    word
                );
            }
            else
            {
                printf(
                    "Coincidencias encontradas: %d\n",
                    result
                );
            }

            continue;
        }

        if (strcmp(command, "s") == 0)
        {
            printf("Uso: s [palabra]\n");
            continue;
        }

        if (strcmp(command, "iniciar") == 0)
        {
            printf(
                "El comando iniciar fue reemplazado.\n"
                "Usa comprimir o descomprimir.\n"
            );
            continue;
        }

        printf(
            "Comando no reconocido: %s\n",
            command
        );
    }

    /*
     * Esperar a que termine el trabajador
     * antes de liberar sus recursos.
     */
    tarea_finalizar(&tarea);

    if (fd != -1 &&
        editor_close_file(fd) == -1)
    {
        resultado = 1;
    }

    printf("Editor cerrado correctamente.\n");

    return resultado;
}
