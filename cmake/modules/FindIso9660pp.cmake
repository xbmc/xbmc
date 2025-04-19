#.rst:
# FindIso9660pp
# --------
# Finds the iso9660++ library
#
# This will define the following target:
#
#   ${APP_NAME_LC}::Iso9660pp - The Iso9660pp library

if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})

  include(cmake/scripts/common/ModuleHelpers.cmake)

  SETUP_FIND_SPECS()

  find_package(Cdio ${SEARCH_QUIET})

  if(Cdio_FOUND)
    find_package(PkgConfig ${SEARCH_QUIET})
    if(PKG_CONFIG_FOUND AND NOT (WIN32 OR WINDOWS_STORE))
      pkg_check_modules(PC_ISO9660PP libiso9660++${PC_${CMAKE_FIND_PACKAGE_NAME}_FIND_SPEC} ${SEARCH_QUIET})
      pkg_check_modules(PC_ISO9660 libiso9660${PC_${CMAKE_FIND_PACKAGE_NAME}_FIND_SPEC} ${SEARCH_QUIET})
    endif()

    find_path(ISO9660PP_INCLUDE_DIR NAMES cdio++/iso9660.hpp
                                    HINTS ${DEPENDS_PATH}/include ${PC_ISO9660PP_INCLUDEDIR}
                                    ${${CORE_PLATFORM_LC}_SEARCH_CONFIG})

    find_library(ISO9660PP_LIBRARY NAMES libiso9660++ iso9660++
                                   HINTS ${DEPENDS_PATH}/lib ${PC_ISO9660PP_LIBDIR}
                                   ${${CORE_PLATFORM_LC}_SEARCH_CONFIG})

    find_path(ISO9660_INCLUDE_DIR NAMES cdio/iso9660.h
                                  HINTS ${DEPENDS_PATH}/include ${PC_ISO9660_INCLUDEDIR}
                                  ${${CORE_PLATFORM_LC}_SEARCH_CONFIG})

    find_library(ISO9660_LIBRARY NAMES libiso9660 iso9660
                                 HINTS ${DEPENDS_PATH}/lib ${PC_ISO9660_LIBDIR}
                                 ${${CORE_PLATFORM_LC}_SEARCH_CONFIG})

    set(ISO9660PP_VERSION ${PC_ISO9660PP_VERSION})

    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(Iso9660pp
                                      REQUIRED_VARS ISO9660PP_LIBRARY ISO9660PP_INCLUDE_DIR ISO9660_LIBRARY ISO9660_INCLUDE_DIR
                                      VERSION_VAR ISO9660PP_VERSION)

    if(ISO9660PP_FOUND)
      add_library(${APP_NAME_LC}::Iso9660 UNKNOWN IMPORTED)
      set_target_properties(${APP_NAME_LC}::Iso9660 PROPERTIES
                                                    IMPORTED_LOCATION "${ISO9660_LIBRARY}"
                                                    INTERFACE_INCLUDE_DIRECTORIES "${ISO9660_INCLUDE_DIR}")

      add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} UNKNOWN IMPORTED)
      set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                       IMPORTED_LOCATION "${ISO9660PP_LIBRARY}"
                                                                       INTERFACE_INCLUDE_DIRECTORIES "${ISO9660PP_INCLUDE_DIR}"
                                                                       INTERFACE_LINK_LIBRARIES "${APP_NAME_LC}::Iso9660;${APP_NAME_LC}::Cdio"
                                                                       INTERFACE_COMPILE_DEFINITIONS HAS_ISO9660PP)
    endif()
  else()
    include(FindPackageMessage)
    find_package_message(Iso9660pp "Iso9660pp: Can not find libcdio (REQUIRED)" "")
  endif()
endif()
