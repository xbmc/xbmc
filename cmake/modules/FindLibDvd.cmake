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
  set(dvdlibs libdvdread libdvdnav)
  if(ENABLE_DVDCSS)
    list(APPEND dvdlibs libdvdcss)
  endif()
  set(DEPENDS_TARGETS_DIR ${CMAKE_SOURCE_DIR}/tools/depends/target)
  foreach(dvdlib ${dvdlibs})
    file(GLOB VERSION_FILE ${DEPENDS_TARGETS_DIR}/${dvdlib}/DVD*-VERSION)
    file(STRINGS ${VERSION_FILE} VER)
    string(REGEX MATCH "VERSION=[^ ]*$.*" ${dvdlib}_VER "${VER}")
    list(GET ${dvdlib}_VER 0 ${dvdlib}_VER)
    string(SUBSTRING "${${dvdlib}_VER}" 8 -1 ${dvdlib}_VER)
    string(REGEX MATCH "BASE_URL=([^ ]*)" ${dvdlib}_BASE_URL "${VER}")
    list(GET ${dvdlib}_BASE_URL 0 ${dvdlib}_BASE_URL)
    string(SUBSTRING "${${dvdlib}_BASE_URL}" 9 -1 ${dvdlib}_BASE_URL)
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
      set(${DVDLIB}_URL ${${dvdlib}_BASE_URL}/archive/${${dvdlib}_VER}.tar.gz)
    endif()
    if(VERBOSE)
      message(STATUS "${DVDLIB}_URL: ${${DVDLIB}_URL}")
    endif()
  endforeach()

  set(DVDREAD_CFLAGS "${DVDREAD_CFLAGS} -I${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/libdvd/include")

  if(APPLE)
    set(CMAKE_LD_FLAGS "-framework IOKit -framework CoreFoundation")
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
                                  DOWNLOAD_NAME libdvdcss-${libdvdcss_VER}.tar.gz
                                  DOWNLOAD_DIR ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/download
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
                                                    "LDFLAGS=${CMAKE_LD_FLAGS}"
                                  BUILD_COMMAND ${MAKE_COMMAND}
                                  BUILD_BYPRODUCTS ${DVDCSS_LIBRARY})
      ExternalProject_Add_Step(dvdcss autoreconf
                                      DEPENDEES download update patch
                                      DEPENDERS configure
                                      COMMAND PATH=${NATIVEPREFIX}/bin:$ENV{PATH} autoreconf -vif
                                      WORKING_DIRECTORY <SOURCE_DIR>)
    else()
      ExternalProject_Add(dvdcss
        URL ${LIBDVDCSS_URL}
        DOWNLOAD_DIR ${CMAKE_SOURCE_DIR}/project/BuildDependencies/downloads
        DOWNLOAD_NAME libdvdcss-${libdvdcss_VER}.tar.gz
        CMAKE_ARGS
          ${LIBDVD_ADDITIONAL_ARGS}
          -DCMAKE_INSTALL_PREFIX:PATH=${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/libdvd
      )
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
                                DOWNLOAD_NAME libdvdread-${libdvdread_VER}.tar.gz
                                DOWNLOAD_DIR ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/download
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
                                                  "LDFLAGS=${CMAKE_LD_FLAGS}"
                                BUILD_COMMAND ${MAKE_COMMAND}
                                BUILD_BYPRODUCTS ${DVDREAD_LIBRARY})
    ExternalProject_Add_Step(dvdread autoreconf
                                      DEPENDEES download update patch
                                      DEPENDERS configure
                                      COMMAND PATH=${NATIVEPREFIX}/bin:$ENV{PATH} autoreconf -vif
                                      WORKING_DIRECTORY <SOURCE_DIR>)
  else()
    ExternalProject_Add(dvdread
      URL ${LIBDVDREAD_URL}
      DOWNLOAD_DIR ${CMAKE_SOURCE_DIR}/project/BuildDependencies/downloads
      DOWNLOAD_NAME libdvdread-${libdvdread_VER}.tar.gz
      CMAKE_ARGS
        ${LIBDVD_ADDITIONAL_ARGS}
        -DCMAKE_PREFIX_PATH:PATH=${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/libdvd
        -DCMAKE_INSTALL_PREFIX:PATH=${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/libdvd
    )
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
                                DOWNLOAD_NAME libdvdnav-${libdvdnav_VER}.tar.gz
                                DOWNLOAD_DIR ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/download
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
                                                  "LDFLAGS=${CMAKE_LD_FLAGS} -L${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/libdvd/lib"
                                                  "CFLAGS=${CMAKE_C_FLAGS} ${DVDREAD_CFLAGS}"
                                                  "DVDREAD_CFLAGS=${DVDREAD_CFLAGS}"
                                                  "DVDREAD_LIBS=${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/libdvd/lib/libdvdread.la"
                                                  "LIBS=${DVDNAV_LIBS}"
                                BUILD_COMMAND ${MAKE_COMMAND}
                                BUILD_BYPRODUCTS ${DVDNAV_LIBRARY})
    ExternalProject_Add_Step(dvdnav autoreconf
                                    DEPENDEES download update patch
                                    DEPENDERS configure
                                    COMMAND PATH=${NATIVEPREFIX}/bin:$ENV{PATH} autoreconf -vif
                                    WORKING_DIRECTORY <SOURCE_DIR>)
  else()
    set(DVDNAV_LIBRARY ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/libdvd/lib/libdvdnav.lib)
    ExternalProject_Add(dvdnav
      URL ${LIBDVDNAV_URL}
      DOWNLOAD_DIR ${CMAKE_SOURCE_DIR}/project/BuildDependencies/downloads
      DOWNLOAD_NAME libdvdnav-${libdvdnav_VER}.tar.gz
      CMAKE_ARGS
        ${LIBDVD_ADDITIONAL_ARGS}
        -DCMAKE_PREFIX_PATH:PATH=${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/libdvd
        -DCMAKE_INSTALL_PREFIX:PATH=${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/libdvd
    )
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
