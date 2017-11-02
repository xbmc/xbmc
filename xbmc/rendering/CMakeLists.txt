set(SOURCES RenderSystem.cpp)

set(HEADERS RenderSystem.h
            RenderSystemTypes.h)

core_add_library(rendering)
if(NOT CORE_SYSTEM_NAME STREQUAL windows AND NOT CORE_SYSTEM_NAME STREQUAL windowsstore)
  if(HAVE_SSE)
    target_compile_options(${CORE_LIBRARY} PRIVATE -msse)
  endif()
  if(HAVE_SSE2)
    target_compile_options(${CORE_LIBRARY} PRIVATE -msse2)
  endif()
endif()

