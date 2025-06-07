#.rst:
# FindDav1d
# --------
# Finds the dav1d library
#
# This will define the following target:
#
#   ${APP_NAME_LC}::Dav1d   - The dav1d library

if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})
  include(cmake/scripts/common/ModuleHelpers.cmake)

  set(${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC dav1d)

  SETUP_BUILD_VARS()

  SETUP_FIND_SPECS()

  SEARCH_EXISTING_PACKAGES()

  if((ENABLE_INTERNAL_DAV1D AND ENABLE_INTERNAL_FFMPEG) AND NOT (WIN32 OR WINDOWS_STORE))

    set(DAV1D_VERSION ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER})

    find_program(NINJA_EXECUTABLE ninja REQUIRED)
    find_program(MESON_EXECUTABLE meson REQUIRED)

    set(CONFIGURE_COMMAND ${MESON_EXECUTABLE}
                          --buildtype=release
                          --default-library=static
                          --prefix=${DEPENDS_PATH}
                          --libdir=lib
                          -Denable_asm=true
                          -Denable_tools=false
                          -Denable_examples=false
                          -Denable_tests=false
                          ../${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})
    set(BUILD_COMMAND ${NINJA_EXECUTABLE})
    set(INSTALL_COMMAND ${NINJA_EXECUTABLE} install)

    BUILD_DEP_TARGET()
  endif()

  if(${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_FOUND)
    if(TARGET PkgConfig::${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME} AND NOT TARGET ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})
      add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ALIAS PkgConfig::${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME})
    else()
      SETUP_BUILD_TARGET()
      add_dependencies(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})
    endif()
  endif()
endif()
