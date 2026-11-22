#pragma once

#include <memory>
#include <stdexcept>
#include <string>

#include "Models.h"

// значення поля notification.status
namespace Status
{
    inline constexpr const char* kNew = "new";              
    inline constexpr const char* kSent = "sent";            
    inline constexpr const char* kDelivered = "delivered"; 
    inline constexpr const char* kFailed = "failed";     
    inline constexpr const char* kLegacyRead = "read";      // прочитане = доставлене
}

// кількість повтрорних надсилань в разі помилки доставки
inline constexpr int kMaxRetries = 5;

// заборонений перехід між станами
class InvalidTransitionError : public std::logic_error
{
public:
    using std::logic_error::logic_error;
};

class NotificationContext;

// інтерфейс стану
class NotificationState
{
public:
    virtual ~NotificationState() = default;

    virtual void send(NotificationContext& context);
    virtual void markDelivered(NotificationContext& context);
    virtual void markFailed(NotificationContext& context);

    virtual std::string name() const = 0;

protected:
    [[noreturn]] void reject(const char* operation) const;
};

//обгортка над сутністю Notification, що не знає про бд, лише
// змінює status/retryCount сутності; збереження виконує NotificationService
class NotificationContext
{
public:
    explicit NotificationContext(Notification notification);

    void send();
    void markDelivered();
    void markFailed();

    const Notification& data() const noexcept { return data_; }
    std::string stateName() const;
    void transitionTo(std::unique_ptr<NotificationState> next);
    void registerRetry();
    int retryCount() const noexcept { return data_.retryCount; }

private:
    Notification data_;
    std::unique_ptr<NotificationState> state_;
};

std::unique_ptr<NotificationState> makeStateFromStatus(const std::string& status);
