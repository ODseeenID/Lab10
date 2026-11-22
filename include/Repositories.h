#pragma once

#include <optional>
#include <vector>

#include "Database.h"
#include "Models.h"

//  інтерфейси 

class ICategoryRepository
{
public:
    virtual ~ICategoryRepository() = default;

    virtual int add(const NotificationCategory& category) = 0;          // create
    virtual std::optional<NotificationCategory> findById(int id) = 0;   // read
    virtual std::vector<NotificationCategory> findAll() = 0;            // read
    virtual bool update(const NotificationCategory& category) = 0;      // update
    virtual bool remove(int id) = 0;                                    // delete
};

class INotificationRepository
{
public:
    virtual ~INotificationRepository() = default;

    virtual int add(const Notification& notification) = 0;
    virtual std::optional<Notification> findById(int id) = 0;
    virtual std::vector<Notification> findAll() = 0;
    virtual bool update(const Notification& notification) = 0;
    virtual bool remove(int id) = 0;
};

// історія статусів
class IStatusHistoryRepository
{
public:
    virtual ~IStatusHistoryRepository() = default;

    virtual int add(const NotificationStatusHistory& entry) = 0;
    virtual std::vector<NotificationStatusHistory>
        findByNotification(int notificationId) = 0;
};

// sqlite_orm

class SQLiteCategoryRepository : public ICategoryRepository
{
public:
    explicit SQLiteCategoryRepository(Storage& storage);

    int add(const NotificationCategory& category) override;
    std::optional<NotificationCategory> findById(int id) override;
    std::vector<NotificationCategory> findAll() override;
    bool update(const NotificationCategory& category) override;
    bool remove(int id) override;

private:
    Storage& storage_; 
};

class SQLiteNotificationRepository : public INotificationRepository
{
public:
    explicit SQLiteNotificationRepository(Storage& storage);

    int add(const Notification& notification) override;
    std::optional<Notification> findById(int id) override;
    std::vector<Notification> findAll() override;
    bool update(const Notification& notification) override;
    bool remove(int id) override;

private:
    Storage& storage_;
};

class SQLiteStatusHistoryRepository : public IStatusHistoryRepository
{
public:
    explicit SQLiteStatusHistoryRepository(Storage& storage);

    int add(const NotificationStatusHistory& entry) override;
    std::vector<NotificationStatusHistory>
        findByNotification(int notificationId) override;

private:
    Storage& storage_;
};
