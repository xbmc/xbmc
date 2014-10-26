project(libvorbis)

cmake_minimum_required(VERSION 2.8)

add_definitions(-DWIN32 -D_WINDOWS -D_USRDLL -DLIBVORBIS_EXPORTS)

set(LIBVROBIS_SOURCES lib/analysis.c
                      lib/bitrate.c
                      lib/block.c
                      lib/codebook.c
                      lib/envelope.c
                      lib/floor0.c
                      lib/floor1.c
                      lib/info.c
                      lib/lookup.c
                      lib/lpc.c
                      lib/lsp.c
                      lib/mapping0.c
                      lib/mdct.c
                      lib/psy.c
                      lib/registry.c
                      lib/res0.c
                      lib/sharedbook.c
                      lib/smallft.c
                      lib/synthesis.c
                      lib/vorbisenc.c
                      lib/window.c)

include_directories(${PROJECT_SOURCE_DIR}/include ${CMAKE_INSTALL_PREFIX}/include)

add_library(vorbis ${LIBVROBIS_SOURCES})

set(HEADERS ${PROJECT_SOURCE_DIR}/include/vorbis/codec.h
            ${PROJECT_SOURCE_DIR}/include/vorbis/vorbisenc.h
            ${PROJECT_SOURCE_DIR}/include/vorbis/vorbisfile.h)
install(FILES ${HEADERS} DESTINATION ${CMAKE_INSTALL_PREFIX}/include/vorbis)
install(TARGETS vorbis DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
