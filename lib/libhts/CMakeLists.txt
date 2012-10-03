
include_directories(${root}/xbmc/osx)

aux_source_directory(. hts_SRCS)
list(REMOVE_ITEM hts_SRCS ./net_winsock.c)

add_library(lib_hts STATIC ${hts_SRCS})
