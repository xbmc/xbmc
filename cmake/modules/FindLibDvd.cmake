#.rst:
# FindLibDvd
# ----------
#
# This will define the following target:
#
#   ${APP_NAME_LC}::Dvd  - Wrapper target to generate/build libdvdnav shared library

if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})
  find_package(LibDvdNav MODULE REQUIRED)

  set(_dvdlibs LibDvdNav::LibDvdNav
               $<$<TARGET_EXISTS:LibDvdCSS::LibDvdCSS>:LibDvdCSS::LibDvdCSS>>)

  if(CORE_SYSTEM_NAME MATCHES windows)
    set(LIBDVD_TARGET_DIR .)
    copy_file_to_buildtree($<TARGET_FILE:LibDvdNav::LibDvdNav> DIRECTORY ${LIBDVD_TARGET_DIR})
  else()
    # link a shared dvdnav library that includes the whole archives of dvdread and dvdcss as well
    # the quotes around _dvdlibs are on purpose, since we want to pass a list to the function that will be unpacked automatically
    core_link_library(LibDvdNav::LibDvdNav system/players/VideoPlayer/libdvdnav libdvdnav archives "${_dvdlibs}")
  endif()

  add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} INTERFACE IMPORTED)
  add_dependencies(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} LibDvdNav::LibDvdNav)
endif()
