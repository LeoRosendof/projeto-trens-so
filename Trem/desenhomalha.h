#ifndef DESENHOMALHA_H
#define DESENHOMALHA_H

#include <QWidget>

/*
 * Desenha os trilhos da malha ferroviária (Figura 1 do enunciado): o retângulo
 * de cada trem e uma seta indicando o sentido anti-horário.
 */
class DesenhoMalha : public QWidget
{
public:
    explicit DesenhoMalha(QWidget *parent = 0);

protected:
    void paintEvent(QPaintEvent *event);
};

#endif // DESENHOMALHA_H
