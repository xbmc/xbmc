set(SOURCES Atomics.cpp
            Event.cpp
            Thread.cpp
            Timer.cpp
            SystemClock.cpp)

set(HEADERS Atomics.h
            Condition.h
            CriticalSection.h
            Event.h
            Helpers.h
            Lockables.h
            SharedSection.h
            SingleLock.h
            SystemClock.h
            Thread.h
            ThreadImpl.h
            ThreadLocal.h
            Timer.h
            platform/Condition.h
            platform/CriticalSection.h
            platform/ThreadImpl.h
            platform/ThreadLocal.h)

if(NOT CORE_SYSTEM_NAME STREQUAL windows AND NOT CORE_SYSTEM_NAME STREQUAL windowsstore)
  list(APPEND SOURCES platform/pthreads/Implementation.cpp)
endif()

core_add_library(threads)
