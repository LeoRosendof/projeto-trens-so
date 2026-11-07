#include "trem.h"

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <ctime>
#include <semaphore.h>
#include <stdexcept>
#include <string>

using namespace Malha;

static pthread_mutex_t regiao[NUM_REGIOES];
static pthread_mutex_t cruzamento[NUM_CRUZAMENTOS];
static sem_t semaforo[NUM_SEMAFOROS];
static pthread_once_t recursosCriados = PTHREAD_ONCE_INIT;

static void verificar(int erro, const char *operacao) {
  if (erro != 0)
    throw std::runtime_error(std::string(operacao) + ": " + strerror(erro));
}

static void criarRecursos() {
  for (int i = 0; i < NUM_REGIOES; i++)
    pthread_mutex_init(&regiao[i], nullptr);
  for (int i = 0; i < NUM_CRUZAMENTOS; i++)
    pthread_mutex_init(&cruzamento[i], nullptr);
  for (int i = 0; i < NUM_SEMAFOROS; i++)
    sem_init(&semaforo[i], 0, VAGAS_SEMAFORO);
}

static void esperarSemaforo(sem_t *s) {
  while (sem_wait(s) == -1 && errno == EINTR) {
  }
}

static void dormir(long microssegundos) {
  struct timespec t;
  t.tv_sec = microssegundos / 1000000;
  t.tv_nsec = (microssegundos % 1000000) * 1000;
  while (nanosleep(&t, &t) == -1 && errno == EINTR) {
  }
}

Trem::Trem(int ID)
    : ID(ID), velocidade(100), executando(false), iniciada(false),
      possuidos(0) {
  verificar(pthread_once(&recursosCriados, criarRecursos), "pthread_once");
  percurso = montarPercurso(ID - 1);
  x = percurso.pontos[percurso.inicio].x;
  y = percurso.pontos[percurso.inicio].y;
}

Trem::~Trem() {
  parar();
  aguardar();
}

void Trem::iniciar() {
  if (iniciada)
    return;
  executando = true;
  verificar(pthread_create(&thread, nullptr, &Trem::executar, this),
            "pthread_create");
  iniciada = true;
}

void Trem::parar() { executando = false; }

void Trem::aguardar() {
  if (!iniciada)
    return;
  pthread_join(thread, nullptr);
  iniciada = false;
}

void Trem::setVelocidade(int valor) {
  velocidade = std::max(0, std::min(valor, VELOCIDADE_MAXIMA));
}

void *Trem::executar(void *arg) {
  static_cast<Trem *>(arg)->run();
  return nullptr;
}

void Trem::adquirir(unsigned recursos) {
  for (int i = 0; i < NUM_RECURSOS && executando; i++) {
    unsigned bit = 1u << i;
    if (!(recursos & bit))
      continue;
    if (i < NUM_SEMAFOROS)
      esperarSemaforo(&semaforo[i]);
    else if (i < NUM_SEMAFOROS + NUM_REGIOES)
      pthread_mutex_lock(&regiao[i - NUM_SEMAFOROS]);
    else
      pthread_mutex_lock(&cruzamento[i - NUM_SEMAFOROS - NUM_REGIOES]);
    possuidos |= bit;
  }
}

void Trem::liberar(unsigned recursos) {
  for (int i = NUM_RECURSOS - 1; i >= 0; i--) {
    unsigned bit = 1u << i;
    if (!(recursos & possuidos & bit))
      continue;
    if (i < NUM_SEMAFOROS)
      sem_post(&semaforo[i]);
    else if (i < NUM_SEMAFOROS + NUM_REGIOES)
      pthread_mutex_unlock(&regiao[i - NUM_SEMAFOROS]);
    else
      pthread_mutex_unlock(&cruzamento[i - NUM_SEMAFOROS - NUM_REGIOES]);
    possuidos &= ~bit;
  }
}

void Trem::run() {
  const int n = percurso.pontos.size();
  int pos = percurso.inicio;
  emit updateGUI(ID, x, y);

  while (executando) {
    if (velocidade == 0) {
      dormir(10000);
      continue;
    }

    int prox = (pos + 1) % n;
    adquirir(percurso.recursos[prox] & ~possuidos);
    if (!executando)
      break;

    if (velocidade == 0) {
      liberar(possuidos & ~percurso.recursos[pos]);
      continue;
    }

    pos = prox;
    x = percurso.pontos[pos].x;
    y = percurso.pontos[pos].y;
    emit updateGUI(ID, x, y);
    liberar(possuidos & ~percurso.recursos[pos]);

    int v = velocidade;
    if (v > 0)
      dormir(100000 / v);
  }

  liberar(possuidos);
}
