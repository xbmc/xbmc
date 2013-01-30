include(CheckIncludeFiles)

set(headers  
  libavcodec/avcodec.h
  libavfilter/avfilter.h
  libavformat/avformat.h
  libavutil/avutil.h
  libavutil/pixfmt.h
  libpostproc/postprocess.h
  libswscale/swscale.h
  
  ffmpeg/avcodec.h
  ffmpeg/avfilter.h
  ffmpeg/avformat.h
  ffmpeg/avutil.h
  postproc/postprocess.h
  ffmpeg/swscale.h
  
  libavcore/avcore.h
  libavcore/samplefmt.h
  libavutil/mem.h
  libavutil/samplefmt.h
  libavutil/opt.h
  libavutil/mathematics.h
  libswscale/rgb2rgb.h
  ffmpeg/rgb2rgb.h
  libswresample/swresample.h
  libavresample/avresample.h
)

foreach(header ${headers})
  plex_find_header(${header} ${FFMPEG_INCLUDE_DIRS})
endforeach()

include(CheckCSourceCompiles)
include(CheckCXXSourceCompiles)

#### check FFMPEG member name
if(DEFINED HAVE_LIBAVFILTER_AVFILTER_H)
  set(AVFILTER_INC "#include <libavfilter/avfilter.h>")
else()
  set(AVFILTER_INC "#include <ffmpeg/avfilter.h>")
endif()
CHECK_C_SOURCE_COMPILES("
  ${AVFILTER_INC}
  int main(int argc, char *argv[])
  { 
    static AVFilterBufferRefVideoProps test;
    if(sizeof(test.sample_aspect_ratio))
      return 0;
    return 0;
  }
"
HAVE_AVFILTERBUFFERREFVIDEOPROPS_SAMPLE_ASPECT_RATIO)

if(DEFINED HAVE_LIBAVUTIL_PIXFMT_H)
    CHECK_CXX_SOURCE_COMPILES("
      #include <libavutil/pixfmt.h>
      int main() { PixelFormat format = PIX_FMT_VDPAU_MPEG4; }" 
    PIX_FMT_VDPAU_MPEG4_IN_AVUTIL)
endif()

