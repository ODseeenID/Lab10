#pragma once

#include "Database.h"
#include "Repositories.h"

// наповнення порожньої бази контрольним набором даних
void seedTestData(ICategoryRepository& categories,
    INotificationRepository& notifications);

// crud через інтерфейси репозиторіїв
void runCrudDemo();

// перевірка, що таблиці й зв'язки створені правильно
void printDatabaseOverview(Storage& storage);

// фільтрація, сортування, агрегати, групування, join
void runQueryDemo(Storage& storage);
void runAggregateDemo(Storage& storage);
void runJoinDemo(Storage& storage);

// транзакції 
void runTransactionDemo(Storage& storage,
    ICategoryRepository& categories,
    INotificationRepository& notifications);

