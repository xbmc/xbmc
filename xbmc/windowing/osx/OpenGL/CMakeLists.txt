if(OPENGL_FOUND)
  set(SOURCES WinSystemOSXGL.mm)
  set(HEADERS WinSystemOSXGL.h)

  if(NOT SDL_FOUND)
    list(APPEND SOURCES OSXGLView.mm
                        OSXGLWindow.mm)
    list(APPEND HEADERS OSXGLView.h
                        OSXGLWindow.h)
  endif()

  core_add_library(windowing_osx_opengl)

endif()
