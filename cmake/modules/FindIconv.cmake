#.rst:
# FindIconv
# --------
# Finds the ICONV library
#
# This will define the following targets:
#
#   ${APP_NAME_LC}::Iconv - An alias of the Iconv::Iconv target
#   Iconv::Iconv - The ICONV library

if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})

  # We do this dance to utilise cmake system FindIconv. Saves us dealing with it
  set(_temp_CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH})
  unset(CMAKE_MODULE_PATH)

  if(Iconv_FIND_REQUIRED)
    set(ICONV_REQUIRED "REQUIRED")
  endif()

  find_package(Iconv ${ICONV_REQUIRED})

  # Back to our normal module paths
  set(CMAKE_MODULE_PATH ${_temp_CMAKE_MODULE_PATH})

  if(ICONV_FOUND)
    # We still want to Alias its "standard" target to our APP_NAME_LC based target
    # for integration into our core dep packaging
    add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ALIAS Iconv::Iconv)
  else()
    if(Iconv_FIND_REQUIRED)
      message(FATAL_ERROR "Iconv libraries were not found.")
    endif()
  endif()
endif()
