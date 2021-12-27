#include "common.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define BUFSZ 512

void tratarParametroIncorreto(char* comandoPrograma) {
    // Imprime o uso correto dos parâmetros do programa e encerra o programa
    printf("Uso: %s <ip do servidor> <porta do servidor>\n", comandoPrograma);
    printf("Exemplo: %s 127.0.0.1 51511\n", comandoPrograma);
    exit(EXIT_FAILURE);
}

void verificarParametros(int argc, char **argv) {
    // Caso o programa possua menos de 2 argumentos imprime a mensagem do uso correto de seus parâmetros
    if(argc < 3) {
        tratarParametroIncorreto(argv[0]);
    }
}

void inicializarDadosSocket(const char *enderecoStr, unsigned short porta, struct sockaddr_storage *dadosSocket, char* comandoPrograma) {
    // Se a porta ou o endereço não estiverem certos imprime o uso correto dos parâmetros
    if(enderecoStr == NULL) {
        tratarParametroIncorreto(comandoPrograma);
    }

    porta = htons(porta); // Converte a porta para network short

    struct in_addr inaddr4; // Struct do IPv4
    if(inet_pton(AF_INET, enderecoStr, &inaddr4)) { // Tenta criar um socket IPv4 com os parâmetros
        struct sockaddr_in *dadosSocketv4 = (struct sockaddr_in*)dadosSocket;
        dadosSocketv4->sin_family = AF_INET;
        dadosSocketv4->sin_port = porta;
        dadosSocketv4->sin_addr = inaddr4;
    } else {
        struct in6_addr inaddr6; // Struct do IPv6
        if (inet_pton(AF_INET6, enderecoStr, &inaddr6)) { // Tenta criar um socket IPv6 com os parâmetros
            struct sockaddr_in6 *dadosSocketv6 = (struct sockaddr_in6*)dadosSocket;
            dadosSocketv6->sin6_family = AF_INET6;
            dadosSocketv6->sin6_port = porta;
            memcpy(&(dadosSocketv6->sin6_addr), &inaddr6, sizeof(inaddr6));
        } else { // Parâmetros incorretos, pois não é IPv4 nem IPv6
            tratarParametroIncorreto(comandoPrograma);
        }
    }
}

int inicializarSocketClienteIPv4() {
    int socketCliente = socket(AF_INET, SOCK_DGRAM, 0);
    if(socketCliente < 0) {
        sairComMensagem("Erro ao criar o socket");
    }
    return socketCliente;
}

int inicializarSocketClienteIPv6() {
    int socketCliente = socket(AF_INET6, SOCK_DGRAM, 0);
    if(socketCliente < 0) {
        sairComMensagem("Erro ao criar o socket");
    }
    return socketCliente;
}

void leMensagemEntrada(char mensagem[BUFSZ]) {
    // Lê a mensagem digitada e a salva em 'mensagem'
    memset(mensagem, 0, sizeof(*mensagem));
    fgets(mensagem, BUFSZ-1, stdin);
    // Caso a mensagem lida não termine com \n coloca um \n
    if(mensagem[strlen(mensagem)-1] != '\n') {
        strcat(mensagem, "\n");
    }
}

void enviaMensagemServidor(int socketClienteIPv4, int socketClienteIPv6, struct sockaddr_storage dadosServidor, char mensagem[BUFSZ]) {
    int socketCliente;
    if(dadosServidor.ss_family == AF_INET) {
        socketCliente = socketClienteIPv4;
    } else {
        socketCliente = socketClienteIPv6;
    }
    // Envia o parâmetro 'mensagem' para o servidor
    size_t tamanhoMensagemEnviada = sendto(socketCliente, (const char *)mensagem, strlen(mensagem), MSG_CONFIRM, (const struct sockaddr *) &dadosServidor, sizeof(dadosServidor));
    if (strlen(mensagem) != tamanhoMensagemEnviada) {
        sairComMensagem("Erro ao enviar mensagem");
    }
    printf("> %s", mensagem);
}

void recebeMensagemServidor(int socketClienteIPv4, int socketClienteIPv6, struct sockaddr_storage dadosServidor, char mensagem[BUFSZ]) {
    int socketCliente;
    if(dadosServidor.ss_family == AF_INET) {
        socketCliente = socketClienteIPv4;
    } else {
        socketCliente = socketClienteIPv6;
    }
    memset(mensagem, 0, BUFSZ);
    size_t tamanhoMensagem = 0;
    // Recebe mensagens do servidor enquanto elas não terminarem com \n
    do {
        socklen_t len = sizeof(dadosServidor);
        size_t tamanhoLidoAgora = recvfrom(socketCliente, (char *)mensagem, BUFSZ, MSG_WAITALL, (struct sockaddr *) &dadosServidor, &len);
        if(tamanhoLidoAgora == 0) {
            break;
        }
        tamanhoMensagem += tamanhoLidoAgora;
    }while(mensagem[strlen(mensagem)-1] != '\n');
    mensagem[tamanhoMensagem] = '\0';
    printf("< %s\n", mensagem);
    // Caso a mensagem lida tenha tamanho zero, o servidor foi desconectado
    if(strlen(mensagem) == 0) {
        // Conexão caiu
        exit(EXIT_SUCCESS);
    }
}

void comunicarComServidor(int socketClienteIPv4, int socketClienteIPv6, struct sockaddr_storage dadosServidor[4]) {
    int i;

    // Inicia o jogo

    for(i=0 ; i<4; i++) {
        char mensagem[BUFSZ];
        strcpy(mensagem, "start\n");
        enviaMensagemServidor(socketClienteIPv4, socketClienteIPv6, dadosServidor[i], mensagem);
        recebeMensagemServidor(socketClienteIPv4, socketClienteIPv6, dadosServidor[i], mensagem);
    }

    char mensagem[BUFSZ];
    strcpy(mensagem, "getdefenders\n");
    int servidorAReceber = rand()%4;
    enviaMensagemServidor(socketClienteIPv4, socketClienteIPv6, dadosServidor[servidorAReceber], mensagem);
    recebeMensagemServidor(socketClienteIPv4, socketClienteIPv6, dadosServidor[servidorAReceber], mensagem);
    printf("%s", mensagem);

    for(i=0 ; i<4; i++) {
        char mensagem[BUFSZ];
        strcpy(mensagem, "getturn 0\n");
        enviaMensagemServidor(socketClienteIPv4, socketClienteIPv6, dadosServidor[i], mensagem);
        recebeMensagemServidor(socketClienteIPv4, socketClienteIPv6, dadosServidor[i], mensagem);
        printf("%s", mensagem);
    }
    
    // Laço para a comunicação do cliente com o servidor
    while(1) {
        leMensagemEntrada(mensagem);
        enviaMensagemServidor(socketClienteIPv4, socketClienteIPv6, dadosServidor[servidorAReceber], mensagem);
        recebeMensagemServidor(socketClienteIPv4, socketClienteIPv6, dadosServidor[servidorAReceber], mensagem);
        printf("%s", mensagem);
    }
}

int main(int argc, char **argv) {
    verificarParametros(argc, argv);
    struct sockaddr_storage dadosServidor[4];
    int i;
    if(argv[2] == NULL) {
        tratarParametroIncorreto(argv[0]);
    }
    // Converte o parâmetro da porta para unsigned short
    unsigned short porta = (unsigned short) atoi(argv[2]); // unsigned short

    for(i=0; i<4; i++) {
        inicializarDadosSocket(argv[1], porta+i, &dadosServidor[i], argv[0]);
    }

    int socketClienteIPv4 = inicializarSocketClienteIPv4();
    int socketClienteIPv6 = inicializarSocketClienteIPv6();
    comunicarComServidor(socketClienteIPv4, socketClienteIPv6, dadosServidor);
    close(socketClienteIPv4);
    close(socketClienteIPv6);
	exit(EXIT_SUCCESS);
}