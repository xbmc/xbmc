# FindLibAndroidJNI
# -------
# Finds the LibAndroidJNI library
#
# This will define the following target:
#
#   ${APP_NAME_LC}::LibAndroidJNI   - The LibAndroidJNI library

if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})
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

  add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} STATIC IMPORTED)
  set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                   FOLDER "External Projects"
                                                                   IMPORTED_LOCATION "${LIBANDROIDJNI_LIBRARY}"
                                                                   INTERFACE_INCLUDE_DIRECTORIES "${LIBANDROIDJNI_INCLUDE_DIR}")

  add_dependencies(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} libandroidjni)
endif()
