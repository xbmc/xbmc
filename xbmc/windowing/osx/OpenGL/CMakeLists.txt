if(TARGET ${APP_NAME_LC}::OpenGl)
  list(APPEND SOURCES OSXGLView.mm
                      WindowControllerMacOS.mm
                      WinSystemOSXGL.mm)
  list(APPEND HEADERS OSXGLView.h
                      WindowControllerMacOS.h
                      WinSystemOSXGL.h)

  core_add_library(windowing_osx_opengl)

endif()
