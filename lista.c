#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include "lista.h"
void insertarEnLista(proceso_s mi_pid, nodo_s **head)
{

    nodo_s *aux = NULL; // Comenzando para luego saber si se debe utilizar
    if ((*head) == NULL)
    {
        if (mi_pid.ID == -99)
        {
            mi_pid.ID = 1;
        }
        (*head) = (nodo_s *)malloc(sizeof(nodo_s));
        (*head)->pid = mi_pid;
        (*head)->sig = NULL;
    }
    else
    {
        if (mi_pid.ID == -99)
        {
            mi_pid.ID = (*head)->pid.ID + 1;
        }
        aux = (nodo_s *)malloc(sizeof(nodo_s));
        aux->pid = mi_pid;
        aux->sig = (*head);
        (*head) = aux;
    }
}
void imprimirLaLista(nodo_s **head)
{
    int numero = 0;
    if ((*head) != NULL)
    {
        nodo_s *aux = (*head);
        while (aux != NULL)
        {
            numero++;
            printf("No. %d nombre :%s , Burst time:%d  \n", aux->pid.ID, aux->pid.nombreProceso, aux->pid.burstTime);
            aux = aux->sig;
        };
    }
    else
    {
        printf("Nada que mostrar :(");
    }
}
// Aqui no deberia estar el de ordenar pero queria ordenarla
void ordenarLista(nodo_s **head)
{
    // Se hara el selectionShort
    int contadorPrimerCiclo = 0;
    int contadorSegundoCiclo = 0;
    int contadorDelIntercambiable = 0;
    nodo_s *aux = (*head);
    nodo_s *auxDespues = (nodo_s *)malloc(sizeof(nodo_s));
    nodo_s *minimo = (nodo_s *)malloc(sizeof(nodo_s));
    while (aux->sig != NULL)
    {
        // Debug
        // printf("Impresion de la lista: \n");
        // imprimirLaLista(head);
        // printf("Nodo Actual %s \n", aux->pid.nombreProceso);
        // printf("Valor del contador Intercambiable(2) %d, Valor del contador del primer ciclo %d \n", contadorDelIntercambiable, contadorPrimerCiclo);
        minimo = aux;
        auxDespues = aux->sig;
        contadorSegundoCiclo = contadorPrimerCiclo;
        contadorDelIntercambiable = -1;
        while (auxDespues != NULL)
        {
            contadorSegundoCiclo++;
            // Si minimo tiene mayor busrt time o si son iguales y minimo llego despues intercambia
            if (minimo->pid.burstTime > auxDespues->pid.burstTime || (minimo->pid.burstTime == auxDespues->pid.burstTime && minimo->pid.ID > auxDespues->pid.ID))
            {
                minimo = auxDespues;
                contadorDelIntercambiable = contadorSegundoCiclo;
            }

            auxDespues = auxDespues->sig;
        }

        // Ahora comienza el intercambio
        proceso_s procesoi = aux->pid;
        proceso_s procesoMinimo = minimo->pid;
        if (contadorDelIntercambiable != -1)
        {
            eliminarElementoEnPosicion(head, contadorPrimerCiclo);
            insertarElementoEnPosicion(head, contadorPrimerCiclo, procesoMinimo);
            eliminarElementoEnPosicion(head, contadorDelIntercambiable);
            insertarElementoEnPosicion(head, contadorDelIntercambiable, procesoi);
        }
        //
        contadorPrimerCiclo++;
        aux = aux->sig;
    }
}
void insertarElementoEnPosicion(nodo_s **head, int pos, proceso_s mi_pid)
{
    if (head != NULL || pos >= 0)
    {
        nodo_s *aux = (*head);
        nodo_s *nodoAIngresar = NULL;
        int posInterna = 1;
        if (pos == 0)
        {
            insertarEnLista(mi_pid, head);
        }
        else
        {
            while (aux != NULL)
            {
                if (posInterna == pos)
                {
                    if (aux->sig == NULL)
                    {
                        insertarAlFinal(head, mi_pid);
                    }
                    else
                    {
                        nodoAIngresar = (nodo_s *)malloc(sizeof(nodo_s));
                        nodoAIngresar->pid = mi_pid;
                        nodoAIngresar->sig = aux->sig;
                        aux->sig = nodoAIngresar;
                        break;
                    }
                }
                posInterna++;
                aux = aux->sig;
            }
        }
    }
}
void obtenerElPrimerElemento(nodo_s **head);
void insertarAlFinal(nodo_s **head, proceso_s proceso)
{

    if ((*head) == NULL)
    { // Si no hay nada pues que lo meta el inicio
        insertarEnLista(proceso, head);
    }
    else
    {

        nodo_s *aux = (*head);
        while (aux->sig != NULL)
        {
            aux = aux->sig;
        }
        // Ahora estamos en el ultimo elemento
        nodo_s *nodo = NULL;
        nodo = (nodo_s *)malloc(sizeof(nodo_s));
        if (proceso.ID == -99)
        {
            proceso.ID = aux->pid.ID + 1;
        }
        nodo->pid = proceso;
        nodo->sig = NULL;

        aux->sig = nodo;
    }
}

void eliminarElInicio(nodo_s **head)
{
    {
        nodo_s *aux;
        nodo_s *aux2;
        if ((*head) != NULL) // Si la lista existe
        {
            aux2 = (nodo_s *)malloc(sizeof(nodo_s));
            aux2 = (*head);
            aux = (nodo_s *)malloc(sizeof(nodo_s));
            aux = (*head)->sig;
            (*head) = aux;
            free(aux2);
        }
    }
}
void eliminarElementoEnPosicion(nodo_s **head, int pos)
{
    if (head != NULL || pos >= 0)
    {
        nodo_s *aux = (*head)->sig;
        nodo_s *auxAnt = (*head);
        int posInterna = 1;
        if (pos == 0)
        {
            eliminarElInicio(head);
        }
        else
        {
            while (aux != NULL)
            {
                if (posInterna == pos)
                {
                    if (aux->sig != NULL)
                    {
                        auxAnt->sig = aux->sig;
                    }
                    else
                    {
                        auxAnt->sig = NULL;
                    }
                    free(aux);
                }
                posInterna++;
                auxAnt = aux;
                aux = aux->sig;
            }
        }
    }
}
void eliminarAlFinal(nodo_s **head)
{
    if (head != NULL)
    {
        nodo_s *aux = (*head);
        nodo_s *auxAnt = (nodo_s *)malloc(sizeof(nodo_s));
        while (aux->sig != NULL)
        {
            auxAnt = aux;
            aux = aux->sig;
        }
        if (auxAnt == NULL)
        {
            eliminarElInicio(head);
        }
        else
        {
            auxAnt->sig = NULL;
            free(aux);
        }
    }
}

void leerDeDocumento(nodo_s **head, char *nombreDelDocumento)
{
    proceso_s nuevoProceso;
    int fd = open(nombreDelDocumento, O_RDONLY);
    int final; // end of file
    final = read(fd, &nuevoProceso, sizeof(proceso_s));
    int contador = 1;
    while (final > 0)
    {
        nuevoProceso.ID = contador;
        insertarAlFinal(head,nuevoProceso);
        final = read(fd, &nuevoProceso, sizeof(proceso_s));
        contador++;
    }
    close(fd);
}
void escribirEnDocumento(nodo_s **head, char* nombreDelDocumento){
    printf("Luego se implementa");
}
int obtenerElTamano(nodo_s **head){
    int tamano=0;
    nodo_s *aux = (* head);
    if(aux!=NULL){
        while(aux!=NULL){
            tamano++;
            aux = aux->sig;
        }
    }
    return tamano;
}