set(SOURCES CPUInfoWasm.cpp
            GPUInfoWasm.cpp
            MemUtils.cpp
            PlatformWasm.cpp
            TitanWebApis.cpp
            TizenWebApis.cpp
            WasmAudioWorkletManager.cpp
            WasmWebApis.cpp)

set(HEADERS CPUInfoWasm.h
            GPUInfoWasm.h
            PlatformWasm.h
            TitanWebApis.h
            TizenWebApis.h
            WasmAudioWorkletManager.h
            WasmWebApis.h)

core_add_library(platform_wasm)
