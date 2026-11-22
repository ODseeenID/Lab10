#pragma once

#include <memory>
#include <string>

// вхідні параметри обчислення пріоритету
struct PriorityInput
{
    double importance = 1;    
    double urgency = 1;       
    double waitingHours = 0;  
};

// спільний інтерфейс алгоритмів
class IPriorityStrategy
{
public:
    virtual ~IPriorityStrategy() = default;

    virtual double calculate(const PriorityInput& input) const = 0;
    virtual std::string name() const = 0;
};

// максимум із значень параметрів
class MaxPriorityStrategy final : public IPriorityStrategy
{
public:
    double calculate(const PriorityInput& input) const override;
    std::string name() const override { return "max"; }
};

// зважена сума: 0.5*важливість + 0.3*терміновість + 0.2*очікування
class WeightedSumPriorityStrategy final : public IPriorityStrategy
{
public:
    double calculate(const PriorityInput& input) const override;
    std::string name() const override { return "weighted"; }
};

// нормована зважена сума: кожен параметр приводиться до 0..1
// (важливість і терміновість діляться на 5, очікування на 24 год і
// обмежується одиницею); результат у межах 0..1
class NormalizedWeightedPriorityStrategy final : public IPriorityStrategy
{
public:
    double calculate(const PriorityInput& input) const override;
    std::string name() const override { return "normalized"; }
};

class PriorityCalculator
{
public:
    explicit PriorityCalculator(std::unique_ptr<IPriorityStrategy> strategy);

    void setStrategy(std::unique_ptr<IPriorityStrategy> strategy);
    std::string strategyName() const;

    // перевіряє вхідні дані й делегує обчислення стратегії
    double calculate(const PriorityInput& input) const;

private:
    std::unique_ptr<IPriorityStrategy> strategy_;
};

// вибір за ключем: "max", "weighted", "normalized"
std::unique_ptr<IPriorityStrategy> makePriorityStrategy(const std::string& key);
