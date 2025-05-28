
typedef struct proceso
{
    char nombreProceso[50];
    int burstTime;
    int ID;
} proceso_s;
typedef struct nodo
{
    proceso_s pid;
    struct nodo *sig; // Yo digo que no hay problema

} nodo_s;

void imprimirLaLista(nodo_s **head);
// Aqui no deberia estar el de ordenar pero queria ordenarla
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
//Mecanismos de lectura y escritura
void leerDeDocumento(nodo_s **head, char* nombreDelDocumento);
void escribirEnDocumento(nodo_s **head, char* nombreDelDocumento);
int obtenerElTamano(nodo_s **head);