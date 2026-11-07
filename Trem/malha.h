#ifndef MALHA_H
#define MALHA_H

#include <vector>

namespace Malha {

const int NUM_TRENS = 6;
const int NUM_REGIOES = 9;
const int NUM_CRUZAMENTOS = 4;
const int NUM_SEMAFOROS = NUM_CRUZAMENTOS;
const int VAGAS_SEMAFORO = 2;
const int NUM_RECURSOS = NUM_SEMAFOROS + NUM_REGIOES + NUM_CRUZAMENTOS;

const int ORIGEM_X = 40;
const int ORIGEM_Y = 40;
const int ESPACO_X = 90;
const int ESPACO_Y = 120;

const int PASSO = 5;
const int TAM_TREM = 20;

const int DIST_COLISAO = 25;

struct Ponto {
  int x, y;
};

struct Segmento {
  Ponto a, b;
};

struct Retangulo {
  int esq, dir, topo, base;
};

struct Reserva {
  int trem;
  int regiao;
  int ate;
};

struct VagaAntecipada {
  int trem;
  int cruzamento;
};

struct Config {
  bool semaforos;
  std::vector<VagaAntecipada> antecipadas;
  std::vector<Reserva> reservas;
};

struct Percurso {
  std::vector<Ponto> pontos;
  std::vector<unsigned> recursos;
  int inicio;
};

inline unsigned bitSemaforo(int k) { return 1u << k; }
inline unsigned bitRegiao(int r) { return 1u << (NUM_SEMAFOROS + r - 1); }
inline unsigned bitCruzamento(int k) {
  return 1u << (NUM_SEMAFOROS + NUM_REGIOES + k);
}
const unsigned MASCARA_SEMAFOROS = (1u << NUM_SEMAFOROS) - 1;
const unsigned MASCARA_MUTEXES =
    ((1u << NUM_RECURSOS) - 1) & ~MASCARA_SEMAFOROS;

int colunaX(int coluna);
int linhaY(int linha);

char letra(int trem);
Retangulo retangulo(int trem);
Segmento segmentoRegiao(int regiao);
Ponto pontoCruzamento(int cruzamento);

bool colidem(Ponto p, Ponto q);

const Config &configPadrao();

Percurso montarPercurso(int trem, const Config &config = configPadrao());

}

#endif
