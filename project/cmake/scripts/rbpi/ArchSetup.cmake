set(ARCH_DEFINES -DTARGET_POSIX -DTARGET_LINUX -D_LINUX -D_ARMEL -DTARGET_RASPBERRY_PI
                 -DHAS_OMXPLAYER -DHAVE_OMXLIB)
set(SYSTEM_DEFINES -D__STDC_CONSTANT_MACROS -D_FILE_DEFINED
                   -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64)
set(PLATFORM_DIR linux)

string(REGEX REPLACE "[ ]+" ";" SYSTEM_LDFLAGS "$ENV{LDFLAGS}")
set(CMAKE_SYSTEM_NAME Linux)

if(WITH_ARCH)
  set(ARCH ${WITH_ARCH})
else()
  if(CPU STREQUAL arm1176jzf-s)
    set(ARCH arm)
    set(NEON False)
    set(NEON_FLAGS "-mcpu=arm1176jzf-s -mtune=arm1176jzf-s -mfloat-abi=hard -mfpu=vfp")
  elseif(CPU MATCHES "cortex-a7" OR CPU MATCHES "cortex-a53")
    set(ARCH arm)
    set(NEON True)
    set(NEON_FLAGS "-fPIC -mcpu=cortex-a7 -mfloat-abi=hard -mfpu=neon-vfpv4 -mvectorize-with-neon-quad")
  else()
    message(SEND_ERROR "Unknown CPU: ${CPU}")
  endif()
endif()

# Make sure we strip binaries in Release build
if(CMAKE_BUILD_TYPE STREQUAL Release AND CMAKE_COMPILER_IS_GNUCXX)
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -s")
endif()

find_package(CXX11 REQUIRED)

set(MMAL_FOUND 1 CACHE INTERNAL "MMAL")
set(OMX_FOUND 1 CACHE INTERNAL "OMX")
set(OMXLIB_FOUND 1 CACHE INTERNAL "OMX")
