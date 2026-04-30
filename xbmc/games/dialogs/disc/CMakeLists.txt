set(SOURCES DialogGameDiscManager.cpp
            DiscManagerActions.cpp
            DiscManagerButtons.cpp
            DiscManagerDiscList.cpp
            DiscManagerGame.cpp
            DiscManagerMenu.cpp
            DiscManagerSortUtils.cpp
)

set(HEADERS DialogGameDiscManager.h
            DiscManagerActions.h
            DiscManagerButtons.h
            DiscManagerDiscList.h
            DiscManagerGame.h
            DiscManagerIDs.h
            DiscManagerMenu.h
            DiscManagerSortUtils.h
)

core_add_library(game_disc_dialog)
