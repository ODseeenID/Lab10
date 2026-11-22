#include "PatternsDemo.h"

#include <functional>
#include <iomanip>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

#include "NotificationService.h"
#include "Observers.h"
#include "NotificationFacade.h"

namespace
{
    void printHistory(NotificationService& service, int id)
    {
        std::cout << "  history of notification " << id << ":\n";
        for (const auto& row : service.history(id))
        {
            std::cout << "    " << row.changedAt << " | "
                << (row.oldStatus.empty() ? "(none)" : row.oldStatus)
                << " -> " << row.newStatus << '\n';
        }
    }

    // виконує операцію й показує, чи була вона дозволена
    void attempt(const std::string& label, const std::function<void()>& action)
    {
        try
        {
            action();
            std::cout << "  OK       " << label << '\n';
        }
        catch (const InvalidTransitionError& ex)
        {
            std::cout << "  REJECTED " << label << ": " << ex.what() << '\n';
        }
    }

    void printStats(const DeliveryStatsObserver& stats)
    {
        std::cout << "  stats: created=" << stats.created()
            << " delivered=" << stats.delivered()
            << " failed=" << stats.failed()
            << " successRate=" << std::fixed << std::setprecision(2)
            << stats.successRate() << '\n';
    }
}

void runPatternsDemo(Storage& storage,
    ICategoryRepository& categories,
    INotificationRepository& notifications)
{
    std::cout << "\n BEHAVIORAL PATTERNS (State, Strategy, Observer) \n";

    const auto allCategories = categories.findAll();
    if (allCategories.empty())
        throw std::runtime_error("patterns demo needs at least one category");
    const int categoryId = allCategories.front().id;

    // фіксований годинник робить виведення відтворюваним
    auto now = std::make_shared<std::string>("2026-11-08 09:00:00");

    SQLiteStatusHistoryRepository history(storage);
    EventPublisher publisher;

    auto audit = std::make_shared<AuditLogObserver>(&std::cout);
    auto stats = std::make_shared<DeliveryStatsObserver>();
    publisher.subscribe(audit);
    publisher.subscribe(stats);

    NotificationService service(storage, notifications, history, publisher,
        std::make_unique<WeightedSumPriorityStrategy>(),
        [now] { return *now; });

    // State
    std::cout << "\nScenario A: State - transitions and history\n";
    const int id = service.createNotification(categoryId, "Password reset code", 4);

    attempt("markDelivered on 'new'", [&] { service.markDelivered(id); });
    attempt("send", [&] { service.send(id); });
    attempt("markFailed", [&] { service.markFailed(id); });
    attempt("send (retry)", [&] { service.send(id); });
    attempt("markDelivered", [&] { service.markDelivered(id); });
    attempt("send on 'delivered'", [&] { service.send(id); });
    printHistory(service, id);
    std::cout << "  result: invalid transitions were rejected and not saved\n";

    // Observer
    std::cout << "\nScenario B: Observer - unsubscribe and subscribe again\n";
    printStats(*stats);

    publisher.unsubscribe(stats);
    std::cout << "  statistics observer unsubscribed (subscribers: "
        << publisher.subscriberCount() << ")\n";
    const int id2 = service.createNotification(categoryId, "New login detected", 5);
    service.send(id2);
    service.markDelivered(id2);
    printStats(*stats);
    std::cout << "  result: statistics did not change, audit log still received events\n";

    publisher.subscribe(stats);
    const int id3 = service.createNotification(categoryId, "Invoice due", 3);
    service.send(id3);
    service.markFailed(id3);
    printStats(*stats);
    std::cout << "  result: after subscribing again, events are counted again\n";

    // Strategy
    std::cout << "\nScenario C: Strategy - priority of notification " << id
        << " (importance 4, urgency 2, waiting 12 h)\n";
    *now = "2026-11-08 21:00:00";
    for (const char* key : { "max", "weighted", "normalized" })
    {
        service.setPriorityStrategy(makePriorityStrategy(key));
        std::cout << "  " << std::left << std::setw(11)
            << service.priorityStrategyName() << std::right
            << std::fixed << std::setprecision(2)
            << service.calculatePriority(id, 2) << '\n';
    }
    std::cout << "  result: the algorithm was replaced at run time, "
        "the calling code did not change\n";
}

void runFacadeDemo(Storage& storage)
{
    std::cout << "\n FACADE \n";

    // одна змінна замість репозиторіїв, видавця, підписників і сервісу
    auto now = std::make_shared<std::string>("2026-11-08 09:00:00");
    NotificationFacade facade(storage, &std::cout, [now] { return *now; });

    const int categoryId = facade.addCategory(
        "Facade demo", "Created through the facade", "push");

    std::cout << "\nScenario D: lifecycle in single calls\n";
    const int ok = facade.createNotification(categoryId, "Order shipped", 4);
    facade.dispatch(ok, true);
    std::cout << "  dispatch(success) -> status "
        << facade.find(ok)->status << '\n';

    const int bad = facade.createNotification(categoryId, "Payment declined", 5);
    facade.dispatch(bad, false);
    std::cout << "  dispatch(failure) -> status "
        << facade.find(bad)->status << '\n';
    facade.dispatch(bad, true);
    std::cout << "  dispatch(retry)   -> status "
        << facade.find(bad)->status
        << ", retryCount " << facade.find(bad)->retryCount << '\n';

    for (const auto& row : facade.history(bad))
    {
        std::cout << "    " << (row.oldStatus.empty() ? "(none)" : row.oldStatus)
            << " -> " << row.newStatus << '\n';
    }

    std::cout << "\nScenario E: priority and statistics through the facade\n";
    *now = "2026-11-08 21:00:00";
    for (const char* key : { "max", "weighted", "normalized" })
    {
        std::cout << "  " << std::left << std::setw(11) << key << std::right
            << std::fixed << std::setprecision(2)
            << facade.priority(ok, 2, key) << '\n';
    }
    printStats(facade.statistics());
}

