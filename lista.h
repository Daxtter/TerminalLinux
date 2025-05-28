

//Creo que se debe cambiar la estructura de proceso para indicar en que punto de los bloques de memoria se encuentr
//Como int bloque 1-10, 30-50, algo asi,
typedef struct proceso
{
    char nombreProceso[50];
    int burstTime;
    int bloque;
    char estado[30]; //Ready, y lo demas
    int ID;
} proceso_s;
typedef struct nodo
{
    proceso_s pid;
    struct nodo *sig; // Yo digo que no hay problema

} nodo_s;

//Regresa 1 si es cierto y 0 si no lo es
int existeElID(nodo_s **head, char* ID);
void imprimirLaLista(nodo_s **head);
void ordenarLista(nodo_s **head);
void obtenerElPrimerElemento(nodo_s **head);
//Mecanimos de insercion
void insertarEnLista(proceso_s mi_pid, nodo_s **head);
void insertarAlFinal(nodo_s **head, proceso_s proceso);
void insertarElementoEnPosicion(nodo_s **head, int pos, proceso_s mi_pid);
//Mecanismos de eliminacion
void eliminarElInicio(nodo_s **head);
void eliminarElementoEnPosicion(nodo_s **head, int pos);
void eliminarAlFinal(nodo_s **head);
void eliminarPorID(nodo_s **head, char * ID);
//Mecanismos de lectura y escritura
void leerDeDocumento(nodo_s **head, char* nombreDelDocumento);
void escribirEnDocumento(nodo_s **head, char* nombreDelDocumento);
int agregarElementoAlDocumento(nodo_s **head,char* nombreDelDocumento);
int obtenerElTamano(nodo_s **head);