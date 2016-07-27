set(SOURCES DVDFactoryInputStream.cpp
            DVDInputStream.cpp
            DVDInputStreamFFmpeg.cpp
            DVDInputStreamFile.cpp
            DVDInputStreamMemory.cpp
            DVDInputStreamNavigator.cpp
            DVDInputStreamPVRManager.cpp
            DVDInputStreamStack.cpp
            DVDStateSerializer.cpp
            InputStreamAddon.cpp
            InputStreamMultiSource.cpp)

set(HEADERS DVDFactoryInputStream.h
            DVDInputStream.h
            DVDInputStreamFFmpeg.h
            DVDInputStreamFile.h
            DVDInputStreamMemory.h
            DVDInputStreamNavigator.h
            DVDInputStreamPVRManager.h
            DVDInputStreamStack.h
            DVDStateSerializer.h
            DllDvdNav.h
            InputStreamAddon.h
            InputStreamMultiStreams.h
            InputStreamMultiSource.h)

if(BLURAY_FOUND)
  list(APPEND SOURCES DVDInputStreamBluray.cpp)
  list(APPEND HEADERS DVDInputStreamBluray.h)
endif()

core_add_library(dvdinputstreams)
