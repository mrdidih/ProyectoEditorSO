
#ifndef TAREAS_H
#define TAREAS_H

#include <pthread.h>

typedef enum {
    TAREA_PENDIENTE,
    TAREA_EJECUTANDO,
    TAREA_COMPLETADA,
    TAREA_ERROR
} EstadoTarea;

typedef struct {
    pthread_t hilo;
    pthread_mutex_t mutex;
    EstadoTarea estado;
    int progreso;
    int hilo_creado;
} Tarea;

int tarea_inicializar(Tarea *tarea);
int tarea_iniciar(Tarea *tarea);
void tarea_consultar(Tarea *tarea,
                     EstadoTarea *estado,
                     int *progreso);
void tarea_finalizar(Tarea *tarea);

#endif
