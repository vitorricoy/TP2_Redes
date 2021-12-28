#define BUFSZ 512

struct PokemonAtacante {
    int id;
    char nome[BUFSZ];
    int hits;
    int coluna;
    int linha;
};

void adicionarElemento(struct PokemonAtacante elemento);

void removerElemento(struct PokemonAtacante removido);

void atualizarElemento(struct PokemonAtacante novoValor);

struct PokemonAtacante* buscarElementoId(int id);

struct PokemonAtacante* buscarElementoPos(int pos);

int getTamanho();

struct PokemonAtacante* getLista();

void apagaLista();

void avancarTurno();