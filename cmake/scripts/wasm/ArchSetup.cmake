# WebAssembly (Emscripten) architecture setup
include(cmake/scripts/wasm/Linkers.txt)

set(CORE_MAIN_SOURCE ${CMAKE_SOURCE_DIR}/xbmc/platform/wasm/main.cpp)

list(APPEND ARCH_DEFINES -DTARGET_POSIX -DTARGET_WASM)
set(PLATFORM_DIR platform/wasm)
set(PLATFORMDEFS_DIR platform/posix)

set(CMAKE_SYSTEM_NAME Emscripten)

if(WITH_ARCH)
  set(ARCH ${WITH_ARCH})
else()
  set(ARCH wasm32-unknown-emscripten)
endif()

if(WITH_CPU)
  set(CPU ${WITH_CPU})
else()
  set(CPU wasm32)
endif()

set(NEON False)

# Cannot run unit tests on host
set(HOST_CAN_EXECUTE_TARGET FALSE)

# Prefer internal libs when cross-compiling to wasm (typical for depends builds)
set(USE_INTERNAL_LIBS ON)

list(APPEND AUDIO_BACKENDS_LIST "webaudio")

set(APP_BINARY_SUFFIX ".js")

# Threading + memory (COOP/COEP headers required in HTML for pthreads).
# PROXY_TO_PTHREAD: Kodi's init is synchronous/blocking; it must run on a
# pthread so the browser main thread stays responsive.
# OFFSCREEN_FRAMEBUFFER: required for WebGL to work from a pthread. Without it,
# emscripten_webgl_make_context_current silently fails on worker threads.
# The newRenderingFrameStarted crash (GL.currentContext null on main thread) is
# fixed by calling MAIN_THREAD_EM_ASM to mirror the context current state on the
# main thread right after emscripten_webgl_make_context_current (see
# WinSystemWasmGLESContext.cpp).
if(CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
  add_link_options(
    "SHELL:-sUSE_PTHREADS=1"
    "SHELL:-sPTHREAD_POOL_SIZE=8"
    "SHELL:-sINITIAL_MEMORY=2GB"
    "SHELL:-sPROXY_TO_PTHREAD"
    "SHELL:-sOFFSCREEN_FRAMEBUFFER"
    "SHELL:-sMIN_WEBGL_VERSION=2"
    "SHELL:-sMAX_WEBGL_VERSION=2"
    "SHELL:-sFULL_ES3=1"
    "SHELL:-sABORTING_MALLOC=0"
    "SHELL:-lidbfs.js"
  )

  # ---------------------------------------------------------------------------
  # Debug vs Release
  #
  # Debug mode (-DCMAKE_BUILD_TYPE=Debug):
  #   - No optimization, full DWARF symbols (usable in browser DevTools).
  #   - ASSERTIONS=2: extra Emscripten runtime checks + pointer validity tests.
  #   - SAFE_HEAP=1: instrument every heap load/store; catches null-pointer
  #     dereferences and bad casts that otherwise show as opaque JS TypeErrors.
  #   - STACK_OVERFLOW_CHECK=2: canary on every function entry/exit.
  #   - Exception catching enabled: C++ exceptions surface with a message
  #     rather than silently aborting.
  #
  # Release mode: assertions stay at level 1 (cheap; useful while the port is
  # still maturing) and the optimiser is left to Emscripten/CMake defaults.
  # ---------------------------------------------------------------------------
  if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options(-g -O0)
    add_link_options(
      "-g -gsource-map"
      "SHELL:-sASSERTIONS=2"
      "SHELL:-sSAFE_HEAP=1"
      "SHELL:-sSTACK_OVERFLOW_CHECK=2"
      "SHELL:-sDISABLE_EXCEPTION_CATCHING=0"
      "SHELL:--source-map-base=http://localhost:8080/"
    )
    message(STATUS "WASM: Debug build — SAFE_HEAP + DWARF symbols + source maps enabled")
  else()
    add_link_options("SHELL:-sASSERTIONS=1")
  endif()
endif()

# Preload Kodi data directories into the Emscripten virtual filesystem under /kodi.
# The data is copied into the build tree by cmake install rules; we reference it from there.
set(WASM_PRELOAD_ROOT "${CMAKE_BINARY_DIR}")
set(WASM_VFS_PREFIX "/kodi")
foreach(_dir addons media system userdata)
  add_link_options("SHELL:--preload-file ${WASM_PRELOAD_ROOT}/${_dir}@${WASM_VFS_PREFIX}/${_dir}")
endforeach()

set(ENABLE_UPNP OFF CACHE BOOL "UPnP not available in browser" FORCE)
set(ENABLE_TESTING OFF CACHE BOOL "Tests do not run in browser" FORCE)
set(ADDONS_CONFIGURE_AT_STARTUP OFF CACHE BOOL "Binary addons not dynamically loaded in WASM" FORCE)
set(ENABLE_OPTICAL OFF CACHE BOOL "No optical drives in browser" FORCE)
set(ENABLE_DVDCSS OFF CACHE BOOL "libdvdcss disabled for WASM" FORCE)
set(ENABLE_AIRTUNES OFF CACHE BOOL "AirTunes not available in browser" FORCE)
set(ENABLE_EVENTCLIENTS OFF CACHE BOOL "Event clients not used for WASM" FORCE)
set(ENABLE_DVDCSS OFF CACHE BOOL "DVD CSS not applicable for WASM" FORCE)
set(ENABLE_INTERNAL_FFMPEG ON CACHE BOOL "No system FFmpeg for Emscripten" FORCE)

