include_directories(.)
find_all_sources(. squish_SRCS)
add_library(lib_squish STATIC ${squish_SRCS})
