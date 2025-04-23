set(SOURCES AndroidDatabaseManager.cpp
            AndroidSortUtils.cpp
            AndroidTextureManager.cpp
            AndroidPowerManager.cpp
            AndroidTouchInputHandler.cpp
            AndroidTVPerformance.cpp
            AndroidDatabaseOptimizer.cpp)

set(HEADERS AndroidDatabaseManager.h
            AndroidSortUtils.h
            AndroidTextureManager.h
            AndroidPowerManager.h
            AndroidTouchInputHandler.h
            AndroidTVPerformance.h
            AndroidDatabaseOptimizer.h)

core_add_library(android_utils)
