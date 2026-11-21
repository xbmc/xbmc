#.rst:
# FindPlist
# ---------
# Finds the Plist library
#
# This will define the following target:
#
#   ${APP_NAME_LC}::Plist - The Plist library

if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})
  if(WIN32 AND NOT WINDOWS_STORE) # Windows Desktop only
    include(cmake/scripts/common/ModuleHelpers.cmake)

    macro(buildmacroPlist)
      set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_DEBUG_POSTFIX d)
      set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_SHARED_LIB TRUE)

      # libplist has no upstream cmake build system
      generate_patchcommand("${CMAKE_SOURCE_DIR}/tools/depends/target/libplist/01-win-cmake.patch")

      set(CMAKE_ARGS -DBUILD_SHARED_LIBS=ON)

      BUILD_DEP_TARGET()
    endmacro()

    set(${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC libplist)

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
      set(Plist_FOUND TRUE)
      set(PLIST_FOUND TRUE)

      if(TARGET libplist::libplist AND NOT TARGET ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})
        add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ALIAS libplist::libplist)
      else()
        SETUP_BUILD_TARGET()

        add_dependencies(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})
      endif()

      set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_COMPILE_DEFINITIONS HAS_AIRPLAY)
      ADD_TARGET_COMPILE_DEFINITION()

      ADD_MULTICONFIG_BUILDMACRO()
    endif()
  else() # NOT Windows Desktop
    find_package(PkgConfig ${SEARCH_QUIET})
    if(PKG_CONFIG_FOUND AND NOT WINDOWS_STORE)
      pkg_search_module(PC_PLIST libplist-2.0 libplist ${SEARCH_QUIET})
    endif()

    find_path(PLIST_INCLUDE_DIR plist/plist.h
                                HINTS ${DEPENDS_PATH}/include ${PC_PLIST_INCLUDEDIR}
                                ${${CORE_SYSTEM_NAME}_SEARCH_CONFIG})
    find_library(PLIST_LIBRARY NAMES plist-2.0 plist libplist-2.0 libplist
                               HINTS ${DEPENDS_PATH}/lib ${PC_PLIST_LIBDIR}
                               ${${CORE_SYSTEM_NAME}_SEARCH_CONFIG})

    set(PLIST_VERSION ${PC_PLIST_VERSION})

    if(NOT VERBOSE_FIND)
       set(${CMAKE_FIND_PACKAGE_NAME}_FIND_QUIETLY TRUE)
     endif()

    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(Plist
                                      REQUIRED_VARS PLIST_LIBRARY PLIST_INCLUDE_DIR
                                      VERSION_VAR PLIST_VERSION)

    if(PLIST_FOUND)
      add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} UNKNOWN IMPORTED)
      set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                       IMPORTED_LOCATION "${PLIST_LIBRARY}"
                                                                       INTERFACE_INCLUDE_DIRECTORIES "${PLIST_INCLUDE_DIR}"
                                                                       INTERFACE_COMPILE_DEFINITIONS HAS_AIRPLAY)
    endif()
  endif()
endif()
