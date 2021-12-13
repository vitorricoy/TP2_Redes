#include "common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>

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

void enviarMensagem(char* mensagem, int socketCliente) {
    // Coloca um '\n' no fim da mensagem a ser enviada
    strcat(mensagem, "\n");
    // Envia o conteudo de 'mensagem' para o cliente
    size_t tamanhoMensagemEnviada = send(socketCliente, mensagem, strlen(mensagem), 0);
    if (strlen(mensagem) != tamanhoMensagemEnviada) {
        sairComMensagem("Erro ao enviar mensagem ao cliente");
    }
}

void escutarPorConexoes(int socketServidor, struct sockaddr_storage* dadosSocket) {
    struct sockaddr *enderecoSocket = (struct sockaddr*)(dadosSocket);

    // Dá bind do servidor na porta recebida por parâmetro
    if(bind(socketServidor, enderecoSocket, sizeof(*dadosSocket)) != 0) {
        sairComMensagem("Erro ao dar bind no endereço e porta para o socket");
    }

    // Faz o servidor escutar por conexões
    // Limite de 100 conexões na fila de espera
    if(listen(socketServidor, 100) != 0) {
        sairComMensagem("Erro ao escutar por conexoes no servidor");
    }
}

int aceitarSocketCliente(int socketServidor) {
    // Inicializa as estruturas do socket do cliente
    struct sockaddr_storage dadosSocketCliente;
    struct sockaddr* enderecoSocketCliente = (struct sockaddr*)(&dadosSocketCliente);
    socklen_t tamanhoEnderecoSocketCliente = sizeof(enderecoSocketCliente);

    // Aceita uma conexão de um cliente
    int socketCliente = accept(socketServidor, enderecoSocketCliente, &tamanhoEnderecoSocketCliente);

    if(socketCliente == -1) {
        sairComMensagem("Erro ao aceitar a conexao de um cliente");
    }
    return socketCliente;
}

void receberMensagem(int socketCliente, char mensagem[BUFSZ]) {
    memset(mensagem, 0, BUFSZ);
    size_t tamanhoMensagem = 0;
    // Recebe mensagens do cliente enquanto elas não terminarem com \n
    do {
        size_t tamanhoLidoAgora = recv(socketCliente, mensagem+tamanhoMensagem, BUFSZ-(int)tamanhoMensagem-1, 0);
        if(tamanhoLidoAgora == 0) {
            break;
        }
        tamanhoMensagem += tamanhoLidoAgora;
    } while(mensagem[strlen(mensagem)-1] != '\n');

    mensagem[tamanhoMensagem] = '\0';
}

int tratarMensagemRecebida(char* mensagem, int socketCliente) {

}

int tratarMensagensRecebidas(char mensagem[BUFSZ], int socketCliente) {
    // Analisa cada mensagem recebida separada por \n
    char *parteMensagem;
    for(parteMensagem = strtok(mensagem, "\n"); parteMensagem != NULL; parteMensagem = strtok(NULL, "\n")) {
        int retorno = tratarMensagemRecebida(parteMensagem, socketCliente);
        // Indica caso seja necessário ignorar as próximas mensagens recebidas
        if(retorno == PROXIMA_COMUNICACAO || retorno == PROXIMO_CLIENTE || retorno == ENCERRAR) {
            return retorno;
        }
    }
    // Identifica que o servidor deve receber o próximo recv do cliente
    return PROXIMA_COMUNICACAO;
}

int receberETratarMensagemCliente(int socketCliente) {
    char mensagem[BUFSZ];
    receberMensagem(socketCliente, mensagem);
    if(strlen(mensagem) == 0) {
        // Identifica que o servidor deve se conectar com outro cliente
        return PROXIMO_CLIENTE;
    }

    int retorno = tratarMensagensRecebidas(mensagem, socketCliente);
    
    if(retorno == ENCERRAR || retorno == PROXIMO_CLIENTE) { // Se o servidor deve encerrar ou se conectar com outro cliente
        return retorno;
    }
    // Identifica que o servidor deve receber o próximo recv do cliente
    return PROXIMA_COMUNICACAO;
}

int tratarConexaoCliente(int socketServidor) {
    int socketCliente = aceitarSocketCliente(socketServidor);
    // Enquanto a conexão com o cliente esteja ativa
    while(1) {
        int retorno = receberETratarMensagemCliente(socketCliente);
        if(retorno == PROXIMO_CLIENTE || retorno == ENCERRAR) { // Se o servidor deve encerrar ou se conectar com outro cliente
            // Encerra a conexão com o cliente
            close(socketCliente);
            return retorno;
        }
    }
    // Encerra a conexão com o cliente
    close(socketCliente);
    // Indica que o servidor deve se conectar com outro cliente
    return PROXIMO_CLIENTE;
}

void esperarPorConexoesCliente(int socketServidor) {
    // Enquanto o servidor estiver ativo ele deve receber conexões de clientes
    while (1) {
        int retorno = tratarConexaoCliente(socketServidor);
        if(retorno == ENCERRAR) { // Caso o servidor deve ser encerrado finaliza o laço de conexões de clientes
            return;
        }
    }
}

void inicializaServidor(int porta, char* versao, int i) {
    char programa[BUFSZ];
    sprintf("./server%d %s %d", i, versao, porta)
    system(programa);
}

int main(int argc, char** argv) {
    
    verificarParametros(argc, argv);

    // Converte o parâmetro da porta para unsigned short
    unsigned short porta = (unsigned short) atoi(portaStr); // unsigned short

    printf("Servidor 1\n");
    
    inicializaServidor(porta+1, argv[1], 2);

    printf("Servidor 2\n");

    inicializaServidor(porta+2, argv[1], 3);

    printf("Servidor 3\n");

    inicializaServidor(porta+3, argv[1], 4);

    printf("Servidor 4\n");

    struct sockaddr_storage dadosSocket;
    
    inicializarDadosSocket(argv[1], argv[2], &dadosSocket, argv[0]);

    int socketServidor = inicializarSocketServidor(&dadosSocket);

    escutarPorConexoes(socketServidor, &dadosSocket);

    esperarPorConexoesCliente(socketServidor);

    exit(EXIT_SUCCESS);
}