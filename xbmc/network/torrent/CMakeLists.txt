set(SOURCES TorrentUtils.cpp
)

set(HEADERS ILibtorrent.h
            TorrentUtils.h
)

if (ENABLE_LIBTORRENT)
  list(APPEND SOURCES Libtorrent.cpp
                      LibtorrentAlerts.cpp
                      LibtorrentCache.cpp
                      TempDiskIO.cpp
                      TempStorage.cpp
                      Torrent.cpp
                      TorrentPiece.cpp
  )
  list(APPEND HEADERS ILibtorrentAlertHandler.h
                      Libtorrent.h
                      LibtorrentAlerts.h
                      LibtorrentCache.h
                      TempDiskIO.h
                      TempStorage.h
                      Torrent.h
                      TorrentPiece.h
  )
endif()

core_add_library(network_torrent)
