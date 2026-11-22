#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "Database.h"
#include "Events.h"
#include "NotificationState.h"
#include "PriorityStrategy.h"
#include "Repositories.h"

// джерело поточного часу формату YYYY-MM-DD HH:MM:SS ;
using Clock = std::function<std::string()>;

// об'єднання State, Strategy, Observer і репозиторії
// SQL не виконує: дані читає й пише через репозиторії, транзакцію відкриває через storage
class NotificationService
{
public:
    NotificationService(Storage& storage,
        INotificationRepository& notifications,
        IStatusHistoryRepository& history,
        EventPublisher& publisher,
        std::unique_ptr<IPriorityStrategy> strategy
            = std::make_unique<WeightedSumPriorityStrategy>(),
        Clock clock = {});

    int createNotification(int categoryId, const std::string& title, int priority);

    // зміни стану: InvalidTransitionError, БД не змінюється
    void send(int notificationId);
    void markDelivered(int notificationId);
    void markFailed(int notificationId);

    std::vector<NotificationStatusHistory> history(int notificationId);

    // Strategy
    void setPriorityStrategy(std::unique_ptr<IPriorityStrategy> strategy);
    std::string priorityStrategyName() const;
    double calculatePriority(int notificationId, int urgency);

private:
    void changeState(int notificationId,
        const std::function<void(NotificationContext&)>& operation);

    Storage& storage_;
    INotificationRepository& notifications_;
    IStatusHistoryRepository& history_;
    EventPublisher& publisher_;
    PriorityCalculator calculator_;
    Clock clock_;
};
