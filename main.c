#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include "colors.h"
#include "calendarizacionProcesos.h"
#include "lista.h" // asegurarnos que lista.h este antes de memoria.h si hay dependencias
#include "memoria.h"


#define READ_END 0
#define WRITE_END 1
#define NOMBRE_ARCHIVO_PROCESOS "guardarProcesos.bin"
#define DEFAULT_QUANTUM 10 // para round robin

// Tuve que declarar las funciones porque me marcaba error, no deberia marcaar error pero bueno
void crearElArgs(char *instrucciones, char *args[]);
void lecturaDeLaTerminalSuperEpico(char **lecturaDeLaTerminal);
void limpiarArgs(char *args[], int tamano);
void ejecutarCodigo(int numeroDeComandos, char *instrucciones[]); // Cambiado nombre del param para claridad

void ejecutarCodigo(int numeroDeComandos, char *instrucciones[])
{
    char *args[15];
    // Se necesitan 'numeroDeComandos - 1' pipas.
    // Maximo 5 comandos en pipeline => 4 pipas. El array pipas[4][2] es suficiente.
    // El enunciado dice "comunicar hasta cinco procesos", implica 5 comandos, 4 pipas.
    // Si interpretamos 5 pipas literales (como en el codigo original), serian 6 comandos.
    // Vamos a usar maximo 4 pipas (para 5 comandos).
    int pipas[4][2];

    if (numeroDeComandos <= 0) {
        return;
    }
    if (numeroDeComandos > 5) {
        printf("error: maximo 5 comandos permitidos en pipeline. truncando.\n");
        numeroDeComandos = 5;
    }

    // Crear pipas necesarias: numeroDeComandos - 1
    for (int i = 0; i < numeroDeComandos - 1; i++)
    {
        if (pipe(pipas[i]) == -1) {
            perror("pipe");
            // Podríamos intentar cerrar las pipas abiertas antes de salir, pero es complejo
            exit(EXIT_FAILURE);
        }
    }

    pid_t pids[numeroDeComandos];

    for (int i = 0; i < numeroDeComandos; i++)
    {
        pids[i] = fork();
        if (pids[i] == -1)
        {
            perror("fork");
            // Intentar matar hijos ya creados y cerrar pipas antes de salir.
            // Simplificado: salir. En un shell real, esto es más robusto.
            exit(EXIT_FAILURE);
        }

        if (pids[i] == 0) // Proceso hijo
        {
            // Limpiar args de cualquier uso previo (aunque en el hijo no debería haber)
            // Es más una precaución si esta estructura de args se reutilizara mucho.
            for(int k=0; k<15; ++k) args[k] = NULL; // Simple inicializacion a NULL

            crearElArgs(instrucciones[i], args);
            if (args[0] == NULL) { // Si crearElArgs falló o no hay comando
                fprintf(stderr, "error: no se pudo procesar el comando: %s\n", instrucciones[i] ? instrucciones[i] : "(null)");
                exit(EXIT_FAILURE); // Salir si no hay nada que ejecutar
            }


            // Redirección de entrada si no es el primer comando
            if (i > 0)
            {
                if (dup2(pipas[i - 1][READ_END], STDIN_FILENO) == -1) {
                    perror("dup2 read en hijo");
                    exit(EXIT_FAILURE);
                }
            }

            // Redirección de salida si no es el último comando
            if (i < numeroDeComandos - 1)
            {
                if (dup2(pipas[i][WRITE_END], STDOUT_FILENO) == -1) {
                    perror("dup2 write en hijo");
                    exit(EXIT_FAILURE);
                }
            }

            // Cerrar TODOS los descriptores de las pipas en el hijo
            // (tanto los que usa como los que no, después de dup2)
            for (int j = 0; j < numeroDeComandos - 1; j++)
            {
                close(pipas[j][READ_END]);
                close(pipas[j][WRITE_END]);
            }

            execvp(args[0], args);
            // Si execvp retorna, es un error
            perror("execvp fallo");
            // Liberar args si execvp falla (aunque el OS lo hará al salir)
            // limpiarArgs(args, 15); // No estrictamente necesario aquí porque el proceso va a salir.
            exit(EXIT_FAILURE);
        }
    }

    // Proceso padre:
    // Cerrar todos los descriptores de las pipas que el padre no usa.
    // El padre NO debe escribir ni leer de las pipas usadas entre hijos.
    for (int i = 0; i < numeroDeComandos - 1; i++)
    {
        close(pipas[i][READ_END]);
        close(pipas[i][WRITE_END]);
    }

    // Esperar a que todos los hijos terminen
    for (int i = 0; i < numeroDeComandos; i++)
    {
        waitpid(pids[i], NULL, 0);
    }

    // Los args de la funcion ejecutarCodigo (los que se pasaron como `instrucciones`)
    // se liberan en el bucle principal de main.
    // Los `args` locales de esta funcion (usados para execvp) se manejan internamente
    // o son liberados por el SO cuando los hijos terminan o por limpiarArgs si execvp falla.
    // Sin embargo, `limpiarArgs` no se llama aquí en el padre para los `args` locales.
    // El array `args` local de esta función solo contiene punteros,
    // la memoria a la que apuntan es liberada por `limpiarArgs` al inicio de CADA llamada a `crearElArgs`
    // o por el SO en el hijo.
    // Aquí no hay `args` que limpiar en el padre después de que los hijos terminaron.
}

void crearElArgs(char *instrucciones_originales, char *args[]) {
    // Inicializar args a NULL por seguridad
    for(int k=0; k<15; ++k) args[k] = NULL;

    if (!instrucciones_originales || strlen(instrucciones_originales) == 0) {
        args[0] = NULL; // No hay comando
        return;
    }

    char *copia_instrucciones = strdup(instrucciones_originales);
    if (!copia_instrucciones) {
        perror("strdup en crearElArgs (copia_instrucciones)");
        args[0] = NULL;
        return;
    }

    char *ptr_para_strtok = copia_instrucciones; // Usar un puntero separado para strtok
    char *token;
    int cantidadDeArgumentos = 0;

    // Bucle para extraer tokens
    while ((token = strtok_r(ptr_para_strtok, " ", &ptr_para_strtok)) != NULL && cantidadDeArgumentos < 14) {
        // strtok_r es reentrante y más seguro si hubiera multithreading,
        // pero aquí principalmente ayuda a manejar la cadena de forma más limpia.
        // Si no se usa strtok_r, el strtok simple está bien para este caso de un solo hilo.

        // Eliminar espacios en blanco al inicio/final del token (strtok debería manejarlos, pero por si acaso)
        // Esta parte es más compleja de hacer robustamente sin modificar el token in-place si strdup ya lo copió.
        // Por ahora, confiamos en que strtok maneja bien los delimitadores.
        // Si se necesita un trimming más robusto, se haría sobre el 'token' antes de strdup.

        if (strlen(token) > 0) { // Solo añadir si el token no está vacío
            args[cantidadDeArgumentos] = strdup(token);
            if (args[cantidadDeArgumentos] == NULL) {
                perror("strdup en crearElArgs (args[cantidadDeArgumentos])");
                // Limpiar lo que se haya alocado hasta ahora en args
                for (int k = 0; k < cantidadDeArgumentos; k++) {
                    if (args[k]) free(args[k]);
                    args[k] = NULL;
                }
                args[0] = NULL; // Indicar fallo
                free(copia_instrucciones); // Liberar la copia
                return;
            }
            cantidadDeArgumentos++;
        }
    }
    args[cantidadDeArgumentos] = NULL; // Terminador NULL para execvp

    free(copia_instrucciones); // Liberar la copia de instrucciones
}


void limpiarArgs(char *args[], int tamano) // Esta función limpia un array de strings
{
    for (int i = 0; i < tamano; i++)
    {
        if (args[i] != NULL) {
            free(args[i]);
            args[i] = NULL;
        }
    }
}


void lecturaDeLaTerminalSuperEpico(char **lecturaDeLaTerminal)
{
    char c;
    if (*lecturaDeLaTerminal != NULL) { // Si se llama con un buffer existente, liberarlo primero
        free(*lecturaDeLaTerminal);
        *lecturaDeLaTerminal = NULL;
    }

    *lecturaDeLaTerminal = malloc(1);
    if (*lecturaDeLaTerminal == NULL) { perror("malloc inicial en lectura"); exit(EXIT_FAILURE); }
    (*lecturaDeLaTerminal)[0] = '\0';

    size_t buffer_size = 1;
    size_t current_len = 0;

    while ((c = getchar()) != '\n' && c != EOF)
    {
        if (current_len + 1 >= buffer_size) { // +1 para el char actual, +1 para el NUL
            buffer_size = (buffer_size == 0) ? 2 : buffer_size * 2; // Duplicar tamaño
            // Si buffer_size es 1, buffer_size*2 es 2. Si es 2, 4. etc.
            // O una estrategia de incremento fijo: buffer_size += 128;
            char *new_buffer = realloc(*lecturaDeLaTerminal, buffer_size);
            if (new_buffer == NULL) {
                perror("realloc en lecturaDeLaTerminalSuperEpico");
                free(*lecturaDeLaTerminal); // Liberar el buffer antiguo
                *lecturaDeLaTerminal = NULL;
                return;
            }
            *lecturaDeLaTerminal = new_buffer;
        }
        (*lecturaDeLaTerminal)[current_len++] = c;
        (*lecturaDeLaTerminal)[current_len] = '\0'; // Mantener NUL terminado en cada paso
    }

    if (c == EOF && current_len == 0) { // EOF en línea vacía
        if (*lecturaDeLaTerminal) free(*lecturaDeLaTerminal);
        *lecturaDeLaTerminal = strdup("exit"); // Simular comando exit
        if (!*lecturaDeLaTerminal) { perror("strdup para exit"); exit(EXIT_FAILURE);}
    }
}

int main(void)
{
    nodo_s *head = NULL;
    inicializar_memoria();
    leerDeDocumento(&head, NOMBRE_ARCHIVO_PROCESOS);

    // Palabras para los comandos parseados de la línea (separados por '|')
    // Max 5 comandos en pipeline, cada uno puede ser un string.
    char *Palabras[6]; // Array de punteros a char. +1 para NULL terminador si se tratara como argv.
                       // Aquí lo usamos para 5 comandos y el 6to es NULL o no se usa.
    for(int i=0; i<6; ++i) Palabras[i] = NULL; // Inicializar a NULL

    int cantidadDeComandos = 0;
    char *lecturaDeLaTerminal = NULL; // Se alocará dentro de lecturaDeLaTerminalSuperEpico

    while (1)
    {
        printf(LGREEN "\nshell@myos:$ " RESET); // CORREGIDO: sin %s extra
        lecturaDeLaTerminalSuperEpico(&lecturaDeLaTerminal);

        if (lecturaDeLaTerminal == NULL || strlen(lecturaDeLaTerminal) == 0) {
            if (lecturaDeLaTerminal) { // Podría ser NULL si realloc falló
                free(lecturaDeLaTerminal);
                lecturaDeLaTerminal = NULL;
            }
            continue;
        }

        if (strcmp(lecturaDeLaTerminal, "exit") == 0) {
            printf("saliendo del shell...\n");
            if (lecturaDeLaTerminal) free(lecturaDeLaTerminal);
            break;
        }

        // Liberar Palabras de la iteración anterior antes de reusarlas
        for(int i=0; i<6; ++i) {
            if(Palabras[i]) {
                free(Palabras[i]);
                Palabras[i] = NULL;
            }
        }
        cantidadDeComandos = 0;

        char *copiaLecturaParaPipas = strdup(lecturaDeLaTerminal);
        if (!copiaLecturaParaPipas) {
            perror("strdup copiaLecturaParaPipas");
            if (lecturaDeLaTerminal) { free(lecturaDeLaTerminal); lecturaDeLaTerminal = NULL; }
            continue;
        }

        char *tokenPipa;
        char *restoPipa = copiaLecturaParaPipas;

        while ((tokenPipa = strtok_r(restoPipa, "|", &restoPipa)) != NULL && cantidadDeComandos < 5) {
            // Quitar espacios al inicio/final del tokenPipa
            char *start = tokenPipa;
            while (*start == ' ' && *start != '\0') start++;
            char *end = start + strlen(start) - 1;
            while (end > start && *end == ' ') end--;
            *(end + 1) = '\0';

            if (strlen(start) > 0) {
                Palabras[cantidadDeComandos] = strdup(start);
                if (!Palabras[cantidadDeComandos]) {
                    perror("strdup para Palabras[comando]");
                    // Liberar lo ya alocado en Palabras y copiaLecturaParaPipas
                    for(int k=0; k<cantidadDeComandos; ++k) if(Palabras[k]) free(Palabras[k]);
                    cantidadDeComandos = 0; // reset
                    free(copiaLecturaParaPipas);
                    goto next_command_loop; // Saltar al final del bucle while(1) para la siguiente iteración
                }
                cantidadDeComandos++;
            }
        }
        free(copiaLecturaParaPipas); // Liberar la copia usada para parsear pipas

        if (cantidadDeComandos == 0 && strlen(lecturaDeLaTerminal) > 0) { // Si no hubo pipas, es un solo comando
            // Quitar espacios de lecturaDeLaTerminal antes de strdup
            char *start_single = lecturaDeLaTerminal;
            while(*start_single == ' ' && *start_single != '\0') start_single++;
            char *end_single = start_single + strlen(start_single) - 1;
            while(end_single > start_single && *end_single == ' ') end_single--;
            *(end_single + 1) = '\0';

            if (strlen(start_single) > 0) {
                Palabras[0] = strdup(start_single);
                if (!Palabras[0]) {
                    perror("strdup para Palabras[0] single command");
                    goto next_command_loop;
                }
                cantidadDeComandos = 1;
            } else { // Si la línea solo tenía espacios
                goto next_command_loop;
            }
        }


        if (cantidadDeComandos > 0) {
            char *comando_principal_str = Palabras[0]; // No necesitamos otra copia para el primer comando si es para strtok
                                                       // Pero para seguridad, si Palabras[0] no debe ser modificado, copiar.
                                                       // strtok en 'instruccion' lo modificará.
            char *copia_comando_principal = NULL;
            if (comando_principal_str) {
                copia_comando_principal = strdup(comando_principal_str);
                if (!copia_comando_principal) {
                    perror("strdup copia_comando_principal");
                    goto next_command_loop;
                }
            } else { // No debería pasar si cantidadDeComandos > 0
                goto next_command_loop;
            }


            char *instruccion = strtok(copia_comando_principal, " "); // strtok modifica copia_comando_principal
            int ejecuto_interno = 0;

            if (instruccion == NULL) { // Comando era solo espacios
                if(copia_comando_principal) free(copia_comando_principal);
                goto next_command_loop;
            }

            // --- Comandos Internos ---
            if (strcmp(instruccion, "sjf") == 0) {
                ejecuto_interno = 1;
                if (cantidadDeComandos > 1) printf("sjf no soporta pipas. se ignoraran otros comandos.\n");
                sjf(&head, NOMBRE_ARCHIVO_PROCESOS);
            }
            else if (strcmp(instruccion, "fcfs") == 0) {
                ejecuto_interno = 1;
                if (cantidadDeComandos > 1) printf("fcfs no soporta pipas. se ignoraran otros comandos.\n");
                fcfs(&head, NOMBRE_ARCHIVO_PROCESOS);
            }
            else if (strcmp(instruccion, "rr") == 0) {
                ejecuto_interno = 1;
                if (cantidadDeComandos > 1) printf("rr no soporta pipas. se ignoraran otros comandos.\n");
                char *quantum_str = strtok(NULL, " ");
                int quantum_val = DEFAULT_QUANTUM;
                if (quantum_str) {
                    quantum_val = atoi(quantum_str);
                    if (quantum_val <= 0) quantum_val = DEFAULT_QUANTUM;
                }
                roundrobin(&head, NOMBRE_ARCHIVO_PROCESOS, quantum_val);
            }
            else if (strcmp(instruccion, "lstprocss") == 0) {
                ejecuto_interno = 1;
                if (cantidadDeComandos > 1) printf("lstprocss no soporta pipas. se ignoraran otros comandos.\n");
                imprimirLaLista(&head);
            }
            else if (strcmp(instruccion, "my_kill") == 0) {
                ejecuto_interno = 1;
                if (cantidadDeComandos > 1) printf("my_kill no soporta pipas. se ignoraran otros comandos.\n");
                char *nombre_proceso_a_matar = strtok(NULL, " ");
                if (nombre_proceso_a_matar) {
                    proceso_s *proc = buscarProcesoPorNombre(&head, nombre_proceso_a_matar);
                    if (proc) {
                        if (strcmp(proc->estado, "ready") == 0 ) { // Solo si está 'ready' tiene sentido liberar memoria
                            free_memoria_por_nombre(proc->nombreProceso);
                            proc->direccion_memoria = -1; // Marcar como no en memoria
                            // No cambiar a 'new' aquí, my_kill lo elimina de la lista de todos modos
                        }
                        eliminarPorID(&head, nombre_proceso_a_matar);
                        escribirEnDocumento(&head, NOMBRE_ARCHIVO_PROCESOS);
                    } else {
                        printf("my_kill: proceso '%s' no encontrado.\n", nombre_proceso_a_matar);
                    }
                } else {
                    printf("uso: my_kill <nombre_proceso>\n");
                }
            }
            else if (strcmp(instruccion, "mkprocess") == 0) {
                ejecuto_interno = 1;
                if (cantidadDeComandos > 1) printf("mkprocss no soporta pipas. se ignoraran otros comandos.\n");
                char *nombre = strtok(NULL, " ");
                char *burstTime_str = strtok(NULL, " ");
                char *bloques_str = strtok(NULL, " ");

                if (nombre && burstTime_str && bloques_str) {
                    if (existeElID(&head, nombre) == 0) {
                        proceso_s p_nuevo;
                        strncpy(p_nuevo.nombreProceso, nombre, sizeof(p_nuevo.nombreProceso) - 1);
                        p_nuevo.nombreProceso[sizeof(p_nuevo.nombreProceso) - 1] = '\0';
                        p_nuevo.burstTime = atoi(burstTime_str);
                        p_nuevo.bloque = atoi(bloques_str);
                        p_nuevo.ID = -99;
                        strcpy(p_nuevo.estado, "new");
                        p_nuevo.direccion_memoria = -1;
                        p_nuevo.burst_time_restante = p_nuevo.burstTime;
                        p_nuevo.tiempo_llegada_a_ready = 0;
                        p_nuevo.tiempo_finalizacion = 0;
                        p_nuevo.tiempo_espera_total = 0;
                        p_nuevo.tiempo_cpu_usado_rr = 0;

                        if (p_nuevo.burstTime <= 0 || p_nuevo.bloque <= 0) {
                            printf("error: burst time y bloques deben ser positivos.\n");
                        } else {
                            insertarAlFinal(&head, p_nuevo);
                            escribirEnDocumento(&head, NOMBRE_ARCHIVO_PROCESOS);
                            printf("proceso %s creado (estado: new, bt:%d, bloques:%d).\n", nombre, p_nuevo.burstTime, p_nuevo.bloque);
                        }
                    } else {
                        printf("error: ya existe un proceso con el nombre '%s'.\n", nombre);
                    }
                } else {
                    printf("uso: mkprocss <nombre> <burst_time> <bloques>\n");
                }
            }
            else if (strcmp(instruccion, "alloc") == 0) {
                ejecuto_interno = 1;
                if (cantidadDeComandos > 1) printf("alloc no soporta pipas. se ignoraran otros comandos.\n");
                char *nombre_proc = strtok(NULL, " ");
                char *estrategia = strtok(NULL, " ");
                if (nombre_proc && estrategia) {
                    proceso_s *proc = buscarProcesoPorNombre(&head, nombre_proc);
                    if (proc) {
                        if (strcmp(proc->estado, "new") == 0) {
                            if (alloc_memoria(proc, estrategia, &head) != -1) {
                                strcpy(proc->estado, "ready");
                                // Idealmente, aquí se registraría el tiempo_llegada_a_ready si hubiera un reloj global
                                escribirEnDocumento(&head, NOMBRE_ARCHIVO_PROCESOS);
                                printf("proceso %s movido a estado 'ready' y asignado a memoria.\n", nombre_proc);
                            } else {
                                printf("alloc: no se pudo asignar memoria para %s con estrategia %s.\n", nombre_proc, estrategia);
                            }
                        } else {
                             printf("alloc: proceso %s no esta en estado 'new' (estado actual: %s).\n", nombre_proc, proc->estado);
                        }
                    } else {
                        printf("alloc: proceso '%s' no encontrado.\n", nombre_proc);
                    }
                } else {
                    printf("uso: alloc <nombre_proceso> <first|best|worst>\n");
                }
            }
            else if (strcmp(instruccion, "free") == 0) {
                ejecuto_interno = 1;
                 if (cantidadDeComandos > 1) printf("free no soporta pipas. se ignoraran otros comandos.\n");
                char *nombre_proc = strtok(NULL, " ");
                if (nombre_proc) {
                    proceso_s *proc = buscarProcesoPorNombre(&head, nombre_proc);
                    if (proc) {
                        if (strcmp(proc->estado, "ready") == 0) {
                            free_memoria_por_nombre(proc->nombreProceso);
                            strcpy(proc->estado, "new");
                            proc->direccion_memoria = -1;
                            escribirEnDocumento(&head, NOMBRE_ARCHIVO_PROCESOS);
                            printf("proceso %s liberado de memoria y movido a estado 'new'.\n", nombre_proc);
                        } else {
                            printf("free: proceso %s no esta en estado 'ready' para ser liberado (estado: %s).\n", nombre_proc, proc->estado);
                        }
                    } else {
                        printf("free: proceso '%s' no encontrado.\n", nombre_proc);
                    }
                } else {
                    printf("uso: free <nombre_proceso>\n");
                }
            }
            else if (strcmp(instruccion, "mstatus") == 0) {
                ejecuto_interno = 1;
                if (cantidadDeComandos > 1) printf("mstatus no soporta pipas. se ignoraran otros comandos.\n");
                mstatus_memoria();
            }
            else if (strcmp(instruccion, "compact") == 0) {
                ejecuto_interno = 1;
                if (cantidadDeComandos > 1) printf("compact no soporta pipas. se ignoraran otros comandos.\n");
                compactar_memoria(&head);
                escribirEnDocumento(&head, NOMBRE_ARCHIVO_PROCESOS);
            }
            // --- Fin Comandos Internos ---

            if (copia_comando_principal) free(copia_comando_principal); // Liberar la copia del primer comando

            if (ejecuto_interno == 0) {
                if (cantidadDeComandos > 0) { // Debería ser siempre el caso aquí
                     ejecutarCodigo(cantidadDeComandos, Palabras);
                }
            }
        } // fin if (cantidadDeComandos > 0)

    next_command_loop: // Etiqueta para goto en caso de error de alocación de Palabras
        // Liberar Palabras[] antes de la siguiente iteración del bucle while(1)
        // Esto se hace al inicio del bucle ahora.

        if (lecturaDeLaTerminal) {
            free(lecturaDeLaTerminal);
            lecturaDeLaTerminal = NULL;
        }
    } // fin while(1)

    // Limpieza final
    // Liberar Palabras[] por si el bucle se rompió antes de la limpieza al inicio del bucle
    for(int i=0; i<6; ++i) {
        if(Palabras[i]) {
            free(Palabras[i]);
            Palabras[i] = NULL;
        }
    }
    if (lecturaDeLaTerminal) free(lecturaDeLaTerminal); // Por si se salió con 'exit' y no se liberó

    escribirEnDocumento(&head, NOMBRE_ARCHIVO_PROCESOS);
    limpiar_lista_al_salir(&head);
    limpiar_memoria_al_salir();

    return 0;
}