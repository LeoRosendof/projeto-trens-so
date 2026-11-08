#ifndef TREM_H
#define TREM_H

#include <QThread>
#include <atomic>

#include "malha.h"

/*
 * Classe Trem herda QThread
 * Classe Trem passa a ser uma thread.
 * A função START inicializa a thread. Após inicializada, a thread irá executar a função RUN.
 *
*/
class Trem: public QThread{
 Q_OBJECT
public:
    Trem(int,int,int);  //construtor
    void run();         //função a ser executada pela thread
    void setVelocidade(int);

//Cria um sinal
signals:
    void updateGUI(int,int,int);

private:
   void adquirir(unsigned recursos);
   void liberar(unsigned recursos);

   int x;           //posição X do trem na tela
   int y;           //posição Y do trem na tela
   int ID;          //ID do trem
   std::atomic<int> velocidade;  //Valor do slider: 0 (parado) até 200 (máxima)

   Malha::Percurso percurso;     //Pontos da volta e recursos exigidos em cada um
   int posicao;                  //Índice do ponto atual no percurso
   unsigned possuidos;           //Recursos que este trem tem no momento
};

#endif // TREM_H
