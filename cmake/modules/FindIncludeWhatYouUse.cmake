#.rst:
# FindIncludeWhatYouUse
# ---------------------
# Finds include-what-you-use and sets it up to run along with the compiler for C and CXX.

find_program(IWYU_EXECUTABLE NAMES include-what-you-use iwyu)

if(IWYU_EXECUTABLE)
  execute_process(COMMAND "${IWYU_EXECUTABLE}" --version
                  OUTPUT_VARIABLE IWYU_VERSION
                  OUTPUT_STRIP_TRAILING_WHITESPACE)
  string(REGEX MATCH "[^\n]*include-what-you-use [^\n]*" IWYU_VERSION "${IWYU_VERSION}")
  string(REGEX REPLACE "include-what-you-use ([^ \n\r\t]+).*" "\\1" IWYU_VERSION "${IWYU_VERSION}")
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(IncludeWhatYouUse REQUIRED_VARS IWYU_EXECUTABLE
                                  VERSION_VAR IWYU_VERSION)

if(INCLUDEWHATYOUUSE_FOUND)
  # Supports Unix Makefiles and Ninja
  set(CMAKE_C_INCLUDE_WHAT_YOU_USE "${IWYU_EXECUTABLE}" PARENT_SCOPE)
  set(CMAKE_CXX_INCLUDE_WHAT_YOU_USE "${IWYU_EXECUTABLE}" PARENT_SCOPE)
endif()

mark_as_advanced(IWYU_EXECUTABLE)
