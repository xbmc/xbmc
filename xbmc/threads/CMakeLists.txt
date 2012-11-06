add_subdirectory(platform)
find_all_sources(. threads_SRCS)
list(APPEND threads_SRCS ./SingleLock.h)
add_sources(${threads_SRCS})
