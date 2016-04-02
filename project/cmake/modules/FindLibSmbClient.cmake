# - Try to find Libsmbclient

if(PKGCONFIG_FOUND)
  pkg_check_modules(LIBSMBCLIENT smbclient)
  set(LIBSMBCLIENT_DEFINITIONS -DHAVE_LIBSMBCLIENT=1)
endif()

if (LIBSMBCLIENT_LIBRARIES AND LIBSMBCLIENT_INCLUDE_DIRS)
  # in cache already
  set(LIBSMBCLIENT_FOUND TRUE)
else (LIBSMBCLIENT_LIBRARIES AND LIBSMBCLIENT_INCLUDE_DIRS)
  find_path(LIBSMBCLIENT_INCLUDE_DIR
    NAMES
      libsmbclient.h
    PATHS
      /usr/include
      /usr/local/include
      /opt/local/include
      /sw/include
  )

  find_library(SMBCLIENT_LIBRARY
    NAMES
      smbclient
    PATHS
      /usr/lib
      /usr/local/lib
      /opt/local/lib
      /sw/lib
  )

  if (SMBCLIENT_LIBRARY)
    set(SMBCLIENT_FOUND TRUE)
  endif (SMBCLIENT_LIBRARY)

  set(LIBSMBCLIENT_INCLUDE_DIRS
    ${LIBSMBCLIENT_INCLUDE_DIR}
  )

  if (SMBCLIENT_FOUND)
    set(LIBSMBCLIENT_LIBRARIES
      ${LIBSMBCLIENT_LIBRARIES}
      ${SMBCLIENT_LIBRARY}
    )
  endif (SMBCLIENT_FOUND)

  if (LIBSMBCLIENT_INCLUDE_DIRS AND LIBSMBCLIENT_LIBRARIES)
     set(LIBSMBCLIENT_FOUND TRUE)
  endif (LIBSMBCLIENT_INCLUDE_DIRS AND LIBSMBCLIENT_LIBRARIES)

  if (LIBSMBCLIENT_FOUND)
    if (NOT Libsmbclient_FIND_QUIETLY)
      message(STATUS "Found Libsmbclient: ${LIBSMBCLIENT_LIBRARIES}")
    endif (NOT Libsmbclient_FIND_QUIETLY)
  else (LIBSMBCLIENT_FOUND)
    if (Libsmbclient_FIND_REQUIRED)
      message(FATAL_ERROR "Could not find Libsmbclient")
    endif (Libsmbclient_FIND_REQUIRED)
  endif (LIBSMBCLIENT_FOUND)
  set(LIBSMBCLIENT_DEFINITIONS -DHAVE_LIBSMBCLIENT=1)

  # show the LIBSMBCLIENT_INCLUDE_DIRS and LIBSMBCLIENT_LIBRARIES variables only in the advanced view
  mark_as_advanced(LIBSMBCLIENT_INCLUDE_DIRS LIBSMBCLIENT_LIBRARIES LIBSMBCLIENT_DEFINITIONS)

endif (LIBSMBCLIENT_LIBRARIES AND LIBSMBCLIENT_INCLUDE_DIRS)
