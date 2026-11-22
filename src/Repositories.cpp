#include "Repositories.h"

// SQLiteCategoryRepository 

SQLiteCategoryRepository::SQLiteCategoryRepository(Storage& storage)
    : storage_(storage)
{
}

int SQLiteCategoryRepository::add(const NotificationCategory& category)
{
    return storage_.insert(category);
}

std::optional<NotificationCategory>
SQLiteCategoryRepository::findById(int id)
{
    return storage_.get_optional<NotificationCategory>(id);
}

std::vector<NotificationCategory> SQLiteCategoryRepository::findAll()
{
    return storage_.get_all<NotificationCategory>();
}

bool SQLiteCategoryRepository::update(const NotificationCategory& category)
{
    if (!storage_.get_optional<NotificationCategory>(category.id))
        return false;

    storage_.update(category);
    return true;
}

bool SQLiteCategoryRepository::remove(int id)
{
    if (!storage_.get_optional<NotificationCategory>(id))
        return false;

    storage_.remove<NotificationCategory>(id);
    return true;
}

// SQLiteNotificationRepository 

SQLiteNotificationRepository::SQLiteNotificationRepository(Storage& storage)
    : storage_(storage)
{
}

int SQLiteNotificationRepository::add(const Notification& notification)
{
    return storage_.insert(notification);
}

std::optional<Notification> SQLiteNotificationRepository::findById(int id)
{
    return storage_.get_optional<Notification>(id);
}

std::vector<Notification> SQLiteNotificationRepository::findAll()
{
    return storage_.get_all<Notification>();
}

bool SQLiteNotificationRepository::update(const Notification& notification)
{
    if (!storage_.get_optional<Notification>(notification.id))
        return false;

    storage_.update(notification);
    return true;
}

bool SQLiteNotificationRepository::remove(int id)
{
    if (!storage_.get_optional<Notification>(id))
        return false;

    storage_.remove<Notification>(id);
    return true;
}

// SQLiteStatusHistoryRepository

SQLiteStatusHistoryRepository::SQLiteStatusHistoryRepository(Storage& storage)
    : storage_(storage)
{
}

int SQLiteStatusHistoryRepository::add(const NotificationStatusHistory& entry)
{
    return storage_.insert(entry);
}

std::vector<NotificationStatusHistory>
SQLiteStatusHistoryRepository::findByNotification(int notificationId)
{
    using namespace sqlite_orm;

    return storage_.get_all<NotificationStatusHistory>(
        where(c(&NotificationStatusHistory::notificationId) == notificationId),
        order_by(&NotificationStatusHistory::id));
}
