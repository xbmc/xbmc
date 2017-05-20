# sndio check, based on FindAlsa.cmake
#

# Copyright (c) 2006, David Faure, <faure@kde.org>
# Copyright (c) 2007, Matthias Kretz <kretz@kde.org>
# Copyright (c) 2009, Jacob Meuser <jakemsr@sdf.lonestar.org>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

include(CheckIncludeFiles)
include(CheckIncludeFileCXX)
include(CheckLibraryExists)

# Already done by toplevel
find_library(SNDIO_LIBRARY sndio)
set(SNDIO_LIBRARY_DIR "")
if(SNDIO_LIBRARY)
   get_filename_component(SNDIO_LIBRARY_DIR ${SNDIO_LIBRARY} PATH)
endif(SNDIO_LIBRARY)

check_library_exists(sndio sio_open "${SNDIO_LIBRARY_DIR}" HAVE_SNDIO)
if(HAVE_SNDIO)
  message(STATUS "Found sndio: ${SNDIO_LIBRARY}")
  set(SNDIO_DEFINITIONS -DHAVE_SNDIO=1)
  set(SNDIO_LIBRARIES ${SNDIO_LIBRARY})
else(HAVE_SNDIO)
    message(STATUS "sndio not found")
endif(HAVE_SNDIO)
set(SNDIO_FOUND ${HAVE_SNDIO})

find_path(SNDIO_INCLUDES sndio.h)

mark_as_advanced(SNDIO_INCLUDES SNDIO_LIBRARY)
