set(SOURCES EventLockHandle.cpp
            EventPollHandle.cpp
            EventScanner.cpp
            Peripherals.cpp)

set(HEADERS EventLockHandle.h
            EventPollHandle.h
            EventScanner.h
            IEventScannerCallback.h
            Peripherals.h
            PeripheralTypes.h)

core_add_library(peripherals)
