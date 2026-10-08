
#ifndef TAREAS_H
#define TAREAS_H

#include <pthread.h>

typedef enum {
    TAREA_PENDIENTE,
    TAREA_EJECUTANDO,
    TAREA_COMPLETADA,
    TAREA_ERROR
} EstadoTarea;

typedef enum {
    OPERACION_NINGUNA,
    OPERACION_COMPRIMIR,
    OPERACION_DESCOMPRIMIR
} TipoOperacion;

typedef struct {
    pthread_t hilo;
    pthread_mutex_t mutex;

    EstadoTarea estado;
    TipoOperacion operacion;

    int progreso;
    int hilo_creado;
    int resultado;

    char *entrada;
    char *salida;
    int num_hilos;
} Tarea;

int tarea_inicializar(Tarea *tarea);

int tarea_iniciar(
    Tarea *tarea,
    TipoOperacion operacion,
    const char *entrada,
    const char *salida,
    int num_hilos
);

void tarea_consultar(
    Tarea *tarea,
    EstadoTarea *estado,
    int *progreso
);

void tarea_finalizar(Tarea *tarea);

#endif
