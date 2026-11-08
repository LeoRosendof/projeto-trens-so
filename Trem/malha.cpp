#include "malha.h"

#include <algorithm>

namespace Malha {

/* Colunas e linhas da grade, em pixels (canto superior esquerdo do trem). */
static const int COLUNA[7] = {60, 180, 300, 420, 540, 660, 780};
static const int LINHA[4] = {30, 190, 350, 510};

/* Retângulo de cada trem: coluna esq., coluna dir., linha topo, linha base. */
static const int RETANGULO[NUM_TRENS][4] = {
    {1, 5, 0, 1}, // a
    {1, 3, 1, 2}, // b
    {3, 5, 1, 2}, // c
    {0, 2, 2, 3}, // d
    {2, 4, 2, 3}, // e
    {4, 6, 2, 3}, // f
};

/* Trecho de cada região crítica: (coluna, linha) até (coluna, linha). */
static const int REGIAO[NUM_REGIOES][4] = {
    {1, 1, 3, 1}, // 1
    {3, 1, 5, 1}, // 2
    {3, 1, 3, 2}, // 3
    {1, 2, 2, 2}, // 4
    {2, 2, 3, 2}, // 5
    {3, 2, 4, 2}, // 6
    {4, 2, 5, 2}, // 7
    {2, 2, 2, 3}, // 8
    {4, 2, 4, 3}, // 9
};

/* Regiões que cada trem atravessa, na ordem em que passa por elas. */
static const std::vector<int> ROTA[NUM_TRENS] = {
    {1, 2},       // a
    {4, 5, 3, 1}, // b
    {2, 3, 6, 7}, // c
    {8, 4},       // d
    {9, 6, 5, 8}, // e
    {7, 9},       // f
};

/*
 * Em cada entroncamento três trens formam um ciclo de espera. Exemplo em S1:
 * a está na 1 e quer a 2, c está na 2 e quer a 3, b está na 3 e quer a 1.
 * Cada linha é uma passagem (região de entrada, região de saída, semáforo).
 */
static const int PASSAGEM[12][3] = {
    {1, 2, 0}, {2, 3, 0}, {3, 1, 0}, // S1: regiões 1, 2 e 3
    {4, 5, 1}, {5, 8, 1}, {8, 4, 1}, // S2: regiões 4, 5 e 8
    {5, 3, 2}, {3, 6, 2}, {6, 5, 2}, // S3: regiões 3, 5 e 6
    {6, 7, 3}, {7, 9, 3}, {9, 6, 3}, // S4: regiões 6, 7 e 9
};

const Config &configPadrao() {
  /* b, c e e atravessam três entroncamentos seguidos. Cada um pega uma vaga
   * antes de ocupar a primeira região do trecho (ver verificador --busca). */
  static const Config config = {
      true, {0, bitSemaforo(0), bitSemaforo(3), 0, bitSemaforo(1), 0}};
  return config;
}

static int distanciaRegiao(Ponto p, int regiao) {
  const int *g = REGIAO[regiao - 1];
  int x1 = COLUNA[g[0]], y1 = LINHA[g[1]], x2 = COLUNA[g[2]], y2 = LINHA[g[3]];
  int dx = std::max({0, x1 - p.x, p.x - x2});
  int dy = std::max({0, y1 - p.y, p.y - y2});
  return std::max(dx, dy);
}

static std::vector<Ponto> voltaCompleta(int trem) {
  const int *r = RETANGULO[trem];
  int esq = COLUNA[r[0]], dir = COLUNA[r[1]];
  int topo = LINHA[r[2]], base = LINHA[r[3]];

  std::vector<Ponto> pontos;
  for (int x = dir; x > esq; x -= PASSO) // topo: para a esquerda
    pontos.push_back({x, topo});
  for (int y = topo; y < base; y += PASSO) // esquerda: para baixo
    pontos.push_back({esq, y});
  for (int x = esq; x < dir; x += PASSO) // base: para a direita
    pontos.push_back({x, base});
  for (int y = base; y > topo; y -= PASSO) // direita: para cima
    pontos.push_back({dir, y});
  return pontos;
}

static int semaforoDaPassagem(int entrada, int saida) {
  for (const int *p : {PASSAGEM[0], PASSAGEM[1], PASSAGEM[2], PASSAGEM[3],
                       PASSAGEM[4], PASSAGEM[5], PASSAGEM[6], PASSAGEM[7],
                       PASSAGEM[8], PASSAGEM[9], PASSAGEM[10], PASSAGEM[11]})
    if (p[0] == entrada && p[1] == saida)
      return p[2];
  return -1;
}

Percurso montarPercurso(int trem, const Config &config) {
  Percurso p;
  p.pontos = voltaCompleta(trem);
  const int n = p.pontos.size();
  auto ant = [n](int i) { return (i + n - 1) % n; };

  /* Perto de uma região da sua rota, o trem precisa do mutex dela. Num
   * entroncamento isso exige as duas regiões (entrada e saída) ao mesmo tempo.
   * Três regiões só formam pares que se sobrepõem, então nunca há dois trens
   * no mesmo entroncamento. */
  std::vector<unsigned> regioes(n, 0);
  for (int i = 0; i < n; i++)
    for (int r : ROTA[trem])
      if (distanciaRegiao(p.pontos[i], r) < DISTANCIA_SEGURA)
        regioes[i] |= bitRegiao(r);

  p.recursos = regioes;
  if (!config.semaforos)
    return p;

  /* A vaga do semáforo vale desde a entrada na região até sair do
   * entroncamento. Com vagas antecipadas, começa no início do trecho. */
  const std::vector<int> &rota = ROTA[trem];
  for (size_t a = 0; a + 1 < rota.size(); a++) {
    unsigned entrada = bitRegiao(rota[a]), saida = bitRegiao(rota[a + 1]);
    int k = semaforoDaPassagem(rota[a], rota[a + 1]);
    unsigned ambas = entrada | saida;

    for (int s = 0; s < n; s++) {
      if ((regioes[s] & ambas) != ambas || (regioes[ant(s)] & ambas) == ambas)
        continue;
      int inicio = s;
      if (config.antecipadas[trem] & bitSemaforo(k)) {
        while (regioes[ant(inicio)])
          inicio = ant(inicio);
      } else {
        while (regioes[ant(inicio)] & entrada)
          inicio = ant(inicio);
      }
      int fim = s;
      while ((regioes[(fim + 1) % n] & ambas) == ambas)
        fim = (fim + 1) % n;
      for (int i = inicio;; i = (i + 1) % n) {
        p.recursos[i] |= bitSemaforo(k);
        if (i == fim)
          break;
      }
    }
  }
  return p;
}

}
