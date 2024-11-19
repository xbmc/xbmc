# FindLibAndroidJNI
# -------
# Finds the LibAndroidJNI library
#
# This will define the following target:
#
#   libandroidjni::libandroidjni   - The LibAndroidJNI library

if(NOT TARGET libandroidjni::libandroidjni)
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

  add_library(libandroidjni::libandroidjni STATIC IMPORTED)
  set_target_properties(libandroidjni::libandroidjni PROPERTIES
                                                     FOLDER "External Projects"
                                                     IMPORTED_LOCATION "${LIBANDROIDJNI_LIBRARY}"
                                                     INTERFACE_INCLUDE_DIRECTORIES "${LIBANDROIDJNI_INCLUDE_DIR}")

  add_dependencies(libandroidjni::libandroidjni libandroidjni)

  set_property(GLOBAL APPEND PROPERTY INTERNAL_DEPS_PROP libandroidjni::libandroidjni)
endif()
