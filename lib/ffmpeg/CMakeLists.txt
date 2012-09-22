set(ffmpeg_cflags "-I${dependdir}/include -arch i386")
set(ffmpeg_ldflags "-L${dependdir}/lib -arch i386")
set(ffmpeg_conf_flags --prefix=${CMAKE_BINARY_DIR}/ffmpeg --extra-cflags=${ffmpeg_cflags} --extra-ldflags=${ffmpeg_ldflags} --disable-amd3dnow --disable-armv5te --disable-armv6t2 --disable-static --disable-muxers --enable-muxer=spdif --enable-muxer=adts --disable-encoders --enable-encoder=ac3 --enable-encoder=aac --enable-libvorbis --enable-muxer=ogg --enable-encoder=libvorbis --disable-devices --disable-ffprobe --disable-ffplay --disable-ffserver --disable-ffmpeg --enable-shared --disable-doc --disable-decoder=mpeg_xvmc --enable-postproc --enable-gpl --enable-protocol=http --enable-pthreads --enable-runtime-cpudetect)
if(APPLE)
  set(ffmpeg_conf_flags ${ffmpeg_conf_flags} --cc=${CMAKE_C_COMPILER})
endif()
if(CMAKE_BUILD_TYPE STREQUAL "Release")
  set(ffmpeg_conf_flags ${ffmpeg_conf_flags} --disable-debug)
endif()

externalproject_add(ffmpeg
  URL ${root}/lib/ffmpeg
  CONFIGURE_COMMAND ./configure ${ffmpeg_conf_flags}
  BUILD_COMMAND make -j4
  BUILD_IN_SOURCE 1
  INSTALL_COMMAND ""
  LOG_CONFIGURE 0
  LOG_BUILD 0
  LOG_INSTALL 0
)

if(APPLE)
	set(ldflag -arch i386 -bundle -flat_namespace -undefined suppress -read_only_relocs suppress -Wl,-alias_list ${root}/xbmc/cores/DllLoader/exports/wrapper_mach_alias)
	set(ARCH x86-osx)
	set(wrapper ${CMAKE_BINARY_DIR}/xbmc/cores/DllLoader/exports/CMakeFiles/exports.dir/wrapper.c.o)
endif()

macro(wrap_lib srcpath lib)
	file(GLOB objects ${srcpath}/*.o)
	add_custom_target(wrap_${lib} COMMAND ${CMAKE_C_COMPILER} ${ldflag} -o ${CMAKE_BINARY_DIR}/lib/${lib}-${ARCH}.so ${wrapper} ${objects} DEPENDS ffmpeg wrapper)
endmacro()

set(pref ${CMAKE_BINARY_DIR}/lib/ffmpeg/ffmpeg-prefix/src/ffmpeg)
wrap_lib(${pref}/libavcodec avcodec-52)
wrap_lib(${pref}/libutil avutil-50)
wrap_lib(${pref}/libavcore avcore-0)
wrap_lib(${pref}/libavformat avformat-52)
wrap_lib(${pref}/libavfilter avfilter-1)
wrap_lib(${pref}/libswscale swscale-0)
wrap_lib(${pref}/libpostproc postproc-51)
