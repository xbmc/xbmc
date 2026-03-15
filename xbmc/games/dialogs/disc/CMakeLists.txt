set(SOURCES DialogGameDiscChanger.cpp
            DialogGameDiscManager.cpp
            DiscManagerActions.cpp
            DiscManagerButtons.cpp
            DiscManagerDiscList.cpp
            DiscManagerGame.cpp
            DiscManagerMenu.cpp
)

set(HEADERS DialogGameDiscChanger.h
            DialogGameDiscManager.h
            DiscManagerActions.h
            DiscManagerButtons.h
            DiscManagerDiscList.h
            DiscManagerGame.h
            DiscManagerMenu.h
)

core_add_library(game_disc_dialog)
