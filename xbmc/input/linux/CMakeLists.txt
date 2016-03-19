set(SOURCES LinuxInputDevices.cpp
            LIRC.cpp)

set(HEADERS Keymap.h
            LinuxInputDevices.h
            LIRC.h)

core_add_library(input_linux)
