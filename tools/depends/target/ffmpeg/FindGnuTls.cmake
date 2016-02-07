# - Try to find gnutls
# Once done this will define
#
# GNUTLS_FOUND - system has gnutls
# GNUTLS_INCLUDE_DIRS - the gnutls include directory
# GNUTLS_LIBRARIES - The gnutls libraries

include(FindPkgConfig)
find_package(PkgConfig)

if(PKG_CONFIG_FOUND)
  pkg_check_modules (GNUTLS gnutls)
endif()

if(NOT GNUTLS_FOUND)
  find_path(GNUTLS_INCLUDE_DIRS gnutls/gnutls.h)
  find_library(GNUTLS_LIBRARIES gnutls)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GnuTls DEFAULT_MSG GNUTLS_INCLUDE_DIRS GNUTLS_LIBRARIES)

if(GNUTLS_FOUND)
  list(APPEND GNUTLS_DEFINITIONS -DHAVE_GNUTLS=1)
endif()

mark_as_advanced(GNUTLS_INCLUDE_DIRS GNUTLS_LIBRARIES GNUTLS_DEFINITIONS)
