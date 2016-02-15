set(SOURCES Atomics.cpp
            Event.cpp
            Thread.cpp
            Timer.cpp
            SystemClock.cpp
            platform/Implementation.cpp)

set(HEADERS Atomics.h
            Condition.h
            CriticalSection.h
            Event.h
            Helpers.h
            Lockables.h
            MipsAtomics.h
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

core_add_library(threads)
