# FindPythonmoduledummylib
# --------
# Android requires linking with the eventual shared library of Kodi.
# As we dont have this available, we use a dummy shared lib to mock
# the linking required
#
# --------
#
# This module will define the following variables:
#
# Python::Pythonmoduledummylib - The libkodi dummy lib
#
# --------
#

if(NOT TARGET Python::${CMAKE_FIND_PACKAGE_NAME})

  find_library(DUMMYLIB_LIBRARY NAMES lib${APP_NAME_LC}.so
                                HINTS ${DEPENDS_PATH}/lib/dummy-lib${APP_NAME_LC})

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(Pythonmoduledummylib
                                    REQUIRED_VARS DUMMYLIB_LIBRARY)

  if(Pythonmoduledummylib_FOUND)
    add_library(Python::${CMAKE_FIND_PACKAGE_NAME} UNKNOWN IMPORTED)
    set_target_properties(Python::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                             IMPORTED_LOCATION "${DUMMYLIB_LIBRARY}")

  endif()
endif()
