#ifndef MEMORIA_H
#define MEMORIA_H

#include "lista.h" // para proceso_s y nodo_s

#define MEMORY_SIZE 1024 // tamano total de la memoria en bloques

// estructura para un bloque de memoria
typedef struct mem_block {
    int start_address;
    int size;
    char process_name[50]; // nombre del proceso que ocupa este bloque, "" si esta libre
    int process_id_num;    // id numerico del proceso (proceso_s.ID)
    struct mem_block *next;
    struct mem_block *prev;
} mem_block_s;

// funcion para inicializar la memoria
// crea un solo bloque grande libre
void inicializar_memoria();

// funcion para asignar memoria a un proceso
// retorna la direccion de inicio si tiene exito, -1 si falla
int alloc_memoria(proceso_s *proceso, const char *estrategia, nodo_s **lista_procesos_global);

// funcion para liberar memoria ocupada por un proceso
// necesita el nombre del proceso para identificar el/los bloque(s) a liberar
void free_memoria_por_nombre(const char *process_name);

// funcion para mostrar el estado actual de la memoria
void mstatus_memoria();

// funcion para compactar la memoria
// mueve todos los bloques ocupados al inicio de la memoria
// actualiza las direcciones de los procesos en la lista global de procesos
void compactar_memoria(nodo_s **lista_procesos_global);

// funcion para limpiar la memoria (liberar todos los bloques)
void limpiar_memoria_al_salir();

#endif // MEMORIA_H