set(SOURCES KeyboardEasterEgg.cpp
)

set(HEADERS interfaces/IKeyboardDriverHandler.h
            interfaces/IKeyboardInputHandler.h
            interfaces/IKeyboardInputProvider.h
            KeyboardEasterEgg.h
            KeyboardTypes.h
)

core_add_library(input_keyboard)
