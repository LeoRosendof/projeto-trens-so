#include "malha.h"

#include <algorithm>
#include <cstdlib>
#include <stdexcept>

namespace Malha {

int colunaX(int coluna){
    return ORIGEM_X + coluna * ESPACO_X;
}

int linhaY(int linha){
    return ORIGEM_Y + linha * ESPACO_Y;
}

char letra(int trem){
    return 'a' + trem;
}

//Retângulo (em px) percorrido por cada trem
Retangulo retangulo(int trem){
    //                         col esq, col dir, lin topo, lin base
    static const int grade[NUM_TRENS][4] = {
        {1, 5, 0, 1},   // a
        {1, 3, 1, 2},   // b
        {3, 5, 1, 2},   // c
        {0, 2, 2, 3},   // d
        {2, 4, 2, 3},   // e
        {4, 6, 2, 3},   // f
    };
    const int *g = grade[trem];
    return {colunaX(g[0]), colunaX(g[1]), linhaY(g[2]), linhaY(g[3])};
}

//Trecho de trilho (em px) de cada região crítica, conforme a Figura 2
Segmento segmentoRegiao(int regiao){
    //                            col1, lin1, col2, lin2
    static const int grade[NUM_REGIOES][4] = {
        {1, 1, 3, 1},   // 1: a (base) x b (topo)
        {3, 1, 5, 1},   // 2: a (base) x c (topo)
        {3, 1, 3, 2},   // 3: b (direita) x c (esquerda)
        {1, 2, 2, 2},   // 4: b (base) x d (topo)
        {2, 2, 3, 2},   // 5: b (base) x e (topo)
        {3, 2, 4, 2},   // 6: c (base) x e (topo)
        {4, 2, 5, 2},   // 7: c (base) x f (topo)
        {2, 2, 2, 3},   // 8: d (direita) x e (esquerda)
        {4, 2, 4, 3},   // 9: e (direita) x f (esquerda)
    };
    const int *g = grade[regiao - 1];
    return {{colunaX(g[0]), linhaY(g[1])}, {colunaX(g[2]), linhaY(g[3])}};
}

//Cruzamentos em "T", onde três regiões se encontram
Ponto pontoCruzamento(int cruzamento){
    static const int grade[NUM_CRUZAMENTOS][2] = {
        {3, 1},   // J1: regiões 1, 2, 3
        {2, 2},   // J2: regiões 4, 5, 8
        {3, 2},   // J3: regiões 3, 5, 6
        {4, 2},   // J4: regiões 6, 7, 9
    };
    return {colunaX(grade[cruzamento][0]), linhaY(grade[cruzamento][1])};
}

//Regiões que se encontram em cada cruzamento (formam um ciclo de espera)
static unsigned regioesDoCruzamento(int cruzamento){
    static const int regioes[NUM_CRUZAMENTOS][3] = {
        {1, 2, 3}, {4, 5, 8}, {3, 5, 6}, {6, 7, 9},
    };
    const int *r = regioes[cruzamento];
    return bitRegiao(r[0]) | bitRegiao(r[1]) | bitRegiao(r[2]);
}

//Distância (máximo entre as distâncias nos eixos X e Y) de um ponto a um segmento
static int distancia(Ponto p, Segmento s){
    int dx = std::max({0, std::min(s.a.x, s.b.x) - p.x, p.x - std::max(s.a.x, s.b.x)});
    int dy = std::max({0, std::min(s.a.y, s.b.y) - p.y, p.y - std::max(s.a.y, s.b.y)});
    return std::max(dx, dy);
}

bool colidem(Ponto p, Ponto q){
    return std::abs(p.x - q.x) < DIST_COLISAO && std::abs(p.y - q.y) < DIST_COLISAO;
}

static bool ehCruzamento(Ponto p){
    for (int k = 0; k < NUM_CRUZAMENTOS; k++) {
        Ponto c = pontoCruzamento(k);
        if (c.x == p.x && c.y == p.y)
            return true;
    }
    return false;
}

//Aproxima o ponto 'p' de 'q' em 'd' px (os dois na mesma linha ou coluna)
static Ponto aproximar(Ponto p, Ponto q, int d){
    if (p.x != q.x) p.x += (q.x > p.x) ? d : -d;
    if (p.y != q.y) p.y += (q.y > p.y) ? d : -d;
    return p;
}

/*
 * Trecho de uma região sem as pontas que ficam dentro de um cruzamento: perto
 * do cruzamento quem protege é o mutex do cruzamento, e não as outras regiões.
 */
static Segmento trechoProprio(int regiao){
    Segmento s = segmentoRegiao(regiao);
    Segmento t = s;
    if (ehCruzamento(s.a)) t.a = aproximar(s.a, s.b, DIST_COLISAO);
    if (ehCruzamento(s.b)) t.b = aproximar(s.b, s.a, DIST_COLISAO);
    return t;
}

/*
 * Configuração escolhida pelo verificador (sem colisão, sem deadlock e com o
 * maior aproveitamento da malha): semáforos de 2 vagas em todos os cruzamentos
 * e, para os trens do meio (b, c, e), que passam por três cruzamentos seguidos,
 * a vaga do último cruzamento é pega já no início do trecho compartilhado:
 *   b: J2 -> J3 -> J1, antecipa S1
 *   c: J1 -> J3 -> J4, antecipa S4
 *   e: J4 -> J3 -> J2, antecipa S2
 */
const Config &configPadrao(){
    static const Config config = {
        true,
        {{1, 0}, {2, 3}, {4, 1}},
        {},
    };
    return config;
}

/*
 * A partir de cada posição 's' em que 'deveIniciar' é verdadeiro, o trem passa
 * a possuir de uma vez tudo de que vai precisar até a primeira posição 'q' em
 * que 'chegou' é verdadeiro. Retorna true se alterou alguma posição.
 */
template <class Inicio, class Chegada>
static bool reservarAte(const std::vector<unsigned> &base, std::vector<unsigned> &posse,
                        Inicio deveIniciar, Chegada chegou){
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
        //Percorre de q até s acumulando os recursos necessários à frente
        unsigned acumulado = 0;
        for (int i = q; ; i = (i + n - 1) % n) {
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

Percurso montarPercurso(int trem, const Config &config){
    Percurso p;
    Retangulo r = retangulo(trem);

    //Sentido anti-horário na tela: topo para a esquerda, lado esquerdo para baixo,
    //base para a direita e lado direito para cima
    for (int x = r.dir; x > r.esq; x -= PASSO)  p.pontos.push_back({x, r.topo});
    for (int y = r.topo; y < r.base; y += PASSO) p.pontos.push_back({r.esq, y});
    for (int x = r.esq; x < r.dir; x += PASSO)  p.pontos.push_back({x, r.base});
    for (int y = r.base; y > r.topo; y -= PASSO) p.pontos.push_back({r.dir, y});

    const int n = p.pontos.size();
    auto ant = [n](int i){ return (i + n - 1) % n; };

    //1) Regiões e cruzamentos a menos de DIST_COLISAO px de cada posição
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

    //2) Não bloquear o cruzamento: ao entrar nele, o trem já pega a região de
    //   saída. 3) Reservas antecipadas extras da configuração.
    //   Repete até estabilizar, para que reservas encadeadas se somem.
    unsigned todasRegioes = 0;
    for (int reg = 1; reg <= NUM_REGIOES; reg++)
        todasRegioes |= bitRegiao(reg);

    std::vector<unsigned> posse = base;
    bool mudou = true;
    while (mudou) {
        mudou = false;
        for (int k = 0; k < NUM_CRUZAMENTOS; k++) {
            unsigned bj = bitCruzamento(k);
            mudou |= reservarAte(base, posse,
                [&](int s){ return (base[s] & bj) && !(base[ant(s)] & bj); },
                [&](int q){ return base[q] & ~base[ant(q)] & todasRegioes; });
        }
        for (const Reserva &res : config.reservas) {
            if (res.trem != trem)
                continue;
            unsigned bx = bitRegiao(res.regiao), by = bitRegiao(res.ate);
            mudou |= reservarAte(base, posse,
                [&](int s){ return (base[s] & bx) && !(base[ant(s)] & bx); },
                [&](int q){ return base[q] & by; });
        }
    }

    //4) Semáforo do cruzamento: o trem ocupa uma das 2 vagas desde que entra na
    //   região que leva ao cruzamento até sair do cruzamento
    if (config.semaforos) {
        for (int k = 0; k < NUM_CRUZAMENTOS; k++) {
            unsigned bj = bitCruzamento(k);
            for (int s = 0; s < n; s++) {
                if (!(base[s] & bj) || (base[ant(s)] & bj))
                    continue;
                //Início: onde começa a posse da região de entrada no cruzamento
                //(que pode ter sido antecipada ao passar por outro cruzamento)
                unsigned entrada = base[s] & regioesDoCruzamento(k);
                int ini = s;
                while (entrada && (posse[ant(ini)] & entrada))
                    ini = ant(ini);
                for (const VagaAntecipada &v : config.antecipadas)
                    if (v.trem == trem && v.cruzamento == k)
                        while (posse[ant(ini)] & MASCARA_MUTEXES)
                            ini = ant(ini);
                //Fim: última posição dentro do cruzamento
                int fim = s;
                while (base[(fim + 1) % n] & bj)
                    fim = (fim + 1) % n;
                for (int i = ini; ; i = (i + 1) % n) {
                    posse[i] |= bitSemaforo(k);
                    if (i == fim)
                        break;
                }
            }
        }
    }
    p.recursos = posse;

    //Posição inicial: meio de um lado do retângulo que não é compartilhado
    Ponto inicio;
    switch (trem) {
    case 0:  inicio = {(r.esq + r.dir) / 2, r.topo}; break;   // a: topo
    case 1:
    case 3:  inicio = {r.esq, (r.topo + r.base) / 2}; break;  // b, d: lado esquerdo
    case 4:  inicio = {(r.esq + r.dir) / 2, r.base}; break;   // e: base
    default: inicio = {r.dir, (r.topo + r.base) / 2}; break;  // c, f: lado direito
    }
    p.inicio = -1;
    for (int i = 0; i < n; i++)
        if (p.pontos[i].x == inicio.x && p.pontos[i].y == inicio.y)
            p.inicio = i;
    if (p.inicio < 0 || p.recursos[p.inicio] != 0)
        throw std::logic_error("posicao inicial invalida");

    return p;
}

} // namespace Malha
