/*
 *  kernel/kernel.c
 *
 *  Minikernel. Versi�n 1.0
 *
 *  Fernando P�rez Costoya
 *
 */

/*
 *
 * Fichero que contiene la funcionalidad del sistema operativo
 *
 */

#include "kernel.h"	/* Contiene defs. usadas por este modulo */
#include <string.h>
#include <stdlib.h>

/*
 *
 * Funciones relacionadas con la tabla de procesos:
 *	iniciar_tabla_proc buscar_BCP_libre
 *
 */

/*
 * Funci�n que inicia la tabla de procesos
 */
int obtener_id_pr(){
	return p_proc_actual->id;
}

static void iniciar_tabla_proc(){
	int i;

	for (i=0; i<MAX_PROC; i++)
		tabla_procs[i].estado=NO_USADA;
}
/*
 * Funci�n que busca una entrada libre en la tabla de procesos
 */
static int buscar_BCP_libre(){
	int i;

	for (i=0; i<MAX_PROC; i++)
		if (tabla_procs[i].estado==NO_USADA)
			return i;
	return -1;
}

/*
 *
 * Funciones que facilitan el manejo de las listas de BCPs
 *	insertar_ultimo eliminar_primero eliminar_elem
 *
 * NOTA: PRIMERO SE DEBE LLAMAR A eliminar Y LUEGO A insertar
 */

/*
 * Inserta un BCP al final de la lista.
 */
static void insertar_ultimo(lista_BCPs *lista, BCP * proc){
	if (lista->primero==NULL){
		lista->primero= proc;
	} else {
		// SI PETA AQUI QUITAR EL IF ELSE Y QUEDARSE SOLO CON LA PARTE DEL IF QUE ERA LA ORIGINAL
		if(lista->ultimo!=NULL)
			lista->ultimo->siguiente=proc;
		else
			lista->ultimo=proc;
	}
	lista->ultimo= proc;
	proc->siguiente=NULL;
}

/*
 * Elimina el primer BCP de la lista.
 */
static void eliminar_primero(lista_BCPs *lista){

	if (lista->ultimo==lista->primero)
		lista->ultimo=NULL;
	lista->primero=lista->primero->siguiente;
}

/*
 * Elimina un determinado BCP de la lista.
 */
static void eliminar_elem(lista_BCPs *lista, BCP * proc){
	BCP *paux=lista->primero;

	if (paux==proc)
		eliminar_primero(lista);
	else {
		for ( ; ((paux) && (paux->siguiente!=proc));
			paux=paux->siguiente);
		if (paux) {
			if (lista->ultimo==paux->siguiente)
				lista->ultimo=paux;
			paux->siguiente=paux->siguiente->siguiente;
		}
	}
}

/*
 *
 * Funciones relacionadas con la planificacion
 *	espera_int planificador
 */

/*
 * Espera a que se produzca una interrupcion
 */
static void espera_int(){
	int nivel;

	//printk("-> NO HAY LISTOS. ESPERA INT\n");

	/* Baja al m�nimo el nivel de interrupci�n mientras espera */
	nivel=fijar_nivel_int(NIVEL_1);
	halt();
	fijar_nivel_int(nivel);
}

/*
 * Funci�n de planificacion que implementa un algoritmo FIFO.
 */
static BCP * planificador(){
	while (lista_listos.primero==NULL)
		espera_int();		/* No hay nada que hacer */
	return lista_listos.primero;
}

/*
 *
 * Funcion auxiliar que termina proceso actual liberando sus recursos.
 * Usada por llamada terminar_proceso y por rutinas que tratan excepciones
 *
 */
static void liberar_proceso(){
	BCP * p_proc_anterior;

	liberar_imagen(p_proc_actual->info_mem); /* liberar mapa */

	p_proc_actual->estado=TERMINADO;
	eliminar_primero(&lista_listos); /* proc. fuera de listos */

	/* Realizar cambio de contexto */
	p_proc_anterior=p_proc_actual;
	p_proc_actual=planificador();

	printk("-> C.CONTEXTO POR FIN: de %d a %d\n",
			p_proc_anterior->id, p_proc_actual->id);

	liberar_pila(p_proc_anterior->pila);
	cambio_contexto(NULL, &(p_proc_actual->contexto_regs));
        return; /* no deber�a llegar aqui */
}

/*
 *
 * Funciones relacionadas con el tratamiento de interrupciones
 *	excepciones: exc_arit exc_mem
 *	interrupciones de reloj: int_reloj
 *	interrupciones del terminal: int_terminal
 *	llamadas al sistemas: llam_sis
 *	interrupciones SW: int_sw
 *
 */

/*
 * Tratamiento de excepciones aritmeticas
 */
static void exc_arit(){

	if (!viene_de_modo_usuario())
		panico("excepcion aritmetica cuando estaba dentro del kernel");


	printk("-> EXCEPCION ARITMETICA EN PROC %d\n", p_proc_actual->id);
	liberar_proceso();

        return; /* no deber�a llegar aqui */
}

/*
 * Tratamiento de excepciones en el acceso a memoria
 */
static void exc_mem(){
	if (!viene_de_modo_usuario())
		panico("excepcion de memoria cuando estaba dentro del kernel");

	printk("-> EXCEPCION DE MEMORIA EN PROC %d\n", p_proc_actual->id);
	liberar_proceso();

        return; /* no deber�a llegar aqui */
}


/*
 * Tratamiento de interrupciuones software
 */

static void int_sw(){
	//printk("-> TRATANDO INT. SW\n");
	int nivel=fijar_nivel_int(NIVEL_1);
	BCP *aux=p_proc_actual;
	p_proc_actual->tiempo_rodaja=TICKS_POR_RODAJA;
	if(p_proc_actual->siguiente!=NULL){
		eliminar_primero(&lista_listos);
		insertar_ultimo(&lista_listos,aux);
	}
	p_proc_actual=planificador();
	//printk("FIN RODAJA: C.contexto DE %d a %d \n",id_antiguo,id_actual);
	fijar_nivel_int(nivel);
	cambio_contexto(&(aux->contexto_regs), &(p_proc_actual->contexto_regs));
	return;
}
/*
 * Tratamiento de interrupciones de reloj
 */
static void int_reloj(){
	int nivel=fijar_nivel_int(NIVEL_3);
	//printk("-> TRATANDO INT. DE RELOJ\n");
	if(p_proc_actual->tiempo_rodaja==0){

		activar_int_SW();
	}
	else{
		p_proc_actual->tiempo_rodaja--;
	}
	BCPptr recorrido=lista_bloqueados.primero;
	while(recorrido!=NULL){
		recorrido->msegundos=recorrido->msegundos-1;
		if(recorrido->msegundos==0){
			eliminar_elem(&lista_bloqueados,recorrido);
			insertar_ultimo(&lista_listos,recorrido);
		}
		recorrido=recorrido->siguiente;
	}
	fijar_nivel_int(nivel);
	return;
}



/*
 * Tratamiento de llamadas al sistema
 */
static void tratar_llamsis(){
	int nserv, res;

	nserv=leer_registro(0);
	if (nserv<NSERVICIOS)
		res=(tabla_servicios[nserv].fservicio)();
	else
		res=-1;		/* servicio no existente */
	escribir_registro(0,res);
	return;
}


/*
 *
 * Funcion auxiliar que crea un proceso reservando sus recursos.
 * Usada por llamada crear_proceso.
 *
 */
static int crear_tarea(char *prog){
	void * imagen, *pc_inicial;
	int error=0;
	int proc;
	BCP *p_proc;

	proc=buscar_BCP_libre();
	if (proc==-1)
		return -1;	/* no hay entrada libre */

	/* A rellenar el BCP ... */
	p_proc=&(tabla_procs[proc]);

	/* crea la imagen de memoria leyendo ejecutable */
	imagen=crear_imagen(prog, &pc_inicial);
	if (imagen)
	{
		p_proc->info_mem=imagen;
		p_proc->pila=crear_pila(TAM_PILA);
		fijar_contexto_ini(p_proc->info_mem, p_proc->pila, TAM_PILA,
			pc_inicial,
			&(p_proc->contexto_regs));
		p_proc->id=proc;
		p_proc->estado=LISTO;
		p_proc->msegundos=0;
		p_proc->tiempo_rodaja=TICKS_POR_RODAJA;
		/* lo inserta al final de cola de listos */
		insertar_ultimo(&lista_listos, p_proc);
		error= 0;
	}
	else
		error= -1; /* fallo al crear imagen */

	return error;
}

/*
 *
 * Rutinas que llevan a cabo las llamadas al sistema
 *	sis_crear_proceso sis_escribir
 *
 */

/*
 * Tratamiento de llamada al sistema crear_proceso. Llama a la
 * funcion auxiliar crear_tarea sis_terminar_proceso
 */
int sis_crear_proceso(){
	char *prog;
	int res;

	printk("-> PROC %d: CREAR PROCESO\n", p_proc_actual->id);
	prog=(char *)leer_registro(1);
	res=crear_tarea(prog);
	return res;
}

/*
 * Tratamiento de llamada al sistema escribir. Llama simplemente a la
 * funcion de apoyo escribir_ker
 */
int sis_escribir()
{
	char *texto;
	unsigned int longi;

	texto=(char *)leer_registro(1);
	longi=(unsigned int)leer_registro(2);

	escribir_ker(texto, longi);
	return 0;
}

/*
 * Tratamiento de llamada al sistema terminar_proceso. Llama a la
 * funcion auxiliar liberar_proceso
 */
int sis_terminar_proceso(){

	printk("-> FIN PROCESO %d\n", p_proc_actual->id);

	liberar_proceso();

        return 0; /* no deber�a llegar aqui */
}
/*Funcion sis_dormir*/
int sis_dormir(){
		int aux2=leer_registro(1);
		BCPptr aux=p_proc_actual;
		p_proc_actual->msegundos=aux2*TICK;
		eliminar_primero(&lista_listos);
		insertar_ultimo(&lista_bloqueados,aux);
		p_proc_actual=planificador();
		cambio_contexto(&(aux->contexto_regs),&(p_proc_actual->contexto_regs));
	return 0;
}

int sis_leer_caracter(){
	while(1){
		if (num_car==0){
			int nivel=fijar_nivel_int(NIVEL_3);
			BCP *aux=p_proc_actual;
			eliminar_elem(&lista_listos, p_proc_actual);
			insertar_ultimo(&lista_lectura,aux);
			fijar_nivel_int(nivel);
			nivel=fijar_nivel_int(NIVEL_2);
			p_proc_actual=planificador();
			cambio_contexto(&(aux->contexto_regs),&(p_proc_actual->contexto_regs));
			fijar_nivel_int(nivel);
		}else{
			int i;
			// Solicita el primer caracter del buffer
			int nivel= fijar_nivel_int(NIVEL_2);
			char car = buffer[0];
			num_car--;

			// Reordena el buffer
			for (i = 0; i < num_car; i++){
				buffer[i] = buffer[i+1];
			}
			fijar_nivel_int(nivel);
			return (int)car;
		}
	}
}

/*
 * Tratamiento de interrupciones de terminal
 */

static void int_terminal(){

	char car;
	car = leer_puerto(DIR_TERMINAL);
	printk("-> TRATANDO INT. DE TERMINAL %c\n", car);
	if(num_car<TAM_BUF_TERM){
		buffer[num_car]=car;
		num_car++;
		BCP *aux=lista_lectura.primero;
		if(aux!=NULL){
			int nivel=fijar_nivel_int(NIVEL_3);
			eliminar_primero(&lista_lectura);
			insertar_ultimo(&lista_listos,aux);
			fijar_nivel_int(nivel);
		}

	}
        return;
}

/*
 *	Conjunto de funciones auxiliares para la creacion de mutex
 */

 static void iniciar_tabla_mutex(){
 	int i;
 	for(i = 0; i < NUM_MUT; i++){
 		tabla_mutex[i].estado = 0; /* indica que el mutex esta libre */
 	}
 }

 static int buscar_descriptor_libre(){
 	int i;
 	for(i = 0; i < NUM_MUT_PROC; i++){
 		if(p_proc_actual->descriptores_mutex[i] == NULL)
 			return i; /* devuelve el numero del descriptor */
 	}
 	return -1; /* no hay descriptor libre */
 }

 static int buscar_nombre_mutex(char *nombre_mutex){
 	int i;
 	for(i = 0; i < NUM_MUT; i++){
		if(tabla_mutex[i].nombre == NULL){
			tabla_mutex[i].nombre = malloc(strlen(nombre_mutex)+1);
		}
 		if(strcmp((char *)tabla_mutex[i].nombre, nombre_mutex) == 0){
 			return i;	/* el nombre existe y devuelve su posicion en la tabla de mutex */
		}
 	}
 	return -1; /* el nombre no existe */
 }

 static int buscar_mutex_libre(){
 	int i;
 	for(i = 0; i < NUM_MUT; i++){
 		if(tabla_mutex[i].estado == 0)
 			return i;	/* el mutex esta libre y devuelve su posicion en la tabla de mutex */
 	}
 	return -1; /* no hay mutex libre */
 }

void inicializar_mutex(char* nombre, int tipo, int pos_mutex_libre, int estado) {

}

 /*
  * Llamada al sistema crear_mutex
  * parametro nombre en registro 1
  * parametro tipo en registro 2
  */
	int sis_crear_mutex(){
  	char * nombre = (char*)leer_registro(1);
  	int tipo = (int)leer_registro(2);
  	if(strlen(nombre) > MAX_NOM_MUT){
  		printk("(SIS_CREAR_MUTEX) Error: Nombre de mutex demasiado largo\n");
  		return -1;
  	}
  	if(buscar_nombre_mutex(nombre) >= 0){
  		printk("(SIS_CREAR_MUTEX) Error: Nombre de mutex ya existente\n");
  		return -2;
  	}
  	int descr_mutex_libre = buscar_descriptor_libre();
  	if(descr_mutex_libre < 0){
  		printk("(SIS_CREAR_MUTEX) Error: No hay descriptores de mutex libres\n");
  		return -3;
  	}else{
  		printk("(SIS_CREAR_MUTEX) Devolviendo descriptor numero %d\n", descr_mutex_libre);
  	}
  	int pos_mutex_libre = buscar_mutex_libre();

  	if(pos_mutex_libre < 0){
  		printk("(SIS_CREAR_MUTEX) No hay mutex libre, bloqueando proceso\n");
  		// bloquear a la espera de que quede alguno libre
  		BCP * proc_a_bloquear=p_proc_actual;
  		proc_a_bloquear->estado=BLOQUEADO;
  		int nivel_int = fijar_nivel_int(3);
  		//printk("(SIS_CREAR_MUTEX) Eliminando proceso con PID %d de la cola de listos\n", proc_a_bloquear->id);
  		eliminar_primero(&lista_listos);
  		//printk("(SIS_CREAR_MUTEX) Insertando proceso con PID %d en la lista de bloqueados mutex libre\n", proc_a_bloquear-> id);
  		insertar_ultimo(&lista_bloqueados_mutex_libre, proc_a_bloquear);
  		p_proc_actual=planificador();
  		fijar_nivel_int(nivel_int);
  		cambio_contexto(&(proc_a_bloquear->contexto_regs),
  			 	&(p_proc_actual->contexto_regs));
  	}else{
			if(tabla_mutex[pos_mutex_libre].nombre == NULL){
				tabla_mutex[pos_mutex_libre].nombre = malloc(strlen(nombre)+1);
			}
  		strcpy(tabla_mutex[pos_mutex_libre].nombre, nombre); // se copia el nombre al mutex
  		tabla_mutex[pos_mutex_libre].tipo = tipo; // se asigna el tipo
  		tabla_mutex[pos_mutex_libre].estado = 1; // se cambia el estado a OCUPADO
  		tabla_mutex[pos_mutex_libre].num_bloqueos = 0;
  		tabla_mutex[pos_mutex_libre].num_procesos_bloqueados = 0;
  		p_proc_actual->descriptores_mutex[descr_mutex_libre] = &tabla_mutex[pos_mutex_libre]; // el descr apunta al mutex libre en la tabla
  	}
  	return descr_mutex_libre;
  }

 int sis_abrir_mutex(){
	 char* nombre = (char *)leer_registro(1);

	 //COMPROBAR CONDICIONES DE APERTURA DEL MUTEX: no haber alcanzado ni el numero max de mutex por proceso ni en general
	 if(p_proc_actual->nMutex == NUM_MUT_PROC || num_mut == NUM_MUT) {
		 return -1;
	 }

	 // SI ESTAMOS AQUI ES POSIBLE ABRIR EL MUTEX
	 // BUSCAMOS EL MUTEX EN LA LISTA: tabla_mutex
	 int mutex_elegido = buscar_nombre_mutex(nombre);

	 if(mutex_elegido >= 0) {
		 //marcamos el proceso dentro de nuestra tabla_mutex
		 tabla_mutex[mutex_elegido].procesosMutex[p_proc_actual->id] = 1;
	 } else {
		 return -1;
	 }

	 // BUSCAMOS UN DESCRIPTOR LIBRE PARA EL MUTEX
	 int descr_mutex = buscar_descriptor_libre();

	 if(descr_mutex != -1) {
		 p_proc_actual->descriptores_mutex[descr_mutex] = &tabla_mutex[mutex_elegido];
		 p_proc_actual->boolean[descr_mutex] = 1;
		 p_proc_actual->nMutex++;
	 } else {
		 return -1;
	 }

	 return descr_mutex;

 }

int sis_lock() {
	unsigned int mutexid = (unsigned int)leer_registro(1);

	if(p_proc_actual->descriptores_mutex[mutexid] == NULL){
		return -1;
	}



	// BUSCAMOS EL MUTEX EN LA TABLA PARA PODER SABER SI ERA RECURSIVO O NO Y MODIFICAR LAS VARIABLES CORRESPONDIENTES
	if(p_proc_actual->descriptores_mutex[mutexid]->estado == 0) {
		return -1;
	}
	int pos_tabla_mutex = buscar_nombre_mutex(p_proc_actual->descriptores_mutex[mutexid]->nombre);
	if(tabla_mutex[pos_tabla_mutex].tipo == 0) {
		return -1;
	}
	// AUMENTAMOS TANTO EL NUMERO DE BLOQUEOS PRODUCIDOS POR EL MUTEX COMO EL NUMERO DE PROCESOS BLOQUEADOS POR DICHO MUTEX
	tabla_mutex[pos_tabla_mutex].num_bloqueos++;
	tabla_mutex[pos_tabla_mutex].num_procesos_bloqueados++;
	// CAMBIAMOS EL ESTADO DEL MUTEX A BLOQUEADO Y AUMENTAMOS EL NUMERO DE MUTEX TOTALES
	p_proc_actual->descriptores_mutex[mutexid]->estado = 1;
	num_mut++;
	// ELIMINAMOS EL PROCESO DE LA LISTA DE LISTOS Y LO INCORPORAMOS AL FINAL DE LA LISTA DE MUTEX
	BCPptr p_proc_elim = p_proc_actual;
	int nivel = fijar_nivel_int(NIVEL_3);
	eliminar_elem(&lista_listos, p_proc_elim);
	insertar_ultimo(&lista_mutex, p_proc_elim);
	fijar_nivel_int(nivel);
	return 0;
}

int sis_unlock() {
	unsigned int mutexid = (unsigned int)leer_registro(1);

	int pos_tabla_mutex = buscar_nombre_mutex(p_proc_actual->descriptores_mutex[mutexid]->nombre);
	mutex mutexComprobar = tabla_mutex[pos_tabla_mutex];

	BCPptr procesoComprobar = lista_mutex.primero;
	BCPptr procesoSiguiente = NULL;
	if(procesoComprobar != NULL)
		procesoSiguiente = procesoComprobar->siguiente;


	while(procesoComprobar != NULL) {
		procesoComprobar->estado = LISTO;
		int nivel = fijar_nivel_int(NIVEL_3);
		int i=0;
		for(i=0; i<MAX_PROC; i++) {
			if(strcmp(procesoComprobar->descriptores_mutex[i]->nombre, mutexComprobar.nombre)) {
				eliminar_elem(&lista_mutex, procesoComprobar);
				insertar_ultimo(&lista_listos, procesoComprobar);
				break;
			}
		}

		fijar_nivel_int(nivel);
		procesoComprobar = procesoSiguiente;
		if(procesoComprobar != NULL)
			procesoSiguiente = procesoComprobar->siguiente;
	}

	return 0;
}


int sis_cerrar_mutex() {
	int mutexid = (int)leer_registro(1);

	int pos_tabla_mutex = buscar_nombre_mutex(p_proc_actual->descriptores_mutex[mutexid]->nombre);
	mutex mutexComprobar = tabla_mutex[pos_tabla_mutex];

	BCPptr procesoComprobar = lista_mutex.primero;
	BCPptr procesoSiguiente = NULL;
	if(procesoComprobar != NULL)
		procesoSiguiente = procesoComprobar->siguiente;


	while(procesoComprobar != NULL) {
		procesoComprobar->estado = LISTO;
		int nivel = fijar_nivel_int(NIVEL_3);
		int i=0;
		for(i=0; i<MAX_PROC; i++) {
			if(strcmp(procesoComprobar->descriptores_mutex[i]->nombre, mutexComprobar.nombre)) {
				eliminar_elem(&lista_mutex, procesoComprobar);
				insertar_ultimo(&lista_listos, procesoComprobar);
				break;
			}
		}

		fijar_nivel_int(nivel);
		procesoComprobar = procesoSiguiente;
		if(procesoComprobar != NULL)
			procesoSiguiente = procesoComprobar->siguiente;
	}

	// aqui termina



	if(p_proc_actual->descriptores_mutex[mutexid]->num_procesos_bloqueados == 0){
		BCPptr procesoALiberar = lista_bloqueados_mutex_libre.primero;
		BCPptr procesoSiguiente = NULL;
		if(procesoALiberar != NULL)
			procesoSiguiente = procesoALiberar->siguiente;

		while(procesoALiberar != NULL) {
			procesoALiberar->estado = LISTO;
			int nivel = fijar_nivel_int(NIVEL_3);
			eliminar_elem(&lista_bloqueados_mutex_libre, procesoALiberar);
			insertar_ultimo(&lista_listos, procesoALiberar);
			fijar_nivel_int(nivel);

			procesoALiberar = procesoSiguiente;
			if(procesoALiberar != NULL)
				procesoSiguiente = procesoALiberar->siguiente;
		}
		if(p_proc_actual->descriptores_mutex[mutexid]->estado == 1) {
			int posElim = buscar_nombre_mutex(p_proc_actual->descriptores_mutex[mutexid]->nombre);
			tabla_mutex[posElim].estado = 0; // se cambia el estado a LIBRE
		}

	}

	// BORRAMOS EL MUTEX DEL ARRAY DE DESCRIPTORES
	p_proc_actual->descriptores_mutex[mutexid] = NULL;



	return 0;

}


/*
 *
 * Rutina de inicializacion invocada en arranque
 *
 */
int main(){
	/* se llega con las interrupciones prohibidas */

	instal_man_int(EXC_ARITM, exc_arit);
	instal_man_int(EXC_MEM, exc_mem);
	instal_man_int(INT_RELOJ, int_reloj);
	instal_man_int(INT_TERMINAL, int_terminal);
	instal_man_int(LLAM_SIS, tratar_llamsis);
	instal_man_int(INT_SW, int_sw);

	iniciar_cont_int();		/* inicia cont. interr. */
	iniciar_cont_reloj(TICK);	/* fija frecuencia del reloj */
	iniciar_cont_teclado();		/* inici cont. teclado */

	iniciar_tabla_proc();		/* inicia BCPs de tabla de procesos */
	iniciar_tabla_mutex();  /* inicia tabla de mutex */

	/* crea proceso inicial */
	if (crear_tarea((void *)"init")<0)
		panico("no encontrado el proceso inicial");

	/* activa proceso inicial */
	p_proc_actual=planificador();
	cambio_contexto(NULL, &(p_proc_actual->contexto_regs));
	panico("S.O. reactivado inesperadamente");
	return 0;
}
