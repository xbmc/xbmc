set(SOURCES NetworkLinux.cpp
            ZeroconfAvahi.cpp
            ZeroconfBrowserAvahi.cpp)

set(HEADERS NetworkLinux.h
            ZeroconfAvahi.h
            ZeroconfBrowserAvahi.h)

core_add_library(network_linux)
