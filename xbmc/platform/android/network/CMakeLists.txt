set(SOURCES NetworkAndroid.cpp
            ZeroconfAndroid.cpp
            ZeroconfBrowserAndroid.cpp
      )

set(HEADERS NetworkAndroid.h
            ZeroconfAndroid.h
            ZeroconfBrowserAndroid.h
      )

core_add_library(platform_android_network)
