
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

void *trabajador(void *arg) {
    int id = *(int *)arg;

    printf("Hilo %d: iniciando trabajo\n", id);
    sleep(2);
    printf("Hilo %d: trabajo terminado\n", id);

    return NULL;
}

int main(void) {
    pthread_t hilos[3];
    int ids[3] = {1, 2, 3};

    for (int i = 0; i < 3; i++) {
        if (pthread_create(&hilos[i], NULL, trabajador, &ids[i]) != 0) {
            fprintf(stderr, "Error al crear hilo %d\n", i + 1);
            return 1;
        }
    }

    for (int i = 0; i < 3; i++) {
        pthread_join(hilos[i], NULL);
    }

    printf("Todos los hilos terminaron.\n");
    return 0;
}
