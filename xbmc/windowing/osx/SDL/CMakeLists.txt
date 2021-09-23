if(SDL_FOUND)
  set(SOURCES WinEventsSDL.cpp
              WinSystemOSXSDL.mm)
  set(HEADERS WinEventsSDL.h
              WinSystemOSXSDL.h)
endif()

if(SOURCES)
  core_add_library(windowing_osx_SDL)
endif()
