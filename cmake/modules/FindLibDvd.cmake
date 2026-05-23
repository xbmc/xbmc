#.rst:
# FindLibDvd
# ----------
#
# This will define the following target:
#
#   ${APP_NAME_LC}::LibDvd  - Wrapper target to generate/build libdvdnav shared library

if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})
  find_package(LibDvdNav REQUIRED)

  # Nothing to be done for windows, as binary dll's are copied to output folder
  # as part of cmake/installdata/windows*/dlls.txt
  if(NOT CORE_SYSTEM_NAME MATCHES windows)
    set(_dvdlibs LIBRARY::LibDvdRead)

    if(TARGET LIBRARY::LibDvdCSS)
      list(APPEND _dvdlibs LIBRARY::LibDvdCSS)
    endif()

    # link a shared dvdnav library that includes the whole archives of dvdread and dvdcss as well
    # the quotes around _dvdlibs are on purpose, since we want to pass a list to the function that will be unpacked automatically
    core_link_library(LIBRARY::LibDvdNav system/players/VideoPlayer/libdvdnav archives "${_dvdlibs}")
  endif()

  add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} INTERFACE IMPORTED)
  add_dependencies(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} LIBRARY::LibDvdNav)
endif()
