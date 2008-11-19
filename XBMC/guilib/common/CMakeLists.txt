SET(SRCS SDLMouse.cpp Mouse.cpp SDLKeyboard.cpp SDLJoystick.cpp Keyboard.cpp)

INCLUDE_DIRECTORIES(../)

ADD_LIBRARY(common STATIC ${SRCS})
