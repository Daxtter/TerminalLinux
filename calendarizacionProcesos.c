#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
// #include <sys/types.h> // ya incluido por otros headers probablemente
#include <sys/stat.h>
#include <time.h>
#include <string.h>
#include "lista.h"
#include "memoria.h" // para free_memoria_por_nombre

// funcion auxiliar para inicializar arreglos con ceros
void inicializarConCerosInt(int arr[], int tamano) {
    for (int i = 0; i < tamano; i++) {
        arr[i] = 0;
    }
}

// funcion auxiliar para sumar los tiempos de espera (burst times de procesos anteriores)
int sumatoriaBurstHasta(proceso_s *procesos_ready[], int indice_actual, int num_procesos_ready) {
    int sumatoria = 0;
    for (int i = 0; i < indice_actual; i++) {
        if (i < num_procesos_ready && procesos_ready[i] != NULL) { // chequeo de seguridad
            sumatoria += procesos_ready[i]->burstTime;
        }
    }
    return sumatoria;
}


void fcfs(nodo_s **head, char *nombreDelDocumento) {
    int num_ready = obtenerNumeroProcesosReady(head);
    if (num_ready == 0) {
        printf("fcfs: no hay procesos en estado 'ready' para ejecutar.\n");
        return;
    }

    // crear un array de punteros a los procesos 'ready'
    // no se reordena la lista original, fcfs es por orden de llegada (o como esten en la lista filtrada)
    proceso_s *procesos_a_ejecutar[num_ready];
    int k = 0;
    nodo_s *iter = *head;
    // en fcfs, el "orden de llegada" a ready es importante.
    // si no tenemos timestamps, asumimos el orden en la lista de procesos "ready"
    // o podriamos ordenarlos por ID si queremos una politica de llegada consistente.
    // por ahora, tomaremos el orden en que aparecen en la lista `head` que estan `ready`.
    while (iter != NULL && k < num_ready) {
        if (strcmp(iter->pid.estado, "ready") == 0) {
            procesos_a_ejecutar[k++] = &(iter->pid);
        }
        iter = iter->sig;
    }
    // reasegurar que k sea igual a num_ready
    num_ready = k;
    if (num_ready == 0) { // doble chequeo por si acaso
        printf("fcfs: no hay procesos 'ready' (despues de filtrar).\n");
        return;
    }


    printf("\nejecutando FCFS...\n");
    int tiempo_actual = 0;
    float total_turnaround_time = 0;
    float total_waiting_time = 0;

    for (int i = 0; i < num_ready; i++) {
        proceso_s *proc_actual = procesos_a_ejecutar[i];

        // simular llegada (para calculos, aunque aqui todos "llegan" al scheduler al mismo tiempo)
        // el tiempo de espera es el tiempo actual (cuando empieza a ejecutarse)
        // menos su tiempo de llegada a la cola de ready (que es 0 para el primero, o tiempo_actual antes de este)

        // para fcfs, el tiempo de espera del proceso i es la suma de los burst times de los procesos 0 a i-1
        // si todos llegaron al mismo tiempo.
        // si usamos proc_actual->tiempo_llegada_a_ready:
        // proc_actual->tiempo_espera_total = tiempo_actual - proc_actual->tiempo_llegada_a_ready;
        // si asumimos que tiempo_llegada_a_ready es 0 para todos cuando se llama fcfs:
        proc_actual->tiempo_espera_total = tiempo_actual;


        printf("entro el proceso %s (id:%d), con bt %d en tiempo %d.\n",
               proc_actual->nombreProceso, proc_actual->ID, proc_actual->burstTime, tiempo_actual);

        tiempo_actual += proc_actual->burstTime; // el proceso corre por su burst time
        proc_actual->tiempo_finalizacion = tiempo_actual;

        printf("salio el proceso %s (id:%d) en tiempo %d.\n\n",
               proc_actual->nombreProceso, proc_actual->ID, proc_actual->tiempo_finalizacion);

        // calcular metricas para este proceso
        // turnaround time = tiempo finalizacion - tiempo llegada (a ready)
        // int turnaround_proceso = proc_actual->tiempo_finalizacion - proc_actual->tiempo_llegada_a_ready;
        // si asumimos tiempo_llegada_a_ready es 0:
        int turnaround_proceso = proc_actual->tiempo_finalizacion;


        total_turnaround_time += turnaround_proceso;
        total_waiting_time += proc_actual->tiempo_espera_total;

        // actualizar estado del proceso y liberar memoria
        strcpy(proc_actual->estado, "terminated");
        free_memoria_por_nombre(proc_actual->nombreProceso);
        proc_actual->direccion_memoria = -1;
    }

    printf("-------------------------------------------------------------------\n");
    printf("resumen FCFS:\n");
    iter = *head; // iterar sobre la lista original para imprimir en orden original (o como este)
    while(iter != NULL) {
        // solo imprimir los que procesamos y estan ahora terminated
        if(strcmp(iter->pid.estado, "terminated") == 0 && iter->pid.tiempo_finalizacion > 0) {
            // turnaround = finalizacion - llegada_a_ready (asumiendo llegada_a_ready=0 para este calculo simple)
            int turnaround = iter->pid.tiempo_finalizacion; // - iter->pid.tiempo_llegada_a_ready;
            printf("proceso %s (id:%d): bt=%d, t.espera=%d, t.retorno=%d\n",
                   iter->pid.nombreProceso, iter->pid.ID, iter->pid.burstTime,
                   iter->pid.tiempo_espera_total, turnaround);
        }
        iter = iter->sig;
    }


    if (num_ready > 0) {
        printf("\npromedio t.retorno: %.2f\n", total_turnaround_time / num_ready);
        printf("promedio t.espera: %.2f\n", total_waiting_time / num_ready);
    }
    printf("-------------------------------------------------------------------\n");

    escribirEnDocumento(head, nombreDelDocumento);
}


void sjf(nodo_s **head, char *nombreDelDocumento) {
    int num_ready = obtenerNumeroProcesosReady(head);
    if (num_ready == 0) {
        printf("sjf: no hay procesos en estado 'ready' para ejecutar.\n");
        return;
    }

    // crear un array de punteros a los procesos 'ready'
    proceso_s *procesos_a_ejecutar[num_ready];
    int k = 0;
    nodo_s *iter = *head;
    while (iter != NULL && k < num_ready) {
        if (strcmp(iter->pid.estado, "ready") == 0) {
            // para sjf, el burst_time_restante es el burstTime original
            iter->pid.burst_time_restante = iter->pid.burstTime;
            procesos_a_ejecutar[k++] = &(iter->pid);
        }
        iter = iter->sig;
    }
    num_ready = k;
     if (num_ready == 0) {
        printf("sjf: no hay procesos 'ready' (despues de filtrar).\n");
        return;
    }


    // ordenar la lista de procesos_a_ejecutar por burst time (SJF no expropiativo)
    // bubble sort simple para punteros a proceso_s
    for (int i = 0; i < num_ready - 1; i++) {
        for (int j = 0; j < num_ready - i - 1; j++) {
            if (procesos_a_ejecutar[j]->burstTime > procesos_a_ejecutar[j+1]->burstTime) {
                proceso_s *temp = procesos_a_ejecutar[j];
                procesos_a_ejecutar[j] = procesos_a_ejecutar[j+1];
                procesos_a_ejecutar[j+1] = temp;
            }
            // desempate por ID si burst times son iguales (opcional, pero bueno para consistencia)
            else if (procesos_a_ejecutar[j]->burstTime == procesos_a_ejecutar[j+1]->burstTime &&
                     procesos_a_ejecutar[j]->ID > procesos_a_ejecutar[j+1]->ID) {
                proceso_s *temp = procesos_a_ejecutar[j];
                procesos_a_ejecutar[j] = procesos_a_ejecutar[j+1];
                procesos_a_ejecutar[j+1] = temp;
            }
        }
    }
    // la funcion ordenarLista(head) del .c original modificaba la lista global,
    // lo cual podria no ser deseable solo para una corrida de SJF.
    // aqui ordenamos una copia de punteros.

    printf("\nejecutando SJF (no expropiativo)...\n");
    int tiempo_actual = 0;
    float total_turnaround_time = 0;
    float total_waiting_time = 0;

    for (int i = 0; i < num_ready; i++) {
        proceso_s *proc_actual = procesos_a_ejecutar[i];

        // proc_actual->tiempo_espera_total = tiempo_actual - proc_actual->tiempo_llegada_a_ready;
        // asumiendo tiempo_llegada_a_ready es 0 para este calculo simple:
        proc_actual->tiempo_espera_total = tiempo_actual;


        printf("entro el proceso %s (id:%d), con bt %d en tiempo %d.\n",
               proc_actual->nombreProceso, proc_actual->ID, proc_actual->burstTime, tiempo_actual);

        tiempo_actual += proc_actual->burstTime;
        proc_actual->tiempo_finalizacion = tiempo_actual;

        printf("salio el proceso %s (id:%d) en tiempo %d.\n\n",
               proc_actual->nombreProceso, proc_actual->ID, proc_actual->tiempo_finalizacion);

        // int turnaround_proceso = proc_actual->tiempo_finalizacion - proc_actual->tiempo_llegada_a_ready;
        // asumiendo tiempo_llegada_a_ready es 0:
        int turnaround_proceso = proc_actual->tiempo_finalizacion;

        total_turnaround_time += turnaround_proceso;
        total_waiting_time += proc_actual->tiempo_espera_total;

        strcpy(proc_actual->estado, "terminated");
        free_memoria_por_nombre(proc_actual->nombreProceso);
        proc_actual->direccion_memoria = -1;
    }

    printf("-------------------------------------------------------------------\n");
    printf("resumen SJF:\n");
    // para imprimir en el orden de la lista original, pero solo los procesados
    iter = *head;
    while(iter != NULL) {
        if(strcmp(iter->pid.estado, "terminated") == 0 && iter->pid.tiempo_finalizacion > 0 && iter->pid.burstTime > 0) { // bursttime > 0 para evitar los que ya terminaron antes
            // int turnaround = iter->pid.tiempo_finalizacion - iter->pid.tiempo_llegada_a_ready;
             int turnaround = iter->pid.tiempo_finalizacion;
            printf("proceso %s (id:%d): bt=%d, t.espera=%d, t.retorno=%d\n",
                   iter->pid.nombreProceso, iter->pid.ID, iter->pid.burstTime,
                   iter->pid.tiempo_espera_total, turnaround);
        }
        iter = iter->sig;
    }


    if (num_ready > 0) {
        printf("\npromedio t.retorno: %.2f\n", total_turnaround_time / num_ready);
        printf("promedio t.espera: %.2f\n", total_waiting_time / num_ready);
    }
     printf("-------------------------------------------------------------------\n");

    escribirEnDocumento(head, nombreDelDocumento);
}


void roundrobin(nodo_s **head, char *nombreDelDocumento, int quantum) {
    int num_ready_inicial = obtenerNumeroProcesosReady(head);
    if (num_ready_inicial == 0) {
        printf("roundrobin: no hay procesos en estado 'ready' para ejecutar.\n");
        return;
    }

    // crear una cola (usando un array de punteros) para los procesos 'ready'
    proceso_s *cola_ready[num_ready_inicial];
    int k = 0;
    nodo_s *iter = *head;
    while (iter != NULL && k < num_ready_inicial) {
        if (strcmp(iter->pid.estado, "ready") == 0) {
            iter->pid.burst_time_restante = iter->pid.burstTime; // inicializar bt restante
            iter->pid.tiempo_cpu_usado_rr = 0; // inicializar tiempo cpu usado
            iter->pid.tiempo_espera_total = 0; // se acumulara
            // iter->pid.tiempo_llegada_a_ready se asume que ya esta o es 0.
            cola_ready[k++] = &(iter->pid);
        }
        iter = iter->sig;
    }
    int num_procesos_en_cola = k;
    if (num_procesos_en_cola == 0) return;

    printf("\nejecutando Round Robin (quantum=%d)...\n", quantum);
    int tiempo_actual = 0;
    int procesos_terminados = 0;
    int indice_cola = 0;

    // para calculos de promedios
    float total_turnaround_time = 0;
    float total_waiting_time = 0;

    // inicializar tiempo de espera para cada proceso
    // (el tiempo de espera es tiempo_final - llegada_a_ready - burst_original)
    // otra forma: cada vez que un proceso NO esta corriendo y esta en ready, suma a su espera.
    // aqui, vamos a calcularlo al final.

    // para que los procesos mantengan su tiempo de espera acumulado
    // vamos a asumir que tiempo_llegada_a_ready es 0 para todos para este calculo simplificado.
    // si un proceso P_i llega a la cola de listos en t_llegada[i], y termina en t_final[i]
    // Turnaround_i = t_final[i] - t_llegada[i]
    // Espera_i = Turnaround_i - Burst_Original_i
    // En RR, el tiempo_actual global avanza.
    // Cuando un proceso P entra a CPU en tiempo_actual_entrada:
    // Si esta es su primera vez, su espera_hasta_ahora = tiempo_actual_entrada - t_llegada[P]
    // Si ya corrio antes, y su ultima salida de CPU fue en t_ultima_salida[P]
    // espera_adicional = tiempo_actual_entrada - t_ultima_salida[P]
    // Sumar todas estas esperas.

    // Usaremos el tiempo de la ultima vez que el proceso estuvo en la CPU
    // int tiempo_ultima_salida_cpu[num_procesos_en_cola]; // Esta variable no se está usando consistentemente
    // inicializarConCerosInt(tiempo_ultima_salida_cpu, num_procesos_en_cola);
    // for(int i=0; i < num_procesos_en_cola; ++i) {
    // Asumimos que todos llegan a ready en tiempo 0 para este ejemplo
    // o que cola_ready[i]->tiempo_llegada_a_ready ya fue fijado por alloc
    // tiempo_ultima_salida_cpu[i] = cola_ready[i]->tiempo_llegada_a_ready;
    // Para el calculo de espera, mas simple:
    // tiempo_espera = tiempo_finalizacion - burst_time_original (si todos llegan en 0)
    // Para RR, esto se calcula al final.
    // }


    while (procesos_terminados < num_procesos_en_cola) {
        proceso_s *proc_actual = cola_ready[indice_cola];

        if (proc_actual->burst_time_restante > 0) { // si el proceso no ha terminado
            printf("tiempo %d: entra proceso %s (id:%d), bt_restante=%d.\n",
                   tiempo_actual, proc_actual->nombreProceso, proc_actual->ID, proc_actual->burst_time_restante);

            // acumular tiempo de espera desde la ultima vez que salio de cpu o desde su llegada
            // proc_actual->tiempo_espera_total += (tiempo_actual - tiempo_ultima_salida_cpu[indice_cola]); // esto es complejo de hacer bien aqui

            // int tiempo_ejecucion_este_ciclo = 0; // VARIABLE NO USADA - ELIMINADA
            if (proc_actual->burst_time_restante > quantum) {
                // tiempo_ejecucion_este_ciclo = quantum; // VARIABLE NO USADA - ELIMINADA
                proc_actual->burst_time_restante -= quantum;
                tiempo_actual += quantum;
                printf("tiempo %d: sale proceso %s, bt_restante=%d (uso quantum completo).\n\n",
                       tiempo_actual, proc_actual->nombreProceso, proc_actual->burst_time_restante);
            } else {
                // tiempo_ejecucion_este_ciclo = proc_actual->burst_time_restante; // VARIABLE NO USADA - ELIMINADA
                tiempo_actual += proc_actual->burst_time_restante;
                proc_actual->burst_time_restante = 0; // proceso terminado

                proc_actual->tiempo_finalizacion = tiempo_actual;
                // turnaround = finalizacion - llegada_a_ready (asumir llegada_a_ready=0 para este calculo)
                // o si tiempo_llegada_a_ready fue fijado por alloc, usarlo:
                // int turnaround_proceso = proc_actual->tiempo_finalizacion - proc_actual->tiempo_llegada_a_ready;
                int turnaround_proceso = proc_actual->tiempo_finalizacion; // Simplificacion: asume llegada_a_ready = 0
                proc_actual->tiempo_espera_total = turnaround_proceso - proc_actual->burstTime;

                if (proc_actual->tiempo_espera_total < 0) proc_actual->tiempo_espera_total = 0;


                total_turnaround_time += turnaround_proceso;
                total_waiting_time += proc_actual->tiempo_espera_total;

                procesos_terminados++;
                strcpy(proc_actual->estado, "terminated");
                free_memoria_por_nombre(proc_actual->nombreProceso);
                proc_actual->direccion_memoria = -1;
                printf("tiempo %d: TERMINA proceso %s. t.espera=%d, t.retorno=%d.\n\n",
                       tiempo_actual, proc_actual->nombreProceso, proc_actual->tiempo_espera_total, turnaround_proceso);
            }
            // tiempo_ultima_salida_cpu[indice_cola] = tiempo_actual; // Necesitaria manejo mas cuidadoso
        }
        indice_cola = (indice_cola + 1) % num_procesos_en_cola;

        // si todos los procesos restantes tienen bt_restante 0, salir
        // esto es un seguro, la condicion principal es procesos_terminados
        int todos_listos_rr = 1;
        for(int i=0; i < num_procesos_en_cola; ++i) {
            if(cola_ready[i]->burst_time_restante > 0) {
                todos_listos_rr = 0;
                break;
            }
        }
        if(todos_listos_rr && procesos_terminados == num_procesos_en_cola) break;

    }

    printf("-------------------------------------------------------------------\n");
    printf("resumen Round Robin (quantum=%d):\n", quantum);
    // iterar sobre la lista original para imprimir en orden (o como este)
    // los que fueron procesados por RR.
    iter = *head;
    while(iter != NULL) {
        int procesado_en_rr = 0;
        for(int i=0; i < num_procesos_en_cola; ++i) {
            if (cola_ready[i]->ID == iter->pid.ID) {
                 procesado_en_rr = 1;
                 break;
            }
        }

        if(procesado_en_rr && strcmp(iter->pid.estado, "terminated") == 0) {
            // int turnaround = iter->pid.tiempo_finalizacion - iter->pid.tiempo_llegada_a_ready;
            int turnaround = iter->pid.tiempo_finalizacion; // Simplificacion
            printf("proceso %s (id:%d): bt_orig=%d, t.espera=%d, t.retorno=%d\n",
                   iter->pid.nombreProceso, iter->pid.ID, iter->pid.burstTime,
                   iter->pid.tiempo_espera_total, turnaround);
        }
        iter = iter->sig;
    }


    if (num_procesos_en_cola > 0) {
        printf("\npromedio t.retorno: %.2f\n", total_turnaround_time / num_procesos_en_cola);
        printf("promedio t.espera: %.2f\n", total_waiting_time / num_procesos_en_cola);
    }
    printf("-------------------------------------------------------------------\n");

    escribirEnDocumento(head, nombreDelDocumento);
}