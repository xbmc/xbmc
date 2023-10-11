#.rst:
# FindICONV
# --------
# Finds the ICONV library
#
# This will define the following target:
#
#   ICONV::ICONV - The ICONV library

if(NOT TARGET ICONV::ICONV)
  find_path(ICONV_INCLUDE_DIR NAMES iconv.h
                              HINTS ${DEPENDS_PATH}/include
                              NO_CACHE)

  find_library(ICONV_LIBRARY NAMES iconv libiconv c
                             HINTS ${DEPENDS_PATH}/lib
                             NO_CACHE)

  set(CMAKE_REQUIRED_LIBRARIES ${ICONV_LIBRARY})
  check_function_exists(iconv HAVE_ICONV_FUNCTION)
  if(NOT HAVE_ICONV_FUNCTION)
    check_function_exists(libiconv HAVE_LIBICONV_FUNCTION2)
    set(HAVE_ICONV_FUNCTION ${HAVE_LIBICONV_FUNCTION2})
    unset(HAVE_LIBICONV_FUNCTION2)
  endif()

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(Iconv
                                    REQUIRED_VARS ICONV_LIBRARY ICONV_INCLUDE_DIR HAVE_ICONV_FUNCTION)

  if(ICONV_FOUND)
    # Libc causes grief for linux, so search if found library is libc.* and only
    # create imported TARGET if its not
    if(NOT ${ICONV_LIBRARY} MATCHES ".*libc\..*")
      add_library(ICONV::ICONV UNKNOWN IMPORTED)
      set_target_properties(ICONV::ICONV PROPERTIES
                                         IMPORTED_LOCATION "${ICONV_LIBRARY}"
                                         INTERFACE_INCLUDE_DIRECTORIES "${ICONV_INCLUDE_DIR}")
      set_property(GLOBAL APPEND PROPERTY INTERNAL_DEPS_PROP ICONV::ICONV)
    endif()
  endif()
endif()
