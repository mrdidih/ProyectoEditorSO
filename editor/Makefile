
CC = gcc

CFLAGS = -Wall -Wextra -Wpedantic -std=c11 \
         -D_POSIX_C_SOURCE=200809L -pthread

TARGET = editor

SRCS = \
	src/main.c \
	src/editor.c \
	src/tareas.c \
	src/huffman/frecuencias.c \
	src/huffman/arbol.c \
	src/huffman/bloque.c \
	src/huffman/pool.c \
	src/huffman/archivo.c \
	src/huffman/compresor.c \
	src/huffman/descompresor.c

OBJ = $(SRCS:.c=.o)
DEPS = $(OBJ:.o=.d)

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

-include $(DEPS)

clean:
	rm -f $(OBJ) $(DEPS) $(TARGET)

.PHONY: all clean
