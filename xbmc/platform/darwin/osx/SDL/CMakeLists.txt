if(SDL_FOUND)
  set(SOURCES OSXTextInputResponder.mm)

  set(HEADERS OSXTextInputResponder.h)
endif()

if(SOURCES)
  core_add_library(platform_osx_SDL)
endif()
