#include "NotificationState.h"

// операція в цьому стані недозволена:

void NotificationState::send(NotificationContext&) { reject("send"); }
void NotificationState::markDelivered(NotificationContext&) { reject("markDelivered"); }
void NotificationState::markFailed(NotificationContext&) { reject("markFailed"); }

void NotificationState::reject(const char* operation) const
{
    throw InvalidTransitionError(std::string("operation '") + operation
        + "' is not allowed in state '" + name() + "'");
}

// конкретні стани

namespace
{
    // Створене -> Надіслане
    class CreatedState final : public NotificationState
    {
    public:
        void send(NotificationContext& context) override;
        std::string name() const override { return Status::kNew; }
    };

    // Надіслане -> Доставлене + Помилка доставки
    class SentState final : public NotificationState
    {
    public:
        void markDelivered(NotificationContext& context) override;
        void markFailed(NotificationContext& context) override;
        std::string name() const override { return Status::kSent; }
    };

    // кінцевий стан, жодна операція не дозволена
    class DeliveredState final : public NotificationState
    {
    public:
        std::string name() const override { return Status::kDelivered; }
    };

    // Помилка доставки -> Надіслане 
    class FailedState final : public NotificationState
    {
    public:
        void send(NotificationContext& context) override;
        std::string name() const override { return Status::kFailed; }
    };

    void CreatedState::send(NotificationContext& context)
    {
        context.transitionTo(std::make_unique<SentState>());
    }

    void SentState::markDelivered(NotificationContext& context)
    {
        context.transitionTo(std::make_unique<DeliveredState>());
    }

    void SentState::markFailed(NotificationContext& context)
    {
        context.transitionTo(std::make_unique<FailedState>());
    }

    void FailedState::send(NotificationContext& context)
    {
        if (context.retryCount() >= kMaxRetries)
        {
            throw InvalidTransitionError("retry limit ("
                + std::to_string(kMaxRetries) + ") exceeded");
        }

        context.registerRetry();
        context.transitionTo(std::make_unique<SentState>());
    }
}

std::unique_ptr<NotificationState> makeStateFromStatus(const std::string& status)
{
    if (status == Status::kNew)
        return std::make_unique<CreatedState>();
    if (status == Status::kSent)
        return std::make_unique<SentState>();
    if (status == Status::kDelivered || status == Status::kLegacyRead)
        return std::make_unique<DeliveredState>();
    if (status == Status::kFailed)
        return std::make_unique<FailedState>();

    throw std::invalid_argument("unknown notification status: '" + status + "'");
}

// NotificationContext

NotificationContext::NotificationContext(Notification notification)
    : data_(std::move(notification)),
      state_(makeStateFromStatus(data_.status))
{
}

void NotificationContext::send() { state_->send(*this); }
void NotificationContext::markDelivered() { state_->markDelivered(*this); }
void NotificationContext::markFailed() { state_->markFailed(*this); }

std::string NotificationContext::stateName() const
{
    return state_->name();
}

void NotificationContext::transitionTo(std::unique_ptr<NotificationState> next)
{
    if (!next)
        throw std::invalid_argument("next state must not be null");

    data_.status = next->name();
    state_ = std::move(next);  // попередній стан знищується автоматично
}

void NotificationContext::registerRetry()
{
    ++data_.retryCount;
}
