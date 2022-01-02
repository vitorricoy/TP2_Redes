#include "common.h"
#include "lista.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <arpa/inet.h>

#define TAMANHO_NOME 32
#define BUFSZ 512
#define MAX_TURNOS 50
#define NUMERO_COLUNAS 4

struct PosPokemonDefensor {
    int posX;
    int posY;
};

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
    struct timeval timeout = {.tv_sec = 1, .tv_usec = 0};
    if (setsockopt(socketCliente, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        perror("Erro ao definir as configuracoes do socket");
    }
    return socketCliente;
}

int inicializarSocketClienteIPv6() {
    int socketCliente = socket(AF_INET6, SOCK_DGRAM, 0);
    if(socketCliente < 0) {
        sairComMensagem("Erro ao criar o socket");
    }
    struct timeval timeout = {.tv_sec = 1, .tv_usec = 0};
    if (setsockopt(socketCliente, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        perror("Erro ao definir as configuracoes do socket");
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

void enviaERecebeMensagemServidor(int socketClienteIPv4, int socketClienteIPv6, struct sockaddr_storage dadosServidor, char mensagem[BUFSZ]) {
    int socketCliente;
    if(dadosServidor.ss_family == AF_INET) {
        socketCliente = socketClienteIPv4;
    } else {
        socketCliente = socketClienteIPv6;
    }
    // Envia o parâmetro 'mensagem' para o servidor
    size_t tamanhoMensagemEnviada = sendto(socketCliente, (const char *)mensagem, strlen(mensagem), 0, (const struct sockaddr *) &dadosServidor, sizeof(dadosServidor));
    if(strlen(mensagem) != tamanhoMensagemEnviada) {
        sairComMensagem("Erro ao enviar mensagem");
    }
    printf("> %s", mensagem);
    char mensagemResposta[BUFSZ];
    memset(mensagemResposta, 0, BUFSZ);
    socklen_t len = sizeof(dadosServidor);
    size_t tamanhoMensagem = recvfrom(socketCliente, (char *)mensagemResposta, BUFSZ, MSG_WAITALL, (struct sockaddr *) &dadosServidor, &len);
    while(tamanhoMensagem == -1) {
        if(errno != EAGAIN && errno != EWOULDBLOCK) {
            perror("Erro ao receber mensagem de resposta");
        }
        printf("Timeout no recvfrom\n");
        size_t tamanhoMensagemEnviada = sendto(socketCliente, (const char *)mensagem, strlen(mensagem), 0, (const struct sockaddr *) &dadosServidor, sizeof(dadosServidor));
        if (strlen(mensagem) != tamanhoMensagemEnviada) {
            sairComMensagem("Erro ao enviar mensagem");
        }
        printf("> %s", mensagem);
        memset(mensagemResposta, 0, BUFSZ);
        tamanhoMensagem = recvfrom(socketCliente, (char *)mensagemResposta, BUFSZ, MSG_WAITALL, (struct sockaddr *) &dadosServidor, &len);
    } 
    mensagemResposta[tamanhoMensagem] = '\0';
    printf("< %s", mensagemResposta);
    memcpy(mensagem, mensagemResposta, BUFSZ);
}

void comunicarComServidor(int socketClienteIPv4, int socketClienteIPv6, struct sockaddr_storage dadosServidor[4]) {
    int i, j, k;

    // Inicia o jogo
    char mensagem[BUFSZ];
    for(i=0 ; i<4; i++) {
        strcpy(mensagem, "start\n");
        enviaERecebeMensagemServidor(socketClienteIPv4, socketClienteIPv6, dadosServidor[i], mensagem);
    }

    char lixo[BUFSZ];
    strcpy(mensagem, "getdefenders\n");
    int servidorAReceber = rand()%4;
    enviaERecebeMensagemServidor(socketClienteIPv4, socketClienteIPv6, dadosServidor[servidorAReceber], mensagem);
    int tamMsg = strlen(mensagem);
    int contVirgulas = 0;
    for(i=0; i<tamMsg; i++) {
        if(mensagem[i] == ',') {
            contVirgulas++;
        }
    }
    int numeroDefensores = (contVirgulas+1)/2;
    struct PosPokemonDefensor defensores[numeroDefensores];
    int offset = 10;
    for(i=0; i<numeroDefensores; i++) {
        int lido;
        sscanf(mensagem+offset, "[%d, %d]%n", &defensores[i].posX, &defensores[i].posY, &lido);
        offset+=lido;
        if(i != numeroDefensores-1) {
            offset+=2;
        }
    }
    for(i=0; i<=MAX_TURNOS; i++) {
        apagaLista();

        for(j=0; j<4; j++) {
            sprintf(mensagem, "%s %d\n", "getturn", i);
            enviaERecebeMensagemServidor(socketClienteIPv4, socketClienteIPv6, dadosServidor[j], mensagem);
            char* parte;
            parte = strtok(mensagem, "\n");
            int idLinha, idColuna;
            sscanf(parte, "%s %d", lixo, &idLinha);
            parte = strtok(NULL, "\n");
            while(parte != NULL) {
                parte = strtok(NULL, "\n");
                sscanf(parte, "%s %d", lixo, &idColuna);
                parte = strtok(NULL, "\n");
                while(parte != NULL && strncmp("turn ", parte, 5) != 0) {
                    struct PokemonAtacante* pokemonAtacante = (struct PokemonAtacante*) malloc(sizeof(struct PokemonAtacante));
                    int id, hits;
                    char nome[TAMANHO_NOME];
                    sscanf(parte, "%d %s %d", &id, nome, &hits);
                    pokemonAtacante->coluna = idColuna-1;
                    pokemonAtacante->linha = idLinha-1;
                    pokemonAtacante->hits = hits;
                    pokemonAtacante->id = id;
                    printf("%s: %s\n", parte, nome);
                    strcpy(pokemonAtacante->nome, nome);
                    adicionarElemento(*pokemonAtacante);
                    parte = strtok(NULL, "\n");
                }
            }
        }

        for(j=0; j<numeroDefensores; j++) {
            for(k=0; k<getTamanho(); k++) {
                struct PosPokemonDefensor defensor = defensores[j];
                struct PokemonAtacante atacante = *buscarElementoPos(k);
                if(getHitsPokemon(atacante.nome) > atacante.hits && defensor.posX == atacante.coluna && (defensor.posY == atacante.linha || defensor.posY-1 == atacante.linha)) {
                    // Gera mensagem de ataque
                    char mensagemAtaque[BUFSZ];
                    sprintf(mensagemAtaque, "shot %d %d %d\n", defensor.posX+1, defensor.posY+1, atacante.id);
                    printf("Atirando de (%d, %d) em (%d, %d)\n", defensor.posX, defensor.posY, atacante.coluna, atacante.linha);
                    printf("shot %d %d %d\n", defensor.posX+1, defensor.posY+1, atacante.id);
                    enviaERecebeMensagemServidor(socketClienteIPv4, socketClienteIPv6, dadosServidor[atacante.linha], mensagemAtaque);
                    int posX, posY, id, status;
                    sscanf(mensagemAtaque, "%s %d %d %d %d", lixo, &posX, &posY, &id, &status);
                    if(status != 0) {
                        sairComMensagem("Erro ao atirar em pokemons atacantes");
                    }
                    atacante.hits++;
                    atualizarElemento(atacante);
                    break;
                }
            }
        }
    }
    apagaLista();
    sprintf(mensagem, "%s %d\n", "getturn", MAX_TURNOS+1);
    servidorAReceber = rand()%4;
    enviaERecebeMensagemServidor(socketClienteIPv4, socketClienteIPv6, dadosServidor[servidorAReceber], mensagem);
    strcpy(mensagem, "quit\n");
    enviaERecebeMensagemServidor(socketClienteIPv4, socketClienteIPv6, dadosServidor[servidorAReceber], mensagem);
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