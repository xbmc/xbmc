# FindPythonModule-dummylib
# --------
# Android requires linking with the eventual shared library of Kodi.
# As we dont have this available, we use a dummy shared lib to mock
# the linking required
#
# --------
#
# This module will define the following variables:
#
# Python::dummylib - The libkodi dummy lib
#
# --------
#

if(NOT TARGET Python::dummylib)

  find_library(DUMMYLIB_LIBRARY NAMES dummy-lib${APP_NAME_LC}/lib${APP_NAME_LC}.so 
                                HINTS ${DEPENDS_PATH}/lib
                                NO_CACHE)

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(Pythonmodule-dummylib
                                    REQUIRED_VARS DUMMYLIB_LIBRARY)

  if(Pythonmodule-dummylib_FOUND)
    add_library(Python::dummylib UNKNOWN IMPORTED)
    set_target_properties(Python::dummylib PROPERTIES
                                           IMPORTED_LOCATION "${DUMMYLIB_LIBRARY}")

  endif()
endif()