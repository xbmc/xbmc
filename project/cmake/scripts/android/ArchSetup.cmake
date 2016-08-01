if(NOT CMAKE_TOOLCHAIN_FILE)
  message(FATAL_ERROR "CMAKE_TOOLCHAIN_FILE required for android. See ${PROJECT_SOURCE_DIR}/README.md")
elseif(NOT SDK_PLATFORM)
  message(FATAL_ERROR "Toolchain did not define SDK_PLATFORM. Possibly outdated depends.")
endif()

set(ARCH_DEFINES -DTARGET_POSIX -DTARGET_LINUX -D_LINUX -DTARGET_ANDROID)
set(SYSTEM_DEFINES -D__STDC_CONSTANT_MACROS -D_FILE_DEFINED
                   -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64)
set(PLATFORM_DIR linux)
if(WITH_ARCH)
  set(ARCH ${WITH_ARCH})
else()
  if(CPU STREQUAL armeabi-v7a)
    set(ARCH arm)
  elseif(CPU STREQUAL i686)
    set(ARCH i486-linux)
  else()
    message(SEND_ERROR "Unknown CPU: ${CPU}")
  endif()
endif()

set(FFMPEG_OPTS --enable-cross-compile --cpu=cortex-a9 --arch=arm --target-os=linux --enable-neon
                --disable-vdpau --cc=${CMAKE_C_COMPILER} --host-cc=${CMAKE_C_COMPILER}
                --strip=${CMAKE_STRIP})
set(ENABLE_SDL OFF)
set(ENABLE_X11 OFF)
set(ENABLE_EGL ON)
set(ENABLE_AML ON)
set(ENABLE_OPTICAL OFF)

list(APPEND DEPLIBS android log jnigraphics)
