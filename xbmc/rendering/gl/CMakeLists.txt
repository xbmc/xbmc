set(SOURCES RenderSystemGL.cpp
            ../MatrixGL.cpp
            GLShader.cpp)

set(HEADERS RenderSystemGL.h
            ../MatrixGL.h
            GLShader.h)

if(ARCH MATCHES arm AND ENABLE_NEON)
  list(APPEND SOURCES ../MatrixGL.neon.cpp)
  if(NOT DEFINED NEON_FLAGS)
    set_source_files_properties(../MatrixGL.neon.cpp PROPERTIES COMPILE_OPTIONS -mfpu=neon)
  endif()
endif()

core_add_library(rendering_gl)
