add_subdirectory(libscrobbler)
add_subdirectory(websocket)
add_subdirectory(httprequesthandler)
add_subdirectory(upnp)

if(TARGET_COMMON_DARWIN)
  add_subdirectory(osx)
endif(TARGET_COMMON_DARWIN)

if(TARGET_POSIX)
  add_subdirectory(linux)
endif(TARGET_POSIX)

if(TARGET_WIN32)
  add_subdirectory(windows)
endif(TARGET_WIN32)

find_all_sources(. network_SRCS)
list(REMOVE_ITEM network_SRCS ./UPnP.cpp)
add_sources(${network_SRCS})
