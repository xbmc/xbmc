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
            IThreadImpl.h)

core_add_library(threads)
