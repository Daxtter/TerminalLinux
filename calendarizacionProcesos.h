#ifndef CALENDARIZACION_PROCESOS_H // anadido ifndef
#define CALENDARIZACION_PROCESOS_H

#include "lista.h"


void sjf(nodo_s **head, char *nombreDelDocumento);
void roundrobin(nodo_s **head, char *nombreDelDocumento, int quantum); // anadido quantum
void fcfs(nodo_s **head, char *nombreDelDocumento);

#endif // CALENDARIZACION_PROCESOS_H