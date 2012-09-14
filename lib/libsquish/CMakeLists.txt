include_directories(.)
aux_source_directory(. squish_SRCS)
add_library(lib_squish STATIC ${squish_SRCS})
