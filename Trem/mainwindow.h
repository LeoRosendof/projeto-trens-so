#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "trem.h"
#include <QLabel>
#include <QMainWindow>
#include <QSlider>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  explicit MainWindow(QWidget *parent = nullptr);
  ~MainWindow();

public slots:
  void updateInterface(int, int, int);

private slots:
  void alterarVelocidade(int);
  void aplicarControleMestre();

private:
  Ui::MainWindow *ui;

  Trem *trem[Malha::NUM_TRENS];

  QLabel *labelTrem[Malha::NUM_TRENS];
  QSlider *sliderTrem[Malha::NUM_TRENS];
  QLabel *labelValor[Malha::NUM_TRENS];
};

#endif
