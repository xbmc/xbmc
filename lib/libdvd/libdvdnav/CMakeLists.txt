file(GLOB headers src/*h)
file(COPY ${headers} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/../includes/dvdnav)

set(DVDNAV_MAJOR 4)
set(DVDNAV_MINOR 1)
set(DVDNAV_SUB 3)
set(VERSION "${DVDNAV_MAJOR}.${DVDNAV_MINOR}.${DVDNAV_SUB}")

plex_find_headers(HEADERS unistd.h string.h)

add_definitions(-D_FILE_OFFSET_BITS=64 -D_LARGEFILE64_SOURCE -DHAVE_CONFIG_H -D_XBMC)


configure_file(config_plex.h.in config.h)
include_directories(src src/vm ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_BINARY_DIR}/../includes)
set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} ${PLEX_LINK_WRAPPED}")

find_all_sources(src dvdnav_SRC)
find_all_sources(src/vm dvdnavvm_SRC)
add_library(dvdnav-${ARCH} MODULE ${dvdnav_SRC} ${dvdnavvm_SRC})
target_link_libraries(dvdnav-${ARCH} dvdread dvdcss wrapper)

if(APPLE)
  install(TARGETS dvdnav-${ARCH} DESTINATION ${RESOURCEPATH}/system/players/dvdplayer COMPONENT RUNTIME)
elseif(UNIX)
  install(TARGETS dvdnav-${ARCH} DESTINATION ${BINPATH}/system/players/dvdplayer COMPONENT RUNTIME)
elseif(WIN32)
  install(TARGETS dvdnav RUNTIME DESTINATION ${RESOURCEPATH}/system/players/dvdplayer LIBRARY DESTINATION ${RESOURCEPATH}/system COMPONENT RUNTIME)
endif()
