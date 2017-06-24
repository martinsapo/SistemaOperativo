#include "Helper.h"
#include "SocketsL.h"

char* getWord(char* string, int pos) {
	char delimiter[] = " ";
	char *word, *context;
	int inputLength = strlen(string);
	char *inputCopy = (char*) calloc(inputLength + 1, sizeof(char));
	strncpy(inputCopy, string, inputLength);
	if (pos == 0) {
		return strtok_r(inputCopy, delimiter, &context);
	} else {
		int i;
		for (i = 1; i <= pos; i++) {
			word = strtok_r(NULL, delimiter, &context);
		}
	}
	return word;
}

char* integer_to_string(int x) {
	char* buffer = malloc(sizeof(char) * sizeof(int) * 4 + 1);
	if (buffer) {
		sprintf(buffer, "%d", x);
	}
	return buffer; // caller is expected to invoke free() on this buffer to release memory
}

int GetTamanioArchivo(FILE * f) {
	fseek(f, 0L, SEEK_END);
	int size = ftell(f);
	return size;
}

void pcb_Create(BloqueControlProceso* pecebe, uint32_t pid_actual){
	pecebe->PID = pid_actual;
	pecebe->ProgramCounter = 0;
	pecebe->PaginasDeCodigo=0;
	pecebe->ExitCode = 0;
	pecebe->etiquetas_size = 0;
	pecebe->etiquetas = string_new();
	pecebe->cantidad_de_etiquetas = 0;
	pecebe->cantidad_de_funciones = 0;
	pecebe->cantidadDeRafagasAEjecutar = 0;
	pecebe->cantidadDeRafagasEjecutadas = 0;
	pecebe->cantidadDeRafagasEjecutadasHistorica = 0;
	pecebe->IndiceDeCodigo = list_create();
	pecebe->IndiceDeEtiquetas = dictionary_create();
	pecebe->IndiceDelStack = list_create();
}

void pcb_Destroy(BloqueControlProceso* pecebe){
	//Creo el pcb y lo guardo en la lista de nuevos
	free(pecebe->etiquetas);
	list_destroy_and_destroy_elements(pecebe->IndiceDeCodigo,free);
	list_destroy_and_destroy_elements(pecebe->IndiceDelStack,free);
	dictionary_destroy_and_destroy_elements(pecebe->IndiceDeEtiquetas,free);
	free(pecebe);
}

