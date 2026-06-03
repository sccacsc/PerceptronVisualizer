#pragma once

#include <QWidget>
#include <QPointF>

#include "NetworkModel.h"

class ArchitectureWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ArchitectureWidget(QWidget* parent = nullptr);

    void setModel(NetworkModel* model);
    void setEvaluation(const EvaluationResult& evaluation);
    void setShowEvaluation(bool show);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    NetworkModel* m_model = nullptr;
    EvaluationResult m_eval;
    bool m_showEvaluation = false;

    void drawConnection(QPainter& painter, const QPointF& from, const QPointF& to, bool active, bool enabled);
    void drawNode(QPainter& painter, const QPointF& center, const QString& text, bool active, bool enabled);
};
