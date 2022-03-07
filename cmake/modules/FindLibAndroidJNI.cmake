# FindLibAndroidJNI
# -------
# Finds the LibAndroidJNI library
#
# This will define the following variables::
#
# LIBANDROIDJNI_FOUND - system has LibAndroidJNI
# LIBANDROIDJNI_INCLUDE_DIRS - the LibAndroidJNI include directory
# LIBANDROIDJNI_LIBRARIES - the LibAndroidJNI libraries
#
# and the following imported targets::
#
#   libandroidjni   - The LibAndroidJNI library

include(cmake/scripts/common/ModuleHelpers.cmake)

set(MODULE_LC libandroidjni)

SETUP_BUILD_VARS()

set(LIBANDROIDJNI_BUILD_TYPE Release)

# We still need to supply SOMETHING to CMAKE_ARGS to initiate a cmake BUILD_DEP_TARGET
# Setting cmake_build_type twice wont cause issues
set(CMAKE_ARGS -DCMAKE_BUILD_TYPE=Release)

BUILD_DEP_TARGET()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LibAndroidJNI
                                  REQUIRED_VARS LIBANDROIDJNI_LIBRARY LIBANDROIDJNI_INCLUDE_DIR
                                  VERSION_VAR LIBANDROIDJNI_VER)

if(LIBANDROIDJNI_FOUND)
  set(LIBANDROIDJNI_LIBRARIES ${LIBANDROIDJNI_LIBRARY})
  set(LIBANDROIDJNI_INCLUDE_DIRS ${LIBANDROIDJNI_INCLUDE_DIR})

  set_property(GLOBAL APPEND PROPERTY INTERNAL_DEPS_PROP libandroidjni)
endif()
mark_as_advanced(LIBANDROIDJNI_INCLUDE_DIR LIBANDROIDJNI_LIBRARY)
