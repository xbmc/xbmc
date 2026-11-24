set(SOURCES AnnouncementEvents.cpp
            AnnouncementManager.cpp
            AnnouncementV14.cpp)

set(HEADERS AnnouncementEvents.h
            AnnouncementManager.h
            AnnouncementMessages.h
            AnnouncementV14.h
            IAnnouncer.h)

core_add_library(interfaces)
