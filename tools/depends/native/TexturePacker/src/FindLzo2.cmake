#.rst:
# FindLzo2
# --------
# Finds the Lzo2 library
#
# This will define the following target:
#
#   ${APP_NAME_LC}::Lzo2   - The Lzo2 library

if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})

  find_path(LZO2_INCLUDE_DIR NAMES lzo1x.h
                             PATH_SUFFIXES lzo
                             HINTS ${DEPENDS_PATH}/include
                             ${${CORE_PLATFORM_NAME_LC}_SEARCH_CONFIG})

  find_library(LZO2_LIBRARY NAMES lzo2 liblzo2
                            HINTS ${DEPENDS_PATH}/lib
                            ${${CORE_PLATFORM_NAME_LC}_SEARCH_CONFIG})

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(Lzo2
                                    REQUIRED_VARS LZO2_LIBRARY LZO2_INCLUDE_DIR)

  if(LZO2_FOUND)
    add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} UNKNOWN IMPORTED)
    set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                     IMPORTED_LOCATION "${LZO2_LIBRARY}"
                                                                     INTERFACE_INCLUDE_DIRECTORIES "${LZO2_INCLUDE_DIR}")
  else()
    if(LibLzo2_FIND_REQUIRED)
      message(FATAL_ERROR "Lzo2 library was not found.")
    endif()
  endif()
endif()
