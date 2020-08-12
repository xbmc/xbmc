option(DEVKIT_TEST "To test dev-kit" TRUE)
option(DEVKIT_TEST_CPP "To test dev-kit headers correct in \"C++\" compile" FALSE)
option(DEVKIT_TEST_STOP_ON_ERROR "To stop build if during \"C\" ABI test an error was found" FALSE)

if(NOT DEVKIT_TEST)
  return()
endif()

include(CheckCSourceCompiles)
include(CheckCXXSourceCompiles)

list(APPEND CMAKE_REQUIRED_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/include/kodi
                                    ${CMAKE_CURRENT_SOURCE_DIR}/../..)

message(STATUS "################################################################################")
message(STATUS "# Run test about dev-kit headers for correct style")
message(STATUS "#")

# Set minimal required defines
if(CORE_SYSTEM_NAME STREQUAL android)
  set(DEVKIT_TEST_DEFINES "\
#define HAS_GLES 3
#define TARGET_POSIX
#define TARGET_LINUX
#define TARGET_ANDROID
")
elseif(CORE_SYSTEM_NAME STREQUAL darwin_embedded)
  set(DEVKIT_TEST_DEFINES "\
#define HAS_GLES 2
#define TARGET_POSIX
#define TARGET_DARWIN
#define TARGET_DARWIN_EMBEDDED
")
elseif(CORE_SYSTEM_NAME STREQUAL osx)
  set(DEVKIT_TEST_DEFINES "\
#define HAS_GL 1
#define TARGET_DARWIN
#define TARGET_DARWIN_OSX
")
elseif(CORE_SYSTEM_NAME STREQUAL windows)
  set(DEVKIT_TEST_DEFINES "\
#define HAS_DX 1
#define WIN32
#define TARGET_WINDOWS
#define TARGET_WINDOWS_DESKTOP
")
elseif(CORE_SYSTEM_NAME STREQUAL linux)
  set(DEVKIT_TEST_DEFINES "\
#define HAS_GL 1
#define TARGET_POSIX
#define TARGET_LINUX
")
else()
  set(DEVKIT_TEST_DEFINES "\
#define HAS_GL 1
#define TARGET_POSIX
#define TARGET_FREEBSD
")
endif()

# Read list of all available headers
file(GLOB_RECURSE DEVKIT_HEADERS
  include/kodi/*.h
)

##
# Check the "C" headers itself with compile in "C"
#

message(STATUS "")
message(STATUS "================================================================================")
message(STATUS "Run \"C\" headers:")

set(DEVKIT_HEADER_NO 0)
foreach(DEVKIT_HEADER ${DEVKIT_HEADERS})
  if(NOT DEVKIT_HEADER MATCHES "c-api")
    continue()
  endif()

  string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}/include/" "" DEVKIT_HEADER_NAME ${DEVKIT_HEADER})

  message(STATUS "--------------------------------------------------------------------------------")
  message(STATUS "Checking \"C\" ABI about \"#include <${DEVKIT_HEADER_NAME}>\":")

  math(EXPR DEVKIT_HEADER_NO "${DEVKIT_HEADER_NO}+1")
  set(DEVKIT_TEST_NAME "C_TEST_BUILD_A_${DEVKIT_HEADER_NO}")

  check_c_source_compiles("\
${DEVKIT_TEST_DEFINES}
#include \"${DEVKIT_HEADER}\"
int main()
{
  return 0;
}" ${DEVKIT_TEST_NAME})

  if (NOT ${DEVKIT_TEST_NAME} EQUAL 1)
    if(${DEVKIT_TEST_STOP_ON_ERROR})
      message(FATAL_ERROR " - Header not match needed \"C\" ABI: \"#include <${DEVKIT_HEADER_NAME}>\" (see CMakeError.log)")
    else()
      message(STATUS " - WARNING: Header not match needed \"C\" ABI: \"#include <${DEVKIT_HEADER_NAME}>\" (see CMakeError.log)")
    endif()
  endif()
endforeach()

##
# Check the "C++" headers itself with compile in "C"
#
message(STATUS "")
message(STATUS "================================================================================")
message(STATUS "Run \"C++\" headers (to confirm there a safe within \"C\"):")

set(DEVKIT_HEADER_NO 0)
foreach(DEVKIT_HEADER ${DEVKIT_HEADERS})
  if(DEVKIT_HEADER MATCHES "c-api")
    continue()
  endif()

  string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}/include/" "" DEVKIT_HEADER_NAME ${DEVKIT_HEADER})

  message(STATUS "--------------------------------------------------------------------------------")
  message(STATUS "Checking \"C\" ABI about \"#include <${DEVKIT_HEADER_NAME}>\":")

  math(EXPR DEVKIT_HEADER_NO "${DEVKIT_HEADER_NO}+1")
  set(DEVKIT_TEST_NAME "C_TEST_BUILD_B_${DEVKIT_HEADER_NO}")

  check_c_source_compiles("\
${DEVKIT_TEST_DEFINES}
#include \"${DEVKIT_HEADER}\"
int main()
{
  return 0;
}" ${DEVKIT_TEST_NAME})

  if (NOT ${DEVKIT_TEST_NAME} EQUAL 1)
    if(${DEVKIT_TEST_STOP_ON_ERROR})
      message(FATAL_ERROR " - \"C++\" header not safe about \"C\" ABI: \"#include <${DEVKIT_HEADER_NAME}>\" (see CMakeError.log)")
    else()
      message(STATUS " - WARNING: \"C++\" header not safe about \"C\" ABI: \"#include <${DEVKIT_HEADER_NAME}>\" (see CMakeError.log)")
    endif()
  endif()
endforeach()

##
# Check the "C++" headers itself with compile in "C++"
#
if(DEVKIT_TEST_CPP)
  message(STATUS "")
  message(STATUS "================================================================================")
  message(STATUS "Run \"C++\" headers (to confirm there a correct within \"C++\"):")

  set(DEVKIT_HEADER_NO 0)
  foreach(DEVKIT_HEADER ${DEVKIT_HEADERS})
    if(DEVKIT_HEADER MATCHES "c-api")
      continue()
    endif()

    string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}/include/" "" DEVKIT_HEADER_NAME ${DEVKIT_HEADER})

    message(STATUS "--------------------------------------------------------------------------------")
    message(STATUS "Checking \"C++\" about \"#include <${DEVKIT_HEADER_NAME}>\":")

    math(EXPR DEVKIT_HEADER_NO "${DEVKIT_HEADER_NO}+1")
    set(DEVKIT_TEST_NAME "CPP_TEST_BUILD_${DEVKIT_HEADER_NO}")

    check_cxx_source_compiles("\
${DEVKIT_TEST_DEFINES}
#include \"${DEVKIT_HEADER}\"
int main()
{
  return 0;
}" ${DEVKIT_TEST_NAME})

    if (NOT ${DEVKIT_TEST_NAME} EQUAL 1)
      if(${DEVKIT_TEST_STOP_ON_ERROR})
        message(FATAL_ERROR " - \"C++\" header failed to build: \"#include <${DEVKIT_HEADER_NAME}>\" (see CMakeError.log)")
      else()
        message(STATUS " - \"C++\" header failed to build: \"#include <${DEVKIT_HEADER_NAME}>\" (see CMakeError.log)")
      endif()
    endif()
  endforeach()
endif()
