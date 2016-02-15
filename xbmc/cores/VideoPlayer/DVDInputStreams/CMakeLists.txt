set(SOURCES DVDFactoryInputStream.cpp
            DVDInputStream.cpp
            DVDInputStreamFFmpeg.cpp
            DVDInputStreamFile.cpp
            DVDInputStreamMemory.cpp
            DVDInputStreamNavigator.cpp
            DVDInputStreamPVRManager.cpp
            DVDInputStreamRTMP.cpp
            DVDInputStreamStack.cpp
            DVDStateSerializer.cpp)

set(HEADERS DVDFactoryInputStream.h
            DVDInputStream.h
            DVDInputStreamFFmpeg.h
            DVDInputStreamFile.h
            DVDInputStreamMemory.h
            DVDInputStreamNavigator.h
            DVDInputStreamPVRManager.h
            DVDInputStreamRTMP.h
            DVDInputStreamStack.h
            DVDStateSerializer.h
            DllDvdNav.h
            DllLibRTMP.h)

if(BLURAY_FOUND)
  list(APPEND SOURCES DVDInputStreamBluray.cpp)
  list(APPEND HEADERS DVDInputStreamBluray.h)
endif()

core_add_library(dvdinputstreams)
add_dependencies(dvdinputstreams libcpluff)

if(ENABLE_INTERNAL_FFMPEG)
  add_dependencies(dvdinputstreams ffmpeg)
endif()

if(NOT CORE_SYSTEM_NAME STREQUAL windows)
  add_dependencies(dvdinputstreams dvdnav)
endif()
