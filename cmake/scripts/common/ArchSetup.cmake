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

include(CheckCXXSourceCompiles)
include(CheckSymbolExists)
include(CheckFunctionExists)
include(CheckIncludeFile)

# Macro to check if a given type exists in a given header
# Arguments:
#   header the header to check
#   type   the type to check for existence
#   var    the compiler definition to set if type exists
# On return:
#   If type was found, the definition is added to SYSTEM_DEFINES
macro(check_type header type var)
  check_cxx_source_compiles("#include <${header}>
                             int main()
                             {
                               ${type} s;
                             }" ${var})
  if(${var})
    list(APPEND SYSTEM_DEFINES -D${var}=1)
  endif()
endmacro()

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
if(NOT CORE_SYSTEM_NAME)
  string(TOLOWER ${CMAKE_SYSTEM_NAME} CORE_SYSTEM_NAME)
endif()

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
  set(CORE_HOST_IS_TARGET TRUE)
else()
  set(CORE_HOST_IS_TARGET FALSE)
endif()

# Remove any optimization flags that might come from the toolchain file
string(REPLACE "-g" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
string(REGEX REPLACE "-O[^ ]*" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0")

# Main cpp
set(CORE_MAIN_SOURCE ${CMAKE_SOURCE_DIR}/xbmc/platform/posix/main.cpp)

# system specific arch setup
if(NOT EXISTS ${CMAKE_SOURCE_DIR}/cmake/scripts/${CORE_SYSTEM_NAME}/ArchSetup.cmake)
  message(FATAL_ERROR "Couldn't find configuration for '${CORE_SYSTEM_NAME}' "
                      "Either the platform is not (yet) supported "
                      "or a toolchain file has to be specified. "
                      "Consult ${CMAKE_SOURCE_DIR}/cmake/README.md for instructions. "
                      "Note: Specifying a toolchain requires a clean build directory!")
endif()
include(${CMAKE_SOURCE_DIR}/cmake/scripts/${CORE_SYSTEM_NAME}/ArchSetup.cmake)

message(STATUS "Core system type: ${CORE_SYSTEM_NAME}")
message(STATUS "Platform: ${PLATFORM}")
message(STATUS "CPU: ${CPU}, ARCH: ${ARCH}")
message(STATUS "Cross-Compiling: ${CMAKE_CROSSCOMPILING}")
message(STATUS "Execute build artefacts on host: ${CORE_HOST_IS_TARGET}")
message(STATUS "Depends based build: ${KODI_DEPENDSBUILD}")

check_type(string std::u16string HAVE_STD__U16_STRING)
check_type(string std::u32string HAVE_STD__U32_STRING)
check_type(string char16_t HAVE_CHAR16_T)
check_type(string char32_t HAVE_CHAR32_T)
check_type(stdint.h uint_least16_t HAVE_STDINT_H)
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
if(HAVE_INTTYPES_H)
  list(APPEND SYSTEM_DEFINES -DHAVE_INTTYPES_H=1)
endif()

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
    add_options(ALL_LANGUAGES ALL_BUILDS ${NEON_FLAGS})
  endif()
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  add_options (ALL_LANGUAGES DEBUG "-g" "-D_DEBUG" "-Wall")
endif()

