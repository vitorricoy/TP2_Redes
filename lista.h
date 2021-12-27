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

int getTamanho();

struct PokemonAtacante* getLista();

void deleteLista();

void avancarTurno(int numTurno);