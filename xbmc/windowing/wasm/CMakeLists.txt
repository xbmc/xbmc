set(SOURCES WinEventsWasm.cpp
            WasmVsync.cpp)

set(HEADERS WinEventsWasm.h
            WasmVsync.h
            WebGLCommit.h)

if(TARGET ${APP_NAME_LC}::OpenGLES)
  list(APPEND SOURCES WinSystemWasmGLESContext.cpp)
  list(APPEND HEADERS WinSystemWasmGLESContext.h)
endif()

core_add_library(windowing_wasm)
