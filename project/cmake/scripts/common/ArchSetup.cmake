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
elseif(NOT CMAKE_TOOLCHAIN_FILE)
  set(CPU ${CMAKE_SYSTEM_PROCESSOR})
endif()

if(CMAKE_TOOLCHAIN_FILE)
  if(NOT EXISTS "${CMAKE_TOOLCHAIN_FILE}")
    message(FATAL_ERROR "Toolchain file ${CMAKE_TOOLCHAIN_FILE} does not exist.")
  elseif(NOT DEPENDS_PATH OR NOT NATIVEPREFIX)
    message(FATAL_ERROR "Toolchain did not define DEPENDS_PATH or NATIVEPREFIX. Possibly outdated depends.")
  endif()
endif()

# Main cpp
set(CORE_MAIN_SOURCE ${CORE_SOURCE_DIR}/xbmc/platform/posix/main.cpp)

# system specific arch setup
if(NOT EXISTS ${PROJECT_SOURCE_DIR}/scripts/${CORE_SYSTEM_NAME}/ArchSetup.cmake)
  message(FATAL_ERROR "Couldn't find configuration for '${CORE_SYSTEM_NAME}' "
                      "Either the platform is not (yet) supported "
                      "or a toolchain file has to be specified. "
                      "Consult ${CMAKE_SOURCE_DIR}/README.md for instructions. "
                      "Note: Specifying a toolchain requires a clean build directory!")
endif()
include(${PROJECT_SOURCE_DIR}/scripts/${CORE_SYSTEM_NAME}/ArchSetup.cmake)

message(STATUS "Core system type: ${CORE_SYSTEM_NAME}")
message(STATUS "Platform: ${PLATFORM}")
message(STATUS "CPU: ${CPU}, ARCH: ${ARCH}")
message(STATUS "Cross-Compiling: ${CMAKE_CROSSCOMPILING}")

check_type(string std::u16string HAVE_STD__U16_STRING)
check_type(string std::u32string HAVE_STD__U32_STRING)
check_type(string char16_t HAVE_CHAR16_T)
check_type(string char32_t HAVE_CHAR32_T)
check_type(stdint.h uint_least16_t HAVE_STDINT_H)
check_symbol_exists(posix_fadvise fcntl.h HAVE_POSIX_FADVISE)
check_builtin("long* temp=0; long ret=__sync_add_and_fetch(temp, 1)" HAS_BUILTIN_SYNC_ADD_AND_FETCH)
check_builtin("long* temp=0; long ret=__sync_sub_and_fetch(temp, 1)" HAS_BUILTIN_SYNC_SUB_AND_FETCH)
check_builtin("long* temp=0; long ret=__sync_val_compare_and_swap(temp, 1, 1)" HAS_BUILTIN_SYNC_VAL_COMPARE_AND_SWAP)
if(HAVE_POSIX_FADVISE)
  list(APPEND SYSTEM_DEFINES -DHAVE_POSIX_FADVISE=1)
endif()
check_function_exists(localtime_r HAVE_LOCALTIME_R)
if(HAVE_LOCALTIME_R)
  list(APPEND SYSTEM_DEFINES -DHAVE_LOCALTIME_R=1)
endif()
