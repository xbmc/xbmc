set(SOURCES MouseWindowingButtonMap.cpp)

set(HEADERS IMouseButtonMap.h
            IMouseDriverHandler.h
            IMouseInputHandler.h
            IMouseInputProvider.h
            MouseWindowingButtonMap.h)

core_add_library(input_mouse)
