set(SOURCES     AxisDetector.cpp
                ButtonMapping.cpp
                ButtonDetector.cpp
                HatDetector.cpp
                KeyDetector.cpp
                MouseButtonDetector.cpp
                PointerDetector.cpp
                PrimitiveDetector.cpp)

set(HEADERS     AxisConfiguration.h
                AxisDetector.h
                ButtonDetector.h
                ButtonMapping.h
                HatDetector.h
                KeyDetector.h
                MouseButtonDetector.h
                PointerDetector.h
                PrimitiveDetector.h)

core_add_library(input_joystick_mapping)
