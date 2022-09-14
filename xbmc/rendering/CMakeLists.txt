set(SOURCES RenderSystem.cpp)

set(HEADERS RenderSystem.h
            RenderSystemTypes.h)

if(OPENGL_FOUND OR OPENGLES_FOUND)
  list(APPEND SOURCES MatrixGL.cpp)
  list(APPEND HEADERS MatrixGL.h)

  if(ARCH MATCHES arm AND ENABLE_NEON)
    list(APPEND SOURCES MatrixGL.neon.cpp)
    if(NOT DEFINED NEON_FLAGS)
      set_source_files_properties(MatrixGL.neon.cpp PROPERTIES COMPILE_OPTIONS -mfpu=neon)
    endif()
  endif()
endif()

core_add_library(rendering)
