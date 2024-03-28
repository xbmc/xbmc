if(ENABLE_LIBTORRENT)
  set(SOURCES TestLibtorrent.cpp)

  core_add_test_library(torrent_test)
endif()
