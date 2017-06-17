set(SOURCES BinaryAddonManager.cpp
            AddonDll.cpp
            AddonInstanceHandler.cpp
            BinaryAddonBase.cpp
            BinaryAddonExtensions.cpp
            BinaryAddonType.cpp)

set(HEADERS BinaryAddonManager.h
            AddonDll.h
            AddonInstanceHandler.h
            BinaryAddonBase.h
            BinaryAddonExtensions.h
            BinaryAddonType.h
            DllAddon.h)

core_add_library(addons_binary-addons)
