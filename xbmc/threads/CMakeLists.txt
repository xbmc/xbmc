set(SOURCES Event.cpp
            Thread.cpp
            Timer.cpp)

set(HEADERS Condition.h
            CriticalSection.h
            Event.h
            Lockables.h
            SharedSection.h
            SingleLock.h
            SystemClock.h
            Thread.h
            Timer.h
            IThreadImpl.h
            IRunnable.h)

core_add_library(threads)
