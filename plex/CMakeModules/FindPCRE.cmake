# - Try to find the PCRE regular expression library
# Once done this will define
#
#  PCRE_FOUND - system has the PCRE library
#  PCRE_INCLUDE_DIR - the PCRE include directory
#  PCRE_LIBRARIES - The libraries needed to use PCRE

# Copyright (c) 2006, Alexander Neundorf, <neundorf@kde.org>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

if (PCRE_INCLUDE_DIR AND PCRE_PCRECPP_LIBRARY AND PCRE_PCRE_LIBRARY)
  # Already in cache, be silent
  set(PCRE_FIND_QUIETLY TRUE)
endif (PCRE_INCLUDE_DIR AND PCRE_PCRECPP_LIBRARY AND PCRE_PCRE_LIBRARY)
	
if (NOT WIN32)
  # use pkg-config to get the directories and then use these values
  # in the FIND_PATH() and FIND_LIBRARY() calls
  find_package(PkgConfig)
  pkg_check_modules(PC_PCRE QUIET libpcre)
  set(PCRE_DEFINITIONS ${PC_PCRE_CFLAGS_OTHER})
endif (NOT WIN32)

find_path(PCRE_INCLUDE_DIR pcre.h
          HINTS ${PC_PCRE_INCLUDEDIR} ${PC_PCRE_INCLUDE_DIRS}
          PATH_SUFFIXES pcre)

find_library(PCRE_PCRE_LIBRARY NAMES pcre HINTS ${PC_PCRE_LIBDIR} ${PC_PCRE_LIBRARY_DIRS})
find_library(PCRE_PCRECPP_LIBRARY NAMES pcrecpp HINTS ${PC_PCRE_LIBDIR} ${PC_PCRE_LIBRARY_DIRS})

include(FindPackageHandleStandardArgs)

IF(NOT WIN32)
  find_package_handle_standard_args(PCRE DEFAULT_MSG PCRE_INCLUDE_DIR PCRE_PCRE_LIBRARY PCRE_PCRECPP_LIBRARY )
  mark_as_advanced(PCRE_INCLUDE_DIR PCRE_LIBRARIES PCRE_PCRECPP_LIBRARY PCRE_PCRE_LIBRARY)
  set(PCRE_LIBRARIES ${PCRE_PCRECPP_LIBRARY} ${PCRE_PCRE_LIBRARY})
  set(HAVE_LIBPCRECPP 1)
ELSE()
  find_package_handle_standard_args(PCRE DEFAULT_MSG PCRE_INCLUDE_DIR PCRE_PCRE_LIBRARY  )
  set(PCRE_LIBRARIES ${PCRE_PCRE_LIBRARY} )
  mark_as_advanced(PCRE_INCLUDE_DIR PCRE_LIBRARIES PCRE_PCRE_LIBRARY)
ENDIF()
