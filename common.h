#pragma once

#include <stdlib.h>

#include <arpa/inet.h>

#define TAMANHO_NOME 32

// Encerra o programa com a mensagem 'msg'
void sairComMensagem(char *msg);

int getHitsPokemon(char nome[TAMANHO_NOME]);