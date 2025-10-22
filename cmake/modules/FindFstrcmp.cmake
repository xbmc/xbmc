#.rst:
# FindFstrcmp
# --------
# Finds the fstrcmp library
#
# This will define the following target:
#
#   ${APP_NAME_LC}::Fstrcmp   - The fstrcmp library
#

if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})
  if(ENABLE_INTERNAL_FSTRCMP)
    find_program(LIBTOOL libtool REQUIRED)
    include(cmake/scripts/common/ModuleHelpers.cmake)

    set(${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC fstrcmp)

    SETUP_BUILD_VARS()

    find_program(AUTORECONF autoreconf REQUIRED)

    set(CONFIGURE_COMMAND ${AUTORECONF} -vif
                  COMMAND ./configure --prefix ${DEPENDS_PATH})
    set(BUILD_COMMAND make lib/libfstrcmp.la)
    set(BUILD_IN_SOURCE 1)
    set(INSTALL_COMMAND make install-libdir install-include)

    BUILD_DEP_TARGET()
  else()
    find_package(PkgConfig ${SEARCH_QUIET})
    if(PKG_CONFIG_FOUND AND NOT (WIN32 OR WINDOWS_STORE))
      pkg_check_modules(PC_FSTRCMP fstrcmp ${SEARCH_QUIET})
    endif()

    find_path(FSTRCMP_INCLUDE_DIR NAMES fstrcmp.h
                                  HINTS ${DEPENDS_PATH}/include ${PC_FSTRCMP_INCLUDEDIR}
                                  ${${CORE_SYSTEM_NAME}_SEARCH_CONFIG})

    find_library(FSTRCMP_LIBRARY NAMES fstrcmp
                                 HINTS ${DEPENDS_PATH}/lib ${PC_FSTRCMP_LIBDIR}
                                 ${${CORE_SYSTEM_NAME}_SEARCH_CONFIG})

    set(FSTRCMP_VER ${PC_FSTRCMP_VERSION})
  endif()

  if(NOT VERBOSE_FIND)
     set(${CMAKE_FIND_PACKAGE_NAME}_FIND_QUIETLY TRUE)
   endif()

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(Fstrcmp
                                    REQUIRED_VARS FSTRCMP_LIBRARY FSTRCMP_INCLUDE_DIR
                                    VERSION_VAR FSTRCMP_VER)

  if(FSTRCMP_FOUND)
    add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} UNKNOWN IMPORTED)
    set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                     IMPORTED_LOCATION "${FSTRCMP_LIBRARY}"
                                                                     INTERFACE_INCLUDE_DIRECTORIES "${FSTRCMP_INCLUDE_DIR}")

    if(TARGET ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})
      add_dependencies(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})
    endif()
  else()
    if(Fstrcmp_FIND_REQUIRED)
      message(FATAL_ERROR "Fstrcmp libraries were not found. You may want to use -DENABLE_INTERNAL_FSTRCMP=ON")
    endif()
  endif()
endif()
