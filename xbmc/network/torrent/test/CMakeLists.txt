set(SOURCES TestTorrentUtils.cpp)

if(TARGET ${APP_NAME_LC}::Libtorrent)
  list(APPEND SOURCES TestLibtorrent.cpp)
endif()

core_add_test_library(torrent_test)
