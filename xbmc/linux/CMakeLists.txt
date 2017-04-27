set(SOURCES ConvUtils.cpp
            LinuxResourceCounter.cpp
            LinuxTimezone.cpp
            PosixMountProvider.cpp
            RBP.cpp
            XFileUtils.cpp
            XHandle.cpp
            XMemUtils.cpp
            XTimeUtils.cpp)

set(HEADERS ConvUtils.h
            DllBCM.h
            LinuxResourceCounter.h
            LinuxTimezone.h
            PlatformDefs.h
            PlatformInclude.h
            PosixMountProvider.h
            RBP.h
            stat_utf8.h
            stdio_utf8.h
            XFileUtils.h
            XHandle.h
            XHandlePublic.h
            XMemUtils.h
            XTimeUtils.h)

if(ALSA_FOUND)
  list(APPEND SOURCES FDEventMonitor.cpp)
  list(APPEND HEADERS FDEventMonitor.h)
endif()

if(DBUS_FOUND)
  list(APPEND SOURCES DBusMessage.cpp
                      DBusReserve.cpp
                      DBusUtil.cpp)
  list(APPEND HEADERS DBusMessage.h
                      DBusReserve.h
                      DBusUtil.h)
endif()

if(OMXLIB_FOUND)
  list(APPEND SOURCES OMXClock.cpp
                      OMXCore.cpp)
  list(APPEND HEADERS DllOMX.h
                      OMXClock.h
                      OMXCore.h)
endif()

if(HAVE_SSE4_1)
  add_subdirectory(sse4)
endif()

if(IMX_FOUND)
  add_subdirectory(imx)
endif()

core_add_library(linuxsupport)
