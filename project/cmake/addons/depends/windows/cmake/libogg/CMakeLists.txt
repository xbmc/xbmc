project(libogg)

cmake_minimum_required(VERSION 2.8)

add_definitions(-DWIN32 -D_WINDOWS -D_USRDLL -DLIBOGG_EXPORTS)

set(LIBOGG_SOURCES src/bitwise.c
                   src/framing.c)

include_directories(${PROJECT_SOURCE_DIR}/include)

add_library(ogg ${LIBOGG_SOURCES})

set(HEADERS ${PROJECT_SOURCE_DIR}/include/ogg/ogg.h
            ${PROJECT_SOURCE_DIR}/include/ogg/os_types.h)
install(FILES ${HEADERS} DESTINATION ${CMAKE_INSTALL_PREFIX}/include/ogg)
install(TARGETS ogg DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
