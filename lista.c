#include <stdlib.h>

#define BUFSZ 512

struct PokemonAtacante {
    int id;
    char nome[BUFSZ];
    int hits;
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
    } else {
        sentinela.ant->prox = novoNo;
        novoNo->ant = sentinela.ant;
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

int getTamanho() {
    return tamanho;
}

struct PokemonAtacante* getLista() {
    struct PokemonAtacante* ret = (struct PokemonAtacante*) malloc(sizeof(struct PokemonAtacante)*tamanho);
    struct No* noAtual = sentinela.prox;
    for(int i = 0; i<tamanho; i++) {
        ret[i] = noAtual->valor;
        noAtual = noAtual->prox;
    }
    return ret;
}

void deleteLista() {
    struct No* noAtual = sentinela.prox;
    struct No* proximo = noAtual->prox;
    for(int i = 0; i<tamanho; i++) {
        free(noAtual);
        noAtual = proximo;
        proximo = noAtual->prox;
    }
}