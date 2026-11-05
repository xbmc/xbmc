#.rst:
# FindCCache
# ----------
# Finds ccache and sets it up as compiler wrapper.
# Visual Studio support requires this to be called after project().
#
# See: https://crascit.com/2016/04/09/using-ccache-with-cmake/

set(_ccache_explicit_program "${CCACHE_PROGRAM}")
find_program(CCACHE_PROGRAM ccache)

set(_ccache_required_vars CCACHE_PROGRAM)
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" AND CMAKE_GENERATOR MATCHES "Visual Studio")
  if(CCACHE_PROGRAM AND NOT _ccache_explicit_program)
    get_filename_component(_ccache_directory "${CCACHE_PROGRAM}" DIRECTORY)
    file(GLOB _ccache_chocolatey_programs
         "${_ccache_directory}/../lib/ccache/tools/ccache.exe"
         "${_ccache_directory}/../lib/ccache/tools/*/ccache.exe")
    if(_ccache_chocolatey_programs)
      list(SORT _ccache_chocolatey_programs COMPARE NATURAL ORDER DESCENDING)
      list(GET _ccache_chocolatey_programs 0 CCACHE_PROGRAM)
      set(CCACHE_PROGRAM "${CCACHE_PROGRAM}" CACHE FILEPATH "Path to ccache executable" FORCE)
    endif()
  endif()

  # Package-manager shims may work as ccache.exe but fail when relocated as cl.exe.
  set(CCACHE_MSVC_USABLE FALSE)
  if(CCACHE_PROGRAM)
    set(_ccache_probe "${CMAKE_BINARY_DIR}/CMakeFiles/ccache/cl.exe")
    file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/CMakeFiles/ccache")
    file(COPY_FILE "${CCACHE_PROGRAM}" "${_ccache_probe}" ONLY_IF_DIFFERENT
         RESULT _ccache_copy_result)
    if(_ccache_copy_result STREQUAL "0")
      execute_process(COMMAND "${CMAKE_COMMAND}" -E env CCACHE_DISABLE=1 CCACHE_NOSTATS=1
                              "CCACHE_COMPILER=${CMAKE_CXX_COMPILER}"
                              "${_ccache_probe}" /help
                      RESULT_VARIABLE _ccache_probe_result
                      OUTPUT_QUIET ERROR_QUIET TIMEOUT 10)
      if(_ccache_probe_result STREQUAL "0")
        set(CCACHE_MSVC_USABLE TRUE)
      endif()
    endif()
  endif()
  list(APPEND _ccache_required_vars CCACHE_MSVC_USABLE)
  set(_ccache_failure_reason
      "Visual Studio requires a real ccache executable that works when copied as cl.exe. Set CCACHE_PROGRAM to the real executable (for Chocolatey, under lib/ccache/tools), or use ENABLE_CCACHE=OFF.")
  if(CCACHE_PROGRAM AND NOT CCACHE_MSVC_USABLE AND NOT CCache_FIND_REQUIRED)
    message(WARNING "Disabling ccache: ${CCACHE_PROGRAM} cannot be used as cl.exe. ${_ccache_failure_reason}")
  endif()
endif()

if(CCACHE_PROGRAM AND (NOT DEFINED CCACHE_MSVC_USABLE OR CCACHE_MSVC_USABLE))
  execute_process(COMMAND "${CCACHE_PROGRAM}" --version
                  OUTPUT_VARIABLE CCACHE_VERSION
                  OUTPUT_STRIP_TRAILING_WHITESPACE)
  string(REGEX MATCH "[^\n]* version [^\n]*" CCACHE_VERSION "${CCACHE_VERSION}")
  string(REGEX REPLACE ".* version (.*)" "\\1" CCACHE_VERSION "${CCACHE_VERSION}")
endif()

include(FindPackageHandleStandardArgs)
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" AND CMAKE_GENERATOR MATCHES "Visual Studio")
  set(CCache_FIND_VERSION 4.13.3)
endif()
find_package_handle_standard_args(CCache REQUIRED_VARS ${_ccache_required_vars}
                                  VERSION_VAR CCACHE_VERSION
                                  REASON_FAILURE_MESSAGE "${_ccache_failure_reason}")

if(CCACHE_FOUND)
  # Supports Unix Makefiles, Ninja, Visual Studio and Xcode
  if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" AND CMAKE_GENERATOR MATCHES "Visual Studio")
    include(${CMAKE_CURRENT_LIST_DIR}/../../scripts/windows/CCache.cmake)
    set(CMAKE_CXX_SCAN_FOR_MODULES OFF PARENT_SCOPE)
    foreach(_variable IN LISTS _ccache_variables)
      set(${_variable} "${${_variable}}" PARENT_SCOPE)
    endforeach()
  elseif(XCODE)
    file(WRITE "${CMAKE_BINARY_DIR}/launch-c" "#!/bin/sh\nexec \"${CCACHE_PROGRAM}\" \"${CMAKE_C_COMPILER}\" \"$@\"\n")
    file(WRITE "${CMAKE_BINARY_DIR}/launch-cxx" "#!/bin/sh\nexec \"${CCACHE_PROGRAM}\" \"${CMAKE_CXX_COMPILER}\" \"$@\"\n")
    execute_process(COMMAND chmod +x "${CMAKE_BINARY_DIR}/launch-c" "${CMAKE_BINARY_DIR}/launch-cxx")

    set(CMAKE_XCODE_ATTRIBUTE_LD "${CMAKE_BINARY_DIR}/launch-c" PARENT_SCOPE)
    set(CMAKE_XCODE_ATTRIBUTE_LDPLUSPLUS "${CMAKE_BINARY_DIR}/launch-cxx" PARENT_SCOPE)

    if(XCODE_VERSION VERSION_GREATER_EQUAL 26)
      set(CMAKE_XCODE_ATTRIBUTE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}" PARENT_SCOPE)
      set(CMAKE_XCODE_ATTRIBUTE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}" PARENT_SCOPE)
    else()
      set(CMAKE_XCODE_ATTRIBUTE_CC "${CMAKE_BINARY_DIR}/launch-c" PARENT_SCOPE)
      set(CMAKE_XCODE_ATTRIBUTE_CXX "${CMAKE_BINARY_DIR}/launch-cxx" PARENT_SCOPE)
    endif()
  else()
    set(_launcher "${CCACHE_PROGRAM}")
    if(CORE_USE_PCH)
      # Without these ccache doesn't cache anything that uses a precompiled header
      execute_process(COMMAND "${CCACHE_PROGRAM}" --get-config sloppiness
                      OUTPUT_VARIABLE _sloppiness
                      OUTPUT_STRIP_TRAILING_WHITESPACE)
      if(_sloppiness)
        string(APPEND _sloppiness ",")
      endif()
      string(APPEND _sloppiness "pch_defines,time_macros")

      if(CCACHE_VERSION VERSION_GREATER_EQUAL 4.8)
        list(APPEND _launcher "sloppiness=${_sloppiness}")
      else()
        set(_launcher "${CMAKE_COMMAND}" -E env "CCACHE_SLOPPINESS=${_sloppiness}" "${CCACHE_PROGRAM}")
      endif()
    endif()
    set(CMAKE_CXX_COMPILER_LAUNCHER ${_launcher} PARENT_SCOPE)
    set(CMAKE_C_COMPILER_LAUNCHER ${_launcher} PARENT_SCOPE)
  endif()
endif()

mark_as_advanced(CCACHE_PROGRAM)
