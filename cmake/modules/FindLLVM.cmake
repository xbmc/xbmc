#.rst:
# FindLLVM
# ----------
# Finds llvm tools
#

find_program(LLVM_AR_EXECUTABLE NAMES llvm-ar llvm-ar-12 llvm-ar-11 llvm-ar-10 llvm-ar-9 llvm-ar-8)
find_program(LLVM_NM_EXECUTABLE NAMES llvm-nm llvm-nm-12 llvm-nm-11 llvm-nm-10 llvm-nm-9 llvm-nm-8)
find_program(LLVM_RANLIB_EXECUTABLE NAMES llvm-ranlib llvm-ranlib-12 llvm-ranlib-11 llvm-ranlib-10 llvm-ranlib-9 llvm-ranlib-8)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LLVM REQUIRED_VARS LLVM_AR_EXECUTABLE LLVM_NM_EXECUTABLE LLVM_RANLIB_EXECUTABLE)

if(LLVM_FOUND)
  set(CMAKE_AR ${LLVM_AR_EXECUTABLE})
  set(CMAKE_NM ${LLVM_NM_EXECUTABLE})
  set(CMAKE_RANLIB ${LLVM_RANLIB_EXECUTABLE})
endif()
mark_as_advanced(CMAKE_AR CMAKE_NM CMAKE_RANLIB)
