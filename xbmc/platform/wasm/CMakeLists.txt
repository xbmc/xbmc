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
            MseBridge.h
            PlatformWasm.h
            TitanWebApis.h
            TizenEmss.h
            TizenWebApis.h
            WasmAudioWorkletManager.h
            WasmMediaLayer.h
            WasmWebApis.h)

core_add_library(platform_wasm)
