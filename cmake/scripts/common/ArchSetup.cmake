# This script configures the build for a given architecture.
# Flags and stringified arch is set up.
# General compiler tests belongs here.
#
# On return, the following variables are set:
# CMAKE_SYSTEM_NAME - a lowercased system name
# CPU - the CPU on the target
# ARCH - the system architecture
# ARCH_DEFINES - list of compiler definitions for this architecture
# SYSTEM_DEFINES - list of compiler definitions for this system
# DEP_DEFINES - compiler definitions for system dependencies (e.g. LIRC)
# + the results of compiler tests etc.

# workaround a bug in older cmake, where binutils wouldn't be set after deleting CMakeCache.txt
include(CMakeFindBinUtils)

include(CheckCXXSourceCompiles)
include(CheckSymbolExists)
include(CheckFunctionExists)
include(CheckIncludeFile)
include(CheckTypeSize)

# Macro to check if a given builtin function exists
# Arguments:
#   func   the function to check
#   var    the compiler definition to set if type exists
# On return:
#   If type was found, the definition is added to SYSTEM_DEFINES
macro(check_builtin func var)
  check_cxx_source_compiles("
                             int main()
                             {
                               ${func};
                             }" ${var})
  if(${var})
    list(APPEND SYSTEM_DEFINES -D${var}=1)
  endif()
endmacro()


# -------- Main script ---------
message(STATUS "System type: ${CMAKE_SYSTEM_NAME}")

if(WITH_CPU)
  set(CPU ${WITH_CPU})
elseif(NOT KODI_DEPENDSBUILD)
  set(CPU ${CMAKE_SYSTEM_PROCESSOR})
endif()

if(CMAKE_TOOLCHAIN_FILE)
  if(NOT EXISTS "${CMAKE_TOOLCHAIN_FILE}")
    message(FATAL_ERROR "Toolchain file ${CMAKE_TOOLCHAIN_FILE} does not exist.")
  elseif(KODI_DEPENDSBUILD AND (NOT DEPENDS_PATH OR NOT NATIVEPREFIX))
    message(FATAL_ERROR "Toolchain did not define DEPENDS_PATH or NATIVEPREFIX. Possibly outdated depends.")
  endif()
endif()

# While CMAKE_CROSSCOMPILING is set unconditionally if there's a toolchain file,
# this variable is set if we can execute build artefacts on the host system (for example unit tests).
if(CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL CMAKE_SYSTEM_PROCESSOR AND
   CMAKE_HOST_SYSTEM_NAME STREQUAL CMAKE_SYSTEM_NAME)
  if(NOT DEFINED HOST_CAN_EXECUTE_TARGET)
    set(HOST_CAN_EXECUTE_TARGET TRUE)
  endif()
else()
  if(NOT HOST_CAN_EXECUTE_TARGET)
    set(HOST_CAN_EXECUTE_TARGET FALSE)
  endif()
endif()

# system specific arch setup
if(NOT EXISTS ${CMAKE_SOURCE_DIR}/cmake/scripts/${CORE_SYSTEM_NAME}/ArchSetup.cmake)
  message(FATAL_ERROR "Couldn't find configuration for '${CORE_SYSTEM_NAME}' "
                      "Either the platform is not (yet) supported "
                      "or a toolchain file has to be specified. "
                      "Consult ${CMAKE_SOURCE_DIR}/cmake/README.md for instructions. "
                      "Note: Specifying a toolchain requires a clean build directory!")
endif()
include(${CMAKE_SOURCE_DIR}/cmake/scripts/${CORE_SYSTEM_NAME}/ArchSetup.cmake)

# No TARBALL_DIR given, or no arch specific default set
if(NOT TARBALL_DIR)
  set(TARBALL_DIR ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/download)
endif()

message(STATUS "Core system type: ${CORE_SYSTEM_NAME}")
message(STATUS "Platform: ${CORE_PLATFORM_NAME}")
message(STATUS "CPU: ${CPU}, ARCH: ${ARCH}")
message(STATUS "Cross-Compiling: ${CMAKE_CROSSCOMPILING}")
message(STATUS "Execute build artefacts on host: ${CORE_HOST_IS_TARGET}")
message(STATUS "Depends based build: ${KODI_DEPENDSBUILD}")

check_symbol_exists(posix_fadvise fcntl.h HAVE_POSIX_FADVISE)
check_symbol_exists(PRIdMAX inttypes.h HAVE_INTTYPES_H)
check_builtin("long* temp=0; long ret=__sync_add_and_fetch(temp, 1)" HAS_BUILTIN_SYNC_ADD_AND_FETCH)
check_builtin("long* temp=0; long ret=__sync_sub_and_fetch(temp, 1)" HAS_BUILTIN_SYNC_SUB_AND_FETCH)
check_builtin("long* temp=0; long ret=__sync_val_compare_and_swap(temp, 1, 1)" HAS_BUILTIN_SYNC_VAL_COMPARE_AND_SWAP)
check_include_file(sys/inotify.h HAVE_INOTIFY)
if(HAVE_INOTIFY)
  list(APPEND SYSTEM_DEFINES -DHAVE_INOTIFY=1)
endif()
if(HAVE_POSIX_FADVISE)
  list(APPEND SYSTEM_DEFINES -DHAVE_POSIX_FADVISE=1)
endif()
check_function_exists(localtime_r HAVE_LOCALTIME_R)
if(HAVE_LOCALTIME_R)
  list(APPEND SYSTEM_DEFINES -DHAVE_LOCALTIME_R=1)
endif()
check_function_exists(gmtime_r HAVE_GMTIME_R)
if(HAVE_GMTIME_R)
list(APPEND SYSTEM_DEFINES -DHAVE_GMTIME_R=1)
endif()
if(HAVE_INTTYPES_H)
  list(APPEND SYSTEM_DEFINES -DHAVE_INTTYPES_H=1)
endif()

set(CMAKE_REQUIRED_DEFINITIONS "-D_GNU_SOURCE")
check_symbol_exists("STATX_BTIME" "linux/stat.h" HAVE_STATX)
if(HAVE_STATX)
  check_function_exists("statx" FOUND_STATX_FUNCTION)
  if(FOUND_STATX_FUNCTION)
    message(STATUS "statx is available")
    list(APPEND ARCH_DEFINES "-DHAVE_STATX=1")
  else()
    message(STATUS "statx flags found but no linkable function : C library too old ?")
  endif()
else()
  message(STATUS "statx() not found")
endif()
set(CMAKE_REQUIRED_DEFINITIONS "")

find_package(SSE)
foreach(_sse SSE SSE2 SSE3 SSSE3 SSE4_1 SSE4_2 AVX AVX2)
  if(${${_sse}_FOUND})
    # enable SSE versions up to 4.1 by default, if available
    if(NOT ${_sse} MATCHES "AVX" AND NOT ${_sse} STREQUAL "SSE4_2")
      option(ENABLE_${_sse} "Enable ${_sse}" ON)
    else()
      option(ENABLE_${_sse} "Enable ${_sse}" OFF)
    endif()
  endif()
  if(ENABLE_${_sse})
    set(HAVE_${_sse} TRUE CACHE STRING "${_sse} enabled")
    list(APPEND ARCH_DEFINES -DHAVE_${_sse}=1)
  endif()
endforeach()

if(NOT DEFINED NEON OR NEON)
  option(ENABLE_NEON "Enable NEON optimization" ${NEON})
  if(ENABLE_NEON)
    message(STATUS "NEON optimization enabled")
    add_definitions(-DHAS_NEON)
    if(NEON_FLAGS)
      add_options(ALL_LANGUAGES ALL_BUILDS ${NEON_FLAGS})
    endif()
  endif()
endif()

if(NOT MSVC)
  # these options affect all code built by cmake including external projects.
  add_options(ALL_LANGUAGES ALL_BUILDS
    -Wall
    -Wdouble-promotion
    -Wmissing-field-initializers
    -Wsign-compare
    -Wextra
    -Wno-unused-parameter # from -Wextra
  )

  add_options(CXX ALL_BUILDS
    -Wnon-virtual-dtor
  )

  add_options(ALL_LANGUAGES DEBUG
    -g
    -D_DEBUG
  )

  # these options affect only core code
  if(NOT CORE_COMPILE_OPTIONS)
    set(CORE_COMPILE_OPTIONS
      -Werror=double-promotion
      -Werror=missing-field-initializers
      -Werror=sign-compare
    )
  endif()
endif()

# set for compile info to help detect binary addons
set(APP_SHARED_LIBRARY_SUFFIX "${CMAKE_SHARED_LIBRARY_SUFFIX}")
