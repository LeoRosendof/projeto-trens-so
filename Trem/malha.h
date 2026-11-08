#ifndef MALHA_H
#define MALHA_H

#include <vector>

/*
 * Descreve a malha da Figura 2 e calcula, para cada posição de cada trem,
 * quais mutexes e semáforos ele precisa ter para estar ali.
 * Não depende do Qt: o verificador (pasta verificacao/) usa as mesmas tabelas.
 */
namespace Malha {

const int NUM_TRENS = 6;
const int NUM_SEMAFOROS = 4; // um por entroncamento (S1 a S4)
const int NUM_REGIOES = 9;   // regiões críticas 1 a 9
const int NUM_RECURSOS = NUM_SEMAFOROS + NUM_REGIOES;
const int VAGAS_SEMAFORO = 2;

const int PASSO = 10;           // pixels andados a cada movimento
const int LARGURA_TREM = 21;    // tamanho do label_trem
const int ALTURA_TREM = 17;     // tamanho do label_trem
const int DISTANCIA_SEGURA = 30; // mais perto que isso, os trens disputam a região

struct Ponto {
  int x, y;
};

struct Percurso {
  std::vector<Ponto> pontos;      // volta completa, no sentido anti-horário
  std::vector<unsigned> recursos; // recursos necessários em cada ponto
};

/* Os semáforos ocupam os bits baixos: em cada passo, são pedidos antes dos
 * mutexes, sempre em ordem crescente de bit. */
inline unsigned bitSemaforo(int k) { return 1u << k; }
inline unsigned bitRegiao(int r) { return 1u << (NUM_SEMAFOROS + r - 1); }
const unsigned MASCARA_SEMAFOROS = (1u << NUM_SEMAFOROS) - 1;
const unsigned MASCARA_MUTEXES = ((1u << NUM_RECURSOS) - 1) & ~MASCARA_SEMAFOROS;

struct Config {
  bool semaforos;
  unsigned antecipadas[NUM_TRENS]; // vagas pegas já no início do trecho
};

const Config &configPadrao();
Percurso montarPercurso(int trem, const Config &config = configPadrao());

}

#endif
