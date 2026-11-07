#ifndef DESENHOMALHA_H
#define DESENHOMALHA_H

#include <QWidget>

class DesenhoMalha : public QWidget {
public:
  explicit DesenhoMalha(QWidget *parent = 0);

protected:
  void paintEvent(QPaintEvent *event);
};

#endif
