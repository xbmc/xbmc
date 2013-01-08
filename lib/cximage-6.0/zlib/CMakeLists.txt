find_all_sources(. zlib_SRCS)
list(REMOVE_ITEM zlib_SRCS ./example.c)
list(REMOVE_ITEM zlib_SRCS ./minigzip.c)
add_library(cx_zlib OBJECT ${zlib_SRCS})
