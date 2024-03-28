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
# This will define the following target:
#
# ffmpeg::ffmpeg  - The FFmpeg interface target
# --------
#

# Macro to build internal FFmpeg
# Refactoring to a macro allows simple fallthrough callback if system ffmpeg failure
macro(buildFFMPEG)
  include(cmake/scripts/common/ModuleHelpers.cmake)

  # Check for dependencies - Must be done before SETUP_BUILD_VARS
  get_libversion_data("dav1d" "target")
  find_package(Dav1d ${LIB_DAV1D_VER} MODULE)
  if(NOT TARGET dav1d::dav1d)
    message(STATUS "dav1d not found, internal ffmpeg build will be missing AV1 support!")
  else()
    set(FFMPEG_OPTIONS -DENABLE_DAV1D=ON)
  endif()

  set(MODULE_LC ffmpeg)

  SETUP_BUILD_VARS()

  list(APPEND FFMPEG_OPTIONS -DENABLE_CCACHE=${ENABLE_CCACHE}
                             -DCCACHE_PROGRAM=${CCACHE_PROGRAM}
                             -DENABLE_VAAPI=${ENABLE_VAAPI}
                             -DENABLE_VDPAU=${ENABLE_VDPAU}
                             -DEXTRA_FLAGS=${FFMPEG_EXTRA_FLAGS})

  if(KODI_DEPENDSBUILD)
    set(CROSS_ARGS -DDEPENDS_PATH=${DEPENDS_PATH}
                   -DPKG_CONFIG_EXECUTABLE=${PKG_CONFIG_EXECUTABLE}
                   -DCROSSCOMPILING=${CMAKE_CROSSCOMPILING}
                   -DOS=${OS}
                   -DCMAKE_AR=${CMAKE_AR})
  endif()

  if(USE_LTO)
    list(APPEND FFMPEG_OPTIONS -DUSE_LTO=ON)
  endif()

  set(LINKER_FLAGS ${CMAKE_EXE_LINKER_FLAGS})
  list(APPEND LINKER_FLAGS ${SYSTEM_LDFLAGS})

  # Some list shenanigans not being passed through without stringify/listify
  # externalproject_add allows declaring list separator to generate a list for the target
  string(REPLACE ";" "|" FFMPEG_MODULE_PATH "${CMAKE_MODULE_PATH}")
  set(FFMPEG_LIST_SEPARATOR LIST_SEPARATOR |)

  set(CMAKE_ARGS -DCMAKE_MODULE_PATH=${FFMPEG_MODULE_PATH}
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

  if(CMAKE_GENERATOR STREQUAL Xcode)
    set(FFMPEG_GENERATOR CMAKE_GENERATOR "Unix Makefiles")
  endif()

  BUILD_DEP_TARGET()

  if(TARGET dav1d::dav1d)
    add_dependencies(ffmpeg dav1d::dav1d)
  endif()

  find_program(BASH_COMMAND bash)
  if(NOT BASH_COMMAND)
    message(FATAL_ERROR "Internal FFmpeg requires bash.")
  endif()
  file(WRITE ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/ffmpeg/ffmpeg-link-wrapper
"#!${BASH_COMMAND}
if [[ $@ == *${APP_NAME_LC}.bin* || $@ == *${APP_NAME_LC}${APP_BINARY_SUFFIX}* || $@ == *${APP_NAME_LC}.so* || $@ == *${APP_NAME_LC}-test* || $@ == *MacOS/Kodi* ]]
then
  avcodec=`PKG_CONFIG_PATH=${DEPENDS_PATH}/lib/pkgconfig ${PKG_CONFIG_EXECUTABLE} --libs --static libavcodec`
  avformat=`PKG_CONFIG_PATH=${DEPENDS_PATH}/lib/pkgconfig ${PKG_CONFIG_EXECUTABLE} --libs --static libavformat`
  avfilter=`PKG_CONFIG_PATH=${DEPENDS_PATH}/lib/pkgconfig ${PKG_CONFIG_EXECUTABLE} --libs --static libavfilter`
  avutil=`PKG_CONFIG_PATH=${DEPENDS_PATH}/lib/pkgconfig ${PKG_CONFIG_EXECUTABLE} --libs --static libavutil`
  swscale=`PKG_CONFIG_PATH=${DEPENDS_PATH}/lib/pkgconfig ${PKG_CONFIG_EXECUTABLE} --libs --static libswscale`
  swresample=`PKG_CONFIG_PATH=${DEPENDS_PATH}/lib/pkgconfig ${PKG_CONFIG_EXECUTABLE} --libs --static libswresample`
  gnutls=`PKG_CONFIG_PATH=${DEPENDS_PATH}/lib/pkgconfig/ ${PKG_CONFIG_EXECUTABLE}  --libs-only-l --static --silence-errors gnutls`
  $@ $avcodec $avformat $avcodec $avfilter $swscale $swresample -lpostproc $gnutls
else
  $@
fi")
  file(COPY ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/ffmpeg/ffmpeg-link-wrapper
       DESTINATION ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}
       FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE)

  set(FFMPEG_LINK_EXECUTABLE "${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/ffmpeg-link-wrapper <CMAKE_CXX_COMPILER> <FLAGS> <CMAKE_CXX_LINK_FLAGS> <LINK_FLAGS> <OBJECTS> -o <TARGET> <LINK_LIBRARIES>" PARENT_SCOPE)
  set(FFMPEG_INCLUDE_DIRS ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/include)
  set(FFMPEG_FOUND 1)
  set(FFMPEG_VERSION ${FFMPEG_VER})

  # Whilst we use ffmpeg-link-wrapper, we only need INTERFACE at most, and possibly
  # just not at all. However this gives target consistency with external FFMPEG usage
  # The benefit and reason to continue to use the wrapper is to automate the collection
  # of the actual linker flags from pkg-config lookup

  add_library(ffmpeg::libavcodec INTERFACE IMPORTED)
  set_target_properties(ffmpeg::libavcodec PROPERTIES
                                           INTERFACE_INCLUDE_DIRECTORIES "${FFMPEG_INCLUDE_DIR}")

  add_library(ffmpeg::libavfilter INTERFACE IMPORTED)
  set_target_properties(ffmpeg::libavfilter PROPERTIES
                                            INTERFACE_INCLUDE_DIRECTORIES "${FFMPEG_INCLUDE_DIR}")

  add_library(ffmpeg::libavformat INTERFACE IMPORTED)
  set_target_properties(ffmpeg::libavformat PROPERTIES
                                            INTERFACE_INCLUDE_DIRECTORIES "${FFMPEG_INCLUDE_DIR}")

  add_library(ffmpeg::libavutil INTERFACE IMPORTED)
  set_target_properties(ffmpeg::libavutil PROPERTIES
                                          INTERFACE_INCLUDE_DIRECTORIES "${FFMPEG_INCLUDE_DIR}")

  add_library(ffmpeg::libswscale INTERFACE IMPORTED)
  set_target_properties(ffmpeg::libswscale PROPERTIES
                                           INTERFACE_INCLUDE_DIRECTORIES "${FFMPEG_INCLUDE_DIR}")

  add_library(ffmpeg::libswresample INTERFACE IMPORTED)
  set_target_properties(ffmpeg::libswresample PROPERTIES
                                              INTERFACE_INCLUDE_DIRECTORIES "${FFMPEG_INCLUDE_DIR}")

  add_library(ffmpeg::libpostproc INTERFACE IMPORTED)
  set_target_properties(ffmpeg::libpostproc PROPERTIES
                                            INTERFACE_INCLUDE_DIRECTORIES "${FFMPEG_INCLUDE_DIR}")
endmacro()


# Allows building with external ffmpeg not found in system paths,
# without library version checks
if(WITH_FFMPEG)
  set(FFMPEG_PATH ${WITH_FFMPEG})
  message(STATUS "Warning: FFmpeg version checking disabled")
  set(REQUIRED_FFMPEG_VERSION undef)
else()
  # required ffmpeg library versions
  set(REQUIRED_FFMPEG_VERSION 6.0.0)
  set(_avcodec_ver ">=60.2.100")
  set(_avfilter_ver ">=9.3.100")
  set(_avformat_ver ">=60.3.100")
  set(_avutil_ver ">=58.2.100")
  set(_postproc_ver ">=57.1.100")
  set(_swresample_ver ">=4.10.100")
  set(_swscale_ver ">=7.1.100")
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

  if(NOT WIN32)
    find_package(PkgConfig REQUIRED)

    pkg_check_modules(PC_FFMPEG ${FFMPEG_PKGS})
  endif()

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

    # macro for find_library usage
    # arg1: lowercase libname (eg libavcodec, libpostproc, etc)
    macro(ffmpeg_find_lib libname)
      string(TOUPPER ${libname} libname_UPPER)
      string(REPLACE "lib" "" name ${libname})

      find_library(FFMPEG_${libname_UPPER}
                   NAMES ${name} ${libname}
                   PATH_SUFFIXES ffmpeg/${libname}
                   HINTS ${DEPENDS_PATH}/lib ${PC_FFMPEG_${libname}_LIBDIR}
                   ${${CORE_PLATFORM_LC}_SEARCH_CONFIG})
    endmacro()

    find_path(FFMPEG_INCLUDE_DIRS libavcodec/avcodec.h libavfilter/avfilter.h libavformat/avformat.h
                                  libavutil/avutil.h libswscale/swscale.h libpostproc/postprocess.h
              PATH_SUFFIXES ffmpeg
              HINTS ${DEPENDS_PATH}/include
              ${${CORE_PLATFORM_LC}_SEARCH_CONFIG})

    foreach(_ffmpeg_pkg IN ITEMS ${FFMPEG_PKGS})
      string(REGEX REPLACE ">=.*" "" _libname ${_ffmpeg_pkg})
      ffmpeg_find_lib(${_libname})
    endforeach()

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

    # Macro to populate target
    # arg1: lowercase libname (eg libavcodec, libpostproc, etc)
    macro(ffmpeg_create_target libname)
      string(TOUPPER ${libname} libname_UPPER)
      string(REPLACE "lib" "" name ${libname})

      if(PKG_CONFIG_FOUND AND NOT WIN32)
        # We have to run the check against the single lib a second time, as when
        # pkg_check_modules is run with a list, the only *_LDFLAGS set is a concatenated 
        # list of all checked modules. Ideally we want each target to only have the LDFLAGS
        # required for that specific module
        pkg_check_modules(PC_FFMPEG_${libname} ${libname}${_${name}_ver} QUIET)

        # pkg-config LDFLAGS always seem to have -l<name> listed. We dont need that, as
        # the target gets a direct path to the physical lib
        list(REMOVE_ITEM PC_FFMPEG_${libname}_LDFLAGS "-l${name}")

        # Darwin platforms return a list that cmake splits "framework libname" into separate
        # items, therefore the link arguments become -framework -libname causing link failures
        # we just force concatenation of these instances, so cmake passes it as "-framework libname"
        if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
          string(REGEX REPLACE "framework;" "framework " PC_FFMPEG_${libname}_LDFLAGS "${PC_FFMPEG_${libname}_LDFLAGS}")
        endif()
      endif()

      add_library(ffmpeg::${libname} UNKNOWN IMPORTED)
      set_target_properties(ffmpeg::${libname} PROPERTIES
                                           FOLDER "FFMPEG - External Projects"
                                           IMPORTED_LOCATION "${FFMPEG_${libname_UPPER}}"
                                           INTERFACE_LINK_LIBRARIES "${PC_FFMPEG_${libname}_LDFLAGS}"
                                           INTERFACE_INCLUDE_DIRECTORIES "${FFMPEG_INCLUDE_DIRS}")
    endmacro()

    foreach(_ffmpeg_pkg IN ITEMS ${FFMPEG_PKGS})
      string(REGEX REPLACE ">=.*" "" _libname ${_ffmpeg_pkg})
      ffmpeg_create_target(${_libname})
    endforeach()

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
  set(_ffmpeg_definitions FFMPEG_VER_SHA=${FFMPEG_VERSION})

  if(NOT TARGET ffmpeg::ffmpeg)
    add_library(ffmpeg::ffmpeg INTERFACE IMPORTED)
    set_target_properties(ffmpeg::ffmpeg PROPERTIES
                                         INTERFACE_INCLUDE_DIRECTORIES "${FFMPEG_INCLUDE_DIRS}"
                                         INTERFACE_COMPILE_DEFINITIONS "${_ffmpeg_definitions}")
  endif()

  target_link_libraries(ffmpeg::ffmpeg INTERFACE ffmpeg::libavcodec)
  target_link_libraries(ffmpeg::ffmpeg INTERFACE ffmpeg::libavfilter)
  target_link_libraries(ffmpeg::ffmpeg INTERFACE ffmpeg::libavformat)
  target_link_libraries(ffmpeg::ffmpeg INTERFACE ffmpeg::libavutil)
  target_link_libraries(ffmpeg::ffmpeg INTERFACE ffmpeg::libswscale)
  target_link_libraries(ffmpeg::ffmpeg INTERFACE ffmpeg::libswresample)
  target_link_libraries(ffmpeg::ffmpeg INTERFACE ffmpeg::libpostproc)

  if(TARGET ffmpeg)
    add_dependencies(ffmpeg::ffmpeg ffmpeg)
  endif()

  set_property(GLOBAL APPEND PROPERTY INTERNAL_DEPS_PROP ffmpeg::ffmpeg)
endif()
