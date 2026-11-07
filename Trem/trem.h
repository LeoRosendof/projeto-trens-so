#ifndef TREM_H
#define TREM_H

#include <QObject>
#include <atomic>
#include <pthread.h>

#include "malha.h"

class Trem : public QObject {
  Q_OBJECT
public:
  explicit Trem(int ID);
  ~Trem();

  void iniciar();
  void parar();
  void aguardar();
  void setVelocidade(int valor);

  static const int VELOCIDADE_MAXIMA = 200;

signals:
  void updateGUI(int, int, int);

private:
  static void *executar(void *arg);
  void run();
  void adquirir(unsigned recursos);
  void liberar(unsigned recursos);

  int x;
  int y;
  int ID;
  std::atomic<int> velocidade;
  std::atomic<bool> executando;

  pthread_t thread;
  bool iniciada;

  Malha::Percurso percurso;
  unsigned possuidos;
};

#endif
