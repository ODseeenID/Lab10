#include "NotificationFacade.h"

NotificationFacade::NotificationFacade(Storage& storage,
    std::ostream* auditOut, Clock clock)
    : categories_(storage),
      notifications_(storage),
      history_(storage),
      audit_(std::make_shared<AuditLogObserver>(auditOut)),
      stats_(std::make_shared<DeliveryStatsObserver>()),
      service_(storage, notifications_, history_, publisher_,
          std::make_unique<WeightedSumPriorityStrategy>(), std::move(clock))
{
    publisher_.subscribe(audit_);
    publisher_.subscribe(stats_);
}

int NotificationFacade::addCategory(const std::string& name,
    const std::string& description, const std::string& channel)
{
    return categories_.add({ 0, name, description, channel, 1 });
}

int NotificationFacade::createNotification(
    int categoryId, const std::string& title, int priority)
{
    return service_.createNotification(categoryId, title, priority);
}

std::optional<Notification> NotificationFacade::find(int notificationId)
{
    return notifications_.findById(notificationId);
}

std::vector<NotificationStatusHistory>
NotificationFacade::history(int notificationId)
{
    return service_.history(notificationId);
}

void NotificationFacade::dispatch(int notificationId, bool success)
{
    service_.send(notificationId);

    if (success)
        service_.markDelivered(notificationId);
    else
        service_.markFailed(notificationId);
}

double NotificationFacade::priority(
    int notificationId, int urgency, const std::string& strategyKey)
{
    service_.setPriorityStrategy(makePriorityStrategy(strategyKey));
    return service_.calculatePriority(notificationId, urgency);
}

void NotificationFacade::setStatisticsEnabled(bool enabled)
{
    if (enabled)
        publisher_.subscribe(stats_);
    else
        publisher_.unsubscribe(stats_);
}

void NotificationFacade::setAuditEnabled(bool enabled)
{
    if (enabled)
        publisher_.subscribe(audit_);
    else
        publisher_.unsubscribe(audit_);
}
