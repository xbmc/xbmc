set(SOURCES "")
set(HEADERS "")

if(TARGET LIRCCLIENT::LIRCCLIENT)
  list(APPEND SOURCES LIRC.cpp)
  list(APPEND HEADERS LIRC.h)
endif()

if("gbm" IN_LIST CORE_PLATFORM_NAME_LC)
  if(LIBINPUT_FOUND)
    list(APPEND SOURCES LibInputHandler.cpp
                        LibInputKeyboard.cpp
                        LibInputPointer.cpp
                        LibInputSettings.cpp
                        LibInputTouch.cpp)

    list(APPEND HEADERS LibInputHandler.h
                        LibInputKeyboard.h
                        LibInputPointer.h
                        LibInputSettings.h
                        LibInputTouch.h)
  endif()
endif()

if(SOURCES)
  core_add_library(input_linux)
endif()
