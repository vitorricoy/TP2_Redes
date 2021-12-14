#include "common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>

#define PROXIMA_MENSAGEM 0
#define PROXIMA_COMUNICACAO 1
#define PROXIMO_CLIENTE 2
#define ENCERRAR 3

#define BUFSZ 512

void tratarParametroIncorreto(char* comandoPrograma) {
    // Imprime o uso correto dos parâmetros do programa e encerra o programa
    printf("Uso: %s <v4|v6> <porta do servidor>\n", comandoPrograma);
    printf("Exemplo: %s v4 51511\n", comandoPrograma);
    exit(EXIT_FAILURE);
}

void verificarParametros(int argc, char** argv) {
    // Caso o programa possua menos de 2 argumentos imprime a mensagem do uso correto de seus parâmetros
    if(argc < 3) {
        tratarParametroIncorreto(argv[0]);
    }
}

void inicializarDadosSocket(const char* protocolo, const char* portaStr, struct sockaddr_storage* dadosSocket, char* comandoPrograma) {
    // Caso a porta seja nula, imprime o uso correto dos parâmetros
    if(portaStr == NULL) {
        tratarParametroIncorreto(comandoPrograma);
    }
    
    // Converte o parâmetro da porta para unsigned short
    unsigned short porta = (unsigned short) atoi(portaStr); // unsigned short


    if(porta == 0) { // Se a porta dada é inválida imprime a mensagem de uso dos parâmetros
        tratarParametroIncorreto(comandoPrograma);
    }

    // Converte a porta para network short
    porta = htons(porta);

    // Inicializa a estrutura de dados do socket
    memset(dadosSocket, 0, sizeof(*dadosSocket));
    if(strcmp(protocolo, "v4") == 0) { // Verifica se o protocolo escolhido é IPv4
        // Inicializa a estrutura do IPv4
        struct sockaddr_in *dadosSocketv4 = (struct sockaddr_in*)dadosSocket;
        dadosSocketv4->sin_family = AF_INET;
        dadosSocketv4->sin_addr.s_addr = INADDR_ANY;
        dadosSocketv4->sin_port = porta;
    } else {
        if(strcmp(protocolo, "v6") == 0) { // Verifica se o protocolo escolhido é IPv6
            // Inicializa a estrutura do IPv6
            struct sockaddr_in6 *dadosSocketv6 = (struct sockaddr_in6*)dadosSocket;
            dadosSocketv6->sin6_family = AF_INET6;
            dadosSocketv6->sin6_addr = in6addr_any;
            dadosSocketv6->sin6_port = porta;
        } else { // O parâmetro do protocolo não era 'v4' ou 'v6', portanto imprime o uso correto dos parâmetros
            tratarParametroIncorreto(comandoPrograma);
        }
    }
}

int inicializarSocketServidor(struct sockaddr_storage* dadosSocket) {
    // Inicializa o socket do servidor
    int socketServidor;
    socketServidor = socket(dadosSocket -> ss_family, SOCK_DGRAM, 0);

    if(socketServidor == -1) {
        sairComMensagem("Erro ao iniciar o socket");
    }
    
    // Define as opções do socket do servidor
    int enable = 1;
    if(setsockopt(socketServidor, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) != 0) {
        sairComMensagem("Erro ao definir as opcoes do socket");
    }

    return socketServidor;
}

// Retorna um novo ponteiro da string deslocado para o caractere após o espaço encontrado
char* extrairStringAteEspaco(char* string, char* dest) {
    int posicao;
    int tamanhoString = strlen(string);
    // Preenche o destino com o conteúdo da string até o primeiro espaço encontrado
    for(posicao = 0; posicao < tamanhoString; posicao++) {
        if(string[posicao] == ' ') {
            dest[posicao] = '\0';
            // Retorna a string deslocada para a posição depois do espaço
            return string+posicao+1;
        } else {
            dest[posicao] = string[posicao];
        }
    }
    // Se não encontrar um espaço retorna NULL
    dest[posicao] = '\0';
    return NULL;
}

int mensagemInvalida(char* mensagem) {
    if(mensagem == NULL) {
        return 0;
    }
    int tamanhoMensagem = strlen(mensagem);
    int posicao;
    // Verifica se 'mensagem' consiste apenas de letras minúsculas, números, espaços e '\n's
    for(posicao = 0; posicao < tamanhoMensagem; posicao++) {
        if((!isalnum(mensagem[posicao]) && mensagem[posicao] != ' ' && mensagem[posicao] != '\n') || (mensagem[posicao] >= 'A' && mensagem[posicao] <= 'Z')) { // letra minuscula, numero ou espaço
            return 1;
        }
    }
    return 0;
}

void enviarMensagem(char* mensagem, int socketServidor, struct sockaddr_storage dadosSocketCliente) {
    // Coloca um '\n' no fim da mensagem a ser enviada
    strcat(mensagem, "\n");
    // Envia o conteudo de 'mensagem' para o cliente
    size_t tamanhoMensagemEnviada = sendto(socketServidor, (const char *)mensagem, strlen(mensagem), MSG_CONFIRM, (const struct sockaddr *) &dadosSocketCliente, sizeof(dadosSocketCliente));
    if (strlen(mensagem) != tamanhoMensagemEnviada) {
        sairComMensagem("Erro ao enviar mensagem ao cliente");
    }
}

void bindServidor(int socketServidor, struct sockaddr_storage* dadosSocket) {
    struct sockaddr *enderecoSocket = (struct sockaddr*)(dadosSocket);

    // Dá bind do servidor na porta recebida por parâmetro
    if(bind(socketServidor, enderecoSocket, sizeof(*dadosSocket)) != 0) {
        sairComMensagem("Erro ao dar bind no endereço e porta para o socket");
    }
}

struct sockaddr_storage receberMensagem(int socketServidor, struct sockaddr_storage dadosSocket, char mensagem[BUFSZ]) {
    memset(mensagem, 0, BUFSZ);
    size_t tamanhoMensagem = 0;
    struct sockaddr_storage dadosSocketCliente;
    // Recebe mensagens do cliente enquanto elas não terminarem com \n
    do {
        socklen_t len = sizeof(dadosSocketCliente); 
        size_t tamanhoLidoAgora = recvfrom(socketServidor, (char *)mensagem, BUFSZ, MSG_WAITALL, ( struct sockaddr *) &dadosSocketCliente, &len);
        if(tamanhoLidoAgora == 0) {
            break;
        }
        tamanhoMensagem += tamanhoLidoAgora;
    } while(mensagem[strlen(mensagem)-1] != '\n');

    mensagem[tamanhoMensagem] = '\0';
    return dadosSocketCliente;
}

int tratarMensagensRecebidas(char mensagem[BUFSZ], int socketServidor, struct sockaddr_storage dadosSocketCliente) {
    if(strcmp(mensagem, "start\n") == 0) {
        char resposta[BUFSZ];
        strcpy(resposta, "game started: path 1");
        enviarMensagem(resposta, socketServidor, dadosSocketCliente);
    }
    // Identifica que o servidor deve receber o próximo recv do cliente
    return PROXIMA_COMUNICACAO;
}

int receberETratarMensagemCliente(int socketServidor, struct sockaddr_storage dadosSocket) {
    char mensagem[BUFSZ];
    struct sockaddr_storage dadosSocketCliente = receberMensagem(socketServidor, dadosSocket, mensagem);

    int retorno = tratarMensagensRecebidas(mensagem, socketServidor, dadosSocketCliente);


    
    if(retorno == ENCERRAR || retorno == PROXIMO_CLIENTE) { // Se o servidor deve encerrar ou se conectar com outro cliente
        return retorno;
    }
    // Identifica que o servidor deve receber o próximo recv do cliente
    return PROXIMA_COMUNICACAO;
}

void esperarPorConexoesCliente(int socketServidor, struct sockaddr_storage dadosSocket) {
    // Enquanto o servidor estiver ativo ele deve receber conexões de clientes
    while (1) {
        int retorno = receberETratarMensagemCliente(socketServidor, dadosSocket);
        if(retorno == ENCERRAR) { // Caso o servidor deve ser encerrado finaliza o laço de conexões de clientes
            return;
        }
    }
}

void inicializaServidor(unsigned short porta, char* versao, int i) {
    int pid = fork();
    if(pid == -1) {
        sairComMensagem("Erro ao executar o comando fork");
    }
    if(pid == 0) {
        return;
    }
    char programa[15];
    sprintf(programa, "./server%d", i);
    char* args[4];
    args[1] = (char*) malloc(sizeof(char)*15);
    args[1] = (char*) malloc(sizeof(char)*10);
    args[2] = (char*) malloc(sizeof(char)*10);
    args[3] = NULL;
    char portaStr[10];
    char versaoStr[10];
    sprintf(portaStr, "%d", porta);
    strcpy(versaoStr, versao);
    strcpy(args[0], programa);
    strcpy(args[1], versaoStr);
    strcpy(args[2], portaStr);
    int ret = execv(programa, args);
    if(ret < 0) {
        sairComMensagem("Erro ao executar o comando execv");
    }
}

int main(int argc, char** argv) {
    
    verificarParametros(argc, argv);

    // Converte o parâmetro da porta para unsigned short
    unsigned short porta = (unsigned short) atoi(argv[2]); // unsigned short
    
    inicializaServidor(porta+1, argv[1], 2);

    inicializaServidor(porta+2, argv[1], 3);

    inicializaServidor(porta+3, argv[1], 4);

    struct sockaddr_storage dadosSocket;
    
    inicializarDadosSocket(argv[1], argv[2], &dadosSocket, argv[0]);

    int socketServidor = inicializarSocketServidor(&dadosSocket);

    bindServidor(socketServidor, &dadosSocket);

    esperarPorConexoesCliente(socketServidor, dadosSocket);

    close(socketServidor);
    exit(EXIT_SUCCESS);
}