#.rst:
# FindClangTidy
# -------------
# Finds clang-tidy and sets it up to run along with the compiler for C and CXX.

find_program(CLANG_TIDY_EXECUTABLE clang-tidy)

if(CLANG_TIDY_EXECUTABLE)
  execute_process(COMMAND "${CLANG_TIDY_EXECUTABLE}" --version
                  OUTPUT_VARIABLE CLANG_TIDY_VERSION
                  OUTPUT_STRIP_TRAILING_WHITESPACE)
  string(REGEX MATCH "[^\n]* version [^\n]*" CLANG_TIDY_VERSION "${CLANG_TIDY_VERSION}")
  string(REGEX REPLACE ".* version (.*)" "\\1" CLANG_TIDY_VERSION "${CLANG_TIDY_VERSION}")
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(ClangTidy REQUIRED_VARS CLANG_TIDY_EXECUTABLE
                                  VERSION_VAR CLANG_TIDY_VERSION)

if(CLANGTIDY_FOUND)
  if(CORE_SYSTEM_NAME STREQUAL android)
    set(CLANG_TIDY_EXECUTABLE ${CLANG_TIDY_EXECUTABLE};--extra-arg-before=--target=${HOST})
  endif()
  # Supports Unix Makefiles and Ninja
  set(CMAKE_C_CLANG_TIDY "${CLANG_TIDY_EXECUTABLE}" PARENT_SCOPE)
  set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_EXECUTABLE}" PARENT_SCOPE)
endif()

mark_as_advanced(CLANG_TIDY_EXECUTABLE)
