include(ExternalProject)

if(TARGET_LINUX OR TARGET_RPI)
  set(OS linux)
elseif(TARGET_DARWIN)
  set(OS darwin)
endif()



set(CONFIG_FLAGS --target-os=${OS} --disable-muxers --disable-encoders --disable-devices --disable-doc --disable-ffplay --disable-ffmpeg --disable-ffprobe --disable-ffserver --disable-vda --disable-crystalhd --disable-decoder=mpeg_xvmc --disable-debug --disable-amd3dnow --disable-libvorbis --enable-gpl --enable-postproc --enable-shared --disable-static --enable-pthreads --enable-muxer=spdif --enable-muxer=adts --enable-encoder=ac3 --enable-encoder=aac --enable-protocol=http --enable-runtime-cpudetect)


if(TARGET_RPI)

  list(APPEND CONFIG_FLAGS --enable-hardcoded-tables --enable-pic
  --cpu=arm1176jzf-s --arch=arm --enable-cross-compile
  --cross-prefix=${TARGET_PREFIX} --sysroot=${SYSROOT_PREFIX}
  --sysinclude="${SYSROOT_PREFIX}/usr/include" )

endif()


ExternalProject_Add(
  ffmpeg
  URL .
  PREFIX ffmpeg
  CONFIGURE_COMMAND <SOURCE_DIR>/configure --prefix=<INSTALL_DIR> ${CONFIG_FLAGS}
  BUILD_COMMAND make -j 4
  INSTALL_COMMAND make install
)

#define DLL_PATH_LIBAVCODEC    "special://xbmcbin/system/players/dvdplayer/avcodec-53-x86_64-linux.so"
#define DLL_PATH_LIBAVFORMAT   "special://xbmcbin/system/players/dvdplayer/avformat-53-x86_64-linux.so"
#define DLL_PATH_LIBAVUTIL     "special://xbmcbin/system/players/dvdplayer/avutil-51-x86_64-linux.so"
#define DLL_PATH_LIBPOSTPROC   "special://xbmcbin/system/players/dvdplayer/postproc-52-x86_64-linux.so"
#define DLL_PATH_LIBSWSCALE    "special://xbmcbin/system/players/dvdplayer/swscale-2-x86_64-linux.so"
#define DLL_PATH_LIBAVFILTER   "special://xbmcbin/system/players/dvdplayer/avfilter-2-x86_64-linux.so"
#define DLL_PATH_LIBSWRESAMPLE "special://xbmcbin/system/players/dvdplayer/swresample-0-x86_64-linux.so"

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/ffmpeg/lib/libavcodec.so.53.61.100 DESTINATION ${BINPATH}/system/players/dvdplayer RENAME avcodec-53-${ARCH}.so)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/ffmpeg/lib/libavformat.so.53.32.100 DESTINATION ${BINPATH}/system/players/dvdplayer RENAME avformat-53-${ARCH}.so)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/ffmpeg/lib/libavutil.so.51.35.100 DESTINATION ${BINPATH}/system/players/dvdplayer RENAME avutil-51-${ARCH}.so)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/ffmpeg/lib/libpostproc.so.52.0.100 DESTINATION ${BINPATH}/system/players/dvdplayer RENAME postproc-52-${ARCH}.so)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/ffmpeg/lib/libswscale.so.2.1.100 DESTINATION ${BINPATH}/system/players/dvdplayer RENAME swscale-2-${ARCH}.so)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/ffmpeg/lib/libavfilter.so.2.61.100 DESTINATION ${BINPATH}/system/players/dvdplayer RENAME avfilter-2-${ARCH}.so)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/ffmpeg/lib/libswresample.so.0.6.100 DESTINATION ${BINPATH}/system/players/dvdplayer RENAME swresample-0-${ARCH}.so)

