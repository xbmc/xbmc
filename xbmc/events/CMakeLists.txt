set(SOURCES AddonEvent.cpp
            AddonManagementEvent.cpp
            BaseEvent.cpp
            EventLog.cpp
            EventLogManager.cpp
            MediaLibraryEvent.cpp)

set(HEADERS AddonEvent.h
            AddonManagementEvent.h
            BaseEvent.h
            EventLog.h
            EventLogManager.h
            IEvent.h
            MediaLibraryEvent.h
            NotificationEvent.h
            UniqueEvent.h)

core_add_library(events)
