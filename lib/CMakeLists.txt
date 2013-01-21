add_subdirectory(libsquish)
add_subdirectory(libhts)
add_subdirectory(libUPnP)

if(UNIX AND NOT APPLE)
  add_subdirectory(xbmc-dll-symbols)
endif()

if(WIN32)
  add_subdirectory(win32)
  add_subdirectory(libmad)
  add_subdirectory(libass)
  add_subdirectory(enca)
endif()

# build our external projects
set(CMAKE_SHARED_MODULE_PREFIX "")
if(NOT WIN32)
  set(CMAKE_SHARED_MODULE_SUFFIX ".so")
endif()

if(APPLE)
	set(CMAKE_SHARED_MODULE_CREATE_C_FLAGS "-bundle")
endif(APPLE)

add_subdirectory(cpluff)
add_subdirectory(cximage-6.0)
