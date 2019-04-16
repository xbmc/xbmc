set(SOURCES ZeroconfBrowserDarwin.cpp
            ZeroconfDarwin.cpp)

set(HEADERS ioshacks.h
            ZeroconfBrowserDarwin.h
            ZeroconfDarwin.h)

core_add_library(platform_darwin_network)
