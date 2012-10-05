find_all_sources(. dbwrappers_SRCS)
list(REMOVE_ITEM dbwrappers_SRCS ./mysqldataset.cpp)
add_sources(${dbwrappers_SRCS})
