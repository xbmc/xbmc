set(SOURCES ButtonMapping.cpp
            DriverReceiving.cpp
            FeatureHandling.cpp
            InputHandling.cpp)

set(HEADERS ButtonMapping.h
            DriverReceiving.h
            FeatureHandling.h
            InputHandling.h)

core_add_library(input_joystick_generic)
