#pragma once

#include "Database.h"
#include "Repositories.h"

//  State, Strategy, Observer на даних основної бази
void runPatternsDemo(Storage& storage,
    ICategoryRepository& categories,
    INotificationRepository& notifications);

// те саме через Facade
void runFacadeDemo(Storage& storage);

