
#define _POSIX_C_SOURCE 200809L

#include "tareas.h"
#include "huffman/compresor.h"
#include "huffman/descompresor.h"

#include <stdlib.h>
#include <string.h>

/*
 * Actualiza de manera segura el porcentaje
 * de avance de una operacion Huffman.
 *
 * Puede ser llamado desde un hilo trabajador.
 */
static void actualizar_progreso(
    int porcentaje,
    void *contexto)
{
    Tarea *tarea = (Tarea *)contexto;

    if (tarea == NULL)
        return;

    if (porcentaje < 0)
        porcentaje = 0;

    if (porcentaje > 100)
        porcentaje = 100;

    pthread_mutex_lock(&tarea->mutex);

    tarea->progreso = porcentaje;

    pthread_mutex_unlock(&tarea->mutex);
}

/*
 * Hilo principal de la tarea asincrona.
 *
 * Ejecuta la compresion o descompresion
 * sin bloquear la consola del editor.
 */
static void *ejecutar_tarea(void *arg)
{
    Tarea *tarea = (Tarea *)arg;

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
        resultado = huffman_descomprimir_con_progreso(
            entrada,
            salida,
            actualizar_progreso,
            tarea
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
 *
 * Debe llamarse una sola vez antes
 * de utilizar la estructura Tarea.
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
    tarea->progreso = 0;
    tarea->resultado = 0;
    tarea->hilo_creado = 0;

    return 0;
}

/*
 * Inicia una nueva operacion Huffman.
 *
 * Solo admite una tarea activa a la vez.
 * Si el trabajo anterior termino, recoge
 * su hilo antes de crear el siguiente.
 *
 * Debe llamarse desde el hilo principal
 * del editor, no simultaneamente desde
 * multiples hilos.
 */
int tarea_iniciar(
    Tarea *tarea,
    TipoOperacion operacion,
    const char *entrada,
    const char *salida,
    int num_hilos)
{
    if (tarea == NULL ||
        entrada == NULL ||
        salida == NULL ||
        entrada[0] == '\0' ||
        salida[0] == '\0')
    {
        return -1;
    }

    if (operacion != OPERACION_COMPRIMIR &&
        operacion != OPERACION_DESCOMPRIMIR)
    {
        return -1;
    }

    if (operacion == OPERACION_COMPRIMIR &&
        (num_hilos < 1 || num_hilos > 32))
    {
        return -1;
    }

    pthread_mutex_lock(&tarea->mutex);

    /*
     * Rechazar una nueva tarea si
     * existe otra pendiente o ejecutandose.
     */
    if (tarea->hilo_creado &&
        (tarea->estado == TAREA_PENDIENTE ||
         tarea->estado == TAREA_EJECUTANDO))
    {
        pthread_mutex_unlock(&tarea->mutex);
        return -1;
    }

    int recoger = tarea->hilo_creado;
    pthread_t hilo_anterior = tarea->hilo;

    pthread_mutex_unlock(&tarea->mutex);

    /*
     * Esperar y recoger el hilo anterior
     * si ya termino su procesamiento.
     */
    if (recoger)
    {
        if (pthread_join(hilo_anterior, NULL) != 0)
            return -1;

        tarea->hilo_creado = 0;
    }

    /*
     * Copiar las rutas para que sigan
     * existiendo durante toda la tarea.
     */
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

    tarea->estado = TAREA_PENDIENTE;
    tarea->progreso = 0;
    tarea->resultado = -1;

    /*
     * Crear el trabajador mientras
     * mantenemos el mutex.
     *
     * El nuevo hilo esperara hasta
     * que registremos hilo_creado.
     */
    if (pthread_create(
            &tarea->hilo,
            NULL,
            ejecutar_tarea,
            tarea) != 0)
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
 * Consulta el estado y el progreso
 * utilizando exclusion mutua.
 */
void tarea_consultar(
    Tarea *tarea,
    EstadoTarea *estado,
    int *progreso)
{
    if (tarea == NULL ||
        estado == NULL ||
        progreso == NULL)
    {
        return;
    }

    pthread_mutex_lock(&tarea->mutex);

    *estado = tarea->estado;
    *progreso = tarea->progreso;

    pthread_mutex_unlock(&tarea->mutex);
}

/*
 * Finaliza el administrador.
 *
 * Espera al trabajador si sigue activo,
 * libera memoria y destruye el mutex.
 *
 * Debe ejecutarse cuando ningun otro
 * hilo vaya a acceder a Tarea.
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
