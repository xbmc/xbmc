set(SOURCES MouseStat.cpp
            MouseTranslator.cpp
            MouseWindowingButtonMap.cpp)

set(HEADERS interfaces/IMouseButtonMap.h
            interfaces/IMouseDriverHandler.h
            interfaces/IMouseInputHandler.h
            interfaces/IMouseInputProvider.h
            MouseStat.h
            MouseTranslator.h
            MouseWindowingButtonMap.h)

core_add_library(input_mouse)
