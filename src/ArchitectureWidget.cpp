#include "ArchitectureWidget.h"

#include <QPainter>
#include <algorithm>
#include <cmath>

ArchitectureWidget::ArchitectureWidget(QWidget* parent) : QWidget(parent)
{
    setMinimumHeight(220);
}

void ArchitectureWidget::setModel(NetworkModel* model)
{
    m_model = model;
    update();
}

void ArchitectureWidget::setEvaluation(const EvaluationResult& evaluation)
{
    m_eval = evaluation;
    update();
}

void ArchitectureWidget::setShowEvaluation(bool show)
{
    m_showEvaluation = show;
    update();
}

void ArchitectureWidget::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.fillRect(rect(), Qt::white);

    if (!m_model) return;

    EvaluationResult emptyEvaluation;
    const EvaluationResult& eval = m_showEvaluation ? m_eval : emptyEvaluation;

    const double xInput = 60;
    const double xH = width() * 0.34;
    const double xS = width() * 0.66;
    const double xO = width() - 65;

    QVector<QPointF> inputPos = {
        QPointF(xInput, height() * 0.38),
        QPointF(xInput, height() * 0.62)
    };

    QVector<QPointF> hPos;
    for (int i = 0; i < 6; ++i) {
        double y = 30 + i * ((height() - 60.0) / 5.0);
        hPos << QPointF(xH, y);
    }

    QVector<QPointF> sPos = {
        QPointF(xS, height() * 0.38),
        QPointF(xS, height() * 0.62)
    };
    QPointF oPos(xO, height() * 0.5);

    const auto& hidden = m_model->hiddenLayer();
    const auto& second = m_model->secondLayer();
    const auto& output = m_model->outputNeuron();

    // Input -> H
    for (int h = 0; h < 6; ++h) {
        for (const auto& in : inputPos) {
            drawConnection(painter, in, hPos[h], m_showEvaluation && eval.h[h] == 1, hidden[h].enabled);
        }
    }

    // H -> S
    for (int h = 0; h < 6; ++h) {
        for (int s = 0; s < 2; ++s) {
            bool enabled = second[s].enabled && hidden[h].enabled && h < static_cast<int>(second[s].weights.size()) && std::abs(second[s].weights[h]) > 1e-9;
            bool active = m_showEvaluation && enabled && eval.h[h] == 1 && eval.s[s] == 1;
            drawConnection(painter, hPos[h], sPos[s], active, enabled);
        }
    }

    // S -> O
    for (int s = 0; s < 2; ++s) {
        bool enabled = output.enabled && second[s].enabled && s < static_cast<int>(output.weights.size()) && std::abs(output.weights[s]) > 1e-9;
        bool active = m_showEvaluation && enabled && eval.s[s] == 1 && eval.o == 1;
        drawConnection(painter, sPos[s], oPos, active, enabled);
    }

    drawNode(painter, inputPos[0], "x1", true, true);
    drawNode(painter, inputPos[1], "x2", true, true);

    for (int i = 0; i < 6; ++i) {
        const QString text = m_showEvaluation
            ? QString("H%1\n%2").arg(i + 1).arg(eval.h[i])
            : QString("H%1").arg(i + 1);
        drawNode(painter, hPos[i], text, m_showEvaluation && eval.h[i] == 1, hidden[i].enabled);
    }

    for (int i = 0; i < 2; ++i) {
        const QString text = m_showEvaluation
            ? QString("S%1\n%2").arg(i + 1).arg(eval.s[i])
            : QString("S%1").arg(i + 1);
        drawNode(painter, sPos[i], text, m_showEvaluation && eval.s[i] == 1, second[i].enabled);
    }

    const QString outputText = m_showEvaluation
        ? QString("O1\n%1").arg(eval.finalClass)
        : QString("O1");
    drawNode(painter, oPos, outputText,
             m_showEvaluation && eval.finalClass == 1, output.enabled || !m_model->usesOutputLayer());

    painter.setPen(Qt::black);
    painter.drawText(QRectF(8, 4, width() - 16, 20), Qt::AlignLeft,
                     m_showEvaluation
                         ? "Активные нейроны и связи подсвечиваются для предъявленного объекта"
                         : "Схема сети. Подсветка появится после команды «Показать объект на плоскости»");
}

void ArchitectureWidget::drawConnection(QPainter& painter, const QPointF& from, const QPointF& to, bool active, bool enabled)
{
    if (!enabled) {
        painter.setPen(QPen(QColor(210, 210, 210), 1, Qt::DotLine));
    } else if (active) {
        painter.setPen(QPen(Qt::black, 3));
    } else {
        painter.setPen(QPen(QColor(120, 120, 120), 1));
    }
    painter.drawLine(from, to);
}

void ArchitectureWidget::drawNode(QPainter& painter, const QPointF& center, const QString& text, bool active, bool enabled)
{
    const double rx = 24;
    const double ry = 20;

    QColor fill = Qt::white;
    QColor border = Qt::black;
    if (!enabled) {
        fill = QColor(235, 235, 235);
        border = QColor(160, 160, 160);
    } else if (active) {
        fill = Qt::black;
        border = Qt::black;
    }

    painter.setPen(QPen(border, active ? 2 : 1));
    painter.setBrush(fill);
    painter.drawEllipse(center, rx, ry);

    painter.setPen(active && enabled ? Qt::white : (enabled ? Qt::black : QColor(120, 120, 120)));
    painter.drawText(QRectF(center.x() - rx, center.y() - ry, 2 * rx, 2 * ry), Qt::AlignCenter, text);
}
