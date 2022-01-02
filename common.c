#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arpa/inet.h>

#define TAMANHO_NOME 32

void sairComMensagem(char *msg) {
	perror(msg);
	exit(EXIT_FAILURE);
}

int getHitsPokemon(char nome[TAMANHO_NOME]) {
    if(strcmp(nome, "Mewtwo") == 0) {
        return 3;
    }
    if(strcmp(nome, "Lugia") == 0) {
        return 2;
    }
    return 1;
}