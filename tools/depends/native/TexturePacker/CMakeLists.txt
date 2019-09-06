list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules)

if(APPLE)
  set(CMAKE_FIND_ROOT_PATH ${NATIVEPREFIX})
endif()

find_package(Lzo2 REQUIRED)
find_package(PNG REQUIRED)
find_package(GIF REQUIRED)
find_package(JPEG REQUIRED)

if(GIF_VERSION LESS 4)
  message(FATAL_ERROR "giflib < 4 not supported")
else()
  file(STRINGS ${GIF_INCLUDE_DIR}/gif_lib.h GIFSTRINGS)
  string(REGEX MATCH "GIFLIB_MAJOR ([0-9])" GIFLIB_MAJOR ${GIFSTRINGS})
  if(GIFLIB_MAJOR)
    string(REPLACE " " ";" GIFLIB_MAJOR ${GIFLIB_MAJOR})
    list(GET GIFLIB_MAJOR 1 GIFLIB_MAJOR)
  else()
    set(GIFLIB_MAJOR ${GIF_VERSION})
  endif()
  if(NOT GIFLIB_MAJOR OR GIFLIB_MAJOR LESS 5)
    message(WARNING "giflib${GIFLIB_MAJOR} support is experimental. Consider updating to giflib5")
  endif()
endif()

set(SOURCES src/md5.cpp
            src/DecoderManager.cpp
            src/TexturePacker.cpp
            src/XBTFWriter.cpp
            src/decoder/GIFDecoder.cpp
            src/decoder/GifHelper.cpp
            src/decoder/JPGDecoder.cpp
            src/decoder/PNGDecoder.cpp
            ${CMAKE_SOURCE_DIR}/xbmc/guilib/XBTF.cpp)

set(CMAKE_POSITITION_INDEPENDENT_CODE 1)

add_executable(TexturePacker ${SOURCES})
target_include_directories(TexturePacker
                           PRIVATE ${PNG_INCLUDE_DIRS}
                                    ${JPEG_INCLUDE_DIR}
                                    ${GIF_INCLUDE_DIR}
                                    ${CMAKE_SOURCE_DIR}/xbmc
                                    ${CMAKE_CURRENT_SOURCE_DIR}/src
                                    ${CMAKE_CURRENT_SOURCE_DIR}/src/decoder)
target_link_libraries(TexturePacker
                      PRIVATE ${SYSTEM_LDFLAGS}
                              ${GIF_LIBRARIES}
                              ${PNG_LIBRARIES}
                              ${JPEG_LIBRARIES}
                              ${LZO2_LIBRARIES})
target_compile_options(TexturePacker PRIVATE ${ARCH_DEFINES} ${SYSTEM_DEFINES})
