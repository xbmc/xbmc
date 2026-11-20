#.rst:
# FindCdio
# --------
# Finds the cdio library
#
# This will define the following target:
#
# ${APP_NAME_LC}::Cdio - The Cdio library
#

if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})
  if(WIN32 OR WINDOWS_STORE)
    include(cmake/scripts/common/ModuleHelpers.cmake)

    macro(buildmacroCdio)
      set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_DEBUG_POSTFIX d)
      set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_SHARED_LIB TRUE)

      # libcdio has no upstream cmake build system
      generate_patchcommand("${CMAKE_SOURCE_DIR}/tools/depends/target/libcdio-gplv3/02-win-cmake.patch")

      set(CMAKE_ARGS -DBUILD_SHARED_LIBS=ON)

      BUILD_DEP_TARGET()
    endmacro()

    set(${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC libcdio)
    set(libcdio_MODULE_LOCATION libcdio-gplv3)

    SETUP_BUILD_VARS()

    SETUP_FIND_SPECS()

    SEARCH_EXISTING_PACKAGES()

    if("${${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_VERSION}" VERSION_LESS ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER})
      message(STATUS "Building ${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}: \(version \"${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER}\"\)")

      cmake_language(EVAL CODE "
        buildmacro${CMAKE_FIND_PACKAGE_NAME}()
      ")
    endif()

    if(${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_FOUND)
      set(Cdio_FOUND TRUE)
      set(CDIO_FOUND TRUE)

      if(TARGET libcdio::libcdio AND NOT TARGET ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})
        add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ALIAS libcdio::libcdio)
      else()
        SETUP_BUILD_TARGET()

        add_dependencies(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})
      endif()

      ADD_MULTICONFIG_BUILDMACRO()
    else()
      if(Cdio_FIND_REQUIRED)
        message(FATAL_ERROR "cdio library not found.")
      endif()
    endif()
  else() # NOT WIN32 OR WINDOWS_STORE
    find_package(PkgConfig ${SEARCH_QUIET})

    if(PKG_CONFIG_FOUND)
      pkg_check_modules(PC_CDIO libcdio>=0.80 ${SEARCH_QUIET})
      pkg_check_modules(PC_CDIOPP libcdio++>=2.1.0 ${SEARCH_QUIET})
    endif()

    find_path(CDIO_INCLUDE_DIR NAMES cdio/cdio.h
                               HINTS ${PC_CDIO_INCLUDEDIR}
                                     ${DEPENDS_PATH}/include)

    find_library(CDIO_LIBRARY NAMES cdio libcdio
                              HINTS ${PC_CDIO_LIBDIR}
                                    ${DEPENDS_PATH}/lib)

    if(DEFINED PC_CDIO_VERSION AND DEFINED PC_CDIOPP_VERSION AND NOT "${PC_CDIO_VERSION}" VERSION_EQUAL "${PC_CDIOPP_VERSION}")
      message(WARNING "Detected libcdio (${PC_CDIO_VERSION}) and libcdio++ (${PC_CDIOPP_VERSION}) version mismatch. libcdio++ will not be used.")
    else()
      find_path(CDIOPP_INCLUDE_DIR NAMES cdio++/cdio.hpp
                                   HINTS ${PC_CDIOPP_INCLUDEDIR} ${CDIO_INCLUDE_DIR})

      set(CDIO_VERSION ${PC_CDIO_VERSION})
    endif()

    if(NOT VERBOSE_FIND)
       set(${CMAKE_FIND_PACKAGE_NAME}_FIND_QUIETLY TRUE)
     endif()

    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(Cdio
                                      REQUIRED_VARS CDIO_LIBRARY CDIO_INCLUDE_DIR
                                      VERSION_VAR CDIO_VERSION)

    if(CDIO_FOUND)

      add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} UNKNOWN IMPORTED)
      set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                       IMPORTED_LOCATION "${CDIO_LIBRARY}"
                                                                       INTERFACE_INCLUDE_DIRECTORIES "${CDIO_INCLUDE_DIR}")

      if(CDIOPP_INCLUDE_DIR)
        add_library(${APP_NAME_LC}::CdioPP INTERFACE IMPORTED)
        set_target_properties(${APP_NAME_LC}::CdioPP PROPERTIES
                                                     INTERFACE_INCLUDE_DIRECTORIES "${CDIOPP_INCLUDE_DIR}")
        set_property(TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} APPEND PROPERTY
                                                                              INTERFACE_LINK_LIBRARIES "${APP_NAME_LC}::CdioPP")
      endif()
    else()
      if(Cdio_FIND_REQUIRED)
        message(FATAL_ERROR "cdio library not found.")
      endif()
    endif()
  endif()
endif()
