add_subdirectory(libsquish)
add_subdirectory(libapetag)
add_subdirectory(libhts)
add_subdirectory(cpluff)

include(ExternalProject)

set(ffmpeg_cflags "-I${dependdir}/include -arch i386")
set(ffmpeg_ldflags "-L${dependdir}/lib -arch i386")
set(ffmpeg_conf_flags --prefix=${CMAKE_BINARY_DIR}/ffmpeg --extra-cflags=${ffmpeg_cflags} --extra-ldflags=${ffmpeg_ldflags} --disable-amd3dnow --disable-armv5te --disable-armv6t2 --disable-static --disable-muxers --enable-muxer=spdif --enable-muxer=adts --disable-encoders --enable-encoder=ac3 --enable-encoder=aac --enable-libvorbis --enable-muxer=ogg --enable-encoder=libvorbis --disable-devices --disable-ffprobe --disable-ffplay --disable-ffserver --disable-ffmpeg --enable-shared --disable-doc --disable-decoder=mpeg_xvmc --enable-postproc --enable-gpl --enable-protocol=http --enable-pthreads --enable-runtime-cpudetect)
if(APPLE)
  set(ffmpeg_conf_flags ${ffmpeg_conf_flags} --cc=clang)
endif()
if(CMAKE_BUILD_TYPE STREQUAL "Release")
  set(ffmpeg_conf_flags ${ffmpeg_conf_flags} --disable-debug)
endif()

externalproject_add(ffmpeg
  URL ${root}/lib/ffmpeg
  CONFIGURE_COMMAND ./configure ${ffmpeg_conf_flags}
  BUILD_COMMAND make -j4
  BUILD_IN_SOURCE 1
  INSTALL_COMMAND make install
)
