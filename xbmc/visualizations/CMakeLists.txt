set(CMAKE_SHARED_MODULE_SUFFIX ".vis")
set(CMAKE_SHARED_MODULE_PREFIX "")
if(NOT WIN32)
  add_definitions(-DHAS_SDL -DHAS_SDL_OPENGL)
endif()

add_subdirectory(WaveForm)

if(NOT WIN32)

  if(TARGET_RPI)
    include_directories(
     ${root}/xbmc/visualizations/EGLHelpers
    )
    add_subdirectory(EGLHelpers)
  else()
    add_subdirectory(XBMCProjectM)
  endif()

  add_subdirectory(OpenGLSpectrum)
  add_subdirectory(XBMCProjectM)
  
  if (NOT APPLE)
    add_subdirectory(Goom)
  endif()
else()
  add_subdirectory(Milkdrop)
  add_subdirectory(DirectXSpectrum)
endif()
