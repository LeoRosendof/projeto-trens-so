// Verificador opcional: não faz parte do executável dos trens.
// Usa as mesmas tabelas de Trem/malha.cpp e confere, para qualquer combinação
// de velocidades, que não há colisão nem deadlock.
#include "../Trem/malha.h"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <queue>
#include <random>
#include <string>
#include <unordered_set>
#include <vector>

using namespace Malha;

typedef std::vector<Percurso> Percursos;
typedef uint64_t Estado;

static const int TODOS = (1 << NUM_TRENS) - 1;

static char letra(int t) { return 'a' + t; }
static unsigned menorBit(unsigned m) { return m & (~m + 1); }

static Percursos montar(const Config &c) {
  Percursos p;
  for (int t = 0; t < NUM_TRENS; t++)
    p.push_back(montarPercurso(t, c));
  return p;
}

static int inicioDe(const Percurso &p, int t) {
  static const Ponto INICIO[NUM_TRENS] = {{420, 30},  {180, 270}, {660, 270},
                                          {60, 430},  {420, 510}, {780, 430}};
  for (size_t i = 0; i < p.pontos.size(); i++)
    if (p.pontos[i].x == INICIO[t].x && p.pontos[i].y == INICIO[t].y)
      return i;
  std::abort();
}

static std::string nomes(unsigned m) {
  std::string s;
  for (int k = 0; k < NUM_SEMAFOROS; k++)
    if (m & bitSemaforo(k))
      s += "S" + std::to_string(k + 1) + " ";
  for (int r = 1; r <= NUM_REGIOES; r++)
    if (m & bitRegiao(r))
      s += std::to_string(r) + " ";
  return s.empty() ? "-" : s.substr(0, s.size() - 1);
}

// Dois trens que se tocariam precisam sempre ter um mutex em comum.
static bool semColisao(const Percursos &p) {
  bool ok = true;
  for (int i = 0; i < NUM_TRENS; i++)
    for (int j = i + 1; j < NUM_TRENS; j++)
      for (size_t a = 0; a < p[i].pontos.size(); a++)
        for (size_t b = 0; b < p[j].pontos.size(); b++) {
          Ponto u = p[i].pontos[a], v = p[j].pontos[b];
          if (std::abs(u.x - v.x) < LARGURA_TREM + 1 &&
              std::abs(u.y - v.y) < ALTURA_TREM + 1 &&
              !(p[i].recursos[a] & p[j].recursos[b] & MASCARA_MUTEXES)) {
            if (ok)
              printf("  colisao: %c (%d,%d) e %c (%d,%d)\n", letra(i), u.x, u.y,
                     letra(j), v.x, v.y);
            ok = false;
          }
        }
  return ok;
}

// Junta os pontos seguidos que exigem os mesmos recursos.
struct Trechos {
  std::vector<unsigned> mascara;
  int inicial;
};

static Trechos trechos(const Percurso &p, int t) {
  const int n = p.recursos.size(), ini = inicioDe(p, t);
  Trechos r;
  r.inicial = 0;
  for (int k = 0; k < n; k++) {
    unsigned m = p.recursos[(ini + k) % n];
    if (r.mascara.empty() || r.mascara.back() != m)
      r.mascara.push_back(m);
  }
  if (r.mascara.size() > 1 && r.mascara.back() == r.mascara.front())
    r.mascara.pop_back();
  return r;
}

// Por trem: 6 bits do trecho e 4 bits de quantos recursos novos já pegou.
static int trechoDe(Estado e, int t) { return (e >> (10 * t)) & 0x3f; }
static int pegosDe(Estado e, int t) { return (e >> (10 * t + 6)) & 0xf; }
static Estado comTrem(Estado e, int t, int k, int j) {
  return (e & ~(Estado(0x3ff) << (10 * t))) | (Estado(k | (j << 6)) << (10 * t));
}

struct Situacao {
  unsigned posse[NUM_TRENS], quer[NUM_TRENS];
};

static Situacao situacao(const std::vector<Trechos> &tr, Estado e) {
  Situacao s;
  for (int t = 0; t < NUM_TRENS; t++) {
    int k = trechoDe(e, t), n = tr[t].mascara.size();
    unsigned atual = tr[t].mascara[k];
    unsigned novas = tr[t].mascara[(k + 1) % n] & ~atual;
    for (int i = 0; i < pegosDe(e, t); i++) {
      atual |= menorBit(novas);
      novas &= ~menorBit(novas);
    }
    s.posse[t] = atual;
    s.quer[t] = menorBit(novas);
  }
  return s;
}

static bool bloqueado(const Situacao &s, int t, int entre) {
  unsigned b = s.quer[t];
  if (!b)
    return false;
  int ocupantes = 0;
  for (int o = 0; o < NUM_TRENS; o++)
    if (o != t && (entre & (1 << o)) && (s.posse[o] & b))
      ocupantes++;
  return ocupantes >= ((b & MASCARA_SEMAFOROS) ? VAGAS_SEMAFORO : 1);
}

// Conjunto de trens em que cada um espera por recursos presos só pelo próprio
// conjunto. Se não for vazio, esses trens nunca mais andam.
static int travados(const Situacao &s) {
  int conj = 0;
  for (int t = 0; t < NUM_TRENS; t++)
    if (bloqueado(s, t, TODOS))
      conj |= 1 << t;
  for (bool mudou = true; mudou;) {
    mudou = false;
    for (int t = 0; t < NUM_TRENS; t++)
      if ((conj & (1 << t)) && !bloqueado(s, t, conj)) {
        conj &= ~(1 << t);
        mudou = true;
      }
  }
  return conj;
}

// Busca em largura por todos os estados alcançáveis.
static bool semDeadlock(const Percursos &p, long *estados, bool imprimir) {
  std::vector<Trechos> tr;
  Estado ini = 0;
  for (int t = 0; t < NUM_TRENS; t++) {
    tr.push_back(trechos(p[t], t));
    ini = comTrem(ini, t, tr[t].inicial, 0);
  }
  std::unordered_set<Estado> visto{ini};
  std::queue<Estado> fila;
  fila.push(ini);
  while (!fila.empty()) {
    Estado e = fila.front();
    fila.pop();
    Situacao s = situacao(tr, e);
    int conj = travados(s);
    if (conj) {
      if (imprimir)
        for (int t = 0; t < NUM_TRENS; t++)
          if (conj & (1 << t))
            printf("    trem %c tem {%s} e espera {%s}\n", letra(t),
                   nomes(s.posse[t]).c_str(), nomes(s.quer[t]).c_str());
      *estados = visto.size();
      return false;
    }
    for (int t = 0; t < NUM_TRENS; t++) {
      if (bloqueado(s, t, TODOS))
        continue;
      int k = trechoDe(e, t), n = tr[t].mascara.size();
      Estado novo = s.quer[t] ? comTrem(e, t, k, pegosDe(e, t) + 1)
                              : comTrem(e, t, (k + 1) % n, 0);
      if (visto.insert(novo).second)
        fila.push(novo);
    }
  }
  *estados = visto.size();
  return true;
}

// Simula 60 s e mede quanto cada trem andou em relação a andar sem esperar.
static void simular(const Percursos &p, const int vel[], double *media,
                    double *pior) {
  const long DURACAO = 60L * 1000000;
  long prox[NUM_TRENS], passos[NUM_TRENS] = {0};
  int pos[NUM_TRENS];
  unsigned posse[NUM_TRENS];
  for (int t = 0; t < NUM_TRENS; t++) {
    pos[t] = inicioDe(p[t], t);
    posse[t] = 0;
    prox[t] = t;
  }
  while (true) {
    int t = std::min_element(prox, prox + NUM_TRENS) - prox;
    if (prox[t] >= DURACAO)
      break;
    int np = (pos[t] + 1) % int(p[t].pontos.size());
    unsigned faltam = p[t].recursos[np] & ~posse[t];
    while (faltam) {
      unsigned b = menorBit(faltam);
      int ocupantes = 0;
      for (int o = 0; o < NUM_TRENS; o++)
        if (o != t && (posse[o] & b))
          ocupantes++;
      if (ocupantes >= ((b & MASCARA_SEMAFOROS) ? VAGAS_SEMAFORO : 1))
        break;
      posse[t] |= b;
      faltam &= ~b;
    }
    if (faltam) {
      prox[t] += 100;
      continue;
    }
    pos[t] = np;
    posse[t] = p[t].recursos[np];
    passos[t]++;
    prox[t] += 200000 / vel[t];
  }
  *media = 0;
  *pior = 1;
  for (int t = 0; t < NUM_TRENS; t++) {
    double r = passos[t] / (double(DURACAO) / (200000 / vel[t]));
    *media += r / NUM_TRENS;
    *pior = std::min(*pior, r);
  }
}

static void aproveitamento(const Percursos &p, double *media, double *pior) {
  std::mt19937 rng(2026);
  std::uniform_int_distribution<int> dist(1, 200);
  std::vector<std::vector<int>> cenarios = {std::vector<int>(NUM_TRENS, 200),
                                            std::vector<int>(NUM_TRENS, 100),
                                            std::vector<int>(NUM_TRENS, 20)};
  for (int c = 0; c < 30; c++) {
    std::vector<int> v;
    for (int t = 0; t < NUM_TRENS; t++)
      v.push_back(dist(rng));
    cenarios.push_back(v);
  }
  *media = 0;
  *pior = 1;
  for (const auto &v : cenarios) {
    double me, pi;
    simular(p, v.data(), &me, &pi);
    *media += me / cenarios.size();
    *pior = std::min(*pior, pi);
  }
}

// Testa todas as combinações de vagas antecipadas para b, c e e.
static void buscar() {
  static const int MEIO[3] = {1, 2, 4};
  struct Item {
    int m;
    double media, pior;
  };
  std::vector<Item> validas;
  for (int m = 0; m < (1 << 9); m++) {
    Config c = {true, {0, 0, 0, 0, 0, 0}};
    for (int i = 0; i < 9; i++)
      if (m & (1 << i))
        c.antecipadas[MEIO[i / 3]] |= bitSemaforo(i % 3 == 0 ? 0 : i % 3 == 1 ? 1 : 3);
    long estados;
    Percursos p = montar(c);
    if (!semColisao(p) || !semDeadlock(p, &estados, false))
      continue;
    Item it{m, 0, 0};
    aproveitamento(p, &it.media, &it.pior);
    validas.push_back(it);
  }
  std::sort(validas.begin(), validas.end(),
            [](const Item &a, const Item &b) { return a.media > b.media; });
  printf("%zu combinacoes validas de 512. Melhores:\n", validas.size());
  for (size_t i = 0; i < validas.size() && i < 8; i++) {
    std::string s;
    for (int k = 0; k < 9; k++)
      if (validas[i].m & (1 << k))
        s += std::string(1, letra(MEIO[k / 3])) + ":S" +
             std::to_string(k % 3 == 0 ? 1 : k % 3 == 1 ? 2 : 4) + " ";
    printf("  %-28s media %.1f%%  pior trem %.1f%%\n", s.c_str(),
           100 * validas[i].media, 100 * validas[i].pior);
  }
}

static bool analisar(const char *titulo, const Config &c) {
  long estados;
  Percursos p = montar(c);
  printf("== %s ==\n", titulo);
  bool a = semColisao(p);
  bool b = semDeadlock(p, &estados, true);
  double media, pior;
  aproveitamento(p, &media, &pior);
  printf("  colisao: %s\n  deadlock: %s (%ld estados explorados)\n",
         a ? "nenhuma" : "POSSIVEL", b ? "nenhum" : "ENCONTRADO", estados);
  printf("  aproveitamento medio %.1f%% (pior trem %.1f%%)\n", 100 * media,
         100 * pior);
  return a && b;
}

int main(int argc, char **argv) {
  if (argc > 1 && std::string(argv[1]) == "--busca") {
    buscar();
    return 0;
  }
  Config semSemaforos = {false, {0, 0, 0, 0, 0, 0}};
  analisar("Sem semaforos (o deadlock deve aparecer)", semSemaforos);
  return analisar("Configuracao usada pelo programa", configPadrao()) ? 0 : 1;
}
