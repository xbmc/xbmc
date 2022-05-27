set(SOURCES BlurayStateSerializer.cpp
            DVDFactoryInputStream.cpp
            DVDInputStream.cpp
            DVDInputStreamFFmpeg.cpp
            DVDInputStreamFile.cpp
            DVDInputStreamMemory.cpp
            DVDInputStreamNavigator.cpp
            DVDInputStreamStack.cpp
            DVDStateSerializer.cpp
            InputStreamAddon.cpp
            InputStreamMultiSource.cpp
            InputStreamPVRBase.cpp
            InputStreamPVRChannel.cpp
            InputStreamPVRRecording.cpp)

set(HEADERS BlurayStateSerializer.h
            DVDFactoryInputStream.h
            DVDInputStream.h
            DVDInputStreamFFmpeg.h
            DVDInputStreamFile.h
            DVDInputStreamMemory.h
            DVDInputStreamNavigator.h
            DVDInputStreamStack.h
            DVDStateSerializer.h
            DllDvdNav.h
            InputStreamAddon.h
            InputStreamMultiStreams.h
            InputStreamMultiSource.h
            InputStreamPVRBase.h
            InputStreamPVRChannel.h
            InputStreamPVRRecording.h)

if(BLURAY_FOUND)
  list(APPEND SOURCES DVDInputStreamBluray.cpp)
  list(APPEND HEADERS DVDInputStreamBluray.h)
endif()

core_add_library(dvdinputstreams)
