#ifndef MALHA_H
#define MALHA_H

#include <vector>

/*
 * Geometria da malha ferroviária e tabela de recursos (mutexes e semáforos)
 * que cada trem precisa possuir em cada posição do seu percurso.
 *
 * Este arquivo é C++ puro (não depende do Qt) para que a mesma tabela usada
 * pelo programa seja usada pelo verificador em ../verificacao, que prova a
 * ausência de colisões e de deadlocks.
 *
 * A malha é uma grade de 7 colunas (0..6) e 4 linhas (0..3). Números são as
 * regiões críticas e '*' os cruzamentos em "T" (J1..J4):
 *
 *          c0    c1    c2    c3    c4    c5    c6
 *   lin0         +-----------a-----------+
 *                |                       |
 *   lin1         +-----1-----*-----2-----+             * em (c3,lin1) = J1
 *                |     b     3     c     |
 *   lin2   +-----+--4--*--5--*--6--*--7--+-----+       * em c2, c3, c4 = J2, J3, J4
 *          |     d     8     e     9     f     |
 *   lin3   +-----------+-----------+-----------+
 *
 * Trens: 0..5 (a..f). Regiões críticas: 1..9 (Figura 2 do enunciado).
 *
 * Recursos usados para evitar colisão e deadlock:
 *   - 9 mutexes de região: cada região é um trilho único percorrido em sentidos
 *     opostos pelos dois trens que o compartilham, então só um trem por vez.
 *   - 4 mutexes de cruzamento (J1..J4): o ponto em "T" onde três regiões se
 *     encontram. O trem só entra no cruzamento se já tiver a região de saída
 *     ("não bloquear o cruzamento").
 *   - 4 semáforos com 2 vagas, um por cruzamento: em cada cruzamento há 3 trens
 *     que formam um ciclo de espera (cada um precisa da região do outro).
 *     Permitindo no máximo 2 deles perto do cruzamento, o ciclo não se fecha.
 *
 * Cada recurso é um bit de uma máscara. A ordem dos bits é a ordem em que o
 * trem os adquire num mesmo passo: semáforos, depois regiões, depois cruzamentos.
 */
namespace Malha {

const int NUM_TRENS = 6;
const int NUM_REGIOES = 9;
const int NUM_CRUZAMENTOS = 4;
const int NUM_SEMAFOROS = NUM_CRUZAMENTOS;
const int VAGAS_SEMAFORO = 2;
const int NUM_RECURSOS = NUM_SEMAFOROS + NUM_REGIOES + NUM_CRUZAMENTOS;

const int ORIGEM_X = 40;   // posição (px) da coluna 0
const int ORIGEM_Y = 40;   // posição (px) da linha 0
const int ESPACO_X = 90;   // distância (px) entre colunas
const int ESPACO_Y = 120;  // distância (px) entre linhas

const int PASSO = 5;       // deslocamento (px) do trem a cada movimento
const int TAM_TREM = 20;   // lado (px) do quadrado que representa o trem

/*
 * Dois trens cujos centros estão a menos de DIST_COLISAO px um do outro, nos
 * dois eixos, são considerados em colisão (os quadrados de 20 px se tocariam,
 * com 5 px de folga). Pelo mesmo motivo, um trem precisa da posse de uma região
 * ou cruzamento sempre que estiver a menos de DIST_COLISAO px dele.
 */
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

/*
 * Reserva antecipada: ao adquirir a região 'regiao', o trem adquire também tudo
 * de que vai precisar até chegar à região 'ate'. Não é usada pela configuração
 * padrão, mas o verificador a usa para comparar alternativas.
 */
struct Reserva {
    int trem;
    int regiao;
    int ate;
};

/*
 * Vaga antecipada: o trem pega a vaga do semáforo do cruzamento logo no início
 * do trecho compartilhado que leva até ele (onde ainda não possui nenhum mutex),
 * em vez de pegá-la apenas ao entrar na região que dá acesso ao cruzamento.
 */
struct VagaAntecipada {
    int trem;
    int cruzamento;
};

struct Config {
    bool semaforos;                          // usa os semáforos de 2 vagas
    std::vector<VagaAntecipada> antecipadas; // vagas pegas no início do trecho
    std::vector<Reserva> reservas;           // reservas antecipadas extras
};

struct Percurso {
    std::vector<Ponto> pontos;      // posições do trem, no sentido anti-horário
    std::vector<unsigned> recursos; // recursos que o trem deve possuir em cada posição
    int inicio;                     // índice da posição inicial (sem nenhum recurso)
};

//Bits de cada recurso na máscara
inline unsigned bitSemaforo(int k){ return 1u << k; }                                   // k = 0..3
inline unsigned bitRegiao(int r){ return 1u << (NUM_SEMAFOROS + r - 1); }               // r = 1..9
inline unsigned bitCruzamento(int k){ return 1u << (NUM_SEMAFOROS + NUM_REGIOES + k); } // k = 0..3
const unsigned MASCARA_SEMAFOROS = (1u << NUM_SEMAFOROS) - 1;
const unsigned MASCARA_MUTEXES = ((1u << NUM_RECURSOS) - 1) & ~MASCARA_SEMAFOROS;

int colunaX(int coluna);
int linhaY(int linha);

char letra(int trem);
Retangulo retangulo(int trem);
Segmento segmentoRegiao(int regiao);
Ponto pontoCruzamento(int cruzamento);

bool colidem(Ponto p, Ponto q);

const Config &configPadrao();

Percurso montarPercurso(int trem, const Config &config = configPadrao());

} // namespace Malha

#endif // MALHA_H
