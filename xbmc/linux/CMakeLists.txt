set(SOURCES ConvUtils.cpp
            DBusMessage.cpp
            DBusReserve.cpp
            DBusUtil.cpp
            FDEventMonitor.cpp
            LinuxResourceCounter.cpp
            LinuxTimezone.cpp
            PosixMountProvider.cpp
            RBP.cpp
            XFileUtils.cpp
            XHandle.cpp
            XMemUtils.cpp
            XTimeUtils.cpp)

set(HEADERS ConvUtils.h
            DBusMessage.h
            DBusReserve.h
            DBusUtil.h
            DllBCM.h
            FDEventMonitor.h
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

if(OMXLIB_FOUND)
  list(APPEND SOURCES OMXClock.cpp
                      OMXCore.cpp)
  list(APPEND HEADERS DllOMX.h
                      OMXClock.h
                      OMXCore.h)
endif()

if(IMX_FOUND)
  add_subdirectory(imx)
endif()

core_add_library(linuxsupport)
