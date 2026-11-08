#include "trem.h"
#include <QtCore>
#include <QMutex>
#include <QSemaphore>

using namespace Malha;

//Um mutex por região crítica: só um trem por vez em cada trecho compartilhado
static QMutex regiao[NUM_REGIOES];

//Um semáforo por entroncamento: no máximo 2 dos 3 trens do ciclo de espera
static QSemaphore s1(VAGAS_SEMAFORO), s2(VAGAS_SEMAFORO), s3(VAGAS_SEMAFORO), s4(VAGAS_SEMAFORO);
static QSemaphore *semaforo[NUM_SEMAFOROS] = {&s1, &s2, &s3, &s4};

//Construtor
Trem::Trem(int ID, int x, int y){
    this->ID = ID;
    this->x = x;
    this->y = y;
    velocidade = 100;
    possuidos = 0;

    percurso = montarPercurso(ID - 1);
    posicao = -1;
    for(size_t i = 0; i < percurso.pontos.size(); i++)
        if(percurso.pontos[i].x == x && percurso.pontos[i].y == y)
            posicao = i;
    if(posicao < 0 || percurso.recursos[posicao] != 0)
        qFatal("Posicao inicial invalida para o trem %d", ID);
}

void Trem::setVelocidade(int valor){
    velocidade = valor;
}

//Pede os recursos um a um, na ordem fixa: semáforos e depois regiões
void Trem::adquirir(unsigned recursos){
    for(int i = 0; i < NUM_RECURSOS; i++){
        if(!(recursos & (1u << i)))
            continue;
        if(i < NUM_SEMAFOROS)
            semaforo[i]->acquire();
        else
            regiao[i - NUM_SEMAFOROS].lock();
        possuidos |= 1u << i;
    }
}

void Trem::liberar(unsigned recursos){
    for(int i = NUM_RECURSOS - 1; i >= 0; i--){
        if(!(recursos & possuidos & (1u << i)))
            continue;
        if(i < NUM_SEMAFOROS)
            semaforo[i]->release();
        else
            regiao[i - NUM_SEMAFOROS].unlock();
        possuidos &= ~(1u << i);
    }
}

//Função a ser executada após executar trem->START
void Trem::run(){
    const int total = percurso.pontos.size();
    emit updateGUI(ID, x, y);

    while(!isInterruptionRequested()){
        if(velocidade == 0){    //Slider na extremidade esquerda: trem parado
            msleep(10);
            continue;
        }

        int proxima = (posicao + 1) % total;
        adquirir(percurso.recursos[proxima] & ~possuidos);

        //Se o trem foi parado durante a espera, não prende o trecho à frente
        if(velocidade == 0 || isInterruptionRequested()){
            liberar(possuidos & ~percurso.recursos[posicao]);
            continue;
        }

        posicao = proxima;
        x = percurso.pontos[posicao].x;
        y = percurso.pontos[posicao].y;
        emit updateGUI(ID, x, y);    //Emite um sinal
        liberar(possuidos & ~percurso.recursos[posicao]);

        //10 px por movimento: 50 px/s em v = 1 e 10000 px/s em v = 200
        int v = velocidade;
        if(v > 0)
            usleep(200000 / v);
    }
    liberar(possuidos);
}
