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
            IThreadImpl.h)

if(NOT CORE_SYSTEM_NAME STREQUAL windows AND NOT CORE_SYSTEM_NAME STREQUAL windowsstore)
    list(APPEND SOURCES platform/RecursiveMutex.cpp
                        platform/pthreads/ThreadImpl.cpp)

    list(APPEND HEADERS platform/pthreads/ThreadImpl.h)
else()
    list(APPEND SOURCES platform/win/ThreadImpl.cpp)
    list(APPEND HEADERS platform/win/ThreadImpl.h)
endif()

core_add_library(threads)
