/*
 *      Copyright (C) 2005-2010 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#if (defined HAVE_CONFIG_H) && (!defined WIN32)
  #include "config.h"
#elif defined(_WIN32)
#include "system.h"
#endif

#if defined(HAVE_LIBVDADECODER)
#include "DynamicDll.h"
#include "GUISettings.h"
#include "DVDClock.h"
#include "DVDStreamInfo.h"
#include "DVDVideoCodecVDA.h"
#include "utils/log.h"
#include "utils/TimeUtils.h"

#include <CoreFoundation/CoreFoundation.h>
#include <CoreVideo/CoreVideo.h>

// missing in 10.4/10.5 SDKs
#if (MAC_OS_X_VERSION_MAX_ALLOWED < 1060)
#include "dlfcn.h"
enum {
  // Component Y'CbCr 8-bit 4:2:2, ordered Cb Y'0 Cr Y'1
  kCVPixelFormatType_422YpCbCr8 = FourCharCode('2vuy')
};
enum {
  // Planar Component Y'CbCr 8-bit 4:2:0
  kCVPixelFormatType_420YpCbCr8Planar = FourCharCode('y420')
};
#endif

////////////////////////////////////////////////////////////////////////////////////////////
//#include <VideoDecodeAcceleration/VDADecoder.h>
// http://developer.apple.com/mac/library/technotes/tn2010/tn2267.html
// VDADecoder API (keep this until VDADecoder.h is public)
enum {
  kVDADecoderNoErr = 0,
  kVDADecoderHardwareNotSupportedErr = -12470,		
  kVDADecoderFormatNotSupportedErr = -12471,
  kVDADecoderConfigurationError = -12472,
  kVDADecoderDecoderFailedErr = -12473,
};
enum {
  kVDADecodeInfo_Asynchronous = 1UL << 0,
  kVDADecodeInfo_FrameDropped = 1UL << 1
};
enum {
  // tells the decoder not to bother returning
  // a CVPixelBuffer in the outputCallback. The
  // output callback will still be called.
  kVDADecoderDecodeFlags_DontEmitFrame = 1 << 0
};
enum {
  // decode and return buffers for all frames currently in flight
  kVDADecoderFlush_EmitFrames = 1 << 0		
};
typedef struct OpaqueVDADecoder* VDADecoder;

typedef void (*VDADecoderOutputCallback)(
  void *decompressionOutputRefCon, 
  CFDictionaryRef frameInfo, 
  OSStatus status, 
  uint32_t infoFlags,
  CVImageBufferRef imageBuffer);

class DllLibVDADecoderInterface
{
public:
  virtual ~DllLibVDADecoderInterface() {}
  virtual OSStatus VDADecoderCreate(
    CFDictionaryRef decoderConfiguration, CFDictionaryRef destinationImageBufferAttributes,
    VDADecoderOutputCallback *outputCallback, void *decoderOutputCallbackRefcon, VDADecoder *decoderOut) = 0;
  virtual OSStatus VDADecoderDecode(
    VDADecoder decoder, uint32_t decodeFlags, CFTypeRef compressedBuffer, CFDictionaryRef frameInfo) = 0;
  virtual OSStatus VDADecoderFlush(VDADecoder decoder, uint32_t flushFlags) = 0;
  virtual OSStatus VDADecoderDestroy(VDADecoder decoder) = 0;
  virtual CFStringRef Get_kVDADecoderConfiguration_Height() = 0;
  virtual CFStringRef Get_kVDADecoderConfiguration_Width() = 0;
  virtual CFStringRef Get_kVDADecoderConfiguration_SourceFormat() = 0;
  virtual CFStringRef Get_kVDADecoderConfiguration_avcCData() = 0;
};

class DllLibVDADecoder : public DllDynamic, DllLibVDADecoderInterface
{
  DECLARE_DLL_WRAPPER(DllLibVDADecoder, "/System/Library/Frameworks/VideoDecodeAcceleration.framework/Versions/Current/VideoDecodeAcceleration")

  DEFINE_METHOD5(OSStatus, VDADecoderCreate, (CFDictionaryRef p1, CFDictionaryRef p2, VDADecoderOutputCallback* p3, void* p4, VDADecoder* p5))
  DEFINE_METHOD4(OSStatus, VDADecoderDecode, (VDADecoder p1, uint32_t p2, CFTypeRef p3, CFDictionaryRef p4))
  DEFINE_METHOD2(OSStatus, VDADecoderFlush, (VDADecoder p1, uint32_t p2))
  DEFINE_METHOD1(OSStatus, VDADecoderDestroy, (VDADecoder p1))
  DEFINE_GLOBAL(CFStringRef, kVDADecoderConfiguration_Height)
  DEFINE_GLOBAL(CFStringRef, kVDADecoderConfiguration_Width)
  DEFINE_GLOBAL(CFStringRef, kVDADecoderConfiguration_SourceFormat)
  DEFINE_GLOBAL(CFStringRef, kVDADecoderConfiguration_avcCData)
  BEGIN_METHOD_RESOLVE()
    RESOLVE_METHOD(VDADecoderCreate)
    RESOLVE_METHOD(VDADecoderDecode)
    RESOLVE_METHOD(VDADecoderFlush)
    RESOLVE_METHOD(VDADecoderDestroy)
    RESOLVE_METHOD(kVDADecoderConfiguration_Height)
    RESOLVE_METHOD(kVDADecoderConfiguration_Width)
    RESOLVE_METHOD(kVDADecoderConfiguration_SourceFormat)
    RESOLVE_METHOD(kVDADecoderConfiguration_avcCData)
  END_METHOD_RESOLVE()
};

////////////////////////////////////////////////////////////////////////////////////////////
// helper function that wraps a time into a dictionary
static CFDictionaryRef MakeDictionaryWithDisplayTime(double inFrameDisplayTime)
{
  CFStringRef key = CFSTR("VideoDisplayTimeKey");
  CFNumberRef value = CFNumberCreate(
    kCFAllocatorDefault, kCFNumberDoubleType, &inFrameDisplayTime);

  return CFDictionaryCreate(
    kCFAllocatorDefault,
    (const void **)&key,
    (const void **)&value,
    1,
    &kCFTypeDictionaryKeyCallBacks,
    &kCFTypeDictionaryValueCallBacks);
}

// helper function to extract a time from a dictionary
static double GetFrameDisplayTimeFromDictionary(CFDictionaryRef inFrameInfoDictionary)
{
  CFNumberRef timeNumber = NULL;
  double outValue = 0.0;

  if (NULL == inFrameInfoDictionary)
    return 0.0;

  timeNumber = (CFNumberRef)CFDictionaryGetValue(inFrameInfoDictionary, CFSTR("VideoDisplayTimeKey"));
  if (timeNumber)
    CFNumberGetValue(timeNumber, kCFNumberDoubleType, &outValue);

  return outValue;
}

////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////
CDVDVideoCodecVDA::CDVDVideoCodecVDA() : CDVDVideoCodec()
{
  m_dll = new DllLibVDADecoder;
  m_vda_decoder = NULL;
  m_pFormatName = "vda-unknown";

  m_queue_depth = 0;
  m_display_queue = NULL;
  pthread_mutex_init(&m_queue_mutex, NULL);

  m_swcontext = NULL;
  memset(&m_videobuffer, 0, sizeof(DVDVideoPicture));
}

CDVDVideoCodecVDA::~CDVDVideoCodecVDA()
{
  Dispose();
  delete m_dll;
}

#if (MAC_OS_X_VERSION_MAX_ALLOWED < 1060)
//  extern const CFStringRef kCVPixelBufferIOSurfacePropertiesKey __attribute__((weak_import));
#endif

bool CDVDVideoCodecVDA::Open(CDVDStreamInfo &hints, CDVDCodecOptions &options)
{
  if (g_guiSettings.GetBool("videoplayer.usevda") && !hints.software)
  {
    switch (hints.codec)
    {
      case CODEC_ID_H264:
        // source must be H.264 with valid avcC atom in extradata
        if (hints.extrasize < 7 || hints.extradata == NULL)
          return false;
        m_format = 'avc1';
        m_pFormatName = "vda-h264";
      break;
      default:
        return false;
      break;
    }

    // Input stream is qualified, now we can load the dlls.
    if (!m_dll->Load() || !m_dllSwScale.Load())
      return false;

    CFMutableDictionaryRef decoderConfiguration = (CFDictionaryCreateMutable(
      kCFAllocatorDefault, 4, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks));

    SInt32 width  = (SInt32)hints.width;
    SInt32 height = (SInt32)hints.height;

    CFNumberRef avcWidth  = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &width);
    CFNumberRef avcHeight = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &height);
    CFNumberRef avcFormat = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &m_format);
    CFDataRef   avcCData  = CFDataCreate(kCFAllocatorDefault, (const uint8_t*)hints.extradata, hints.extrasize);
    
    CFDictionarySetValue(decoderConfiguration, m_dll->Get_kVDADecoderConfiguration_Height(), avcHeight);
    CFDictionarySetValue(decoderConfiguration, m_dll->Get_kVDADecoderConfiguration_Width(), avcWidth);
    CFDictionarySetValue(decoderConfiguration, m_dll->Get_kVDADecoderConfiguration_SourceFormat(), avcFormat);
    CFDictionarySetValue(decoderConfiguration, m_dll->Get_kVDADecoderConfiguration_avcCData(), avcCData);
    
    // create the hardware decoder object
    OSStatus status = m_dll->VDADecoderCreate(decoderConfiguration, NULL, 
      (VDADecoderOutputCallback *)&VDADecoderCallback, this, (VDADecoder*)&m_vda_decoder);
    
    if (kVDADecoderNoErr != status) 
    {
      CLog::Log(LOGNOTICE, "%s - failed to open VDADecoder Codec", __FUNCTION__);
      return false;
    }

    if (decoderConfiguration)
      CFRelease(decoderConfiguration);
    
    //Allocate for YV12 frame
    //First make sure all properties are reset
    memset(&m_videobuffer, 0, sizeof(DVDVideoPicture));
    unsigned int iPixels = width*height;
    unsigned int iChromaPixels = iPixels/4;

    m_videobuffer.iWidth  = width;
    m_videobuffer.iHeight = height;
    m_videobuffer.iDisplayWidth  = width;
    m_videobuffer.iDisplayHeight = height;
    m_videobuffer.format = DVDVideoPicture::FMT_YUV420P;
    m_videobuffer.color_range = 0;
    m_videobuffer.color_matrix = 4;

    m_videobuffer.iLineSize[0] = width;   //Y
    m_videobuffer.iLineSize[1] = width/2; //U
    m_videobuffer.iLineSize[2] = width/2; //V
    m_videobuffer.iLineSize[3] = 0;

    m_videobuffer.data[0] = (BYTE*)_aligned_malloc(iPixels, 16);       //Y
    m_videobuffer.data[1] = (BYTE*)_aligned_malloc(iChromaPixels, 16); //U
    m_videobuffer.data[2] = (BYTE*)_aligned_malloc(iChromaPixels, 16); //V
    m_videobuffer.data[3] = NULL;

    //Set all data to 0 for less artifacts.. hmm.. what is black in YUV??
    memset(m_videobuffer.data[0], 0, iPixels);
    memset(m_videobuffer.data[1], 0, iChromaPixels);
    memset(m_videobuffer.data[2], 0, iChromaPixels);
    m_videobuffer.pts = DVD_NOPTS_VALUE;
    m_videobuffer.iFlags = DVP_FLAG_ALLOCATED;

    // pre-alloc ffmpeg swscale context
    m_swcontext = m_dllSwScale.sws_getContext(
      m_videobuffer.iWidth, m_videobuffer.iHeight, PIX_FMT_UYVY422, 
      m_videobuffer.iWidth, m_videobuffer.iHeight, PIX_FMT_YUV420P, 
      SWS_FAST_BILINEAR, NULL, NULL, NULL);

    m_DropPictures = false;

    return true;
  }

  return false;
}

void CDVDVideoCodecVDA::Dispose()
{
  if (m_vda_decoder)
  {
    m_dll->VDADecoderDestroy((VDADecoder)m_vda_decoder);
    m_vda_decoder = NULL;
  }
  if( !(m_videobuffer.iFlags & DVP_FLAG_ALLOCATED) )
  {
    _aligned_free(m_videobuffer.data[0]);
    _aligned_free(m_videobuffer.data[1]);
    _aligned_free(m_videobuffer.data[2]);
  }
  if (m_swcontext)
    m_dllSwScale.sws_freeContext(m_swcontext);
  m_dllSwScale.Unload();
  m_dll->Unload();
}

void CDVDVideoCodecVDA::SetDropState(bool bDrop)
{
  m_DropPictures = bDrop;
}

int CDVDVideoCodecVDA::Decode(BYTE* pData, int iSize, double dts, double pts)
{
  OSStatus status;
  uint32_t avc_flags = 0;
  //
  CFDictionaryRef avc_pts = MakeDictionaryWithDisplayTime(pts);
  CFDataRef avc_demux = CFDataCreate(kCFAllocatorDefault, pData, iSize);
	
  pthread_mutex_lock(&m_queue_mutex);
  m_dts_queue.push(dts);
  pthread_mutex_unlock(&m_queue_mutex);
  if (m_DropPictures)
    avc_flags = kVDADecoderDecodeFlags_DontEmitFrame;
  status = m_dll->VDADecoderDecode((VDADecoder)m_vda_decoder, avc_flags, avc_demux, avc_pts);
	
  CFRelease(avc_pts);
  CFRelease(avc_demux);

  if (status = kVDADecoderNoErr) 
  {
    CLog::Log(LOGNOTICE, "%s - VDADecoderDecode failed with status(%d)", __FUNCTION__, (int)status);
    return VC_ERROR;
  }

  // todo: queue depth is related to the number of reference frames in encoded h.264
  // so we need to buffer until we get N ref frames + 1
  if (m_queue_depth < 16)
    return VC_BUFFER;

  return VC_PICTURE | VC_BUFFER;
}

void CDVDVideoCodecVDA::Reset(void)
{
  m_dll->VDADecoderFlush((VDADecoder)m_vda_decoder, 0);

  while (m_queue_depth)
    DisplayQueuePop();
}

bool CDVDVideoCodecVDA::GetPicture(DVDVideoPicture* pDvdVideoPicture)
{
  CVPixelBufferRef yuvframe;
  
  // clone the video picture buffer settings
  *pDvdVideoPicture = m_videobuffer;

  // get the top yuv frame, we risk getting the wrong frame if the frame queue
  // depth is less than the number of encoded reference frames. If queue depth
  // is greater than the number of encoded reference frames, then the top frame
  // will never change and we can just grab a ref to the frame/pts. This way
  // we don't lockout the vdadecoder while doing the memcpy of planes out.
  pthread_mutex_lock(&m_queue_mutex);
  yuvframe = m_display_queue->frame;
  pDvdVideoPicture->dts = m_dts_queue.front(); // m_dts_queue gets popped in DisplayQueuePop
  pDvdVideoPicture->pts = m_display_queue->frametime;
  pthread_mutex_unlock(&m_queue_mutex);

  // lock the yuvframe down
  CVPixelBufferLockBaseAddress(yuvframe, 0);
  int yuv422_stride   = CVPixelBufferGetBytesPerRowOfPlane(yuvframe, 0);
  uint8_t *yuv422_ptr = (uint8_t*)CVPixelBufferGetBaseAddressOfPlane(yuvframe, 0);
  if (yuv422_ptr)
    UYVY422_to_YUV420P(yuv422_ptr, yuv422_stride, pDvdVideoPicture);
  // unlock the pixel buffer
  CVPixelBufferUnlockBaseAddress(yuvframe, 0);
	
  // now we can pop the top frame
  DisplayQueuePop();

  return VC_PICTURE | VC_BUFFER;
}

void CDVDVideoCodecVDA::UYVY422_to_YUV420P(uint8_t *yuv422_ptr, int yuv422_stride, DVDVideoPicture *picture)
{
  // convert PIX_FMT_UYVY422 to PIX_FMT_YUV420P
  if (m_swcontext)
  {
    uint8_t *src[]  = { yuv422_ptr, 0, 0, 0 };
    int srcStride[] = { yuv422_stride, 0, 0, 0 };

    uint8_t *dst[]  = { picture->data[0], picture->data[1], picture->data[2], 0 };
    int dstStride[] = { picture->iLineSize[0], picture->iLineSize[1], picture->iLineSize[2], 0 };
  
    m_dllSwScale.sws_scale(m_swcontext, src, srcStride, 0, picture->iHeight, dst, dstStride);
  }
}

void CDVDVideoCodecVDA::DisplayQueuePop(void)
{
  if (!m_display_queue || m_queue_depth == 0)
    return;

  // pop the current frame off the queue
  pthread_mutex_lock(&m_queue_mutex);
  frame_queue *top_frame = m_display_queue;
  m_display_queue = m_display_queue->nextframe;
  m_queue_depth--;
  if (!m_dts_queue.empty())
    m_dts_queue.pop();
  pthread_mutex_unlock(&m_queue_mutex);

  // release the frame buffer
  CVPixelBufferRelease(top_frame->frame);
  free(top_frame);
}

void CDVDVideoCodecVDA::VDADecoderCallback(
  void                *decompressionOutputRefCon,
   CFDictionaryRef    frameInfo,
   OSStatus           status, 
   uint32_t           infoFlags,
   CVImageBufferRef   imageBuffer)
{
  CDVDVideoCodecVDA *ctx = (CDVDVideoCodecVDA*)decompressionOutputRefCon;

  if (imageBuffer == NULL) 
  {
    CLog::Log(LOGERROR, "%s - imageBuffer is NULL", __FUNCTION__);
    return;
  }
  if (CVPixelBufferGetPixelFormatType(imageBuffer) != kCVPixelFormatType_422YpCbCr8)
  {
    CLog::Log(LOGERROR, "%s - imageBuffer format is not '2vuy", __FUNCTION__);
    return;
  }
  if (kVDADecodeInfo_FrameDropped & infoFlags)
  {
    CLog::Log(LOGDEBUG, "%s - frame dropped", __FUNCTION__);
    pthread_mutex_lock(&ctx->m_queue_mutex);
    if (!ctx->m_dts_queue.empty())
      ctx->m_dts_queue.pop();
    pthread_mutex_unlock(&ctx->m_queue_mutex);
    return;
  }

  // allocate a new frame and populate it with some information
  // this pointer to a frame_queue type keeps track of the newest decompressed frame
  // and is then inserted into a linked list of  frame pointers depending on the display time
  // parsed out of the bitstream and stored in the frameInfo dictionary by the client
  frame_queue *newFrame = (frame_queue*)calloc(sizeof(frame_queue), 1);
  newFrame->nextframe = NULL;
  newFrame->frame = CVPixelBufferRetain(imageBuffer);
  newFrame->frametime = GetFrameDisplayTimeFromDictionary(frameInfo);
	
  // since the frames we get may be in decode order rather than presentation order
  // our hypothetical callback places them in a queue of frames which will
  // hold them in display order for display on another thread
  pthread_mutex_lock(&ctx->m_queue_mutex);
	
  frame_queue *queueWalker = ctx->m_display_queue;
  if (!queueWalker || (newFrame->frametime < queueWalker->frametime))
  {
    // we have an empty queue, or this frame earlier than the current queue head
    newFrame->nextframe = queueWalker;
    ctx->m_display_queue = newFrame;
  } else {
    // walk the queue and insert this frame where it belongs in display order
    bool frameInserted = false;
    frame_queue *nextFrame = NULL;

    while (!frameInserted) {
      nextFrame = queueWalker->nextframe;
      if (!nextFrame || (newFrame->frametime < nextFrame->frametime))
      {
        // if the next frame is the tail of the queue, or our new frame is ealier
        newFrame->nextframe = nextFrame;
        queueWalker->nextframe = newFrame;
        frameInserted = true;
      }
      queueWalker = nextFrame;
    }
  }
  ctx->m_queue_depth++;

  pthread_mutex_unlock(&ctx->m_queue_mutex);	
}
#endif
