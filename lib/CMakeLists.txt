add_subdirectory(libsquish)
add_subdirectory(libapetag)
add_subdirectory(libhts)
add_subdirectory(cximage-6.0)

# build our external projects
include(ExternalProject)

add_subdirectory(ffmpeg)
add_subdirectory(cpluff)

# wrap them
add_custom_target(wraplibs ALL DEPENDS wrap_avcodec-52 wrap_avutil-50 wrap_avcore-0 wrap_avformat-52 wrap_avfilter-1 wrap_swscale-0 wrap_postproc-51 wrap_cpluff)
set(ffmpeglibs avcodec-52 avutil-50 avcore-0 avformat-52 avfilter-1 swscale-0 postproc-51)
foreach(f ${ffmpeglibs})
	install(PROGRAMS ${CMAKE_BINARY_DIR}/lib/${f}-${ARCH}.so DESTINATION ${CMAKE_INSTALL_PREFIX}/Plex.app/Contents/Resources/XBMC/system/players/dvdplayer/)
endforeach()
install(PROGRAMS ${CMAKE_BINARY_DIR}/lib/libcpluff-${ARCH}.so DESTINATION ${CMAKE_INSTALL_PREFIX}/Plex.app/Contents/Resources/XBMC/system)
