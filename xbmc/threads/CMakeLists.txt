add_subdirectory(platform)
aux_source_directory(. threads_SRCS)
list(APPEND threads_SRCS ./SingleLock.h)
add_library(threads STATIC ${threads_SRCS})
