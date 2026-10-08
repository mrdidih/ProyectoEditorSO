
#define _POSIX_C_SOURCE 200809L

#include "pool.h"
#include "bloque.h"
#include "formato.h"

#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <limits.h>

#define MAX_POOL_HILOS 32

typedef struct ContextoPool ContextoPool;

typedef struct {
    ContextoPool *ctx;
    int id;
    pthread_t hilo;
    BloqueComprimido resultado;
    int listo;
} TrabajadorPool;

struct ContextoPool {
    int fd;
    uint64_t tam_archivo;
    uint64_t total_bloques;
    int num_hilos;
    int error;

    const CodigoHuffman *codigos;

    pthread_mutex_t mutex;
    pthread_cond_t condicion;

    TrabajadorPool *trabajadores;
};

static void marcar_error(ContextoPool *ctx)
{
    pthread_mutex_lock(&ctx->mutex);
    ctx->error = 1;
    pthread_cond_broadcast(&ctx->condicion);
    pthread_mutex_unlock(&ctx->mutex);
}

static int leer_bloque(
    ContextoPool *ctx,
    uint64_t indice,
    unsigned char *buffer,
    size_t cantidad)
{
    uint64_t posicion =
        indice * (uint64_t)HUFF_BLOCK_SIZE;

    size_t leidos = 0;

    while (leidos < cantidad)
    {
        ssize_t n = pread(
            ctx->fd,
            buffer + leidos,
            cantidad - leidos,
            (off_t)(posicion + leidos)
        );

        if (n < 0 && errno == EINTR)
            continue;

        if (n <= 0)
            return -1;

        leidos += (size_t)n;
    }

    return 0;
}

static void *ejecutar_trabajador(void *argumento)
{
    TrabajadorPool *trabajador = argumento;
    ContextoPool *ctx = trabajador->ctx;

    unsigned char *buffer =
        malloc(HUFF_BLOCK_SIZE);

    if (buffer == NULL)
    {
        marcar_error(ctx);
        return NULL;
    }

    for (uint64_t indice = (uint64_t)trabajador->id;
         indice < ctx->total_bloques;
         indice += (uint64_t)ctx->num_hilos)
    {
        pthread_mutex_lock(&ctx->mutex);
        int detener = ctx->error;
        pthread_mutex_unlock(&ctx->mutex);

        if (detener)
            break;

        uint64_t inicio =
            indice * (uint64_t)HUFF_BLOCK_SIZE;

        uint64_t restante =
            ctx->tam_archivo - inicio;

        size_t cantidad = restante < HUFF_BLOCK_SIZE
            ? (size_t)restante
            : HUFF_BLOCK_SIZE;

        if (leer_bloque(ctx, indice,
                        buffer, cantidad) != 0)
        {
            marcar_error(ctx);
            break;
        }

        BloqueComprimido resultado = {0};

        if (huffman_codificar_bloque(
                buffer,
                cantidad,
                ctx->codigos,
                &resultado) != 0)
        {
            marcar_error(ctx);
            break;
        }

        pthread_mutex_lock(&ctx->mutex);

        if (ctx->error)
        {
            pthread_mutex_unlock(&ctx->mutex);
            huffman_liberar_bloque(&resultado);
            break;
        }

        trabajador->resultado = resultado;
        trabajador->listo = 1;

        pthread_cond_broadcast(&ctx->condicion);

        /*
         * Esperar a que el coordinador consuma
         * este resultado antes de producir otro.
         */
        while (trabajador->listo && !ctx->error)
        {
            pthread_cond_wait(
                &ctx->condicion,
                &ctx->mutex
            );
        }

        detener = ctx->error;

        pthread_mutex_unlock(&ctx->mutex);

        if (detener)
            break;
    }

    free(buffer);
    return NULL;
}

int huffman_pool_codificar(
    int fd,
    uint64_t tam_archivo,
    int num_hilos,
    const CodigoHuffman codigos[HUFFMAN_SIMBOLOS],
    HuffmanGuardarBloque guardar_bloque,
    void *contexto)
{
    if (fd < 0 || codigos == NULL ||
        guardar_bloque == NULL ||
        num_hilos < 1 ||
        num_hilos > MAX_POOL_HILOS ||
        tam_archivo > (uint64_t)LLONG_MAX)
    {
        return -1;
    }

    struct stat info;

    if (fstat(fd, &info) != 0 ||
        !S_ISREG(info.st_mode) ||
        info.st_size < 0 ||
        (uint64_t)info.st_size != tam_archivo)
    {
        return -1;
    }

    if (tam_archivo == 0)
        return 0;

    uint64_t total_bloques =
        tam_archivo / HUFF_BLOCK_SIZE +
        (tam_archivo % HUFF_BLOCK_SIZE != 0);

    ContextoPool ctx = {0};

    ctx.fd = fd;
    ctx.tam_archivo = tam_archivo;
    ctx.total_bloques = total_bloques;
    ctx.num_hilos = num_hilos;
    ctx.codigos = codigos;

    if (pthread_mutex_init(&ctx.mutex, NULL) != 0)
        return -1;

    if (pthread_cond_init(&ctx.condicion, NULL) != 0)
    {
        pthread_mutex_destroy(&ctx.mutex);
        return -1;
    }

    ctx.trabajadores = calloc(
        (size_t)num_hilos,
        sizeof(TrabajadorPool)
    );

    if (ctx.trabajadores == NULL)
    {
        pthread_cond_destroy(&ctx.condicion);
        pthread_mutex_destroy(&ctx.mutex);
        return -1;
    }

    int creados = 0;

    for (int i = 0; i < num_hilos; i++)
    {
        TrabajadorPool *t = &ctx.trabajadores[i];

        t->ctx = &ctx;
        t->id = i;

        if (pthread_create(
                &t->hilo,
                NULL,
                ejecutar_trabajador,
                t) != 0)
        {
            marcar_error(&ctx);
            break;
        }

        creados++;
    }

    /*
     * El coordinador consume los resultados
     * en el orden original de los bloques.
     */
    if (creados == num_hilos)
    {
        for (uint64_t indice = 0;
             indice < total_bloques;
             indice++)
        {
            TrabajadorPool *t =
                &ctx.trabajadores[
                    indice % (uint64_t)num_hilos
                ];

            pthread_mutex_lock(&ctx.mutex);

            while (!t->listo && !ctx.error)
            {
                pthread_cond_wait(
                    &ctx.condicion,
                    &ctx.mutex
                );
            }

            if (ctx.error)
            {
                pthread_mutex_unlock(&ctx.mutex);
                break;
            }

            /*
             * El trabajador espera mientras listo=1.
             * Por eso sus datos siguen siendo validos
             * durante la llamada al callback.
             */
            BloqueComprimido *resultado =
                &t->resultado;

            pthread_mutex_unlock(&ctx.mutex);

            int rc = guardar_bloque(
                indice,
                resultado->datos,
                resultado->bits_validos,
                contexto
            );

            pthread_mutex_lock(&ctx.mutex);

            huffman_liberar_bloque(resultado);
            t->listo = 0;

            if (rc != 0)
                ctx.error = 1;

            pthread_cond_broadcast(&ctx.condicion);

            int detener = ctx.error;

            pthread_mutex_unlock(&ctx.mutex);

            if (detener)
                break;
        }
    }

    /*
     * Asegurar que ningun trabajador quede
     * esperando cuando se produce un error.
     */
    pthread_mutex_lock(&ctx.mutex);
    if (creados != num_hilos)
        ctx.error = 1;
    pthread_cond_broadcast(&ctx.condicion);
    pthread_mutex_unlock(&ctx.mutex);

    for (int i = 0; i < creados; i++)
    {
        pthread_join(
            ctx.trabajadores[i].hilo,
            NULL
        );
    }

    for (int i = 0; i < num_hilos; i++)
    {
        huffman_liberar_bloque(
            &ctx.trabajadores[i].resultado
        );
    }

    int resultado_final = ctx.error ? -1 : 0;

    free(ctx.trabajadores);

    pthread_cond_destroy(&ctx.condicion);
    pthread_mutex_destroy(&ctx.mutex);

    return resultado_final;
}
