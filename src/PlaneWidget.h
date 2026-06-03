#pragma once

#include <QPointF>
#include <QWidget>

#include "NetworkModel.h"

class PlaneWidget : public QWidget
{
    Q_OBJECT

public:
    explicit PlaneWidget(QWidget* parent = nullptr);

    void setModel(NetworkModel* model);
    void setObjectPoint(const QPointF& point);
    QPointF objectPoint() const;
    void setShowAreas(bool show);
    void setShowLines(bool show);
    void setShowObject(bool show);

signals:
    void pointSelected(double x1, double x2);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;

private:
    NetworkModel* m_model = nullptr;
    QPointF m_point = QPointF(0.0, 0.0);
    bool m_showAreas = true;
    bool m_showLines = true;
    bool m_showObject = false;
    const double m_min = -5.0;
    const double m_max = 5.0;

    QRectF plotRect() const;
    QPointF toScreen(double x1, double x2) const;
    QPointF fromScreen(const QPointF& p) const;

    void drawAreas(QPainter& painter);
    void drawGrid(QPainter& painter);
    void drawBoundaries(QPainter& painter);
    void drawObject(QPainter& painter);
    void drawLegend(QPainter& painter);
};
