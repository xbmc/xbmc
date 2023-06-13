cmake_minimum_required(VERSION 3.15)
project(TexturePacker)

list(APPEND CMAKE_MODULE_PATH ${KODI_SOURCE_DIR}/cmake/modules)

if(NATIVEPREFIX)
  set(CMAKE_FIND_ROOT_PATH ${NATIVEPREFIX})
endif()

if(ENABLE_STATIC)
  set(CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_STATIC_LIBRARY_SUFFIX})
  # Requires cmake 3.24 for ZLIB_USE_STATIC_LIBS to actually do something
  set(ZLIB_USE_STATIC_LIBS ON)
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

set(SOURCES md5.cpp
            DecoderManager.cpp
            TexturePacker.cpp
            XBTFWriter.cpp
            decoder/GIFDecoder.cpp
            decoder/GifHelper.cpp
            decoder/JPGDecoder.cpp
            decoder/PNGDecoder.cpp
            ${KODI_SOURCE_DIR}/xbmc/guilib/XBTF.cpp)

if(WIN32)
  list(APPEND SOURCES Win32/dirent.c)
endif()

set(CMAKE_POSITITION_INDEPENDENT_CODE 1)

add_executable(TexturePacker ${SOURCES})
target_include_directories(TexturePacker
                           PRIVATE ${PNG_INCLUDE_DIRS}
                                   ${JPEG_INCLUDE_DIR}
                                   ${GIF_INCLUDE_DIR}
                                   ${KODI_SOURCE_DIR}/xbmc
                                   ${CMAKE_CURRENT_SOURCE_DIR}
                                   ${CMAKE_CURRENT_SOURCE_DIR}/decoder)
if(WIN32)
  target_include_directories(TexturePacker SYSTEM
                                           PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Win32)
endif()

target_link_libraries(TexturePacker
                      PRIVATE ${SYSTEM_LDFLAGS}
                              ${GIF_LIBRARIES}
                              ${PNG_LIBRARIES}
                              ${JPEG_LIBRARIES}
                              ${LZO2_LIBRARIES})

target_compile_definitions(TexturePacker PRIVATE ${ARCH_DEFINES} ${SYSTEM_DEFINES})
target_compile_features(TexturePacker PUBLIC cxx_std_17)

install(TARGETS TexturePacker EXPORT TexturePacker
  RUNTIME DESTINATION bin
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib)
