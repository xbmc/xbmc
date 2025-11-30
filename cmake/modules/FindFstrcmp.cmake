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
  include(cmake/scripts/common/ModuleHelpers.cmake)

  set(${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC fstrcmp)

  SETUP_BUILD_VARS()

  if(ENABLE_INTERNAL_FSTRCMP)
    message(STATUS "Building ${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}: \(version \"${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER}\"\)")
    find_program(LIBTOOL libtool REQUIRED)

    find_program(AUTORECONF autoreconf REQUIRED)

    set(CONFIGURE_COMMAND ${AUTORECONF} -vif
                  COMMAND ./configure --prefix ${DEPENDS_PATH})
    set(BUILD_COMMAND make lib/libfstrcmp.la)
    set(BUILD_IN_SOURCE 1)
    set(INSTALL_COMMAND make install-libdir install-include)

    BUILD_DEP_TARGET()
  else()
    SETUP_FIND_SPECS()
  
    SEARCH_EXISTING_PACKAGES()
  endif()

  if(${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_FOUND)
    if(TARGET PkgConfig::${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME})
      add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ALIAS PkgConfig::${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME})
    elseif(TARGET fstrcmp::fstrcmp)
      add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ALIAS fstrcmp::fstrcmp)
    else()
      SETUP_BUILD_TARGET()
      add_dependencies(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})
    endif()
  else()
    if(Fstrcmp_FIND_REQUIRED)
      message(FATAL_ERROR "Fstrcmp libraries were not found. You may want to use -DENABLE_INTERNAL_FSTRCMP=ON")
    endif()
  endif()
endif()
