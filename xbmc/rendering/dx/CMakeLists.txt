set(SOURCES GUIWindowTestPatternDX.cpp
            RenderSystemDX.cpp)

set(HEADERS GUIWindowTestPatternDX.h
            RenderSystemDX.h)

core_add_library(rendering_dx)

if(CORE_SYSTEM_NAME STREQUAL windows)
  add_dependencies(rendering_dx d3dx11effects)
endif()
