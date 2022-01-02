#include <stdlib.h>

#define TAMANHO_NOME 32

struct PokemonAtacante {
    int id;
    char nome[TAMANHO_NOME];
    int hits;
    int coluna;
    int linha;
};

struct No {
    struct PokemonAtacante valor;
    struct No* prox;
    struct No* ant;
};

struct Lista {
    struct No sentinela;
    int tamanho;
};

void inicializarLista(struct Lista* lista) {
    lista->tamanho = 0;
    lista->sentinela.prox = NULL;
    lista->sentinela.ant = NULL;
}

void adicionarElemento(struct Lista* lista, struct PokemonAtacante elemento) {
    struct No* novoNo = (struct No*) malloc(sizeof(struct No));
    novoNo->valor = elemento;
    novoNo->prox = NULL;
    novoNo->ant = NULL;
    if(lista->sentinela.ant == NULL) {
        lista->sentinela.prox = novoNo;
        lista->sentinela.ant = novoNo;
        novoNo->ant = &lista->sentinela;
        novoNo->prox = &lista->sentinela;
    } else {
        lista->sentinela.ant->prox = novoNo;
        novoNo->ant = lista->sentinela.ant;
        novoNo->prox = &lista->sentinela;
        lista->sentinela.ant = novoNo;
    }
    lista->tamanho++;
}

void removerElemento(struct Lista* lista, struct PokemonAtacante removido) {
    struct No* proximo = lista->sentinela.prox;
    while(proximo != NULL) {
        if(proximo->valor.id == removido.id) {
            // Remove e sai
            proximo->ant->prox = proximo->prox;
            proximo->prox->ant = proximo->ant;
            free(proximo);
            lista->tamanho--;
            return;
        }
        proximo = proximo->prox;
    }
}

struct PokemonAtacante* buscarElementoId(struct Lista* lista, int id) {
    struct No* proximo = lista->sentinela.prox;
    while(proximo != NULL) {
        if(proximo->valor.id == id) {
            return &proximo->valor;
        }
        proximo = proximo->prox;
    }
    return NULL;
}

struct PokemonAtacante* buscarElementoPos(struct Lista* lista, int pos) {
    struct No* proximo = lista->sentinela.prox;
    int i;
    for(i=0; i<pos-1; i++) {
        if(proximo == NULL) {
            return NULL;
        }
        proximo = proximo->prox;
    }
    return &proximo->valor;
}

void atualizarElemento(struct Lista* lista, struct PokemonAtacante novoValor) {
    struct No* proximo = lista->sentinela.prox;
    while(proximo != NULL) {
        if(proximo->valor.id == novoValor.id) {
            // Atualiza e sai
            proximo->valor = novoValor;
            return;
        }
        proximo = proximo->prox;
    }
}

void avancarTurno(struct Lista* lista) {
    int i;
    struct No* noAtual = lista->sentinela.prox;
    for(i = 0; i<lista->tamanho; i++) {
        noAtual->valor.coluna++;
        noAtual = noAtual->prox;
    }
}

struct PokemonAtacante* getLista(struct Lista* lista) {
    int i;
    struct PokemonAtacante* ret = (struct PokemonAtacante*) malloc(sizeof(struct PokemonAtacante)*lista->tamanho);
    struct No* noAtual = lista->sentinela.prox;
    for(i = 0; i<lista->tamanho; i++) {
        ret[i] = noAtual->valor;
        noAtual = noAtual->prox;
    }
    return ret;
}

void limpaLista(struct Lista* lista) {
    int i;
    struct No* noAtual = lista->sentinela.prox;
    if(noAtual == NULL) {
        return;
    }
    struct No* proximo = noAtual->prox;
    for(i = 0; i<lista->tamanho; i++) {
        free(noAtual);
        noAtual = proximo;
        if(noAtual == NULL) {
            break;
        }
        proximo = noAtual->prox;
    }
    lista->sentinela.prox = NULL;
    lista->sentinela.ant = NULL;
    lista->tamanho = 0;
}