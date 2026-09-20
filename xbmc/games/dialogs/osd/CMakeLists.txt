set(SOURCES DialogGameIndicators.cpp
            DialogGameAchievements.cpp
            DialogGameAdvancedSettings.cpp
            DialogGameLeaderboards.cpp
            DialogGameLeaderboardEntries.cpp
            LeaderboardUtils.cpp
            DialogGameOSD.cpp
            DialogGameOSDHelp.cpp
            DialogGameSaves.cpp
            DialogGameStretchMode.cpp
            DialogGameVideoFilter.cpp
            DialogGameVideoRotation.cpp
            DialogGameVideoSelect.cpp
            DialogGameVolume.cpp
            DialogInGameSaves.cpp
)

set(HEADERS DialogGameIndicators.h
            DialogGameAchievements.h
            DialogGameAdvancedSettings.h
            DialogGameLeaderboards.h
            DialogGameLeaderboardEntries.h
            LeaderboardUtils.h
            DialogGameOSD.h
            DialogGameOSDHelp.h
            DialogGameSaves.h
            DialogGameStretchMode.h
            DialogGameVideoFilter.h
            DialogGameVideoRotation.h
            DialogGameVideoSelect.h
            DialogGameVolume.h
            DialogInGameSaves.h
)

core_add_library(gameosddialogs)

if(ENABLE_TESTING)
  add_subdirectory(test)
endif()
