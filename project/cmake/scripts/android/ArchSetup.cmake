if(NOT CMAKE_TOOLCHAIN_FILE)
  message(FATAL_ERROR "CMAKE_TOOLCHAIN_FILE required for android. See ${PROJECT_SOURCE_DIR}/README.md")
elseif(NOT SDK_PLATFORM)
  message(FATAL_ERROR "Toolchain did not define SDK_PLATFORM. Possibly outdated depends.")
endif()

set(ARCH_DEFINES -DTARGET_POSIX -DTARGET_LINUX -D_LINUX -DTARGET_ANDROID)
set(SYSTEM_DEFINES -D__STDC_CONSTANT_MACROS -D_FILE_DEFINED
                   -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64)
set(PLATFORM_DIR linux)

if(NOT ARCH STREQUAL arm OR NOT ARCH STREQUAL aarch64 or NOT ARCH MATCHES "x86.*")
  message(SEND_ERROR "Unknown ARCH: ${ARCH}")
endif()

set(FFMPEG_OPTS --enable-cross-compile --arch=${ARCH} --target-os=linux --disable-vdpau 
                --cc=${CMAKE_C_COMPILER} --host-cc=${CMAKE_C_COMPILER} --strip=${CMAKE_STRIP})

if(NEON)
  set(FFMPEG_OPTS "${FFMPEG_OPTS} --enable-neon")
endif()

if(CPU)
  set(FFMPEG_OPTS "${FFMPEG_OPTS} --cpu=${CPU}")
endif()

set(ENABLE_SDL OFF CACHE BOOL "" FORCE)
set(ENABLE_X11 OFF CACHE BOOL "" FORCE)
set(ENABLE_AML OFF CACHE BOOL "" FORCE)
set(ENABLE_OPTICAL OFF CACHE BOOL "" FORCE)

list(APPEND DEPLIBS android log jnigraphics)
