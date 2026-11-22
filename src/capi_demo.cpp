#include <cstdio>
#include <iostream>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include <sqlite3.h>

#include "Models.h"

namespace
{
    using DbHandle = std::unique_ptr<sqlite3, decltype(&sqlite3_close)>;

    void execSql(sqlite3* db, const char* sql)
    {
        char* errMsg = nullptr;

        if (sqlite3_exec(db, sql, nullptr, nullptr, &errMsg) != SQLITE_OK)
        {
            std::string message = errMsg ? errMsg : "unknown error";
            sqlite3_free(errMsg); 
            throw std::runtime_error(message);
        }
    }

    // обидві таблиці 
    void createTables(sqlite3* db)
    {
        execSql(db, "PRAGMA foreign_keys = ON;");

        execSql(db, R"(
            CREATE TABLE IF NOT EXISTS notification_category (
                id          INTEGER PRIMARY KEY AUTOINCREMENT,
                name        TEXT NOT NULL UNIQUE,
                description TEXT NOT NULL,
                channel     TEXT NOT NULL,
                is_active   INTEGER NOT NULL DEFAULT 1
            );
        )");

        execSql(db, R"(
            CREATE TABLE IF NOT EXISTS notification (
                id          INTEGER PRIMARY KEY AUTOINCREMENT,
                category_id INTEGER NOT NULL,
                title       TEXT NOT NULL,
                priority    INTEGER NOT NULL,
                status      TEXT NOT NULL,
                retry_count INTEGER NOT NULL DEFAULT 0,
                created_at  TEXT NOT NULL,
                FOREIGN KEY (category_id)
                    REFERENCES notification_category(id)
            );
        )");
    }

    std::string textColumn(sqlite3_stmt* stmt, int col)
    {
        const unsigned char* text = sqlite3_column_text(stmt, col);
        return text ? reinterpret_cast<const char*>(text) : "";
    }

    NotificationCategory readCategory(sqlite3_stmt* stmt)
    {
        NotificationCategory c;
        c.id = sqlite3_column_int(stmt, 0);
        c.name = textColumn(stmt, 1);
        c.description = textColumn(stmt, 2);
        c.channel = textColumn(stmt, 3);
        c.isActive = sqlite3_column_int(stmt, 4);
        return c;
    }

    int addCategory(sqlite3* db, const NotificationCategory& c)
    {
        const char* sql =
            "INSERT INTO notification_category "
            "(name, description, channel, is_active) "
            "VALUES (?, ?, ?, ?);";

        sqlite3_stmt* stmt = nullptr;

        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
            throw std::runtime_error(sqlite3_errmsg(db));

        sqlite3_bind_text(stmt, 1, c.name.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, c.description.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 3, c.channel.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 4, c.isActive);

        if (sqlite3_step(stmt) != SQLITE_DONE)
        {
            std::string message = sqlite3_errmsg(db);
            sqlite3_finalize(stmt);  
            throw std::runtime_error(message);
        }

        sqlite3_finalize(stmt);
        return static_cast<int>(sqlite3_last_insert_rowid(db));
    }

    std::optional<NotificationCategory> findCategoryById(sqlite3* db, int id)
    {
        const char* sql =
            "SELECT id, name, description, channel, is_active "
            "FROM notification_category WHERE id = ?;";

        sqlite3_stmt* stmt = nullptr;

        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
            throw std::runtime_error(sqlite3_errmsg(db));

        sqlite3_bind_int(stmt, 1, id);

        std::optional<NotificationCategory> result;
        int rc = sqlite3_step(stmt);

        if (rc == SQLITE_ROW)
        {
            result = readCategory(stmt);
        }
        else if (rc != SQLITE_DONE)
        {
            std::string message = sqlite3_errmsg(db);
            sqlite3_finalize(stmt);
            throw std::runtime_error(message);
        }

        sqlite3_finalize(stmt);
        return result;
    }

    std::vector<NotificationCategory> findAllCategories(sqlite3* db)
    {
        const char* sql =
            "SELECT id, name, description, channel, is_active "
            "FROM notification_category ORDER BY id;";

        sqlite3_stmt* stmt = nullptr;

        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
            throw std::runtime_error(sqlite3_errmsg(db));

        std::vector<NotificationCategory> result;
        int rc;

        while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
            result.push_back(readCategory(stmt));

        if (rc != SQLITE_DONE)
        {
            std::string message = sqlite3_errmsg(db);
            sqlite3_finalize(stmt);
            throw std::runtime_error(message);
        }

        sqlite3_finalize(stmt);
        return result;
    }

    void print(const NotificationCategory& c)
    {
        std::cout << c.id << " | " << c.name << " | " << c.description
            << " | " << c.channel << " | " << c.isActive << '\n';
    }
}

int main()
{
    const char* dbFile = "notifications_capi.db";
    std::remove(dbFile);

    sqlite3* raw = nullptr;
    int rc = sqlite3_open(dbFile, &raw);
    DbHandle db(raw, &sqlite3_close);

    if (rc != SQLITE_OK)
    {
        std::cerr << "Cannot open database: " << sqlite3_errmsg(raw) << '\n';
        return 1;
    }

    try
    {
        createTables(db.get());

        int id1 = addCategory(db.get(), { 0, "Security", "Logins and password changes", "email", 1 });
        int id2 = addCategory(db.get(), { 0, "Payments", "Payments and invoices", "sms", 1 });
        std::cout << "Added categories with id " << id1 << " and " << id2 << "\n\n";

        std::cout << "Find by id = 1:\n";
        if (auto c = findCategoryById(db.get(), 1)) print(*c);

        std::cout << "\nFind by id = 999:\n";
        if (auto c = findCategoryById(db.get(), 999)) print(*c);
        else std::cout << "Not found\n";

        std::cout << "\nAll categories:\n";
        for (const auto& c : findAllCategories(db.get())) print(c);
    }
    catch (const std::exception& ex)
    {
        std::cerr << "Error: " << ex.what() << '\n';
        return 1;
    }

    return 0;
}

