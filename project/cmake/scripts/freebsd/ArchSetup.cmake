set(ARCH_DEFINES -D_LINUX -DTARGET_POSIX -DTARGET_FREEBSD)
set(SYSTEM_DEFINES -D__STDC_CONSTANT_MACROS -D_FILE_DEFINED
                   -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64)
set(PLATFORM_DIR linux)
set(SYSTEM_LDFLAGS -L/usr/local/lib)
if(WITH_ARCH)
  set(ARCH ${WITH_ARCH})
else()
  if(CMAKE_SYSTEM_PROCESSOR STREQUAL amd64)
    set(ARCH x86_64-freebsd)
  elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "i.86")
    set(ARCH x86-freebsd)
  else()
    message(WARNING "unknown CPU: ${CPU}")
  endif()
endif()

if(CMAKE_BUILD_TYPE STREQUAL Release AND CMAKE_COMPILER_IS_GNUCXX)
  # Make sure we strip binaries in Release build
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -s")

  # moo. -O3 is not safe
  # https://github.com/Kitware/CMake/blob/master/Modules/Compiler/GNU.cmake#L37
  string(REPLACE "-O3" "-O2" CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}")
  string(REPLACE "-O3" "-O2" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
  string(REPLACE "-O3" "-O2" CMAKE_ASM_FLAGS_RELEASE "${CMAKE_ASM_FLAGS_RELEASE}")
endif()

find_package(CXX11 REQUIRED)
