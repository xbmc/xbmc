#.rst:
# FindCCache
# ----------
# Finds ccache and sets it up as compiler wrapper.
# This should ideally be called before the call to project().
#
# See: https://crascit.com/2016/04/09/using-ccache-with-cmake/

find_program(CCACHE_PROGRAM ccache)

if(CCACHE_PROGRAM)
  execute_process(COMMAND "${CCACHE_PROGRAM}" --version
                  OUTPUT_VARIABLE CCACHE_VERSION
                  OUTPUT_STRIP_TRAILING_WHITESPACE)
  string(REGEX MATCH "[^\n]* version [^\n]*" CCACHE_VERSION "${CCACHE_VERSION}")
  string(REGEX REPLACE ".* version (.*)" "\\1" CCACHE_VERSION "${CCACHE_VERSION}")
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(CCache REQUIRED_VARS CCACHE_PROGRAM
                                  VERSION_VAR CCACHE_VERSION)

if(CCACHE_FOUND)
  # Supports Unix Makefiles, Ninja and Xcode
  if(XCODE)
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
