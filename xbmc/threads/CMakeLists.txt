set(SOURCES Atomics.cpp
            Event.cpp
            Thread.cpp
            Timer.cpp
            SystemClock.cpp)

set(HEADERS Atomics.h
            Condition.h
            CriticalSection.h
            Event.h
            Lockables.h
            SharedSection.h
            SingleLock.h
            SystemClock.h
            Thread.h
            Timer.h
            platform/RecursiveMutex.h
            platform/ThreadImpl.h)

if(NOT CORE_SYSTEM_NAME STREQUAL windows AND NOT CORE_SYSTEM_NAME STREQUAL windowsstore)
    list(APPEND SOURCES platform/RecursiveMutex.cpp)
endif()

core_add_library(threads)
