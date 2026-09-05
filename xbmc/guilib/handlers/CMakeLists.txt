set(SOURCES GUIAnnouncementHandlerContainer.cpp
            sources/GUISourcesAnnouncementHandler.cpp
            player/GUIPlayerAnnouncementHandler.cpp
            settings/GUISettingsAnnouncementHandler.cpp)

set(HEADERS GUIAnnouncementHandlerContainer.h
            sources/GUISourcesAnnouncementHandler.h
            player/GUIPlayerAnnouncementHandler.h
            settings/GUISettingsAnnouncementHandler.h)

core_add_library(guilib_announcement_handlers)
