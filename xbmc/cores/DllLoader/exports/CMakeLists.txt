set(SOURCES emu_dummy.cpp
            emu_kernel32.cpp
            emu_msvcrt.cpp)

set(HEADERS emu_dummy.h
            emu_kernel32.h
            emu_msvcrt.h)

core_add_library(dllexports)

if(CORE_SYSTEM_NAME STREQUAL darwin OR CORE_SYSTEM_NAME STREQUAL ios)
  add_library(wrapper OBJECT wrapper.c)
  add_custom_target(wrapper.def ALL ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/wrapper_mach_alias wrapper.def)
  add_dependencies(wrapper.def wrapper)
elseif(NOT CORE_SYSTEM_NAME STREQUAL windows)
  add_options(C ALL_BUILDS "-fPIC")
  add_library(wrapper OBJECT wrapper.c)

  add_custom_target(wrapper.def ALL nm ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/wrapper.dir/wrapper.c.o | grep __wrap | awk '{ printf(\"%s \", \$\$3) }' | sed \"s/___wrap_/__wrap_/g\" | sed \"s/__wrap_/-Wl,-wrap,/g\" > wrapper.def)

  if(CORE_SYSTEM_NAME STREQUAL android)
    add_custom_target(patchdef ALL echo \"-L${CMAKE_INSTALL_PREFIX}/lib/dummy-libxbmc/ -lxbmc\" >> wrapper.def)
    add_dependencies(patchdef wrapper.def)
  endif()

  add_dependencies(wrapper.def wrapper)
endif()
