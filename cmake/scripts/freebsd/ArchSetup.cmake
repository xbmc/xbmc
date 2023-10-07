# Main cpp
set(CORE_MAIN_SOURCE ${CMAKE_SOURCE_DIR}/xbmc/platform/posix/main.cpp)

set(ARCH_DEFINES -DTARGET_POSIX -DTARGET_FREEBSD)
set(SYSTEM_DEFINES -D__STDC_CONSTANT_MACROS -D_LARGEFILE64_SOURCE
                   -D_FILE_OFFSET_BITS=64 -DHAS_OSS)
set(PLATFORM_DIR platform/linux)
set(PLATFORMDEFS_DIR platform/posix)
set(SYSTEM_LDFLAGS -L/usr/local/lib)
if(WITH_ARCH)
  set(ARCH ${WITH_ARCH})
else()
  if(CMAKE_SYSTEM_PROCESSOR STREQUAL amd64)
    set(ARCH x86_64-freebsd)
  elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "i.86")
    set(ARCH x86-freebsd)
  elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL armv6)
    set(ARCH armv6-freebsd)
    set(NEON True)
  elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL armv7)
    set(ARCH armv7-freebsd)
    set(NEON True)
  elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL aarch64)
    set(ARCH aarch64-freebsd)
    set(NEON True)
  else()
    message(WARNING "unknown CPU: ${CPU}")
  endif()
endif()

# Disable ALSA by default
if(NOT ENABLE_ALSA)
  option(ENABLE_ALSA "Enable alsa support?" OFF)
endif()

# Additional SYSTEM_DEFINES
list(APPEND SYSTEM_DEFINES -DHAS_POSIX_NETWORK -DHAS_FREEBSD_NETWORK)

# Build internal libs
if(NOT USE_INTERNAL_LIBS)
  if(KODI_DEPENDSBUILD)
    set(USE_INTERNAL_LIBS ON)
  else()
    set(USE_INTERNAL_LIBS OFF)
  endif()
endif()

list(APPEND AUDIO_BACKENDS_LIST "oss")
