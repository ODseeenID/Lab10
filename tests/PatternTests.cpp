#include <gtest/gtest.h>

#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "Database.h"
#include "Events.h"
#include "NotificationFacade.h"
#include "NotificationService.h"
#include "NotificationState.h"
#include "Observers.h"
#include "PriorityStrategy.h"
#include "Repositories.h"

namespace
{
    Notification makeNotification(const std::string& status, int retryCount = 0)
    {
        return { 1, 1, "Test", 3, status, retryCount, "2026-11-08 00:00:00" };
    }

    class RecordingObserver : public IObserver
    {
    public:
        void update(const NotificationEvent& event) override { events.push_back(event); }
        std::vector<NotificationEvent> events;
    };

    class ThrowingObserver : public IObserver
    {
    public:
        void update(const NotificationEvent&) override
        {
            throw std::runtime_error("observer failure");
        }
    };

    // репозиторій історії, який завжди падає: перевірка відкату транзакції
    class FailingHistoryRepository : public IStatusHistoryRepository
    {
    public:
        int add(const NotificationStatusHistory&) override
        {
            throw std::runtime_error("history write failed");
        }
        std::vector<NotificationStatusHistory> findByNotification(int) override
        {
            return {};
        }
    };

    NotificationEvent makeEvent(EventType type, const std::string& newStatus)
    {
        return { type, 1, "", newStatus, "2026-11-08 00:00:00" };
    }
}

// State

TEST(StateTest, HappyPathNewSentDelivered)
{
    NotificationContext context(makeNotification(Status::kNew));

    context.send();
    EXPECT_EQ(context.data().status, Status::kSent);

    context.markDelivered();
    EXPECT_EQ(context.data().status, Status::kDelivered);
    EXPECT_EQ(context.stateName(), Status::kDelivered);
}

TEST(StateTest, InvalidTransitionsAreRejectedAndStatusIsKept)
{
    NotificationContext fresh(makeNotification(Status::kNew));
    EXPECT_THROW(fresh.markDelivered(), InvalidTransitionError);
    EXPECT_THROW(fresh.markFailed(), InvalidTransitionError);
    EXPECT_EQ(fresh.data().status, Status::kNew);

    NotificationContext delivered(makeNotification(Status::kDelivered));
    EXPECT_THROW(delivered.send(), InvalidTransitionError);
    EXPECT_THROW(delivered.markFailed(), InvalidTransitionError);
    EXPECT_EQ(delivered.data().status, Status::kDelivered);
}

// Strategy

TEST(StrategyTest, ThreeAlgorithmsGiveExpectedValues)
{
    const PriorityInput input{ 4, 2, 12 };

    EXPECT_NEAR(MaxPriorityStrategy().calculate(input), 12.0, 1e-9);
    EXPECT_NEAR(WeightedSumPriorityStrategy().calculate(input), 5.0, 1e-9);
    EXPECT_NEAR(NormalizedWeightedPriorityStrategy().calculate(input), 0.62, 1e-9);

    // нормований результат не виходить за межі 0..1
    EXPECT_NEAR(NormalizedWeightedPriorityStrategy().calculate({ 5, 5, 1000 }), 1.0, 1e-9);
}

TEST(StrategyTest, StrategyCanBeReplacedAtRunTime)
{
    PriorityCalculator calculator(std::make_unique<MaxPriorityStrategy>());
    const PriorityInput input{ 4, 2, 12 };
    6 
    EXPECT_EQ(calculator.strategyName(), "max");
    EXPECT_NEAR(calculator.calculate(input), 12.0, 1e-9);

    calculator.setStrategy(makePriorityStrategy("normalized"));
    EXPECT_EQ(calculator.strategyName(), "normalized");
    EXPECT_NEAR(calculator.calculate(input), 0.62, 1e-9);
}

// Observer

TEST(ObserverTest, SubscribersReceiveEventsAndCanUnsubscribe)
{
    EventPublisher publisher;
    auto first = std::make_shared<RecordingObserver>();
    auto second = std::make_shared<RecordingObserver>();
    publisher.subscribe(first);
    publisher.subscribe(second);

    publisher.publish(makeEvent(EventType::NotificationCreated, Status::kNew));
    EXPECT_EQ(first->events.size(), 1u);
    EXPECT_EQ(second->events.size(), 1u);

    publisher.unsubscribe(second);
    publisher.publish(makeEvent(EventType::DeliveryCompleted, Status::kDelivered));
    EXPECT_EQ(first->events.size(), 2u);
    EXPECT_EQ(second->events.size(), 1u);
}

TEST(ObserverTest, FailingObserverDoesNotBlockOthers)
{
    EventPublisher publisher;
    auto failing = std::make_shared<ThrowingObserver>();
    auto healthy = std::make_shared<RecordingObserver>();
    publisher.subscribe(failing);
    publisher.subscribe(healthy);

    EXPECT_NO_THROW(publisher.publish(
        makeEvent(EventType::NotificationCreated, Status::kNew)));
    EXPECT_EQ(healthy->events.size(), 1u);
}

TEST(ObserverTest, StatsAndAuditObserversReactIndependently)
{
    DeliveryStatsObserver stats;
    AuditLogObserver audit;

    const auto created = makeEvent(EventType::NotificationCreated, Status::kNew);
    const auto delivered = makeEvent(EventType::DeliveryCompleted, Status::kDelivered);
    const auto failed = makeEvent(EventType::DeliveryCompleted, Status::kFailed);
    for (const auto& event : { created, delivered, failed })
    {
        stats.update(event);
        audit.update(event);
    }

    EXPECT_EQ(stats.created(), 1);
    EXPECT_EQ(stats.delivered(), 1);
    EXPECT_EQ(stats.failed(), 1);
    EXPECT_NEAR(stats.successRate(), 0.5, 1e-9);
    EXPECT_EQ(audit.entries().size(), 3u);
}

// Service: State + Observer + Strategy + SQLite (база в пам'яті)

class ServiceTest : public ::testing::Test
{
protected:
    ServiceTest()
        : storage_(makeStorage(":memory:")),
        categories_(storage_),
        notifications_(storage_),
        history_(storage_),
        clockValue_(std::make_shared<std::string>("2026-11-08 00:00:00"))
    {
        storage_.open_forever();  // одне з'єднання, інакше :memory: щоразу нова
        storage_.sync_schema();
        categoryId_ = categories_.add({ 0, "Security", "desc", "email", 1 });

        recorder_ = std::make_shared<RecordingObserver>();
        publisher_.subscribe(recorder_);
    }

    std::unique_ptr<NotificationService> makeService(IStatusHistoryRepository& history)
    {
        auto clockValue = clockValue_;
        return std::make_unique<NotificationService>(
            storage_, notifications_, history, publisher_,
            std::make_unique<WeightedSumPriorityStrategy>(),
            [clockValue] { return *clockValue; });
    }

    Storage storage_;
    SQLiteCategoryRepository categories_;
    SQLiteNotificationRepository notifications_;
    SQLiteStatusHistoryRepository history_;
    EventPublisher publisher_;
    std::shared_ptr<RecordingObserver> recorder_;
    std::shared_ptr<std::string> clockValue_;
    int categoryId_ = 0;
};

TEST_F(ServiceTest, CreateStoresNotificationHistoryAndPublishesEvent)
{
    auto service = makeService(history_);

    const int id = service->createNotification(categoryId_, "Welcome", 3);

    auto stored = notifications_.findById(id);
    ASSERT_TRUE(stored.has_value());
    EXPECT_EQ(stored->status, Status::kNew);

    const auto rows = service->history(id);
    ASSERT_EQ(rows.size(), 1u);
    EXPECT_EQ(rows[0].oldStatus, "");
    EXPECT_EQ(rows[0].newStatus, Status::kNew);

    ASSERT_EQ(recorder_->events.size(), 1u);
    EXPECT_EQ(recorder_->events[0].type, EventType::NotificationCreated);
}

TEST_F(ServiceTest, FullLifecycleIsPersistedInHistory)
{
    auto service = makeService(history_);
    const int id = service->createNotification(categoryId_, "Welcome", 3);

    service->send(id);
    service->markFailed(id);
    service->send(id);
    service->markDelivered(id);

    auto stored = notifications_.findById(id);
    ASSERT_TRUE(stored.has_value());
    EXPECT_EQ(stored->status, Status::kDelivered);
    EXPECT_EQ(stored->retryCount, 1);

    const auto rows = service->history(id);
    ASSERT_EQ(rows.size(), 5u);  // "" -> new -> sent -> failed -> sent -> delivered
    EXPECT_EQ(rows.back().oldStatus, Status::kSent);
    EXPECT_EQ(rows.back().newStatus, Status::kDelivered);

    // події: створення, помилка доставки, успішна доставка
    ASSERT_EQ(recorder_->events.size(), 3u);
    EXPECT_EQ(recorder_->events[1].newStatus, Status::kFailed);
    EXPECT_EQ(recorder_->events[2].newStatus, Status::kDelivered);
}

TEST_F(ServiceTest, InvalidTransitionChangesNothing)
{
    auto service = makeService(history_);
    const int id = service->createNotification(categoryId_, "Welcome", 3);
    const auto eventsBefore = recorder_->events.size();

    EXPECT_THROW(service->markDelivered(id), InvalidTransitionError);

    EXPECT_EQ(notifications_.findById(id)->status, Status::kNew);
    EXPECT_EQ(service->history(id).size(), 1u);
    EXPECT_EQ(recorder_->events.size(), eventsBefore);
}

TEST_F(ServiceTest, FailedHistoryWriteRollsBackStatusChange)
{
    auto service = makeService(history_);
    const int id = service->createNotification(categoryId_, "Welcome", 3);
    const auto eventsBefore = recorder_->events.size();

    FailingHistoryRepository failingHistory;
    auto brokenService = makeService(failingHistory);

    EXPECT_THROW(brokenService->send(id), std::runtime_error);

    // зміна статусу відкотилася разом із невдалим записом в історію
    EXPECT_EQ(notifications_.findById(id)->status, Status::kNew);
    EXPECT_EQ(service->history(id).size(), 1u);
    EXPECT_EQ(recorder_->events.size(), eventsBefore);
}

// Facade

class FacadeTest : public ::testing::Test
{
protected:
    FacadeTest()
        : storage_(makeStorage(":memory:")),
        clockValue_(std::make_shared<std::string>("2026-11-08 00:00:00")),
        facade_(storage_, nullptr, [c = clockValue_] { return *c; })
    {
        storage_.open_forever();
        storage_.sync_schema();
        categoryId_ = facade_.addCategory("Security", "desc", "email");
    }

    Storage storage_;
    std::shared_ptr<std::string> clockValue_;
    NotificationFacade facade_;
    int categoryId_ = 0;
};

TEST_F(FacadeTest, DispatchSuccessDeliversAndUpdatesStatistics)
{
    const int id = facade_.createNotification(categoryId_, "Welcome", 3);

    facade_.dispatch(id, true);

    EXPECT_EQ(facade_.find(id)->status, Status::kDelivered);
    EXPECT_EQ(facade_.history(id).size(), 3u);  // "" -> new -> sent -> delivered
    EXPECT_EQ(facade_.statistics().created(), 1);
    EXPECT_EQ(facade_.statistics().delivered(), 1);
    EXPECT_EQ(facade_.auditLog().entries().size(), 2u);
}

