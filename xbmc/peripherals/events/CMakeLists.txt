set(SOURCES EventLockHandle.cpp
            EventPollHandle.cpp
            EventScanner.cpp
)

set(HEADERS EventLockHandle.h
            EventPollHandle.h
            EventScanner.h
)

core_add_library(peripherals_events)
