#ifndef SLIDERTREM_H
#define SLIDERTREM_H

#include <QPainter>
#include <QSlider>
#include <QStyle>
#include <QStyleOptionSlider>

/*
 * QSlider com a letra do trem desenhada no puxador, como na Figura 3.
 * A letra é o último caractere do nome do objeto (slider_a, slider_b, ...).
 */
class SliderTrem : public QSlider {
public:
    explicit SliderTrem(QWidget *parent = nullptr) : QSlider(Qt::Horizontal, parent) {}

protected:
    void paintEvent(QPaintEvent *event) override {
        QSlider::paintEvent(event);

        QStyleOptionSlider opcao;
        initStyleOption(&opcao);
        QRect puxador = style()->subControlRect(QStyle::CC_Slider, &opcao,
                                                QStyle::SC_SliderHandle, this);
        QPainter painter(this);
        QFont fonte = font();
        fonte.setBold(true);
        painter.setFont(fonte);
        painter.drawText(puxador, Qt::AlignCenter, objectName().right(1));
    }
};

#endif // SLIDERTREM_H
