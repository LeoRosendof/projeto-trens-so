# Projeto dos Trens: resumo da solução

## Estrutura
- `Trem/`: projeto Qt5 construído sobre o código disponibilizado no SIGAA. Abra `Trem.pro` no Qt Creator ou rode `qmake && make` dentro da pasta.
- `verificacao/`: verificador opcional, que não faz parte do executável. Usa as mesmas tabelas da malha e confere colisões e deadlocks. Para rodar: `make -C verificacao && ./verificacao/verificador`.

## Threads
Cada trem é uma `QThread` (classe `Trem`, função `run()`) e não depende das demais. Ela anda 10 px por vez no seu retângulo, no sentido anti-horário, e avisa a interface pelo sinal `updateGUI`. Não há thread central: cada trem decide sozinho, usando os mutexes e semáforos. Os trens partem assim que o programa abre.

## Velocidade
Os sliders vão de 0 a 200. Em 0 o trem para. Com valor v, ele anda 10 px a cada 200000/v µs: 50 px/s em v = 1 e 10000 px/s em v = 200. O slider mestre `m` move todos os sliders individuais, que atualizam a velocidade de cada trem. Depois disso, cada slider individual vale só para o seu trem, até o mestre ser usado de novo.

## Colisões (9 mutexes)
Cada uma das regiões críticas 1 a 9 é um trilho percorrido em sentidos opostos por dois trens, protegido por um `QMutex`.

- O trem pede o mutex antes de chegar a menos de 30 px da região e só o solta depois de se afastar dela. Como o trem tem 21 × 17 px, sobra folga entre os quadrados.
- Num entroncamento, onde três regiões se encontram, o trem precisa ter a região de entrada e a de saída ao mesmo tempo. Com três regiões, dois pares sempre têm uma em comum, então dois trens nunca ficam no mesmo entroncamento.

## Deadlocks (4 semáforos)
- **O problema:** em cada entroncamento, três trens podem formar um ciclo de espera. Exemplo: `a` está na região 1 e quer a 2, `c` está na 2 e quer a 3, `b` está na 3 e quer a 1.
- **Semáforo de 2 vagas por entroncamento:** um `QSemaphore` para cada ciclo (S1 = regiões 1, 2, 3; S2 = 4, 5, 8; S3 = 3, 5, 6; S4 = 6, 7, 9). No máximo 2 dos 3 trens entram no ciclo, então ele nunca se fecha. É a mesma ideia da solução do jantar dos filósofos.
- **Trens do meio:** `b`, `c` e `e` atravessam três entroncamentos seguidos, e os semáforos poderiam travar entre si. Por isso cada um pega uma vaga já no início do trecho: `b` pega S1, `c` pega S4 e `e` pega S2.
- **Ordem fixa:** em cada passo, o trem pede primeiro os semáforos e depois os mutexes, sempre na mesma ordem.
- **Trem parado:** se o slider vai a 0 enquanto o trem espera, ele devolve o que tinha acabado de pegar, para não prender à toa o trecho à frente.

## Verificação
O verificador explora todos os estados alcançáveis, o que cobre qualquer combinação de velocidades: 139.652 estados, sem deadlock. Também confere todos os pares de posições: dois trens que se tocariam sempre precisam de um mesmo mutex. Sem os semáforos, ele mostra o deadlock do exemplo acima. A opção `--busca` compara as combinações de vagas antecipadas.
