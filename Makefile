CC = gcc
CFLAGS = -Wall -Wextra -std=gnu99 -g -D_GNU_SOURCE

ARCHSALIDA = eafitOS

SRCS = main.c cat_datos.c cat_memoria.c cat_monitoreo.c cat_util.c
OBJS = $(SRCS:.c=.o)

EDITOR_DIR = editor

all: editor-build $(ARCHSALIDA)

editor-build:
	$(MAKE) -C $(EDITOR_DIR)

$(ARCHSALIDA): $(OBJS)
	$(CC) $(CFLAGS) -o $(ARCHSALIDA) $(OBJS)

%.o: %.c shell.h
	$(CC) $(CFLAGS) -c $< -o $@

run: all
	./$(ARCHSALIDA)

clean:
	rm -f $(ARCHSALIDA) $(OBJS)
	$(MAKE) -C $(EDITOR_DIR) clean

.PHONY: all run clean editor-build
