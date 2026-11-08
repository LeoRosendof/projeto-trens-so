#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
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
    void on_slider_a_valueChanged(int valor);
    void on_slider_b_valueChanged(int valor);
    void on_slider_c_valueChanged(int valor);
    void on_slider_d_valueChanged(int valor);
    void on_slider_e_valueChanged(int valor);
    void on_slider_f_valueChanged(int valor);
    void on_slider_m_valueChanged(int valor);
    void on_slider_m_sliderPressed();

private:
    Ui::MainWindow *ui;

    //Cria os objetos TREM's
    Trem *trem1;
    Trem *trem2;
    Trem *trem3;
    Trem *trem4;
    Trem *trem5;
    Trem *trem6;
};

#endif // MAINWINDOW_H
