#.rst:
# FindDav1d
# --------
# Finds the dav1d library
#
# This will define the following target:
#
#   ${APP_NAME_LC}::Dav1d   - The dav1d library

if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})
  if((ENABLE_INTERNAL_DAV1D AND ENABLE_INTERNAL_FFMPEG) AND NOT (WIN32 OR WINDOWS_STORE))
    include(cmake/scripts/common/ModuleHelpers.cmake)

    set(${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC dav1d)

    SETUP_BUILD_VARS()

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
  else()
    find_package(PkgConfig ${SEARCH_QUIET})
    # Do not use pkgconfig on windows
    if(PKG_CONFIG_FOUND AND NOT WIN32)
      pkg_check_modules(PC_DAV1D dav1d ${SEARCH_QUIET})
    endif()

    find_library(DAV1D_LIBRARY NAMES dav1d libdav1d
                               HINTS ${DEPENDS_PATH}/lib ${PC_DAV1D_LIBDIR}
                               ${${CORE_SYSTEM_NAME}_SEARCH_CONFIG})

    find_path(DAV1D_INCLUDE_DIR NAMES dav1d/dav1d.h
                                HINTS ${DEPENDS_PATH}/include ${PC_DAV1D_INCLUDEDIR}
                                ${${CORE_SYSTEM_NAME}_SEARCH_CONFIG})

    set(DAV1D_VERSION ${PC_DAV1D_VERSION})
  endif()

  if(NOT VERBOSE_FIND)
     set(${CMAKE_FIND_PACKAGE_NAME}_FIND_QUIETLY TRUE)
   endif()

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(Dav1d
                                    REQUIRED_VARS DAV1D_LIBRARY DAV1D_INCLUDE_DIR
                                    VERSION_VAR DAV1D_VERSION)

  if(DAV1D_FOUND)
    add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} UNKNOWN IMPORTED)
    set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                     IMPORTED_LOCATION "${DAV1D_LIBRARY}"
                                                                     INTERFACE_INCLUDE_DIRECTORIES "${DAV1D_INCLUDE_DIR}")

    if(TARGET ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})
      add_dependencies(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})
    endif()
  endif()
endif()
