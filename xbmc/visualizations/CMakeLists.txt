set(CMAKE_SHARED_MODULE_SUFFIX ".vis")
set(CMAKE_SHARED_MODULE_PREFIX "")
add_definitions(-DHAS_SDL -DHAS_SDL_OPENGL)


if(NOT WIN32)
  add_subdirectory(WaveForm)
  add_subdirectory(OpenGLSpectrum)
  add_subdirectory(XBMCProjectM)
  
  if (NOT APPLE)
    add_subdirectory(Goom)
  endif()
endif()
