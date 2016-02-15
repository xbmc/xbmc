# -*- cmake -*-

# - Find VA-API
# Find the VA-API includes and library
# This module defines
#  VAAPI_INCLUDE_DIRS, where to find db.h, etc.
#  VAAPI_LIBRARIES, the libraries needed to use VA-API.
#  VAAPI_FOUND, If false, do not try to use VA-API.
# also defined, but not for general use are
#  VAAPI_LIBRARY, where to find the VA-API library.

find_path(VAAPI_INCLUDE_DIRS va/va.h)

if(PKG_CONFIG_FOUND)
  pkg_check_modules(VAAPI libva>=0.38 libva-x11)
else()
  find_library(VA_LIBRARY
               NAMES va)

  find_library(VAX11_LIBRARY
               NAMES va-x11)

  set(VAAPI_LIBRARIES ${VAX11_LIBRARY} ${VA_LIBRARY})
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(VAAPI DEFAULT_MSG VAAPI_LIBRARIES)

list(APPEND VAAPI_DEFINITIONS -DHAVE_LIBVA=1)

mark_as_advanced(VAAPI_INCLUDE_DIRS VAAPI_LIBRARIES VAAPI_DEFINITIONS)
