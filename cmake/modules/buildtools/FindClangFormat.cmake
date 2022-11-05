#.rst:
# FindClangFormat
# ----------
# Finds clang-format

find_program(CLANG_FORMAT_EXECUTABLE clang-format)

if(CLANG_FORMAT_EXECUTABLE)
  execute_process(COMMAND "${CLANG_FORMAT_EXECUTABLE}" --version
                  OUTPUT_VARIABLE CLANG_FORMAT_VERSION
                  OUTPUT_STRIP_TRAILING_WHITESPACE)
  string(REGEX REPLACE ".* version (.*)" "\\1" CLANG_FORMAT_VERSION "${CLANG_FORMAT_VERSION}")
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(ClangFormat REQUIRED_VARS CLANG_FORMAT_EXECUTABLE
                                  VERSION_VAR CLANG_FORMAT_VERSION)

mark_as_advanced(CLANG_FORMAT_EXECUTABLE)
