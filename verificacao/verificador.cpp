/*
 * Verificador da tabela de regiões críticas do projeto dos trens.
 *
 * Usa o mesmo malha.cpp do programa e verifica:
 *   1) Colisão: para qualquer par de posições de dois trens em que eles se
 *      tocariam, os dois precisam possuir alguma região em comum (logo o mutex
 *      impede que estejam ali ao mesmo tempo).
 *   2) Deadlock: explora todos os estados alcançáveis (qualquer combinação de
 *      velocidades / escalonamento) e procura um conjunto de trens em que cada
 *      um espera por uma região que está com outro trem do mesmo conjunto.
 *   3) Aproveitamento: simula os trens com várias velocidades e mede quanto
 *      eles andaram em relação ao que andariam sem nunca esperar.
 *
 * Dois modos de aquisição dos recursos novos de cada passo são comparados:
 *   - ordenado (usado pelo programa): trava um a um em ordem crescente,
 *     esperando em cada um (QMutex::lock / QSemaphore::acquire) e mantendo os
 *     que já pegou;
 *   - tudo ou nada: tenta todos; se algum está ocupado, devolve os que pegou.
 *
 * Opções:
 *   --busca           testa todas as combinações de vagas antecipadas de
 *                     semáforo (foi assim que a configuração padrão foi escolhida)
 *   --busca-reservas  testa reservas antecipadas de regiões (alternativa sem
 *                     semáforos, de pior aproveitamento)
 *
 * Compilar e executar:  make -C verificacao && ./verificacao/verificador
 */
#include "../Trem/malha.h"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <queue>
#include <random>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <vector>

using namespace Malha;

typedef std::vector<Percurso> Percursos;

static bool ordenado = true;  // modo de aquisição em análise

static Percursos montar(const Config &config){
    Percursos p;
    for (int t = 0; t < NUM_TRENS; t++)
        p.push_back(montarPercurso(t, config));
    return p;
}

static std::string regioesStr(unsigned m){
    std::string s = "{";
    auto add = [&](const std::string &nome){
        if (s.size() > 1) s += ",";
        s += nome;
    };
    for (int k = 0; k < NUM_SEMAFOROS; k++)
        if (m & bitSemaforo(k)) add("S" + std::to_string(k + 1));
    for (int r = 1; r <= NUM_REGIOES; r++)
        if (m & bitRegiao(r)) add(std::to_string(r));
    for (int k = 0; k < NUM_CRUZAMENTOS; k++)
        if (m & bitCruzamento(k)) add("J" + std::to_string(k + 1));
    return s + "}";
}

/*
 * Recursos de 'quer' que o trem t não consegue obter porque estão com os trens
 * do conjunto 'donos' (mutex: qualquer dono; semáforo: todas as vagas).
 */
static unsigned indisponiveis(unsigned quer, int t, const unsigned posse[], int donos){
    unsigned r = 0;
    for (unsigned m = quer; m; m &= m - 1) {
        unsigned b = m & (~m + 1);
        int ocupantes = 0;
        for (int o = 0; o < NUM_TRENS; o++)
            if (o != t && (donos & (1 << o)) && (posse[o] & b))
                ocupantes++;
        int limite = (b & MASCARA_SEMAFOROS) ? VAGAS_SEMAFORO : 1;
        if (ocupantes >= limite)
            r |= b;
    }
    return r;
}

const int TODOS = (1 << NUM_TRENS) - 1;

static unsigned menorBit(unsigned m){
    return m & (~m + 1);
}

//Os j primeiros bits (menores regiões) de m
static unsigned primeiros(unsigned m, int j){
    unsigned r = 0;
    for (int i = 0; i < j; i++) {
        unsigned b = menorBit(m);
        r |= b;
        m &= ~b;
    }
    return r;
}

/* ---------------------------------------------------------------- colisão */

static bool verificarColisao(const Percursos &p, bool imprimir){
    bool ok = true;
    for (int i = 0; i < NUM_TRENS; i++)
        for (int j = i + 1; j < NUM_TRENS; j++)
            for (size_t a = 0; a < p[i].pontos.size(); a++)
                for (size_t b = 0; b < p[j].pontos.size(); b++)
                    if (colidem(p[i].pontos[a], p[j].pontos[b]) && !(p[i].recursos[a] & p[j].recursos[b] & MASCARA_MUTEXES)) {
                        if (imprimir && ok)
                            printf("  COLISAO possivel: %c em (%d,%d) e %c em (%d,%d)\n",
                                   letra(i), p[i].pontos[a].x, p[i].pontos[a].y,
                                   letra(j), p[j].pontos[b].x, p[j].pontos[b].y);
                        ok = false;
                    }
    //Posições iniciais também não podem colidir
    for (int i = 0; i < NUM_TRENS; i++)
        for (int j = i + 1; j < NUM_TRENS; j++)
            if (colidem(p[i].pontos[p[i].inicio], p[j].pontos[p[j].inicio]))
                ok = false;
    return ok;
}

/* ---------------------------------------------------------------- deadlock */

/*
 * Dentro de um trecho em que as regiões possuídas não mudam, o trem anda
 * livremente. Por isso o estado de cada trem é o índice do trecho em que está
 * e, no modo ordenado, quantas regiões do próximo trecho já travou.
 */
struct Trechos {
    std::vector<unsigned> mascara;  // regiões possuídas em cada trecho
    int inicial;
};

static Trechos trechos(const Percurso &p){
    const int n = p.recursos.size();
    int b0 = 0;
    while (p.recursos[b0] == p.recursos[(b0 + n - 1) % n])
        b0++;
    Trechos t;
    t.inicial = -1;
    for (int k = 0; k < n; k++) {
        int i = (b0 + k) % n;
        if (k == 0 || p.recursos[i] != t.mascara.back())
            t.mascara.push_back(p.recursos[i]);
        if (i == p.inicio)
            t.inicial = t.mascara.size() - 1;
    }
    return t;
}

typedef uint64_t Estado;

static int trechoDe(Estado e, int t){ return (e >> (10 * t)) & 0x3f; }
static int travadasDe(Estado e, int t){ return (e >> (10 * t + 6)) & 0xf; }

static Estado comTrem(Estado e, int t, int k, int j){
    return (e & ~(Estado(0x3ff) << (10 * t))) | (Estado(k | (j << 6)) << (10 * t));
}

struct Situacao {
    unsigned posse[NUM_TRENS];   // regiões travadas por cada trem
    unsigned quer[NUM_TRENS];    // regiões pelas quais o trem espera agora
    unsigned faltam[NUM_TRENS];  // regiões novas que ainda faltam para o próximo trecho
};

static Situacao situacao(const std::vector<Trechos> &tr, Estado e){
    Situacao s;
    for (int t = 0; t < NUM_TRENS; t++) {
        int k = trechoDe(e, t), n = tr[t].mascara.size();
        unsigned atual = tr[t].mascara[k];
        unsigned novas = tr[t].mascara[(k + 1) % n] & ~atual;
        unsigned pegas = ordenado ? primeiros(novas, travadasDe(e, t)) : 0;
        s.posse[t] = atual | pegas;
        s.faltam[t] = novas & ~pegas;
        s.quer[t] = ordenado ? menorBit(s.faltam[t]) : s.faltam[t];
    }
    return s;
}

/*
 * Retorna o conjunto (máscara de trens) em deadlock no estado, ou 0.
 * Parte de todos os trens bloqueados e remove os que esperam apenas por trens
 * fora do conjunto, até estabilizar.
 */
static int conjuntoTravado(const Situacao &s){
    int conj = 0;
    for (int t = 0; t < NUM_TRENS; t++)
        if (indisponiveis(s.quer[t], t, s.posse, TODOS))
            conj |= 1 << t;
    bool mudou = true;
    while (mudou) {
        mudou = false;
        for (int t = 0; t < NUM_TRENS; t++)
            if ((conj & (1 << t)) && !indisponiveis(s.quer[t], t, s.posse, conj)) {
                conj &= ~(1 << t);
                mudou = true;
            }
    }
    return conj;
}

//Retorna true se não há deadlock; 'exemplo' recebe um estado travado, se houver
static bool verificarDeadlock(const Percursos &p, long *estados, Estado *exemplo, int *travados){
    std::vector<Trechos> tr;
    Estado ini = 0;
    for (int t = 0; t < NUM_TRENS; t++) {
        tr.push_back(trechos(p[t]));
        ini = comTrem(ini, t, tr[t].inicial, 0);
    }
    std::unordered_set<Estado> visto{ini};
    std::queue<Estado> fila;
    fila.push(ini);
    while (!fila.empty()) {
        Estado e = fila.front();
        fila.pop();
        Situacao s = situacao(tr, e);
        int conj = conjuntoTravado(s);
        if (conj) {
            *estados = visto.size();
            *exemplo = e;
            *travados = conj;
            return false;
        }
        for (int t = 0; t < NUM_TRENS; t++) {
            if (indisponiveis(s.quer[t], t, s.posse, TODOS))
                continue;
            int k = trechoDe(e, t), n = tr[t].mascara.size();
            Estado novo;
            if (ordenado && s.faltam[t])
                novo = comTrem(e, t, k, travadasDe(e, t) + 1);  // trava a próxima região
            else
                novo = comTrem(e, t, (k + 1) % n, 0);           // avança de trecho
            if (visto.insert(novo).second)
                fila.push(novo);
        }
    }
    *estados = visto.size();
    return true;
}

static void imprimirDeadlock(const Percursos &p, Estado e, int conj){
    std::vector<Trechos> tr;
    for (int t = 0; t < NUM_TRENS; t++)
        tr.push_back(trechos(p[t]));
    Situacao s = situacao(tr, e);
    for (int t = 0; t < NUM_TRENS; t++)
        if (conj & (1 << t))
            printf("    trem %c possui %s e espera por %s\n", letra(t),
                   regioesStr(s.posse[t]).c_str(), regioesStr(s.quer[t]).c_str());
}

/* ----------------------------------------------------------- aproveitamento */

/*
 * Simulação por eventos, como no programa: o trem com velocidade v anda PASSO
 * px a cada 100000/v us. Se não consegue as regiões, tenta de novo em 100 us
 * (no modo ordenado, mantendo as que já travou). Retorna o menor e o médio
 * "passos dados / passos sem espera".
 */
static void simular(const Percursos &p, const int vel[NUM_TRENS], double *media, double *minimo){
    const long DURACAO = 60L * 1000000; // 60 s simulados
    long prox[NUM_TRENS], passos[NUM_TRENS] = {0};
    int pos[NUM_TRENS];
    unsigned posse[NUM_TRENS];
    for (int t = 0; t < NUM_TRENS; t++) {
        pos[t] = p[t].inicio;
        posse[t] = 0;
        prox[t] = t;  // desempate estável
    }
    while (true) {
        int t = std::min_element(prox, prox + NUM_TRENS) - prox;
        if (prox[t] >= DURACAO)
            break;
        int n = p[t].pontos.size(), np = (pos[t] + 1) % n;
        unsigned faltam = p[t].recursos[np] & ~posse[t];
        if (ordenado) {
            while (faltam && !indisponiveis(menorBit(faltam), t, posse, TODOS)) {
                posse[t] |= menorBit(faltam);
                faltam &= ~menorBit(faltam);
            }
        } else if (!indisponiveis(faltam, t, posse, TODOS)) {
            faltam = 0;
        }
        if (faltam) {
            prox[t] += 100;
            continue;
        }
        pos[t] = np;
        posse[t] = p[t].recursos[np];
        passos[t]++;
        prox[t] += 100000 / vel[t];
    }
    *media = 0;
    *minimo = 1;
    for (int t = 0; t < NUM_TRENS; t++) {
        double ideal = double(DURACAO) / (100000 / vel[t]);
        double r = passos[t] / ideal;
        *media += r / NUM_TRENS;
        *minimo = std::min(*minimo, r);
    }
}

static void aproveitamento(const Percursos &p, double *media, double *minimo){
    std::mt19937 rng(2026);
    std::uniform_int_distribution<int> dist(1, 200);
    std::vector<std::vector<int>> cenarios = {
        {200, 200, 200, 200, 200, 200},
        {100, 100, 100, 100, 100, 100},
        {20, 20, 20, 20, 20, 20},
    };
    for (int c = 0; c < 30; c++) {
        std::vector<int> v;
        for (int t = 0; t < NUM_TRENS; t++)
            v.push_back(dist(rng));
        cenarios.push_back(v);
    }
    *media = 0;
    *minimo = 1;
    for (const auto &v : cenarios) {
        double me, mi;
        simular(p, v.data(), &me, &mi);
        *media += me / cenarios.size();
        *minimo = std::min(*minimo, mi);
    }
}

/* ------------------------------------------------------ busca das reservas */

static std::string reservasStr(const std::vector<Reserva> &rs){
    std::string s;
    for (const Reserva &r : rs)
        s += std::string(1, letra(r.trem)) + ":" + std::to_string(r.regiao) + "->" + std::to_string(r.ate) + " ";
    return s.empty() ? "nenhuma" : s;
}

static std::string vagasStr(const std::vector<VagaAntecipada> &vs){
    std::string s;
    for (const VagaAntecipada &v : vs)
        s += std::string(1, letra(v.trem)) + ":S" + std::to_string(v.cruzamento + 1) + " ";
    return s.empty() ? "nenhuma" : s;
}

static std::string configStr(const Config &c){
    return std::string(c.semaforos ? "semaforos 2 vagas" : "sem semaforos") +
           "; vagas antecipadas: " + vagasStr(c.antecipadas) +
           "; reservas extras: " + reservasStr(c.reservas);
}

static unsigned somenteRegioes(unsigned m){
    return m & MASCARA_MUTEXES & ~(bitCruzamento(0) * ((1u << NUM_CRUZAMENTOS) - 1));
}

static int menorRegiao(unsigned m){
    for (int r = 1; r <= NUM_REGIOES; r++)
        if (m & bitRegiao(r)) return r;
    return 0;
}

/*
 * Reservas candidatas: para cada trem, cada par de aquisições de regiões
 * consecutivas sem trecho livre entre elas pode ser fundido em uma só.
 */
static std::vector<Reserva> candidatas(){
    std::vector<Reserva> c;
    for (int t = 0; t < NUM_TRENS; t++) {
        Percurso p = montarPercurso(t, {false, {}, {}});
        const int n = p.recursos.size();
        auto novas = [&](int i){ return somenteRegioes(p.recursos[i] & ~p.recursos[(i + n - 1) % n]); };
        std::vector<int> eventos;
        for (int i = 0; i < n; i++)
            if (novas(i))
                eventos.push_back(i);
        for (size_t k = 0; k < eventos.size(); k++) {
            int s = eventos[k], q = eventos[(k + 1) % eventos.size()];
            bool contiguo = true;
            for (int i = s; i != q; i = (i + 1) % n)
                if (p.recursos[i] == 0)
                    contiguo = false;
            if (contiguo)
                c.push_back({t, menorRegiao(novas(s)), menorRegiao(novas(q))});
        }
    }
    return c;
}

static bool valido(const Config &c){
    Percursos p;
    try {
        p = montar(c);
    } catch (const std::exception &) {
        return false;
    }
    long estados;
    Estado ex;
    int conj;
    return verificarColisao(p, false) && verificarDeadlock(p, &estados, &ex, &conj);
}

struct Resultado {
    Config config;
    double media, minimo;
};

static void buscar(const std::vector<Reserva> &cand, bool semaforos){
    const int nc = cand.size();
    std::vector<char> ok(1 << nc, 0);
    auto config = [&](int m){
        Config c{semaforos, {}, {}};
        for (int k = 0; k < nc; k++)
            if (m & (1 << k)) c.reservas.push_back(cand[k]);
        return c;
    };
    for (int m = 0; m < (1 << nc); m++)
        ok[m] = valido(config(m));
    //Só interessam as combinações mínimas: nenhuma reserva sobrando
    std::vector<Resultado> res;
    int total = 0;
    for (int m = 0; m < (1 << nc); m++) {
        if (!ok[m]) continue;
        total++;
        bool minima = true;
        for (int k = 0; k < nc; k++)
            if ((m & (1 << k)) && ok[m & ~(1 << k)])
                minima = false;
        if (!minima) continue;
        Resultado r{config(m), 0, 0};
        aproveitamento(montar(r.config), &r.media, &r.minimo);
        res.push_back(r);
    }
    std::sort(res.begin(), res.end(),
              [](const Resultado &a, const Resultado &b){ return a.media > b.media; });
    printf("  %s: %d de %d combinacoes validas; %zu minimas. Melhores:\n",
           semaforos ? "com semaforos" : "sem semaforos", total, 1 << nc, res.size());
    for (size_t i = 0; i < res.size() && i < 3; i++)
        printf("    reservas: %s  aproveitamento medio %.1f%%  (pior trem %.1f%%)\n",
               reservasStr(res[i].config.reservas).c_str(), 100 * res[i].media, 100 * res[i].minimo);
}

/*
 * Busca das vagas antecipadas: para os trens que passam por mais de um
 * cruzamento seguido (b, c, e), testa todas as combinações de semáforos cuja
 * vaga é pega no início do trecho compartilhado.
 */
static void buscarVagas(){
    std::vector<VagaAntecipada> cand;
    for (int t = 0; t < NUM_TRENS; t++) {
        Percurso p = montarPercurso(t, {true, {}, {}});
        unsigned usados = 0;
        for (unsigned m : p.recursos)
            usados |= m & MASCARA_SEMAFOROS;
        if ((usados & (usados - 1)) == 0)
            continue;  // usa só um semáforo: a vaga já é pega no início do trecho
        for (int k = 0; k < NUM_SEMAFOROS; k++)
            if (usados & bitSemaforo(k))
                cand.push_back({t, k});
    }
    printf("  candidatas: %s\n", vagasStr(cand).c_str());
    const int nc = cand.size();
    auto config = [&](int m){
        Config c{true, {}, {}};
        for (int k = 0; k < nc; k++)
            if (m & (1 << k)) c.antecipadas.push_back(cand[k]);
        return c;
    };
    std::vector<char> ok(1 << nc, 0);
    for (int m = 0; m < (1 << nc); m++)
        ok[m] = valido(config(m));
    std::vector<Resultado> res;
    int total = 0;
    for (int m = 0; m < (1 << nc); m++) {
        if (!ok[m]) continue;
        total++;
        bool minima = true;
        for (int k = 0; k < nc; k++)
            if ((m & (1 << k)) && ok[m & ~(1 << k)])
                minima = false;
        if (!minima) continue;
        Resultado r{config(m), 0, 0};
        aproveitamento(montar(r.config), &r.media, &r.minimo);
        res.push_back(r);
    }
    std::sort(res.begin(), res.end(),
              [](const Resultado &a, const Resultado &b){ return a.media > b.media; });
    printf("  %d de %d combinacoes validas; %zu minimas. Melhores:\n", total, 1 << nc, res.size());
    for (size_t i = 0; i < res.size() && i < 5; i++)
        printf("    vagas antecipadas: %s  aproveitamento medio %.1f%%  (pior trem %.1f%%)\n",
               vagasStr(res[i].config.antecipadas).c_str(), 100 * res[i].media, 100 * res[i].minimo);
}

/* -------------------------------------------------------------------- main */

static bool analisar(const Config &c, bool detalhar){
    long estados;
    Estado ex;
    int conj;
    Percursos p = montar(c);
    bool semColisao = verificarColisao(p, true);
    bool semDeadlock = verificarDeadlock(p, &estados, &ex, &conj);
    printf("  colisao: %s\n", semColisao ? "nenhuma" : "POSSIVEL");
    printf("  deadlock: %s (%ld estados alcancaveis explorados)\n", semDeadlock ? "nenhum" : "ENCONTRADO", estados);
    if (!semDeadlock)
        imprimirDeadlock(p, ex, conj);
    double media, minimo;
    aproveitamento(p, &media, &minimo);
    printf("  aproveitamento medio %.1f%% (pior trem %.1f%%)\n", 100 * media, 100 * minimo);
    if (detalhar) {
        printf("  recursos possuidos em cada trecho do percurso (a partir da posicao inicial):\n");
        for (int t = 0; t < NUM_TRENS; t++) {
            printf("    %c:", letra(t));
            Trechos tr = trechos(p[t]);
            for (int k = 0; k < (int)tr.mascara.size(); k++) {
                int i = (tr.inicial + k) % tr.mascara.size();
                printf(" %s", regioesStr(tr.mascara[i]).c_str());
            }
            printf("\n");
        }
    }
    return semColisao && semDeadlock;
}

int main(int argc, char **argv){
    bool busca = argc > 1 && std::string(argv[1]) == "--busca";

    printf("== Sem semaforos e sem reservas extras ==\n");
    for (int modo = 1; modo >= 0; modo--) {
        ordenado = modo;
        printf(" modo %s:\n", ordenado ? "ordenado" : "tudo ou nada");
        analisar({false, {}, {}}, false);
    }

    if (busca) {
        for (int modo = 1; modo >= 0; modo--) {
            ordenado = modo;
            printf("\n== Busca de vagas antecipadas (modo %s) ==\n", ordenado ? "ordenado" : "tudo ou nada");
            buscarVagas();
        }
    }

    if (argc > 1 && std::string(argv[1]) == "--busca-reservas") {
        std::vector<Reserva> cand = candidatas();
        printf("\n== Busca de reservas extras ==\n  candidatas: %s\n", reservasStr(cand).c_str());
        for (int modo = 1; modo >= 0; modo--) {
            ordenado = modo;
            printf(" modo %s:\n", ordenado ? "ordenado" : "tudo ou nada");
            buscar(cand, false);
            buscar(cand, true);
        }
    }

    printf("\n== Configuracao usada pelo programa (%s) ==\n", configStr(configPadrao()).c_str());
    bool ok = true;
    for (int modo = 0; modo <= 1; modo++) {
        ordenado = modo;
        printf(" modo %s:\n", ordenado ? "ordenado (usado pelo programa)" : "tudo ou nada");
        bool r = analisar(configPadrao(), ordenado);
        if (ordenado) ok = r;
    }
    return ok ? 0 : 1;
}
