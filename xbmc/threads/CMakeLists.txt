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
            Timer.h
            platform/ThreadImpl.h)

core_add_library(threads)
