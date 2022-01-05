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
    printf("Uso: %s <ip do servidor> <porta do servidor> start\n", comandoPrograma);
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

void enviarEReceberMensagemServidor(int socketClienteIPv4, int socketClienteIPv6, struct sockaddr_storage dadosServidor, char mensagem[BUFSZ]) {
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

void leMensagemEntrada(char mensagem[BUFSZ]) {
    printf("> ");
    // Lê a mensagem digitada e a salva em 'mensagem'
    memset(mensagem, 0, sizeof(*mensagem));
    fgets(mensagem, BUFSZ-1, stdin);
    // Caso a mensagem lida não termine com \n coloca um \n
    if(mensagem[strlen(mensagem)-1] != '\n') {
        strcat(mensagem, "\n");
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
    int i;
    // Executa jogos ate que o cliente receba um 'quit'
    while(1) { // Loop de todos os jogos
        // Inicia o jogo
        char mensagem[BUFSZ];
        printf("> %s", "start\n");
        for(i=0 ; i<4; i++) {
            strcpy(mensagem, "start\n");
            enviarEReceberMensagemServidor(socketClienteIPv4, socketClienteIPv6, dadosServidor[i], mensagem);
        }
        // Busca os pokemons defensores
        strcpy(mensagem, "getdefenders\n");
        printf("> %s", mensagem);
        // Escolhe um servidor para receber as mensagens que podem ser recebidas por qualquer servidor
        int servidorAReceber = rand()%4;
        enviarEReceberMensagemServidor(socketClienteIPv4, socketClienteIPv6, dadosServidor[servidorAReceber], mensagem);
        // Cria uma lista dos pokemons atacantes
        struct Lista listaPokemonsAtacantes;
        inicializarLista(&listaPokemonsAtacantes);
        // Flag para indicar se o cliente leu uma mensagem 'quit'
        int encerrouServidores = 0;
        while(1) { // Loop de um jogo
            // Lê mensagens recebidas pelo teclado do cliente
            leMensagemEntrada(mensagem);
            // Verifica se a mensagem e de getturn
            if(strncmp("getturn ", mensagem, 8) == 0) {
                // Limpa a lista de atacantes, ja que ela sera atualizada
                limpaLista(&listaPokemonsAtacantes);
                // Flag para indicar um encerramento causado por um gameover
                int sair = 0;
                // Array para guardar a mensagem original
                char mensagemOriginal[BUFSZ];
                strcpy(mensagemOriginal, mensagem);
                for(i=0; i<4; i++) {
                    strcpy(mensagem, mensagemOriginal);
                    // Envia a mensagem para os quatro servidores
                    enviarEReceberMensagemServidor(socketClienteIPv4, socketClienteIPv6, dadosServidor[i], mensagem);
                    // Se deu gameover com sucesso
                    if(strncmp("gameover 0 ", mensagem, 11) == 0) {
                        // Encerra os servidores, ja que o jogo foi concluido com sucesso
                        strcpy(mensagem, "quit\n");
                        printf("> %s", mensagem);
                        enviarEReceberMensagemServidor(socketClienteIPv4, socketClienteIPv6, dadosServidor[i], mensagem);
                        sair = 1;
                        encerrouServidores = 1;
                        break;
                    }
                    // Se deu gameover com erro
                    if(strncmp("gameover 1 ", mensagem, 11) == 0) {
                        // Reinicia o jogo ja que ele foi finalizado com erro
                        sair = 1;
                        break;
                    }
                    // Se nao deu gameover preenche a lista de atacantes
                    preencherListaPokemonsAtacantes(mensagem, &listaPokemonsAtacantes);
                }
                if(sair) break;
            } else {
                // Verifica se a mensagem e de shot
                if(strncmp("shot ", mensagem, 5) == 0) {
                    char lixo[BUFSZ];
                    int posX, posY, idAtacante;
                    // Le a posicao do defensor e o id do atacante da mensagem shot
                    sscanf(mensagem, "%s %d %d %d", lixo, &posX, &posY, &idAtacante);
                    struct PokemonAtacante* atacante = buscarElementoId(&listaPokemonsAtacantes, idAtacante);
                    // Se o id informado do pokemon atacante e invalido
                    if(atacante == NULL) {
                        printf("Id do pokemon atacante informado nao existe\n");
                    } else {
                        // Se o id e valido envia o comando de ataque para o servidor correto
                        enviarEReceberMensagemServidor(socketClienteIPv4, socketClienteIPv6, dadosServidor[atacante->linha], mensagem);
                    } 
                } else { // Nesse ponto a mensagem pode ser getdefenders, quit ou invalida
                    enviarEReceberMensagemServidor(socketClienteIPv4, socketClienteIPv6, dadosServidor[servidorAReceber], mensagem);
                    if(strcmp("exited\n", mensagem) == 0) {
                        // Mensagem foi de quit
                        encerrouServidores = 1;
                        break;
                    }
                    // Se deu gameover com sucesso
                    if(strncmp("gameover 0 ", mensagem, 11) == 0) {
                        // Encerra os servidores, ja que o jogo foi concluido com sucesso
                        strcpy(mensagem, "quit\n");
                        printf("> %s", mensagem);
                        enviarEReceberMensagemServidor(socketClienteIPv4, socketClienteIPv6, dadosServidor[servidorAReceber], mensagem);
                        encerrouServidores = 1;
                        break;
                    }
                    // Se deu gameover com erro
                    if(strncmp("gameover 1 ", mensagem, 11) == 0) {
                        // Reinicia o jogo ja que ele foi finalizado com erro
                        break;
                    }
                }
            }
        }
        if(encerrouServidores) break;
    }
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