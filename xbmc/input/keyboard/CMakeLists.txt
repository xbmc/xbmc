set(SOURCES KeyboardEasterEgg.cpp
            KeymapActionMap.cpp
)

set(HEADERS interfaces/IActionMap.h
            interfaces/IKeyboardDriverHandler.h
            interfaces/IKeyboardInputHandler.h
            interfaces/IKeyboardInputProvider.h
            KeyboardEasterEgg.h
            KeyboardTypes.h
            KeymapActionMap.h
)

core_add_library(input_keyboard)
