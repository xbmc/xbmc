set(SOURCES KeyboardEasterEgg.cpp
            KeymapActionMap.cpp
)

set(HEADERS IActionMap.h
            IKeyboardHandler.h
            IKeyboardInputProvider.h
            KeyboardEasterEgg.h
            KeymapActionMap.h
)

core_add_library(input_keyboard)
