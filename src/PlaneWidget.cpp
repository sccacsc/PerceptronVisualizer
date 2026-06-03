#include "PlaneWidget.h"

#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>

#include <algorithm>
#include <cmath>

PlaneWidget::PlaneWidget(QWidget* parent) : QWidget(parent)
{
    setMinimumSize(620, 430);
    setMouseTracking(true);
}

void PlaneWidget::setModel(NetworkModel* model)
{
    m_model = model;
    update();
}

void PlaneWidget::setObjectPoint(const QPointF& point)
{
    m_point = point;
    update();
}

QPointF PlaneWidget::objectPoint() const
{
    return m_point;
}

void PlaneWidget::setShowAreas(bool show)
{
    m_showAreas = show;
    update();
}

void PlaneWidget::setShowLines(bool show)
{
    m_showLines = show;
    update();
}

void PlaneWidget::setShowObject(bool show)
{
    m_showObject = show;
    update();
}

QRectF PlaneWidget::plotRect() const
{
    int margin = 44;
    return QRectF(margin, margin, width() - 2 * margin, height() - 2 * margin);
}

QPointF PlaneWidget::toScreen(double x1, double x2) const
{
    QRectF r = plotRect();
    double sx = r.left() + (x1 - m_min) / (m_max - m_min) * r.width();
    double sy = r.bottom() - (x2 - m_min) / (m_max - m_min) * r.height();
    return QPointF(sx, sy);
}

QPointF PlaneWidget::fromScreen(const QPointF& p) const
{
    QRectF r = plotRect();
    double x = m_min + (p.x() - r.left()) / r.width() * (m_max - m_min);
    double y = m_min + (r.bottom() - p.y()) / r.height() * (m_max - m_min);
    x = std::max(m_min, std::min(m_max, x));
    y = std::max(m_min, std::min(m_max, y));
    return QPointF(x, y);
}

void PlaneWidget::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.fillRect(rect(), Qt::white);

    if (m_model && m_showAreas) drawAreas(painter);
    drawGrid(painter);
    if (m_model && m_showLines) drawBoundaries(painter);
    if (m_model && m_showObject) drawObject(painter);
    drawLegend(painter);
}

void PlaneWidget::mousePressEvent(QMouseEvent* event)
{
    if (!plotRect().contains(event->position())) return;
    QPointF logical = fromScreen(event->position());
    emit pointSelected(logical.x(), logical.y());
}

void PlaneWidget::drawAreas(QPainter& painter)
{
    QRectF r = plotRect();
    painter.save();
    painter.setClipRect(r);

    const int cell = 6;
    for (int y = static_cast<int>(r.top()); y < r.bottom(); y += cell) {
        for (int x = static_cast<int>(r.left()); x < r.right(); x += cell) {
            QPointF logical = fromScreen(QPointF(x + cell / 2.0, y + cell / 2.0));
            EvaluationResult e = m_model->evaluate(logical.x(), logical.y());

            QColor color;
            // Цвет показывает области распознавания: синий — класс 1 (A), красный — класс 2 (B).
            // Остальная графика остаётся строгой: оси, границы и подписи выполняются чёрным цветом.
            if (m_model->preset() == Preset::TwoTriangles) {
                if (e.insideB) color = QColor(255, 205, 205);              // область B — класс 2
                else if (e.insideA) color = QColor(205, 225, 255);         // область A — класс 1
                else color = QColor(245, 245, 245);                       // вне демонстрационных областей
            } else {
                color = e.finalClass ? QColor(255, 205, 205) : QColor(205, 225, 255);
            }
            painter.fillRect(QRect(x, y, cell + 1, cell + 1), color);
        }
    }

    painter.restore();
}

void PlaneWidget::drawGrid(QPainter& painter)
{
    QRectF r = plotRect();
    painter.save();

    painter.setPen(QPen(QColor(225, 225, 225), 1));
    for (int i = static_cast<int>(m_min); i <= static_cast<int>(m_max); ++i) {
        QPointF a = toScreen(i, m_min);
        QPointF b = toScreen(i, m_max);
        painter.drawLine(a, b);
        QPointF c = toScreen(m_min, i);
        QPointF d = toScreen(m_max, i);
        painter.drawLine(c, d);
    }

    const QPointF xAxisLeft = toScreen(m_min, 0.0);
    const QPointF xAxisRight = toScreen(m_max, 0.0);
    const QPointF yAxisBottom = toScreen(0.0, m_min);
    const QPointF yAxisTop = toScreen(0.0, m_max);

    painter.setPen(QPen(Qt::black, 2));
    painter.drawLine(xAxisLeft, xAxisRight);
    painter.drawLine(yAxisBottom, yAxisTop);

    QFont tickFont = painter.font();
    tickFont.setPointSize(9);
    painter.setFont(tickFont);
    QFontMetrics fm(tickFont);

    painter.setPen(QPen(Qt::black, 1));
    const int tickSize = 4;

    for (int value = static_cast<int>(m_min); value <= static_cast<int>(m_max); ++value) {
        const QPointF xTick = toScreen(value, 0.0);
        painter.drawLine(QPointF(xTick.x(), xAxisLeft.y() - tickSize),
                         QPointF(xTick.x(), xAxisLeft.y() + tickSize));

        const QString xText = QString::number(value);
        const int xTextWidth = fm.horizontalAdvance(xText);
        painter.drawText(QPointF(xTick.x() - xTextWidth / 2.0,
                                 xAxisLeft.y() + fm.height() + 4),
                         xText);

        const QPointF yTick = toScreen(0.0, value);
        painter.drawLine(QPointF(yAxisBottom.x() - tickSize, yTick.y()),
                         QPointF(yAxisBottom.x() + tickSize, yTick.y()));

        if (value != 0) {
            const QString yText = QString::number(value);
            const int yTextWidth = fm.horizontalAdvance(yText);
            painter.drawText(QPointF(yAxisBottom.x() - yTextWidth - 8,
                                     yTick.y() + fm.ascent() / 2.0),
                             yText);
        }
    }

    QFont axisFont = painter.font();
    axisFont.setPointSize(10);
    axisFont.setBold(true);
    painter.setFont(axisFont);
    painter.setPen(Qt::black);
    painter.drawText(toScreen(m_max, 0) + QPointF(-18, -8), "x1");
    painter.drawText(toScreen(0, m_max) + QPointF(8, 18), "x2");

    painter.setPen(QPen(Qt::black, 1));
    painter.drawRect(r);
    painter.restore();
}

void PlaneWidget::drawBoundaries(QPainter& painter)
{
    const auto& hidden = m_model->hiddenLayer();
    QVector<Qt::PenStyle> styles = {
        Qt::SolidLine, Qt::DashLine, Qt::DotLine,
        Qt::DashDotLine, Qt::DashDotDotLine, Qt::SolidLine
    };

    painter.save();
    painter.setClipRect(plotRect().adjusted(-2, -2, 2, 2));

    for (int i = 0; i < 6; ++i) {
        const Neuron& n = hidden[i];
        if (!n.enabled || n.weights.size() < 2) continue;

        double w1 = n.weights[0];
        double w2 = n.weights[1];
        double b = n.bias;
        QVector<QPointF> points;

        if (std::abs(w2) > 1e-9) {
            double y1 = -(w1 * m_min + b) / w2;
            double y2 = -(w1 * m_max + b) / w2;
            points << toScreen(m_min, y1) << toScreen(m_max, y2);
        } else if (std::abs(w1) > 1e-9) {
            double x = -b / w1;
            points << toScreen(x, m_min) << toScreen(x, m_max);
        }

        if (points.size() == 2) {
            QPen linePen(Qt::black, i == 5 ? 1 : 2, styles[i]);
            painter.setPen(linePen);
            painter.drawLine(points[0], points[1]);
            painter.setPen(Qt::black);
            QPointF mid = (points[0] + points[1]) / 2.0;
            painter.drawText(mid + QPointF(6, -6), n.name);
        }
    }

    painter.restore();
}

void PlaneWidget::drawObject(QPainter& painter)
{
    EvaluationResult e = m_model->evaluate(m_point.x(), m_point.y());
    QPointF p = toScreen(m_point.x(), m_point.y());

    painter.save();
    painter.setPen(QPen(Qt::black, 2));
    if (m_model->preset() == Preset::TwoTriangles) {
        if (e.insideB) painter.setBrush(QColor(220, 0, 0));          // класс 2
        else if (e.insideA) painter.setBrush(QColor(0, 70, 170));    // класс 1
        else painter.setBrush(QColor(245, 245, 245));                // вне классов
    } else {
        painter.setBrush(e.finalClass ? QColor(220, 0, 0) : QColor(0, 70, 170));
    }
    painter.drawEllipse(p, 7, 7);

    painter.setPen(QPen(Qt::black, 2));
    painter.drawLine(p + QPointF(-10, 0), p + QPointF(10, 0));
    painter.drawLine(p + QPointF(0, -10), p + QPointF(0, 10));

    QString label = QString("Объект (%1; %2)")
                        .arg(QString::number(m_point.x(), 'f', 2), QString::number(m_point.y(), 'f', 2));
    QFont f = painter.font();
    f.setPointSize(10);
    f.setBold(true);
    painter.setFont(f);
    QFontMetrics fm(f);
    QRect textRect = fm.boundingRect(label).adjusted(-6, -4, 6, 4);
    textRect.moveTopLeft((p + QPointF(12, -28)).toPoint());
    painter.setPen(QPen(Qt::black, 1));
    painter.setBrush(Qt::white);
    painter.drawRoundedRect(textRect, 5, 5);
    painter.setPen(Qt::black);
    painter.drawText(textRect, Qt::AlignCenter, label);
    painter.restore();
}

void PlaneWidget::drawLegend(QPainter& painter)
{
    painter.save();

    const QColor class0Color(205, 225, 255);
    const QColor class1Color(255, 205, 205);
    const QColor outsideColor(245, 245, 245);

    const bool twoDifferentAreas = m_model && m_model->preset() == Preset::TwoTriangles;
    QRectF box(12, 12, twoDifferentAreas ? 260 : 170, twoDifferentAreas ? 98 : 74);
    painter.setPen(QPen(Qt::black, 1));
    painter.setBrush(QColor(255, 255, 255, 235));
    painter.drawRoundedRect(box, 8, 8);

    QFont titleFont = painter.font();
    titleFont.setPointSize(9);
    titleFont.setBold(true);
    painter.setFont(titleFont);
    painter.setPen(Qt::black);
    painter.drawText(QRectF(box.left() + 10, box.top() + 6, box.width() - 20, 16),
                     Qt::AlignLeft | Qt::AlignVCenter,
                     "Легенда");

    QFont textFont = painter.font();
    textFont.setPointSize(10);
    textFont.setBold(false);
    painter.setFont(textFont);

    const QRectF swatch0(box.left() + 12, box.top() + 28, 18, 18);
    const QRectF swatch1(box.left() + 12, box.top() + 50, 18, 18);
    const QRectF swatch2(box.left() + 12, box.top() + 72, 18, 18);

    painter.setPen(QPen(Qt::black, 1));
    painter.setBrush(class0Color);
    painter.drawRect(swatch0);
    painter.setBrush(class1Color);
    painter.drawRect(swatch1);

    painter.setPen(Qt::black);
    if (twoDifferentAreas) {
        painter.drawText(QPointF(swatch0.right() + 10, swatch0.bottom() - 4), "Класс 1: область A");
        painter.drawText(QPointF(swatch1.right() + 10, swatch1.bottom() - 4), "Класс 2: область B");
        painter.setBrush(outsideColor);
        painter.drawRect(swatch2);
        painter.drawText(QPointF(swatch2.right() + 10, swatch2.bottom() - 4), "Вне A и B");
    } else {
        painter.drawText(QPointF(swatch0.right() + 10, swatch0.bottom() - 4), "Класс 0");
        painter.drawText(QPointF(swatch1.right() + 10, swatch1.bottom() - 4), "Класс 1");
    }

    painter.restore();
}
