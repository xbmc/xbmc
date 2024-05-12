#.rst:
# FindAtomic
# -----
# Finds the ATOMIC library
#
# This will define the following target:
#
#   ${APP_NAME_LC}::ATOMIC    - The ATOMIC library

if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})
  include(CheckCXXSourceCompiles)
  include(FindPackageMessage)

  set(atomic_code
      "
       #include <atomic>
       #include <cstdint>
       std::atomic<uint8_t> n8 (0); // riscv64
       std::atomic<uint64_t> n64 (0); // armel, mipsel, powerpc
       int main() {
         ++n8;
         ++n64;
         return 0;
       }")

  check_cxx_source_compiles("${atomic_code}" ATOMIC_LOCK_FREE_INSTRUCTIONS)

  if(ATOMIC_LOCK_FREE_INSTRUCTIONS)
    find_package_message(Atomic "Found Atomic: Lock Free" "")
  else()
    set(CMAKE_REQUIRED_LIBRARIES "-latomic")
    check_cxx_source_compiles("${atomic_code}" ATOMIC_IN_LIBRARY)
    set(CMAKE_REQUIRED_LIBRARIES)
    if(ATOMIC_IN_LIBRARY)
      find_package_message(Atomic "Found Atomic library: -latomic" "")

      add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} UNKNOWN IMPORTED)
      set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                       IMPORTED_LOCATION "-latomic")
    else()
      if(Atomic_FIND_REQUIRED)
        message(FATAL_ERROR "Neither lock free instructions nor -latomic found.")
      endif()
    endif()
  endif()
  unset(atomic_code)
endif()
