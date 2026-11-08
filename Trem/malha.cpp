#include "malha.h"

#include <algorithm>
#include <cstdlib>
#include <stdexcept>

namespace Malha {

int colunaX(int coluna) { return ORIGEM_X + coluna * ESPACO_X; }

int linhaY(int linha) { return ORIGEM_Y + linha * ESPACO_Y; }

char letra(int trem) { return 'a' + trem; }

Retangulo retangulo(int trem) {

  static const int grade[NUM_TRENS][4] = {
      {1, 5, 0, 1}, {1, 3, 1, 2}, {3, 5, 1, 2},
      {0, 2, 2, 3}, {2, 4, 2, 3}, {4, 6, 2, 3},
  };
  const int *g = grade[trem];
  return {colunaX(g[0]), colunaX(g[1]), linhaY(g[2]), linhaY(g[3])};
}

Segmento segmentoRegiao(int regiao) {

  static const int grade[NUM_REGIOES][4] = {
      {1, 1, 3, 1}, {3, 1, 5, 1}, {3, 1, 3, 2}, {1, 2, 2, 2}, {2, 2, 3, 2},
      {3, 2, 4, 2}, {4, 2, 5, 2}, {2, 2, 2, 3}, {4, 2, 4, 3},
  };
  const int *g = grade[regiao - 1];
  return {{colunaX(g[0]), linhaY(g[1])}, {colunaX(g[2]), linhaY(g[3])}};
}

Ponto pontoCruzamento(int cruzamento) {
  static const int grade[NUM_CRUZAMENTOS][2] = {
      {3, 1},
      {2, 2},
      {3, 2},
      {4, 2},
  };
  return {colunaX(grade[cruzamento][0]), linhaY(grade[cruzamento][1])};
}

static unsigned regioesDoCruzamento(int cruzamento) {
  static const int regioes[NUM_CRUZAMENTOS][3] = {
      {1, 2, 3},
      {4, 5, 8},
      {3, 5, 6},
      {6, 7, 9},
  };
  const int *r = regioes[cruzamento];
  return bitRegiao(r[0]) | bitRegiao(r[1]) | bitRegiao(r[2]);
}

static int distancia(Ponto p, Segmento s) {
  int dx =
      std::max({0, std::min(s.a.x, s.b.x) - p.x, p.x - std::max(s.a.x, s.b.x)});
  int dy =
      std::max({0, std::min(s.a.y, s.b.y) - p.y, p.y - std::max(s.a.y, s.b.y)});
  return std::max(dx, dy);
}

bool colidem(Ponto p, Ponto q) {
  return std::abs(p.x - q.x) < DIST_COLISAO &&
         std::abs(p.y - q.y) < DIST_COLISAO;
}

static bool ehCruzamento(Ponto p) {
  for (int k = 0; k < NUM_CRUZAMENTOS; k++) {
    Ponto c = pontoCruzamento(k);
    if (c.x == p.x && c.y == p.y)
      return true;
  }
  return false;
}

static Ponto aproximar(Ponto p, Ponto q, int d) {
  if (p.x != q.x)
    p.x += (q.x > p.x) ? d : -d;
  if (p.y != q.y)
    p.y += (q.y > p.y) ? d : -d;
  return p;
}

static Segmento trechoProprio(int regiao) {
  Segmento s = segmentoRegiao(regiao);
  Segmento t = s;
  if (ehCruzamento(s.a))
    t.a = aproximar(s.a, s.b, DIST_COLISAO);
  if (ehCruzamento(s.b))
    t.b = aproximar(s.b, s.a, DIST_COLISAO);
  return t;
}

const Config &configPadrao() {
  static const Config config = {
      true,
      {{1, 0}, {2, 3}, {4, 1}},
      {},
  };
  return config;
}

template <class Inicio, class Chegada>
static bool reservarAte(const std::vector<unsigned> &base,
                        std::vector<unsigned> &posse, Inicio deveIniciar,
                        Chegada chegou) {
  const int n = base.size();
  bool mudou = false;
  for (int s = 0; s < n; s++) {
    if (!deveIniciar(s))
      continue;
    int q = s;
    while (!chegou(q)) {
      q = (q + 1) % n;
      if (base[q] == 0 || q == s)
        throw std::logic_error("reserva sem ponto de chegada");
    }

    unsigned acumulado = 0;
    for (int i = q;; i = (i + n - 1) % n) {
      acumulado |= posse[i];
      if (posse[i] != (posse[i] | acumulado)) {
        posse[i] |= acumulado;
        mudou = true;
      }
      if (i == s)
        break;
    }
  }
  return mudou;
}

Percurso montarPercurso(int trem, const Config &config) {
  Percurso p;
  Retangulo r = retangulo(trem);

  for (int x = r.dir; x > r.esq; x -= PASSO)
    p.pontos.push_back({x, r.topo});
  for (int y = r.topo; y < r.base; y += PASSO)
    p.pontos.push_back({r.esq, y});
  for (int x = r.esq; x < r.dir; x += PASSO)
    p.pontos.push_back({x, r.base});
  for (int y = r.base; y > r.topo; y -= PASSO)
    p.pontos.push_back({r.dir, y});

  const int n = p.pontos.size();
  auto ant = [n](int i) { return (i + n - 1) % n; };

  std::vector<unsigned> base(n, 0);
  for (int i = 0; i < n; i++) {
    for (int reg = 1; reg <= NUM_REGIOES; reg++)
      if (distancia(p.pontos[i], trechoProprio(reg)) < DIST_COLISAO)
        base[i] |= bitRegiao(reg);
    for (int k = 0; k < NUM_CRUZAMENTOS; k++) {
      Ponto c = pontoCruzamento(k);
      if (distancia(p.pontos[i], {c, c}) < DIST_COLISAO)
        base[i] |= bitCruzamento(k);
    }
  }

  unsigned todasRegioes = 0;
  for (int reg = 1; reg <= NUM_REGIOES; reg++)
    todasRegioes |= bitRegiao(reg);

  std::vector<unsigned> posse = base;
  bool mudou = true;
  while (mudou) {
    mudou = false;
    for (int k = 0; k < NUM_CRUZAMENTOS; k++) {
      unsigned bj = bitCruzamento(k);
      mudou |= reservarAte(
          base, posse,
          [&](int s) { return (base[s] & bj) && !(base[ant(s)] & bj); },
          [&](int q) { return base[q] & ~base[ant(q)] & todasRegioes; });
    }
    for (const Reserva &res : config.reservas) {
      if (res.trem != trem)
        continue;
      unsigned bx = bitRegiao(res.regiao), by = bitRegiao(res.ate);
      mudou |= reservarAte(
          base, posse,
          [&](int s) { return (base[s] & bx) && !(base[ant(s)] & bx); },
          [&](int q) { return base[q] & by; });
    }
  }

  if (config.semaforos) {
    for (int k = 0; k < NUM_CRUZAMENTOS; k++) {
      unsigned bj = bitCruzamento(k);
      for (int s = 0; s < n; s++) {
        if (!(base[s] & bj) || (base[ant(s)] & bj))
          continue;

        unsigned entrada = base[s] & regioesDoCruzamento(k);
        int ini = s;
        while (entrada && (posse[ant(ini)] & entrada))
          ini = ant(ini);
        for (const VagaAntecipada &v : config.antecipadas)
          if (v.trem == trem && v.cruzamento == k)
            while (posse[ant(ini)] & MASCARA_MUTEXES)
              ini = ant(ini);

        int fim = s;
        while (base[(fim + 1) % n] & bj)
          fim = (fim + 1) % n;
        for (int i = ini;; i = (i + 1) % n) {
          posse[i] |= bitSemaforo(k);
          if (i == fim)
            break;
        }
      }
    }
  }
  p.recursos = posse;

  Ponto inicio;
  switch (trem) {
  case 0:
    inicio = {(r.esq + r.dir) / 2, r.topo};
    break;
  case 1:
  case 3:
    inicio = {r.esq, (r.topo + r.base) / 2};
    break;
  case 4:
    inicio = {(r.esq + r.dir) / 2, r.base};
    break;
  default:
    inicio = {r.dir, (r.topo + r.base) / 2};
    break;
  }
  p.inicio = -1;
  for (int i = 0; i < n; i++)
    if (p.pontos[i].x == inicio.x && p.pontos[i].y == inicio.y)
      p.inicio = i;
  if (p.inicio < 0 || p.recursos[p.inicio] != 0)
    throw std::logic_error("posicao inicial invalida");

  return p;
}

}
