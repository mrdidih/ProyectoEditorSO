
CC = gcc
CFLAGS = -Wall -Wextra -pedantic -pthread

TARGET = editor

OBJ = src/main.o src/editor.o src/tareas.o

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $(TARGET)

src/main.o: src/main.c src/editor.h src/tareas.h
	$(CC) $(CFLAGS) -c src/main.c -o src/main.o

src/editor.o: src/editor.c src/editor.h
	$(CC) $(CFLAGS) -c src/editor.c -o src/editor.o

src/tareas.o: src/tareas.c src/tareas.h
	$(CC) $(CFLAGS) -c src/tareas.c -o src/tareas.o

clean:
	rm -f $(OBJ) $(TARGET)

.PHONY: all clean
