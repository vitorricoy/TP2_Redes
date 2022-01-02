#pragma once

#include <stdlib.h>

#include <arpa/inet.h>

#define TAMANHO_NOME 32
#define BUFSZ 512

// Encerra o programa com a mensagem 'msg'
void sairComMensagem(char *msg);

// Busca quantos hits o pokemon com o nome recebido pode receber
int getHitsPokemon(char nome[TAMANHO_NOME]);

// Envia uma mensagem e espera a resposta, dando timeout em 1 segundo indicando perda de pacotes
void enviarEReceberMensagem(int socketCliente, struct sockaddr_storage dadosServidor, char mensagem[BUFSZ], int debug);