#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "memoria.h"
#include "lista.h" // para nodo_s y proceso_s

mem_block_s *mem_list_head = NULL;

void inicializar_memoria() {
    // liberar cualquier lista de memoria existente
    mem_block_s *current = mem_list_head;
    mem_block_s *next_block;
    while (current != NULL) {
        next_block = current->next;
        free(current);
        current = next_block;
    }

    // crear el bloque inicial grande y libre
    mem_list_head = (mem_block_s *)malloc(sizeof(mem_block_s));
    if (mem_list_head == NULL) {
        perror("error al inicializar memoria");
        exit(EXIT_FAILURE);
    }
    mem_list_head->start_address = 0;
    mem_list_head->size = MEMORY_SIZE;
    strcpy(mem_list_head->process_name, ""); // vacio significa libre
    mem_list_head->process_id_num = -1;
    mem_list_head->next = NULL;
    mem_list_head->prev = NULL;
    printf("memoria inicializada con %d bloques.\n", MEMORY_SIZE);
}

void mstatus_memoria() {
    printf("estado de la memoria (total %d bloques):\n", MEMORY_SIZE);
    mem_block_s *current = mem_list_head;
    int total_allocated_blocks = 0;
    int total_free_blocks = 0;
    int num_occupied_blocks = 0;
    int num_free_blocks = 0;

    if (current == NULL) {
        printf("  la lista de memoria esta vacia o no inicializada.\n");
        return;
    }

    while (current != NULL) {
        if (strcmp(current->process_name, "") != 0) { // ocupado
            printf("  bloque [ocupado]: inicio=%d, tamano=%d, proceso=%s (id:%d)\n",
                   current->start_address, current->size, current->process_name, current->process_id_num);
            total_allocated_blocks += current->size;
            num_occupied_blocks++;
        } else { // libre
            printf("  bloque [libre]:   inicio=%d, tamano=%d\n",
                   current->start_address, current->size);
            total_free_blocks += current->size;
            num_free_blocks++;
        }
        current = current->next;
    }
    printf("resumen:\n");
    printf("  bloques ocupados: %d (%d en total)\n", total_allocated_blocks, num_occupied_blocks);
    printf("  bloques libres:   %d (%d en total)\n", total_free_blocks, num_free_blocks);
    if (num_free_blocks > 1 && total_free_blocks > 0) {
         // la fragmentacion externa existe si hay multiples bloques libres no contiguos
         // o si hay bloques libres que no estan al final.
         // una metrica simple es el tamano del bloque libre mas grande vs el total libre
        mem_block_s *largest_free = NULL;
        current = mem_list_head;
        while(current != NULL) {
            if(strcmp(current->process_name, "") == 0) { // libre
                if(largest_free == NULL || current->size > largest_free->size) {
                    largest_free = current;
                }
            }
            current = current->next;
        }
        if (largest_free != NULL && largest_free->size < total_free_blocks) {
             printf("  fragmentacion externa detectada. bloque libre mas grande: %d, total libre: %d\n", largest_free->size, total_free_blocks);
        }
    }
}


// fusiona bloques libres adyacentes
void fusionar_bloques_libres() {
    mem_block_s *current = mem_list_head;
    while (current != NULL && current->next != NULL) {
        if (strcmp(current->process_name, "") == 0 && strcmp(current->next->process_name, "") == 0) {
            // ambos bloques estan libres, fusionar
            mem_block_s *next_block_to_remove = current->next;
            current->size += next_block_to_remove->size;
            current->next = next_block_to_remove->next;
            if (current->next != NULL) {
                current->next->prev = current;
            }
            free(next_block_to_remove);
            // no avanzar current, revisar de nuevo por si hay mas fusiones
        } else {
            current = current->next;
        }
    }
}

int alloc_memoria(proceso_s *proceso, const char *estrategia, nodo_s **lista_procesos_global) {
    if (proceso == NULL || proceso->bloque <= 0) {
        printf("error: proceso invalido o tamano de bloque cero/negativo.\n");
        return -1;
    }

    mem_block_s *current = mem_list_head;
    mem_block_s *candidate_block = NULL;

    if (strcmp(estrategia, "first") == 0) {
        while (current != NULL) {
            if (strcmp(current->process_name, "") == 0 && current->size >= proceso->bloque) {
                candidate_block = current;
                break;
            }
            current = current->next;
        }
    } else if (strcmp(estrategia, "best") == 0) {
        int min_diff = MEMORY_SIZE + 1; // un valor mayor que cualquier diferencia posible
        while (current != NULL) {
            if (strcmp(current->process_name, "") == 0 && current->size >= proceso->bloque) {
                if (current->size - proceso->bloque < min_diff) {
                    min_diff = current->size - proceso->bloque;
                    candidate_block = current;
                }
            }
            current = current->next;
        }
    } else if (strcmp(estrategia, "worst") == 0) {
        int max_size = -1;
        while (current != NULL) {
            if (strcmp(current->process_name, "") == 0 && current->size >= proceso->bloque) {
                if (current->size > max_size) {
                    max_size = current->size;
                    candidate_block = current;
                }
            }
            current = current->next;
        }
    } else {
        printf("error: estrategia de asignacion desconocida '%s'. use 'first', 'best' o 'worst'.\n", estrategia);
        return -1;
    }

    if (candidate_block == NULL) {
        printf("no hay suficiente memoria para el proceso %s (tamano %d) usando la estrategia %s.\n",
               proceso->nombreProceso, proceso->bloque, estrategia);
        return -1;
    }

    // asignar el bloque
    int original_candidate_size = candidate_block->size;
    int start_addr = candidate_block->start_address;

    strncpy(candidate_block->process_name, proceso->nombreProceso, sizeof(candidate_block->process_name) - 1);
    candidate_block->process_name[sizeof(candidate_block->process_name) - 1] = '\0';
    candidate_block->process_id_num = proceso->ID;
    candidate_block->size = proceso->bloque;
    proceso->direccion_memoria = start_addr; // actualizar en la estructura del proceso

    // si queda espacio en el bloque candidato, crear un nuevo bloque libre
    if (original_candidate_size > proceso->bloque) {
        mem_block_s *new_free_block = (mem_block_s *)malloc(sizeof(mem_block_s));
        if (new_free_block == NULL) {
            perror("error al crear nuevo bloque libre restante");
            // revertir asignacion (simplificado, podria ser mas robusto)
            strcpy(candidate_block->process_name, "");
            candidate_block->process_id_num = -1;
            candidate_block->size = original_candidate_size;
            proceso->direccion_memoria = -1;
            return -1;
        }
        new_free_block->start_address = candidate_block->start_address + proceso->bloque;
        new_free_block->size = original_candidate_size - proceso->bloque;
        strcpy(new_free_block->process_name, ""); // libre
        new_free_block->process_id_num = -1;

        // insertar new_free_block despues de candidate_block
        new_free_block->next = candidate_block->next;
        new_free_block->prev = candidate_block;
        if (candidate_block->next != NULL) {
            candidate_block->next->prev = new_free_block;
        }
        candidate_block->next = new_free_block;
    }
    printf("proceso %s (id:%d, tamano:%d) asignado a memoria en %d usando %s-fit.\n",
           proceso->nombreProceso, proceso->ID, proceso->bloque, start_addr, estrategia);
    fusionar_bloques_libres(); // fusionar si la division creo bloques libres adyacentes
    return start_addr;
}

void free_memoria_por_nombre(const char *process_name) {
    mem_block_s *current = mem_list_head;
    int freed = 0;
    while (current != NULL) {
        if (strcmp(current->process_name, process_name) == 0) {
            printf("liberando memoria para el proceso %s en la direccion %d (tamano %d).\n",
                   current->process_name, current->start_address, current->size);
            strcpy(current->process_name, ""); // marcar como libre
            current->process_id_num = -1;
            freed = 1;
            // no romper, un proceso podria (incorrectamente) tener multiples bloques
            // aunque con asignacion contigua esto no deberia pasar.
        }
        current = current->next;
    }

    if (freed) {
        fusionar_bloques_libres();
    } else {
        printf("proceso %s no encontrado en memoria para liberar.\n", process_name);
    }
}

void compactar_memoria(nodo_s **lista_procesos_global) {
    printf("iniciando compactacion de memoria...\n");
    if (mem_list_head == NULL) return;

    // mem_block_s *write_ptr = mem_list_head; // VARIABLE NO USADA - ELIMINADA
    mem_block_s *read_ptr = mem_list_head;  // para iterar sobre todos los bloques
    int current_address = 0;
    mem_block_s *new_mem_list_head = NULL;
    mem_block_s *new_mem_list_tail = NULL;

    // primera pasada: mover todos los bloques ocupados al principio y construir nueva lista
    while (read_ptr != NULL) {
        mem_block_s *next_read_ptr = read_ptr->next; // guardar el siguiente antes de modificar
        if (strcmp(read_ptr->process_name, "") != 0) { // si es un bloque ocupado
            // actualizar la direccion de inicio del proceso en la lista global
            nodo_s *proc_node = *lista_procesos_global;
            while(proc_node != NULL) {
                if(strcmp(proc_node->pid.nombreProceso, read_ptr->process_name) == 0) {
                    //printf("actualizando direccion de %s de %d a %d\n", proc_node->pid.nombreProceso, proc_node->pid.direccion_memoria, current_address);
                    proc_node->pid.direccion_memoria = current_address;
                    break;
                }
                proc_node = proc_node->sig;
            }

            // actualizar el bloque de memoria
            read_ptr->start_address = current_address;
            current_address += read_ptr->size;

            // anadir a la nueva lista
            read_ptr->prev = new_mem_list_tail;
            read_ptr->next = NULL;
            if (new_mem_list_head == NULL) {
                new_mem_list_head = read_ptr;
            } else {
                new_mem_list_tail->next = read_ptr;
            }
            new_mem_list_tail = read_ptr;
        } else {
            // si es un bloque libre, simplemente lo liberamos (se recreara uno grande al final)
            free(read_ptr);
        }
        read_ptr = next_read_ptr;
    }

    mem_list_head = new_mem_list_head; // la nueva lista solo tiene bloques ocupados

    // si queda espacio libre, anadir un solo bloque libre grande al final
    if (current_address < MEMORY_SIZE) {
        mem_block_s *free_block = (mem_block_s *)malloc(sizeof(mem_block_s));
        if (free_block == NULL) {
            perror("error al crear bloque libre despues de compactacion");
            // la memoria estaria en un estado inconsistente aqui.
            return;
        }
        free_block->start_address = current_address;
        free_block->size = MEMORY_SIZE - current_address;
        strcpy(free_block->process_name, "");
        free_block->process_id_num = -1;
        free_block->next = NULL;
        free_block->prev = new_mem_list_tail;

        if (new_mem_list_head == NULL) { // si no habia bloques ocupados
            mem_list_head = free_block;
        } else {
            new_mem_list_tail->next = free_block;
        }
    } else if (new_mem_list_tail != NULL) { // si la memoria esta llena, el ultimo no tiene next
         new_mem_list_tail->next = NULL;
    }


    printf("compactacion completada.\n");
    fusionar_bloques_libres(); // por si acaso, aunque no deberia ser necesario aqui
    mstatus_memoria();
}


void limpiar_memoria_al_salir() {
    mem_block_s *current = mem_list_head;
    mem_block_s *next_block;
    while (current != NULL) {
        next_block = current->next;
        free(current);
        current = next_block;
    }
    mem_list_head = NULL;
    printf("memoria limpiada.\n");
}