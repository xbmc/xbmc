set(SOURCES CopyFrame.cpp)

set(HEADERS DllLibSSE4.h)

core_add_shared_library(sse4)
set_target_properties(sse4 PROPERTIES FOLDER lib)
target_compile_options(sse4 PRIVATE -msse4.1)
