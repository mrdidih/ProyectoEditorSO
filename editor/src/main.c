#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <stdlib.h>
#include "editor.h"

#define COMMAND_SIZE 256

int main(void)
{
    char command[COMMAND_SIZE];
    int fd = -1;

    printf("Editor SO iniciado correctamente.\n");

    while (1)
    {
        printf("editor> ");
        fflush(stdout);

        if (fgets(command, sizeof(command), stdin) == NULL)
        {
            printf("\nFin de entrada. Cerrando editor.\n");

            if (editor_close_file(fd) == -1)
            {
                return 1;
            }

            break;
        }

        command[strcspn(command, "\n")] = '\0';

        if (strcmp(command, "q") == 0)
        {
            if (editor_close_file(fd) == -1)
            {
                return 1;
            }

            break;
        }

        if (command[0] == 'o' && command[1] == ' ')
        {
            char *filename = command + 2;

            while (*filename == ' ')
            {
                filename++;
            }

            if (*filename == '\0')
            {
                printf("Uso: o [archivo]\n");
                continue;
            }

            int new_fd = editor_open_file(filename);

            if (new_fd == -1)
            {
                continue;
            }

            if (fd != -1)
            {
                if (editor_close_file(fd) == -1)
                {
                    editor_close_file(new_fd);
                    return 1;
                }
            }

            fd = new_fd;

            printf("Archivo abierto correctamente: %s (fd=%d)\n",
                   filename,
                   fd);

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

    if (editor_append_line(fd, text) == -1)
    {
        continue;
    }

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
    {
        continue;
    }

    continue;
}

if (command[0] == 'p' && command[1] == ' ')
{
    int line_number;
    char extra;

    if (sscanf(command + 2, "%d %c", &line_number, &extra) != 1 ||
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

    int result = editor_print_line(fd, line_number);

    if (result == -1)
    {
        continue;
    }

    if (result == 1)
    {
        printf("La linea %d no existe.\n", line_number);
    }

    continue;
}

if (command[0] == 'd' && command[1] == ' ')
{
    int line_number;
    char extra;

    if (sscanf(command + 2, "%d %c", &line_number, &extra) != 1 ||
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

    int result = editor_delete_line(fd, line_number);

    if (result == -1)
    {
        continue;
    }

    if (result == 1)
    {
        printf("La linea %d no existe.\n", line_number);
        continue;
    }

    printf("Linea %d eliminada correctamente.\n", line_number);
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
    {
        args++;
    }

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
    {
        endptr++;
    }

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

    int result = editor_insert_line(
        fd,
        (int)parsed_line,
        endptr
    );

    if (result == -1)
    {
        continue;
    }

    if (result == 1)
    {
        printf("No se puede insertar en la linea %ld.\n", parsed_line);
        continue;
    }

    printf("Linea insertada correctamente en la posicion %ld.\n",
           parsed_line);

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
    {
        word++;
    }

    if (*word == '\0' || strchr(word, ' ') != NULL)
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
    {
        continue;
    }

    if (result == 0)
    {
        printf("No se encontraron coincidencias para: %s\n", word);
    }
    else
    {
        printf("Coincidencias encontradas: %d\n", result);
    }

    continue;
}

if (strcmp(command, "s") == 0)
{
    printf("Uso: s [palabra]\n");
    continue;
}

        printf("Comando no reconocido: %s\n", command);
    }

    printf("Editor cerrado correctamente.\n");

    return 0;
}
