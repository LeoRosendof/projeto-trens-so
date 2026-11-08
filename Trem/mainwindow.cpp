#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    //Cria o trem com seu (ID, posição X, posição Y)
    trem1 = new Trem(1,420,30);
    trem2 = new Trem(2,180,270);
    trem3 = new Trem(3,660,270);
    trem4 = new Trem(4,60,430);
    trem5 = new Trem(5,420,510);
    trem6 = new Trem(6,780,430);

    /*
     * Conecta o sinal UPDATEGUI à função UPDATEINTERFACE.
     * Ou seja, sempre que o sinal UPDATEGUI foi chamado, será executada a função UPDATEINTERFACE.
     * Os 3 parâmetros INT do sinal serão utilizados na função.
     */
    connect(trem1,SIGNAL(updateGUI(int,int,int)),SLOT(updateInterface(int,int,int)));
    connect(trem2,SIGNAL(updateGUI(int,int,int)),SLOT(updateInterface(int,int,int)));
    connect(trem3,SIGNAL(updateGUI(int,int,int)),SLOT(updateInterface(int,int,int)));
    connect(trem4,SIGNAL(updateGUI(int,int,int)),SLOT(updateInterface(int,int,int)));
    connect(trem5,SIGNAL(updateGUI(int,int,int)),SLOT(updateInterface(int,int,int)));
    connect(trem6,SIGNAL(updateGUI(int,int,int)),SLOT(updateInterface(int,int,int)));

    //Os trens começam a andar assim que o programa abre
    trem1->start();
    trem2->start();
    trem3->start();
    trem4->start();
    trem5->start();
    trem6->start();
}

//Função que será executada quando o sinal UPDATEGUI for emitido
void MainWindow::updateInterface(int id, int x, int y){
    switch(id){
    case 1: //Atualiza a posição do objeto da tela (quadrado) que representa o trem1
        ui->label_trem1->setGeometry(x,y,21,17);
        break;
    case 2: //Atualiza a posição do objeto da tela (quadrado) que representa o trem2
        ui->label_trem2->setGeometry(x,y,21,17);
        break;
    case 3:
        ui->label_trem3->setGeometry(x,y,21,17);
        break;
    case 4:
        ui->label_trem4->setGeometry(x,y,21,17);
        break;
    case 5:
        ui->label_trem5->setGeometry(x,y,21,17);
        break;
    case 6:
        ui->label_trem6->setGeometry(x,y,21,17);
        break;
    default:
        break;
    }
}

void MainWindow::on_slider_a_valueChanged(int valor){ trem1->setVelocidade(valor); }
void MainWindow::on_slider_b_valueChanged(int valor){ trem2->setVelocidade(valor); }
void MainWindow::on_slider_c_valueChanged(int valor){ trem3->setVelocidade(valor); }
void MainWindow::on_slider_d_valueChanged(int valor){ trem4->setVelocidade(valor); }
void MainWindow::on_slider_e_valueChanged(int valor){ trem5->setVelocidade(valor); }
void MainWindow::on_slider_f_valueChanged(int valor){ trem6->setVelocidade(valor); }

/*
 * Controle mestre: move os sliders individuais, e cada um deles
 * atualiza a velocidade do seu trem.
 */
void MainWindow::on_slider_m_valueChanged(int valor){
    ui->slider_a->setValue(valor);
    ui->slider_b->setValue(valor);
    ui->slider_c->setValue(valor);
    ui->slider_d->setValue(valor);
    ui->slider_e->setValue(valor);
    ui->slider_f->setValue(valor);
}

//Clicar no mestre sem mudar o valor também sobrescreve os ajustes individuais
void MainWindow::on_slider_m_sliderPressed(){
    on_slider_m_valueChanged(ui->slider_m->value());
}

MainWindow::~MainWindow()
{
    Trem *trens[] = {trem1, trem2, trem3, trem4, trem5, trem6};
    for(Trem *t : trens)
        t->requestInterruption();
    for(Trem *t : trens){
        t->wait();
        delete t;
    }
    delete ui;
}
