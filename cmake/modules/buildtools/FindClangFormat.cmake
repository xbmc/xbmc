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

if(CLANGFORMAT_FOUND)

  set(CLANGFORMAT_MINVERSION "9.0")

  execute_process(COMMAND clang-format --version ERROR_QUIET OUTPUT_VARIABLE CLANG_FORMAT_VERSION_OUTPUT)
  string(REGEX MATCH "([0-9]+)\\..*" CLANG_FORMAT_VERSION ${CLANG_FORMAT_VERSION_OUTPUT})
  if(CLANG_FORMAT_VERSION VERSION_LESS_EQUAL ${CLANGFORMAT_MINVERSION})
    message(WARNING "clang-format must be at least version ${CLANGFORMAT_MINVERSION}. The version found is ${CLANG_FORMAT_VERSION}")
    set(CLANGFORMAT_FOUND OFF CACHE BOOL "" FORCE)
    set(CLANG_FORMAT_EXECUTABLE "CLANG_FORMAT_EXECUTABLE-NOTFOUND" CACHE STRING "" FORCE)
  endif()
endif()

mark_as_advanced(CLANG_FORMAT_EXECUTABLE)
