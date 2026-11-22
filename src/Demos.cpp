#include "Demos.h"

#include <cstddef>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

// допоміжні речі для цього файлу
namespace
{
    // категорія одним рядком
    void print(const NotificationCategory& c)
    {
        std::cout << c.id << " | " << c.name << " | " << c.description
            << " | " << c.channel << " | " << c.isActive << '\n';
    }

    // те саме для сповіщення
    void print(const Notification& n)
    {
        std::cout << n.id << " | cat " << n.categoryId << " | " << n.title
            << " | prio " << n.priority << " | " << n.status
            << " | retry " << n.retryCount << " | " << n.createdAt << '\n';
    }

    // тестове сповіщення посилається на категорію за номером у списку,
    // а не за id: справжні id ми дізнаємось тільки після вставки
    struct SeedNotification
    {
        std::size_t categoryIndex;
        std::string title;
        int priority;
        std::string status;
        int retryCount;
        std::string createdAt;
    };
}

//  тестові дані

void seedTestData(ICategoryRepository& categories,
    INotificationRepository& notifications)
{
    // 5 категорій з різними каналами й статусами активності
    const std::vector<NotificationCategory> categoryList = {
        {0, "Security",  "Logins and password changes", "email", 1},
        {0, "Payments",  "Payments and invoices",       "sms",   1},
        {0, "Reminders", "Events and tasks",            "push",  1},
        {0, "Marketing", "Promotions and newsletters",  "email", 0},
        {0, "System",    "Technical messages",          "push",  1}
    };

    std::vector<int> categoryIds;
    for (const auto& category : categoryList)
        categoryIds.push_back(categories.add(category));

    // 16 різних сповіщень
    const std::vector<SeedNotification> notificationList = {
        {0, "New login detected",            5, "sent",   0, "2026-11-01 08:15:00"},
        {0, "Password changed",              5, "sent",   1, "2026-11-01 09:40:00"},
        {0, "Suspicious activity",           5, "failed", 3, "2026-11-02 22:05:00"},

        {1, "Payment received",              4, "sent",   0, "2026-11-02 10:30:00"},
        {1, "Invoice due",                   3, "new",    0, "2026-11-03 07:00:00"},
        {1, "Payment declined",              4, "failed", 2, "2026-11-03 12:20:00"},
        {1, "Subscription expiring",         2, "read",   0, "2026-11-04 09:00:00"},

        {2, "Meeting at 2 PM",               3, "sent",   0, "2026-11-04 13:00:00"},
        {2, "Submit the lab report",         3, "new",    0, "2026-11-05 08:00:00"},
        {2, "Colleague's birthday",          1, "read",   0, "2026-11-05 09:30:00"},

        {3, "20% discount",                  1, "new",    0, "2026-11-05 11:00:00"},
        {3, "Weekly news digest",            1, "sent",   1, "2026-11-06 10:00:00"},
        {3, "Personal offer",                2, "failed", 4, "2026-11-06 15:45:00"},

        {4, "Scheduled server maintenance",  2, "sent",   0, "2026-11-06 18:00:00"},
        {4, "App update available",          2, "read",   0, "2026-11-07 08:10:00"},
        {4, "Sync error",                    4, "new",    1, "2026-11-07 09:20:00"}
    };

    for (const auto& item : notificationList)
    {
        notifications.add({ 0,
                           categoryIds[item.categoryIndex],
                           item.title,
                           item.priority,
                           item.status,
                           item.retryCount,
                           item.createdAt });
    }

    std::cout << "Test data prepared: " << categoryList.size()
        << " categories, " << notificationList.size()
        << " notifications\n";
}


// crud


void runCrudDemo()
{
    std::cout << " CRUD DEMONSTRATION \n\n";
    const std::string dbFile = "notifications_crud.db";
    std::filesystem::remove(dbFile);

    Storage storage = makeStorage(dbFile);
    storage.sync_schema(); 

    std::unique_ptr<ICategoryRepository> categories =
        std::make_unique<SQLiteCategoryRepository>(storage);
    std::unique_ptr<INotificationRepository> notifications =
        std::make_unique<SQLiteNotificationRepository>(storage);

    //create: додаємо дві категорії
    int securityId = categories->add(
        { 0, "Security", "Logins and password changes", "email", 1 });
    int paymentsId = categories->add(
        { 0, "Payments", "Payments and invoices", "sms", 1 });
    std::cout << "Created categories with id " << securityId
        << " and " << paymentsId << "\n\n";

    //read: пошук за id і повний список 
    std::cout << "findById(" << securityId << "):\n";
    if (auto c = categories->findById(securityId)) print(*c);

    std::cout << "\nfindById(999): "
        << (categories->findById(999) ? "found" : "not found") << '\n';

    std::cout << "\nfindAll:\n";
    for (const auto& c : categories->findAll()) print(c);

    //update: міняємо опис і вимикаємо категорію
    auto payments = categories->findById(paymentsId);
    payments->description = "Payments, invoices and refunds";
    payments->isActive = 0;

    std::cout << "\nupdate existing: " << std::boolalpha
        << categories->update(*payments) << '\n';

    NotificationCategory ghost;
    ghost.id = 999;
    std::cout << "update missing:  " << categories->update(ghost) << '\n';

    std::cout << "after update:\n";
    print(*categories->findById(paymentsId));

    // залежна сутність: додаємо й оновлюємо сповіщення 
    int notificationId = notifications->add(
        { 0, securityId, "New login detected", 5, "new", 0, "2026-11-01 08:15:00" });

    auto notification = notifications->findById(notificationId);
    notification->status = "sent";
    notification->retryCount = 1;
    notifications->update(*notification);

    std::cout << "\nnotification after update:\n";
    print(*notifications->findById(notificationId));

    // delete: категорію зі сповіщеннями видалити не вийде
    std::cout << "\nremove category that still has notifications:\n";
    try
    {
        categories->remove(securityId);
        std::cout << "removed (unexpected!)\n";
    }
    catch (const std::exception& ex)
    {
        std::cout << "rejected: " << ex.what() << '\n';
    }

    std::cout << "\nremove notification: " << notifications->remove(notificationId) << '\n';
    std::cout << "remove category:     " << categories->remove(securityId) << '\n';
    std::cout << "remove missing:      " << categories->remove(999) << '\n';
    std::cout << "categories left:     " << categories->findAll().size() << '\n';
}

// перевірка структури бази


void printDatabaseOverview(Storage& storage)
{
    std::cout << "\n DATABASE OVERVIEW \n" << std::boolalpha;

    std::cout << "Table notification_category exists: "
        << storage.table_exists("notification_category") << '\n';
    std::cout << "Table notification exists: "
        << storage.table_exists("notification") << '\n';

    std::cout << "Categories: " << storage.count<NotificationCategory>() << '\n';
    std::cout << "Notifications: " << storage.count<Notification>() << '\n';

    // перевіряємо зв'язок 1:N 
    int orphans = 0;
    for (const auto& n : storage.get_all<Notification>())
    {
        if (!storage.get_optional<NotificationCategory>(n.categoryId))
            ++orphans;
    }
    std::cout << "Notifications without parent category: " << orphans << '\n';
    // перевіряємо зовнішній ключ  
    try
    {
        storage.insert(Notification{
            0, -1, "Invalid notification", 1, "new", 0, "2026-11-08 09:00:00" });
        std::cout << "Foreign key test: unexpected insertion success\n";
    }
    catch (const std::exception& ex)
    {
        std::cout << "Foreign key test: insertion rejected: " << ex.what() << '\n';
    }

}

//фільтрація, сортування, обмеження

void runQueryDemo(Storage& storage)
{
    using namespace sqlite_orm;

    std::cout << "\n FILTER, SORT, LIMIT \n";

    //фільтр за 1 полем
    std::cout << "\n1. Failed notifications:\n";
    auto failed = storage.get_all<Notification>(
        where(c(&Notification::status) == "failed"));
    for (const auto& n : failed) print(n);

    //фільтр за 2 умовами
    std::cout << "\n2. Failed notifications with priority >= 4:\n";
    auto urgentFailed = storage.get_all<Notification>(
        where(c(&Notification::status) == "failed" &&
            c(&Notification::priority) >= 4));
    for (const auto& n : urgentFailed) print(n);

    //фільтр плюс сортування
    std::cout << "\n3. Active categories sorted by name:\n";
    auto activeCategories = storage.get_all<NotificationCategory>(
        where(c(&NotificationCategory::isActive) == 1),
        order_by(&NotificationCategory::name).asc());
    for (const auto& category : activeCategories) print(category);

    //сортування й limit
    std::cout << "\n4. Top 3 notifications by retry count:\n";
    auto mostRetried = storage.get_all<Notification>(
        order_by(&Notification::retryCount).desc(),
        limit(3));
    for (const auto& n : mostRetried) print(n);
}

// агрегати й групування


void runAggregateDemo(Storage& storage)
{
    using namespace sqlite_orm;

    std::cout << "\n AGGREGATE QUERIES\n"
        << std::fixed << std::setprecision(2);

    // count
    std::cout << "\nTotal notifications: "
        << storage.count<Notification>() << '\n';
    std::cout << "Failed notifications: "
        << storage.count<Notification>(
            where(c(&Notification::status) == "failed")) << '\n';

    // avg
    std::cout << "Average priority: "
        << storage.avg(&Notification::priority) << '\n';

    // sum
    auto totalRetries = storage.sum(&Notification::retryCount);
    std::cout << "Total retries: "
        << (totalRetries ? static_cast<int>(*totalRetries) : 0) << '\n';

    // group by
    std::cout << "\nStatistics per category "
        "(category id | count | avg priority | total retries):\n";

    auto stats = storage.select(
        columns(&Notification::categoryId,
            count(&Notification::id),
            avg(&Notification::priority),
            sum(&Notification::retryCount)),
        group_by(&Notification::categoryId),
        order_by(&Notification::categoryId));

    // select
    for (const auto& [categoryId, notificationCount, averagePriority, retries] : stats)
    {
        std::cout << categoryId << " | " << notificationCount << " | "
            << averagePriority << " | "
            << (retries ? static_cast<int>(*retries) : 0) << '\n';
    }
}


// join


void runJoinDemo(Storage& storage)
{
    using namespace sqlite_orm;

    std::cout << "\nJOIN QUERIES\n";
    std::cout << "\n1. Failed notifications with their category "
        "(category | channel | id | title | priority | retries):\n";

    auto failedWithCategory = storage.select(
        columns(&NotificationCategory::name,
            &NotificationCategory::channel,
            &Notification::id,
            &Notification::title,
            &Notification::priority,
            &Notification::retryCount),
        inner_join<Notification>(
            on(c(&NotificationCategory::id) == &Notification::categoryId)),
        where(c(&Notification::status) == "failed"),
        order_by(&Notification::id));

    for (const auto& [category, channel, id, title, priority, retries] : failedWithCategory)
    {
        std::cout << category << " | " << channel << " | " << id << " | "
            << title << " | " << priority << " | " << retries << '\n';
    }

    std::cout << "\n2. Notifications of inactive categories "
        "(category | title | priority | status):\n";

    auto inactive = storage.select(
        columns(&NotificationCategory::name,
            &Notification::title,
            &Notification::priority,
            &Notification::status),
        inner_join<Notification>(
            on(c(&NotificationCategory::id) == &Notification::categoryId)),
        where(c(&NotificationCategory::isActive) == 0),
        order_by(&Notification::id));

    for (const auto& [category, title, priority, status] : inactive)
    {
        std::cout << category << " | " << title << " | "
            << priority << " | " << status << '\n';
    }
}

//  сценарій 6: транзакції

namespace
{
    // "до" і "після" кожного сценарію
    struct DbState
    {
        int categories = 0;
        int notifications = 0;
        int categoriesWithoutNotifications = 0;
    };

    DbState captureState(Storage& storage)
    {
        using namespace sqlite_orm;

        DbState state;
        state.categories = storage.count<NotificationCategory>();
        state.notifications = storage.count<Notification>();

        for (const auto& category : storage.get_all<NotificationCategory>())
        {
            int own = storage.count<Notification>(
                where(c(&Notification::categoryId) == category.id));
            if (own == 0)
                ++state.categoriesWithoutNotifications;
        }
        return state;
    }

    void printState(const std::string& label, const DbState& s)
    {
        std::cout << label << ": categories=" << s.categories
            << ", notifications=" << s.notifications
            << ", categories without notifications="
            << s.categoriesWithoutNotifications << '\n';
    }

    // логічно цілісна дія: зареєструвати нову категорію разом з її першим
    // сповіщенням
    // якщо breakSecondStep == true, друга операція навмисно
    // посилається на категорію, якої не існує
    void registerCategoryWithFirstNotification(ICategoryRepository& categories,
        INotificationRepository& notifications,
        const std::string& name,
        bool breakSecondStep)
    {
        // додаємо категорію
        int categoryId = categories.add(
            { 0, name, "Category registered in transaction demo", "email", 1 });

        // додаємо перше сповіщення цієї категорії
        Notification first;
        first.categoryId = breakSecondStep ? 9999 : categoryId;  // в базі нема
        first.title = "Welcome to " + name;
        first.priority = 3;
        first.status = "new";
        first.retryCount = 0;
        first.createdAt = "2026-11-08 10:00:00";

        notifications.add(first);  
    }
}

void runTransactionDemo(Storage& storage,
    ICategoryRepository& categories,
    INotificationRepository& notifications)
{
    using namespace sqlite_orm;

    std::cout << "\n TRANSACTIONS \n";

    // успішна транзакція
    std::cout << "\nScenario A: successful transaction (COMMIT)\n";
    DbState beforeA = captureState(storage);
    printState("before", beforeA);

    bool committed = storage.transaction([&]
        {
            registerCategoryWithFirstNotification(
                categories, notifications, "Delivery", false);
            return true;
        });

    DbState afterA = captureState(storage);
    printState("after ", afterA);
    std::cout << "transaction committed: " << std::boolalpha << committed << '\n';
    std::cout << "result: both rows saved, database is consistent\n";

    // помилка на другому кроці, відкат
    std::cout << "\nScenario B: second step fails inside transaction (ROLLBACK)\n";
    DbState beforeB = captureState(storage);
    printState("before", beforeB);

    try
    {
        storage.transaction([&]
            {
                registerCategoryWithFirstNotification(
                    categories, notifications, "Billing", true);
                return true;
            });
    }
    catch (const std::exception& ex)
    {
        std::cout << "error caught: " << ex.what() << '\n';
    }

    DbState afterB = captureState(storage);
    printState("after ", afterB);
    std::cout << "category 'Billing' saved: " << std::boolalpha
        << (storage.count<NotificationCategory>(
            where(c(&NotificationCategory::name) == "Billing")) > 0)
        << '\n';
    std::cout << "result: the first step was rolled back, database unchanged\n";

    // те саме, але без транзакції
    std::cout << "\nScenario C: the same failure WITHOUT a transaction\n";
    DbState beforeC = captureState(storage);
    printState("before", beforeC);

    try
    {
        registerCategoryWithFirstNotification(
            categories, notifications, "Surveys", true);
    }
    catch (const std::exception& ex)
    {
        std::cout << "error caught: " << ex.what() << '\n';
    }

    DbState afterC = captureState(storage);
    printState("after ", afterC);
    std::cout << "category 'Surveys' saved: "
        << (storage.count<NotificationCategory>(
            where(c(&NotificationCategory::name) == "Surveys")) > 0)
        << '\n';
    std::cout << "result: step 1 stayed in the database, step 2 did not - "
        "the category exists without its first notification, "
        "so the data is inconsistent\n";
}

