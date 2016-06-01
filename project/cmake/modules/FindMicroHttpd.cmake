if(NOT WIN32)
  include(FindPkgConfig)
  if( PKG_CONFIG_FOUND )

     pkg_check_modules (MICROHTTPD libmicrohttpd>=0.4)

     set(MICROHTTPD_DEFINITIONS ${MICROHTTPD_CFLAGS_OTHER})
  endif()
endif()

#
# set defaults
if(NOT MICROHTTPD_FOUND)
  set(_microhttpd_HOME "/usr/local")
  set(_microhttpd_INCLUDE_SEARCH_DIRS
    ${CMAKE_INCLUDE_PATH}
    /usr/local/include
    /usr/include
    )

  set(_microhttpd_LIBRARIES_SEARCH_DIRS
    ${CMAKE_LIBRARY_PATH}
    /usr/local/lib
    /usr/lib
    )

  ##
  if( "${MICROHTTPD_HOME}" STREQUAL "")
    if("" MATCHES "$ENV{MICROHTTPD_HOME}")
      message(STATUS "MICROHTTPD_HOME env is not set, setting it to /usr/local")
      set(MICROHTTPD_HOME ${_microhttpd_HOME})
    else()
      set(MICROHTTPD_HOME "$ENV{MICROHTTPD_HOME}")
    endif()
  else()
    message(STATUS "MICROHTTPD_HOME is not empty: \"${MICROHTTPD_HOME}\"")
  endif()
  ##

  message(STATUS "Looking for microhttpd in ${MICROHTTPD_HOME}")

  if( NOT ${MICROHTTPD_HOME} STREQUAL "" )
      set(_microhttpd_INCLUDE_SEARCH_DIRS ${MICROHTTPD_HOME}/include ${_microhttpd_INCLUDE_SEARCH_DIRS})
      set(_microhttpd_LIBRARIES_SEARCH_DIRS ${MICROHTTPD_HOME}/lib ${_microhttpd_LIBRARIES_SEARCH_DIRS})
      set(_microhttpd_HOME ${MICROHTTPD_HOME})
  endif()

  if( NOT $ENV{MICROHTTPD_INCLUDEDIR} STREQUAL "" )
    set(_microhttpd_INCLUDE_SEARCH_DIRS $ENV{MICROHTTPD_INCLUDEDIR} ${_microhttpd_INCLUDE_SEARCH_DIRS})
  endif()

  if( NOT $ENV{MICROHTTPD_LIBRARYDIR} STREQUAL "" )
    set(_microhttpd_LIBRARIES_SEARCH_DIRS $ENV{MICROHTTPD_LIBRARYDIR} ${_microhttpd_LIBRARIES_SEARCH_DIRS})
  endif()

  if( MICROHTTPD_HOME )
    set(_microhttpd_INCLUDE_SEARCH_DIRS ${MICROHTTPD_HOME}/include ${_microhttpd_INCLUDE_SEARCH_DIRS})
    set(_microhttpd_LIBRARIES_SEARCH_DIRS ${MICROHTTPD_HOME}/lib ${_microhttpd_LIBRARIES_SEARCH_DIRS})
    set(_microhttpd_HOME ${MICROHTTPD_HOME})
  endif()

  # find the include files
  find_path(MICROHTTPD_INCLUDE_DIRS microhttpd.h
     HINTS
       ${_microhttpd_INCLUDE_SEARCH_DIRS}
       ${PC_MICROHTTPD_INCLUDEDIR}
       ${PC_MICROHTTPD_INCLUDE_DIRS}
      ${CMAKE_INCLUDE_PATH}
  )

  # locate the library
  if(WIN32)
    set(MICROHTTPD_LIBRARY_NAMES ${MICROHTTPD_LIBRARY_NAMES} libmicrohttpd.lib)
  else()
    set(MICROHTTPD_LIBRARY_NAMES ${MICROHTTPD_LIBRARY_NAMES} libmicrohttpd.a)
  endif()
  find_library(MICROHTTPD_LIBRARIES NAMES ${MICROHTTPD_LIBRARY_NAMES}
    HINTS
      ${_microhttpd_LIBRARIES_SEARCH_DIRS}
      ${PC_MICROHTTPD_LIBDIR}
      ${PC_MICROHTTPD_LIBRARY_DIRS}
  )

  # if the include and the program are found then we have it
  if(MICROHTTPD_INCLUDE_DIRS AND MICROHTTPD_LIBRARIES)
    set(MICROHTTPD_FOUND "YES")
  endif()

  if( NOT WIN32)
    find_library(GCRYPT_LIBRARY gcrypt)
    find_library(GPGERROR_LIBRARY gpg-error)
    list(APPEND MICROHTTPD_LIBRARIES ${GCRYPT_LIBRARY} ${GPGERROR_LIBRARY})
    if(NOT APPLE AND NOT CORE_SYSTEM_NAME STREQUAL android)
      list(APPEND MICROHTTPD_LIBRARIES "-lrt")
    endif()
  endif()
endif()

list(APPEND MICROHTTPD_DEFINITIONS -DHAVE_LIBMICROHTTPD=1)

mark_as_advanced(
  MICROHTTPD_FOUND
  MICROHTTPD_LIBRARIES
  MICROHTTPD_DEFINITIONS
  MICROHTTPD_INCLUDE_DIRS
)
