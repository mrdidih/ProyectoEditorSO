
#include "tareas.h"
#include <unistd.h>

static void *ejecutar_tarea(void *arg)
{
    Tarea *tarea = (Tarea *)arg;

    pthread_mutex_lock(&tarea->mutex);
    tarea->estado = TAREA_EJECUTANDO;
    pthread_mutex_unlock(&tarea->mutex);

    for (int i = 1; i <= 10; i++)
    {
        sleep(1);

        pthread_mutex_lock(&tarea->mutex);
        tarea->progreso = i * 10;
        pthread_mutex_unlock(&tarea->mutex);
    }

    pthread_mutex_lock(&tarea->mutex);
    tarea->estado = TAREA_COMPLETADA;
    pthread_mutex_unlock(&tarea->mutex);

    return NULL;
}

int tarea_inicializar(Tarea *tarea)
{
    if (pthread_mutex_init(&tarea->mutex, NULL) != 0)
        return -1;

    tarea->estado = TAREA_PENDIENTE;
    tarea->progreso = 0;
    tarea->hilo_creado = 0;

    return 0;
}

int tarea_iniciar(Tarea *tarea)
{
    pthread_mutex_lock(&tarea->mutex);

    if (tarea->hilo_creado)
    {
        pthread_mutex_unlock(&tarea->mutex);
        return -1;
    }

    tarea->progreso = 0;
    tarea->estado = TAREA_PENDIENTE;

    if (pthread_create(&tarea->hilo,
                       NULL,
                       ejecutar_tarea,
                       tarea) != 0)
    {
        tarea->estado = TAREA_ERROR;
        pthread_mutex_unlock(&tarea->mutex);
        return -1;
    }

    tarea->hilo_creado = 1;

    pthread_mutex_unlock(&tarea->mutex);

    return 0;
}

void tarea_consultar(Tarea *tarea,
                     EstadoTarea *estado,
                     int *progreso)
{
    pthread_mutex_lock(&tarea->mutex);

    *estado = tarea->estado;
    *progreso = tarea->progreso;

    pthread_mutex_unlock(&tarea->mutex);
}

void tarea_finalizar(Tarea *tarea)
{
    if (tarea->hilo_creado)
        pthread_join(tarea->hilo, NULL);

    pthread_mutex_destroy(&tarea->mutex);
}
