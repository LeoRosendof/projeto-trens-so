# Projeto dos Trens: resumo da solução

## Estrutura
- `Trem/`: projeto Qt5 (baseado no código do SIGAA). Abra `Trem.pro` no Qt Creator ou rode `qmake && make`.
- `verificacao/`: verificador opcional, que não faz parte do executável. Ele usa a mesma tabela da malha e prova que não há colisão nem deadlock. Para rodar: `make -C verificacao && ./verificacao/verificador`.

## Threads
Cada trem roda numa `pthread` própria (`pthread_create` em `trem.cpp`; o encerramento é feito com `pthread_join`). Ela anda 5 px por vez no seu retângulo, no sentido anti-horário, e avisa a interface pelo sinal `updateGUI`. Não há thread central: cada trem decide sozinho, usando os mutexes e semáforos.

## Velocidade
Os sliders vão de 0 a 200. Em 0 o trem para. Com valor v, ele anda 5 px a cada 100000/v µs: 50 px/s em v = 1 e 10000 px/s em v = 200. O slider mestre `m` sobrescreve todos os trens e move os sliders individuais. Depois disso, cada slider individual volta a valer só para o seu trem.

## Colisões (mutexes)
Todos os recursos são POSIX: `pthread_mutex_t` para regiões e cruzamentos, `sem_t` (`sem_init`/`sem_wait`/`sem_post`) para os semáforos. Compile no Linux com `qmake && make` (o `.pro` já passa `-pthread`).

- **9 mutexes de região (1 a 9):** cada região é um trilho único percorrido em sentidos opostos pelos dois trens vizinhos. Então só um trem por vez pode estar nela.
- **4 mutexes de cruzamento (J1 a J4):** são os pontos em "T" onde três regiões se encontram (J1 = 1/2/3, J2 = 4/5/8, J3 = 3/5/6, J4 = 6/7/9). Perto do cruzamento, quem protege é o mutex dele.
- O trem pede o mutex quando está a menos de 25 px da região ou do cruzamento. Os quadrados têm 20 px, então sobram 5 px de folga. Ele solta o mutex assim que sai dessa distância.
- **Regra de não bloquear o cruzamento:** o trem só entra no cruzamento se já tiver a região de saída.

## Deadlocks (semáforos)
- **O problema:** em cada cruzamento, 3 trens formam um ciclo de espera. Por exemplo, em J1: `a` está na região 1 e quer a 2; `c` está na 2 e quer a 3; `b` está na 3 e quer a 1.
- **Semáforo de 2 vagas por cruzamento (S1 a S4):** no máximo 2 dos 3 trens podem estar perto do cruzamento ao mesmo tempo. Assim o ciclo nunca se fecha, como na solução do jantar dos filósofos.
- **Trens do meio:** b, c e e passam por três cruzamentos seguidos. Cada um pega a vaga do último cruzamento logo no início do trecho, antes de ter qualquer mutex:
  - `b` antecipa S1;
  - `c` antecipa S4;
  - `e` antecipa S2.
- **Ordem fixa:** em cada passo, os recursos são pedidos sempre na mesma ordem: semáforos, depois regiões, depois cruzamentos.

## Prova
O verificador explora todos os estados alcançáveis, ou seja, qualquer combinação de velocidades: são 448.446 estados. Ele não encontra nenhum estado em que um grupo de trens espere um pelo outro para sempre. Também confere todos os pares de posições: dois trens que se tocariam sempre precisam de um mesmo mutex. Sem os semáforos, ele mostra o deadlock do exemplo acima.
