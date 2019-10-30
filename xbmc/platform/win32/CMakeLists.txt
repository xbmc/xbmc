set(SOURCES CharsetConverter.cpp
            CPUInfoWin32.cpp
            Environment.cpp
            MemUtils.cpp
            MessagePrinter.cpp
            dxerr.cpp
            Filesystem.cpp
            pch.cpp
            WIN32Util.cpp
            WindowHelper.cpp)

set(HEADERS CharsetConverter.h
            CPUInfoWin32.h
            dirent.h
            dxerr.h
            IMMNotificationClient.h
            my_ntddcdrm.h
            my_ntddscsi.h
            netdb.h
            pch.h
            PlatformDefs.h
            resource.h
            unistd.h
            WIN32Util.h
            WindowHelper.h)

core_add_library(platform_win32)
