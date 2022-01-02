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

#define PROXIMA_MENSAGEM 1
#define PROXIMO_JOGO 2
#define ENCERRAR 3

#define BUFSZ 512
#define TAMANHO_NOME 32
#define MAX_TURNOS 50
#define NUMERO_DEFENSORES 20
#define NUMERO_COLUNAS 4

// Parametros de uma thread do servidor
struct ParametroThreadServidor {
    int socket;
    struct sockaddr_storage dadosSocket;
    int id;
};

// Posicao do pokemon defensor
struct PosPokemonDefensor {
    int posX;
    int posY;
};

// Lista das posicoes dos pokemons defensores
struct PosPokemonDefensor posPokemonsDefensores[NUMERO_DEFENSORES];

// Proximo identificador de um pokemon atacante
int proximoIdAtacante = 1;
// Numero de pokemons atacantes que foram destruidos
int pokemonsDestruidos = 0;
// Numero de pokemons atacantes que chegaram na pokedex
int chegaramNaPokedex = 0;
// Flags para indicar se o pokemon defensor ja atirou nesse turno
int jaAtirou[NUMERO_DEFENSORES];
// Dados dos sockets de cada servidor
struct sockaddr_storage dadosSockets[4];
// Socket de cada servidor
int socketServidores[4];
// Flag para indicar se o ambiente foi resetado
int ambienteResetado = 1;
// Contabiliza o momento de inicio da execucao dos servidores
time_t inicio = 0;
// Identifica o turno atual
int turnoAtual = -1;
// Listas dos pokemons atacantes controlados por cada servidor
struct Lista pokemonsAtacantesServidor[4];
// Flag para indicar a propagacao de uma mensagem quit
int encerrando = 0;
// Mutex para impedir que mais de uma thread acesse as variaveis e se tenha condicoes de corrida
pthread_mutex_t liberacaoProcessamento;

void tratarParametroIncorreto(char* comandoPrograma) {
    // Imprime o uso correto dos parâmetros do programa e encerra o programa
    printf("Uso: %s v4|v6 porta do servidor \n", comandoPrograma);
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

void enviarEReceberMensagemServidor(int socketCliente, struct sockaddr_storage dadosServidor, char mensagem[BUFSZ]) {
    // Libera o mutex
    pthread_mutex_unlock(&liberacaoProcessamento);
    enviarEReceberMensagem(socketCliente, dadosServidor, mensagem, 0);
    // Bloqueia o mutex
    pthread_mutex_lock(&liberacaoProcessamento);
}

struct sockaddr_storage receberMensagem(int socketServidor, struct sockaddr_storage dadosSocket, char mensagem[BUFSZ]) {
    // Recebe uma mensagem de um cliente
    memset(mensagem, 0, BUFSZ);
    struct sockaddr_storage dadosSocketCliente;
    socklen_t len = sizeof(dadosSocketCliente);
    // Libera o mutex
    pthread_mutex_unlock(&liberacaoProcessamento);
    size_t tamanhoMensagem = recvfrom(socketServidor, (char *)mensagem, BUFSZ, MSG_WAITALL, ( struct sockaddr *) &dadosSocketCliente, &len);
    // Bloqueia o mutex
    pthread_mutex_lock(&liberacaoProcessamento);
    mensagem[tamanhoMensagem] = '\0';
    printf("Recebeu mensagem %s", mensagem);
    return dadosSocketCliente;
}

void gerarPokemonsDefensores() {
    // Gera NUMERO_DEFENSORES pokemons defensores, em posicoes distintas
    int valoresJaGeradosY[NUMERO_DEFENSORES];
    int valoresJaGeradosX[NUMERO_DEFENSORES];
    int i, j;
    for(i=0; i<NUMERO_DEFENSORES; i++) {
        int recalcular = 1;
        int v1, v2;
        while(recalcular) {
            recalcular = 0;
            v1 = rand()%NUMERO_COLUNAS;
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
    // Gera um pokemon atacante com 3/4 de chance, sendo uma chance uniforme para cada tipo de pokemon
    int tipo = rand()%4;
    char nomePokemon[TAMANHO_NOME];
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

void gerarEEnviarResultadoGameOver(char resposta[BUFSZ], int sucesso, int socketServidor, struct sockaddr_storage dadosSocketCliente) {
    // Indica um gameover, de acordo com os parametros recebidos
    clock_t fim = clock();
    double tempoGasto = (double)(fim-inicio)/CLOCKS_PER_SEC;
    tempoGasto = ceil(tempoGasto);
    sprintf(resposta, "gameover %d %d %d %d\n", sucesso, pokemonsDestruidos, chegaramNaPokedex, (int)tempoGasto);
    enviarMensagem(resposta, socketServidor, dadosSocketCliente);
}

void gerarEEnviarRespostaStart(int socketServidor, struct sockaddr_storage dadosSocketCliente, int id) {
    // Indica o inicio do jogo
    char resposta[BUFSZ];
    ambienteResetado = 0;
    strcpy(resposta, "game started: path ");
    if(inicio == 0) {
        inicio = clock();
    }
    char idThread[16];
    sprintf(idThread, "%d\n", id+1);
    strcat(resposta, idThread);
    enviarMensagem(resposta, socketServidor, dadosSocketCliente);
}

int gerarEEnviarRespostaGetDefenders(char mensagem[BUFSZ], int socketServidor, struct sockaddr_storage dadosSocketCliente, int id) {
    int i;
    // Gera os pokemons defensores
    gerarPokemonsDefensores();
    // Gera a mensagem de resposta com a lista de pokemons defensores no formato pedido
    char resposta[BUFSZ];
    strcpy(resposta, "defender [");
    for(i=0; i<NUMERO_DEFENSORES; i++) {
        char pos[10];
        sprintf(pos, "%d", posPokemonsDefensores[i].posX);
        strcat(resposta, "[");
        strcat(resposta, pos);
        strcat(resposta, ", ");
        sprintf(pos, "%d", posPokemonsDefensores[i].posY);
        strcat(resposta, pos);
        if(i != NUMERO_DEFENSORES-1) {
            strcat(resposta, "], ");
        } else {
            strcat(resposta, "]");
        }
    }
    strcat(resposta, "]\n");
    enviarMensagem(resposta, socketServidor, dadosSocketCliente);
    return PROXIMA_MENSAGEM;
}

int gerarEEnviarRespostaGetTurn(char mensagem[BUFSZ], int socketServidor, struct sockaddr_storage dadosSocketCliente, int id) {
    char lixo[BUFSZ];
    int idTurno;
    int i, j;
    // Le o id do turno sendo iniciado
    sscanf(mensagem, "%s %d", lixo, &idTurno);
    // Gera a mensagem de resposta no formato pedido
    char resposta[BUFSZ];
    sprintf(resposta, "base %d\n", id+1);
    if(turnoAtual > MAX_TURNOS) {
        // Caso o turno pedido seja um turno depois do fim do jogo, gera uma mensagem de gameover
        gerarEEnviarResultadoGameOver(resposta, 0, socketServidor, dadosSocketCliente);
        return PROXIMA_MENSAGEM;
    }
    // Avanca o turno do servidor para o turno pedido pelo cliente
    while(turnoAtual < idTurno) {
        for(i=0; i<4; i++) {
            // Avanca os pokemons atacantes
            avancarTurno(&pokemonsAtacantesServidor[i]);
        }
        // Reinicia as flags de tiro dos defensores
        memset(jaAtirou, 0, sizeof(jaAtirou));
        turnoAtual++;
    }
    // Gera um pokemon atacante
    struct PokemonAtacante* pokemonGerado = gerarPokemonAtacante(0, id);
    if(pokemonGerado != NULL) {
        adicionarElemento(&pokemonsAtacantesServidor[id], *pokemonGerado);
    }
    // Busca a lista como um vetor
    struct PokemonAtacante* listaAtacantes = getLista(&pokemonsAtacantesServidor[id]);
    // Percorre as colunas
    for(i=0; i<NUMERO_COLUNAS; i++) {
        char temp[BUFSZ];
        sprintf(temp, "turn %d\nfixedLocation %d\n", idTurno, i+1);
        // Percorre os pokemons atacantes controlados por esse servidor
        for(j=0; j<pokemonsAtacantesServidor[id].tamanho; j++) {
            // Se o pokemon atacante for da coluna buscada
            if(listaAtacantes[j].linha == id && listaAtacantes[j].coluna == i) {
                // Adiciona o pokemon na mensagem de resposta
                char temp2[BUFSZ];
                sprintf(temp2, "%d %s %d\n", listaAtacantes[j].id, listaAtacantes[j].nome, listaAtacantes[j].hits);
                strcat(temp, temp2);
            }
        }
        strcat(resposta, temp);
        strcat(resposta, "\n");
    }
    // Verifica quais pokemons chegaram na pokedex
    for(j=0; j<pokemonsAtacantesServidor[id].tamanho; j++) {
        if(listaAtacantes[j].coluna == NUMERO_COLUNAS) {
            // Atualiza o contador
            chegaramNaPokedex++;
            // Remove o pokemon
            removerElemento(&pokemonsAtacantesServidor[id], listaAtacantes[j]);
        }
    }
    enviarMensagem(resposta, socketServidor, dadosSocketCliente);
    return PROXIMA_MENSAGEM;
}

int gerarEEnviarRespostaShot(char mensagem[BUFSZ], int socketServidor, struct sockaddr_storage dadosSocketCliente, int id) {
    char lixo[BUFSZ], resposta[BUFSZ];
    int posX, posY, idAtacante, i;
    // Le a posicao do defensor e o id do atacante
    sscanf(mensagem, "%s %d %d %d", lixo, &posX, &posY, &idAtacante);
    posX--; posY--;
    // Busca o pokemon atacante (que sera atacado)
    struct PokemonAtacante* pokemonAtacado = buscarElementoId(&pokemonsAtacantesServidor[id], idAtacante);
    if(pokemonAtacado == NULL) {
        // Se nao achou o pokemon atacado envia o shotresp indicando erro
        sprintf(resposta, "%s %d %d %d %d\n", "shotresp", posX+1, posY+1, idAtacante, 1);
    } else {
        // Busca o pokemon defensor na lista de pokemons defensores
        struct PosPokemonDefensor* posPokemonDefensor = NULL;
        int indDefensor = -1;
        for(i = 0; i<NUMERO_DEFENSORES; i++) {
            if(posPokemonsDefensores[i].posX == posX && posPokemonsDefensores[i].posY == posY) {
                posPokemonDefensor = &posPokemonsDefensores[i];
                indDefensor = i;
            }
        }
        if(posPokemonDefensor == NULL) {
            // Se nao achou o pokemon defensor envia o shotresp indicando erro
            sprintf(resposta, "%s %d %d %d %d\n", "shotresp", posX+1, posY+1, idAtacante, 1);
        } else {
            // Verifica se o defensor consegue acertar o atacado e se ele ja nao atirou nesse turno
            if(!jaAtirou[indDefensor] && posPokemonDefensor->posX == pokemonAtacado->coluna && (posPokemonDefensor->posY == pokemonAtacado->linha || posPokemonDefensor->posY-1 == pokemonAtacado->linha)) {
                // Indica que agora o defensor ja atirou nesse turno
                jaAtirou[indDefensor] = 1; 
                // Indica que o pokemon atacado recebeu um ataque
                pokemonAtacado->hits++;
                // Se o pokemon foi destruido
                if(pokemonAtacado->hits == getHitsPokemon(pokemonAtacado->nome)) {
                    // Atualiza o contador
                    pokemonsDestruidos++;
                    // Remove o pokemon da lista
                    removerElemento(&pokemonsAtacantesServidor[id], *pokemonAtacado);
                } else {
                    // Atualiza o pokemon na lista para indicar sua perda de vida
                    atualizarElemento(&pokemonsAtacantesServidor[id], *pokemonAtacado);
                }
                // Envia a mensagem shotresp indicando sucesso
                sprintf(resposta, "%s %d %d %d %d\n", "shotresp", posX+1, posY+1, idAtacante, 0);
            } else {
                // Envia a mensagem shotresp indicando erro
                sprintf(resposta, "%s %d %d %d %d\n", "shotresp", posX+1, posY+1, idAtacante, 1);
            }
        }
    }
    enviarMensagem(resposta, socketServidor, dadosSocketCliente);
    return PROXIMA_MENSAGEM;
}

int gerarEEnviarRespostaQuit(char mensagem[BUFSZ], int socketServidor, struct sockaddr_storage dadosSocketCliente, int id) {
    int i;
    char mensagemOriginal[BUFSZ];
    strcpy(mensagemOriginal, mensagem);
    // Se esse e o primeiro servidor a receber o quit
    if(encerrando == 0) {
        // Indica que uma mensagem quit esta sendo propagada
        encerrando = 1;
        // Percorre os identificadores dos servidores
        for(i=0; i<4; i++) {
            // Para os servidores que nao sao o servidor que recebeu a mensagem
            if(i != id) {
                // Envia uma mensagem ao outro servidor indicando o comando quit
                int socketTemp = socket(AF_INET6, SOCK_DGRAM, 0);
                // Configura um timeout de um segundo para o socket
                struct timeval timeout = {.tv_sec = 1, .tv_usec = 0};
                if (setsockopt(socketTemp, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
                    perror("Erro ao definir as configuracoes do socket");
                }
                // Envia a mensagem a outros servidores
                enviarEReceberMensagemServidor(socketTemp, dadosSockets[i], mensagem);
                close(socketTemp);
                strcpy(mensagem, mensagemOriginal);
            }
        }
    }
    char resposta[BUFSZ];
    // Envia o resultado de gameover com flag 0
    gerarEEnviarResultadoGameOver(resposta, 0, socketServidor, dadosSocketCliente);
    return ENCERRAR;
}

int tratarPassoDoJogo(char mensagem[BUFSZ], int socketServidor, struct sockaddr_storage dadosSocketCliente, int id) {
    // Verifica se a mensagem e de getdefenders
    if(strcmp(mensagem, "getdefenders\n") == 0) {
        return gerarEEnviarRespostaGetDefenders(mensagem, socketServidor, dadosSocketCliente, id);
    }

    // Verifica se a mensagem e de getturn
    if(strncmp("getturn ", mensagem, 8) == 0) {
        return gerarEEnviarRespostaGetTurn(mensagem, socketServidor, dadosSocketCliente, id);
    }

    // Verifica se a mensagem e de shot
    if(strncmp("shot ", mensagem, 5) == 0) {
        return gerarEEnviarRespostaShot(mensagem, socketServidor, dadosSocketCliente, id);
    }

    // Verifica se a mensagem e de quit
    if(strcmp(mensagem, "quit\n") == 0) {
        return gerarEEnviarRespostaQuit(mensagem, socketServidor, dadosSocketCliente, id);
    }
    // Mensagem recebida e invalida, portanto gera um gameover com erro
    char resposta[BUFSZ];
    gerarEEnviarResultadoGameOver(resposta, 1, socketServidor, dadosSocketCliente);
    // Identifica que o servidor deve receber o próximo recv do cliente
    return PROXIMO_JOGO;
}

int executarJogo(int socketServidor, struct sockaddr_storage dadosSocket, int id) {
    int i;
    // Verifica se o ambiente ainda nao foi resetado
    if(!ambienteResetado) {
        // Reseta as variaveis de ambiente
        ambienteResetado = 1;
        proximoIdAtacante = 1;
        pokemonsDestruidos = 0;
        chegaramNaPokedex = 0;
        inicio = 0;
        encerrando = 0;
        for(i=0; i<4; i++) {
            limpaLista(&pokemonsAtacantesServidor[id]);
        }
    }
    // Recebe uma mensagem do cliente
    char mensagem[BUFSZ];
    struct sockaddr_storage dadosSocketCliente = receberMensagem(socketServidor, dadosSocket, mensagem);

    // Verifica se a mensagem e de start
    if(strcmp(mensagem, "start\n") == 0) {
        gerarEEnviarRespostaStart(socketServidor, dadosSocketCliente, id);
    } else {
        printf("Esperava start mas recebeu outro comando\n");
        char resposta[BUFSZ];
        gerarEEnviarResultadoGameOver(resposta, 1, socketServidor, dadosSocketCliente);
        return PROXIMO_JOGO;
    }
    // Loop para receber e tratar as mensagens dos clientes
    while(1) {
        struct sockaddr_storage dadosSocketCliente = receberMensagem(socketServidor, dadosSocket, mensagem);
        int retorno = tratarPassoDoJogo(mensagem, socketServidor, dadosSocketCliente, id);
        
        // Se o servidor deve encerrar ou reiniciar o jogo
        if(retorno == ENCERRAR || retorno == PROXIMO_JOGO) {
            return retorno;
        }
    }
}

void* esperarPorConexoesCliente(void* param) {
    // Le os parametros da thread
    struct ParametroThreadServidor parametros = *(struct ParametroThreadServidor*) param;
    int socketServidor = parametros.socket;
    struct sockaddr_storage dadosSocket = parametros.dadosSocket;
    int id = parametros.id;
    // Bloqueia o mutex
    pthread_mutex_lock(&liberacaoProcessamento);
    // Enquanto o servidor estiver ativo ele deve receber conexões de novos jogos
    printf("Servidor %d esperando conexão\n", id);
    while (1) {
        // Recebe e trata mensagens do cliente, iniciando um jogo
        int retorno = executarJogo(socketServidor, dadosSocket, id);
        // Caso o servidor deve ser encerrado finaliza o laço de conexões de clientes
        if(retorno == ENCERRAR) { 
            break;
        }
    }
    // Libera o mutex
    pthread_mutex_unlock(&liberacaoProcessamento);
    pthread_exit(NULL);
}

int main(int argc, char** argv) {
    // Verifica os parametros recebidos
    verificarParametros(argc, argv);
    // Inicializa a seed para gerar valores aleatorios
    srand(time(NULL));
    // Inicializa o mutex
    pthread_mutex_init(&liberacaoProcessamento, NULL);
    // Declara os dados usados para as threads
    struct ParametroThreadServidor parametros[4];
    pthread_t threads[4];
    int i;
    // Para cada thread de servidor
    for(i=0; i<4; i++) {
        // Inicializa os dados do socket do servidor
        inicializarDadosSocket(argv[1], argv[2], &dadosSockets[i], argv[0], i);
        // Inicializa o socket em si do servidor
        socketServidores[i] = inicializarSocketServidor(&dadosSockets[i]);
        // Da bind do servidor na porta e endereco
        bindServidor(socketServidores[i], &dadosSockets[i]);
        // Preenche os parametros da thread
        parametros[i].socket = socketServidores[i];
        parametros[i].dadosSocket = dadosSockets[i];
        parametros[i].id = i;
        // Inicializa a lista do servidor
        inicializarLista(&pokemonsAtacantesServidor[i]);
        // Cria a thread do servidor
        pthread_create(&threads[i], NULL, esperarPorConexoesCliente, (void*) &parametros[i]);
    }

    for(i=0; i<4; i++) {
        // Espera pela finalizacao de todas as threads dos servidores
        pthread_join(threads[i], NULL);
    }

    for(i=0; i<4; i++) {
        // Encerra o socket dos servidores e limpa a lista de pokemons atacantes deles
        close(socketServidores[i]);
        limpaLista(&pokemonsAtacantesServidor[i]);
    }

    // Destroi o mutex
    pthread_mutex_destroy(&liberacaoProcessamento);

    exit(EXIT_SUCCESS);
}