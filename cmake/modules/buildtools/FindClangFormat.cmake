#.rst:
# FindClangFormat
# ----------
# Finds clang-format

find_program(CLANG_FORMAT_EXECUTABLE clang-format)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(ClangFormat REQUIRED_VARS CLANG_FORMAT_EXECUTABLE)

mark_as_advanced(CLANG_FORMAT_EXECUTABLE)
