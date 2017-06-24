#include "SocketsL.h"

void Servidor(char* ip, int puerto, char nombre[11],
		void (*accion)(Paquete* paquete, int socketFD),
		int (*RecibirPaquete)(int socketFD, char receptor[11], Paquete* paquete)) {
	int SocketEscucha = StartServidor(ip, puerto);
	fd_set master; // conjunto maestro de descriptores de fichero
	fd_set read_fds; // conjunto temporal de descriptores de fichero para select()
	FD_ZERO(&master); // borra los conjuntos maestro y temporal
	FD_ZERO(&read_fds);
	FD_SET(SocketEscucha, &master); // añadir listener al conjunto maestro
	int fdmax = SocketEscucha; // seguir la pista del descriptor de fichero mayor, por ahora es éste
	struct sockaddr_in remoteaddr; // dirección del cliente

	for (;;) {	// bucle principal
		read_fds = master; // cópialo
		if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1) {
			perror("select");
			exit(1);
		}
		// explorar conexiones existentes en busca de datos que leer
		int i;
		for (i = 0; i <= fdmax; i++) {
			if (FD_ISSET(i, &read_fds)) { // ¡¡tenemos datos!!
				if (i == SocketEscucha) { // gestionar nuevas conexiones
					int addrlen = sizeof(remoteaddr);
					int nuevoSocket = accept(SocketEscucha,
							(struct sockaddr*) &remoteaddr, &addrlen);
					if (nuevoSocket == -1)
						perror("accept");
					else {
						FD_SET(nuevoSocket, &master); // añadir al conjunto maestro
						if (nuevoSocket > fdmax)
							fdmax = nuevoSocket; // actualizar el máximo
						printf("\nNueva conexion de %s en " "socket %d\n",
								inet_ntoa(remoteaddr.sin_addr), nuevoSocket);
					}
				} else {
					Paquete paquete;
					int result = RecibirPaquete(i, nombre, &paquete);
					if (result > 0)
						accion(&paquete, i); //>>>>Esto hace el servidor cuando recibe algo<<<<
					else
						FD_CLR(i, &master); // eliminar del conjunto maestro si falla
					free(paquete.Payload); //Y finalmente, no puede faltar hacer el free
				}
			}
		}
	}
}

int ConectarAServidor(int puertoAConectar, char* ipAConectar, char servidor[11], char cliente[11],
		void RecibirElHandshake(int socketFD, char emisor[11])) {
	int socketFD = socket(AF_INET, SOCK_STREAM, 0);

	struct sockaddr_in direccionKernel;

	direccionKernel.sin_family = AF_INET;
	direccionKernel.sin_port = htons(puertoAConectar);
	direccionKernel.sin_addr.s_addr = inet_addr(ipAConectar);
	memset(&(direccionKernel.sin_zero), '\0', 8);
	if (connect(socketFD, (struct sockaddr *) &direccionKernel,
			sizeof(struct sockaddr))>=0){
		EnviarHandshake(socketFD, cliente);
		RecibirElHandshake(socketFD, servidor);
		return socketFD;
	}else{
		return -1;
	}
}

int StartServidor(char* MyIP, int MyPort) // obtener socket a la escucha
{
	struct sockaddr_in myaddr; // dirección del servidor
	int yes = 1; // para setsockopt() SO_REUSEADDR, más abajo
	int SocketEscucha;

	if ((SocketEscucha = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket");
		exit(1);
	}

	if (setsockopt(SocketEscucha, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int))
			== -1) // obviar el mensaje "address already in use" (la dirección ya se está 	usando)
			{
		perror("setsockopt");
		exit(1);
	}

	// enlazar
	myaddr.sin_family = AF_INET;
	myaddr.sin_addr.s_addr = inet_addr(MyIP);
	myaddr.sin_port = htons(MyPort);
	memset(&(myaddr.sin_zero), '\0', 8);

	if (bind(SocketEscucha, (struct sockaddr*) &myaddr, sizeof(myaddr)) == -1) {
		perror("bind");
		exit(1);
	}

	// escuchar
	if (listen(SocketEscucha, BACKLOG) == -1) {
		perror("listen");
		exit(1);
	}

	return SocketEscucha;
}

void EnviarDatos(int socketFD, char emisor[11], void* datos, int tamDatos) {
	Paquete* paquete = malloc(sizeof(Paquete));
	paquete->header.tipoMensaje = ESDATOS;
	paquete->header.tamPayload = tamDatos;
	strcpy(paquete->header.emisor, emisor);
	paquete->Payload = datos;

	EnviarPaquete(socketFD, paquete);

	free(paquete);
}

void EnviarDatosTipo(int socketFD, char emisor[11], void* datos, int tamDatos, int tipoMensaje){
	Paquete* paquete = malloc(sizeof(Paquete));
	paquete->header.tipoMensaje = tipoMensaje;
	paquete->header.tamPayload = tamDatos;
	strcpy(paquete->header.emisor, emisor);
	paquete->Payload = datos;

	EnviarPaquete(socketFD, paquete);

	free(paquete);
}
void EnviarPaquete(int socketCliente, Paquete* paquete) {
	int cantAEnviar = sizeof(Header) + paquete->header.tamPayload;
	void* datos = malloc(cantAEnviar);
	memcpy(datos, &(paquete->header), TAMANIOHEADER);
	if (paquete->header.tamPayload > 0) //No sea handshake
		memcpy(datos + TAMANIOHEADER, (paquete->Payload),
				paquete->header.tamPayload);

	//Paquete* punteroMsg = datos;
	int enviado = 0; //bytes enviados
	int totalEnviado = 0;

	do {
		enviado = send(socketCliente, datos + totalEnviado,
				cantAEnviar - totalEnviado, 0);
		//largo -= totalEnviado;
		totalEnviado += enviado;
		//punteroMsg += enviado; //avanza la cant de bytes que ya mando
	} while (totalEnviado != cantAEnviar);
	free(datos);
}

void EnviarMensaje(int socketFD, char* msg, char emisor[11]) {
	Paquete paquete;
	strcpy(paquete.header.emisor, emisor);
	paquete.header.tipoMensaje = ESSTRING;
	paquete.header.tamPayload = string_length(msg) + 1;
	paquete.Payload = msg;
	EnviarPaquete(socketFD, &paquete);

}

void EnviarHandshake(int socketFD, char emisor[11]) {
	Paquete* paquete = malloc(TAMANIOHEADER);
	Header header;
	header.tipoMensaje = ESHANDSHAKE;
	header.tamPayload = 0;
	strcpy(header.emisor, emisor);
	paquete->header = header;
	EnviarPaquete(socketFD, paquete);

	free(paquete);
}

void RecibirHandshake(int socketFD, char emisor[11]) {
	Header* header = malloc(TAMANIOHEADER);
	int resul = RecibirDatos(header, socketFD, TAMANIOHEADER);
	if (resul > 0) { // si no hubo error en la recepcion
		if (strcmp(header->emisor, emisor) == 0) {
			if (header->tipoMensaje == ESHANDSHAKE)
				printf("\nConectado con el servidor %s\n", emisor);
			else
				perror("Error de Conexion, no se recibio un handshake\n");
		} else
			perror("Error, no se recibio un handshake del servidor esperado\n");
	}
	free(header);
}

int RecibirDatos(void* paquete, int socketFD, uint32_t cantARecibir) {
	void* datos = malloc(cantARecibir);
	//char* punteroMsg = paquete;
	int recibido = 0;
	int totalRecibido = 0;

	do {
		recibido = recv(socketFD, datos + totalRecibido,
				cantARecibir - totalRecibido, 0);
		//send(socketCliente,punteroMsg+totalEnviado,cantAEnviar-totalEnviado,0);
		totalRecibido += recibido;
	} while (totalRecibido != cantARecibir && recibido > 0);
	memcpy(paquete, datos, cantARecibir);
	free(datos);

	if (recibido < 0) {
		perror("Error de Recepcion, no se pudo leer el mensaje\n");
		close(socketFD); // ¡Hasta luego!
	} else if (recibido == 0) {
		printf("\nSocket %d: ", socketFD);
		perror("Fin de Conexion, se cerro la conexion\n");
		close(socketFD); // ¡Hasta luego!
	}

	return recibido;
}

int RecibirPaqueteServidor(int socketFD, char receptor[11], Paquete* paquete) {
	paquete->Payload = malloc(1);
	int resul = RecibirDatos(&(paquete->header), socketFD, TAMANIOHEADER);
	if (resul > 0) { //si no hubo error
		if (paquete->header.tipoMensaje == ESHANDSHAKE) { //vemos si es un handshake
			printf("Se establecio conexion con %s\n", paquete->header.emisor);
			EnviarHandshake(socketFD, receptor); // paquete->header.emisor
		} else { //recibimos un payload y lo procesamos (por ej, puede mostrarlo)
			paquete->Payload = realloc(paquete->Payload,
					paquete->header.tamPayload);
			resul = RecibirDatos(paquete->Payload, socketFD,
					paquete->header.tamPayload);
		}
	}

	return resul;
}

int RecibirPaqueteCliente(int socketFD, char receptor[11], Paquete* paquete) {
	paquete->Payload = malloc(1);
	int resul = RecibirDatos(&(paquete->header), socketFD, TAMANIOHEADER);
	if (resul > 0 && paquete->header.tipoMensaje >= 0) { //si no hubo error ni es un handshake
		paquete->Payload = realloc(paquete->Payload,
				paquete->header.tamPayload);
		resul = RecibirDatos(paquete->Payload, socketFD,
				paquete->header.tamPayload);
	}

	return resul;
}

//Interfaz para comunicarse con Memoria: (definido por el TP, no se puede modificar)
uint32_t IM_InicializarPrograma(int socketFD, char emisor[11], uint32_t ID_Prog,
		uint32_t CantPag) { //Solicitar paginas para un programa nuevo, devuelve la cant de paginas que pudo asignar
	int tamDatos = sizeof(uint32_t) * 3;
	void* datos = malloc(tamDatos);
	((uint32_t*) datos)[0] = INIC_PROG;
	((uint32_t*) datos)[1] = ID_Prog;
	((uint32_t*) datos)[2] = CantPag;
	EnviarDatos(socketFD, emisor, datos, tamDatos);
	free(datos);
	Paquete* paquete = malloc(sizeof(Paquete));
	while (RecibirPaqueteCliente(socketFD, MEMORIA, paquete) <= 0);
	uint32_t r = *(uint32_t*) (paquete->Payload);
	free(paquete->Payload);
	free(paquete);
	return r;
}
void* IM_LeerDatos(int socketFD, char emisor[11], uint32_t ID_Prog,
		uint32_t PagNum, uint32_t offset, uint32_t cantBytes) { //Devuelve los datos de una pagina, ¡Recordar hacer free(puntero) cuando los terminamos de usar!
	int tamDatos = sizeof(uint32_t) * 5;
	void* datos = malloc(tamDatos);
	((uint32_t*) datos)[0] = SOL_BYTES;
	((uint32_t*) datos)[1] = ID_Prog;
	((uint32_t*) datos)[2] = PagNum;
	((uint32_t*) datos)[3] = offset;
	((uint32_t*) datos)[4] = cantBytes;
	EnviarDatos(socketFD, emisor, datos, tamDatos);
	free(datos);
	Paquete* paquete = malloc(sizeof(Paquete));
	while (RecibirPaqueteCliente(socketFD, MEMORIA, paquete) <= 0);
	void* r;
	if(paquete->header.tipoMensaje == ESERROR)
		r = NULL;
	else if(paquete->header.tipoMensaje == ESDATOS)
		r = paquete->Payload;
	free(paquete);
	return r;
}
uint32_t IM_GuardarDatos(int socketFD, char emisor[11], uint32_t ID_Prog,
		uint32_t PagNum, uint32_t offset, uint32_t cantBytes, void* buffer) {
	int tamDatos = sizeof(uint32_t) * 5 + cantBytes;
	void* datos = malloc(tamDatos);
	((uint32_t*) datos)[0] = ALM_BYTES;
	((uint32_t*) datos)[1] = ID_Prog;
	((uint32_t*) datos)[2] = PagNum;
	((uint32_t*) datos)[3] = offset;
	((uint32_t*) datos)[4] = cantBytes;
	memcpy(datos+sizeof(uint32_t) * 5, buffer, cantBytes);
	EnviarDatos(socketFD, emisor, datos, tamDatos);
	free(datos);
	Paquete* paquete = malloc(sizeof(Paquete));
	while (RecibirPaqueteCliente(socketFD, MEMORIA, paquete) <= 0);
	uint32_t r = *(uint32_t*) (paquete->Payload);
	free(paquete->Payload);
	free(paquete);
	return r;
}
uint32_t IM_AsignarPaginas(int socketFD, char emisor[11], uint32_t ID_Prog,
		uint32_t CantPag) { //Devuelve la cant de paginas que pudo asignar
	int tamDatos = sizeof(uint32_t) * 3;
	void* datos = malloc(tamDatos);
	((uint32_t*) datos)[0] = ASIG_PAG;
	((uint32_t*) datos)[1] = ID_Prog;
	((uint32_t*) datos)[2] = CantPag;
	EnviarDatos(socketFD, emisor, datos, tamDatos);
	free(datos);
	Paquete* paquete = malloc(sizeof(Paquete));
	while (RecibirPaqueteCliente(socketFD, MEMORIA, paquete) <= 0);
	uint32_t r = *(uint32_t*) (paquete->Payload);
	free(paquete->Payload);
	free(paquete);
	return r;
}
uint32_t IM_LiberarPagina(int socketFD, char emisor[11], uint32_t ID_Prog, uint32_t NumPag) {//Agregado en el Fe de Erratas, responde 0 si hubo error y 1 si libero la pag.
	int tamDatos = sizeof(uint32_t) * 3;
	void* datos = malloc(tamDatos);
	((uint32_t*) datos)[0] = LIBE_PAG;
	((uint32_t*) datos)[1] = ID_Prog;
	((uint32_t*) datos)[2] = NumPag;
	EnviarDatos(socketFD, emisor, datos, tamDatos);
	free(datos);
	Paquete* paquete = malloc(sizeof(Paquete));
	while (RecibirPaqueteCliente(socketFD, MEMORIA, paquete) <= 0);
	uint32_t r = *(uint32_t*) (paquete->Payload);
	free(paquete->Payload);
	free(paquete);
	return r;
}

uint32_t IM_FinalizarPrograma(int socketFD, char emisor[11], uint32_t ID_Prog) { //Borra las paginas de ese programa.
	int tamDatos = sizeof(uint32_t) * 2;
	void* datos = malloc(tamDatos);
	((uint32_t*) datos)[0] = FIN_PROG;
	((uint32_t*) datos)[1] = ID_Prog;
	EnviarDatos(socketFD, emisor, datos, tamDatos);
	free(datos);
	Paquete* paquete = malloc(sizeof(Paquete));
	while (RecibirPaqueteCliente(socketFD, MEMORIA, paquete) <= 0);
	uint32_t r = *(uint32_t*) (paquete->Payload);
	free(paquete->Payload);
	free(paquete);
	return r;
}
/*
void EnviarPCB(int socketCliente, char emisor[11], BloqueControlProceso* pecebe) {

	int tamIndiceCodigo = list_size(pecebe->IndiceDeCodigo) * sizeof(uint32_t) * 2; //IndiceDeCodigo

	int tamIndiceEtiquetas = 0; //IndiceDeEtiquetas

	int i;

	for (i = 0; i < dictionary_size(pecebe->IndiceDeEtiquetas); i++){
		tamIndiceEtiquetas += sizeof(uint32_t);
	}

	int tamIndiceStack = 0; //IndiceDelStack

	for (i = 0; i < list_size(pecebe->IndiceDelStack); i++){
		tamIndiceStack += list_size(((IndiceStack*)list_get(pecebe->IndiceDelStack, i)).Variables) * sizeof(uint32_t)
						+ list_size(((IndiceStack*)list_get(pecebe->IndiceDelStack, i)).Argumentos) * sizeof(uint32_t)
						+ sizeof(uint32_t)
						+ sizeof(PosicionDeMemoria);
	}

	int tamDatos = sizeof(uint32_t) * 3 //PID, ProgramCounter, PaginasDeCodigo
						+ sizeof(int32_t) //ExitCode
						+ tamIndiceCodigo //IndiceDeCodigo
						+ tamIndiceEtiquetas //IndiceDeEtiquetas
						+ tamIndiceStack; //IndiceDelStack

	void* pcbSerializado = malloc(tamDatos);

	((uint32_t*) pcbSerializado)[0] = pecebe->PID;
	((uint32_t*) pcbSerializado)[1] = pecebe->ProgramCounter;
	((uint32_t*) pcbSerializado)[2] = pecebe->PaginasDeCodigo;
	((int32_t*) pcbSerializado)[3] = pecebe->ExitCode;
	((uint32_t*) pcbSerializado)[4] = list_size(pecebe->IndiceDeCodigo); //cantElementosIndiceDeCodigo
	((uint32_t*) pcbSerializado)[5] = dictionary_size(pecebe->IndiceDeEtiquetas); //cantElementosIndiceDeEtiquetas
	((uint32_t*) pcbSerializado)[6] = list_size(pecebe->IndiceDelStack); //cantElementosIndiceDelStack
	for (i = 0 ; i < list_size(pecebe->IndiceDeCodigo); i++){
		((uint32_t*) pcbSerializado)[7+i] = (uint32_t) list_get(pecebe->IndiceDeCodigo, i);

	}
	//memcpy(pcbSerializado+sizeof(uint32_t) * 7, pecebe->IndiceDeCodigo, tamIndiceCodigo);
	//memcpy(pcbSerializado+sizeof(uint32_t) * 7 + tamIndiceCodigo, pecebe->IndiceDeEtiquetas, tamIndiceEtiquetas);
	//memcpy(pcbSerializado+sizeof(uint32_t) * 7 + tamIndiceCodigo + tamIndiceEtiquetas, pecebe->IndiceDelStack, tamIndiceStack);
	pcb_Destroy(pecebe);
}*/


void EnviarPCB(int socketCliente, char emisor[11], BloqueControlProceso* pecebe, uint32_t cantRafagas) {

	int i = 0;
	t_size sizeIndCod = list_size(pecebe->IndiceDeCodigo);
	uint32_t sizeIndEtiq = dictionary_size(pecebe->IndiceDeEtiquetas);

	//Serialización ExitCode
	int tam = sizeof(uint32_t) * 7 +
				sizeof(int32_t) +
				sizeof(uint32_t) * 2 * sizeIndCod +
				sizeIndEtiq * sizeof(t_puntero_instruccion) +
				pecebe->etiquetas_size * sizeof(char);

	void* pcbSerializado = malloc(tam);
	*((uint32_t*)pcbSerializado) = cantRafagas;
	pcbSerializado += sizeof(uint32_t);
	//Serialización PID
	*((uint32_t*)pcbSerializado) = pecebe->PID;
	pcbSerializado += sizeof(uint32_t);

	//Serialización ProgramCounter

	*((uint32_t*)pcbSerializado) = pecebe->ProgramCounter;
	pcbSerializado += sizeof(uint32_t);

	//Serialización PaginasDeCodigo
	*((uint32_t*)pcbSerializado) = pecebe->PaginasDeCodigo;
	pcbSerializado += sizeof(uint32_t);

	//Serialización ExitCode
	*((int32_t*)pcbSerializado) = pecebe->ExitCode;
	pcbSerializado += sizeof(int32_t);


	//Serialización IndiceDeCodigo

	*((uint32_t*)pcbSerializado) = sizeIndCod;
	pcbSerializado += sizeof(uint32_t);

	for (i = 0; i < sizeIndCod; i++){
		*((uint32_t*)pcbSerializado) = ((uint32_t*)(list_get(pecebe->IndiceDeCodigo, i)))[0];
		pcbSerializado += sizeof(uint32_t);
		*((uint32_t*)pcbSerializado)= ((uint32_t*)(list_get(pecebe->IndiceDeCodigo, i)))[1];
		pcbSerializado += sizeof(uint32_t);
	}

	//Serialización IndiceDelStack

	uint32_t sizeIndStack = list_size(pecebe->IndiceDelStack);

	*((uint32_t*)pcbSerializado) = sizeIndStack;
	pcbSerializado += sizeof(uint32_t);

	for (i = 0; i < sizeIndStack; i++){
		IndiceStack indice = *((IndiceStack*)(list_get(pecebe->IndiceDelStack, i)));
		uint32_t sizeVariables = list_size(indice.Variables);
		uint32_t sizeArgumentos = list_size(indice.Argumentos);
		int j;

		tam += sizeof(uint32_t) * 3 +
				sizeof(PosicionDeMemoria) +
				sizeVariables * sizeof(uint32_t) +
				sizeArgumentos * sizeof(uint32_t) ;

		pcbSerializado = realloc(pcbSerializado, tam);
		*((uint32_t*)pcbSerializado) = indice.DireccionDeRetorno;
		pcbSerializado += sizeof(uint32_t);
		*((PosicionDeMemoria*)pcbSerializado) = indice.PosVariableDeRetorno;
		pcbSerializado += sizeof(PosicionDeMemoria);
		*((uint32_t*)pcbSerializado) = sizeVariables;
		pcbSerializado += sizeof(uint32_t);

		for (j = 0; j < sizeVariables; j++){
			*((uint32_t*)pcbSerializado) = *((uint32_t*)list_get(indice.Variables, j));
			pcbSerializado += sizeof(uint32_t);
		}

		*((uint32_t*)pcbSerializado) = sizeArgumentos;
		pcbSerializado += sizeof(uint32_t);

		for (j = 0; j < sizeArgumentos; j++){
			*((uint32_t*)pcbSerializado) = *((uint32_t*)list_get(indice.Argumentos, j));
			pcbSerializado += sizeof(uint32_t);
		}
	}

	//Serialización Índice De Etiquetas

	*((uint32_t*)pcbSerializado) = sizeIndEtiq;
	pcbSerializado += sizeof(uint32_t);

	char** etiquetas = string_n_split(pecebe->etiquetas, pecebe->cantidad_de_etiquetas ,(char*)&sizeIndCod);
	string_iterate_lines(etiquetas, LAMBDA(void _(char* etiqueta) {
		t_puntero_instruccion* instruccion = dictionary_get(pecebe->IndiceDeEtiquetas, etiqueta);
		*((t_puntero_instruccion*)pcbSerializado) = *instruccion;
		pcbSerializado += sizeof(t_puntero_instruccion);
	}));

	*((uint32_t*)pcbSerializado) = pecebe->etiquetas_size;
	pcbSerializado += sizeof(uint32_t);

	memcpy(pcbSerializado, pecebe->etiquetas, pecebe->etiquetas_size);

	pcbSerializado -= tam;
	EnviarDatos(socketCliente, emisor, pcbSerializado, tam);
	free(pcbSerializado);
}

void RecibirPCB(BloqueControlProceso* pecebe, int socketFD){

}

