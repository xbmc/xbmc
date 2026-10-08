set(SOURCES OSScreenSaverWasm.cpp
            VideoSyncWasm.cpp
            WinEventsWasm.cpp
            WasmClipboard.cpp
            WasmKeyboard.cpp
            WasmVsync.cpp)

set(HEADERS OSScreenSaverWasm.h
            VideoSyncWasm.h
            WinEventsWasm.h
            WasmClipboard.h
            WasmKeyboard.h
            WasmVsync.h
            WebGLCommit.h)

if(TARGET ${APP_NAME_LC}::OpenGLES)
  list(APPEND SOURCES WinSystemWasmGLESContext.cpp)
  list(APPEND HEADERS WinSystemWasmGLESContext.h)
endif()

core_add_library(windowing_wasm)
