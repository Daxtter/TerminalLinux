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

#define READ_END 0
#define WRITE_END 1
#define NOMBRE_ARCHIVO_PROCESOS "guardarProcesos.bin"
// Tuve que declarar las funciones porque me marcaba error, no deberia marcaar error pero bueno
void crearElArgs(char *instrucciones, char *args[]);
void lecturaDeLaTerminalSuperEpico(char **lecturaDeLaTerminal);
void limpiarArgs(char *args[], int tamano);
void ejecutarCodigo(int numeroDePipas, char *instrucciones[]);
// ls -l -h | head -1 |wc -c
void ejecutarCodigo(int numeroDePipas, char *instrucciones[])
{
    char *args[15]; // Por si acaso alguien le mete 15 argumentos, no creo , pero podria pasar
    int pipas[5][2];
    pipe(pipas[0]);
    pipe(pipas[1]);
    pipe(pipas[2]);
    pipe(pipas[3]);
    pipe(pipas[4]);
    int PID_Multiples[5];
    int pipaEntrada = 0;
    int pipaSalida = 1;
    int pipaTemp;
    if (numeroDePipas > 1)
    {
        for (int i = 0; i < numeroDePipas; i++)
        {
            pipe(pipas[i]);
        }
        for (int i = 0; i < numeroDePipas; i++)
        {
            PID_Multiples[i] = fork();
            if (PID_Multiples[i] == 0)
            {
                // Crear los argumentos
                limpiarArgs(args, 15);
                crearElArgs(instrucciones[i], args);

                // no es la primera instrucciones
                if (i > 0)
                {
                    dup2(pipas[i - 1][READ_END], STDIN_FILENO);
                }

                // Se debe asegurar que la salida no este a ninguna pipe cuando sea la ultima porque entonces el resultado no se imprimira
                if (i < numeroDePipas - 1)
                {
                    dup2(pipas[i][WRITE_END], STDOUT_FILENO);
                }

                // Cerrar todas las pipes en el hijo
                for (int j = 0; j < numeroDePipas; j++)
                {
                    close(pipas[j][READ_END]);
                    close(pipas[j][WRITE_END]);
                }

                execvp(args[0], args);
                perror("execvp");
                exit(1);
            }
        }

        // se Asegura que cierre todas las pipas
        for (int i = 0; i < numeroDePipas; i++)
        {
            close(pipas[i][READ_END]);
            close(pipas[i][WRITE_END]);
        }

        for (int i = 0; i < numeroDePipas; i++)
        {
            waitpid(PID_Multiples[i], NULL, 0); // espera segun el PID dado
        }
    }
    else
    {
        // En caso de que solo se busque el hacer una cosa, podria pasar supongo.
        limpiarArgs(args, 15);
        crearElArgs(instrucciones[0], args);
        int PID_Unico = fork();
        if (PID_Unico == 0)
        {
            // Estamos en PID_Unico
            execvp(args[0], args);
        }
        else
        {
            wait(NULL); // Esperamos a que se acabe
        }
    }

    // char * primeraIntruccion[]= {instrucciones[0], , NULL}
}
// Verifica si tiene opciones para la ejecucion, un -help, por ejemplo
void crearElArgs(char *instrucciones, char *args[])
{ // el exec se detiene al primer NULL
    char *token = strtok(instrucciones, " ");
    int cantidadDeArgumentos = 0;
    while (token != NULL)
    {
        args[cantidadDeArgumentos] = malloc(strlen(token) + 1);
        strcpy(args[cantidadDeArgumentos], token);
        cantidadDeArgumentos++;
        token = strtok(NULL, " ");
    }
    args[cantidadDeArgumentos] = NULL;
    // printf("Se creo el args \n");
}
void limpiarArgs(char *args[], int tamano)
{
    for (int i = 0; i < tamano; i++)
    {
        args[i] = NULL;
    }
}

// Se usa el doble puntero, para poder cambiar el valor origial
// Esto debido a que se trabaja con el realloc ya que cambia el tamaño y todo eso
void lecturaDeLaTerminalSuperEpico(char **lecturaDeLaTerminal)
{
    char c;
    char tmp[2];
    strcpy(*lecturaDeLaTerminal, "\0");
    c = getchar();
    while (c != '\n' && c != EOF)
    {

        *lecturaDeLaTerminal = realloc(*lecturaDeLaTerminal, (strlen(*lecturaDeLaTerminal) + 2)); // Cambia el tamaño de la cadena para evitar una explosion multiversal
        tmp[0] = c;                                                                               // Esto es lo que le va a agregae
        tmp[1] = '\0';                                                                            // Esto es para decir que es el final de la cadena
        strcat(*lecturaDeLaTerminal, tmp);
        c = getchar();
    }
    tmp[0] = 0;
}

int main(void)
{
    //Recordatorio es muy recomendable usar como open, close y asi, funciones de linux
    nodo_s *head = NULL;
    leerDeDocumento(&head, NOMBRE_ARCHIVO_PROCESOS);
    //Creo que se deberia tener otra lista para determinar los bloques de memoria o algoa si
    char *Palabras[5]; // es en donde se guardara los comando
    // Ejemplo ls|head -1 -> palabras[0]=ls, palabras[1]= head -1
    int cantidadDePips = 0; // Describe la cantidad de | que hay

    while (1)
    {
        printf(LGREEN "\n manuelYGuillermo@PcGamerProRGB: " RESET);
        char *lecturaDeLaTerminal = malloc(1);
        lecturaDeLaTerminalSuperEpico(&lecturaDeLaTerminal);
        // printf("Lectura : %s\n",lecturaDeLaTerminal);
        char superTemporal[strlen(lecturaDeLaTerminal) + 1];
        strcpy(superTemporal, lecturaDeLaTerminal);
        superTemporal[strlen(lecturaDeLaTerminal) + 1] = '\0'; // Se crea una cadena de char porque al strtken no le gusta el char *
        // printf("SuperTemp: %s\n",superTemporal);
        char *token;
        token = strtok(superTemporal, "|");
        while (token != 0)
        {
            if (cantidadDePips < 6)

            {
                Palabras[cantidadDePips] = malloc(strlen(token) + 1); // Se debe apartar primero la memoria, porque no sabe cuanto va a recibir
                strcpy(Palabras[cantidadDePips], token);
                cantidadDePips++;
            }
            else
            {
                cantidadDePips = -1;
            }
            token = strtok(NULL, "|");
        }

        if (cantidadDePips <= -1)
        {
            printf("No se puede mayor a 5 pipas \n");
        }
        else
        {
            // Aqui se hace mas cosas porque el mkprocss espera parametros
            // Pero se puede usar esta logica para los otros comandos que vamos a utilzar
            char ingresarProceso[strlen(Palabras[0]) + 1];
            char *instruccion; // Verificar si esta bien escrita la instruccion
            strcpy(ingresarProceso, Palabras[0]);
            ingresarProceso[strlen(ingresarProceso) + 1] = '\0'; // Cerrar la cadena por si acaso, just in case
            instruccion = strtok(ingresarProceso, " ");
            int ejecuto = 0;
            // Se usara varios if para
            // evitar el super acordeon de if else
            // Se usa ademas para tener mejor redaccion
            if (strcmp(instruccion, "sfj") == 0)
            {
                ejecuto = 1;
                sjf(&head, NOMBRE_ARCHIVO_PROCESOS);
            }
            if (strcmp(instruccion, "fcfs") == 0)
            {
                ejecuto = 1;
                fcfs(&head, NOMBRE_ARCHIVO_PROCESOS);
            }
            if (strcmp(instruccion, "roundrobin") == 0)
            {
                ejecuto = 1;
                roundrobin(&head, NOMBRE_ARCHIVO_PROCESOS);
            }
            // En este caso el lstprocss,
            if (strcmp(instruccion, "lstprocss") == 0)
            {
                ejecuto = 1;
                imprimirLaLista(&head);
            }
            if (strcmp(instruccion, "mykill") == 0)
            {
                char *nombreConPuntero = strtok(NULL, " ");
                char nombre[50];
                strncpy(nombre, nombreConPuntero, sizeof(nombre) - 1);
                nombre[49] = '\0';
                ejecuto = 1;
                eliminarPorID(&head,nombre);
                escribirEnDocumento(&head,NOMBRE_ARCHIVO_PROCESOS);
            }
        
            if (strcmp(instruccion, "mkprocss") == 0)
            {

                ejecuto = 1;
                char *saveptr;
                // Logica del funcionamiento de mkprocss
                char *nombreConPuntero = strtok(NULL, " ");
                char nombre[50];
                strncpy(nombre, nombreConPuntero, sizeof(nombre) - 1);
                nombre[49] = '\0';
                char *burstTimeEnChar = strtok(NULL, " ");
                int burstTime = atoi(burstTimeEnChar);
                char *bloqueDeMemoriaEnChar = strtok(NULL, " ");
                int bloqueDeMemoria = atoi(bloqueDeMemoriaEnChar);

                if (nombre != NULL && burstTimeEnChar != NULL && bloqueDeMemoria != NULL)
                {
                    if (existeElID(&head, nombre) == 0)
                    {

                        proceso_s procesoAIngresar;
                        strncpy(procesoAIngresar.nombreProceso, nombre, sizeof(procesoAIngresar.nombreProceso) - 1);
                        procesoAIngresar.nombreProceso[sizeof(procesoAIngresar.nombreProceso) - 1] = '\0';
                        procesoAIngresar.burstTime = burstTime;
                        procesoAIngresar.bloque = bloqueDeMemoria;
                        procesoAIngresar.ID = -99;
                        //Para hacer las pruebas hice que fuera ready
                        //pero ready debe ser dado por alloc
                        strcpy(procesoAIngresar.estado, "new");
                        procesoAIngresar.estado[sizeof("new") + 1] = '\0';
                        int fd = open(NOMBRE_ARCHIVO_PROCESOS, O_CREAT | O_WRONLY | O_APPEND, S_IRWXO | S_IRWXU);
                        write(fd, &procesoAIngresar, sizeof(proceso_s));
                        close(fd);
                        insertarAlFinal(&head, procesoAIngresar);
                        printf("Ingresado satisfactoriamente. \n");
                    }
                    else
                    {
                        printf("Ya existe el ID");
                    }
                }
            }
            // Si no se ejecuto nada
            if (ejecuto == 0)
            {
                //Ejecuta las instrucciones en la terminal
                ejecutarCodigo(cantidadDePips, Palabras);
            }
            // Limpiarlo para asegurar
            char *token = strtok(NULL, " ");
            while (token != 0)
            {
                token = strtok(NULL, " ");
            }
        }

        free(lecturaDeLaTerminal);
        lecturaDeLaTerminal = NULL;
        cantidadDePips = 0;
    }
    return 0;
}
