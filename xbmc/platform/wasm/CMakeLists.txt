set(SOURCES CPUInfoWasm.cpp
            GPUInfoWasm.cpp
            MemUtils.cpp
            PlatformWasm.cpp
            WasmAudioWorkletManager.cpp)

set(HEADERS CPUInfoWasm.h
            GPUInfoWasm.h
            PlatformWasm.h
            TizenEmss.h
            WasmAudioWorkletManager.h
            WasmMediaLayer.h)

core_add_library(platform_wasm)
