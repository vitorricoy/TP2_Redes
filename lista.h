#pragma once

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

void inicializarLista(struct Lista* lista);

void adicionarElemento(struct Lista* lista, struct PokemonAtacante elemento);

void removerElemento(struct Lista* lista, struct PokemonAtacante removido);

struct PokemonAtacante* buscarElementoId(struct Lista* lista, int id);

struct PokemonAtacante* buscarElementoPos(struct Lista* lista, int pos);

void atualizarElemento(struct Lista* lista, struct PokemonAtacante novoValor);

struct PokemonAtacante* getLista(struct Lista* lista);

void limpaLista(struct Lista* lista);