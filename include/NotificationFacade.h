#pragma once

#include <iosfwd>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "Database.h"
#include "Events.h"
#include "NotificationService.h"
#include "Observers.h"
#include "Repositories.h"

class NotificationFacade
{
public:
    explicit NotificationFacade(Storage& storage,
        std::ostream* auditOut = nullptr, Clock clock = {});

    NotificationFacade(const NotificationFacade&) = delete;
    NotificationFacade& operator=(const NotificationFacade&) = delete;

    // дані
    int addCategory(const std::string& name,
        const std::string& description, const std::string& channel);
    int createNotification(int categoryId, const std::string& title, int priority);
    std::optional<Notification> find(int notificationId);
    std::vector<NotificationStatusHistory> history(int notificationId);

    void dispatch(int notificationId, bool success);

    // пріоритет за обраним алгоритмом: "max", "weighted", "normalized"
    double priority(int notificationId, int urgency, const std::string& strategyKey);

    // підписники
    void setStatisticsEnabled(bool enabled);
    void setAuditEnabled(bool enabled);
    const DeliveryStatsObserver& statistics() const { return *stats_; }
    const AuditLogObserver& auditLog() const { return *audit_; }

private:
    // порядок оголошення = порядок ініціалізації
    SQLiteCategoryRepository categories_;
    SQLiteNotificationRepository notifications_;
    SQLiteStatusHistoryRepository history_;
    EventPublisher publisher_;
    std::shared_ptr<AuditLogObserver> audit_;
    std::shared_ptr<DeliveryStatsObserver> stats_;
    NotificationService service_;
};
