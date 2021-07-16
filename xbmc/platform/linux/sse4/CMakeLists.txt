if(HAVE_SSE4_1)
  set(SOURCES SSE4.cpp)
  set(HEADERS SSE4.h)

  core_add_library(sse4)
  target_compile_options(${CORE_LIBRARY} PRIVATE -msse4.1)
endif()
