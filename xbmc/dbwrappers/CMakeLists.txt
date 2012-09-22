aux_source_directory(. dbwrappers_SRCS)
list(REMOVE_ITEM dbwrappers_SRCS ./mysqldataset.cpp)
add_sources(${dbwrappers_SRCS})
