#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
// #include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <string.h>
#include "lista.h"
int inicializarConCeros(int restaEpicaParaDeterminarBT[], int tamano)
{
    for (size_t i = 0; i < tamano; i++)
    {
        restaEpicaParaDeterminarBT[i] = 0;
    }
}
int sumatoriaDelWaitHastaUnValor(int tiempoWait[], int indice)
{
    int sumatoria = 0;
    for (int i = 0; i <= indice; i++)
    {
        // printf("Valor del tiempo wait %d",tiempoWait[i]);
        sumatoria = tiempoWait[i] + sumatoria;
    }
    // printf("Sumatoria final %d \n",sumatoria);
    return sumatoria;
}
void sjf(nodo_s **head, char *nombreDelDocumento)
{

    ordenarLista(head);
    nodo_s *aux = *(head);
    int indice = 0;
    int tamano = obtenerElTamano(head);
    int tiempoWait[tamano];
    int tiempoRetorno[tamano];
    while (aux != NULL)
    {

        if (strcmp(aux->pid.estado, "ready") == 0)
        {
            if (indice == 0)
            {
                printf("Entro el proceso %s,con BT %d con tiempo de 0.\n", aux->pid.nombreProceso, aux->pid.burstTime);
                tiempoRetorno[indice] = aux->pid.burstTime;
            }
            else
            {
                printf("Entro el proceso %s,con BT %d con tiempo de %d.\n", aux->pid.nombreProceso, aux->pid.burstTime, sumatoriaDelWaitHastaUnValor(tiempoWait, indice - 1));

                tiempoRetorno[indice] = aux->pid.burstTime + sumatoriaDelWaitHastaUnValor(tiempoWait, indice - 1);
            }
            tiempoWait[indice] = aux->pid.burstTime;
            printf("Salió el proceso %s,con BT %d con tiempo de %d.\n\n", aux->pid.nombreProceso, aux->pid.burstTime, sumatoriaDelWaitHastaUnValor(tiempoWait, indice));

            aux->pid.estado[sizeof(aux->pid.estado) - 1] = '\0';
            indice++;
        }
        aux = aux->sig;
    }
    free(aux);
    //  Imprimir el turnAround y el timewait de cada uno
    nodo_s *aux2 = (*head);
    indice = 0;
    printf("\n");
    while (aux2 != NULL)
    {
        if (strcmp(aux2->pid.estado, "ready") == 0)
        {
            if (indice == 0)
            {
                printf("Proceso %s con BT de %d, tiempo de espera: 0, tiempo de retorno %d\n", aux2->pid.nombreProceso, aux2->pid.burstTime, tiempoRetorno[indice]);
            }
            else
            {
                printf("Proceso %s con BT de %d, tiempo de espera: %d, tiempo de retorno %d\n", aux2->pid.nombreProceso, aux2->pid.burstTime, sumatoriaDelWaitHastaUnValor(tiempoWait, indice - 1), tiempoRetorno[indice]);
            }
            strcpy(aux2->pid.estado, "terminated");
            indice++;
        }
        aux2 = aux2->sig;
    }
    free(aux2);
    escribirEnDocumento(head, "guardarProcesos.bin");
}
void roundrobin(nodo_s **head, char *nombreDelDocumento)
{
        // Primero lee para obtener la lista
        int indice = 0;
        int indiceDeS = 0;
        int indiceDeTerminacion = 0;
        int tamano = obtenerElTamano(head);
        int tiempoEspera = 0;
        int tiempoWait[tamano];
        int restaEpicaParaDeterminarFinDelBT[tamano];
        int tiempoRetorno[tamano];
        inicializarConCeros(restaEpicaParaDeterminarFinDelBT, tamano);
        nodo_s *aux = (*head);
        while (indiceDeTerminacion != tamano)
        {
            if (aux == NULL)
                {
                    indice = 0;
                    indiceDeTerminacion = 0;
                    aux = (*head);
                }
            if (strcmp(aux->pid.estado, "ready") == 0)
            {
                
                restaEpicaParaDeterminarFinDelBT[indice] = restaEpicaParaDeterminarFinDelBT[indice] + 10;
    
                // Supooniendo que le quede 1 BT y el QT es de 10 entonces la resta sería de -9
                // Porque para determinar que no es la primera vez que el BT llega menor a 0 entonces
                // Se determina si la resta es menor a -9 es decir -10, porque significaria que se termino por mas de una vez
                if ((aux->pid.burstTime - restaEpicaParaDeterminarFinDelBT[indice]) < -9)
                {
    
                    indiceDeTerminacion++;
                }
                else
                {
    
                    if ((aux->pid.burstTime - restaEpicaParaDeterminarFinDelBT[indice]) > -9 && (aux->pid.burstTime - restaEpicaParaDeterminarFinDelBT[indice]) < 0)
                    {
                        // printf("La resta con el proceso %s Si es menor \n", aux->pid.nombreProceso);
                        // printf("La suma es %d + %d \n", tiempoEspera, aux->pid.burstTime - restaEpicaParaDeterminarFinDelBT[indice]+10);
                        tiempoEspera = tiempoEspera + aux->pid.burstTime - restaEpicaParaDeterminarFinDelBT[indice] + 10;
                    }
                    else
                    {
                        tiempoEspera = tiempoEspera + 10;
                    }
                    printf("Proceso %s entro con BT %d \n", aux->pid.nombreProceso, aux->pid.burstTime - restaEpicaParaDeterminarFinDelBT[indice] + 10);
                    printf("Proceso %s salio con BT %d \n", aux->pid.nombreProceso, aux->pid.burstTime - restaEpicaParaDeterminarFinDelBT[indice]);
                    // Significa que es la primera vez que llega al 0 es decir que ya se quito de la lista porque se le acabo el burst time
                    if ((aux->pid.burstTime - restaEpicaParaDeterminarFinDelBT[indice]) <= 0)
                    {
                        if (tiempoEspera == 0)
                        {
                            tiempoWait[indice] = 0;
                        }
                        else
                        {
                            // printf("twait %d - BT %d", tiempoEspera, aux->pid.burstTime);
                            tiempoWait[indice] = tiempoEspera - aux->pid.burstTime;
                        }
                    }
                }
                indice++;
                // Algo debe pasar para determinar que debe terminar
            }else{
                indiceDeTerminacion++;
            }
            aux = aux->sig;
        }
        nodo_s *aux2 = (*head);
        indice = 0;
        printf("-------------------------------------------------------------------\n");
        while (aux2 != NULL)
        {
            if (strcmp(aux2->pid.estado, "ready") == 0)
            {
                printf("Proceso %s con BT de %d, tiempo de espera: %d, tiempo de retorno %d\n", aux2->pid.nombreProceso, aux2->pid.burstTime, tiempoWait[indice], tiempoWait[indice] + aux2->pid.burstTime);
                indice++;
                strcpy(aux2->pid.estado, "terminated");
            }
            aux2 = aux2->sig;
        }
        free(aux2);
        escribirEnDocumento(head, "guardarProcesos.bin");
     
}
void fcfs(nodo_s **head, char *nombreDelDocumento)
{
    
    int indice = 0;
    int tamano = obtenerElTamano(head);
    int tiempoWait[tamano];
    int tiempoRetorno[tamano];
    nodo_s *aux = (*head);
    while (aux != NULL)
    {
        if (strcmp(aux->pid.estado, "ready") == 0)
        {

            if (indice == 0)
            {
                printf("Entro el proceso %s, con BT %d con tiempo de 0.\n", aux->pid.nombreProceso, aux->pid.burstTime);
                tiempoRetorno[indice] = aux->pid.burstTime;
            }
            else
            {
                printf("Entro el proceso %s,con BT %d con tiempo de %d.\n", aux->pid.nombreProceso, aux->pid.burstTime, sumatoriaDelWaitHastaUnValor(tiempoWait, indice - 1));

                tiempoRetorno[indice] = aux->pid.burstTime + sumatoriaDelWaitHastaUnValor(tiempoWait, indice - 1);
            }
            tiempoWait[indice] = aux->pid.burstTime;
            printf("Salió el proceso %s,con BT %d con tiempo de %d.\n", aux->pid.nombreProceso, aux->pid.burstTime, sumatoriaDelWaitHastaUnValor(tiempoWait, indice));
            indice++;
        }
        aux = aux->sig;
    }
    free(aux);
    //  Imprimir el turnAround y el timewait de cada uno
    nodo_s *aux2 = (*head);
    indice = 0;
    printf("-------------------------------------------------------------------\n");
    while (aux2 != NULL)
    {
        if (strcmp(aux2->pid.estado, "ready") == 0)
        {
            if (indice == 0)
            {
                printf("Proceso %s con BT de %d, tiempo de espera: 0, tiempo de retorno %d\n", aux2->pid.nombreProceso, aux2->pid.burstTime, tiempoRetorno[indice]);
            }
            else
            {
                printf("Proceso %s con BT de %d, tiempo de espera: %d, tiempo de retorno %d\n", aux2->pid.nombreProceso, aux2->pid.burstTime, sumatoriaDelWaitHastaUnValor(tiempoWait, indice - 1), tiempoRetorno[indice]);
            }
            indice++;
            strcpy(aux2->pid.estado, "terminated");
        }
        aux2 = aux2->sig;
    }
    free(aux2);
    escribirEnDocumento(head, "guardarProcesos.bin");
 
}