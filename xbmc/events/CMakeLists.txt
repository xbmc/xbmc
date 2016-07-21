set(SOURCES AddonEvent.cpp
            AddonManagementEvent.cpp
            BaseEvent.cpp
            EventLog.cpp
            MediaLibraryEvent.cpp)

set(HEADERS AddonEvent.h
            AddonManagementEvent.h
            BaseEvent.h
            EventLog.h
            IEvent.h
            MediaLibraryEvent.h
            NotificationEvent.h
            UniqueEvent.h)

core_add_library(events)
