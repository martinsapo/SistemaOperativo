/*
 ============================================================================
 Name        : Consola.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <commons/log.h>
#include <commons/collections/list.h>
#include <commons/string.h>
#include <commons/txt.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
char* IP_KERNEL;
int PUERTO_KERNEL;

void obtenerValoresArchivoConfiguracion() {
	int contadorDeVariables = 0;
	int c;
	FILE *file;
	file = fopen("ArchivoConfiguracion.txt", "r");
	if (file) {
		while ((c = getc(file)) != EOF)
			if (c == '=')
			{
				if (contadorDeVariables == 1) {
					fscanf(file, "%i", &PUERTO_KERNEL);
				}
				if (contadorDeVariables == 0)
				{
					char buffer[10000];
					IP_KERNEL = fgets(buffer, sizeof buffer, file);
					strtok(IP_KERNEL, "\n");

					contadorDeVariables++;
				}
			}
		fclose(file);
	}
}

void imprimirArchivoConfiguracion() {
	int c;
	FILE *file;
	file = fopen("ArchivoConfiguracion.txt", "r");
	if (file) {
		while ((c = getc(file)) != EOF)
			putchar(c);
		fclose(file);
	}
}

void ConectarServidor(){
	int socketFD = socket(AF_INET,SOCK_STREAM,0);
	printf("%i\n", socketFD);
	struct sockaddr_in direccionKernel;
	direccionKernel.sin_family = AF_INET;
	direccionKernel.sin_port = htons(5000/*PUERTO_KERNEL*/);
	direccionKernel.sin_addr.s_addr = (int) htonl("10.0.2.15"/*IP_KERNEL*/);
	connect(socketFD,(struct sockaddr *)&direccionKernel, sizeof(struct sockaddr));
	printf("%s", "se conecto! anda a kernel y apreta\n");
}


int main(void) {
	obtenerValoresArchivoConfiguracion();
	imprimirArchivoConfiguracion();
	ConectarServidor();
	return 0;
}
