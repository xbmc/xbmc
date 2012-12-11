add_subdirectory(libsquish)
add_subdirectory(libhts)
if(WIN32)
  add_subdirectory(win32)
endif()

# build our external projects
set(CMAKE_SHARED_MODULE_PREFIX "")
if(NOT WIN32)
  set(CMAKE_SHARED_MODULE_SUFFIX ".so")
endif()
if(APPLE)
	set(CMAKE_MODULE_LINKER_FLAGS "-arch i386 -undefined dynamic_lookup -read_only_relocs suppress")
	set(CMAKE_SHARED_MODULE_CREATE_C_FLAGS "-bundle")
endif(APPLE)

add_subdirectory(cpluff)
add_subdirectory(cximage-6.0)
