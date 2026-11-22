#include "PriorityStrategy.h"

#include <algorithm>
#include <stdexcept>

namespace
{
    constexpr double kMinScore = 1.0;
    constexpr double kMaxScore = 5.0;
    constexpr double kMaxWaitingHours = 24.0;

    constexpr double kImportanceWeight = 0.5;
    constexpr double kUrgencyWeight = 0.3;
    constexpr double kWaitingWeight = 0.2;
}

double MaxPriorityStrategy::calculate(const PriorityInput& input) const
{
    return std::max({ input.importance, input.urgency, input.waitingHours });
}

double WeightedSumPriorityStrategy::calculate(const PriorityInput& input) const
{
    return kImportanceWeight * input.importance
        + kUrgencyWeight * input.urgency
        + kWaitingWeight * input.waitingHours;
}

double NormalizedWeightedPriorityStrategy::calculate(const PriorityInput& input) const
{
    const double importance = input.importance / kMaxScore;
    const double urgency = input.urgency / kMaxScore;
    const double waiting = std::min(input.waitingHours / kMaxWaitingHours, 1.0);

    return kImportanceWeight * importance
        + kUrgencyWeight * urgency
        + kWaitingWeight * waiting;
}

PriorityCalculator::PriorityCalculator(std::unique_ptr<IPriorityStrategy> strategy)
{
    setStrategy(std::move(strategy));
}

void PriorityCalculator::setStrategy(std::unique_ptr<IPriorityStrategy> strategy)
{
    if (!strategy)
        throw std::invalid_argument("strategy must not be null");

    strategy_ = std::move(strategy);
}

std::string PriorityCalculator::strategyName() const
{
    return strategy_->name();
}

double PriorityCalculator::calculate(const PriorityInput& input) const
{
    if (input.importance < kMinScore || input.importance > kMaxScore)
        throw std::invalid_argument("importance must be in range 1..5");
    if (input.urgency < kMinScore || input.urgency > kMaxScore)
        throw std::invalid_argument("urgency must be in range 1..5");
    if (input.waitingHours < 0)
        throw std::invalid_argument("waiting time must not be negative");

    return strategy_->calculate(input);
}

std::unique_ptr<IPriorityStrategy> makePriorityStrategy(const std::string& key)
{
    if (key == "max")
        return std::make_unique<MaxPriorityStrategy>();
    if (key == "weighted")
        return std::make_unique<WeightedSumPriorityStrategy>();
    if (key == "normalized")
        return std::make_unique<NormalizedWeightedPriorityStrategy>();

    throw std::invalid_argument("unknown priority strategy: '" + key + "'");
}
