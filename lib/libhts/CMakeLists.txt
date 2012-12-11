include_directories(${root}/xbmc)
if(WIN32)
  include_directories(Win32/include)
endif()

find_all_sources(. hts_SRCS)
if(WIN32)
  list(REMOVE_ITEM hts_SRCS ./net_posix.c)
else()
  list(REMOVE_ITEM hts_SRCS ./net_winsock.c)
endif()

add_library(lib_hts STATIC ${hts_SRCS})
