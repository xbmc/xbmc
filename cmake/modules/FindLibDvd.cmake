
# Check for existing LIBDVDREAD.
# Suppress mismatch warning, see https://cmake.org/cmake/help/latest/module/FindPackageHandleStandardArgs.html
set(FPHSA_NAME_MISMATCHED 1)
find_package(LibDvdNav MODULE REQUIRED)
unset(FPHSA_NAME_MISMATCHED)

set(_dvdlibs LibDvdNav::LibDvdNav
             $<$<TARGET_EXISTS:LibDvdCSS::LibDvdCSS>:LibDvdCSS::LibDvdCSS>>)

if(NOT CORE_SYSTEM_NAME MATCHES windows)
  # link a shared dvdnav library that includes the whole archives of dvdread and dvdcss as well
  # the quotes around _dvdlibs are on purpose, since we want to pass a list to the function that will be unpacked automatically
  core_link_library(LibDvdNav::LibDvdNav system/players/VideoPlayer/libdvdnav libdvdnav archives "${_dvdlibs}")
else()
  set(LIBDVD_TARGET_DIR .)
  copy_file_to_buildtree(${DEPENDS_PATH}/bin/libdvdnav.dll DIRECTORY ${LIBDVD_TARGET_DIR})
endif()

add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} INTERFACE IMPORTED)
set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                 INTERFACE_LINK_LIBRARIES "LibDvdNav::LibDvdNav")
