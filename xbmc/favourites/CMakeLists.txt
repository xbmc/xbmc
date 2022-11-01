set(SOURCES ContextMenus.cpp
            GUIDialogFavourites.cpp
            GUIViewStateFavourites.cpp
            GUIWindowFavourites.cpp
            FavouritesService.cpp
            FavouritesURL.cpp)

set(HEADERS ContextMenus.h
            GUIDialogFavourites.h
            GUIViewStateFavourites.h
            GUIWindowFavourites.h
            FavouritesService.h
            FavouritesURL.h)

core_add_library(favourites)
