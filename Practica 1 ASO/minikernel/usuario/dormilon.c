/*
 * usuario/dormilon.c
 *
 *  Minikernel. Versión 1.0
 *
 *  Fernando Pérez Costoya
 *
 */

/*
 * Programa de usuario que duerme dos veces.
 */

#include "servicios.h"

int main(){
	int segs, id;

	id=obtener_id_pr();
	printf("dormilon (%d): comienza\n", id);

	/* primero duerme 1 segundo */
	printf("dormilon (%d) duerme 1 segundo\n", id);
	dormir(1);
	printf("dormilon (%d) vuelve de dormir 1 segundo\n", id);
	/* despues duerme numero de segundos dependiendo de su pid */
	segs=id+1;
	printf("dormilon (%d) duerme %d segundos\n", id, segs);
	dormir(segs);
	printf("dormilon (%d) vuelve de dormir %d segundos\n", id, segs);

	printf("dormilon (%d): termina\n", id);
	return 0;
}
