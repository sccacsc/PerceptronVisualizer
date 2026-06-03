#pragma once

#include <array>
#include <vector>

#include <QPointF>
#include <QString>
#include <QStringList>

struct Neuron
{
    bool enabled = false;
    std::vector<double> weights;
    double bias = 0.0;
    QString name;

    explicit Neuron(int inputCount = 0, const QString& neuronName = QString());

    double net(const std::vector<double>& input) const;
    int activate(const std::vector<double>& input) const;
};

enum class Preset
{
    SingleNeuron = 0,
    TwoLayerTwoLines,
    TwoLayerTriangle,
    TwoLayerQuadrilateral,
    TwoTriangles,
    BNotA,
    AOrB,
    AAndB,
    ANotB
};

struct EvaluationResult
{
    std::array<double, 6> hNet{};
    std::array<int, 6> h{};
    std::array<double, 2> sNet{};
    std::array<int, 2> s{};
    double oNet = 0.0;
    int o = 0;
    int finalClass = 0;
    bool insideA = false;
    bool insideB = false;
    bool inDemoArea = true;       // false для режима распознавания, если точка вне A и B
    int demoClass = -1;           // -2 — A и B, -1 — вне классов, 1 — класс A, 2 — класс B
    QString resultText;
    QString explanation;
    QStringList steps;
};

struct PresetDescription
{
    QString title;
    QString description;
    QPointF defaultPoint;
};

class NetworkModel
{
public:
    NetworkModel();

    QStringList presetNames() const;
    void setPreset(Preset preset);
    Preset preset() const;
    PresetDescription description() const;

    EvaluationResult evaluate(double x1, double x2) const;

    const std::array<Neuron, 6>& hiddenLayer() const;
    const std::array<Neuron, 2>& secondLayer() const;
    const Neuron& outputNeuron() const;

    int enabledHiddenCount() const;
    bool usesSecondLayer() const;
    bool usesOutputLayer() const;
    bool usesAreaB() const;

    void setHiddenNeuron(int index, const Neuron& neuron);
    Neuron hiddenNeuron(int index) const;

    QString hiddenEquation(int index) const;
    QString modeLogicText() const;

private:
    Preset m_preset = Preset::SingleNeuron;
    std::array<Neuron, 6> m_hidden;
    std::array<Neuron, 2> m_second;
    Neuron m_output;
    PresetDescription m_description;

    void clearNetwork();
    void configureSingleNeuron();
    void configureTwoLayerTwoLines();
    void configureTwoLayerTriangle();
    void configureTwoLayerQuadrilateral();
    void configureTwoTriangles();
    void configureBNotA();
    void configureAOrB();
    void configureAAndB();
    void configureANotB();

    std::vector<double> hiddenInput(double x1, double x2) const;
    std::vector<double> secondInput(const std::array<int, 2>& s) const;
};
