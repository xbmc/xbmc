# Generic externaproject_add call for windows platforms
function(windows_externalprojectadd module_name)
  string(TOUPPER ${module_name} DVDLIB)
  string(PREPEND DVDLIB "LIB")
  ExternalProject_Add(${module_name}
    URL ${${DVDLIB}_URL}
    URL_HASH ${${DVDLIB}_HASH}
    DOWNLOAD_DIR ${TARBALL_DIR}
    DOWNLOAD_NAME ${${DVDLIB}_ARCHIVE}
    CMAKE_ARGS
      ${LIBDVD_ADDITIONAL_ARGS}
      -DCMAKE_PREFIX_PATH:PATH=${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/libdvd
      -DCMAKE_INSTALL_PREFIX:PATH=${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/libdvd
  )
endfunction()

# Generic autoreconf step
function(addstep_autoreconf module_name)
  ExternalProject_Add_Step(${module_name} autoreconf
                                  DEPENDEES download update patch
                                  DEPENDERS configure
                                  COMMAND PATH=${NATIVEPREFIX}/bin:$ENV{PATH} autoreconf -vif
                                  WORKING_DIRECTORY <SOURCE_DIR>)
endfunction()

if(KODI_DEPENDSBUILD)
  set(_dvdlibs dvdread dvdnav)
  set(_handlevars LIBDVD_INCLUDE_DIRS DVDREAD_LIBRARY DVDNAV_LIBRARY)
  if(ENABLE_DVDCSS)
    list(APPEND _dvdlibs libdvdcss)
    list(APPEND _handlevars DVDCSS_LIBRARY)
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

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(LibDvd REQUIRED_VARS ${_handlevars})
  if(LIBDVD_FOUND)
    add_library(dvdnav UNKNOWN IMPORTED)
    set_target_properties(dvdnav PROPERTIES
                                  FOLDER "External Projects"
                                  IMPORTED_LOCATION "${DVDNAV_LIBRARY}")

    add_library(dvdread UNKNOWN IMPORTED)
    set_target_properties(dvdread PROPERTIES
                                  FOLDER "External Projects"
                                  IMPORTED_LOCATION "${DVDREAD_LIBRARY}")
    add_library(dvdcss UNKNOWN IMPORTED)
    set_target_properties(dvdcss PROPERTIES
                                  FOLDER "External Projects"
                                  IMPORTED_LOCATION "${DVDCSS_LIBRARY}")

    set(_linklibs ${DVDREAD_LIBRARY})
    if(ENABLE_DVDCSS)
      list(APPEND _linklibs ${DVDCSS_LIBRARY})
    endif()
    core_link_library(${DVDNAV_LIBRARY} system/players/VideoPlayer/libdvdnav dvdnav archives "${_linklibs}")
    set(LIBDVD_LIBRARIES ${DVDNAV_LIBRARY})
    mark_as_advanced(LIBDVD_INCLUDE_DIRS LIBDVD_LIBRARIES)
  endif()
else()
  include(cmake/scripts/common/ModuleHelpers.cmake)

  set(dvdlibs libdvdread libdvdnav)
  if(ENABLE_DVDCSS)
    list(APPEND dvdlibs libdvdcss)
  endif()
  set(DEPENDS_TARGETS_DIR ${CMAKE_SOURCE_DIR}/tools/depends/target)
  foreach(dvdlib ${dvdlibs})

    get_archive_name(${dvdlib})
    string(TOUPPER ${dvdlib} DVDLIB)

    # allow user to override the download URL with a local tarball
    # needed for offline build envs
    # allow upper and lowercase var name
    if(${dvdlib}_URL)
      set(${DVDLIB}_URL ${${dvdlib}_URL})
    endif()
    if(${DVDLIB}_URL)
      get_filename_component(${DVDLIB}_URL "${${DVDLIB}_URL}" ABSOLUTE)
    else()
      # github tarball format is tagname.tar.gz where tagname is VERSION in lib VERSION file
      set(${DVDLIB}_URL ${${DVDLIB}_BASE_URL}/archive/${${DVDLIB}_VER}.tar.gz)
    endif()
    if(VERBOSE)
      message(STATUS "${DVDLIB}_URL: ${${DVDLIB}_URL}")
    endif()
  endforeach()

  set(DVDREAD_CFLAGS "${DVDREAD_CFLAGS} -I${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/libdvd/include")

  if(APPLE)
    string(APPEND CMAKE_EXE_LINKER_FLAGS "-framework CoreFoundation")
    if(NOT CORE_SYSTEM_NAME STREQUAL darwin_embedded)
      string(APPEND CMAKE_EXE_LINKER_FLAGS " -framework IOKit")
    endif()
  endif()

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

  if(ENABLE_DVDCSS)
    if(NOT CORE_SYSTEM_NAME MATCHES windows)
      set(DVDCSS_LIBRARY ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/libdvd/lib/libdvdcss.a)
      ExternalProject_Add(dvdcss URL ${LIBDVDCSS_URL}
                                  URL_HASH ${LIBDVDCSS_HASH}
                                  DOWNLOAD_NAME ${LIBDVDCSS_ARCHIVE}
                                  DOWNLOAD_DIR ${TARBALL_DIR}
                                  PREFIX ${CORE_BUILD_DIR}/libdvd
                                  CONFIGURE_COMMAND ac_cv_path_GIT= <SOURCE_DIR>/configure
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
                                                    "LDFLAGS=${CMAKE_EXE_LINKER_FLAGS}"
                                  BUILD_COMMAND ${MAKE_COMMAND}
                                  BUILD_BYPRODUCTS ${DVDCSS_LIBRARY})
      addstep_autoreconf("dvdcss")
    else()
      windows_externalprojectadd("dvdcss")
    endif()
    set_target_properties(dvdcss PROPERTIES FOLDER "External Projects")
  endif()

  set(DVDREAD_CFLAGS "-D_XBMC -I${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/libdvd/include")
  if(ENABLE_DVDCSS)
    set(DVDREAD_CFLAGS "${DVDREAD_CFLAGS} -DHAVE_DVDCSS_DVDCSS_H")
  endif()

  if(NOT CORE_SYSTEM_NAME MATCHES windows)
    set(DVDREAD_LIBRARY ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/libdvd/lib/libdvdread.a)
    ExternalProject_Add(dvdread URL ${LIBDVDREAD_URL}
                                URL_HASH ${LIBDVDREAD_HASH}
                                DOWNLOAD_NAME ${LIBDVDREAD_ARCHIVE}
                                DOWNLOAD_DIR ${TARBALL_DIR}
                                PREFIX ${CORE_BUILD_DIR}/libdvd
                                CONFIGURE_COMMAND ac_cv_path_GIT= <SOURCE_DIR>/configure
                                                  --target=${HOST_ARCH}
                                                  --host=${HOST_ARCH}
                                                  --enable-static
                                                  --disable-shared
                                                  --with-pic
                                                  --prefix=<INSTALL_DIR>
                                                  --libdir=<INSTALL_DIR>/lib
                                                  "CC=${CMAKE_C_COMPILER}"
                                                  "CFLAGS=${CMAKE_C_FLAGS} ${DVDREAD_CFLAGS}"
                                                  "LDFLAGS=${CMAKE_EXE_LINKER_FLAGS}"
                                BUILD_COMMAND ${MAKE_COMMAND}
                                BUILD_BYPRODUCTS ${DVDREAD_LIBRARY})
    addstep_autoreconf("dvdread")
  else()
    windows_externalprojectadd("dvdread")
  endif()
  if(ENABLE_DVDCSS)
    add_dependencies(dvdread dvdcss)
  endif()

  set_target_properties(dvdread PROPERTIES FOLDER "External Projects")

  if(ENABLE_DVDCSS)
    set(DVDNAV_LIBS -ldvdcss)
  endif()

  if(NOT CORE_SYSTEM_NAME MATCHES windows)
    set(DVDNAV_LIBRARY ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/libdvd/lib/libdvdnav.a)
    ExternalProject_Add(dvdnav URL ${LIBDVDNAV_URL}
                                URL_HASH ${LIBDVDNAV_HASH}
                                DOWNLOAD_NAME ${LIBDVDNAV_ARCHIVE}
                                DOWNLOAD_DIR ${TARBALL_DIR}
                                PREFIX ${CORE_BUILD_DIR}/libdvd
                                CONFIGURE_COMMAND ac_cv_path_GIT= <SOURCE_DIR>/configure
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
                                                  "LIBS=${DVDNAV_LIBS}"
                                BUILD_COMMAND ${MAKE_COMMAND}
                                BUILD_BYPRODUCTS ${DVDNAV_LIBRARY})
    addstep_autoreconf("dvdnav")
  else()
    set(DVDNAV_LIBRARY ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/libdvd/lib/libdvdnav.lib)
    windows_externalprojectadd("dvdnav")
  endif()
  add_dependencies(dvdnav dvdread)
  set_target_properties(dvdnav PROPERTIES FOLDER "External Projects")

  set(_dvdlibs ${DVDREAD_LIBRARY} ${DVDCSS_LIBRARY})
  if(NOT CORE_SYSTEM_NAME MATCHES windows)
    # link a shared dvdnav library that includes the whole archives of dvdread and dvdcss as well
    # the quotes around _dvdlibs are on purpose, since we want to pass a list to the function that will be unpacked automatically
    core_link_library(${DVDNAV_LIBRARY} system/players/VideoPlayer/libdvdnav dvdnav archives "${_dvdlibs}")
  else()
    set(LIBDVD_TARGET_DIR .)
    if(CORE_SYSTEM_NAME STREQUAL windowsstore)
      set(LIBDVD_TARGET_DIR dlls)
    endif()
    copy_file_to_buildtree(${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/libdvd/bin/libdvdnav.dll DIRECTORY ${LIBDVD_TARGET_DIR})
    add_dependencies(export-files dvdnav)
  endif()

  set(LIBDVD_INCLUDE_DIRS ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/libdvd/include)
  set(LIBDVD_LIBRARIES ${DVDNAV_LIBRARY} ${DVDREAD_LIBRARY})
  if(ENABLE_DVDCSS)
    list(APPEND LIBDVD_LIBRARIES ${DVDCSS_LIBRARY})
  endif()
  set(LIBDVD_LIBRARIES ${LIBDVD_LIBRARIES} CACHE STRING "libdvd libraries" FORCE)
  set(LIBDVD_FOUND 1 CACHE BOOL "libdvd found" FORCE)
endif()

set_property(GLOBAL APPEND PROPERTY INTERNAL_DEPS_PROP dvdnav)
