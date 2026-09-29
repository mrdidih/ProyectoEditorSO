#include "editor.h"
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int editor_open_file(const char *filename)
{
    int fd = open(filename, O_RDWR | O_CREAT, 0644);

    if (fd == -1)
    {
        perror("open");
        return -1;
    }

    return fd;
}

int editor_close_file(int fd)
{
    if (fd == -1)
    {
        return 0;
    }

    if (close(fd) == -1)
    {
        perror("close");
        return -1;
    }

    return 0;
}

int editor_append_line(int fd, const char *text)
{
    off_t end_position;

    if (fd == -1)
    {
        printf("No hay un archivo abierto.\n");
        return -1;
    }

    end_position = lseek(fd, 0, SEEK_END);

    if (end_position == -1)
    {
        perror("lseek");
        return -1;
    }

    if (end_position > 0)
    {
        char last_char;

        if (lseek(fd, -1, SEEK_END) == -1)
        {
            perror("lseek");
            return -1;
        }

        if (read(fd, &last_char, 1) == -1)
        {
            perror("read");
            return -1;
        }

        if (last_char != '\n')
        {
            if (write(fd, "\n", 1) != 1)
            {
                perror("write");
                return -1;
            }
        }
    }

    if (lseek(fd, 0, SEEK_END) == -1)
    {
        perror("lseek");
        return -1;
    }

    size_t length = strlen(text);

    if (write(fd, text, length) != (ssize_t)length)
    {
        perror("write");
        return -1;
    }

    if (write(fd, "\n", 1) != 1)
    {
        perror("write");
        return -1;
    }

    return 0;
}
int editor_print_file(int fd)
{
    char buffer[256];
    ssize_t bytes_read;

    if (fd == -1)
    {
        printf("No hay un archivo abierto.\n");
        return -1;
    }

    if (lseek(fd, 0, SEEK_SET) == -1)
    {
        perror("lseek");
        return -1;
    }

    while ((bytes_read = read(fd, buffer, sizeof(buffer))) > 0)
    {
        ssize_t total_written = 0;

        while (total_written < bytes_read)
        {
            ssize_t bytes_written = write(
                STDOUT_FILENO,
                buffer + total_written,
                bytes_read - total_written
            );

            if (bytes_written == -1)
            {
                perror("write");
                return -1;
            }

            total_written += bytes_written;
        }
    }

    if (bytes_read == -1)
    {
        perror("read");
        return -1;
    }

    return 0;
}

int editor_print_line(int fd, int line_number)
{
    char current_char;
    int current_line = 1;
    int found = 0;
    ssize_t bytes_read;

    if (fd == -1)
    {
        printf("No hay un archivo abierto.\n");
        return -1;
    }

    if (line_number < 1)
    {
        return 1;
    }

    if (lseek(fd, 0, SEEK_SET) == -1)
    {
        perror("lseek");
        return -1;
    }

    while ((bytes_read = read(fd, &current_char, 1)) > 0)
    {
        if (current_line == line_number)
        {
            found = 1;

            if (write(STDOUT_FILENO, &current_char, 1) != 1)
            {
                perror("write");
                return -1;
            }
        }

        if (current_char == '\n')
        {
            if (current_line == line_number)
            {
                return 0;
            }

            current_line++;
        }
    }

    if (bytes_read == -1)
    {
        perror("read");
        return -1;
    }

    if (found)
    {
        if (write(STDOUT_FILENO, "\n", 1) != 1)
        {
            perror("write");
            return -1;
        }

        return 0;
    }

    return 1;
}
int editor_delete_line(int fd, int line_number)
{
    off_t file_size;
    off_t line_start = 0;
    off_t line_end = -1;
    off_t current_position;
    off_t tail_size;
    off_t new_size;

    int current_line = 1;
    char current_char;
    char *buffer = NULL;

    ssize_t bytes_read;

    if (fd == -1)
    {
        printf("No hay un archivo abierto.\n");
        return -1;
    }

    if (line_number < 1)
    {
        return 1;
    }

    file_size = lseek(fd, 0, SEEK_END);

    if (file_size == -1)
    {
        perror("lseek");
        return -1;
    }

    if (file_size == 0)
    {
        return 1;
    }

    if (lseek(fd, 0, SEEK_SET) == -1)
    {
        perror("lseek");
        return -1;
    }

    while ((bytes_read = read(fd, &current_char, 1)) > 0)
    {
        current_position = lseek(fd, 0, SEEK_CUR);

        if (current_position == -1)
        {
            perror("lseek");
            return -1;
        }

        if (current_line == line_number && current_char == '\n')
        {
            line_end = current_position;
            break;
        }

        if (current_char == '\n')
        {
            current_line++;
            line_start = current_position;
        }
    }

    if (bytes_read == -1)
    {
        perror("read");
        return -1;
    }

    if (current_line == line_number &&
        line_start < file_size &&
        line_end == -1)
    {
        line_end = file_size;
    }

    if (line_end == -1)
    {
        return 1;
    }

    tail_size = file_size - line_end;

    if (tail_size > 0)
    {
        buffer = malloc((size_t)tail_size);

        if (buffer == NULL)
        {
            perror("malloc");
            return -1;
        }

        if (lseek(fd, line_end, SEEK_SET) == -1)
        {
            perror("lseek");
            free(buffer);
            return -1;
        }

        off_t total_read = 0;

        while (total_read < tail_size)
        {
            ssize_t result = read(
                fd,
                buffer + total_read,
                (size_t)(tail_size - total_read)
            );

            if (result == -1)
            {
                perror("read");
                free(buffer);
                return -1;
            }

            if (result == 0)
            {
                break;
            }

            total_read += result;
        }

        if (lseek(fd, line_start, SEEK_SET) == -1)
        {
            perror("lseek");
            free(buffer);
            return -1;
        }

        off_t total_written = 0;

        while (total_written < total_read)
        {
            ssize_t result = write(
                fd,
                buffer + total_written,
                (size_t)(total_read - total_written)
            );

            if (result == -1)
            {
                perror("write");
                free(buffer);
                return -1;
            }

            total_written += result;
        }
    }

    new_size = file_size - (line_end - line_start);

    if (ftruncate(fd, new_size) == -1)
    {
        perror("ftruncate");
        free(buffer);
        return -1;
    }

    free(buffer);

    return 0;
}

int editor_insert_line(int fd, int line_number, const char *text)
{
    off_t file_size;
    off_t insert_position = -1;
    off_t tail_size;

    int current_line = 1;
    int needs_separator = 0;

    char current_char;
    char *buffer = NULL;

    ssize_t bytes_read;

    if (fd == -1)
    {
        printf("No hay un archivo abierto.\n");
        return -1;
    }

    if (line_number < 1)
    {
        return 1;
    }

    file_size = lseek(fd, 0, SEEK_END);

    if (file_size == -1)
    {
        perror("lseek");
        return -1;
    }

    if (file_size == 0)
    {
        if (line_number != 1)
        {
            return 1;
        }

        insert_position = 0;
    }
    else if (line_number == 1)
    {
        insert_position = 0;
    }
    else
    {
        if (lseek(fd, 0, SEEK_SET) == -1)
        {
            perror("lseek");
            return -1;
        }

        while ((bytes_read = read(fd, &current_char, 1)) > 0)
        {
            if (current_char == '\n')
            {
                current_line++;

                if (current_line == line_number)
                {
                    insert_position = lseek(fd, 0, SEEK_CUR);

                    if (insert_position == -1)
                    {
                        perror("lseek");
                        return -1;
                    }

                    break;
                }
            }
        }

        if (bytes_read == -1)
        {
            perror("read");
            return -1;
        }

        if (insert_position == -1)
        {
            if (line_number == current_line + 1)
            {
                insert_position = file_size;
                needs_separator = 1;
            }
            else
            {
                return 1;
            }
        }
    }

    tail_size = file_size - insert_position;

    if (tail_size > 0)
    {
        buffer = malloc((size_t)tail_size);

        if (buffer == NULL)
        {
            perror("malloc");
            return -1;
        }

        if (lseek(fd, insert_position, SEEK_SET) == -1)
        {
            perror("lseek");
            free(buffer);
            return -1;
        }

        off_t total_read = 0;

        while (total_read < tail_size)
        {
            ssize_t result = read(
                fd,
                buffer + total_read,
                (size_t)(tail_size - total_read)
            );

            if (result == -1)
            {
                perror("read");
                free(buffer);
                return -1;
            }

            if (result == 0)
            {
                break;
            }

            total_read += result;
        }

        tail_size = total_read;
    }

    if (lseek(fd, insert_position, SEEK_SET) == -1)
    {
        perror("lseek");
        free(buffer);
        return -1;
    }

    if (needs_separator)
    {
        if (write(fd, "\n", 1) != 1)
        {
            perror("write");
            free(buffer);
            return -1;
        }
    }

    size_t text_length = strlen(text);

    if (write(fd, text, text_length) != (ssize_t)text_length)
    {
        perror("write");
        free(buffer);
        return -1;
    }

    if (write(fd, "\n", 1) != 1)
    {
        perror("write");
        free(buffer);
        return -1;
    }

    if (tail_size > 0)
    {
        off_t total_written = 0;

        while (total_written < tail_size)
        {
            ssize_t result = write(
                fd,
                buffer + total_written,
                (size_t)(tail_size - total_written)
            );

            if (result == -1)
            {
                perror("write");
                free(buffer);
                return -1;
            }

            total_written += result;
        }
    }

    free(buffer);

    return 0;
}

int editor_search_word(int fd, const char *word)
{
    off_t file_size;
    char *buffer;
    off_t total_read = 0;
    int line_number = 1;
    int matches = 0;

    if (fd == -1)
    {
        printf("No hay un archivo abierto.\n");
        return -1;
    }

    file_size = lseek(fd, 0, SEEK_END);

    if (file_size == -1)
    {
        perror("lseek");
        return -1;
    }

    if (file_size == 0)
    {
        return 0;
    }

    buffer = malloc((size_t)file_size + 1);

    if (buffer == NULL)
    {
        perror("malloc");
        return -1;
    }

    if (lseek(fd, 0, SEEK_SET) == -1)
    {
        perror("lseek");
        free(buffer);
        return -1;
    }

    while (total_read < file_size)
    {
        ssize_t result = read(
            fd,
            buffer + total_read,
            (size_t)(file_size - total_read)
        );

        if (result == -1)
        {
            perror("read");
            free(buffer);
            return -1;
        }

        if (result == 0)
        {
            break;
        }

        total_read += result;
    }

    buffer[total_read] = '\0';

    char *line_start = buffer;

    for (off_t i = 0; i <= total_read; i++)
    {
        if (buffer[i] == '\n' || buffer[i] == '\0')
        {
            char saved_char = buffer[i];
            buffer[i] = '\0';

            if (strstr(line_start, word) != NULL)
            {
                printf("Linea %d: %s\n", line_number, line_start);
                matches++;
            }

            buffer[i] = saved_char;

            if (saved_char == '\0')
            {
                break;
            }

            line_start = buffer + i + 1;
            line_number++;
        }
    }

    free(buffer);

    return matches;
}
