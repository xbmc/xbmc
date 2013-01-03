include(CheckIncludeFiles)

set(headers  
  libavcodec/avcodec
  libavfilter/avfilter
  libavformat/avformat
  libavutil/avutil
  libpostproc/postprocess
  libswscale/swscale
  
  ffmpeg/avcodec
  ffmpeg/avfilter
  ffmpeg/avformat
  ffmpeg/avutil
  postproc/postprocess
  ffmpeg/swscale
  
  libavcore/avcore
  libavcore/samplefmt
  libavutil/mem
  libavutil/samplefmt
  libavutil/opt
  libavutil/mathematics
  libswscale/rgb2rgb
  ffmpeg/rgb2rgb
  libswresample/swresample
  libavresample/avresample
)

foreach(header ${headers})
  set(_HAVE_VAR HAVE_${header}_H)
  string(TOUPPER ${_HAVE_VAR} _HAVE_VAR)
  string(REPLACE "/" "_" _HAVE_VAR ${_HAVE_VAR})
  find_path(_${_HAVE_VAR} NAMES ${header}.h HINTS ${FFMPEG_INCLUDE_DIRS})
  get_property(v VARIABLE PROPERTY _${_HAVE_VAR})
  if(v)
    set(${_HAVE_VAR} 1)
  endif()
endforeach()
