set(SOURCES CopyFrame.cpp)

set(HEADERS DllLibSSE4.h)

set(CMAKE_POSITION_INDEPENDENT_CODE 1)

core_add_library(sse4 NO_MAIN_DEPENDS)
target_compile_options(sse4 PRIVATE -msse4.1)
