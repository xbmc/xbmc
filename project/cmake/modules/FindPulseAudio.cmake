# Try to find the PulseAudio library
#
# Once done this will define:
#
#  PULSEAUDIO_FOUND - system has the PulseAudio library
#  PULSEAUDIO_INCLUDE_DIR - the PulseAudio include directory
#  PULSEAUDIO_LIBRARY - the libraries needed to use PulseAudio
#  PULSEAUDIO_MAINLOOP_LIBRARY - the libraries needed to use PulsAudio Mailoop
#
# Copyright (c) 2008, Matthias Kretz, <kretz@kde.org>
# Copyright (c) 2009, Marcus Hufgard, <Marcus.Hufgard@hufgard.de>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

if(NOT PULSEAUDIO_MINIMUM_VERSION)
  set(PULSEAUDIO_MINIMUM_VERSION "1.0.0")
endif()

if(PULSEAUDIO_INCLUDE_DIRS AND PULSEAUDIO_LIBRARY AND PULSEAUDIO_MAINLOOP_LIBRARY)
   # Already in cache, be silent
   set(PULSEAUDIO_FIND_QUIETLY TRUE)
endif()

if(NOT WIN32)
   include(FindPkgConfig)
   pkg_check_modules(PC_PULSEAUDIO libpulse>=${PULSEAUDIO_MINIMUM_VERSION})
   pkg_check_modules(PC_PULSEAUDIO_MAINLOOP libpulse-mainloop-glib)
endif()

find_path(PULSEAUDIO_INCLUDE_DIRS pulse/pulseaudio.h
   HINTS
   ${PC_PULSEAUDIO_INCLUDEDIR}
   ${PC_PULSEAUDIO_INCLUDE_DIRS}
   )

find_library(PULSEAUDIO_LIBRARY NAMES pulse libpulse
   HINTS
   ${PC_PULSEAUDIO_LIBDIR}
   ${PC_PULSEAUDIO_LIBRARY_DIRS}
   )

find_library(PULSEAUDIO_MAINLOOP_LIBRARY NAMES pulse-mainloop pulse-mainloop-glib libpulse-mainloop-glib
   HINTS
   ${PC_PULSEAUDIO_LIBDIR}
   ${PC_PULSEAUDIO_LIBRARY_DIRS}
   )

if(NOT PULSEAUDIO_INCLUDE_DIRS OR NOT PULSEAUDIO_LIBRARY)
  set(PULSEAUDIO_FOUND FALSE)
else()
  set(PULSEAUDIO_FOUND TRUE)
endif()

if(PULSEAUDIO_FOUND)
   if(NOT PULSEAUDIO_FIND_QUIETLY)
      message(STATUS "Found PulseAudio: ${PULSEAUDIO_LIBRARY}")
      if(PULSEAUDIO_MAINLOOP_LIBRARY)
          message(STATUS "Found PulseAudio Mainloop: ${PULSEAUDIO_MAINLOOP_LIBRARY}")
      else()
          message(STATUS "Could NOT find PulseAudio Mainloop Library")
      endif()
   endif()
else()
   message(STATUS "Could NOT find PulseAudio")
endif()

set(PULSEAUDIO_LIBRARIES ${PULSEAUDIO_LIBRARY})

list(APPEND PULSEAUDIO_DEFINITIONS -DHAVE_LIBPULSE=1)

mark_as_advanced(PULSEAUDIO_INCLUDE_DIRS PULSEAUDIO_LIBRARIES PULSEAUDIO_LIBRARY PULSEAUDIO_MAINLOOP_LIBRARY)
