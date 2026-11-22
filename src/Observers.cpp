#include "Observers.h"

#include <ostream>

#include "NotificationState.h"

AuditLogObserver::AuditLogObserver(std::ostream* out)
    : out_(out)
{
}

void AuditLogObserver::update(const NotificationEvent& event)
{
    std::string line = "[" + event.occurredAt + "] "
        + toString(event.type) + ": notification #"
        + std::to_string(event.notificationId) + ", ";

    if (!event.oldStatus.empty())
        line += event.oldStatus + " -> ";
    line += event.newStatus;

    entries_.push_back(line);
    if (out_)
        *out_ << "  [audit] " << line << '\n';
}

void DeliveryStatsObserver::update(const NotificationEvent& event)
{
    switch (event.type)
    {
    case EventType::NotificationCreated:
        ++created_;
        break;
    case EventType::DeliveryCompleted:
        if (event.newStatus == Status::kDelivered)
            ++delivered_;
        else
            ++failed_;
        break;
    }
}

double DeliveryStatsObserver::successRate() const
{
    const int completed = delivered_ + failed_;
    return completed == 0 ? 0.0 : static_cast<double>(delivered_) / completed;
}
