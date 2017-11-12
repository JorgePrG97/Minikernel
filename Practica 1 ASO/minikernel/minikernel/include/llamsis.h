/*
 *  minikernel/kernel/include/llamsis.h
 *
 *  Minikernel. Versi�n 1.0
 *
 *  Fernando P�rez Costoya
 *
 */

/*
 *
 * Fichero de cabecera que contiene el numero asociado a cada llamada
 *
 * 	SE DEBE MODIFICAR PARA INCLUIR NUEVAS LLAMADAS
 *
 */

#ifndef _LLAMSIS_H
#define _LLAMSIS_H

/* Numero de llamadas disponibles */
#define NSERVICIOS 11

#define CREAR_PROCESO 0
#define TERMINAR_PROCESO 1
#define ESCRIBIR 2
#define OBTENER_ID 3
#define SIS_DORMIR 4
#define SIS_LEER_CARACTER 5
#define SIS_CREAR_MUTEX 6
#define SIS_ABRIR_MUTEX 7
#define SIS_LOCK 8
#define SIS_UNLOCK 9
#define SIS_CERRAR_MUTEX 10



#endif /* _LLAMSIS_H */
