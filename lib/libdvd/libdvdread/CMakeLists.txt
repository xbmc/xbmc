file(GLOB headers src/*h)
file(COPY ${headers} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/../includes/dvdread)

set(DVDREAD_MAJOR 4)
set(DVDREAD_MINOR 1)
set(DVDREAD_SUB 3)
set(VERSION ${DVDREAD_MAJOR}.${DVDREAD_MINOR}.${DVDREAD_SUB})

plex_find_headers(HEADER dlfcn.h)

configure_file(config_plex.h.in config.h)
include_directories(${CMAKE_CURRENT_BINARY_DIR})
add_definitions(-DHAVE_CONFIG_H -D_XBMC)

find_all_sources(src dvdread_SRC)
find_all_sources(src/vm dvdreadvm_SRC)
add_library(dvdread STATIC ${dvdread_SRC} ${dvdreadvm_SRC})

