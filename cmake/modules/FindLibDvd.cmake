macro(SETUP_BUILD_DVD libname)
  include(cmake/scripts/common/ModuleHelpers.cmake)

  set(MODULE_LC ${ARGV0})

  # We require this due to the odd nature of github URL's compared to our other tarball
  # mirror system. If User sets MODULENAME_URL, allow get_filename_component in SETUP_BUILD_VARS
  if(${MODULE}_URL)
    set(${MODULE}_URL_PROVIDED TRUE)
  endif()

  SETUP_BUILD_VARS()

  if(NOT ${MODULE}_URL_PROVIDED)
    # override MODULENAME_URL due to tar naming when retrieved from github release for dvdlibs
    set(${MODULE}_URL ${${MODULE}_BASE_URL}/archive/${${MODULE}_VER}.tar.gz)
  endif()

  if(VERBOSE)
    message(STATUS "${MODULE}_URL: ${${MODULE}_URL}")
  endif()
endmacro()

if(ENABLE_INTERNAL_DVDLIBS)
  include(cmake/scripts/common/ModuleHelpers.cmake)

  set(HOST_ARCH ${ARCH})
  if(CORE_SYSTEM_NAME STREQUAL android)
    if(ARCH STREQUAL arm)
      set(HOST_ARCH arm-linux-androideabi)
    elseif(ARCH STREQUAL aarch64)
      set(HOST_ARCH aarch64-linux-android)
    elseif(ARCH STREQUAL i486-linux)
      set(HOST_ARCH i686-linux-android)
    elseif(ARCH STREQUAL x86_64)
      set(HOST_ARCH x86_64-linux-android)
    endif()
  elseif(CORE_SYSTEM_NAME STREQUAL windowsstore)
    set(LIBDVD_ADDITIONAL_ARGS "-DCMAKE_SYSTEM_NAME=${CMAKE_SYSTEM_NAME}" "-DCMAKE_SYSTEM_VERSION=${CMAKE_SYSTEM_VERSION}")
  elseif(APPLE)
    string(APPEND CMAKE_EXE_LINKER_FLAGS "-framework CoreFoundation")
    if(NOT CORE_SYSTEM_NAME STREQUAL darwin_embedded)
      string(APPEND CMAKE_EXE_LINKER_FLAGS " -framework IOKit")
    endif()
  endif()

  set(MAKE_COMMAND $(MAKE))
  if(CMAKE_GENERATOR STREQUAL Ninja)
    set(MAKE_COMMAND make)
    include(ProcessorCount)
    ProcessorCount(N)
    if(NOT N EQUAL 0)
      set(MAKE_COMMAND make -j${N})
    endif()
  endif()

  set(DVDREAD_CFLAGS "${DVDREAD_CFLAGS} -I${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/libdvd/include")

  if(ENABLE_DVDCSS)

    SETUP_BUILD_DVD("dvdcss")

    set(DVDCSS_PREFIX ${CORE_BUILD_DIR}/libdvd)

    if(CORE_SYSTEM_NAME MATCHES windows)
      set(CMAKE_ARGS ${LIBDVD_ADDITIONAL_ARGS}
                     -DCMAKE_INSTALL_PREFIX:PATH=${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/libdvd)
    else()
      set(DVDCSS_LIBRARY ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/libdvd/lib/libdvdcss.a)
      set(BUILD_BYPRODUCTS ${DVDCSS_LIBRARY})
      set(PATCH_COMMAND PATH=${NATIVEPREFIX}/bin:$ENV{PATH} autoreconf -vif)
      set(CONFIGURE_COMMAND ac_cv_path_GIT= <SOURCE_DIR>/configure
                            --target=${HOST_ARCH}
                            --host=${HOST_ARCH}
                            --disable-doc
                            --enable-static
                            --disable-shared
                            --with-pic
                            --prefix=<INSTALL_DIR>
                            --libdir=<INSTALL_DIR>/lib
                            "CC=${CMAKE_C_COMPILER}"
                            "CFLAGS=${CMAKE_C_FLAGS} ${DVDREAD_CFLAGS}"
                            "LDFLAGS=${CMAKE_EXE_LINKER_FLAGS}")
      set(BUILD_COMMAND ${MAKE_COMMAND})
    endif()
    BUILD_DEP_TARGET()
  endif()

  set(DVDREAD_CFLAGS "-D_XBMC -I${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/libdvd/include")
  if(ENABLE_DVDCSS)
    set(DVDREAD_CFLAGS "${DVDREAD_CFLAGS} -DHAVE_DVDCSS_DVDCSS_H")
  endif()

  SETUP_BUILD_DVD("dvdread")

  set(DVDREAD_PREFIX ${CORE_BUILD_DIR}/libdvd)

  if(CORE_SYSTEM_NAME MATCHES windows)
    set(CMAKE_ARGS ${LIBDVD_ADDITIONAL_ARGS}
                   -DCMAKE_INSTALL_PREFIX:PATH=${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/libdvd)
  else()
    set(DVDREAD_LIBRARY ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/libdvd/lib/libdvdread.a)
    set(BUILD_BYPRODUCTS ${DVDREAD_LIBRARY})
    set(PATCH_COMMAND PATH=${NATIVEPREFIX}/bin:$ENV{PATH} autoreconf -vif)
    set(CONFIGURE_COMMAND ac_cv_path_GIT= <SOURCE_DIR>/configure
                          --target=${HOST_ARCH}
                          --host=${HOST_ARCH}
                          --enable-static
                          --disable-shared
                          --with-pic
                          --prefix=<INSTALL_DIR>
                          --libdir=<INSTALL_DIR>/lib
                          "CC=${CMAKE_C_COMPILER}"
                          "CFLAGS=${CMAKE_C_FLAGS} ${DVDREAD_CFLAGS}"
                          "LDFLAGS=${CMAKE_EXE_LINKER_FLAGS}")
    set(BUILD_COMMAND ${MAKE_COMMAND})
  endif()

  BUILD_DEP_TARGET()

  if(ENABLE_DVDCSS)
    add_dependencies(dvdread dvdcss)
  endif()

  if(ENABLE_DVDCSS)
    set(DVDNAV_LIBS -ldvdcss)
  endif()

  SETUP_BUILD_DVD("dvdnav")

  set(DVDNAV_PREFIX ${CORE_BUILD_DIR}/libdvd)

  if(CORE_SYSTEM_NAME MATCHES windows)
    set(DVDNAV_LIBRARY ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/libdvd/lib/libdvdnav.lib)
    set(BUILD_BYPRODUCTS ${DVDNAV_LIBRARY})
    set(CMAKE_ARGS ${LIBDVD_ADDITIONAL_ARGS}
                   -DCMAKE_INSTALL_PREFIX:PATH=${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/libdvd)
  else()
    set(DVDNAV_LIBRARY ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/libdvd/lib/libdvdnav.a)
    set(BUILD_BYPRODUCTS ${DVDNAV_LIBRARY})
    set(PATCH_COMMAND PATH=${NATIVEPREFIX}/bin:$ENV{PATH} autoreconf -vif)
    set(CONFIGURE_COMMAND ac_cv_path_GIT= <SOURCE_DIR>/configure
                          --target=${HOST_ARCH}
                          --host=${HOST_ARCH}
                          --enable-static
                          --disable-shared
                          --with-pic
                          --prefix=${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/libdvd
                          --libdir=${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/libdvd/lib
                          "CC=${CMAKE_C_COMPILER}"
                          "LDFLAGS=${CMAKE_EXE_LINKER_FLAGS} -L${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/libdvd/lib"
                          "CFLAGS=${CMAKE_C_FLAGS} ${DVDREAD_CFLAGS}"
                          "DVDREAD_CFLAGS=${DVDREAD_CFLAGS}"
                          "DVDREAD_LIBS=${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/libdvd/lib/libdvdread.la"
                          "LIBS=${DVDNAV_LIBS}")
    set(BUILD_COMMAND ${MAKE_COMMAND})
  endif()

  BUILD_DEP_TARGET()

  add_dependencies(dvdnav dvdread)

  set(_dvdlibs ${DVDREAD_LIBRARY} ${DVDCSS_LIBRARY})

  if(CORE_SYSTEM_NAME MATCHES windows)
    set(LIBDVD_TARGET_DIR .)
    if(CORE_SYSTEM_NAME STREQUAL windowsstore)
      set(LIBDVD_TARGET_DIR dlls)
    endif()
    copy_file_to_buildtree(${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/libdvd/bin/libdvdnav.dll DIRECTORY ${LIBDVD_TARGET_DIR})
    add_dependencies(export-files dvdnav)
  else()
    # link a shared dvdnav library that includes the whole archives of dvdread and dvdcss as well
    # the quotes around _dvdlibs are on purpose, since we want to pass a list to the function that will be unpacked automatically
    core_link_library(${DVDNAV_LIBRARY} system/players/VideoPlayer/libdvdnav dvdnav archives "${_dvdlibs}")
  endif()

  set(LIBDVD_INCLUDE_DIRS ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/libdvd/include)
  set(LIBDVD_LIBRARIES ${DVDNAV_LIBRARY} ${DVDREAD_LIBRARY})
  if(ENABLE_DVDCSS)
    list(APPEND LIBDVD_LIBRARIES ${DVDCSS_LIBRARY})
  endif()
  set(LIBDVD_LIBRARIES ${LIBDVD_LIBRARIES} CACHE STRING "libdvd libraries" FORCE)
  set(LIBDVD_FOUND 1 CACHE BOOL "libdvd found" FORCE)
else()
  set(_dvdlibs dvdread dvdnav)
  if(ENABLE_DVDCSS)
    list(APPEND _dvdlibs libdvdcss)
  endif()

  if(PKG_CONFIG_FOUND)
    pkg_check_modules(PC_DVD ${_dvdlibs} QUIET)
  endif()

  find_path(LIBDVD_INCLUDE_DIRS dvdnav/dvdnav.h PATHS ${PC_DVD_INCLUDE_DIRS})
  find_library(DVDREAD_LIBRARY NAMES dvdread libdvdread PATHS ${PC_DVD_dvdread_LIBDIR})
  find_library(DVDNAV_LIBRARY NAMES dvdnav libdvdnav PATHS ${PC_DVD_dvdnav_LIBDIR})
  if(ENABLE_DVDCSS)
    find_library(DVDCSS_LIBRARY NAMES dvdcss libdvdcss PATHS ${PC_DVD_libdvdcss_LIBDIR})
  endif()
endif()

# Windows uses a single bundled lib DVDNAV_LIBRARY
set(_handlevars LIBDVD_INCLUDE_DIRS DVDNAV_LIBRARY)
if(NOT CORE_SYSTEM_NAME MATCHES windows)
  list(APPEND _handlevars DVDREAD_LIBRARY)
  if(ENABLE_DVDCSS)
    list(APPEND _handlevars DVDCSS_LIBRARY)
  endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LibDvd REQUIRED_VARS ${_handlevars})

if(LIBDVD_FOUND)

  if(NOT TARGET dvdnav)
    add_library(dvdnav UNKNOWN IMPORTED)
    set_target_properties(dvdnav PROPERTIES
                                  FOLDER "External Projects"
                                  IMPORTED_LOCATION "${DVDNAV_LIBRARY}")
  endif()
  if(NOT TARGET dvdread)
    add_library(dvdread UNKNOWN IMPORTED)
    set_target_properties(dvdread PROPERTIES
                                  FOLDER "External Projects"
                                  IMPORTED_LOCATION "${DVDREAD_LIBRARY}")
  endif()
  if(NOT TARGET dvdcss)
    add_library(dvdcss UNKNOWN IMPORTED)
    set_target_properties(dvdcss PROPERTIES
                                  FOLDER "External Projects"
                                  IMPORTED_LOCATION "${DVDCSS_LIBRARY}")
  endif()

  if(NOT ENABLE_INTERNAL_DVDLIBS)
    set(_linklibs ${DVDREAD_LIBRARY})
    if(ENABLE_DVDCSS)
      list(APPEND _linklibs ${DVDCSS_LIBRARY})
    endif()
    core_link_library(${DVDNAV_LIBRARY} system/players/VideoPlayer/libdvdnav dvdnav archives "${_linklibs}")
    set(LIBDVD_LIBRARIES ${DVDNAV_LIBRARY})
  endif()

endif()

set_property(GLOBAL APPEND PROPERTY INTERNAL_DEPS_PROP dvdnav)
mark_as_advanced(LIBDVD_INCLUDE_DIRS LIBDVD_LIBRARIES)
