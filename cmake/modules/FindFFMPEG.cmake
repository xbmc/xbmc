# FindFFMPEG
# --------
# Finds FFmpeg libraries
#
# This module will first look for the required library versions on the system.
# If they are not found, it will fall back to downloading and building kodi's own version
#
# --------
# the following variables influence behaviour:
# ENABLE_INTERNAL_FFMPEG - if enabled, kodi's own version will always be built
#
# FFMPEG_PATH - use external ffmpeg not found in system paths
#               usage: -DFFMPEG_PATH=/path/to/ffmpeg_install_prefix
#
# WITH_FFMPEG - use external ffmpeg not found in system paths
#               WARNING: this option is for developers as it will _disable ffmpeg version checks_!
#               Consider using FFMPEG_PATH instead, which _does_ check library versions
#               usage: -DWITH_FFMPEG=/path/to/ffmpeg_install_prefix
#
# --------
# This module will define the following variables:
#
# FFMPEG_FOUND - system has FFmpeg
# FFMPEG_INCLUDE_DIRS - FFmpeg include directory
# FFMPEG_LIBRARIES - FFmpeg libraries
# FFMPEG_DEFINITIONS - pre-processor definitions
# FFMPEG_LDFLAGS - linker flags
#
# and the following imported targets::
#
# ffmpeg  - The FFmpeg libraries
# --------
#

# Macro to build internal FFmpeg
# Refactoring to a macro allows simple fallthrough callback if system ffmpeg failure
macro(buildFFMPEG)
  include(cmake/scripts/common/ModuleHelpers.cmake)

  set(MODULE_LC ffmpeg)

  # We require this due to the odd nature of github URL's compared to our other tarball
  # mirror system. If User sets FFMPEG_URL, allow get_filename_component in SETUP_BUILD_VARS
  if(FFMPEG_URL)
    set(FFMPEG_URL_PROVIDED TRUE)
  endif()

  SETUP_BUILD_VARS()

  if(NOT FFMPEG_URL_PROVIDED)
    # override FFMPEG_URL due to tar naming when retrieved from github release for ffmpeg
    set(FFMPEG_URL ${FFMPEG_BASE_URL}/archive/${FFMPEG_VER}.tar.gz)
  endif()

  if(NOT DAV1D_FOUND)
    message(STATUS "dav1d not found, internal ffmpeg build will be missing AV1 support!")
  endif()

  set(FFMPEG_OPTIONS -DENABLE_CCACHE=${ENABLE_CCACHE}
                     -DCCACHE_PROGRAM=${CCACHE_PROGRAM}
                     -DENABLE_VAAPI=${ENABLE_VAAPI}
                     -DENABLE_VDPAU=${ENABLE_VDPAU}
                     -DENABLE_DAV1D=${DAV1D_FOUND}
                     -DEXTRA_FLAGS=${FFMPEG_EXTRA_FLAGS})

  if(KODI_DEPENDSBUILD)
    set(CROSS_ARGS -DDEPENDS_PATH=${DEPENDS_PATH}
                   -DPKG_CONFIG_EXECUTABLE=${PKG_CONFIG_EXECUTABLE}
                   -DCROSSCOMPILING=${CMAKE_CROSSCOMPILING}
                   -DOS=${OS}
                   -DCMAKE_AR=${CMAKE_AR})
  endif()
  set(LINKER_FLAGS ${CMAKE_EXE_LINKER_FLAGS})
  list(APPEND LINKER_FLAGS ${SYSTEM_LDFLAGS})

  set(CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}
                 -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
                 -DCMAKE_MODULE_PATH=${CMAKE_MODULE_PATH}
                 -DFFMPEG_VER=${FFMPEG_VER}
                 -DCORE_SYSTEM_NAME=${CORE_SYSTEM_NAME}
                 -DCORE_PLATFORM_NAME=${CORE_PLATFORM_NAME_LC}
                 -DCPU=${CPU}
                 -DENABLE_NEON=${ENABLE_NEON}
                 -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
                 -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
                 -DENABLE_CCACHE=${ENABLE_CCACHE}
                 -DCMAKE_C_FLAGS=${CMAKE_C_FLAGS}
                 -DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS}
                 -DCMAKE_EXE_LINKER_FLAGS=${LINKER_FLAGS}
                 ${CROSS_ARGS}
                 ${FFMPEG_OPTIONS}
                 -DPKG_CONFIG_PATH=${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/lib/pkgconfig)
  set(PATCH_COMMAND ${CMAKE_COMMAND} -E copy
                    ${CMAKE_SOURCE_DIR}/tools/depends/target/ffmpeg/CMakeLists.txt
                    <SOURCE_DIR>)

  BUILD_DEP_TARGET()

  if(ENABLE_INTERNAL_DAV1D)
    add_dependencies(ffmpeg dav1d)
  endif()

  find_program(BASH_COMMAND bash)
  if(NOT BASH_COMMAND)
    message(FATAL_ERROR "Internal FFmpeg requires bash.")
  endif()
  file(WRITE ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/ffmpeg/ffmpeg-link-wrapper
"#!${BASH_COMMAND}
if [[ $@ == *${APP_NAME_LC}.bin* || $@ == *${APP_NAME_LC}${APP_BINARY_SUFFIX}* || $@ == *${APP_NAME_LC}.so* || $@ == *${APP_NAME_LC}-test* ]]
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
  set(FFMPEG_DEFINITIONS -DUSE_STATIC_FFMPEG=1)
  set(FFMPEG_FOUND 1)

  set_target_properties(ffmpeg PROPERTIES FOLDER "External Projects")
endmacro()


# Allows building with external ffmpeg not found in system paths,
# without library version checks
if(WITH_FFMPEG)
  set(FFMPEG_PATH ${WITH_FFMPEG})
  message(STATUS "Warning: FFmpeg version checking disabled")
  set(REQUIRED_FFMPEG_VERSION undef)
else()
  # required ffmpeg library versions
  set(REQUIRED_FFMPEG_VERSION 4.4.1)
  set(_avcodec_ver ">=58.134.100")
  set(_avfilter_ver ">=7.110.100")
  set(_avformat_ver ">=58.76.100")
  set(_avutil_ver ">=56.70.100")
  set(_postproc_ver ">=55.9.100")
  set(_swresample_ver ">=3.9.100")
  set(_swscale_ver ">=5.9.100")
endif()

# Allows building with external ffmpeg not found in system paths,
# with library version checks
if(FFMPEG_PATH)
  set(ENABLE_INTERNAL_FFMPEG OFF)
endif()

if(ENABLE_INTERNAL_FFMPEG)
  buildFFMPEG()
else()
  # external FFMPEG
  if(FFMPEG_PATH)
    list(APPEND CMAKE_PREFIX_PATH ${FFMPEG_PATH})
  endif()

  set(FFMPEG_PKGS libavcodec${_avcodec_ver}
                  libavfilter${_avfilter_ver}
                  libavformat${_avformat_ver}
                  libavutil${_avutil_ver}
                  libswscale${_swscale_ver}
                  libswresample${_swresample_ver}
                  libpostproc${_postproc_ver})

  if(PKG_CONFIG_FOUND)
    pkg_check_modules(PC_FFMPEG ${FFMPEG_PKGS} QUIET)
    string(REGEX REPLACE "framework;" "framework " PC_FFMPEG_LDFLAGS "${PC_FFMPEG_LDFLAGS}")
  endif()

  find_path(FFMPEG_INCLUDE_DIRS libavcodec/avcodec.h libavfilter/avfilter.h libavformat/avformat.h
                                libavutil/avutil.h libswscale/swscale.h libpostproc/postprocess.h
            PATH_SUFFIXES ffmpeg
            PATHS ${PC_FFMPEG_INCLUDE_DIRS}
            NO_DEFAULT_PATH)
  find_path(FFMPEG_INCLUDE_DIRS libavcodec/avcodec.h libavfilter/avfilter.h libavformat/avformat.h
                                libavutil/avutil.h libswscale/swscale.h libpostproc/postprocess.h)

  find_library(FFMPEG_LIBAVCODEC
               NAMES avcodec libavcodec
               PATH_SUFFIXES ffmpeg/libavcodec
               PATHS ${PC_FFMPEG_libavcodec_LIBDIR}
               NO_DEFAULT_PATH)
  find_library(FFMPEG_LIBAVCODEC NAMES avcodec libavcodec PATH_SUFFIXES ffmpeg/libavcodec)

  find_library(FFMPEG_LIBAVFILTER
               NAMES avfilter libavfilter
               PATH_SUFFIXES ffmpeg/libavfilter
               PATHS ${PC_FFMPEG_libavfilter_LIBDIR}
               NO_DEFAULT_PATH)
  find_library(FFMPEG_LIBAVFILTER NAMES avfilter libavfilter PATH_SUFFIXES ffmpeg/libavfilter)

  find_library(FFMPEG_LIBAVFORMAT
               NAMES avformat libavformat
               PATH_SUFFIXES ffmpeg/libavformat
               PATHS ${PC_FFMPEG_libavformat_LIBDIR}
               NO_DEFAULT_PATH)
  find_library(FFMPEG_LIBAVFORMAT NAMES avformat libavformat PATH_SUFFIXES ffmpeg/libavformat)

  find_library(FFMPEG_LIBAVUTIL
               NAMES avutil libavutil
               PATH_SUFFIXES ffmpeg/libavutil
               PATHS ${PC_FFMPEG_libavutil_LIBDIR}
               NO_DEFAULT_PATH)
  find_library(FFMPEG_LIBAVUTIL NAMES avutil libavutil PATH_SUFFIXES ffmpeg/libavutil)

  find_library(FFMPEG_LIBSWSCALE
               NAMES swscale libswscale
               PATH_SUFFIXES ffmpeg/libswscale
               PATHS ${PC_FFMPEG_libswscale_LIBDIR}
               NO_DEFAULT_PATH)
  find_library(FFMPEG_LIBSWSCALE NAMES swscale libswscale PATH_SUFFIXES ffmpeg/libswscale)

  find_library(FFMPEG_LIBSWRESAMPLE
               NAMES swresample libswresample
               PATH_SUFFIXES ffmpeg/libswresample
               PATHS ${PC_FFMPEG_libswresample_LIBDIR}
               NO_DEFAULT_PATH)
  find_library(FFMPEG_LIBSWRESAMPLE NAMES NAMES swresample libswresample PATH_SUFFIXES ffmpeg/libswresample)

  find_library(FFMPEG_LIBPOSTPROC
               NAMES postproc libpostproc
               PATH_SUFFIXES ffmpeg/libpostproc
               PATHS ${PC_FFMPEG_libpostproc_LIBDIR}
               NO_DEFAULT_PATH)
  find_library(FFMPEG_LIBPOSTPROC NAMES postproc libpostproc PATH_SUFFIXES ffmpeg/libpostproc)

  if((PC_FFMPEG_FOUND
      AND PC_FFMPEG_libavcodec_VERSION
      AND PC_FFMPEG_libavfilter_VERSION
      AND PC_FFMPEG_libavformat_VERSION
      AND PC_FFMPEG_libavutil_VERSION
      AND PC_FFMPEG_libswscale_VERSION
      AND PC_FFMPEG_libswresample_VERSION
      AND PC_FFMPEG_libpostproc_VERSION)
     OR WIN32)
    set(FFMPEG_VERSION ${REQUIRED_FFMPEG_VERSION})


    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(FFMPEG
                                      VERSION_VAR FFMPEG_VERSION
                                      REQUIRED_VARS FFMPEG_INCLUDE_DIRS
                                                    FFMPEG_LIBAVCODEC
                                                    FFMPEG_LIBAVFILTER
                                                    FFMPEG_LIBAVFORMAT
                                                    FFMPEG_LIBAVUTIL
                                                    FFMPEG_LIBSWSCALE
                                                    FFMPEG_LIBSWRESAMPLE
                                                    FFMPEG_LIBPOSTPROC
                                                    FFMPEG_VERSION
                                      FAIL_MESSAGE "FFmpeg ${REQUIRED_FFMPEG_VERSION} not found, please consider using -DENABLE_INTERNAL_FFMPEG=ON")

    if(FFMPEG_FOUND)
      set(FFMPEG_LDFLAGS ${PC_FFMPEG_LDFLAGS} CACHE STRING "ffmpeg linker flags")

      set(FFMPEG_LIBRARIES ${FFMPEG_LIBAVCODEC} ${FFMPEG_LIBAVFILTER}
                           ${FFMPEG_LIBAVFORMAT} ${FFMPEG_LIBAVUTIL}
                           ${FFMPEG_LIBSWSCALE} ${FFMPEG_LIBSWRESAMPLE}
                           ${FFMPEG_LIBPOSTPROC} ${FFMPEG_LDFLAGS})

      # check if ffmpeg libs are statically linked
      set(FFMPEG_LIB_TYPE SHARED)
      foreach(_fflib IN LISTS FFMPEG_LIBRARIES)
        if(${_fflib} MATCHES ".+\.a$" AND PC_FFMPEG_STATIC_LDFLAGS)
          set(FFMPEG_LDFLAGS ${PC_FFMPEG_STATIC_LDFLAGS} CACHE STRING "ffmpeg linker flags" FORCE)
          set(FFMPEG_LIB_TYPE STATIC)
          break()
        endif()
      endforeach()
    endif()
  else()
    if(FFMPEG_PATH)
      message(FATAL_ERROR "FFmpeg not found, please consider using -DENABLE_INTERNAL_FFMPEG=ON")
    else()
      message(STATUS "FFmpeg ${REQUIRED_FFMPEG_VERSION} not found, falling back to internal build")
      buildFFMPEG()
    endif()
  endif()
endif()

if(FFMPEG_FOUND)

  list(APPEND FFMPEG_DEFINITIONS -DFFMPEG_VER_SHA=\"${FFMPEG_VERSION}\")

  if(NOT TARGET ffmpeg)
    add_library(ffmpeg ${FFMPEG_LIB_TYPE} IMPORTED)
    set_target_properties(ffmpeg PROPERTIES
                                 FOLDER "External Projects"
                                 IMPORTED_LOCATION "${FFMPEG_LIBRARIES}"
                                 INTERFACE_INCLUDE_DIRECTORIES "${FFMPEG_INCLUDE_DIRS}"
                                 INTERFACE_LINK_LIBRARIES "${FFMPEG_LDFLAGS}"
                                 INTERFACE_COMPILE_DEFINITIONS "${FFMPEG_DEFINITIONS}")
  endif()

  set_property(GLOBAL APPEND PROPERTY INTERNAL_DEPS_PROP ffmpeg)
endif()

mark_as_advanced(FFMPEG_INCLUDE_DIRS FFMPEG_LIBRARIES FFMPEG_LDFLAGS FFMPEG_DEFINITIONS FFMPEG_FOUND)
