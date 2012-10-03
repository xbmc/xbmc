add_subdirectory(libsquish)
add_subdirectory(libapetag)
add_subdirectory(libhts)

# build our external projects
set(CMAKE_SHARED_MODULE_PREFIX "")
set(CMAKE_SHARED_MODULE_SUFFIX ".so")
if(APPLE)
	set(CMAKE_MODULE_LINKER_FLAGS "-arch i386 -undefined dynamic_lookup -read_only_relocs suppress")
	set(ARCH x86-osx)
	set(CMAKE_SHARED_MODULE_CREATE_C_FLAGS "-bundle")
endif()

add_subdirectory(cximage-6.0)
add_subdirectory(ffmpeg)
add_subdirectory(cpluff)

# wrap them
add_custom_target(wraplibs ALL DEPENDS
  avcodec-52-${ARCH}
  avutil-50-${ARCH}
  avcore-0-${ARCH}
  avformat-52-${ARCH}
  avfilter-1-${ARCH} 
  swscale-0-${ARCH} 
  postproc-51-${ARCH}
  cpluff-${ARCH}
  ImageLib-${ARCH}
)
