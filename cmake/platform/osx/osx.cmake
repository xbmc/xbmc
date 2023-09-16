if(NOT APP_RENDER_SYSTEM OR APP_RENDER_SYSTEM STREQUAL "gl")
  list(APPEND PLATFORM_REQUIRED_DEPS OpenGl)
  set(APP_RENDER_SYSTEM gl)
  list(APPEND SYSTEM_DEFINES -DGL_DO_NOT_WARN_IF_MULTI_GL_VERSION_HEADERS_INCLUDED
                             -DGL_SILENCE_DEPRECATION)
else()
  message(SEND_ERROR "Currently only OpenGL rendering is supported. Please set APP_RENDER_SYSTEM to \"gl\"")
endif()

if(NOT APP_WINDOW_SYSTEM OR APP_WINDOW_SYSTEM STREQUAL sdl)
  list(APPEND SYSTEM_DEFINES -DHAS_SDL)
  list(APPEND PLATFORM_REQUIRED_DEPS Sdl)
  list(APPEND CORE_MAIN_SOURCE ${CMAKE_SOURCE_DIR}/xbmc/platform/darwin/osx/SDL/SDLMain.mm
                               ${CMAKE_SOURCE_DIR}/xbmc/platform/posix/main.cpp)
elseif(APP_WINDOW_SYSTEM STREQUAL native)
  # native windowing and input
  list(APPEND CORE_MAIN_SOURCE ${CMAKE_SOURCE_DIR}/xbmc/platform/darwin/osx/XBMCApplication.mm)
else()
  message(SEND_ERROR "Only SDL or native windowing options are supported.")
endif()

set(${CORE_PLATFORM_NAME_LC}_SEARCH_CONFIG NO_DEFAULT_PATH CACHE STRING "")
