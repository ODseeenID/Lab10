#pragma once

#include <stdexcept>
#include <string>

#include <sqlite_orm/sqlite_orm.h>

#include "Models.h"

inline auto makeStorage(const std::string& path)
{
    using namespace sqlite_orm;

    auto storage = make_storage(
        path,

        // таблиця категорій: поле структури -> стовпець
        make_table(
            "notification_category",
            make_column("id", &NotificationCategory::id,
                primary_key().autoincrement()),
            make_column("name", &NotificationCategory::name, unique()),
            make_column("description", &NotificationCategory::description),
            make_column("channel", &NotificationCategory::channel),
            make_column("is_active", &NotificationCategory::isActive,
                default_value(1))),

        // таблиця сповіщень із зовнішнім ключем на категорію
        make_table(
            "notification",
            make_column("id", &Notification::id,
                primary_key().autoincrement()),
            make_column("category_id", &Notification::categoryId),
            make_column("title", &Notification::title),
            make_column("priority", &Notification::priority),
            make_column("status", &Notification::status),
            make_column("retry_count", &Notification::retryCount,
                default_value(0)),
            make_column("created_at", &Notification::createdAt),
            foreign_key(&Notification::categoryId)
            .references(&NotificationCategory::id)),

        // історія змін статусу сповіщень 
        make_table(
            "notification_status_history",
            make_column("id", &NotificationStatusHistory::id,
                primary_key().autoincrement()),
            make_column("notification_id", &NotificationStatusHistory::notificationId),
            make_column("old_status", &NotificationStatusHistory::oldStatus),
            make_column("new_status", &NotificationStatusHistory::newStatus),
            make_column("changed_at", &NotificationStatusHistory::changedAt),
            foreign_key(&NotificationStatusHistory::notificationId)
            .references(&Notification::id)));

    storage.on_open = [](sqlite3* db)
        {
            if (sqlite3_exec(db, "PRAGMA foreign_keys = ON;",
                nullptr, nullptr, nullptr) != SQLITE_OK)
            {
                throw std::runtime_error("cannot enable foreign key checks");
            }
        };

    return storage;
}

using Storage = decltype(makeStorage(std::string{}));

