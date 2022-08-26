#.rst:
# FindCppcheck
# ------------
# Finds cppcheck and sets it up to run along with the compiler for C and CXX.

find_program(CPPCHECK_EXECUTABLE cppcheck)

if(CPPCHECK_EXECUTABLE)
  execute_process(COMMAND "${CPPCHECK_EXECUTABLE}" --version
                  OUTPUT_VARIABLE CPPCHECK_VERSION
                  OUTPUT_STRIP_TRAILING_WHITESPACE)
  string(REGEX REPLACE "Cppcheck (.*)" "\\1" CPPCHECK_VERSION "${CPPCHECK_VERSION}")
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Cppcheck REQUIRED_VARS CPPCHECK_EXECUTABLE
                                  VERSION_VAR CPPCHECK_VERSION)

if(CPPCHECK_FOUND)
  # CMake < 3.16 treats Objective-C (OBJC) files as C files and Objective-C++ (OBJCXX) files as C++ files,
  # but cppcheck doesn't support Objective-C and Objective-C++.
  # CMake >= 3.16 added support for Objective-C and Objective-C++ language,
  # but doesn't support OBJC and OBJCXX for <LANG>_CLANG_TIDY.
  file(WRITE "${CMAKE_BINARY_DIR}/cppcheck" "case \"$@\" in *.m|*.mm) exit 0; esac\nexec \"${CPPCHECK_EXECUTABLE}\" --enable=performance --quiet --relative-paths=\"${CMAKE_SOURCE_DIR}\" \"$@\"\n")
  execute_process(COMMAND chmod +x "${CMAKE_BINARY_DIR}/cppcheck")

  # Supports Unix Makefiles and Ninja
  set(CMAKE_C_CPPCHECK "${CMAKE_BINARY_DIR}/cppcheck" PARENT_SCOPE)
  set(CMAKE_CXX_CPPCHECK "${CMAKE_BINARY_DIR}/cppcheck" PARENT_SCOPE)
endif()

mark_as_advanced(CPPCHECK_EXECUTABLE)
