#ifndef TREM_H
#define TREM_H

#include <QThread>
#include <atomic>
#include "malha.h"

/*
 * Classe Trem herda QThread
 * Classe Trem passa a ser uma thread.
 * A função START inicializa a thread. Após inicializada, a thread irá executar a função RUN.
 * Para encerrar a execução da função RUN da thread, basta executar a função PARAR.
 *
 * Cada trem anda no seu retângulo, no sentido anti-horário, de forma totalmente
 * independente dos demais. Antes de cada passo ele adquire os mutexes/semáforos
 * de que vai precisar na próxima posição (tabela montada em malha.cpp) e, após
 * o passo, libera os que não precisa mais.
*/
class Trem: public QThread{
 Q_OBJECT
public:
    Trem(int ID);                   //construtor (ID de 1 a 6 = trens a..f)
    void run();                     //função a ser executada pela thread
    void setVelocidade(int valor);  //0 (parado) a 200 (velocidade máxima)
    void parar();                   //encerra a thread (ao fechar o programa)

    static const int VELOCIDADE_MAXIMA = 200;

//Cria um sinal
signals:
    void updateGUI(int,int,int);

private:
    void adquirir(unsigned recursos);
    void liberar(unsigned recursos);

    int x;                          //posição X do trem na tela (centro)
    int y;                          //posição Y do trem na tela (centro)
    int ID;                         //ID do trem
    std::atomic<int> velocidade;    //0 (parado) a VELOCIDADE_MAXIMA
    std::atomic<bool> executando;

    Malha::Percurso percurso;       //posições e recursos necessários em cada uma
    unsigned possuidos;             //recursos (mutexes/semáforos) que o trem possui agora
};

#endif // TREM_H
