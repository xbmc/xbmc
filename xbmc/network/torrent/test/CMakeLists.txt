if(TARGET ${APP_NAME_LC}::Libtorrent)
  set(SOURCES TestLibtorrent.cpp)

  core_add_test_library(torrent_test)
endif()
