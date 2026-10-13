#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "desenhomalha.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    //Desenho dos trilhos, atrás dos trens
    DesenhoMalha *desenho = new DesenhoMalha(ui->centralWidget);
    desenho->setGeometry(0, 0, 600, height());
    desenho->lower();

    labelTrem[0] = ui->label_trem1;
    labelTrem[1] = ui->label_trem2;
    labelTrem[2] = ui->label_trem3;
    labelTrem[3] = ui->label_trem4;
    labelTrem[4] = ui->label_trem5;
    labelTrem[5] = ui->label_trem6;

    sliderTrem[0] = ui->slider_a;
    sliderTrem[1] = ui->slider_b;
    sliderTrem[2] = ui->slider_c;
    sliderTrem[3] = ui->slider_d;
    sliderTrem[4] = ui->slider_e;
    sliderTrem[5] = ui->slider_f;

    labelValor[0] = ui->valor_a;
    labelValor[1] = ui->valor_b;
    labelValor[2] = ui->valor_c;
    labelValor[3] = ui->valor_d;
    labelValor[4] = ui->valor_e;
    labelValor[5] = ui->valor_f;

    for (int i = 0; i < Malha::NUM_TRENS; i++) {
        //Cria o trem com seu ID (1 a 6 = trens a..f); a posição inicial vem da malha
        trem[i] = new Trem(i + 1);
        trem[i]->setVelocidade(sliderTrem[i]->value());
        labelValor[i]->setNum(sliderTrem[i]->value());

        /*
         * Conecta o sinal UPDATEGUI à função UPDATEINTERFACE.
         * Ou seja, sempre que o sinal UPDATEGUI foi chamado, será executada a função UPDATEINTERFACE.
         * Os 3 parâmetros INT do sinal serão utilizados na função.
         */
        connect(trem[i],SIGNAL(updateGUI(int,int,int)),SLOT(updateInterface(int,int,int)));

        //Slider individual: altera apenas a velocidade do próprio trem
        connect(sliderTrem[i],SIGNAL(valueChanged(int)),SLOT(alterarVelocidade(int)));
    }

    //Slider mestre: sempre que acionado, sobrescreve a velocidade de todos os trens
    ui->valor_m->setNum(ui->slider_m->value());
    connect(ui->slider_m,SIGNAL(valueChanged(int)),SLOT(aplicarControleMestre()));
    connect(ui->slider_m,SIGNAL(sliderPressed()),SLOT(aplicarControleMestre()));

    //Os trens começam a andar assim que o programa abre
    for (int i = 0; i < Malha::NUM_TRENS; i++)
        trem[i]->start();
}

//Função que será executada quando o sinal UPDATEGUI for emitido
void MainWindow::updateInterface(int id, int x, int y){
    if (id < 1 || id > Malha::NUM_TRENS)
        return;
    //Atualiza a posição do objeto da tela (quadrado) que representa o trem, centrado no trilho
    const int lado = Malha::TAM_TREM;
    labelTrem[id - 1]->setGeometry(x - lado / 2, y - lado / 2, lado, lado);
}

void MainWindow::alterarVelocidade(int valor){
    for (int i = 0; i < Malha::NUM_TRENS; i++)
        if (sender() == sliderTrem[i]) {
            trem[i]->setVelocidade(valor);
            labelValor[i]->setNum(valor);
        }
}

/*
 * Controle mestre: posiciona todos os sliders individuais no valor do mestre,
 * o que (via alterarVelocidade) aplica a nova velocidade a cada trem.
 */
void MainWindow::aplicarControleMestre(){
    int valor = ui->slider_m->value();
    ui->valor_m->setNum(valor);
    for (int i = 0; i < Malha::NUM_TRENS; i++) {
        sliderTrem[i]->setValue(valor);
        trem[i]->setVelocidade(valor);
        labelValor[i]->setNum(valor);
    }
}

MainWindow::~MainWindow()
{
    //Encerra as threads antes de destruir a interface
    for (int i = 0; i < Malha::NUM_TRENS; i++)
        trem[i]->parar();
    for (int i = 0; i < Malha::NUM_TRENS; i++) {
        trem[i]->wait();
        delete trem[i];
    }
    delete ui;
}
