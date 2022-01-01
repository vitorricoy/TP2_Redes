#include "common.h"
#include "lista.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <math.h>
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
#define MAX_TURNOS 50

int numeroDefensores = 20;
int numeroColunas = 4;


struct ParametroThreadServidor {
    int socket;
    struct sockaddr_storage dadosSocket;
    int id;
};

struct PosPokemonDefensor {
    int posX;
    int posY;
};

struct PosPokemonDefensor* posPokemonsDefensores;

int proximoIdAtacante = 1;
int pokemonsDestruidos = 0;
int chegaramNaPokedex = 0;
struct PokemonAtacante infoPokemonsAtacantes[BUFSZ];
struct sockaddr_storage dadosSockets[4];
int socketServidores[4];
int ambienteResetado = 1;
time_t inicio = 0;
int turnoAtual = -1;

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
        struct PosPokemonDefensor posicaoPokemon;
        posicaoPokemon.posX = v1;
        posicaoPokemon.posY = v2;
        posPokemonsDefensores[i] = posicaoPokemon;
    }
}

struct PokemonAtacante* gerarPokemonAtacante(int coluna, int linha) {
    int tipo = rand()%4;
    char nomePokemon[BUFSZ];
    switch(tipo) {
        case 0: strcpy(nomePokemon, "Mewtwo"); break;
        case 1: strcpy(nomePokemon, "Lugia"); break;
        case 2: strcpy(nomePokemon, "Zubat"); break;
        default: return NULL;
    }
    int hits = 0;
    int id = proximoIdAtacante;
    proximoIdAtacante++;
    struct PokemonAtacante* pokemon = (struct PokemonAtacante*) malloc(sizeof(struct PokemonAtacante));
    pokemon->id = id;
    strcpy(pokemon->nome, nomePokemon);
    pokemon->hits = hits;
    pokemon->coluna = coluna;
    pokemon->linha = linha;
    return pokemon;
}

void gerarResultadoGameOver(char resposta[BUFSZ], int sucesso) {
    clock_t fim = clock();
    double tempoGasto = (double)(fim-inicio)/CLOCKS_PER_SEC;
    tempoGasto = ceil(tempoGasto);
    sprintf(resposta, "gameover %d %d %d %d\n", sucesso, pokemonsDestruidos, chegaramNaPokedex, (int)tempoGasto);
}

int getHitsPokemon(char nome[BUFSZ]) {
    if(strcmp(nome, "Mewtwo") == 0) {
        return 3;
    }
    if(strcmp(nome, "Lugia") == 0) {
        return 2;
    }
    return 1;
}

int tratarMensagensRecebidas(char mensagem[BUFSZ], int socketServidor, struct sockaddr_storage dadosSocketCliente, int id) {
    printf("Recebeu mensagem %s", mensagem);
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
        return PROXIMA_COMUNICACAO;
    }

    if(strncmp("getturn ", mensagem, 8) == 0) {
        // Eh mensagem de turno
        char lixo[BUFSZ];
        int idTurno;
        int i, j;
        sscanf(mensagem, "%s %d", lixo, &idTurno);
        char resposta[BUFSZ];
        sprintf(resposta, "base %d\n", id+1);
        int avancouTurno = 0;
        if(turnoAtual > MAX_TURNOS) {
            gerarResultadoGameOver(resposta, 0);
            enviarMensagem(resposta, socketServidor, dadosSocketCliente);
            return PROXIMA_COMUNICACAO;
        }
        while(turnoAtual < idTurno) {
            avancarTurno();
            avancouTurno = 1;
            turnoAtual++;
        }
        if(avancouTurno) {
            for(i=0; i<4; i++) {
                if(i != id) {
                    int socketTemp = socket(AF_INET6, SOCK_DGRAM, 0);
                    // TODO: colocar suporte à perda e colocar um envio apenas de resposta
                    enviarMensagem(mensagem, socketTemp, dadosSockets[i]);
                    char respTemp[BUFSZ];
                    receberMensagem(socketTemp, dadosSockets[i], respTemp);
                    close(socketTemp);
                    enviarMensagem(respTemp, socketServidor, dadosSocketCliente);
                }
            }
        }
        struct PokemonAtacante* pokemonGerado = gerarPokemonAtacante(0, id);
        if(pokemonGerado != NULL) {
            adicionarElemento(*pokemonGerado);
        }
        struct PokemonAtacante* listaAtacantes = getLista();        
        // Gera os pokemons atacantes novos
        // Vai precisar de uma lista encadeada para remover pokemons atacantes facilmente
        for(i=0; i<numeroColunas; i++) {
            char temp[BUFSZ];
            sprintf(temp, "turn %d\nfixedLocation %d\n", idTurno, i+1);
            for(j=0; j<getTamanho(); j++) {
                if(listaAtacantes[j].linha == id && listaAtacantes[j].coluna == i) {
                    char temp2[BUFSZ];
                    sprintf(temp2, "%d %s %d\n", listaAtacantes[j].id, listaAtacantes[j].nome, listaAtacantes[j].hits);
                    strcat(temp, temp2);
                }
            }
            strcat(resposta, temp);
            strcat(resposta, "\n");
        }
        for(j=0; j<getTamanho(); j++) {
            if(listaAtacantes[j].coluna == numeroColunas) {
                chegaramNaPokedex++;
                removerElemento(listaAtacantes[j]);
            }
        }
        enviarMensagem(resposta, socketServidor, dadosSocketCliente);
        return PROXIMA_COMUNICACAO;
    }

    if(strncmp("shot ", mensagem, 5) == 0) {
        // Eh mensagem de defesa
        char lixo[BUFSZ];
        int posX;
        int posY;
        int id;
        int i;
        char resposta[BUFSZ];
        sscanf(mensagem, "%s %d %d %d", lixo, &posX, &posY, &id);
        posX--; posY--;
        struct PokemonAtacante* pokemonAtacado = buscarElementoId(id);
        if(pokemonAtacado == NULL) {
            printf("Nao achou atacado\n");
            sprintf(resposta, "%s %d %d %d %d\n", "shotresp", posX+1, posY+1, id, 1);
        } else {
            struct PosPokemonDefensor* posPokemonDefensor = NULL;
            for(i = 0; i<numeroDefensores; i++) {
                if(posPokemonsDefensores[i].posX == posX && posPokemonsDefensores[i].posY == posY) {
                    posPokemonDefensor = &posPokemonsDefensores[i];
                }
            }
            if(posPokemonDefensor == NULL) {
                printf("Nao achou defensor\n");
                sprintf(resposta, "%s %d %d %d %d\n", "shotresp", posX+1, posY+1, id, 1);
            } else {
                if(posPokemonDefensor->posX == pokemonAtacado->coluna && (posPokemonDefensor->posY == pokemonAtacado->linha || posPokemonDefensor->posY-1 == pokemonAtacado->linha)) {
                    pokemonAtacado->hits++;
                    if(pokemonAtacado->hits == getHitsPokemon(pokemonAtacado->nome)) {
                        pokemonsDestruidos++;
                        removerElemento(*pokemonAtacado);
                    } else {
                        atualizarElemento(*pokemonAtacado);
                    }
                    sprintf(resposta, "%s %d %d %d %d\n", "shotresp", posX+1, posY+1, id, 0);
                } else {
                    sprintf(resposta, "%s %d %d %d %d\n", "shotresp", posX+1, posY+1, id, 1);
                }
            }
        }
        enviarMensagem(resposta, socketServidor, dadosSocketCliente);
        return PROXIMA_COMUNICACAO;
    }

    if(strcmp(mensagem, "quit\n") == 0) {
        int i;
        for(i=0; i<4; i++) {
            if(i != id) {
                int socketTemp = socket(AF_INET6, SOCK_DGRAM, 0);
                enviarMensagem(mensagem, socketTemp, dadosSockets[i]);
                close(socketTemp);
            }
        }
        return ENCERRAR;
    }
    char resposta[BUFSZ];
    gerarResultadoGameOver(resposta, 1);
    enviarMensagem(resposta, socketServidor, dadosSocketCliente);
    // Identifica que o servidor deve receber o próximo recv do cliente
    return PROXIMO_CLIENTE;
}

int receberETratarMensagensCliente(int socketServidor, struct sockaddr_storage dadosSocket, int id) {
    if(!ambienteResetado) {
        ambienteResetado = 1;
        proximoIdAtacante = 1;
        pokemonsDestruidos = 0;
        chegaramNaPokedex = 0;
        inicio = 0;
        apagaLista();
    }
    char mensagem[BUFSZ];
    struct sockaddr_storage dadosSocketCliente = receberMensagem(socketServidor, dadosSocket, mensagem);

    if(strcmp(mensagem, "start\n") == 0) {
        char resposta[BUFSZ];
        strcpy(resposta, "game started: path ");
        if(inicio == 0) {
            inicio = clock();
        }
        char idThread[2];
        sprintf(idThread, "%d\n", id+1);
        strcat(resposta, idThread);
        enviarMensagem(resposta, socketServidor, dadosSocketCliente);
    } else {
        char resposta[BUFSZ];
        gerarResultadoGameOver(resposta, 1);
        enviarMensagem(resposta, socketServidor, dadosSocketCliente);
        return PROXIMO_CLIENTE;
    }
    while(1) {
        struct sockaddr_storage dadosSocketCliente = receberMensagem(socketServidor, dadosSocket, mensagem);
        int retorno = tratarMensagensRecebidas(mensagem, socketServidor, dadosSocketCliente, id);
        
        if(retorno == ENCERRAR || retorno == PROXIMO_CLIENTE) { // Se o servidor deve encerrar ou se conectar com outro cliente
            return retorno;
        }
    }
}

void* esperarPorConexoesCliente(void* param) {
    struct ParametroThreadServidor parametros = *(struct ParametroThreadServidor*) param;
    int socketServidor = parametros.socket;
    struct sockaddr_storage dadosSocket = parametros.dadosSocket;
    int id = parametros.id;
    printf("Servidor %d esperando conexão\n", id);
    // Enquanto o servidor estiver ativo ele deve receber conexões de clientes
    while (1) {
        int retorno = receberETratarMensagensCliente(socketServidor, dadosSocket, id);
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

    posPokemonsDefensores = (struct PosPokemonDefensor*) malloc(sizeof(struct PosPokemonDefensor)*numeroDefensores);

    struct ParametroThreadServidor parametros[4];
    pthread_t threads[4];
    int i;
    
    for(i=0; i<4; i++) {
        inicializarDadosSocket(argv[1], argv[2], &dadosSockets[i], argv[0], i);
        socketServidores[i] = inicializarSocketServidor(&dadosSockets[i]);
        bindServidor(socketServidores[i], &dadosSockets[i]);
        
        parametros[i].socket = socketServidores[i];
        parametros[i].dadosSocket = dadosSockets[i];
        parametros[i].id = i;
        pthread_create(&threads[i], NULL, esperarPorConexoesCliente, (void*) &parametros[i]);
    }

    for(i=0; i<4; i++) {
        pthread_join(threads[i], NULL);
    }

    for(i=0; i<4; i++) {
        close(socketServidores[i]);
    }

    free(posPokemonsDefensores);
    apagaLista();
    exit(EXIT_SUCCESS);
}