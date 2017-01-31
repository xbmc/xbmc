set(SOURCES WinEvents.cpp
            WinSystem.cpp)

set(HEADERS WindowingFactory.h
            WinEvents.h
            WinSystem.h
            XBMC_events.h
            VideoSync.h)

if (NOT MIR_FOUND)
  if(SDL_FOUND AND NOT X_FOUND)
    list(APPEND SOURCES WinEventsSDL.cpp)
    list(APPEND HEADERS WinEventsSDL.h)
  endif()

  if(X_FOUND)
    list(APPEND SOURCES WinEventsX11.cpp)
    list(APPEND HEADERS WinEventsX11.h)
  endif()

  if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    list(APPEND SOURCES WinEventsLinux.cpp)
    list(APPEND HEADERS WinEventsLinux.h)
  endif()
endif()

core_add_library(windowing)
