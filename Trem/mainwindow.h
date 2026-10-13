#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QSlider>
#include "trem.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();


public slots:
    void updateInterface(int,int,int);

private slots:
    void alterarVelocidade(int);    //slider individual de um trem
    void aplicarControleMestre();   //slider mestre (m): vale para todos os trens

private:
    Ui::MainWindow *ui;

    //Cria os objetos TREM's (a..f)
    Trem *trem[Malha::NUM_TRENS];

    QLabel *labelTrem[Malha::NUM_TRENS];
    QSlider *sliderTrem[Malha::NUM_TRENS];
    QLabel *labelValor[Malha::NUM_TRENS];
};

#endif // MAINWINDOW_H
