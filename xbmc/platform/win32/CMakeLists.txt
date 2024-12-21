set(SOURCES CharsetConverter.cpp
            CPUInfoWin32.cpp
            Environment.cpp
            GPUInfoWin32.cpp
            MemUtils.cpp
            MessagePrinter.cpp
            dxerr.cpp
            Filesystem.cpp
            pch.cpp
            PlatformWin32.cpp
            WIN32Util.cpp
            WindowHelper.cpp
            XTimeUtils.cpp)

set(HEADERS CharsetConverter.h
            CPUInfoWin32.h
            dirent.h
            dxerr.h
            GPUInfoWin32.h
            IMMNotificationClient.h
            my_ntddcdrm.h
            my_ntddscsi.h
            netdb.h
            pch.h
            PlatformDefs.h
            PlatformWin32.h
            resource.h
            unistd.h
            WIN32Util.h
            WindowHelper.h)

core_add_library(platform_win32)
