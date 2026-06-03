#include "MainWindow.h"

#include "ArchitectureWidget.h"
#include "PlaneWidget.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFrame>
#include <QGridLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QRandomGenerator>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QSizePolicy>
#include <QSplitter>
#include <QStatusBar>
#include <QTableWidget>
#include <QTabWidget>
#include <QVBoxLayout>

namespace {
QString n(double value)
{
    return QString::number(value, 'f', 3);
}
}

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent)
{
    setWindowTitle("Демонастрационная нейронная сеть: построение областей классов и распознавание объекта");
    setMinimumSize(1180, 720);
    resize(1280, 780);

    QWidget* central = new QWidget(this);
    auto* rootLayout = new QHBoxLayout(central);

    QWidget* controls = createControlPanel();

    m_plane = new PlaneWidget(this);
    m_plane->setModel(&m_model);
    m_plane->setShowObject(false);

    m_architecture = new ArchitectureWidget(this);
    m_architecture->setModel(&m_model);
    m_architecture->setShowEvaluation(false);

    auto* right = new QWidget(this);
    auto* rightLayout = new QVBoxLayout(right);
    rightLayout->setContentsMargins(8, 8, 8, 8);
    rightLayout->setSpacing(8);
    rightLayout->addWidget(m_plane, 5);
    rightLayout->addWidget(m_architecture, 2);

    auto* splitter = new QSplitter(Qt::Horizontal, this);
    splitter->addWidget(controls);
    splitter->addWidget(right);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    splitter->setSizes({430, 850});

    rootLayout->addWidget(splitter);
    setCentralWidget(central);

    populatePresetCombo();
    populateNeuronTable();

    connect(m_presetCombo, &QComboBox::currentIndexChanged, this, &MainWindow::onPresetChanged);
    connect(m_xSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &MainWindow::onPointChanged);
    connect(m_ySpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &MainWindow::onPointChanged);
    connect(m_plane, &PlaneWidget::pointSelected, this, &MainWindow::onPlanePointSelected);
    connect(m_randomButton, &QPushButton::clicked, this, &MainWindow::onRandomPoint);
    connect(m_recognizeButton, &QPushButton::clicked, this, &MainWindow::onRecognizeObject);
    connect(m_visualizeButton, &QPushButton::clicked, this, &MainWindow::onVisualizeObject);
    connect(m_neuronTable, &QTableWidget::cellChanged, this, &MainWindow::onNeuronTableChanged);
    connect(m_showAreas, &QCheckBox::toggled, m_plane, &PlaneWidget::setShowAreas);
    connect(m_showLines, &QCheckBox::toggled, m_plane, &PlaneWidget::setShowLines);

    m_point = m_model.description().defaultPoint;
    setPoint(m_point.x(), m_point.y());
    resetRecognition();
    updateView();

    statusBar()->showMessage("Сначала постройте области классов, затем предъявите объект для распознавания");
}

QWidget* MainWindow::createControlPanel()
{
    auto* panel = new QWidget(this);
    panel->setMinimumWidth(420);
    panel->setMaximumWidth(500);
    panel->setStyleSheet(R"(
        QWidget { background: #ffffff; color: #000000; font-size: 13px; }
        QLabel { color: #000000; background: transparent; }
        QGroupBox {
            border: 1px solid #000000;
            margin-top: 14px;
            padding: 10px 8px 8px 8px;
            font-weight: bold;
            background: #ffffff;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 8px;
            padding: 0 4px;
            background: #ffffff;
        }
        QGroupBox QLabel { font-weight: normal; }
        QComboBox, QDoubleSpinBox, QPushButton, QTableWidget {
            background: #ffffff;
            color: #000000;
            border: 1px solid #000000;
            selection-background-color: #000000;
            selection-color: #ffffff;
        }
        QPushButton { padding: 6px 8px; font-weight: bold; }
        QPushButton:hover { background: #eeeeee; }
        QPushButton:disabled { color: #777777; border-color: #777777; background: #eeeeee; }
        QCheckBox { color: #000000; font-weight: normal; background: transparent; }
        QTabWidget::pane { border: 1px solid #000000; background: #ffffff; }
        QTabBar::tab {
            background: #ffffff;
            color: #000000;
            border: 1px solid #000000;
            padding: 7px 10px;
            min-width: 155px;
            font-weight: bold;
        }
        QTabBar::tab:selected { background: #000000; color: #ffffff; }
        QHeaderView::section {
            background: #ffffff;
            color: #000000;
            border: 1px solid #000000;
            font-weight: bold;
        }
        QScrollArea { border: 0; background: #ffffff; }
    )");

    auto* layout = new QVBoxLayout(panel);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(8);

    auto* title = new QLabel("Демонстрационная нейронная сеть", panel);
    title->setWordWrap(true);
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet("font-size: 17px; font-weight: bold; border: 2px solid #000; padding: 8px; background: #fff;");
    layout->addWidget(title);

    auto* tabs = new QTabWidget(panel);
    layout->addWidget(tabs, 1);

    // Вкладка 1. Построение областей классов.
    auto* buildTabContent = new QWidget(tabs);
    auto* buildLayout = new QVBoxLayout(buildTabContent);
    buildLayout->setContentsMargins(8, 8, 8, 8);
    buildLayout->setSpacing(8);

    auto* presetBox = new QGroupBox("1. Построение областей классов", buildTabContent);
    auto* presetLayout = new QVBoxLayout(presetBox);
    presetLayout->setSpacing(6);

    m_presetCombo = new QComboBox(presetBox);
    m_presetCombo->setMinimumHeight(30);

    m_modeDescription = new QLabel(presetBox);
    m_modeDescription->setWordWrap(true);
    m_modeDescription->setTextFormat(Qt::RichText);
    m_modeDescription->setMinimumHeight(130);
    m_modeDescription->setStyleSheet("font-size: 12px; color: #000; background: #fff; border: 1px solid #000; padding: 6px;");

    presetLayout->addWidget(new QLabel("Демонстрационный режим:", presetBox));
    presetLayout->addWidget(m_presetCombo);
    presetLayout->addWidget(m_modeDescription);
    buildLayout->addWidget(presetBox);

    auto* displayBox = new QGroupBox("2. Отображение построенных областей", buildTabContent);
    auto* displayLayout = new QVBoxLayout(displayBox);
    displayLayout->setSpacing(4);
    m_showAreas = new QCheckBox("Области классов", displayBox);
    m_showLines = new QCheckBox("Границы H-нейронов", displayBox);
    m_showAreas->setChecked(true);
    m_showLines->setChecked(true);
    displayLayout->addWidget(m_showAreas);
    displayLayout->addWidget(m_showLines);
    buildLayout->addWidget(displayBox);

    auto* tableBox = new QGroupBox("3. Параметры нейронов H1-H6", buildTabContent);
    auto* tableLayout = new QVBoxLayout(tableBox);
    tableLayout->setSpacing(6);
    auto* paramsHint = new QLabel(
        "Нейроны H задают граничные прямые. Изменение активности, весов w1, w2 и смещения b перестраивает области классов.",
        tableBox);
    paramsHint->setWordWrap(true);

    m_neuronTable = new QTableWidget(tableBox);
    m_neuronTable->setColumnCount(5);
    m_neuronTable->setHorizontalHeaderLabels({"Нейрон", "On", "w1", "w2", "b"});
    m_neuronTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_neuronTable->verticalHeader()->setVisible(false);
    m_neuronTable->setMinimumHeight(300);
    tableLayout->addWidget(paramsHint);
    tableLayout->addWidget(m_neuronTable, 1);
    buildLayout->addWidget(tableBox, 1);

    auto* buildScroll = new QScrollArea(tabs);
    buildScroll->setWidgetResizable(true);
    buildScroll->setFrameShape(QFrame::NoFrame);
    buildScroll->setWidget(buildTabContent);
    tabs->addTab(buildScroll, "Построение областей классов");

    // Вкладка 2. Распознавание объекта.
    auto* recognitionTabContent = new QWidget(tabs);
    auto* recognitionLayout = new QVBoxLayout(recognitionTabContent);
    recognitionLayout->setContentsMargins(8, 8, 8, 8);
    recognitionLayout->setSpacing(8);

    auto* classesBox = new QGroupBox("1. Построенные классы", recognitionTabContent);
    auto* classesLayout = new QVBoxLayout(classesBox);
    m_classDescription = new QLabel(classesBox);
    m_classDescription->setWordWrap(true);
    m_classDescription->setTextFormat(Qt::RichText);
    m_classDescription->setMinimumHeight(105);
    m_classDescription->setStyleSheet("font-size: 13px; background: #fff; color: #000; border: 1px solid #000; padding: 8px;");
    classesLayout->addWidget(m_classDescription);
    recognitionLayout->addWidget(classesBox);

    auto* objectBox = new QGroupBox("2. Предъявление объекта", recognitionTabContent);
    auto* objectLayout = new QGridLayout(objectBox);
    objectLayout->setHorizontalSpacing(8);
    objectLayout->setVerticalSpacing(6);
    m_xSpin = new QDoubleSpinBox(objectBox);
    m_ySpin = new QDoubleSpinBox(objectBox);
    for (QDoubleSpinBox* spin : {m_xSpin, m_ySpin}) {
        spin->setRange(-5.0, 5.0);
        spin->setDecimals(2);
        spin->setSingleStep(0.1);
        spin->setMinimumHeight(28);
    }
    m_randomButton = new QPushButton("Случайный объект", objectBox);
    m_recognizeButton = new QPushButton("Распознать", objectBox);
    objectLayout->addWidget(new QLabel("x1:"), 0, 0);
    objectLayout->addWidget(m_xSpin, 0, 1);
    objectLayout->addWidget(new QLabel("x2:"), 1, 0);
    objectLayout->addWidget(m_ySpin, 1, 1);
    objectLayout->addWidget(m_randomButton, 2, 0, 1, 2);
    objectLayout->addWidget(m_recognizeButton, 3, 0, 1, 2);
    recognitionLayout->addWidget(objectBox);

    auto* resultBox = new QGroupBox("3. Результат распознавания", recognitionTabContent);
    auto* resultLayout = new QVBoxLayout(resultBox);
    resultLayout->setSpacing(6);
    m_resultLabel = new QLabel(resultBox);
    m_resultLabel->setAlignment(Qt::AlignCenter);
    m_resultLabel->setWordWrap(true);
    m_resultLabel->setMinimumHeight(86);
    m_resultLabel->setStyleSheet("font-size: 18px; font-weight: bold; padding: 8px; border: 2px solid #000; background: #fff; color: #000;");
    m_visualizeButton = new QPushButton("Показать объект и активные нейроны", resultBox);
    m_visualizeButton->setEnabled(false);
    resultLayout->addWidget(m_resultLabel);
    resultLayout->addWidget(m_visualizeButton);
    recognitionLayout->addWidget(resultBox);

    auto* explanationBox = new QGroupBox("4. Пояснение распознавания", recognitionTabContent);
    auto* explanationLayout = new QVBoxLayout(explanationBox);
    explanationLayout->setSpacing(6);
    m_explanationLabel = new QLabel(explanationBox);
    m_explanationLabel->setWordWrap(true);
    m_explanationLabel->setTextFormat(Qt::RichText);
    m_explanationLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    m_explanationLabel->setMinimumHeight(230);
    m_explanationLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::MinimumExpanding);
    m_explanationLabel->setStyleSheet("font-size: 13px; background: #fff; color: #000; border: 1px solid #000; padding: 8px;");
    explanationLayout->addWidget(m_explanationLabel);
    recognitionLayout->addWidget(explanationBox, 1);

    auto* recognitionScroll = new QScrollArea(tabs);
    recognitionScroll->setWidgetResizable(true);
    recognitionScroll->setFrameShape(QFrame::NoFrame);
    recognitionScroll->setWidget(recognitionTabContent);
    tabs->addTab(recognitionScroll, "Распознавание объекта");

    return panel;
}

void MainWindow::populatePresetCombo()
{
    m_presetCombo->clear();
    m_presetCombo->addItems(m_model.presetNames());
}

void MainWindow::populateNeuronTable()
{
    m_updatingTable = true;
    const auto& hidden = m_model.hiddenLayer();
    m_neuronTable->setRowCount(6);

    for (int i = 0; i < 6; ++i) {
        const Neuron& neuron = hidden[i];
        auto* nameItem = new QTableWidgetItem(neuron.name);
        nameItem->setFlags(nameItem->flags() & ~Qt::ItemIsEditable);
        m_neuronTable->setItem(i, 0, nameItem);
        m_neuronTable->setItem(i, 1, new QTableWidgetItem(neuron.enabled ? "1" : "0"));
        m_neuronTable->setItem(i, 2, new QTableWidgetItem(neuron.weights.size() > 0 ? n(neuron.weights[0]) : "0"));
        m_neuronTable->setItem(i, 3, new QTableWidgetItem(neuron.weights.size() > 1 ? n(neuron.weights[1]) : "0"));
        m_neuronTable->setItem(i, 4, new QTableWidgetItem(n(neuron.bias)));
    }

    m_updatingTable = false;
}

void MainWindow::onPresetChanged(int index)
{
    if (index < 0) return;
    m_model.setPreset(static_cast<Preset>(index));
    m_point = m_model.description().defaultPoint;
    setPoint(m_point.x(), m_point.y());
    populateNeuronTable();
    resetRecognition();
    updateView();
}

void MainWindow::onPointChanged()
{
    m_point = QPointF(m_xSpin->value(), m_ySpin->value());
    resetRecognition();
    updateView();
}

void MainWindow::onPlanePointSelected(double x1, double x2)
{
    setPoint(x1, x2);
    resetRecognition();
    updateView();
}

void MainWindow::onRandomPoint()
{
    double x = QRandomGenerator::global()->bounded(1001) / 100.0 - 5.0;
    double y = QRandomGenerator::global()->bounded(1001) / 100.0 - 5.0;
    setPoint(x, y);
    resetRecognition();
    updateView();
}

void MainWindow::onRecognizeObject()
{
    m_point = QPointF(m_xSpin->value(), m_ySpin->value());
    m_hasRecognition = true;
    m_objectVisualized = false;
    if (m_visualizeButton) m_visualizeButton->setEnabled(true);
    if (m_plane) m_plane->setShowObject(false);
    updateView();
    statusBar()->showMessage("Объект распознан. Для проверки можно нажать «Показать объект и активные нейроны»");
}

void MainWindow::onVisualizeObject()
{
    if (!m_hasRecognition) return;
    m_objectVisualized = true;
    updateView();
    statusBar()->showMessage("Распознанный объект и активные нейроны показаны для проверки результата");
}

void MainWindow::onNeuronTableChanged(int row, int column)
{
    if (m_updatingTable || row < 0 || row >= 6 || column < 1) return;

    Neuron neuron = m_model.hiddenNeuron(row);
    QString value = m_neuronTable->item(row, column) ? m_neuronTable->item(row, column)->text().trimmed() : QString();

    bool ok = false;
    if (column == 1) {
        neuron.enabled = value != "0";
        ok = true;
    } else {
        double d = value.replace(',', '.').toDouble(&ok);
        if (!ok) return;
        if (neuron.weights.size() < 2) neuron.weights.resize(2, 0.0);
        if (column == 2) neuron.weights[0] = d;
        if (column == 3) neuron.weights[1] = d;
        if (column == 4) neuron.bias = d;
    }

    m_model.setHiddenNeuron(row, neuron);
    resetRecognition();
    updateView();
}

void MainWindow::setPoint(double x1, double x2)
{
    const QSignalBlocker bx(m_xSpin);
    const QSignalBlocker by(m_ySpin);
    m_xSpin->setValue(x1);
    m_ySpin->setValue(x2);
    m_point = QPointF(x1, x2);
}

void MainWindow::resetRecognition()
{
    m_hasRecognition = false;
    m_objectVisualized = false;
    if (m_visualizeButton) m_visualizeButton->setEnabled(false);
    if (m_plane) {
        m_plane->setObjectPoint(m_point);
        m_plane->setShowObject(false);
        m_plane->update();
    }
    if (m_architecture) {
        EvaluationResult empty;
        m_architecture->setEvaluation(empty);
        m_architecture->setShowEvaluation(false);
    }
}

void MainWindow::updateView()
{
    const PresetDescription d = m_model.description();

    QString constructionText = "<b>" + d.title + "</b><br>" + d.description
        + "<br><br><b>Построение областей классов:</b> слой H задаёт граничные прямые, "
          "слой S формирует области, а выходной слой определяет итоговую принадлежность."
        + "<br><br><b>Логика:</b> " + m_model.modeLogicText();
    m_modeDescription->setText(constructionText);

    if (m_classDescription) {
        QString classesText;
        if (m_model.preset() == Preset::TwoTriangles) {
            classesText = "<b>Выбранный режим:</b> " + d.title +
                "<br><b>Класс 1:</b> область A" 
                "<br><b>Класс 2:</b> область B"
                "<br><b>Третий случай:</b> объект вне областей A и B не принадлежит ни одному из заданных классов.";
        } else {
            classesText = "<b>Выбранный режим:</b> " + d.title +
                "<br><b>Класс 1:</b> заданная область, сформированная сетью"
                "<br><b>Класс 0:</b> область вне заданного класса.";
        }
        m_classDescription->setText(classesText);
    }

    m_plane->setObjectPoint(m_point);
    m_plane->setShowObject(m_hasRecognition && m_objectVisualized);
    m_plane->update();

    if (!m_hasRecognition) {
        EvaluationResult empty;
        m_architecture->setEvaluation(empty);
        m_architecture->setShowEvaluation(false);
        m_resultLabel->setStyleSheet("font-size: 18px; font-weight: bold; padding: 8px; border: 2px solid #000; background: #fff; color: #000;");
        m_resultLabel->setText("Введите координаты и нажмите «Распознать»");
        m_explanationLabel->setText("<p><b>Порядок работы:</b></p>"
                                   "<ol>"
                                   "<li>Во вкладке «Построение областей классов» выберите режим и проверьте построенные области.</li>"
                                   "<li>Во вкладке «Распознавание объекта» введите координаты предъявляемого объекта.</li>"
                                   "<li>Нажмите «Распознать» и получите результат принадлежности к классу.</li>"
                                   "<li>После распознавания можно показать объект на плоскости и подсветку активных нейронов для проверки результата.</li>"
                                   "</ol>");
        return;
    }

    m_eval = m_model.evaluate(m_point.x(), m_point.y());
    if (m_objectVisualized) {
        m_architecture->setEvaluation(m_eval);
        m_architecture->setShowEvaluation(true);
    } else {
        EvaluationResult empty;
        m_architecture->setEvaluation(empty);
        m_architecture->setShowEvaluation(false);
    }

    if (m_model.preset() == Preset::TwoTriangles && !m_eval.inDemoArea) {
        m_resultLabel->setStyleSheet("font-size: 18px; font-weight: bold; padding: 8px; border: 2px solid #000; background: #eeeeee; color: #000;");
    } else if (m_model.preset() == Preset::TwoTriangles && m_eval.insideA) {
        m_resultLabel->setStyleSheet("font-size: 18px; font-weight: bold; padding: 8px; border: 2px solid #000; background: #fff; color: #000;");
    } else if (m_eval.finalClass) {
        m_resultLabel->setStyleSheet("font-size: 18px; font-weight: bold; padding: 8px; border: 2px solid #000; background: #000; color: #fff;");
    } else {
        m_resultLabel->setStyleSheet("font-size: 18px; font-weight: bold; padding: 8px; border: 2px solid #000; background: #fff; color: #000;");
    }

    m_resultLabel->setText(formatRecognitionText(m_eval));
    m_explanationLabel->setText(formatEvaluationText(m_eval));
}

QString MainWindow::formatRecognitionText(const EvaluationResult& result) const
{
    if (m_model.preset() == Preset::TwoTriangles) {
        if (result.insideA && result.insideB) {
            return "Неоднозначное распознавание\nобъект попал в области классов 1 и 2";
        }
        if (result.insideA) {
            return "Объект принадлежит классу 1\nобласть A";
        }
        if (result.insideB) {
            return "Объект принадлежит классу 2\nобласть B";
        }
        return "Объект не принадлежит\nни одному из заданных классов";
    }

    return result.resultText;
}

QString MainWindow::formatEvaluationText(const EvaluationResult& result) const
{
    QString html;
    html += QString("<p><b>Координаты предъявленного объекта:</b> x1 = %1; x2 = %2</p>")
                .arg(QString::number(m_point.x(), 'f', 2), QString::number(m_point.y(), 'f', 2));

    if (m_model.preset() == Preset::TwoTriangles) {
        html += "<p><b>Интерпретация режима распознавания:</b><br>"
                "область A объявлена классом 1, область B объявлена классом 2. "
                "Если объект не попал ни в A, ни в B, он не принадлежит ни одному из заданных классов.</p>";
    }

    html += "<p><b>Пояснение:</b><br>" + result.explanation + "</p>";
    html += "<p><b>Пошаговое прохождение сигнала:</b></p><ol>";
    for (const QString& step : result.steps) html += "<li>" + step.toHtmlEscaped() + "</li>";
    html += "</ol>";
    html += "<p><b>Уравнения активных H-нейронов:</b></p><ul>";
    for (int i = 0; i < 6; ++i) {
        if (m_model.hiddenLayer()[i].enabled) html += "<li>" + m_model.hiddenEquation(i).toHtmlEscaped() + "</li>";
    }
    html += "</ul>";
    return html;
}
