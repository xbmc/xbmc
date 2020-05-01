if(KODI_DEPENDSBUILD OR NOT ENABLE_INTERNAL_LIBDVD)
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

  if(CORE_SYSTEM_NAME STREQUAL windowsstore)
    set(LIBDVD_ADDITIONAL_ARGS "-DCMAKE_SYSTEM_NAME=${CMAKE_SYSTEM_NAME}" "-DCMAKE_SYSTEM_VERSION=${CMAKE_SYSTEM_VERSION}")
  endif()

  if(ENABLE_DVDCSS)
    if(NOT CORE_SYSTEM_NAME MATCHES windows)

      include(${WITH_KODI_DEPENDS}/packages/libdvdcss/package.cmake)
      add_depends_for_targets("HOST")

      add_custom_target(dvdcss ALL DEPENDS libdvdcss-host)

      set(DVDCSS_LIBRARY ${INSTALL_PREFIX_HOST}/lib/libdvdcss.a)

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

  if(NOT CORE_SYSTEM_NAME MATCHES windows)

    include(${WITH_KODI_DEPENDS}/packages/libdvdread/package.cmake)
    add_depends_for_targets("HOST")

    add_custom_target(dvdread ALL DEPENDS libdvdread-host)

    set(DVDREAD_LIBRARY ${INSTALL_PREFIX_HOST}/lib/libdvdread.a)

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

  if(NOT CORE_SYSTEM_NAME MATCHES windows)

    include(${WITH_KODI_DEPENDS}/packages/libdvdnav/package.cmake)
    add_depends_for_targets("HOST")

    set(DVDNAV_LIBRARY ${INSTALL_PREFIX_HOST}/lib/libdvdnav.a)

    add_custom_target(dvdnav ALL DEPENDS libdvdnav-host)

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
  set_target_properties(dvdnav PROPERTIES FOLDER "External Projects"
                                          IMPORTED_LOCATION "${DVDNAV_LIBRARY}")

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
