#include "NotificationService.h"

#include <algorithm>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace
{
    std::string systemClock()
    {
        const std::time_t now = std::chrono::system_clock::to_time_t(
            std::chrono::system_clock::now());

        std::tm utc{};
#if defined(_WIN32)
        gmtime_s(&utc, &now);
#else
        gmtime_r(&now, &utc);
#endif
        std::ostringstream out;
        out << std::put_time(&utc, "%Y-%m-%d %H:%M:%S");
        return out.str();
    }

    // кількість днів від 1970-01-01 для календарної дати (алгоритм Говарда Хіннанта)
    long long daysFromCivil(int y, unsigned m, unsigned d)
    {
        y -= m <= 2;
        const long long era = (y >= 0 ? y : y - 399) / 400;
        const unsigned yoe = static_cast<unsigned>(y - era * 400);
        const unsigned doy = (153 * (m > 2 ? m - 3 : m + 9) + 2) / 5 + d - 1;
        const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
        return era * 146097 + static_cast<long long>(doe) - 719468;
    }

    // "YYYY-MM-DD HH:MM:SS" (залежності від часової зони нема)
    long long toEpochSeconds(const std::string& timestamp)
    {
        std::tm tm{};
        std::istringstream in(timestamp);
        in >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
        if (in.fail())
            throw std::invalid_argument("invalid timestamp: '" + timestamp + "'");

        return daysFromCivil(tm.tm_year + 1900,
                   static_cast<unsigned>(tm.tm_mon + 1),
                   static_cast<unsigned>(tm.tm_mday)) * 86400
            + tm.tm_hour * 3600 + tm.tm_min * 60 + tm.tm_sec;
    }
}

NotificationService::NotificationService(Storage& storage,
    INotificationRepository& notifications,
    IStatusHistoryRepository& history,
    EventPublisher& publisher,
    std::unique_ptr<IPriorityStrategy> strategy,
    Clock clock)
    : storage_(storage),
      notifications_(notifications),
      history_(history),
      publisher_(publisher),
      calculator_(std::move(strategy)),
      clock_(clock ? std::move(clock) : Clock(systemClock))
{
}

int NotificationService::createNotification(
    int categoryId, const std::string& title, int priority)
{
    if (title.empty())
        throw std::invalid_argument("title must not be empty");
    if (priority < 1 || priority > 5)
        throw std::invalid_argument("priority must be in range 1..5");

    const std::string now = clock_();
    int id = 0;

    // сповіщення і перший запис історії зберігаються разом або не зберігаються взагалі
    storage_.transaction([&]
        {
            id = notifications_.add(
                { 0, categoryId, title, priority, Status::kNew, 0, now });
            history_.add({ 0, id, "", Status::kNew, now });
            return true;
        });

    // подія публікується тільки після успішного COMMIT
    publisher_.publish({ EventType::NotificationCreated, id, "", Status::kNew, now });
    return id;
}

void NotificationService::send(int notificationId)
{
    changeState(notificationId, [](NotificationContext& c) { c.send(); });
}

void NotificationService::markDelivered(int notificationId)
{
    changeState(notificationId, [](NotificationContext& c) { c.markDelivered(); });
}

void NotificationService::markFailed(int notificationId)
{
    changeState(notificationId, [](NotificationContext& c) { c.markFailed(); });
}

void NotificationService::changeState(int notificationId,
    const std::function<void(NotificationContext&)>& operation)
{
    auto stored = notifications_.findById(notificationId);
    if (!stored)
        throw std::runtime_error(
            "notification not found: " + std::to_string(notificationId));

    NotificationContext context(*stored);
    const std::string oldStatus = stored->status;

    // допустимість переходу перевіряє State; при помилці нічого не записано
    operation(context);

    const Notification& updated = context.data();
    const std::string now = clock_();

    // зміна стану і запис в історію - одна транзакція SQLite
    storage_.transaction([&]
        {
            if (!notifications_.update(updated))
                throw std::runtime_error("notification disappeared during update");
            history_.add({ 0, notificationId, oldStatus, updated.status, now });
            return true;
        });

    if (updated.status == Status::kDelivered || updated.status == Status::kFailed)
    {
        publisher_.publish({ EventType::DeliveryCompleted,
            notificationId, oldStatus, updated.status, now });
    }
}

std::vector<NotificationStatusHistory>
NotificationService::history(int notificationId)
{
    return history_.findByNotification(notificationId);
}

void NotificationService::setPriorityStrategy(
    std::unique_ptr<IPriorityStrategy> strategy)
{
    calculator_.setStrategy(std::move(strategy));
}

std::string NotificationService::priorityStrategyName() const
{
    return calculator_.strategyName();
}

double NotificationService::calculatePriority(int notificationId, int urgency)
{
    auto stored = notifications_.findById(notificationId);
    if (!stored)
        throw std::runtime_error(
            "notification not found: " + std::to_string(notificationId));

    const long long waitingSeconds
        = toEpochSeconds(clock_()) - toEpochSeconds(stored->createdAt);

    PriorityInput input;
    input.importance = stored->priority;
    input.urgency = urgency;
    input.waitingHours = std::max(0LL, waitingSeconds) / 3600.0;

    return calculator_.calculate(input);
}
