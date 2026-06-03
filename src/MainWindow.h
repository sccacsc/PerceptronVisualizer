#pragma once

#include <QMainWindow>
#include <QPointF>

#include "NetworkModel.h"

class ArchitectureWidget;
class PlaneWidget;
class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QLabel;
class QPushButton;
class QTableWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

private slots:
    void onPresetChanged(int index);
    void onPointChanged();
    void onRecognizeObject();
    void onVisualizeObject();
    void onPlanePointSelected(double x1, double x2);
    void onRandomPoint();
    void onNeuronTableChanged(int row, int column);
    void updateView();

private:
    NetworkModel m_model;
    QPointF m_point;
    EvaluationResult m_eval;
    bool m_updatingTable = false;
    bool m_hasRecognition = false;
    bool m_objectVisualized = false;

    QComboBox* m_presetCombo = nullptr;
    QLabel* m_modeDescription = nullptr;
    QDoubleSpinBox* m_xSpin = nullptr;
    QDoubleSpinBox* m_ySpin = nullptr;
    QPushButton* m_randomButton = nullptr;
    QPushButton* m_recognizeButton = nullptr;
    QPushButton* m_visualizeButton = nullptr;
    QLabel* m_resultLabel = nullptr;
    QLabel* m_explanationLabel = nullptr;
    QLabel* m_classDescription = nullptr;
    QTableWidget* m_neuronTable = nullptr;
    QCheckBox* m_showAreas = nullptr;
    QCheckBox* m_showLines = nullptr;
    PlaneWidget* m_plane = nullptr;
    ArchitectureWidget* m_architecture = nullptr;

    QWidget* createControlPanel();
    void populatePresetCombo();
    void populateNeuronTable();
    void setPoint(double x1, double x2);
    QString formatEvaluationText(const EvaluationResult& result) const;
    QString formatRecognitionText(const EvaluationResult& result) const;
    void resetRecognition();
};
