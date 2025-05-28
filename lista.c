#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>

#include "lista.h"

// variable global para el proximo ID numerico secuencial si el usuario no lo da
static int proximo_id_secuencial = 1;


void insertarEnLista(proceso_s mi_pid, nodo_s **head)
{
    nodo_s *aux = NULL; 
    if ((*head) == NULL)
    {
        if (mi_pid.ID == -99) // si no se asigno id, usar el secuencial
        {
            mi_pid.ID = proximo_id_secuencial++;
        } else if (mi_pid.ID >= proximo_id_secuencial) { // si se asigno, actualizar el secuencial si es mayor
            proximo_id_secuencial = mi_pid.ID + 1;
        }
        (*head) = (nodo_s *)malloc(sizeof(nodo_s));
        (*head)->pid = mi_pid;
        (*head)->sig = NULL;
    }
    else
    {
        if (mi_pid.ID == -99)
        {
             // buscar el id mas alto en la lista para continuar la secuencia
            nodo_s* temp = *head;
            int max_id = 0;
            while(temp != NULL) {
                if(temp->pid.ID > max_id) max_id = temp->pid.ID;
                temp = temp->sig;
            }
            proximo_id_secuencial = max_id + 1;
            mi_pid.ID = proximo_id_secuencial++;

        } else if (mi_pid.ID >= proximo_id_secuencial) {
             proximo_id_secuencial = mi_pid.ID + 1;
        }
        aux = (nodo_s *)malloc(sizeof(nodo_s));
        aux->pid = mi_pid;
        aux->sig = (*head);
        (*head) = aux;
    }
}

void imprimirLaLista(nodo_s **head)
{
    if ((*head) != NULL)
    {
        nodo_s *aux = (*head);
        printf("lista de procesos:\n");
        printf("  %-4s %-20s %-10s %-10s %-12s %-15s %-10s\n", 
               "ID", "Nombre", "Burst T", "Bloques", "Estado", "Dir. Memoria", "BT Rest.");
        printf("  --------------------------------------------------------------------------------------\n");
        while (aux != NULL)
        {
            printf("  %-4d %-20s %-10d %-10d %-12s ", 
                   aux->pid.ID, aux->pid.nombreProceso, aux->pid.burstTime, aux->pid.bloque, aux->pid.estado);
            if (aux->pid.direccion_memoria != -1) {
                printf("%-15d ", aux->pid.direccion_memoria);
            } else {
                printf("%-15s ", "N/A");
            }
            printf("%-10d\n", aux->pid.burst_time_restante);
            aux = aux->sig;
        };
    }
    else
    {
        printf("nada que mostrar en la lista de procesos :(\n");
    }
}

void ordenarLista(nodo_s **head) // ordena por burst time (para sjf), luego por id si bt es igual
{
    if (*head == NULL || (*head)->sig == NULL) return; // no necesita ordenarse

    nodo_s *actual = NULL, *siguiente = NULL;
    proceso_s temp_pid;
    int swapped;

    do {
        swapped = 0;
        actual = *head;

        while (actual->sig != siguiente) {
            if (actual->pid.burst_time_restante > actual->sig->pid.burst_time_restante ||
                (actual->pid.burst_time_restante == actual->sig->pid.burst_time_restante && actual->pid.ID > actual->sig->pid.ID)) {
                // intercambiar pids
                temp_pid = actual->pid;
                actual->pid = actual->sig->pid;
                actual->sig->pid = temp_pid;
                swapped = 1;
            }
            actual = actual->sig;
        }
        siguiente = actual; // el ultimo elemento ya esta en su lugar
    } while (swapped);
}

void insertarElementoEnPosicion(nodo_s **head, int pos, proceso_s mi_pid)
{
    if (head == NULL || pos < 0) return; // cambio: head debe ser != NULL
    
    if (pos == 0 || *head == NULL) // si pos es 0 o la lista esta vacia, insertar al inicio
    {
        // para insertar al inicio, usamos la logica de insertarEnLista pero
        // esta puede ser llamada por ordenarLista, asi que no debemos reasignar ID aqui
        // a menos que sea -99.
        nodo_s* nuevo_nodo = (nodo_s*)malloc(sizeof(nodo_s));
        if (!nuevo_nodo) { perror("malloc en insertarElementoEnPosicion"); return; }
        
        if (mi_pid.ID == -99) { // solo si es un proceso completamente nuevo
            // buscar el id mas alto en la lista para continuar la secuencia
            nodo_s* temp_iter = *head;
            int max_id = 0;
            while(temp_iter != NULL) {
                if(temp_iter->pid.ID > max_id) max_id = temp_iter->pid.ID;
                temp_iter = temp_iter->sig;
            }
            proximo_id_secuencial = max_id + 1;
            mi_pid.ID = proximo_id_secuencial++;
        }

        nuevo_nodo->pid = mi_pid;
        nuevo_nodo->sig = *head;
        *head = nuevo_nodo;
        return;
    }

    nodo_s *aux = (*head);
    nodo_s *nodoAIngresar = NULL;
    int posInterna = 0; // la posicion 0 es la cabeza, la 1 es el siguiente.
                       // si pos = 1, queremos insertar *despues* del primer nodo.

    // iterar hasta el nodo *anterior* a la posicion de insercion
    while (aux != NULL && posInterna < pos -1)
    {
        aux = aux->sig;
        posInterna++;
    }

    if (aux == NULL) { // posicion fuera de rango (demasiado grande)
        // Podriamos insertar al final o reportar error.
        // Por consistencia con la logica original de insertarAlFinal, vamos a hacerlo.
        // Pero la logica original de 'insertarAlFinal' se llama con el head, no pos.
        // Asi que esta condicion implica un error de 'pos' o que la lista es mas corta.
        // Para esta funcion, si pos es mayor que el tamano, no hacemos nada o insertamos al final.
        // El codigo original de ordenarLista espera que esto funcione.
        // Replicando la logica de 'insertarAlFinal' si aux->sig es NULL en el bucle original
        insertarAlFinal(head, mi_pid);
        return;
    }
    
    // aux es ahora el nodo *antes* de la posicion deseada
    nodoAIngresar = (nodo_s *)malloc(sizeof(nodo_s));
    if (!nodoAIngresar) { perror("malloc en insertarElementoEnPosicion"); return; }

    // ID management: solo si es -99
     if (mi_pid.ID == -99) {
        // buscar el id mas alto en la lista para continuar la secuencia
        nodo_s* temp_iter = *head;
        int max_id = 0;
        while(temp_iter != NULL) {
            if(temp_iter->pid.ID > max_id) max_id = temp_iter->pid.ID;
            temp_iter = temp_iter->sig;
        }
        proximo_id_secuencial = max_id + 1;
        mi_pid.ID = proximo_id_secuencial++;
    }
    
    nodoAIngresar->pid = mi_pid;
    nodoAIngresar->sig = aux->sig;
    aux->sig = nodoAIngresar;
}


void obtenerElPrimerElemento(nodo_s **head){
    // esta funcion no esta definida, pero se declara. la dejare asi.
    // si se refiere a imprimir el primer elemento:
    if (*head != NULL) {
         printf("primer elemento: ID %d, Nombre %s\n", (*head)->pid.ID, (*head)->pid.nombreProceso);
    } else {
        printf("la lista esta vacia.\n");
    }
}


void insertarAlFinal(nodo_s **head, proceso_s proceso)
{
    if (proceso.ID == -99) // asignar id si es nuevo
    {
        if (*head == NULL) {
            proceso.ID = proximo_id_secuencial++;
        } else {
            // buscar el id mas alto en la lista para continuar la secuencia
            nodo_s* temp = *head;
            int max_id = 0;
            while(temp != NULL) {
                if(temp->pid.ID > max_id) max_id = temp->pid.ID;
                temp = temp->sig;
            }
            proximo_id_secuencial = max_id + 1;
            proceso.ID = proximo_id_secuencial++;
        }
    } else if (proceso.ID >= proximo_id_secuencial) { // si ya tiene id, actualizar proximo_id_secuencial
        proximo_id_secuencial = proceso.ID + 1;
    }


    nodo_s *nuevo_nodo = (nodo_s *)malloc(sizeof(nodo_s));
    if (nuevo_nodo == NULL) {
        perror("error en malloc para insertarAlFinal");
        return;
    }
    nuevo_nodo->pid = proceso;
    nuevo_nodo->sig = NULL;

    if ((*head) == NULL)
    { 
        (*head) = nuevo_nodo;
    }
    else
    {
        nodo_s *aux = (*head);
        while (aux->sig != NULL)
        {
            aux = aux->sig;
        }
        aux->sig = nuevo_nodo;
    }
}

void eliminarElInicio(nodo_s **head)
{
    if ((*head) != NULL) 
    {
        nodo_s *nodo_a_borrar = (*head);
        (*head) = (*head)->sig;
        free(nodo_a_borrar);
    }
}

void eliminarElementoEnPosicion(nodo_s **head, int pos)
{
    if (head == NULL || *head == NULL || pos < 0) return; // validaciones

    if (pos == 0)
    {
        eliminarElInicio(head);
        return;
    }

    nodo_s *actual = *head;
    nodo_s *anterior = NULL;
    int posInterna = 0;

    while (actual != NULL && posInterna < pos)
    {
        anterior = actual;
        actual = actual->sig;
        posInterna++;
    }

    if (actual != NULL && anterior != NULL) // si se encontro el nodo en la posicion 'pos'
    {
        anterior->sig = actual->sig;
        free(actual);
    } else {
        // la posicion estaba fuera de rango (mas alla del final de la lista)
        // o la lista era mas corta de lo esperado.
        // printf("error: posicion %d fuera de rango para eliminacion.\n", pos);
    }
}

void eliminarAlFinal(nodo_s **head)
{
    if (head == NULL || *head == NULL) return;

    if ((*head)->sig == NULL) // solo un elemento
    {
        eliminarElInicio(head);
        return;
    }

    nodo_s *actual = (*head);
    nodo_s *anterior = NULL;
    while (actual->sig != NULL)
    {
        anterior = actual;
        actual = actual->sig;
    }
    // ahora 'actual' es el ultimo, 'anterior' es el penultimo
    if (anterior != NULL) {
        anterior->sig = NULL;
        free(actual);
    }
    // el caso de anterior == NULL es cubierto por el chequeo de (*head)->sig == NULL
}

void eliminarPorID(nodo_s **head, char *ID_nombre) // nombre del proceso como id
{
    nodo_s *actual = *head;
    nodo_s *anterior = NULL;

    if (actual != NULL && strcmp(actual->pid.nombreProceso, ID_nombre) == 0)
    {
        *head = actual->sig;
        free(actual);
        printf("proceso %s eliminado.\n", ID_nombre);
        return;
    }

    while (actual != NULL && strcmp(actual->pid.nombreProceso, ID_nombre) != 0)
    {
        anterior = actual;
        actual = actual->sig;
    }

    if (actual == NULL)
    {
        printf("proceso %s no encontrado para eliminar.\n", ID_nombre);
        return;
    }

    // desenlazar el nodo de la lista
    anterior->sig = actual->sig;
    free(actual);
    printf("proceso %s eliminado.\n", ID_nombre);
}

void leerDeDocumento(nodo_s **head, char *nombreDelDocumento)
{
    proceso_s nuevoProceso;
    int fd = open(nombreDelDocumento, O_RDONLY);
    if (fd == -1) {
        // si el archivo no existe, no es un error critico, simplemente no hay nada que leer.
        // perror("abriendo archivo de procesos para lectura"); 
        return;
    }
    
    int max_id_leido = 0;
    while (read(fd, &nuevoProceso, sizeof(proceso_s)) > 0)
    {
        // no se asigna ID aqui, se usa el que viene del archivo
        // pero si se actualiza el proximo_id_secuencial
        if (nuevoProceso.ID > max_id_leido) {
            max_id_leido = nuevoProceso.ID;
        }
        insertarAlFinal(head, nuevoProceso); // insertarAlFinal se encarga de los IDs si es necesario
    }
    proximo_id_secuencial = max_id_leido + 1; // asegurar que el proximo ID sea mayor

    close(fd);
    // printf("procesos cargados desde %s.\n", nombreDelDocumento);
}

void escribirEnDocumento(nodo_s **head, char *nombreDelDocumento)
{
    nodo_s *aux = (*head);
    // usar O_TRUNC para sobrescribir el archivo completamente
    int fd = open(nombreDelDocumento, O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (fd == -1) {
        perror("abriendo archivo de procesos para escritura");
        return;
    }
    while (aux != NULL)
    {
        if (write(fd, &aux->pid, sizeof(proceso_s)) == -1) {
            perror("escribiendo proceso al archivo");
            // decidir si continuar o abortar
        }
        aux = aux->sig;
    }
    close(fd);
    // printf("lista de procesos guardada en %s.\n", nombreDelDocumento);
}


int obtenerElTamano(nodo_s **head)
{
    int tamano = 0;
    nodo_s *aux = (*head);
    while (aux != NULL)
    {
        tamano++;
        aux = aux->sig;
    }
    return tamano;
}

int obtenerNumeroProcesosReady(nodo_s **head) {
    int count = 0;
    nodo_s *current = *head;
    while (current != NULL) {
        if (strcmp(current->pid.estado, "ready") == 0) {
            count++;
        }
        current = current->sig;
    }
    return count;
}


int existeElID(nodo_s **head, char *ID_nombre) // busca por nombre de proceso
{
    nodo_s *aux = (*head);
    while (aux != NULL)
    {
        if (strcmp(aux->pid.nombreProceso, ID_nombre) == 0)
        {
            return 1; // encontrado
        }
        aux = aux->sig;
    }
    return 0; // no encontrado
}

int existeElIDNumerico(nodo_s **head, int num_id) // busca por ID numerico
{
    nodo_s *aux = (*head);
    while (aux != NULL)
    {
        if (aux->pid.ID == num_id)
        {
            return 1; // encontrado
        }
        aux = aux->sig;
    }
    return 0; // no encontrado
}

proceso_s* buscarProcesoPorNombre(nodo_s **head, const char *nombreProceso) {
    nodo_s *current = *head;
    while (current != NULL) {
        if (strcmp(current->pid.nombreProceso, nombreProceso) == 0) {
            return &(current->pid);
        }
        current = current->sig;
    }
    return NULL; // no encontrado
}

void limpiar_lista_al_salir(nodo_s **head) {
    nodo_s *current = *head;
    nodo_s *next_node;
    while (current != NULL) {
        next_node = current->sig;
        free(current);
        current = next_node;
    }
    *head = NULL;
    printf("lista de procesos limpiada.\n");
}