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
# ${APP_NAME_LC}::FFMPEG  - The FFmpeg interface target
# --------
#

# Macro to build internal FFmpeg
# Refactoring to a macro allows simple fallthrough callback if system ffmpeg failure
macro(buildFFMPEG)
  include(cmake/scripts/common/ModuleHelpers.cmake)

  # Check for dependencies - Must be done before SETUP_BUILD_VARS
  get_libversion_data("dav1d" "target")
  find_package(Dav1d ${LIB_DAV1D_VER} MODULE ${SEARCH_QUIET})
  if(NOT TARGET ${APP_NAME_LC}::Dav1d)
    message(STATUS "dav1d not found, internal ffmpeg build will be missing AV1 support!")
  else()
    set(FFMPEG_OPTIONS -DENABLE_DAV1D=ON)
  endif()

  set(${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC ffmpeg)

  SETUP_BUILD_VARS()

  if(WIN32 OR WINDOWS_STORE)

    find_package(Msys REQUIRED ${SEARCH_QUIET})
    find_program(msys_BASH NAMES sh bash PATHS ${MSYS_INSTALL_PATH}/usr/bin REQUIRED
                           ${${CORE_SYSTEM_NAME}_SEARCH_CONFIG})

    set(msys_env MSYS2_PATH_TYPE=inherit
                 MSYS_INSTALL_PATH=${MSYS_INSTALL_PATH})

    # Todo: buildmode?
    set(PROMPTLEVEL noprompt)
    set(BUILDMODE clean)

    set(build32 no)
    set(build64 no)
    set(buildArm no)
    set(buildArm64 no)
    set(win10 no)

    if(ARCH STREQUAL arm64)
      set(buildArm64 yes)
    elseif(ARCH STREQUAL arm)
      set(buildArm yes)
    elseif(ARCH STREQUAL win32)
      set(build32 yes)
    elseif(ARCH STREQUAL x64)
      set(build64 yes)
    endif()

    if(CMAKE_SYSTEM_NAME STREQUAL WindowsStore)
      set(win10 yes)
    endif()

    # The msys script will install and do all patching, so point to that source path
    set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_SOURCE_DIR ${CMAKE_SOURCE_DIR}/project/BuildDependencies/build/src/${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}-${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER})
    # We must create the directory, otherwise we get non-existant path errors
    file(MAKE_DIRECTORY ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_SOURCE_DIR})

    # We need the directory to be non-empty for externalproject_add, however we need to clean up the
    # created EMPTYFILE for the msys script existence checks to download the archive if needed.
    file(TOUCH ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_SOURCE_DIR}/EMPTYFILE)
    set(CONFIGURE_COMMAND ${CMAKE_COMMAND} -E rm -rf ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_SOURCE_DIR}
                  COMMAND ${CMAKE_COMMAND} -E make_directory ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_SOURCE_DIR})

    set(BUILD_COMMAND ${CMAKE_COMMAND} -E env ${msys_env} ${msys_BASH}
                                        --login -i ${CMAKE_SOURCE_DIR}/tools/buildsteps/windows/make-mingwlibs.sh
                                        --prompt=${PROMPTLEVEL}
                                        --mode=${BUILDMODE}
                                        --build32=${build32}
                                        --build64=${build64}
                                        --buildArm=${buildArm}
                                        --buildArm64=${buildArm64}
                                        --win10=${win10})
    set(INSTALL_COMMAND ${CMAKE_COMMAND} -E true)

    BUILD_DEP_TARGET()

    set(FFMPEG_INCLUDE_DIRS ${MINGW_LIBS_DIR}/include)
    # We must create the directory, otherwise we get non-existant path errors
    file(MAKE_DIRECTORY ${FFMPEG_INCLUDE_DIRS})
  else()

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
    string(REPLACE ";" "|" ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_MODULE_PATH "${CMAKE_MODULE_PATH}")
    set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LIST_SEPARATOR LIST_SEPARATOR |)

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
                   -DDISABLE_FFMPEG_SOURCE_PLUGINS=${DISABLE_FFMPEG_SOURCE_PLUGINS}
                   ${CROSS_ARGS}
                   ${FFMPEG_OPTIONS}
                   -DPKG_CONFIG_PATH=${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/lib/pkgconfig)
    set(PATCH_COMMAND ${CMAKE_COMMAND} -E copy
                      ${CMAKE_SOURCE_DIR}/tools/depends/target/ffmpeg/CMakeLists.txt
                      <SOURCE_DIR>
    )

    if(NOT DISABLE_FFMPEG_SOURCE_PLUGINS)
      list(APPEND PATCH_COMMAND COMMAND ${CMAKE_COMMAND} -E copy
                                ${CMAKE_SOURCE_DIR}/tools/depends/target/ffmpeg/001-ffmpeg-all-libpostproc-plugin.patch
                                <SOURCE_DIR>)

      set(postproc_pkg_config_search "postproc=`PKG_CONFIG_PATH=${DEPENDS_PATH}/lib/pkgconfig ${PKG_CONFIG_EXECUTABLE} --libs --static libpostproc`")
    endif()

    if(CMAKE_GENERATOR STREQUAL Xcode)
      set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_GENERATOR CMAKE_GENERATOR "Unix Makefiles")
    endif()

    BUILD_DEP_TARGET()

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
    ${postproc_pkg_config_search}
    swscale=`PKG_CONFIG_PATH=${DEPENDS_PATH}/lib/pkgconfig ${PKG_CONFIG_EXECUTABLE} --libs --static libswscale`
    swresample=`PKG_CONFIG_PATH=${DEPENDS_PATH}/lib/pkgconfig ${PKG_CONFIG_EXECUTABLE} --libs --static libswresample`
    gnutls=`PKG_CONFIG_PATH=${DEPENDS_PATH}/lib/pkgconfig/ ${PKG_CONFIG_EXECUTABLE}  --libs-only-l --static --silence-errors gnutls`
    $@ $avcodec $avformat $avfilter $avutil $swscale $swresample $postproc $gnutls
  else
    $@
  fi")
    file(COPY ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/ffmpeg/ffmpeg-link-wrapper
         DESTINATION ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}
         FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE)

    set(FFMPEG_LINK_EXECUTABLE "${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/ffmpeg-link-wrapper <CMAKE_CXX_COMPILER> <FLAGS> <CMAKE_CXX_LINK_FLAGS> <LINK_FLAGS> <OBJECTS> -o <TARGET> <LINK_LIBRARIES>" PARENT_SCOPE)
    set(FFMPEG_INCLUDE_DIRS ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/include)
  endif()

  if(TARGET ${APP_NAME_LC}::Dav1d)
    add_dependencies(${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME} ${APP_NAME_LC}::Dav1d)
  endif()

  set(FFMPEG_FOUND 1)
  set(FFMPEG_VERSION ${FFMPEG_VER})

  if(WIN32 OR WINDOWS_STORE)
    set(target_scope UNKNOWN)
  else()
    # Whilst we use ffmpeg-link-wrapper, we only need INTERFACE at most, and possibly
    # just not at all. However this gives target consistency with external FFMPEG usage
    # The benefit and reason to continue to use the wrapper is to automate the collection
    # of the actual linker flags from pkg-config lookup
    set(target_scope INTERFACE)
  endif()

  foreach(_ffmpeg_pkg IN ITEMS ${FFMPEG_PKGS})
    string(REGEX REPLACE ">=.*" "" _libname ${_ffmpeg_pkg})

    add_library(ffmpeg::${_libname} ${target_scope} IMPORTED)
    set_target_properties(ffmpeg::${_libname} PROPERTIES
                                              INTERFACE_INCLUDE_DIRECTORIES "${FFMPEG_INCLUDE_DIRS}")

    if(WIN32 OR WINDOWS_STORE)
      string(REPLACE "lib" "" name ${_libname})
      set_target_properties(ffmpeg::${_libname} PROPERTIES
                                                IMPORTED_LOCATION "${MINGW_LIBS_DIR}/lib/${name}.lib")
    endif()

  endforeach()
endmacro()


# Allows building with external ffmpeg not found in system paths,
# without library version checks
if(WITH_FFMPEG)
  set(FFMPEG_PATH ${WITH_FFMPEG})
  message(STATUS "Warning: FFmpeg version checking disabled")
  set(REQUIRED_FFMPEG_VERSION undef)
else()
  # required ffmpeg library versions
  set(REQUIRED_FFMPEG_VERSION 7.0.0)
  set(_avcodec_ver ">=61.3.100")
  set(_avfilter_ver ">=10.1.100")
  set(_avformat_ver ">=61.1.100")
  set(_avutil_ver ">=59.8.100")
  set(_postproc_ver ">=58.1.100")
  set(_swresample_ver ">=5.1.100")
  set(_swscale_ver ">=8.1.100")
endif()

# Allows building with external ffmpeg not found in system paths,
# with library version checks
if(FFMPEG_PATH)
  set(ENABLE_INTERNAL_FFMPEG OFF)
endif()

set(FFMPEG_PKGS libavcodec${_avcodec_ver}
                libavfilter${_avfilter_ver}
                libavformat${_avformat_ver}
                libavutil${_avutil_ver}
                libswscale${_swscale_ver}
                libswresample${_swresample_ver})

if(NOT DISABLE_FFMPEG_SOURCE_PLUGINS)
  # Optional ffmpeg sourceplugins
  list(APPEND FFMPEG_PKGS libpostproc${_postproc_ver})
endif()

if(ENABLE_INTERNAL_FFMPEG)
  buildFFMPEG()
else()
  # external FFMPEG
  if(FFMPEG_PATH)
    list(APPEND CMAKE_PREFIX_PATH ${FFMPEG_PATH})
  endif()

  # macro for find_library usage
  # arg1: lowercase libname (eg libavcodec, libpostproc, etc)
  macro(ffmpeg_find_lib libname)
    string(TOUPPER ${libname} libname_UPPER)
    string(REPLACE "lib" "" name ${libname})

    find_library(FFMPEG_${libname_UPPER}
                 NAMES ${name} ${libname}
                 PATH_SUFFIXES ffmpeg/${libname}
                 HINTS ${DEPENDS_PATH}/lib ${MINGW_LIBS_DIR}/lib
                 ${${CORE_SYSTEM_NAME}_SEARCH_CONFIG})
  endmacro()

  foreach(_ffmpeg_pkg IN ITEMS ${FFMPEG_PKGS})
    string(REGEX REPLACE ">=.*" "" _libname ${_ffmpeg_pkg})
    ffmpeg_find_lib(${_libname})
  endforeach()

  # Check all libs are found, and set found.
  # find_package_handle_standard_args isnt usable, as the libs may not exist, and it
  # errors on not found.
  if(FFMPEG_LIBAVCODEC AND
     FFMPEG_LIBAVFILTER AND
     FFMPEG_LIBAVFORMAT AND
     FFMPEG_LIBAVUTIL AND
     FFMPEG_LIBSWSCALE AND
     FFMPEG_LIBSWRESAMPLE)
    set(FFMPEG_FOUND 1)

    # list of sourceplugin headers for find_path
    if(FFMPEG_LIBPOSTPROC)
      set(source_plugin_headers libpostproc/postprocess.h)
    endif()
  endif()

  if(FFMPEG_FOUND)
    # There is no easily identifiable version number for ffmpeg, as all libs have their
    # own versioning. We may not bump REQUIRED_FFMPEG_VERSION if there is not a hard
    # lib version increase due to API/ABI changes in FFMPEG. Due to this, the FFMPEG_VERSION
    # may actually indicate a different version than actually used.
    set(FFMPEG_VERSION ${REQUIRED_FFMPEG_VERSION})

    find_path(FFMPEG_INCLUDE_DIRS libavcodec/avcodec.h libavfilter/avfilter.h libavformat/avformat.h
                                  libavutil/avutil.h libswscale/swscale.h ${source_plugin_headers}
              PATH_SUFFIXES ffmpeg
              HINTS ${DEPENDS_PATH}/include ${MINGW_LIBS_DIR}/include
              ${${CORE_SYSTEM_NAME}_SEARCH_CONFIG})

    # Macro to populate target
    # arg1: lowercase libname (eg libavcodec, libpostproc, etc)
    macro(ffmpeg_create_target libname)
      string(TOUPPER ${libname} libname_UPPER)
      string(REPLACE "lib" "" name ${libname})

      if(FFMPEG_${libname_UPPER})
        if(WIN32 OR WINDOWS_STORE)
          add_library(ffmpeg::${libname} UNKNOWN IMPORTED)
          set_target_properties(ffmpeg::${libname} PROPERTIES
                                                   IMPORTED_LOCATION "${FFMPEG_${libname_UPPER}}"
                                                   INTERFACE_INCLUDE_DIRECTORIES "${FFMPEG_INCLUDE_DIRS}")
        else()
          # We have to run the check against the single lib a second time, as when
          # pkg_check_modules is run with a list, the only *_LDFLAGS set is a concatenated 
          # list of all checked modules. Ideally we want each target to only have the LDFLAGS
          # required for that specific module
          pkg_check_modules(FFMPEG_${libname} ${libname}${_${name}_ver} ${SEARCH_QUIET})

          # pkg-config LDFLAGS always seem to have -l<name> listed. We dont need that, as
          # the target gets a direct path to the physical lib
          list(REMOVE_ITEM FFMPEG_${libname}_LDFLAGS "-l${name}")

          # Darwin platforms return a list that cmake splits "framework libname" into separate
          # items, therefore the link arguments become -framework -libname causing link failures
          # we just force concatenation of these instances, so cmake passes it as "-framework libname"
          if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
            string(REGEX REPLACE "framework;" "framework " FFMPEG_${libname}_LDFLAGS "${FFMPEG_${libname}_LDFLAGS}")
          endif()

          foreach(ldflag IN LISTS FFMPEG_${libname}_LDFLAGS)
            foreach(pkgname IN ITEMS ${FFMPEG_PKGS})
              string(REGEX REPLACE ">=.*" "" _shortlibname ${pkgname})
              string(TOUPPER ${_shortlibname} _shortlibname_UPPER)
              string(REPLACE "lib" "" shortname ${_shortlibname})
  
              # replace -l<ffmpeglib> flag with ffmpeg target
              # This provides correct link ordering and deduplication
              string(REGEX REPLACE "-l${shortname}" "ffmpeg::${_shortlibname}" ldflag ${ldflag})
            endforeach()
            list(APPEND ${libname}_LDFLAGS ${ldflag})
          endforeach()

          add_library(ffmpeg::${libname} STATIC IMPORTED)
          set_target_properties(ffmpeg::${libname} PROPERTIES
                                                   IMPORTED_LOCATION "${FFMPEG_${libname_UPPER}}"
                                                   INTERFACE_LINK_LIBRARIES "${${libname}_LDFLAGS}"
                                                   INTERFACE_INCLUDE_DIRECTORIES "${FFMPEG_INCLUDE_DIRS}")
        endif()
      endif()
    endmacro()

    foreach(_ffmpeg_pkg IN ITEMS ${FFMPEG_PKGS})
      string(REGEX REPLACE ">=.*" "" _libname ${_ffmpeg_pkg})
      ffmpeg_create_target(${_libname})
    endforeach()
  else()
    if(KODI_DEPENDSBUILD OR (WIN32 OR WINDOWS_STORE))
      message(WARNING "Suitable FFmpeg version not found, consider explicitly using -DENABLE_INTERNAL_FFMPEG=ON. Internal FFMPEG will be built")
      buildFFMPEG()
    else()
      message(FATAL_ERROR "Suitable FFmpeg version not found, consider using -DENABLE_INTERNAL_FFMPEG=ON")
    endif()
  endif()
endif()

if(FFMPEG_FOUND)
  set(_ffmpeg_definitions FFMPEG_VER_SHA=${FFMPEG_VERSION})

  if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})
    add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} INTERFACE IMPORTED)
    set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                     INTERFACE_INCLUDE_DIRECTORIES "${FFMPEG_INCLUDE_DIRS}"
                                                                     INTERFACE_COMPILE_DEFINITIONS "${_ffmpeg_definitions}")
  endif()

  if(TARGET ffmpeg::libpostproc)
    set_property(TARGET ffmpeg::libpostproc APPEND PROPERTY
                                                   INTERFACE_COMPILE_DEFINITIONS "HAVE_LIBPOSTPROC")
  endif()

  foreach(_ffmpeg_pkg IN ITEMS ${FFMPEG_PKGS})
    string(REGEX REPLACE ">=.*" "" _libname ${_ffmpeg_pkg})
    if(TARGET ffmpeg::${_libname})
      target_link_libraries(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} INTERFACE ffmpeg::${_libname})
    endif()
  endforeach()

  if(TARGET ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})
    add_dependencies(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})
  endif()
endif()
