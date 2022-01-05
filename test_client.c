#include "common.h"
#include "lista.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

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

int inicializarSocketCliente(int familia) {
    // Inicializa o socket do cliente
    int socketCliente = socket(familia, SOCK_DGRAM, 0);
    if(socketCliente < 0) {
        sairComMensagem("Erro ao criar o socket");
    }
    struct timeval timeout = {.tv_sec = 1, .tv_usec = 0};
    if (setsockopt(socketCliente, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        perror("Erro ao definir as configuracoes do socket");
    }
    return socketCliente;
}

int inicializarSocketClienteIPv4() {
    // Inicializa o socket do cliente com IPv4
    return inicializarSocketCliente(AF_INET);
}

int inicializarSocketClienteIPv6() {
    // Inicializa o socket do cliente com IPv6
    return inicializarSocketCliente(AF_INET6);
}

void enviarEReceberMensagemServidorTurn(int socketClienteIPv4, int socketClienteIPv6, struct sockaddr_storage dadosServidor, char mensagem[BUFSZ]) {
    // Escolhe o socket a ser usado com base na versao do protocolo IP do servidor
    int socketCliente;
    if(dadosServidor.ss_family == AF_INET) {
        socketCliente = socketClienteIPv4;
    } else {
        socketCliente = socketClienteIPv6;
    }
    // Envia a mensagem e recebe a resposta
    enviarEReceberMensagem(socketCliente, dadosServidor, mensagem, 1);
}

void enviarEReceberMensagemServidor(int socketClienteIPv4, int socketClienteIPv6, struct sockaddr_storage dadosServidor, char mensagem[BUFSZ]) {
    // Escolhe o socket a ser usado com base na versao do protocolo IP do servidor
    int socketCliente;
    if(dadosServidor.ss_family == AF_INET) {
        socketCliente = socketClienteIPv4;
    } else {
        socketCliente = socketClienteIPv6;
    }
    // Imprime a mensagem enviada
    printf("> %s", mensagem);
    // Envia a mensagem e recebe a resposta
    enviarEReceberMensagem(socketCliente, dadosServidor, mensagem, 1);
}

int getNumeroDefensores(char mensagem[BUFSZ]) {
    int i;
    // Identifica o numero de defensores com base na mensagem de resposta do getdefenders
    int tamMsg = strlen(mensagem);
    int contVirgulas = 0;
    for(i=0; i<tamMsg; i++) {
        if(mensagem[i] == ',') {
            contVirgulas++;
        }
    }
    return (contVirgulas+1)/2;
}

void preencherListaPokemonsDefensores(int numeroDefensores, char mensagem[BUFSZ], struct PosPokemonDefensor defensores[]) {
    int i;
    int offset = 10;
    for(i=0; i<numeroDefensores; i++) {
        int lido;
        sscanf(mensagem+offset, "[%d, %d]%n", &defensores[i].posX, &defensores[i].posY, &lido);
        offset+=lido;
        if(i != numeroDefensores-1) {
            offset+=2;
        }
    }
}

void preencherListaPokemonsAtacantes(char mensagem[BUFSZ], struct Lista* listaPokemonsAtacantes) {
    char lixo[BUFSZ];
    // Divide a mensagem em linhas
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
            strcpy(pokemonAtacante->nome, nome);
            adicionarElemento(listaPokemonsAtacantes, *pokemonAtacante);
            parte = strtok(NULL, "\n");
        }
    }
}

void comunicarComServidor(int socketClienteIPv4, int socketClienteIPv6, struct sockaddr_storage dadosServidor[4]) {
    int i, j, k;

    // Inicia o jogo
    char mensagem[BUFSZ];
    for(i=0 ; i<4; i++) {
        strcpy(mensagem, "start\n");
        enviarEReceberMensagemServidor(socketClienteIPv4, socketClienteIPv6, dadosServidor[i], mensagem);
    }
    // Busca os pokemons defensores
    char lixo[BUFSZ];
    strcpy(mensagem, "getdefenders\n");
    int servidorAReceber = rand()%4;
    enviarEReceberMensagemServidor(socketClienteIPv4, socketClienteIPv6, dadosServidor[servidorAReceber], mensagem);
    // Converte a mensagem para a lista de pokemons defensores
    int numeroDefensores = getNumeroDefensores(mensagem);
    struct PosPokemonDefensor defensores[numeroDefensores];
    preencherListaPokemonsDefensores(numeroDefensores, mensagem, defensores);
    // Loop para executar os turnos
    for(i=0; i<=MAX_TURNOS; i++) {
        // Inicializa uma lista encadeada para os pokemons atacantes
        struct Lista listaPokemonsAtacantes;
        inicializarLista(&listaPokemonsAtacantes);
        // Imprime o getturn assim como o cliente por teclado imprimiria
        printf("> getturn %d\n", i);
        // Para cada servidor
        for(j=0; j<4; j++) {
            // Envia a mensagem de getturn
            sprintf(mensagem, "%s %d\n", "getturn", i);
            enviarEReceberMensagemServidorTurn(socketClienteIPv4, socketClienteIPv6, dadosServidor[j], mensagem);
            // Adiciona os pokemons atacantes recebidos na lista
            preencherListaPokemonsAtacantes(mensagem, &listaPokemonsAtacantes);
        }
        // Para cada defensor
        for(j=0; j<numeroDefensores; j++) {
            // E para cada pokemon atacante
            for(k=0; k<listaPokemonsAtacantes.tamanho; k++) {
                struct PosPokemonDefensor defensor = defensores[j];
                struct PokemonAtacante atacante = *buscarElementoPos(&listaPokemonsAtacantes, k);
                // Verifica se o pokemon defensor pode atacar esse pokemon atacante
                if(getHitsPokemon(atacante.nome) > atacante.hits && defensor.posX == atacante.coluna && (defensor.posY == atacante.linha || defensor.posY-1 == atacante.linha)) {
                    // Caso o ataque possa ser feito, gera a mensagem de ataque
                    char mensagemAtaque[BUFSZ];
                    sprintf(mensagemAtaque, "shot %d %d %d\n", defensor.posX+1, defensor.posY+1, atacante.id);
                    enviarEReceberMensagemServidor(socketClienteIPv4, socketClienteIPv6, dadosServidor[atacante.linha], mensagemAtaque);
                    // Verifica se o ataque foi um sucesso
                    // Pelo algoritmo usado pelo cliente, os ataques devem sempre ser um sucesso
                    int posX, posY, id, status;
                    sscanf(mensagemAtaque, "%s %d %d %d %d", lixo, &posX, &posY, &id, &status);
                    if(status != 0) {
                        sairComMensagem("Erro ao atirar em pokemons atacantes");
                    }
                    // Se o ataque deu certo, atualiza a vida do pokemon atacado
                    atacante.hits++;
                    // Atualiza o pokemon atacado na lista
                    atualizarElemento(&listaPokemonsAtacantes, atacante);
                    break;
                }
            }
        }
        // Limpa a lista de pokemons atacantes
        limpaLista(&listaPokemonsAtacantes);
    }
    // Envia um getturn apos o fim do jogo para receber a mensagem de gameover
    sprintf(mensagem, "%s %d\n", "getturn", MAX_TURNOS+1);
    servidorAReceber = rand()%4;
    enviarEReceberMensagemServidor(socketClienteIPv4, socketClienteIPv6, dadosServidor[servidorAReceber], mensagem);
    // Envvia um quit para encerrar os servidores
    strcpy(mensagem, "quit\n");
    enviarEReceberMensagemServidor(socketClienteIPv4, socketClienteIPv6, dadosServidor[servidorAReceber], mensagem);
}

int main(int argc, char **argv) {
    // Verifica os parametros recebidos
    verificarParametros(argc, argv);
    struct sockaddr_storage dadosServidor[4];
    int i;
    if(argv[2] == NULL) {
        tratarParametroIncorreto(argv[0]);
    }
    // Converte o parâmetro da porta para unsigned short
    unsigned short porta = (unsigned short) atoi(argv[2]);

    // Inicializa os dados dos sockets dos servidores
    for(i=0; i<4; i++) {
        inicializarDadosSocket(argv[1], porta+i, &dadosServidor[i], argv[0]);
    }

    // Incializa os sockets do cliente
    int socketClienteIPv4 = inicializarSocketClienteIPv4();
    int socketClienteIPv6 = inicializarSocketClienteIPv6();
    // Realiza a comunicacao do cliente com os servidores
    comunicarComServidor(socketClienteIPv4, socketClienteIPv6, dadosServidor);
    // Encerra os sockets
    close(socketClienteIPv4);
    close(socketClienteIPv6);
	exit(EXIT_SUCCESS);
}