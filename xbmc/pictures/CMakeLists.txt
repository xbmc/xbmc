find_all_sources(. pictures_SRCS)
list(REMOVE_ITEM pictures_SRCS ./PictureThumbLoader.cpp)
add_sources(${pictures_SRCS})
