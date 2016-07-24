set(SOURCES MessagePrinter.cpp
            WinMain.cpp
            crts_caller.cpp
            dxerr.cpp
            pch.cpp
            stat_utf8.cpp
            stdio_utf8.cpp
            strverscmp.cpp
            WIN32Util.cpp
            WindowHelper.cpp)

set(HEADERS crts_caller.h
            dirent.h
            dxerr.h
            IMMNotificationClient.h
            my_ntddcdrm.h
            my_ntddscsi.h
            netdb.h
            pch.h
            PlatformDefs.h
            PlatformInclude.h
            resource.h
            unistd.h
            WIN32Util.h
            WindowHelper.h)

core_add_library(platform_win32)
