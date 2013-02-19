include_directories(${root}/xbmc)
if(TARGET_WIN32)
  include_directories(Win32/include)
endif(TARGET_WIN32)

find_all_sources(. hts_SRCS)
if(TARGET_WIN32)
  list(REMOVE_ITEM hts_SRCS ./net_posix.c)
else(TARGET_WIN32)
  list(REMOVE_ITEM hts_SRCS ./net_winsock.c)
endif(TARGET_WIN32)

add_library(lib_hts STATIC ${hts_SRCS})
