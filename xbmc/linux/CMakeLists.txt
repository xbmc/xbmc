set(SOURCES ConvUtils.cpp
            LinuxResourceCounter.cpp
            LinuxTimezone.cpp
            PosixMountProvider.cpp
            TimeUtils.cpp
            XFileUtils.cpp
            XHandle.cpp
            XMemUtils.cpp
            XTimeUtils.cpp)

set(HEADERS ConvUtils.h
            LinuxResourceCounter.h
            LinuxTimezone.h
            PlatformConstants.h
            PlatformDefs.h
            PlatformInclude.h
            PosixMountProvider.h
            stat_utf8.h
            stdio_utf8.h
            TimeUtils.h
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

if(CORE_PLATFORM_NAME_LC STREQUAL rbpi)
  list(APPEND SOURCES RBP.cpp
                      OMXClock.cpp
                      OMXCore.cpp)
  list(APPEND HEADERS RBP.h
                      DllBCM.h
                      DllOMX.h
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
