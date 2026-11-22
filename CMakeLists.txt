cmake_minimum_required(VERSION 3.21)
project(NotificationORM LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(SqliteOrm CONFIG REQUIRED)
find_package(unofficial-sqlite3 CONFIG REQUIRED)
find_package(GTest CONFIG REQUIRED)

# репозиторії, патерни State, Strategy,Observer, сервіс, Facade
add_library(NotificationCore STATIC
    src/Repositories.cpp
    src/NotificationState.cpp
    src/PriorityStrategy.cpp
    src/Observers.cpp
    src/NotificationService.cpp
    src/NotificationFacade.cpp
)

target_include_directories(NotificationCore
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(NotificationCore
    PUBLIC
        sqlite_orm::sqlite_orm
        unofficial::sqlite3::sqlite3
)

# Основна програма: запити, агрегати, JOIN, транзакції, демонстрація патернів
add_executable(NotificationORM
    src/main.cpp
    src/Demos.cpp
    src/PatternsDemo.cpp
)

target_link_libraries(NotificationORM
    PRIVATE
        NotificationCore
)

# Модульні тести
enable_testing()

add_executable(PatternTests
    tests/PatternTests.cpp
)

target_link_libraries(PatternTests
    PRIVATE
        NotificationCore
        GTest::gtest_main
)

add_test(NAME PatternTests COMMAND PatternTests)

#  SQLite C API
add_executable(CApiDemo
    src/capi_demo.cpp
)

target_include_directories(CApiDemo
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(CApiDemo
    PRIVATE
        unofficial::sqlite3::sqlite3
)

# sqlite_orm
add_executable(OrmDemo
    src/orm_demo.cpp
)

target_include_directories(OrmDemo
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(OrmDemo
    PRIVATE
        sqlite_orm::sqlite_orm
        unofficial::sqlite3::sqlite3
)
