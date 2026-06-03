#include "NetworkModel.h"

#include <cmath>
#include <sstream>

namespace {
QString f(double v)
{
    QString s = QString::number(v, 'f', 2);
    while (s.contains('.') && s.endsWith('0')) s.chop(1);
    if (s.endsWith('.')) s.chop(1);
    return s;
}

QString signedTerm(double value, const QString& variable, bool first)
{
    if (std::abs(value) < 1e-9) return QString();
    QString sign;
    if (value >= 0) sign = first ? "" : " + ";
    else sign = first ? "-" : " - ";

    double a = std::abs(value);
    if (std::abs(a - 1.0) < 1e-9) return sign + variable;
    return sign + f(a) + "·" + variable;
}
}

Neuron::Neuron(int inputCount, const QString& neuronName)
    : enabled(false), weights(static_cast<size_t>(inputCount), 0.0), bias(0.0), name(neuronName)
{
}

double Neuron::net(const std::vector<double>& input) const
{
    if (!enabled) return -1000000.0;

    double value = bias;
    const size_t n = std::min(weights.size(), input.size());
    for (size_t i = 0; i < n; ++i) value += weights[i] * input[i];
    return value;
}

int Neuron::activate(const std::vector<double>& input) const
{
    return net(input) >= 0.0 ? 1 : 0;
}

NetworkModel::NetworkModel()
    : m_hidden{Neuron(2, "H1"), Neuron(2, "H2"), Neuron(2, "H3"),
               Neuron(2, "H4"), Neuron(2, "H5"), Neuron(2, "H6")},
      m_second{Neuron(6, "S1"), Neuron(6, "S2")},
      m_output(2, "O1")
{
    setPreset(Preset::SingleNeuron);
}

QStringList NetworkModel::presetNames() const
{
    return {
        "1 нейрон",
        "2 слоя: 2 → 1",
        "2 слоя: 3 → 1, треугольник",
        "2 слоя: 4 → 1, четырёхугольник",
        "3 слоя: две области A и B",
        "3 слоя: B и не A",
        "3 слоя: A или B",
        "3 слоя: A и B",
        "3 слоя: A и не B"
    };
}

void NetworkModel::setPreset(Preset preset)
{
    m_preset = preset;
    switch (preset) {
    case Preset::SingleNeuron: configureSingleNeuron(); break;
    case Preset::TwoLayerTwoLines: configureTwoLayerTwoLines(); break;
    case Preset::TwoLayerTriangle: configureTwoLayerTriangle(); break;
    case Preset::TwoLayerQuadrilateral: configureTwoLayerQuadrilateral(); break;
    case Preset::TwoTriangles: configureTwoTriangles(); break;
    case Preset::BNotA: configureBNotA(); break;
    case Preset::AOrB: configureAOrB(); break;
    case Preset::AAndB: configureAAndB(); break;
    case Preset::ANotB: configureANotB(); break;
    }
}

Preset NetworkModel::preset() const
{
    return m_preset;
}

PresetDescription NetworkModel::description() const
{
    return m_description;
}

void NetworkModel::clearNetwork()
{
    for (int i = 0; i < 6; ++i) {
        m_hidden[i] = Neuron(2, QString("H%1").arg(i + 1));
    }
    for (int i = 0; i < 2; ++i) {
        m_second[i] = Neuron(6, QString("S%1").arg(i + 1));
    }
    m_output = Neuron(2, "O1");
}

void NetworkModel::configureSingleNeuron()
{
    clearNetwork();
    m_hidden[0].enabled = true;
    m_hidden[0].weights = {1.0, 1.0};
    m_hidden[0].bias = 0.0;

    m_description = {
        "Один нейрон",
        "Первый слой содержит один активный пороговый нейрон H1. Он задаёт одну прямую и делит плоскость на две полуплоскости. Класс 1 соответствует активной полуплоскости.",
        QPointF(1.5, 1.0)
    };
}

void NetworkModel::configureTwoLayerTwoLines()
{
    clearNetwork();
    m_hidden[0].enabled = true;
    m_hidden[0].weights = {1.0, 0.0};
    m_hidden[0].bias = 1.5;       // x >= -1.5

    m_hidden[1].enabled = true;
    m_hidden[1].weights = {0.0, 1.0};
    m_hidden[1].bias = 1.5;       // y >= -1.5

    m_second[0].enabled = true;
    m_second[0].weights = {1.0, 1.0, 0.0, 0.0, 0.0, 0.0};
    m_second[0].bias = -1.5;      // H1 AND H2

    m_description = {
        "Две полуплоскости",
        "Нейроны H1 и H2 задают две полуплоскости. Нейрон S1 второго слоя активируется только тогда, когда объект удовлетворяет обоим условиям одновременно.",
        QPointF(0.5, 0.5)
    };
}

void NetworkModel::configureTwoLayerTriangle()
{
    clearNetwork();
    m_hidden[0].enabled = true;
    m_hidden[0].weights = {1.0, 0.0};
    m_hidden[0].bias = 2.5;       // x >= -2.5

    m_hidden[1].enabled = true;
    m_hidden[1].weights = {0.0, 1.0};
    m_hidden[1].bias = 2.5;       // y >= -2.5

    m_hidden[2].enabled = true;
    m_hidden[2].weights = {-1.0, -1.0};
    m_hidden[2].bias = 2.5;       // x + y <= 2.5

    m_second[0].enabled = true;
    m_second[0].weights = {1.0, 1.0, 1.0, 0.0, 0.0, 0.0};
    m_second[0].bias = -2.5;      // H1 AND H2 AND H3

    m_description = {
        "Треугольная область A",
        "Три нейрона первого слоя задают три стороны треугольника. S1 активируется только внутри пересечения трёх полуплоскостей. Класс 1 соответствует попаданию в треугольник A.",
        QPointF(0.0, 0.0)
    };
}


void NetworkModel::configureTwoLayerQuadrilateral()
{
    clearNetwork();
    // Rectangle-like convex area A: -2.5 <= x <= 2.5, -1.8 <= y <= 2.2
    m_hidden[0].enabled = true;
    m_hidden[0].weights = {1.0, 0.0};
    m_hidden[0].bias = 2.5;       // x >= -2.5

    m_hidden[1].enabled = true;
    m_hidden[1].weights = {-1.0, 0.0};
    m_hidden[1].bias = 2.5;       // x <= 2.5

    m_hidden[2].enabled = true;
    m_hidden[2].weights = {0.0, 1.0};
    m_hidden[2].bias = 1.8;       // y >= -1.8

    m_hidden[3].enabled = true;
    m_hidden[3].weights = {0.0, -1.0};
    m_hidden[3].bias = 2.2;       // y <= 2.2

    m_second[0].enabled = true;
    m_second[0].weights = {1.0, 1.0, 1.0, 1.0, 0.0, 0.0};
    m_second[0].bias = -3.5;      // H1 AND H2 AND H3 AND H4

    m_description = {
        "Четырёхугольная область A",
        "Четыре нейрона первого слоя задают четыре стороны выпуклого многоугольника. Нейрон S1 активируется только тогда, когда объект находится внутри всех четырёх полуплоскостей.",
        QPointF(0.6, 0.6)
    };
}

void NetworkModel::configureTwoTriangles()
{
    clearNetwork();
    // A: left triangle
    m_hidden[0].enabled = true;
    m_hidden[0].weights = {1.0, 0.0};
    m_hidden[0].bias = 4.0;       // x >= -4

    m_hidden[1].enabled = true;
    m_hidden[1].weights = {0.0, 1.0};
    m_hidden[1].bias = 2.5;       // y >= -2.5

    m_hidden[2].enabled = true;
    m_hidden[2].weights = {-1.0, -1.0};
    m_hidden[2].bias = -0.5;      // x + y <= -0.5

    // B: right triangle
    m_hidden[3].enabled = true;
    m_hidden[3].weights = {-1.0, 0.0};
    m_hidden[3].bias = 4.0;       // x <= 4

    m_hidden[4].enabled = true;
    m_hidden[4].weights = {0.0, -1.0};
    m_hidden[4].bias = 3.0;       // y <= 3

    m_hidden[5].enabled = true;
    m_hidden[5].weights = {1.0, 1.0};
    m_hidden[5].bias = -0.5;      // x + y >= 0.5

    m_second[0].enabled = true;
    m_second[0].weights = {1.0, 1.0, 1.0, 0.0, 0.0, 0.0};
    m_second[0].bias = -2.5;      // A

    m_second[1].enabled = true;
    m_second[1].weights = {0.0, 0.0, 0.0, 1.0, 1.0, 1.0};
    m_second[1].bias = -2.5;      // B

    m_output.enabled = true;
    m_output.weights = {1.0, 1.0};
    m_output.bias = -0.5;         // O1 = 1, если объект попал хотя бы в один из заданных классов

    m_description = {
        "Две области A и B",
        "Трёхслойная сеть формирует две области A и B. Нейроны H1-H3 задают область A, а H4-H6 задают область B. В режиме распознавания область A интерпретируется как класс 1, область B — как класс 2. Если объект не попал ни в A, ни в B, он не принадлежит ни одному из заданных классов.",
        QPointF(2.0, 1.0)
    };
}

void NetworkModel::configureBNotA()
{
    clearNetwork();
    // A: central triangle
    m_hidden[0].enabled = true;
    m_hidden[0].weights = {1.0, 0.0};
    m_hidden[0].bias = 2.5;       // x >= -2.5

    m_hidden[1].enabled = true;
    m_hidden[1].weights = {0.0, 1.0};
    m_hidden[1].bias = 2.5;       // y >= -2.5

    m_hidden[2].enabled = true;
    m_hidden[2].weights = {-1.0, -1.0};
    m_hidden[2].bias = 2.5;       // x+y <= 2.5

    // B: larger overlapping triangle
    m_hidden[3].enabled = true;
    m_hidden[3].weights = {-1.0, 0.0};
    m_hidden[3].bias = 3.8;       // x <= 3.8

    m_hidden[4].enabled = true;
    m_hidden[4].weights = {0.0, -1.0};
    m_hidden[4].bias = 3.8;       // y <= 3.8

    m_hidden[5].enabled = true;
    m_hidden[5].weights = {1.0, 1.0};
    m_hidden[5].bias = 2.0;       // x+y >= -2

    m_second[0].enabled = true;
    m_second[0].weights = {1.0, 1.0, 1.0, 0.0, 0.0, 0.0};
    m_second[0].bias = -2.5;      // A

    m_second[1].enabled = true;
    m_second[1].weights = {0.0, 0.0, 0.0, 1.0, 1.0, 1.0};
    m_second[1].bias = -2.5;      // B

    m_output.enabled = true;
    m_output.weights = {-1.0, 1.0};
    m_output.bias = -0.5;         // B AND NOT A

    m_description = {
        "Область B и не A",
        "Третий слой реализует логическую операцию B AND NOT A. Класс 1 получает объект, который попал в область B, но не попал в область A. Это показывает, как из выпуклых областей можно получить составную невыпуклую область.",
        QPointF(2.5, 1.5)
    };
}


void NetworkModel::configureAOrB()
{
    configureBNotA();
    m_preset = Preset::AOrB;
    m_output.enabled = true;
    m_output.weights = {1.0, 1.0};
    m_output.bias = -0.5;         // A OR B
    m_description = {
        "Область A или B",
        "Второй слой выделяет две треугольные области A и B. Выходной нейрон реализует операцию OR: объект получает класс 1, если попал хотя бы в одну из областей.",
        QPointF(0.5, 0.5)
    };
}

void NetworkModel::configureAAndB()
{
    configureBNotA();
    m_preset = Preset::AAndB;
    m_output.enabled = true;
    m_output.weights = {1.0, 1.0};
    m_output.bias = -1.5;         // A AND B
    m_description = {
        "Область A и B",
        "Второй слой выделяет области A и B, а выходной нейрон активируется только в их пересечении. Этот режим демонстрирует классификацию объекта по общей части двух областей.",
        QPointF(0.0, 0.0)
    };
}

void NetworkModel::configureANotB()
{
    configureBNotA();
    m_preset = Preset::ANotB;
    m_output.enabled = true;
    m_output.weights = {1.0, -1.0};
    m_output.bias = -0.5;         // A AND NOT B
    m_description = {
        "Область A и не B",
        "Выходной нейрон реализует операцию A AND NOT B. Объект получает класс 1, если попал в область A, но не попал в область B.",
        QPointF(-1.5, -1.0)
    };
}

EvaluationResult NetworkModel::evaluate(double x1, double x2) const
{
    EvaluationResult r;
    std::vector<double> input = {x1, x2};

    for (int i = 0; i < 6; ++i) {
        r.hNet[i] = m_hidden[i].net(input);
        r.h[i] = m_hidden[i].activate(input);
        if (m_hidden[i].enabled) {
            r.steps << QString("%1: net = %2, выход = %3")
                           .arg(m_hidden[i].name, f(r.hNet[i]))
                           .arg(r.h[i]);
        }
    }

    std::vector<double> hInput = hiddenInput(x1, x2);
    for (int i = 0; i < 2; ++i) {
        r.sNet[i] = m_second[i].net(hInput);
        r.s[i] = m_second[i].activate(hInput);
        if (m_second[i].enabled) {
            r.steps << QString("%1: net = %2, выход = %3")
                           .arg(m_second[i].name, f(r.sNet[i]))
                           .arg(r.s[i]);
        }
    }

    r.insideA = m_second[0].enabled && r.s[0] == 1;
    r.insideB = m_second[1].enabled && r.s[1] == 1;

    if (m_output.enabled) {
        std::vector<double> sInput = secondInput(r.s);
        r.oNet = m_output.net(sInput);
        r.o = m_output.activate(sInput);
        r.finalClass = r.o;
        r.steps << QString("O1: net = %1, выход = %2").arg(f(r.oNet)).arg(r.o);
    } else if (m_second[0].enabled) {
        r.oNet = r.sNet[0];
        r.o = r.s[0];
        r.finalClass = r.s[0];
    } else {
        r.oNet = r.hNet[0];
        r.o = r.h[0];
        r.finalClass = r.h[0];
    }

    r.inDemoArea = true;
    r.demoClass = r.finalClass;

    if (m_preset == Preset::TwoTriangles) {
        // В режиме распознавания две промежуточные области объявлены классами:
        // S1 = область A = класс 1, S2 = область B = класс 2.
        // O1 показывает сам факт принадлежности одному из заданных классов.
        if (r.insideA && r.insideB) {
            r.finalClass = 1;
            r.o = 1;
            r.inDemoArea = true;
            r.demoClass = -2;     // неоднозначность: объект попал в обе области
        } else if (r.insideA) {
            r.finalClass = 1;
            r.o = 1;
            r.inDemoArea = true;
            r.demoClass = 1;
        } else if (r.insideB) {
            r.finalClass = 1;
            r.o = 1;
            r.inDemoArea = true;
            r.demoClass = 2;
        } else {
            r.finalClass = 0;
            r.o = 0;
            r.inDemoArea = false;
            r.demoClass = -1;     // не принадлежит ни одному классу
        }
    }

    switch (m_preset) {
    case Preset::SingleNeuron:
        r.resultText = r.finalClass ? "Класс 1: активная полуплоскость" : "Класс 0: неактивная полуплоскость";
        r.explanation = r.finalClass
            ? "Объект находится в полуплоскости, для которой нейрон H1 выдаёт 1. Поэтому объект отнесён к классу 1."
            : "Объект находится по другую сторону разделяющей прямой H1. Нейрон H1 выдаёт 0, поэтому объект отнесён к классу 0.";
        break;
    case Preset::TwoLayerTwoLines:
        r.resultText = r.finalClass ? "Класс 1: выполнены оба условия" : "Класс 0: выполнены не все условия";
        r.explanation = r.finalClass
            ? "Объект удовлетворяет условиям H1 и H2. Второй слой выполняет операцию AND, поэтому S1 = 1 и итоговый класс равен 1."
            : "Для попадания в область нужно, чтобы H1 = 1 и H2 = 1. Хотя одно из условий может выполняться, полного пересечения нет, поэтому S1 = 0 и итоговый класс равен 0.";
        break;
    case Preset::TwoLayerTriangle:
        r.resultText = r.finalClass ? "Класс 1: внутри треугольника A" : "Класс 0: вне треугольника A";
        r.explanation = r.finalClass
            ? "Объект попал внутрь треугольной области A: H1 = H2 = H3 = 1. Нейрон S1 активирован, поэтому объект относится к классу 1."
            : "Объект не попал в треугольник A. По крайней мере одно из условий H1-H3 не выполнено, поэтому S1 = 0 и объект относится к классу 0.";
        break;
    case Preset::TwoLayerQuadrilateral:
        r.resultText = r.finalClass ? "Класс 1: внутри четырёхугольника A" : "Класс 0: вне четырёхугольника A";
        r.explanation = r.finalClass
            ? "Объект попал внутрь четырёхугольной области A: H1 = H2 = H3 = H4 = 1. Нейрон S1 активирован, поэтому объект относится к классу 1."
            : "Объект не попал в четырёхугольник A. Для принадлежности области нужно выполнить все четыре условия H1-H4 одновременно.";
        break;
    case Preset::TwoTriangles:
        if (r.insideA && r.insideB) {
            r.resultText = "Неоднозначно: объект в A и B";
            r.explanation = "Объект попал одновременно в область A и область B. В режиме распознавания это означает неоднозначность, так как A объявлена классом 1, а B объявлена классом 2. В исходной демонстрационной конфигурации эти области не пересекаются.";
        } else if (r.insideA) {
            r.resultText = "Класс 1: область A";
            r.explanation = "Объект попал в область A. Нейроны H1-H3 активировали S1. В режиме распознавания область A объявлена классом 1, поэтому объект распознан как объект класса 1.";
        } else if (r.insideB) {
            r.resultText = "Класс 2: область B";
            r.explanation = "Объект попал в область B. Нейроны H4-H6 активировали S2. В режиме распознавания область B объявлена классом 2, поэтому объект распознан как объект класса 2.";
        } else {
            r.resultText = "Не принадлежит ни одному классу";
            r.explanation = "Объект не попал ни в область A, ни в область B. Поэтому в режиме распознавания он не относится ни к классу 1, ни к классу 2. Визуально эта зона показывается нейтральной заливкой.";
        }
        break;
    case Preset::BNotA:
        r.resultText = r.finalClass ? "Класс 1: B и не A" : "Класс 0";
        r.explanation = r.finalClass
            ? "Объект попал в область B, но не попал в область A. Условие B AND NOT A выполнено, поэтому выходной нейрон O1 выдаёт класс 1."
            : "Условие B AND NOT A не выполнено. Объект либо не попал в B, либо одновременно попал в A, поэтому итоговый класс равен 0.";
        break;
    case Preset::AOrB:
        r.resultText = r.finalClass ? "Класс 1: A или B" : "Класс 0: вне A и B";
        r.explanation = r.finalClass
            ? "Операция OR выполнена: объект попал хотя бы в одну из областей A или B. Поэтому выходной нейрон O1 активирован."
            : "Объект не попал ни в A, ни в B. Операция OR не выполнена, поэтому итоговый класс равен 0.";
        break;
    case Preset::AAndB:
        r.resultText = r.finalClass ? "Класс 1: пересечение A и B" : "Класс 0: не в пересечении";
        r.explanation = r.finalClass
            ? "Объект одновременно попал в область A и в область B. Операция AND выполнена, поэтому выходной нейрон O1 выдаёт класс 1."
            : "Для класса 1 объект должен одновременно находиться в A и B. Одно из условий не выполнено, поэтому итоговый класс равен 0.";
        break;
    case Preset::ANotB:
        r.resultText = r.finalClass ? "Класс 1: A и не B" : "Класс 0";
        r.explanation = r.finalClass
            ? "Объект попал в область A, но не попал в область B. Условие A AND NOT B выполнено."
            : "Условие A AND NOT B не выполнено: объект либо не попал в A, либо одновременно попал в B.";
        break;
    }

    return r;
}

const std::array<Neuron, 6>& NetworkModel::hiddenLayer() const
{
    return m_hidden;
}

const std::array<Neuron, 2>& NetworkModel::secondLayer() const
{
    return m_second;
}

const Neuron& NetworkModel::outputNeuron() const
{
    return m_output;
}

int NetworkModel::enabledHiddenCount() const
{
    int count = 0;
    for (const auto& n : m_hidden) if (n.enabled) ++count;
    return count;
}

bool NetworkModel::usesSecondLayer() const
{
    return m_second[0].enabled || m_second[1].enabled;
}

bool NetworkModel::usesOutputLayer() const
{
    return m_output.enabled;
}

bool NetworkModel::usesAreaB() const
{
    return m_second[1].enabled;
}

void NetworkModel::setHiddenNeuron(int index, const Neuron& neuron)
{
    if (index >= 0 && index < 6) m_hidden[index] = neuron;
}

Neuron NetworkModel::hiddenNeuron(int index) const
{
    if (index >= 0 && index < 6) return m_hidden[index];
    return Neuron(2);
}

QString NetworkModel::hiddenEquation(int index) const
{
    if (index < 0 || index >= 6) return QString();
    const auto& n = m_hidden[index];
    if (!n.enabled) return n.name + ": выключен";

    QString equation;
    bool first = true;
    QString t1 = signedTerm(n.weights.size() > 0 ? n.weights[0] : 0.0, "x1", first);
    if (!t1.isEmpty()) { equation += t1; first = false; }
    QString t2 = signedTerm(n.weights.size() > 1 ? n.weights[1] : 0.0, "x2", first);
    if (!t2.isEmpty()) { equation += t2; first = false; }
    if (std::abs(n.bias) > 1e-9) {
        equation += (n.bias >= 0.0 ? (first ? "" : " + ") : (first ? "-" : " - "));
        equation += f(std::abs(n.bias));
        first = false;
    }
    if (equation.isEmpty()) equation = "0";
    return n.name + ": " + equation + " ≥ 0";
}

QString NetworkModel::modeLogicText() const
{
    switch (m_preset) {
    case Preset::SingleNeuron:
        return "Класс = H1";
    case Preset::TwoLayerTwoLines:
        return "S1 = H1 AND H2; класс = S1";
    case Preset::TwoLayerTriangle:
        return "S1 = H1 AND H2 AND H3; класс = S1";
    case Preset::TwoLayerQuadrilateral:
        return "S1 = H1 AND H2 AND H3 AND H4; класс = S1";
    case Preset::TwoTriangles:
        return "S1 = A = класс 1; S2 = B = класс 2; O1 = A OR B; вне A и B → не принадлежит ни одному классу";
    case Preset::BNotA:
        return "S1 = A; S2 = B; O1 = B AND NOT A";
    case Preset::AOrB:
        return "S1 = A; S2 = B; O1 = A OR B";
    case Preset::AAndB:
        return "S1 = A; S2 = B; O1 = A AND B";
    case Preset::ANotB:
        return "S1 = A; S2 = B; O1 = A AND NOT B";
    }
    return QString();
}

std::vector<double> NetworkModel::hiddenInput(double x1, double x2) const
{
    std::vector<double> input = {x1, x2};
    std::vector<double> result;
    result.reserve(6);
    for (const auto& h : m_hidden) result.push_back(h.activate(input));
    return result;
}

std::vector<double> NetworkModel::secondInput(const std::array<int, 2>& s) const
{
    return {static_cast<double>(s[0]), static_cast<double>(s[1])};
}
