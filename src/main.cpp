#include <filesystem>
#include <iostream>
#include <string>

#include "Database.h"
#include "Demos.h"
#include "PatternsDemo.h"
#include "Repositories.h"

int main()
{
    try
    {
        //  crud через інтерфейси
        runCrudDemo();

        // файл щоразу видаляється і створюється щоб тестові дані не накопичувалися між запусками
        std::cout << "\n PREPARING MAIN DATABASE\n";

        const std::string dbFile = "notifications.db";
        std::filesystem::remove(dbFile);

        Storage storage = makeStorage(dbFile);
        storage.open_forever();  
        storage.sync_schema();  

        SQLiteCategoryRepository categories(storage);
        SQLiteNotificationRepository notifications(storage);

        seedTestData(categories, notifications);

        // сценарії
        printDatabaseOverview(storage);
        runQueryDemo(storage);
        runAggregateDemo(storage);
        runJoinDemo(storage);
        runTransactionDemo(storage, categories, notifications);
        runPatternsDemo(storage, categories, notifications);
        runFacadeDemo(storage);

    }
    catch (const std::exception& ex)
    {
        std::cerr << "Application error: " << ex.what() << '\n';
        return 1;
    }

    return 0;
}

