#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arpa/inet.h>

void sairComMensagem(char *msg) {
	perror(msg);
	exit(EXIT_FAILURE);
}

void converterEnderecoParaString(struct sockaddr *endereco, char *string, size_t tamanhoString) {
    int versao;
    char stringEndereco[INET6_ADDRSTRLEN + 1] = "";
    unsigned short porta;

    if(endereco->sa_family == AF_INET) { // Endereço é IPv4
        versao = 4;
        struct sockaddr_in *enderecov4 = (struct sockaddr_in *)endereco;
        if(!inet_ntop(AF_INET, &(enderecov4->sin_addr), stringEndereco, INET_ADDRSTRLEN + 1)) {
            sairComMensagem("Erro ao converter endereço para string com o comando ntop");
        }
        porta = ntohs(enderecov4->sin_port); // Converte a porta de network para host short
    } else {
        if(endereco->sa_family == AF_INET6) { // Endereço é IPv6
            versao = 6;
            struct sockaddr_in6 *enderecov6 = (struct sockaddr_in6 *)endereco;
            if(!inet_ntop(AF_INET6, &(enderecov6->sin6_addr), stringEndereco, INET6_ADDRSTRLEN + 1)) {
                sairComMensagem("Erro ao converter endereço para string com o comando ntop");
            }
            porta = ntohs(enderecov6->sin6_port); // Converte a porta de network para host short
        } else {
            sairComMensagem("Familia de protocolo desconhecida");
        }
    }
    if(string) { // Formata o endereço em string para um formato legível
        snprintf(string, tamanhoString, "IPv%d %s %hu", versao, stringEndereco, porta);
    }
}
