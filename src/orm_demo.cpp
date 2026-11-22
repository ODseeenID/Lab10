#include <cstdio>
#include <iostream>

#include "Database.h"

namespace
{
    void print(const NotificationCategory& c)
    {
        std::cout << c.id << " | " << c.name << " | " << c.description
            << " | " << c.channel << " | " << c.isActive << '\n';
    }
}

int main()
{
    // чистий файл на кожен запуск
    const char* dbFile = "notifications_orm.db";
    std::remove(dbFile);

    try
    {
        auto storage = makeStorage(dbFile);

        // обидві таблиці з описаного відображення
        storage.sync_schema();

        int id1 = storage.insert(NotificationCategory{
            0, "Security", "Logins and password changes", "email", 1 });
        int id2 = storage.insert(NotificationCategory{
            0, "Payments", "Payments and invoices", "sms", 1 });
        std::cout << "Added categories with id " << id1 << " and " << id2 << "\n\n";

        std::cout << "Find by id = 1:\n";
        if (auto c = storage.get_pointer<NotificationCategory>(1))
            print(*c);

        std::cout << "\nFind by id = 999:\n";
        if (auto c = storage.get_pointer<NotificationCategory>(999))
            print(*c);
        else
            std::cout << "Not found\n";

        std::cout << "\nAll categories:\n";
        for (const auto& c : storage.get_all<NotificationCategory>(
            sqlite_orm::order_by(&NotificationCategory::id)))
            print(c);

        storage.insert(Notification{
            0, id1, "New login detected", 5, "sent", 0, "2026-11-01 08:15:00" });
        std::cout << "\nNotifications in table: "
            << storage.count<Notification>() << '\n';
    }
    catch (const std::exception& ex)
    {
        std::cerr << "Error: " << ex.what() << '\n';
        return 1;
    }

    return 0;
}
