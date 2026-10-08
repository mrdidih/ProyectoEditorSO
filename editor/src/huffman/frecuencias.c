
#include "frecuencias.h"

#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>

#define TAM_BLOQUE 65536
#define MAX_HILOS 32

typedef struct {
    int fd;
    off_t tam_archivo;
    off_t siguiente_bloque;

    pthread_mutex_t mutex;
    int error;

    uint64_t frecuencias[NUM_SIMBOLOS];
} ContextoConteo;

typedef struct {
    ContextoConteo *contexto;
    uint64_t frecuencias_locales[NUM_SIMBOLOS];
} TrabajadorConteo;

static void *contar_bloques(void *argumento)
{
    TrabajadorConteo *trabajador = argumento;
    ContextoConteo *ctx = trabajador->contexto;

    unsigned char buffer[TAM_BLOQUE];

    while (1)
    {
        off_t inicio;
        size_t cantidad;

        pthread_mutex_lock(&ctx->mutex);

        if (ctx->error ||
            ctx->siguiente_bloque >= ctx->tam_archivo)
        {
            pthread_mutex_unlock(&ctx->mutex);
            break;
        }

        inicio = ctx->siguiente_bloque;

        off_t restante = ctx->tam_archivo - inicio;

        cantidad = restante < TAM_BLOQUE
            ? (size_t)restante
            : TAM_BLOQUE;

        ctx->siguiente_bloque += (off_t)cantidad;

        pthread_mutex_unlock(&ctx->mutex);

        size_t leidos = 0;

        while (leidos < cantidad)
        {
            ssize_t n = pread(
                ctx->fd,
                buffer + leidos,
                cantidad - leidos,
                inicio + (off_t)leidos
            );

            if (n < 0 && errno == EINTR)
                continue;

            if (n <= 0)
            {
                pthread_mutex_lock(&ctx->mutex);
                ctx->error = 1;
                pthread_mutex_unlock(&ctx->mutex);
                return NULL;
            }

            leidos += (size_t)n;
        }

        for (size_t i = 0; i < cantidad; i++)
        {
            trabajador->frecuencias_locales[buffer[i]]++;
        }
    }

    return NULL;
}

int contar_frecuencias_paralelo(
    const char *ruta,
    int num_hilos,
    uint64_t frecuencias[NUM_SIMBOLOS])
{
    if (!ruta || !frecuencias ||
        num_hilos < 1 || num_hilos > MAX_HILOS)
    {
        return -1;
    }

    int fd = open(ruta, O_RDONLY);

    if (fd == -1)
    {
        perror("open");
        return -1;
    }

    struct stat info;

    if (fstat(fd, &info) == -1 ||
        !S_ISREG(info.st_mode) ||
        info.st_size < 0)
    {
        close(fd);
        return -1;
    }

    ContextoConteo ctx;
    memset(&ctx, 0, sizeof(ctx));

    ctx.fd = fd;
    ctx.tam_archivo = info.st_size;

    if (pthread_mutex_init(&ctx.mutex, NULL) != 0)
    {
        close(fd);
        return -1;
    }

    pthread_t hilos[MAX_HILOS];
    TrabajadorConteo trabajadores[MAX_HILOS];

    memset(trabajadores, 0, sizeof(trabajadores));

    int creados = 0;

    for (int i = 0; i < num_hilos; i++)
    {
        trabajadores[i].contexto = &ctx;

        int rc = pthread_create(
            &hilos[i],
            NULL,
            contar_bloques,
            &trabajadores[i]
        );

        if (rc != 0)
        {
            pthread_mutex_lock(&ctx.mutex);
            ctx.error = 1;
            pthread_mutex_unlock(&ctx.mutex);
            break;
        }

        creados++;
    }

    for (int i = 0; i < creados; i++)
    {
        pthread_join(hilos[i], NULL);
    }

    int error = ctx.error;

    if (!error)
    {
        memset(frecuencias, 0,
               NUM_SIMBOLOS * sizeof(uint64_t));

        for (int i = 0; i < creados; i++)
        {
            for (int j = 0; j < NUM_SIMBOLOS; j++)
            {
                frecuencias[j] +=
                    trabajadores[i].frecuencias_locales[j];
            }
        }
    }

    pthread_mutex_destroy(&ctx.mutex);

    if (close(fd) == -1)
        error = 1;

    return error ? -1 : 0;
}
