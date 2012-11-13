add_subdirectory(libsquish)
add_subdirectory(libhts)

# build our external projects
set(CMAKE_SHARED_MODULE_PREFIX "")
set(CMAKE_SHARED_MODULE_SUFFIX ".so")
if(APPLE)
	set(CMAKE_MODULE_LINKER_FLAGS "-arch i386 -undefined dynamic_lookup -read_only_relocs suppress")
	set(CMAKE_SHARED_MODULE_CREATE_C_FLAGS "-bundle")
endif(APPLE)


if(NOT WIN32)
  add_subdirectory(cximage-6.0)
  add_subdirectory(cpluff)
endif()
