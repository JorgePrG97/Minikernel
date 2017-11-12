/*
 *  minikernel/include/kernel.h
 *
 *  Minikernel. Versi�n 1.0
 *
 *  Fernando P�rez Costoya
 *
 */

/*
 *
 * Fichero de cabecera que contiene definiciones usadas por kernel.c
 *
 *      SE DEBE MODIFICAR PARA INCLUIR NUEVA FUNCIONALIDAD
 *
 */

#ifndef _KERNEL_H
#define _KERNEL_H

#include "const.h"
#include "HAL.h"
#include "llamsis.h"


/*
 * MUTEX
 */

/*
 * Definicion de un mutex
 */
typedef struct{
  char *nombre; 	                       // nombre del mutex
	int tipo;		                           // tipo del mutex (no recursivo = 0, recursivo = 1)
	int estado;                            // 0 libre, 1 ocupado
  int num_bloqueos;
  int num_procesos_bloqueados;
  int procesosMutex[MAX_PROC];           // procesos que tienen un mutex abierto
} mutex;

/*
 * Variable global que representa la tabla de mutex
 */

mutex tabla_mutex[NUM_MUT];

/*
 * Variable que controla el numero de mutex en el sistema
 */

int num_mut = 0;

/*
 *
 * Definicion del tipo que corresponde con el BCP.
 * Se va a modificar al incluir la funcionalidad pedida.
 *
 */
typedef struct BCP_t *BCPptr;

typedef struct BCP_t {
        int id;				                           /* ident. del proceso */
        int estado;			                         /* TERMINADO|LISTO|EJECUCION|BLOQUEADO*/
        contexto_t contexto_regs;	               /* copia de regs. de UCP */
        void * pila;			                       /* dir. inicial de la pila */
	      BCPptr siguiente;		                     /* puntero a otro BCP */
	      void *info_mem;			                     /* descriptor del mapa de memoria */
	      unsigned int msegundos;
	      unsigned int mticks;
	      int tiempo_rodaja;
        mutex *descriptores_mutex[NUM_MUT_PROC];    /* numero identificador del mutex */
        int boolean[NUM_MUT_PROC];                /* 1 posicion ocupada, 0 posicion libre */
        int nMutex;                              /* numero de mutex del proceso */
} BCP;

/*
 *
 * Definicion del tipo que corresponde con la cabecera de una lista
 * de BCPs. Este tipo se puede usar para diversas listas (procesos listos,
 * procesos bloqueados en sem�foro, etc.).
 *
 */

typedef struct{
	BCP *primero;
	BCP *ultimo;
} lista_BCPs;

char buffer[TAM_BUF_TERM];
int num_car=0;
int escribir=0;
/*
 * Variable global que identifica el proceso actual
 */

BCP * p_proc_actual=NULL;

/*
 * Variable global que representa la tabla de procesos
 */

BCP tabla_procs[MAX_PROC];

/*
 *
 */

/*
 * Variables globales que representan:
 *    la cola de procesos listos
 *    la cola de procesos bloqueados
 *    la cola de procesos para lectura
 *    la cola de procesos bloqueados por el mutex
 */
lista_BCPs lista_listos= {NULL, NULL};
lista_BCPs lista_bloqueados={NULL,NULL};
lista_BCPs lista_lectura={NULL,NULL};
lista_BCPs lista_bloqueados_mutex_libre={NULL, NULL};
lista_BCPs lista_mutex={NULL,NULL};
/*
 *
 * Definici�n del tipo que corresponde con una entrada en la tabla de
 * llamadas al sistema.
 *
 */
typedef struct{
	int (*fservicio)();
} servicio;



/*
 * Prototipos de las rutinas que realizan cada llamada al sistema
 */
int sis_crear_proceso();
int sis_terminar_proceso();
int sis_escribir();
int obtener_id_pr();
int sis_dormir();
int sis_leer_caracter();
int sis_crear_mutex();
int sis_abrir_mutex();
int sis_lock();
int sis_unlock();
int sis_cerrar_mutex();
/*
 * Variable global que contiene las rutinas que realizan cada llamada
 */
servicio tabla_servicios[NSERVICIOS]={	{sis_crear_proceso},
					{sis_terminar_proceso},
					{sis_escribir},
					{obtener_id_pr},
					{sis_dormir},
					{sis_leer_caracter},
          {sis_crear_mutex},
          {sis_abrir_mutex},
          {sis_lock},
          {sis_unlock},
          {sis_cerrar_mutex}};

#endif /* _KERNEL_H */
