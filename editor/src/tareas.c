
#define _POSIX_C_SOURCE 200809L

#include "tareas.h"
#include "huffman/compresor.h"
#include "huffman/descompresor.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * Callback del compresor Huffman.
 * Protege el progreso compartido con mutex.
 */
static void actualizar_progreso(
    int porcentaje,
    void *contexto)
{
    Tarea *tarea = contexto;

    if (porcentaje < 0)
        porcentaje = 0;

    if (porcentaje > 100)
        porcentaje = 100;

    pthread_mutex_lock(&tarea->mutex);

    tarea->progreso = porcentaje;

    pthread_mutex_unlock(&tarea->mutex);
}

/*
 * Trabajador asincrono.
 * Ejecuta la operacion Huffman solicitada.
 */
static void *ejecutar_tarea(void *arg)
{
    Tarea *tarea = arg;

    pthread_mutex_lock(&tarea->mutex);

    tarea->estado = TAREA_EJECUTANDO;

    TipoOperacion operacion = tarea->operacion;
    const char *entrada = tarea->entrada;
    const char *salida = tarea->salida;
    int num_hilos = tarea->num_hilos;

    pthread_mutex_unlock(&tarea->mutex);

    int resultado = -1;

    if (operacion == OPERACION_COMPRIMIR)
    {
        resultado = huffman_comprimir(
            entrada,
            salida,
            num_hilos,
            actualizar_progreso,
            tarea
        );
    }
    else if (operacion == OPERACION_DESCOMPRIMIR)
    {
        resultado = huffman_descomprimir(
            entrada,
            salida
        );
    }

    pthread_mutex_lock(&tarea->mutex);

    tarea->resultado = resultado;

    if (resultado == 0)
    {
        tarea->estado = TAREA_COMPLETADA;
        tarea->progreso = 100;
    }
    else
    {
        tarea->estado = TAREA_ERROR;
    }

    pthread_mutex_unlock(&tarea->mutex);

    return NULL;
}

/*
 * Inicializa el administrador de tareas.
 */
int tarea_inicializar(Tarea *tarea)
{
    if (tarea == NULL)
        return -1;

    memset(tarea, 0, sizeof(*tarea));

    if (pthread_mutex_init(&tarea->mutex, NULL) != 0)
        return -1;

    tarea->estado = TAREA_PENDIENTE;
    tarea->operacion = OPERACION_NINGUNA;
    tarea->resultado = 0;

    return 0;
}

/*
 * Inicia un trabajo nuevo.
 * Solo permite un trabajo activo.
 * Recoge el hilo anterior si ya termino.
 *
 * Esta funcion debe llamarse desde el
 * hilo de comandos del editor.
 */
int tarea_iniciar(
    Tarea *tarea,
    TipoOperacion operacion,
    const char *entrada,
    const char *salida,
    int num_hilos)
{
    if (tarea == NULL ||
        entrada == NULL || salida == NULL ||
        entrada[0] == '\0' || salida[0] == '\0')
        return -1;

    if (operacion != OPERACION_COMPRIMIR &&
        operacion != OPERACION_DESCOMPRIMIR)
        return -1;

    if (operacion == OPERACION_COMPRIMIR &&
        (num_hilos < 1 || num_hilos > 32))
        return -1;

    pthread_mutex_lock(&tarea->mutex);

    if (tarea->estado == TAREA_EJECUTANDO)
    {
        pthread_mutex_unlock(&tarea->mutex);
        return -1;
    }

    /*
     * PENDIENTE con hilo creado representa
     * una tarea que aun no ha arrancado.
     */
    if (tarea->hilo_creado &&
        tarea->estado == TAREA_PENDIENTE)
    {
        pthread_mutex_unlock(&tarea->mutex);
        return -1;
    }

    int recoger = tarea->hilo_creado;
    pthread_t anterior = tarea->hilo;

    pthread_mutex_unlock(&tarea->mutex);

    /*
     * El hilo anterior ya alcanzo un estado
     * terminal. No retenemos el mutex en join.
     */
    if (recoger)
    {
        if (pthread_join(anterior, NULL) != 0)
            return -1;

        tarea->hilo_creado = 0;
    }

    char *nueva_entrada = strdup(entrada);
    char *nueva_salida = strdup(salida);

    if (nueva_entrada == NULL ||
        nueva_salida == NULL)
    {
        free(nueva_entrada);
        free(nueva_salida);
        return -1;
    }

    pthread_mutex_lock(&tarea->mutex);

    free(tarea->entrada);
    free(tarea->salida);

    tarea->entrada = nueva_entrada;
    tarea->salida = nueva_salida;
    tarea->operacion = operacion;
    tarea->num_hilos = num_hilos;
    tarea->progreso = 0;
    tarea->resultado = -1;
    tarea->estado = TAREA_PENDIENTE;

    /*
     * Creamos el hilo mientras mantenemos
     * el mutex. El trabajador espera hasta
     * que terminemos de registrar su estado.
     */
    int rc = pthread_create(
        &tarea->hilo,
        NULL,
        ejecutar_tarea,
        tarea
    );

    if (rc != 0)
    {
        tarea->estado = TAREA_ERROR;
        tarea->hilo_creado = 0;
        pthread_mutex_unlock(&tarea->mutex);
        return -1;
    }

    tarea->hilo_creado = 1;

    pthread_mutex_unlock(&tarea->mutex);

    return 0;
}

/*
 * Consulta segura del estado y progreso.
 */
void tarea_consultar(
    Tarea *tarea,
    EstadoTarea *estado,
    int *progreso)
{
    if (tarea == NULL ||
        estado == NULL ||
        progreso == NULL)
        return;

    pthread_mutex_lock(&tarea->mutex);

    *estado = tarea->estado;
    *progreso = tarea->progreso;

    pthread_mutex_unlock(&tarea->mutex);
}

/*
 * Espera al trabajador y libera memoria.
 * No debe llamarse mientras otro hilo
 * ejecuta tarea_iniciar o tarea_consultar.
 */
void tarea_finalizar(Tarea *tarea)
{
    if (tarea == NULL)
        return;

    if (tarea->hilo_creado)
    {
        pthread_join(tarea->hilo, NULL);
        tarea->hilo_creado = 0;
    }

    free(tarea->entrada);
    free(tarea->salida);

    tarea->entrada = NULL;
    tarea->salida = NULL;

    pthread_mutex_destroy(&tarea->mutex);
}
