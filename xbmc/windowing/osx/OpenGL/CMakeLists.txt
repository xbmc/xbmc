if(TARGET OpenGL::GL)
  set(SOURCES WinSystemOSXGL.mm)
  set(HEADERS WinSystemOSXGL.h)

  if(NOT SDL_FOUND)
    list(APPEND SOURCES OSXGLView.mm
                        WindowControllerMacOS.mm)
    list(APPEND HEADERS OSXGLView.h
                        WindowControllerMacOS.h)
  endif()

  core_add_library(windowing_osx_opengl)

endif()
