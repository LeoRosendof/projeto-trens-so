#include "trem.h"
#include <QtCore>

using namespace Malha;

//Semáforo de um cruzamento: começa com VAGAS_SEMAFORO vagas
class SemaforoCruzamento : public QSemaphore {
public:
    SemaforoCruzamento() : QSemaphore(VAGAS_SEMAFORO) {}
};

/*
 * Recursos compartilhados por todas as threads (trens):
 *   - um mutex por região crítica (1..9);
 *   - um mutex por cruzamento em "T" (J1..J4);
 *   - um semáforo de 2 vagas por cruzamento, para que no máximo 2 dos 3 trens
 *     que formam o ciclo de espera daquele cruzamento estejam perto dele.
 */
static SemaforoCruzamento semaforo[NUM_SEMAFOROS];
static QMutex regiao[NUM_REGIOES];
static QMutex cruzamento[NUM_CRUZAMENTOS];

//Construtor
Trem::Trem(int ID){
    this->ID = ID;
    percurso = montarPercurso(ID - 1);
    x = percurso.pontos[percurso.inicio].x;
    y = percurso.pontos[percurso.inicio].y;
    velocidade = 100;
    executando = true;
    possuidos = 0;
}

void Trem::setVelocidade(int valor){
    velocidade = qBound(0, valor, VELOCIDADE_MAXIMA);
}

void Trem::parar(){
    executando = false;
}

/*
 * Adquire os recursos um a um, sempre em ordem crescente de bit (semáforos,
 * depois regiões, depois cruzamentos), esperando em cada um. Essa é exatamente
 * a ordem analisada pelo verificador, que prova a ausência de deadlock.
 */
void Trem::adquirir(unsigned recursos){
    for (int i = 0; i < NUM_RECURSOS && executando; i++) {
        unsigned bit = 1u << i;
        if (!(recursos & bit))
            continue;
        if (i < NUM_SEMAFOROS)
            semaforo[i].acquire();
        else if (i < NUM_SEMAFOROS + NUM_REGIOES)
            regiao[i - NUM_SEMAFOROS].lock();
        else
            cruzamento[i - NUM_SEMAFOROS - NUM_REGIOES].lock();
        possuidos |= bit;
    }
}

void Trem::liberar(unsigned recursos){
    for (int i = 0; i < NUM_RECURSOS; i++) {
        unsigned bit = 1u << i;
        if (!(recursos & bit))
            continue;
        if (i < NUM_SEMAFOROS)
            semaforo[i].release();
        else if (i < NUM_SEMAFOROS + NUM_REGIOES)
            regiao[i - NUM_SEMAFOROS].unlock();
        else
            cruzamento[i - NUM_SEMAFOROS - NUM_REGIOES].unlock();
        possuidos &= ~bit;
    }
}

//Função a ser executada após executar trem->START
void Trem::run(){
    const int n = percurso.pontos.size();
    int pos = percurso.inicio;
    emit updateGUI(ID, x, y);

    while (executando) {
        //Velocidade mínima: o trem fica parado
        if (velocidade == 0) {
            msleep(10);
            continue;
        }

        //Reserva o que falta para a próxima posição (pode esperar outro trem)
        int prox = (pos + 1) % n;
        adquirir(percurso.recursos[prox] & ~possuidos);
        if (!executando)
            break;

        //Se a velocidade foi zerada enquanto esperava, não anda e devolve o
        //que acabou de reservar
        if (velocidade == 0) {
            liberar(possuidos & ~percurso.recursos[pos]);
            continue;
        }

        //Anda e libera o que não é mais necessário na nova posição
        pos = prox;
        x = percurso.pontos[pos].x;
        y = percurso.pontos[pos].y;
        emit updateGUI(ID, x, y);    //Emite um sinal
        liberar(possuidos & ~percurso.recursos[pos]);

        //Velocidade v: anda PASSO px a cada 100000/v microssegundos
        //(v = 1: 50 px/s; v = 200: 10000 px/s)
        int v = velocidade;
        if (v > 0)
            usleep(100000 / v);
    }

    liberar(possuidos);
}
