# NotificationORM система обробки сповіщень.
Основна сутність: **категорія сповіщень** (`notification_category`), залежна: **сповіщення** (`notification`), зв'язок 1:N.

## Структура проєкту

```
include/
  Models.h          структури NotificationCategory і Notification
  Database.h        відображення структур на таблиці (make_storage), тип Storage
  Repositories.h    інтерфейси репозиторіїв і їхні реалізації на sqlite_orm
  Demos.h           оголошення демонстраційних сценаріїв
  NotificationState.h  State: інтерфейс стану, контекст, помилка переходу
  PriorityStrategy.h   Strategy: інтерфейс, три алгоритми, контекст-калькулятор
  Events.h             Observer: події, інтерфейс спостерігача, видавець
  Observers.h          підписники: журнал сповіщень, статистика доставки
  NotificationService.h  бізнес-логіка: об'єднує патерни й репозиторії
  NotificationFacade.h Facade: єдина точка входу до підсистеми
  PatternsDemo.h       демонстрація патернів
src/
  main.cpp          послідовний запуск сценаріїв
  Repositories.cpp  реалізація репозиторіїв
  Demos.cpp         тестові дані, CRUD, запити, агрегати, JOIN, транзакції
  capi_demo.cpp     порівняння SQLite C API
  orm_demo.cpp      порівняння sqlite_orm
  NotificationState.cpp, PriorityStrategy.cpp, Observers.cpp,
  NotificationService.cpp, NotificationFacade.cpp, PatternsDemo.cpp   реалізація патернів і демонстрація
tests/
  PatternTests.cpp  модульні тести (Google Test)
vcpkg.json          
CMakeLists.txt      
CMakePresets.json   
```

## Запуск

| Файл | Що робить |
|---|---|
| `build\NotificationORM.exe` | основна програма: CRUD через репозиторії, запити, агрегати, JOIN, транзакції |
| `build\CApiDemo.exe` | додавання й пошук запису через SQLite C API |
| `build\OrmDemo.exe` | ті самі операції через sqlite_orm |

Кожна програма створює свої файли `*.db` у поточній папці й видаляє їх на старті,
тому повторні запуски не накопичують дані.

## Що демонструє основна програма

1. CRUD категорій і сповіщень через інтерфейси `ICategoryRepository` та `INotificationRepository`.
2. Перевірка таблиць і зв'язків після `sync_schema()` (5 категорій, 16 сповіщень).
3. Фільтрація, сортування, `LIMIT`.
4. Агрегати `COUNT`, `AVG`, `SUM` та `GROUP BY`.
5. `INNER JOIN` із полями обох таблиць.
6. Транзакції: успішна (COMMIT), із відкатом (ROLLBACK) і та сама помилка без транзакції.

## Лаба 10: 

| Патерн | Де | Роль у системі |
|---|---|---|
| State | `NotificationState.*` | стани new -> sent -> delivered, окремий стан failed; недопустимі переходи відхиляються |
| Strategy | `PriorityStrategy.*` | пріоритет сповіщення: max, weighted, normalized; вибір і заміна під час роботи |
| Observer | `Events.h`, `Observers.*` | події NotificationCreated і DeliveryCompleted; підписники AuditLogObserver і DeliveryStatsObserver |
| Facade | `NotificationFacade.*` | один клас створює й з'єднує підсистему; `dispatch()` замінює кроки send + delivered/failed |

Зміна стану і запис у таблицю `notification_status_history` виконуються в одній
транзакції SQLite; події публікуються лише після COMMIT.

Допустимі переходи: `new -> sent`, `sent -> delivered`, `sent -> failed`,
`failed -> sent` (повтор, не більше `kMaxRetries` разів). Значення `read` із
тестових даних лаби 8 трактується як `delivered`.

Тести: `build\PatternTests.exe`. 
Залежності: `vcpkg install` підтягне `gtest`.

