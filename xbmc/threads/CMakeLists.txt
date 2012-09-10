add_subdirectory(platform)
aux_source_directory(. threads_SRCS)
add_library(threads STATIC ${threads_SRCS})