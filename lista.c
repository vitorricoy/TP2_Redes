#include <stdlib.h>

#define BUFSZ 512

struct PokemonAtacante {
    int id;
    char nome[BUFSZ];
    int hits;
    int coluna;
    int linha;
};

struct No {
    struct PokemonAtacante valor;
    struct No* prox;
    struct No* ant;
};

struct No sentinela;
int tamanho = 0;

void adicionarElemento(struct PokemonAtacante elemento) {
    struct No* novoNo = (struct No*) malloc(sizeof(struct No));
    novoNo->valor = elemento;
    novoNo->prox = NULL;
    novoNo->ant = NULL;
    if(sentinela.ant == NULL) {
        sentinela.prox = novoNo;
        sentinela.ant = novoNo;
        novoNo->ant = &sentinela;
        novoNo->prox = &sentinela;
    } else {
        sentinela.ant->prox = novoNo;
        novoNo->ant = sentinela.ant;
        novoNo->prox = &sentinela;
        sentinela.ant = novoNo;
    }
    tamanho++;
}

void removerElemento(struct PokemonAtacante removido) {
    struct No* proximo = sentinela.prox;
    while(proximo != NULL) {
        if(proximo->valor.id == removido.id) {
            // Remove e sai
            proximo->ant->prox = proximo->prox;
            proximo->prox->ant = proximo->ant;
            free(proximo);
            tamanho--;
            return;
        }
        proximo = proximo->prox;
    }
}

struct PokemonAtacante* buscarElementoId(int id) {
    struct No* proximo = sentinela.prox;
    while(proximo != NULL) {
        if(proximo->valor.id == id) {
            return &proximo->valor;
        }
        proximo = proximo->prox;
    }
    return NULL;
}

struct PokemonAtacante* buscarElementoPos(int pos) {
    struct No* proximo = sentinela.prox;
    int i;
    for(i=0; i<pos-1; i++) {
        if(proximo == NULL) {
            return NULL;
        }
        proximo = proximo->prox;
    }
    return &proximo->valor;
}

void atualizarElemento(struct PokemonAtacante novoValor) {
    struct No* proximo = sentinela.prox;
    while(proximo != NULL) {
        if(proximo->valor.id == novoValor.id) {
            // Atualiza e sai
            proximo->valor = novoValor;
            return;
        }
        proximo = proximo->prox;
    }
}

int getTamanho() {
    return tamanho;
}

void avancarTurno() {
    int i;
    struct No* noAtual = sentinela.prox;
    for(i = 0; i<tamanho; i++) {
        noAtual->valor.coluna++;
        noAtual = noAtual->prox;
    }
}

struct PokemonAtacante* getLista() {
    int i;
    struct PokemonAtacante* ret = (struct PokemonAtacante*) malloc(sizeof(struct PokemonAtacante)*tamanho);
    struct No* noAtual = sentinela.prox;
    for(i = 0; i<tamanho; i++) {
        ret[i] = noAtual->valor;
        noAtual = noAtual->prox;
    }
    return ret;
}

void apagaLista() {
    int i;
    struct No* noAtual = sentinela.prox;
    if(noAtual == NULL) {
        return;
    }
    struct No* proximo = noAtual->prox;
    for(i = 0; i<tamanho; i++) {
        free(noAtual);
        noAtual = proximo;
        if(noAtual == NULL) {
            break;
        }
        proximo = noAtual->prox;
    }
    sentinela.prox = NULL;
    sentinela.ant = NULL;
    tamanho = 0;
}