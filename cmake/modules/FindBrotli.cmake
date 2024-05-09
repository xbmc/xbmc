#.rst:
# FindBrotli
# ----------
# Finds the Brotli library
#
# This will define the following target ALIAS:
#
#   Brotli::Brotli   - The Brotli library
#
# The following IMPORTED targets are made
#
#   Brotli::BrotliCommon - The brotlicommon library
#   Brotli::BrotliDec - The brotlidec library
#

if(NOT TARGET Brotli::Brotli)
  find_package(PkgConfig)
  if(PKG_CONFIG_FOUND AND NOT (WIN32 OR WINDOWSSTORE))
    pkg_check_modules(BROTLICOMMON libbrotlicommon QUIET)
    # First item is the full path of the library file found
    # pkg_check_modules does not populate a variable of the found library explicitly
    list(GET BROTLICOMMON_LINK_LIBRARIES 0 BROTLICOMMON_LIBRARY)

    pkg_check_modules(BROTLIDEC libbrotlidec QUIET)
    # First item is the full path of the library file found
    # pkg_check_modules does not populate a variable of the found library explicitly
    list(GET BROTLIDEC_LINK_LIBRARIES 0 BROTLIDEC_LIBRARY)

    set(BROTLI_INCLUDE_DIR ${BROTLICOMMON_INCLUDEDIR})
    set(BROTLI_VERSION ${BROTLICOMMON_VERSION})
  else()
    find_path(BROTLI_INCLUDE_DIR NAMES brotli/decode.h
                                 HINTS ${DEPENDS_PATH}/include ${BROTLICOMMON_INCLUDEDIR}
                                 ${${CORE_PLATFORM_LC}_SEARCH_CONFIG})
    find_library(BROTLICOMMON_LIBRARY NAMES brotlicommon
                                      HINTS ${DEPENDS_PATH}/lib ${BROTLICOMON_LIBDIR}
                                      ${${CORE_PLATFORM_LC}_SEARCH_CONFIG})

    find_library(BROTLIDEC_LIBRARY NAMES brotlidec
                                   HINTS ${DEPENDS_PATH}/lib ${BROTLIDEC_LIBDIR}
                                   ${${CORE_PLATFORM_LC}_SEARCH_CONFIG})
  endif()

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(Brotli
                                    REQUIRED_VARS BROTLICOMMON_LIBRARY BROTLIDEC_LIBRARY BROTLI_INCLUDE_DIR
                                    VERSION_VAR BROTLI_VERSION)

  if(BROTLI_FOUND)
    add_library(Brotli::BrotliCommon UNKNOWN IMPORTED)
    set_target_properties(Brotli::BrotliCommon PROPERTIES
                                               IMPORTED_LOCATION "${BROTLICOMMON_LIBRARY}"
                                               INTERFACE_INCLUDE_DIRECTORIES "${BROTLI_INCLUDE_DIR}")

    add_library(Brotli::BrotliDec UNKNOWN IMPORTED)
    set_target_properties(Brotli::BrotliDec PROPERTIES
                                            IMPORTED_LOCATION "${BROTLIDEC_LIBRARY}"
                                            INTERFACE_LINK_LIBRARIES Brotli::BrotliCommon
                                            INTERFACE_INCLUDE_DIRECTORIES "${BROTLI_INCLUDE_DIR}")

    add_library(Brotli::Brotli ALIAS Brotli::BrotliDec)

  else()
    if(Brotli_FIND_REQUIRED)
      message(FATAL_ERROR "Brotli libraries were not found.")
    endif()
  endif()
endif()
