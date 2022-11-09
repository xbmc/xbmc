
# Check for existing LIBDVDREAD.
# Suppress mismatch warning, see https://cmake.org/cmake/help/latest/module/FindPackageHandleStandardArgs.html
set(FPHSA_NAME_MISMATCHED 1)
find_package(LibDvdNav MODULE REQUIRED)
unset(FPHSA_NAME_MISMATCHED)

set(_dvdlibs ${LIBDVDREAD_LIBRARY} ${LIBDVDCSS_LIBRARY})

if(NOT CORE_SYSTEM_NAME MATCHES windows)
  # link a shared dvdnav library that includes the whole archives of dvdread and dvdcss as well
  # the quotes around _dvdlibs are on purpose, since we want to pass a list to the function that will be unpacked automatically
  core_link_library(${LIBDVDNAV_LIBRARY} system/players/VideoPlayer/libdvdnav libdvdnav archives "${_dvdlibs}")
else()
  set(LIBDVD_TARGET_DIR .)
  copy_file_to_buildtree(${DEPENDS_PATH}/bin/libdvdnav.dll DIRECTORY ${LIBDVD_TARGET_DIR})
  set_property(GLOBAL APPEND PROPERTY INTERNAL_DEPS_PROP LibDvdNav::LibDvdNav)
endif()

set(LIBDVD_INCLUDE_DIRS ${LIBDVDREAD_INCLUDE_DIR} ${LIBDVDNAV_INCLUDE_DIR})
set(LIBDVD_LIBRARIES ${LIBDVDNAV_LIBRARY} ${LIBDVDREAD_LIBRARY})
if(TARGET LibDvdCSS::LibDvdCSS)
  list(APPEND LIBDVD_LIBRARIES ${LIBDVDCSS_LIBRARY})
  list(APPEND LIBDVD_INCLUDE_DIRS ${LIBDVDCSS_INCLUDE_DIR})
endif()
set(LIBDVD_LIBRARIES ${LIBDVD_LIBRARIES} CACHE STRING "libdvd libraries" FORCE)
set(LIBDVD_FOUND 1 CACHE BOOL "libdvd found" FORCE)

mark_as_advanced(LIBDVD_INCLUDE_DIRS LIBDVD_LIBRARIES)
