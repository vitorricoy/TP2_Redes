#include "common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <time.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PROXIMA_MENSAGEM 0
#define PROXIMA_COMUNICACAO 1
#define PROXIMO_CLIENTE 2
#define ENCERRAR 3

#define BUFSZ 512

int numeroDefensores = 6;
int numeroColunas = 4;


struct parametroThreadServidor {
    int socket;
    struct sockaddr_storage dadosSocket;
    int id;
};

struct PosPokemon {
    int posX;
    int posY;
};

struct PosPokemon* posPokemonsDefensores;

void tratarParametroIncorreto(char* comandoPrograma) {
    // Imprime o uso correto dos parâmetros do programa e encerra o programa
    printf("Uso: %s v4|v6 porta do servidor <numero de colunas no campo> <numero de pokemons defensores>\n", comandoPrograma);
    printf("Exemplo: %s v4 51511\n", comandoPrograma);
    exit(EXIT_FAILURE);
}

void verificarParametros(int argc, char** argv) {
    // Caso o programa possua menos de 2 argumentos imprime a mensagem do uso correto de seus parâmetros
    if(argc < 3) {
        tratarParametroIncorreto(argv[0]);
    }
}

void inicializarDadosSocket(const char* protocolo, const char* portaStr, struct sockaddr_storage* dadosSocket, char* comandoPrograma, unsigned short numeroServidor) {
    // Caso a porta seja nula, imprime o uso correto dos parâmetros
    if(portaStr == NULL) {
        tratarParametroIncorreto(comandoPrograma);
    }
    
    // Converte o parâmetro da porta para unsigned short
    unsigned short porta = (unsigned short) atoi(portaStr); // unsigned short

    porta += numeroServidor;


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

void gerarPokemonsDefensores() {
    int valoresJaGeradosY[numeroDefensores];
    int valoresJaGeradosX[numeroDefensores];
    int i, j;
    for(i=0; i<numeroDefensores; i++) {
        int recalcular = 1;
        int v1, v2;
        while(recalcular) {
            recalcular = 0;
            v1 = rand()%numeroColunas;
            v2 = rand()%5;
            for(j=0; j<i; j++) {
                if(v1 == valoresJaGeradosX[j] && v2 == valoresJaGeradosY[j]) {
                    recalcular = 1;
                    break;
                }
            }
        }
        valoresJaGeradosX[i] = v1;
        valoresJaGeradosY[i] = v2;
        struct PosPokemon posicaoPokemon;
        posicaoPokemon.posX = v1;
        posicaoPokemon.posY = v2;
        posPokemonsDefensores[i] = posicaoPokemon;
    }
}

int tratarMensagensRecebidas(char mensagem[BUFSZ], int socketServidor, struct sockaddr_storage dadosSocketCliente, int id) {
    printf("Recebeu mensagem %s", mensagem);
    if(strcmp(mensagem, "start\n") == 0) {
        char resposta[BUFSZ];
        strcpy(resposta, "game started: path ");
        char idThread[2];
        sprintf(idThread, "%d", id+1);
        strcat(resposta, idThread);
        enviarMensagem(resposta, socketServidor, dadosSocketCliente);
    }

    if(strcmp(mensagem, "getdefenders\n") == 0) {
        int i;
        gerarPokemonsDefensores();
        char resposta[BUFSZ];
        strcpy(resposta, "defender [");
        for(i=0; i<numeroDefensores; i++) {
            char pos[10];
            sprintf(pos, "%d", posPokemonsDefensores[i].posX);
            strcat(resposta, "[");
            strcat(resposta, pos);
            strcat(resposta, ", ");
            sprintf(pos, "%d", posPokemonsDefensores[i].posY);
            strcat(resposta, pos);
            if(i != numeroDefensores-1) {
                strcat(resposta, "], ");
            } else {
                strcat(resposta, "]");
            }
        }
        strcat(resposta, "]\n");
        enviarMensagem(resposta, socketServidor, dadosSocketCliente);
    }

    if(strncmp("getturn ", mensagem, 8) == 0) {
        // Eh mensagem de turno
    }
    // Identifica que o servidor deve receber o próximo recv do cliente
    return PROXIMA_COMUNICACAO;
}

int receberETratarMensagemCliente(int socketServidor, struct sockaddr_storage dadosSocket, int id) {
    char mensagem[BUFSZ];
    struct sockaddr_storage dadosSocketCliente = receberMensagem(socketServidor, dadosSocket, mensagem);

    int retorno = tratarMensagensRecebidas(mensagem, socketServidor, dadosSocketCliente, id);
    
    if(retorno == ENCERRAR || retorno == PROXIMO_CLIENTE) { // Se o servidor deve encerrar ou se conectar com outro cliente
        return retorno;
    }
    // Identifica que o servidor deve receber o próximo recv do cliente
    return PROXIMA_COMUNICACAO;
}

void* esperarPorConexoesCliente(void* param) {
    struct parametroThreadServidor parametros = *(struct parametroThreadServidor*) param;
    int socketServidor = parametros.socket;
    struct sockaddr_storage dadosSocket = parametros.dadosSocket;
    int id = parametros.id;
    printf("Servidor %d esperando conexão\n", id);
    // Enquanto o servidor estiver ativo ele deve receber conexões de clientes
    while (1) {
        int retorno = receberETratarMensagemCliente(socketServidor, dadosSocket, id);
        if(retorno == ENCERRAR) { // Caso o servidor deve ser encerrado finaliza o laço de conexões de clientes
            pthread_exit(NULL);
        }
    }

    pthread_exit(NULL);
}

int main(int argc, char** argv) {
    
    verificarParametros(argc, argv);

    if(argc >= 4) {
        numeroColunas = atoi(argv[3]);
        if(numeroColunas <= 0) {
            verificarParametros(argc, argv);
        }
    }

    if(argc >= 5) {
        numeroDefensores = atoi(argv[4]);
        if(numeroDefensores <= 0) {
            verificarParametros(argc, argv);
        }
    }

    srand(time(NULL));

    posPokemonsDefensores = (struct PosPokemon*) malloc(sizeof(struct PosPokemon)*numeroDefensores);

    struct sockaddr_storage dadosSocket[4];
    struct parametroThreadServidor parametros[4];
    int socketServidor[4];
    pthread_t threads[4];
    int i;
    
    for(i=0; i<4; i++) {
        inicializarDadosSocket(argv[1], argv[2], &dadosSocket[i], argv[0], i);
        socketServidor[i] = inicializarSocketServidor(&dadosSocket[i]);
        bindServidor(socketServidor[i], &dadosSocket[i]);
        
        parametros[i].socket = socketServidor[i];
        parametros[i].dadosSocket = dadosSocket[i];
        parametros[i].id = i;
        pthread_create(&threads[i], NULL, esperarPorConexoesCliente, (void*) &parametros[i]);
    }

    for(i=0; i<4; i++) {
        pthread_join(threads[i], NULL);
    }

    for(i=0; i<4; i++) {
        close(socketServidor[i]);
    }

    free(posPokemonsDefensores);
    
    exit(EXIT_SUCCESS);
}