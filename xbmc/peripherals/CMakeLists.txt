set(SOURCES EventPollHandle.cpp
            EventScanner.cpp
            Peripherals.cpp)

set(HEADERS EventPollHandle.h
            EventScanner.h
            Peripherals.h
            PeripheralTypes.h)

core_add_library(peripherals)
