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
    set(NEON True)
  elseif(CPU STREQUAL arm64-v8a)
    set(ARCH aarch64)
  elseif(CPU STREQUAL i686)
    set(ARCH i486-linux)
    set(NEON False)
  else()
    message(SEND_ERROR "Unknown CPU: ${CPU}")
  endif()
endif()

if(CMAKE_BUILD_TYPE STREQUAL Release AND CMAKE_COMPILER_IS_GNUCXX)
  # Make sure we strip binaries in Release build
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -s")

  # moo. -O3 is not safe
  # https://github.com/Kitware/CMake/blob/master/Modules/Compiler/GNU.cmake#L37
  string(REPLACE "-O3" "-Os" CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}")
  string(REPLACE "-O3" "-Os" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
  string(REPLACE "-O3" "-Os" CMAKE_ASM_FLAGS_RELEASE "${CMAKE_ASM_FLAGS_RELEASE}")
endif()

set(FFMPEG_OPTS --enable-cross-compile --cpu=cortex-a9 --arch=arm --target-os=linux --enable-neon
                --disable-vdpau --cc=${CMAKE_C_COMPILER} --host-cc=${CMAKE_C_COMPILER}
                --strip=${CMAKE_STRIP})
set(ENABLE_SDL OFF CACHE BOOL "" FORCE)
set(ENABLE_X11 OFF CACHE BOOL "" FORCE)
set(ENABLE_AML OFF CACHE BOOL "" FORCE)
set(ENABLE_OPTICAL OFF CACHE BOOL "" FORCE)

list(APPEND DEPLIBS android log jnigraphics)
