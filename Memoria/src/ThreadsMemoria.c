#include "ThreadsMemoria.h"

uint32_t Hash(uint32_t x, uint32_t y){
	x--;
	uint32_t r = ((x + y) * (x + y + 1)) / 2 + y;
	while(r>MARCOS)
		r -= MARCOS; //cota superior
	int d = r - MarcosEstructurasAdm;
	if(d<0) r-=d; //cota inferior

	return r;
}
uint32_t FrameLookup(uint32_t pid, uint32_t pag){
	uint32_t frame = Hash(pid, pag);
	while(TablaDePagina[frame].PID != pid || TablaDePagina[frame].Pag != pag){
		frame++;
		if(frame>=MARCOS)
			frame -= MARCOS;
	}
	return frame;
}
uint32_t cuantasPagTieneTodos(uint32_t pid){
	uint32_t c = 0;
	uint32_t i;
	//pthread_mutex_lock( &mutexTablaPagina);
	for(i=0; i < MARCOS; i++){
		if(TablaDePagina[i].PID == pid)
			c++;
	}
	//pthread_mutex_unlock( &mutexTablaPagina );
	return c;
}
uint32_t cuantasPagTieneVivos(uint32_t pid){
	uint32_t c = 0;
	uint32_t i;
	//pthread_mutex_lock( &mutexTablaPagina);
	for(i=0; i < MARCOS; i++){
		if(TablaDePagina[i].PID == pid && TablaDePagina[i].disponible == false)
			c++;
	}
	//pthread_mutex_unlock( &mutexTablaPagina );
	return c;
}

bool estaEnCache(uint32_t pid, uint32_t numPag){
	pthread_mutex_lock( &mutexTablaCache );
	bool esta = list_any_satisfy(tablaCache, LAMBDA(bool _(void* elem) {
		return ((entradaCache*)elem)->PID==pid && ((entradaCache*)elem)->Pag==numPag; }));
	if (esta)
	{//llevar al final de la pila
		entradaCache* entrada = list_remove_by_condition(tablaCache, LAMBDA(bool _(void* elem) {
			return ((entradaCache*)elem)->PID==pid && ((entradaCache*)elem)->Pag==numPag; }));
		list_add(tablaCache, entrada);
		pthread_mutex_unlock( &mutexTablaCache );
		return true;
	}
	else {
		pthread_mutex_unlock( &mutexTablaCache );
		return false;
	}

}
void agregarACache(uint32_t pid, uint32_t numPag){
	entradaCache* entrada = malloc(sizeof(entradaCache));
	entrada->PID=pid;
	entrada->Pag=numPag;
	entrada->Contenido=FrameLookup(pid, numPag);

	pthread_mutex_lock( &mutexTablaCache );
	int cantPagProc = list_count_satisfying(tablaCache, LAMBDA(bool _(void* elem) {
		return ((entradaCache*)elem)->PID==pid; }));
	if(cantPagProc >= CACHE_X_PROC){
		entradaCache* eliminado = list_remove_by_condition(tablaCache,
			LAMBDA(bool _(void* elem) {
			return ((entradaCache*)elem)->PID==pid; }));
		list_add(tablaCacheRastro, eliminado);
	}
	else if(list_size(tablaCache) >= ENTRADAS_CACHE) {
		entradaCache* eliminado = list_remove(tablaCache, 0);
		list_add(tablaCacheRastro, eliminado);
	}

	list_add(tablaCache, entrada);
	pthread_mutex_unlock( &mutexTablaCache );
}

void IniciarPrograma(uint32_t pid, uint32_t cantPag, int socketFD) {
	uint32_t result;
	pthread_mutex_lock( &mutexTablaPagina );
	if (pid != 0 && cuantasPagTieneVivos(pid) == 0 && cantPagAsignadas + cantPag < MARCOS) {
		//si no existe el proceso y hay lugar en la memoria
		int i;

		for (i = 0; i < cantPag; i++) {
			//lo agregamos a la tabla
			uint32_t frame = Hash(pid, i);
			while(TablaDePagina[frame].disponible==false){
				frame++;
				if(frame>=MARCOS)
					frame -= MARCOS;
			}
			TablaDePagina[frame].PID = pid;
			TablaDePagina[frame].Pag = i;
			TablaDePagina[frame].disponible=false;
			cantPagAsignadas++;
		}
		pthread_mutex_unlock( &mutexTablaPagina );
		printf("Nuevo programa iniciado en memoria con PID %u.\n", pid);
		result = 1;
		EnviarDatos(socketFD, MEMORIA, &result , sizeof(uint32_t));
	} else {
		pthread_mutex_unlock( &mutexTablaPagina );
		result = 0;
		EnviarDatosTipo(socketFD, MEMORIA, &result, sizeof(uint32_t), ESERROR);
	}
}
void SleepMemoria(uint32_t milliseconds){
	struct timespec ts;
	ts.tv_sec = milliseconds / 1000;
	ts.tv_nsec = (milliseconds % 1000) * 1000000;
	nanosleep(&ts, NULL);
}

void SolicitarBytes(uint32_t pid, uint32_t numPag, uint32_t offset,	uint32_t tam, int socketFD) {
	//valido los parametros

	pthread_mutex_lock( &mutexTablaPagina);
	pthread_mutex_lock( &mutexContenidoMemoria );
	if(numPag>=0 && cuantasPagTieneVivos(pid)>=numPag && offset+tam <= MARCO_SIZE) {

		//si no esta en chache, esperar tiempo definido por arch de config
		if (!estaEnCache(pid, numPag)) {
			SleepMemoria(RETARDO_MEMORIA);
			agregarACache(pid, numPag);
		}
		//buscar pagina
		uint32_t frame = FrameLookup(pid, numPag);
		void*  datosSolicitados = ContenidoMemoria + MARCO_SIZE * frame + offset;

		EnviarDatos(socketFD, MEMORIA, datosSolicitados, tam);
		pthread_mutex_unlock( &mutexContenidoMemoria );
		pthread_mutex_unlock( &mutexTablaPagina);
	} else {
		pthread_mutex_unlock( &mutexContenidoMemoria );
		pthread_mutex_unlock( &mutexTablaPagina);
		uint32_t r = 0;
		EnviarDatosTipo(socketFD, MEMORIA, &r, sizeof(uint32_t), ESERROR);
	}
}
void AlmacenarBytes(Paquete paquete, int socketFD) {
	//Los datos son parametros
	//Parámetros: PID, #página, offset, tamaño y buffer.
	uint32_t result=0;
	pthread_mutex_lock( &mutexTablaPagina);
	pthread_mutex_lock( &mutexContenidoMemoria );
	if(DATOS[2]>=0 && cuantasPagTieneVivos(DATOS[1])>=DATOS[2] && DATOS[3]+DATOS[4] < MARCO_SIZE) { //valido los parametros

		//esperar tiempo definido por arch de config
		SleepMemoria(RETARDO_MEMORIA);
		//buscar pagina
		void* pagina = ContenidoMemoria + MARCO_SIZE * FrameLookup(DATOS[1], DATOS[2]);

		//escribir en pagina
		memcpy(pagina + DATOS[3],paquete.Payload+(sizeof(uint32_t)*5), DATOS[4]);
		pthread_mutex_unlock( &mutexContenidoMemoria );
		pthread_mutex_unlock( &mutexTablaPagina);
		printf("Datos Almacenados del PID: %u, Pag: %u, Offset: %u\n", DATOS[1], DATOS[2], DATOS[3]);
		//actualizar cache
		if (!estaEnCache(DATOS[1], DATOS[2])) {
			agregarACache(DATOS[1], DATOS[2]);
		}
		result = 1;
		EnviarDatos(socketFD, MEMORIA, &result, sizeof(uint32_t));
	} else {
		pthread_mutex_unlock( &mutexContenidoMemoria );
		pthread_mutex_unlock( &mutexTablaPagina);
		EnviarDatosTipo(socketFD, MEMORIA, &result, sizeof(uint32_t), ESERROR);
	}
}

void AsignarPaginas(uint32_t pid, uint32_t cantPagParaAsignar, int socketFD) {
	uint32_t result=0;
	pthread_mutex_lock( &mutexTablaPagina );
	int32_t cantPaginasPid = cuantasPagTieneVivos(pid);
	if (cantPagAsignadas + cantPagParaAsignar <= MARCOS && cantPaginasPid > 0) {

		int i;
		for (i = cantPaginasPid; i < cantPagParaAsignar+cantPaginasPid; i++) {
			//lo agregamos a la tabla
			uint32_t frame = Hash(pid, i);

			while(TablaDePagina[frame].disponible==false) {
				frame++;
				if(frame>=MARCOS)
					frame -= MARCOS;
			}
			TablaDePagina[frame].PID = pid;
			TablaDePagina[frame].Pag = i;
			TablaDePagina[frame].disponible=false;
			cantPagAsignadas++;


		}
		pthread_mutex_unlock( &mutexTablaPagina );
		result = 1;
		EnviarDatos(socketFD, MEMORIA, &result, sizeof(uint32_t));
	} else {
		pthread_mutex_unlock( &mutexTablaPagina );
		EnviarDatosTipo(socketFD, MEMORIA, &result, sizeof(uint32_t), ESERROR);
	}
}

void LiberarPaginas(uint32_t pid, uint32_t numPag, int socketFD) {
	uint32_t result=0;
	pthread_mutex_lock( &mutexTablaPagina );
	if(cuantasPagTieneVivos(pid) > 0) {

		cantPagAsignadas--;
		TablaDePagina[FrameLookup(pid, numPag)].disponible=true;
		pthread_mutex_unlock( &mutexTablaPagina );
		//sacar de cache.
		pthread_mutex_lock( &mutexTablaCache );
		entradaCache* eliminado = list_remove_by_condition(tablaCache,
				LAMBDA(bool _(void* elem) {
				return ((entradaCache*)elem)->PID==pid && ((entradaCache*)elem)->Pag==numPag; }));
		list_add(tablaCacheRastro, eliminado);
		pthread_mutex_unlock( &mutexTablaCache );
		result =1;
		EnviarDatos(socketFD, MEMORIA, &result, sizeof(uint32_t));
	} else {
		pthread_mutex_unlock( &mutexTablaPagina );
		EnviarDatosTipo(socketFD, MEMORIA, &result, sizeof(uint32_t), ESERROR);
	}
}

void FinalizarPrograma(uint32_t pid, int socketFD) {
	printf("\nFinalizando programa %u\n", pid);
	uint32_t result=0;
	pthread_mutex_lock( &mutexTablaPagina );
	uint32_t cantPag = cuantasPagTieneVivos(pid);
	if(cantPag > 0) {

		uint32_t i;

		pthread_mutex_lock( &mutexTablaCache );
		for(i=0; i < cantPag; i++){
			TablaDePagina[FrameLookup(pid, i)].disponible=true;
			cantPagAsignadas--;
			//sacar de cache.
			entradaCache* eliminado = list_remove_by_condition(tablaCache,
				LAMBDA(bool _(void* elem) {
				return ((entradaCache*)elem)->PID==pid && ((entradaCache*)elem)->Pag==i; }));
			list_add(tablaCacheRastro, eliminado);
		}
		pthread_mutex_unlock( &mutexTablaCache );
		pthread_mutex_unlock( &mutexTablaPagina );
		result=1;
		EnviarDatos(socketFD, MEMORIA, &result, sizeof(uint32_t));
	} else {
		pthread_mutex_unlock( &mutexTablaPagina );
		EnviarDatosTipo(socketFD, MEMORIA, &result, sizeof(uint32_t), ESERROR);
	}
}

int RecibirPaqueteMemoria (int socketFD, char receptor[11], Paquete* paquete) {
	paquete->Payload = NULL;
	int resul = RecibirDatos(&(paquete->header), socketFD, TAMANIOHEADER);
	if (resul > 0) { //si no hubo error
		if (paquete->header.tipoMensaje == ESHANDSHAKE) { //vemos si es un handshake y le respondemos con el tam de pag
			if( !strcmp(paquete->header.emisor, KERNEL) || !strcmp(paquete->header.emisor, CPU)){
				printf("Se establecio conexion con %s\n", paquete->header.emisor);
				Paquete paquete;
				paquete.header.tipoMensaje = ESHANDSHAKE;
				paquete.header.tamPayload = sizeof(uint32_t);
				strcpy(paquete.header.emisor, MEMORIA);
				paquete.Payload=&MARCO_SIZE;
				EnviarPaquete(socketFD, &paquete);
			}
		} else { //recibimos un payload y lo procesamos (por ej, puede mostrarlo)
			paquete->Payload = malloc(paquete->header.tamPayload);
			resul = RecibirDatos(paquete->Payload, socketFD, paquete->header.tamPayload);
		}
	}

	return resul;
}

void accion(void* socket){
	int socketFD = *(int*)socket;
	Paquete paquete;
	while (RecibirPaqueteMemoria(socketFD, MEMORIA, &paquete) > 0) {
		if (paquete.header.tipoMensaje == ESDATOS){
			switch ((*(uint32_t*)paquete.Payload)){
			case INIC_PROG:
				IniciarPrograma(DATOS[1],DATOS[2],socketFD);
			break;
			case SOL_BYTES:
				SolicitarBytes(DATOS[1],DATOS[2],DATOS[3],DATOS[4],socketFD);
			break;
			case ALM_BYTES:
				AlmacenarBytes(paquete,socketFD);
			break;
			case ASIG_PAG:
				AsignarPaginas(DATOS[1],DATOS[2],socketFD);
			break;
			case LIBE_PAG:
				LiberarPaginas(DATOS[1],DATOS[2],socketFD);
			break;
			case FIN_PROG:
				FinalizarPrograma(DATOS[1],socketFD);
			break;
			}
		}
		if (paquete.Payload != NULL)
			free(paquete.Payload);
	}
	close(socketFD);
}
