set(SOURCES input/RemoteControlXbox.cpp
            Environment.cpp
            Win10App.cpp
            MessagePrinter.cpp
            ../win32/CharsetConverter.cpp
            ../win32/dxerr.cpp
            ../win32/Filesystem.cpp
            ../win32/pch.cpp
            ../win32/WIN32Util.cpp)

set(HEADERS input/RemoteControlXbox.h
            AsyncHelpers.h
            Win10App.h
            ../win32/CharsetConverter.h
            ../win32/dirent.h
            ../win32/dxerr.h
            ../win32/my_ntddcdrm.h
            ../win32/my_ntddscsi.h
            ../win32/netdb.h
            ../win32/pch.h
            ../win32/PlatformDefs.h
            ../win32/resource.h
            ../win32/unistd.h
            ../win32/WIN32Util.h)

core_add_library(platform_win10)
