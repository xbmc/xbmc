set(SOURCES Win32InterfaceForCLog.cpp)

set(HEADERS Win32InterfaceForCLog.h)

# Do not add SSE headers for ARM64
if(NOT CMAKE_GENERATOR_PLATFORM STREQUAL arm64)
  list(APPEND HEADERS gpu_memcpy_sse4.h
                      memcpy_sse2.h)
endif()

core_add_library(platform_win32_utils)
