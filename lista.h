#ifndef LISTA_H // anadido ifndef
#define LISTA_H

//Creo que se debe cambiar la estructura de proceso para indicar en que punto de los bloques de memoria se encuentr
//Como int bloque 1-10, 30-50, algo asi,
typedef struct proceso
{
    char nombreProceso[50];
    int burstTime;
    int bloque;          // tamano en bloques que necesita
    char estado[30];     // Ready, y lo demas
    int ID;              // identificador numerico unico, generado secuencialmente o por el usuario
    int direccion_memoria; // direccion de inicio en memoria, -1 si no esta en memoria
    int burst_time_restante; // para round robin, cuanto tiempo de cpu le queda
    // para calculos de metricas
    int tiempo_llegada_a_ready; // tiempo en que el proceso entra a ready (para turnaround/wait)
    int tiempo_finalizacion;    // tiempo en que el proceso termina ejecucion
    int tiempo_espera_total;    // tiempo total que espero en la cola de listos
    int tiempo_cpu_usado_rr; // para rr, cuanto tiempo de cpu ha usado hasta ahora

} proceso_s;

typedef struct nodo
{
    proceso_s pid;
    struct nodo *sig; // Yo digo que no hay problema

} nodo_s;

//Regresa 1 si es cierto y 0 si no lo es
int existeElID(nodo_s **head, char* ID); // busca por nombreProceso
int existeElIDNumerico(nodo_s **head, int num_id); // busca por ID numerico
proceso_s* buscarProcesoPorNombre(nodo_s **head, const char *nombreProceso);
void imprimirLaLista(nodo_s **head);
void ordenarLista(nodo_s **head); // ordena por burst time (para sjf)
void obtenerElPrimerElemento(nodo_s **head);
//Mecanimos de insercion
void insertarEnLista(proceso_s mi_pid, nodo_s **head);
void insertarAlFinal(nodo_s **head, proceso_s proceso);
void insertarElementoEnPosicion(nodo_s **head, int pos, proceso_s mi_pid);
//Mecanismos de eliminacion
void eliminarElInicio(nodo_s **head);
void eliminarElementoEnPosicion(nodo_s **head, int pos);
void eliminarAlFinal(nodo_s **head);
void eliminarPorID(nodo_s **head, char * ID); // elimina por nombreProceso
//Mecanismos de lectura y escritura
void leerDeDocumento(nodo_s **head, char* nombreDelDocumento);
void escribirEnDocumento(nodo_s **head, char* nombreDelDocumento);
int agregarElementoAlDocumento(nodo_s **head,char* nombreDelDocumento); // obsoleto si usamos escribirEnDocumento siempre
int obtenerElTamano(nodo_s **head);
int obtenerNumeroProcesosReady(nodo_s **head); // cuenta procesos en estado "ready"

void limpiar_lista_al_salir(nodo_s **head); // para liberar memoria de la lista

#endif // LISTA_H