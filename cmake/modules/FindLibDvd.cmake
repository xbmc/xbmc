#.rst:
# FindLibDvd
# ----------
#
# This will define the following target:
#
#   ${APP_NAME_LC}::LibDvd  - Wrapper target to generate/build libdvdnav shared library

if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})
  find_package(LibDvdNav MODULE REQUIRED)

  if(CORE_SYSTEM_NAME MATCHES windows)
    set(LIBDVD_TARGET_DIR .)
    copy_file_to_buildtree(${DEPENDS_PATH}/bin/libdvdnav.dll DIRECTORY ${LIBDVD_TARGET_DIR})
  else()
    set(_dvdlibs ${APP_NAME_LC}::LibDvdRead)

    if(TARGET ${APP_NAME_LC}::LibDvdCSS)
      list(APPEND _dvdlibs ${APP_NAME_LC}::LibDvdCSS)
    endif()

    # link a shared dvdnav library that includes the whole archives of dvdread and dvdcss as well
    # the quotes around _dvdlibs are on purpose, since we want to pass a list to the function that will be unpacked automatically
    core_link_library(${APP_NAME_LC}::LibDvdNav system/players/VideoPlayer/libdvdnav archives "${_dvdlibs}")
  endif()

  add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} INTERFACE IMPORTED)
  add_dependencies(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ${APP_NAME_LC}::LibDvdNav)
endif()
