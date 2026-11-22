#pragma once

#include <algorithm>
#include <exception>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

// типи подій
enum class EventType
{
    NotificationCreated,  // створення сповіщення
    DeliveryCompleted     // завершення доставки 
};

inline const char* toString(EventType type)
{
    switch (type)
    {
    case EventType::NotificationCreated: return "NotificationCreated";
    case EventType::DeliveryCompleted:   return "DeliveryCompleted";
    }
    return "Unknown";
}

struct NotificationEvent
{
    EventType type;
    int notificationId = 0;
    std::string oldStatus;   
    std::string newStatus;
    std::string occurredAt;
};

// інтерфейс спостерігача
class IObserver
{
public:
    virtual ~IObserver() = default;
    virtual void update(const NotificationEvent& event) = 0;
};

// видавець: зберігає слабкі посилання, тому не продовжує життя підписників
class EventPublisher
{
public:
    void subscribe(const std::shared_ptr<IObserver>& observer)
    {
        if (!observer)
            throw std::invalid_argument("observer must not be null");

        removeExpired();
        const bool already = std::any_of(observers_.begin(), observers_.end(),
            [&](const std::weak_ptr<IObserver>& item)
            {
                return item.lock() == observer;
            });
        if (!already)
            observers_.push_back(observer);
    }

    void unsubscribe(const std::shared_ptr<IObserver>& observer)
    {
        std::erase_if(observers_,
            [&](const std::weak_ptr<IObserver>& item)
            {
                auto ptr = item.lock();
                return !ptr || ptr == observer;
            });
    }

    // сповіщає всіх активних підписників
    void publish(const NotificationEvent& event)
    {
        removeExpired();

        std::vector<std::shared_ptr<IObserver>> active;
        for (const auto& item : observers_)
        {
            if (auto observer = item.lock())
                active.push_back(std::move(observer));
        }

        for (const auto& observer : active)
        {
            try
            {
                observer->update(event);
            }
            catch (const std::exception& ex)
            {
                std::cerr << "Observer failed: " << ex.what() << '\n';
            }
            catch (...)
            {
                std::cerr << "Observer failed: unknown error\n";
            }
        }
    }

    std::size_t subscriberCount()
    {
        removeExpired();
        return observers_.size();
    }

private:
    void removeExpired()
    {
        std::erase_if(observers_,
            [](const std::weak_ptr<IObserver>& item) { return item.expired(); });
    }

    std::vector<std::weak_ptr<IObserver>> observers_;
};
