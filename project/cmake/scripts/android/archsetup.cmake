set(ARCH_DEFINES -DTARGET_POSIX -DTARGET_LINUX -D_LINUX -DTARGET_ANDROID)
set(SYSTEM_DEFINES -D__STDC_CONSTANT_MACROS -D_FILE_DEFINED
                   -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64)
set(PLATFORM_DIR linux)
if(WITH_ARCH)
  set(ARCH ${WITH_ARCH})
else()
  if(CPU STREQUAL "armeabi-v7a")
    set(ARCH arm-linux-androideabi)
  else()
    message(WARNING "unknown CPU: ${CPU}")
  endif()
endif()

set(FFMPEG_OPTS --enable-cross-compile --cpu=cortex-a9 --arch=arm --target-os=linux --enable-neon
                --disable-vdpau --cc=${CMAKE_C_COMPILER} --host-cc=${CMAKE_C_COMPILER}
                --strip=${CMAKE_STRIP})
set(ENABLE_SDL OFF)
set(ENABLE_X11 OFF)
set(ENABLE_EGL ON)
