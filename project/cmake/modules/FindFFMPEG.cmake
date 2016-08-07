include(ExternalProject)
file(STRINGS ${CORE_SOURCE_DIR}/tools/depends/target/ffmpeg/FFMPEG-VERSION VER)
string(REGEX MATCH "VERSION=[^ ]*$.*" FFMPEG_VER "${VER}")
list(GET FFMPEG_VER 0 FFMPEG_VER)
string(SUBSTRING "${FFMPEG_VER}" 8 -1 FFMPEG_VER)
string(REGEX MATCH "BASE_URL=([^ ]*)" FFMPEG_BASE_URL "${VER}")
list(GET FFMPEG_BASE_URL 0 FFMPEG_BASE_URL)
string(SUBSTRING "${FFMPEG_BASE_URL}" 9 -1 FFMPEG_BASE_URL)


if(ENABLE_INTERNAL_FFMPEG)
  if(FFMPEG_PATH)
    message(WARNING "Internal FFmpeg enabled, but FFMPEG_PATH given, ignoring")
  endif()
  if(CMAKE_CROSSCOMPILING)
    set(CROSS_ARGS -DDEPENDS_PATH=${DEPENDS_PATH}
                   -DPKG_CONFIG_EXECUTABLE=${PKG_CONFIG_EXECUTABLE}
                   -DCROSSCOMPILING=${CMAKE_CROSSCOMPILING}
                   -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
                   -DCORE_SYSTEM_NAME=${CORE_SYSTEM_NAME}
                   -DCPU=${WITH_CPU}
                   -DOS=${OS}
                   -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
                   -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
                   -DCMAKE_AR=${CMAKE_AR}
                   -DCMAKE_C_FLAGS=${CMAKE_C_FLAGS}
                   -DCMAKE_EXE_LINKER_FLAGS=${CMAKE_EXE_LINKER_FLAGS})
  endif()

  externalproject_add(ffmpeg
                      URL ${FFMPEG_BASE_URL}/${FFMPEG_VER}.tar.gz
                      PREFIX ${CORE_BUILD_DIR}/ffmpeg
                      CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}
                                 -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
                                 -DFFMPEG_VER=${FFMPEG_VER}
                                 -DCORE_SYSTEM_NAME=${CORE_SYSTEM_NAME}
                                 ${CROSS_ARGS}
                      PATCH_COMMAND ${CMAKE_COMMAND} -E copy
                                    ${CORE_SOURCE_DIR}/tools/depends/target/ffmpeg/CMakeLists.txt
                                    <SOURCE_DIR> &&
                                    ${CMAKE_COMMAND} -E copy
                                    ${CORE_SOURCE_DIR}/tools/depends/target/ffmpeg/FindGnuTls.cmake
                                    <SOURCE_DIR>)

  file(WRITE ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/ffmpeg/ffmpeg-link-wrapper
"#!/bin/bash
if [[ $@ == *${APP_NAME_LC}.bin* || $@ == *${APP_NAME_LC}.so* || $@ == *${APP_NAME_LC}-test* ]]
then
  avformat=`PKG_CONFIG_PATH=${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/lib/pkgconfig ${PKG_CONFIG_EXECUTABLE} --libs --static libavcodec`
  avcodec=`PKG_CONFIG_PATH=${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/lib/pkgconfig ${PKG_CONFIG_EXECUTABLE} --libs --static libavformat`
  avfilter=`PKG_CONFIG_PATH=${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/lib/pkgconfig ${PKG_CONFIG_EXECUTABLE} --libs --static libavfilter`
  avutil=`PKG_CONFIG_PATH=${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/lib/pkgconfig ${PKG_CONFIG_EXECUTABLE} --libs --static libavutil`
  swscale=`PKG_CONFIG_PATH=${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/lib/pkgconfig ${PKG_CONFIG_EXECUTABLE} --libs --static libswscale`
  swresample=`PKG_CONFIG_PATH=${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/lib/pkgconfig ${PKG_CONFIG_EXECUTABLE} --libs --static libswresample`
  gnutls=`PKG_CONFIG_PATH=${DEPENDS_PATH}/lib/pkgconfig/ ${PKG_CONFIG_EXECUTABLE}  --libs-only-l --static --silence-errors gnutls`
  $@ $avcodec $avformat $avcodec $avfilter $swscale $swresample -lpostproc $gnutls
else
  $@
fi")
  file(COPY ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/ffmpeg/ffmpeg-link-wrapper
       DESTINATION ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}
       FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE)
  set(FFMPEG_LINK_EXECUTABLE "${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/ffmpeg-link-wrapper <CMAKE_CXX_COMPILER> <FLAGS> <CMAKE_CXX_LINK_FLAGS> <LINK_FLAGS> <OBJECTS> -o <TARGET> <LINK_LIBRARIES>" PARENT_SCOPE)
  set(FFMPEG_CREATE_SHARED_LIBRARY "${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/ffmpeg-link-wrapper <CMAKE_CXX_COMPILER> <CMAKE_SHARED_LIBRARY_CXX_FLAGS> <LANGUAGE_COMPILE_FLAGS> <LINK_FLAGS> <CMAKE_SHARED_LIBRARY_CREATE_CXX_FLAGS> <SONAME_FLAG><TARGET_SONAME> -o <TARGET> <OBJECTS> <LINK_LIBRARIES>" PARENT_SCOPE)
  set(FFMPEG_INCLUDE_DIRS ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/include)
  list(APPEND FFMPEG_DEFINITIONS -DFFMPEG_VER_SHA=\"${FFMPEG_VER}\"
                                 -DUSE_STATIC_FFMPEG=1)
  set(FFMPEG_FOUND 1)
else()
  if(FFMPEG_PATH)
    set(ENV{PKG_CONFIG_PATH} "${FFMPEG_PATH}/lib/pkgconfig")
  endif()
  set(FFMPEG_PKGS libavcodec>=56.26.100 libavfilter>=5.11.100 libavformat>=56.25.101
                  libavutil>=54.20.100 libswscale>=3.1.101 libswresample>=1.1.100 libpostproc>=53.3.100)
  if(PKG_CONFIG_FOUND AND NOT WIN32)
    pkg_check_modules (FFMPEG ${FFMPEG_PKGS})
    string(REGEX REPLACE "framework;" "framework " FFMPEG_LDFLAGS "${FFMPEG_LDFLAGS}")
    set(FFMPEG_LIBRARIES ${FFMPEG_LDFLAGS})
    add_custom_target(ffmpeg)
  else()
    find_path(FFMPEG_INCLUDE_DIRS libavcodec/avcodec.h PATH_SUFFIXES ffmpeg)
    find_library(FFMPEG_LIBAVCODEC NAMES avcodec libavcodec PATH_SUFFIXES ffmpeg/libavcodec)
    find_library(FFMPEG_LIBAVFILTER NAMES avfilter libavfilter PATH_SUFFIXES ffmpeg/libavfilter)
    find_library(FFMPEG_LIBAVFORMAT NAMES avformat libavformat PATH_SUFFIXES ffmpeg/libavformat)
    find_library(FFMPEG_LIBAVUTIL NAMES avutil libavutil PATH_SUFFIXES ffmpeg/libavutil)
    find_library(FFMPEG_LIBSWSCALE NAMES swscale libswscale PATH_SUFFIXES ffmpeg/libswscale)
    find_library(FFMPEG_LIBPOSTPROC NAMES postproc libpostproc PATH_SUFFIXES ffmpeg/libpostproc)
    set(FFMPEG_LIBRARIES ${FFMPEG_LIBAVCODEC} ${FFMPEG_LIBAVFILTER} ${FFMPEG_LIBAVFORMAT}
                         ${FFMPEG_LIBAVUTIL} ${FFMPEG_LIBSWSCALE} ${FFMPEG_LIBPOSTPROC})
    add_custom_target(ffmpeg DEPENDS ${FFMPEG_LIBRARIES})
  endif()

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(FFMPEG DEFAULT_MSG FFMPEG_INCLUDE_DIRS FFMPEG_LIBRARIES)
  set(FFMPEG_FOUND 1)
  list(APPEND FFMPEG_DEFINITIONS -DFFMPEG_VER_SHA=\"${FFMPEG_VER}\")
endif()
set_target_properties(ffmpeg PROPERTIES FOLDER "External Projects")

mark_as_advanced(FFMPEG_INCLUDE_DIRS FFMPEG_LIBRARIES FFMPEG_DEFINITIONS FFMPEG_FOUND)
