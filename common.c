#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <arpa/inet.h>

#define TAMANHO_NOME 32
#define BUFSZ 1024
#define MAX_TURNOS 50
#define NUMERO_COLUNAS 4

// Posicao do pokemon defensor
struct PosPokemonDefensor {
    int posX;
    int posY;
};

void sairComMensagem(char *msg) {
	perror(msg);
	exit(EXIT_FAILURE);
}

int getHitsPokemon(char nome[TAMANHO_NOME]) {
    if(strcmp(nome, "Mewtwo") == 0) {
        return 3;
    }
    if(strcmp(nome, "Lugia") == 0) {
        return 2;
    }
    return 1;
}

void enviarEReceberMensagem(int socketCliente, struct sockaddr_storage dadosServidor, char mensagem[BUFSZ], int debug) {
    // Envia o parâmetro 'mensagem' para o servidor
    size_t tamanhoMensagemEnviada = sendto(socketCliente, (const char *)mensagem, strlen(mensagem), 0, (const struct sockaddr *) &dadosServidor, sizeof(dadosServidor));
    if(strlen(mensagem) != tamanhoMensagemEnviada) {
        sairComMensagem("Erro ao enviar mensagem");
    }
    // Imprime a mensagem enviada
    if(debug) printf("> %s", mensagem);
    // Espera a resposta do servidor
    char mensagemResposta[BUFSZ];
    memset(mensagemResposta, 0, BUFSZ);
    socklen_t len = sizeof(dadosServidor);
    size_t tamanhoMensagem = recvfrom(socketCliente, (char *)mensagemResposta, BUFSZ, MSG_WAITALL, (struct sockaddr *) &dadosServidor, &len);
    // Se a resposta nao foi recebida
    while(tamanhoMensagem == -1) {
        // Verifica se foi timeout ou outro erro
        if(errno != EAGAIN && errno != EWOULDBLOCK) {
            perror("Erro ao receber mensagem de resposta");
        }
        // Indica que ocorreu um timeout
        if(debug) printf("Timeout no recvfrom\n");
        // Envia a mensagem para o servidor novamente
        size_t tamanhoMensagemEnviada = sendto(socketCliente, (const char *)mensagem, strlen(mensagem), 0, (const struct sockaddr *) &dadosServidor, sizeof(dadosServidor));
        if (strlen(mensagem) != tamanhoMensagemEnviada) {
            sairComMensagem("Erro ao enviar mensagem");
        }
        if(debug) printf("> %s", mensagem);
        // Espera a resposta do servidor novamente
        memset(mensagemResposta, 0, BUFSZ);
        tamanhoMensagem = recvfrom(socketCliente, (char *)mensagemResposta, BUFSZ, MSG_WAITALL, (struct sockaddr *) &dadosServidor, &len);
    } 
    mensagemResposta[tamanhoMensagem] = '\0';
    // Imprime a resposta recebida
    if(debug) printf("< %s", mensagemResposta);
    memcpy(mensagem, mensagemResposta, BUFSZ);
}