set(SOURCES GamepadTranslator.cpp
            JoystickMapper.cpp
            KeymapHandler.cpp
            KeymapHandling.cpp
)

set(HEADERS GamepadTranslator.h
            JoystickMapper.h
            KeymapHandler.h
            KeymapHandling.h
)

core_add_library(input_keymaps_joysticks)
