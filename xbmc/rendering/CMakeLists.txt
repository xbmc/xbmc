set(SOURCES RenderSystem.cpp)

set(HEADERS RenderSystem.h
            RenderSystemTypes.h)

if(TARGET OpenGL::GL OR TARGET OpenGL::GLES)
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
if(NOT CORE_SYSTEM_NAME STREQUAL windows AND NOT CORE_SYSTEM_NAME STREQUAL windowsstore)
  if(HAVE_SSE)
    target_compile_options(${CORE_LIBRARY} PRIVATE -msse)
  endif()
  if(HAVE_SSE2)
    target_compile_options(${CORE_LIBRARY} PRIVATE -msse2)
  endif()
endif()

