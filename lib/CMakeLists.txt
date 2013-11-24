add_subdirectory(libsquish)
add_subdirectory(libhts)
add_subdirectory(libUPnP)

if(ENABLE_DVD_DRIVE)
  add_subdirectory(libdvd)
endif()

if(TARGET_COMMON_LINUX)
  add_subdirectory(xbmc-dll-symbols)
endif()

if(USE_INTERNAL_FFMPEG)
  add_subdirectory(ffmpeg)
endif(USE_INTERNAL_FFMPEG)

if(TARGET_WIN32)
  add_subdirectory(win32)
  add_subdirectory(libmad)
  add_subdirectory(libass)
  add_subdirectory(enca)
endif()

# build our external projects
set(CMAKE_SHARED_MODULE_PREFIX "")
if(TARGET_POSIX)
  set(CMAKE_SHARED_MODULE_SUFFIX ".so")
endif(TARGET_POSIX)

if(TARGET_COMMON_DARWIN)
	set(CMAKE_SHARED_MODULE_CREATE_C_FLAGS "-bundle")
endif(TARGET_COMMON_DARWIN)

add_subdirectory(cpluff)
add_subdirectory(cximage-6.0)
add_subdirectory(addons)

