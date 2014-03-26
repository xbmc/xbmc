set(main_SRCS main.cpp)
if(TARGET_COMMON_DARWIN)
  list(APPEND main_SRCS osx/SDLMain.mm)
  list(APPEND main_SRCS osx/SDLMain.h)
endif(TARGET_COMMON_DARWIN)

add_library(xbmcmain OBJECT ${main_SRCS})
