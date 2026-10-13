#include "desenhomalha.h"
#include "malha.h"

#include <QPainter>

using namespace Malha;

DesenhoMalha::DesenhoMalha(QWidget *parent) : QWidget(parent)
{
    setAttribute(Qt::WA_TransparentForMouseEvents);
}

void DesenhoMalha::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    //Trilhos: o retângulo de cada trem
    p.setPen(QPen(QColor(0, 102, 230), 4, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin));
    p.setBrush(Qt::NoBrush);
    for (int t = 0; t < NUM_TRENS; t++) {
        Retangulo r = retangulo(t);
        p.drawRect(QRect(QPoint(r.esq, r.topo), QPoint(r.dir, r.base)));
    }

    //Seta anti-horária no centro de cada retângulo
    p.setPen(QPen(QColor(110, 110, 110), 4, Qt::SolidLine, Qt::RoundCap));
    for (int t = 0; t < NUM_TRENS; t++) {
        Retangulo r = retangulo(t);
        QPoint c((r.esq + r.dir) / 2, (r.topo + r.base) / 2 + 8);
        const int raio = 18;
        QRect caixa(c.x() - raio, c.y() - raio, 2 * raio, 2 * raio);
        p.drawArc(caixa, 20 * 16, 150 * 16);   //arco de 20° a 170°, sobre o centro

        //Ponta da seta na extremidade esquerda, apontando para baixo
        QPointF ponta(c.x() - raio * 0.985, c.y() + 2);
        QPolygonF cabeca;
        cabeca << ponta + QPointF(0, 6) << ponta + QPointF(-7, -4) << ponta + QPointF(7, -4);
        p.setBrush(QColor(110, 110, 110));
        p.setPen(Qt::NoPen);
        p.drawPolygon(cabeca);
        p.setPen(QPen(QColor(110, 110, 110), 4, Qt::SolidLine, Qt::RoundCap));
        p.setBrush(Qt::NoBrush);
    }
}
