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

#if defined(__APPLE__)
#if (defined HAVE_CONFIG_H)
  #include "config.h"
#endif

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
  virtual OSStatus VDADecoderDestroy(VDADecoder decoder)= 0;
  virtual CFStringRef Get_kVDADecoderConfiguration_Height() = 0;
  virtual CFStringRef Get_kVDADecoderConfiguration_Width() = 0;
  virtual CFStringRef Get_kVDADecoderConfiguration_SourceFormat() = 0;
  virtual CFStringRef Get_kVDADecoderConfiguration_avcCData() = 0;
};

class DllLibVDADecoder : public DllDynamic, DllLibVDADecoderInterface
{
  DECLARE_DLL_WRAPPER(DllLibVDADecoder, "/System/Frameworks/VideoDecodeAcceleration.framework/VideoDecodeAcceleration")

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

  if (NULL == inFrameInfoDictionary) return 0.0;

  timeNumber = (CFNumberRef)CFDictionaryGetValue(inFrameInfoDictionary, CFSTR("VideoDisplayTimeKey"));
  if (timeNumber) CFNumberGetValue(timeNumber, kCFNumberDoubleType, &outValue);

  return outValue;
}

////////////////////////////////////////////////////////////////////////////////////////////
static void VDADecoderCallback(
  void                *decompressionOutputRefCon,
   CFDictionaryRef    frameInfo,
   OSStatus           status, 
   uint32_t           infoFlags,
   CVImageBufferRef   imageBuffer)
{
  CDVDVideoCodecVDA *ctx = (CDVDVideoCodecVDA *)decompressionOutputRefCon;

  if (imageBuffer == NULL) 
  {
    CLog::Log(LOGERROR, "%s - imageBuffer is NULL", __FUNCTION__);
    return;
  }
  if (kCVPixelFormatType_420YpCbCr8Planar != CVPixelBufferGetPixelFormatType(imageBuffer))
  {
    CLog::Log(LOGERROR, "%s - imageBuffer format is not 'yv12", __FUNCTION__);
    return;
  }
  if (kVDADecodeInfo_FrameDropped & infoFlags)
  {
    CLog::Log(LOGDEBUG, "%s - frame dropped", __FUNCTION__);
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
  if (!queueWalker || (newFrame->frametime < queueWalker->frametime)) {
    // we have an empty queue, or this frame earlier than the current queue head
    newFrame->nextframe = queueWalker;
    ctx->m_display_queue = newFrame;
  } else {
    // walk the queue and insert this frame where it belongs in display order
    bool frameInserted = false;
    frame_queue *nextFrame = NULL;

    while (!frameInserted) {
      nextFrame = queueWalker->nextframe;
      if (!nextFrame || (newFrame->frametime < nextFrame->frametime)) {
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

////////////////////////////////////////////////////////////////////////////////////////////
CDVDVideoCodecVDA::CDVDVideoCodecVDA() : CDVDVideoCodec()
{
  m_pFormatName = "vda-h264";

  m_dll = NULL;
  m_vda_decoder = NULL;

  m_queue_depth = 0;
  m_display_queue = NULL;
  pthread_mutex_init(&m_queue_mutex, NULL);

  memset(&m_pVideoBuffer, 0, sizeof(DVDVideoPicture));
  m_dll = new DllLibVDADecoder;
  if (m_dll)
    m_dll->Load();
}

CDVDVideoCodecVDA::~CDVDVideoCodecVDA()
{
  Dispose();

  if (m_dll)
    delete m_dll;
}

// need to export this as it's only present (CVPixelBuffer.h) in 10.6SDK.
CV_EXPORT const CFStringRef kCVPixelBufferIOSurfacePropertiesKey;

bool CDVDVideoCodecVDA::Open(CDVDStreamInfo &hints, CDVDCodecOptions &options)
{
  if (m_dll && g_guiSettings.GetBool("videoplayer.usevda") && !hints.software)
  {
    switch (hints.codec)
    {
      case CODEC_ID_H264:
        // source must be H.264 with valid avcC atom in extradata
        if (hints.extrasize < 7 || hints.extradata == NULL)
          return FALSE;
        m_pFormatName = "vda-h264";
      break;
      default:
        return false;
      break;
    }

    CFMutableDictionaryRef decoderConfiguration = (CFDictionaryCreateMutable(
      kCFAllocatorDefault,
      4,
      &kCFTypeDictionaryKeyCallBacks,
      &kCFTypeDictionaryValueCallBacks));

    SInt32 width = (SInt32)hints.width;
    SInt32 height = (SInt32)hints.height;
    SInt32 format = 'avc1';

    CFNumberRef avcWidth  = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &width);
    CFNumberRef avcHeight = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &height);
    CFNumberRef avcFormat = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &format);
    CFDataRef avcCData  = CFDataCreate(kCFAllocatorDefault, (const uint8_t*)hints.extradata, hints.extrasize);
    
    CFDictionarySetValue(decoderConfiguration, m_dll->Get_kVDADecoderConfiguration_Height(), avcHeight);
    CFDictionarySetValue(decoderConfiguration, m_dll->Get_kVDADecoderConfiguration_Width(), avcWidth);
    CFDictionarySetValue(decoderConfiguration, m_dll->Get_kVDADecoderConfiguration_SourceFormat(), avcFormat);
    CFDictionarySetValue(decoderConfiguration, m_dll->Get_kVDADecoderConfiguration_avcCData(), avcCData);
    
    // create a CFDictionary describing the desired destination image buffer
    CFMutableDictionaryRef destinationImageBufferAttributes = CFDictionaryCreateMutable(
      kCFAllocatorDefault,
      2,
      &kCFTypeDictionaryKeyCallBacks,
      &kCFTypeDictionaryValueCallBacks);

#if (MAC_OS_X_VERSION_MAX_ALLOWED < 1060)
    CFStringRef kCVPixelBufferIOSurfacePropertiesKey = (CFStringRef)dlsym(RTLD_NEXT, "kCVPixelBufferIOSurfacePropertiesKey");
#endif
    OSType cvPixelFormatType = kCVPixelFormatType_420YpCbCr8Planar;
    CFNumberRef pixelFormat = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &cvPixelFormatType);
    CFDictionaryRef emptyDictionary = CFDictionaryCreate(
      kCFAllocatorDefault, // our empty IOSurface properties dictionary
      NULL,
      NULL,
      0,
      &kCFTypeDictionaryKeyCallBacks,
      &kCFTypeDictionaryValueCallBacks);
    //
    CFDictionarySetValue(
      destinationImageBufferAttributes,
      kCVPixelBufferPixelFormatTypeKey,
      pixelFormat);
    //
    CFDictionarySetValue(
      destinationImageBufferAttributes,
      kCVPixelBufferIOSurfacePropertiesKey,
      emptyDictionary);
    
    // create the hardware decoder object
    OSStatus status = m_dll->VDADecoderCreate(
      decoderConfiguration,
      destinationImageBufferAttributes, 
      (VDADecoderOutputCallback *)&VDADecoderCallback,
      this,
      (VDADecoder*)&m_vda_decoder);
    
    if (kVDADecoderNoErr != status) 
    {
      CLog::Log(LOGNOTICE, "%s - failed to open VDADecoder Codec", __FUNCTION__);
      return FALSE;
    }
    
    //First make sure all properties are reset
    memset(&m_pVideoBuffer, 0, sizeof(DVDVideoPicture));

    //Allocate for YV12 frame
    unsigned int iPixels = width*height;
    unsigned int iChromaPixels = iPixels/4;

    m_pVideoBuffer.iWidth = width;
    m_pVideoBuffer.iHeight = height;

    m_pVideoBuffer.iLineSize[0] = width;   //Y
    m_pVideoBuffer.iLineSize[1] = width/2; //U
    m_pVideoBuffer.iLineSize[2] = width/2; //V
    m_pVideoBuffer.iLineSize[3] = 0;

    m_pVideoBuffer.data[0] = (BYTE*)_aligned_malloc(iPixels, 16);       //Y
    m_pVideoBuffer.data[1] = (BYTE*)_aligned_malloc(iChromaPixels, 16); //U
    m_pVideoBuffer.data[2] = (BYTE*)_aligned_malloc(iChromaPixels, 16); //V
    m_pVideoBuffer.data[3] = NULL;

    //Set all data to 0 for less artifacts.. hmm.. what is black in YUV??
    memset( m_pVideoBuffer.data[0], 0, iPixels );
    memset( m_pVideoBuffer.data[1], 0, iChromaPixels );
    memset( m_pVideoBuffer.data[2], 0, iChromaPixels );
    m_pVideoBuffer.pts = DVD_NOPTS_VALUE;
    m_pVideoBuffer.iFlags = DVP_FLAG_ALLOCATED;

    if (decoderConfiguration) CFRelease(decoderConfiguration);
    if (destinationImageBufferAttributes) CFRelease(destinationImageBufferAttributes);
    if (emptyDictionary) CFRelease(emptyDictionary);

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

    if( !(m_pVideoBuffer.iFlags & DVP_FLAG_ALLOCATED) )
    {
      _aligned_free(m_pVideoBuffer.data[0]);
      _aligned_free(m_pVideoBuffer.data[1]);
      _aligned_free(m_pVideoBuffer.data[2]);
    }
  }
}

void CDVDVideoCodecVDA::SetDropState(bool bDrop)
{
}

int CDVDVideoCodecVDA::Decode(BYTE* pData, int iSize, double dts, double pts)
{
  OSStatus status;

  //
  CFDictionaryRef avc_pts = MakeDictionaryWithDisplayTime(pts);
  CFDataRef avc_demux = CFDataCreate(kCFAllocatorDefault, pData, iSize);
	
  status = m_dll->VDADecoderDecode((VDADecoder)m_vda_decoder, 0, avc_demux, avc_pts);
	
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
  memcpy(pDvdVideoPicture, &m_pVideoBuffer, sizeof(DVDVideoPicture));

  // get the top yuv frame, we risk getting the wrong frame if the frame queue
  // depth is less than the number of encoded reference frames. If queue depth
  // is greater than the number of encoded reference frames, then the top frame
  // will never change and we can just grab a ref to the frame/pts. This way
  // we don't lockout the vdadecoder while doing the memcpy of planes out.
  pthread_mutex_lock(&m_queue_mutex);
  yuvframe = m_display_queue->frame;
  pDvdVideoPicture->pts = m_display_queue->frametime;
  pthread_mutex_unlock(&m_queue_mutex);

  // lock the yuvframe down
  CVPixelBufferLockBaseAddress(yuvframe, 0);
  for (size_t i = 0; i < 3; i++)
  {
    UInt32 width = CVPixelBufferGetBytesPerRowOfPlane(yuvframe, i);
    UInt32 height = CVPixelBufferGetHeightOfPlane(yuvframe, i);

    void *plane_ptr = CVPixelBufferGetBaseAddressOfPlane(yuvframe, i);
    memcpy(pDvdVideoPicture->data[i], plane_ptr, width * height);
  }
  // unlock the pixel buffer
  CVPixelBufferUnlockBaseAddress(yuvframe, 0);
	
  // now we can pop the top frame
  DisplayQueuePop();

  return VC_PICTURE | VC_BUFFER;
}

void CDVDVideoCodecVDA::DisplayQueuePop(void)
{
  if (!m_display_queue || m_queue_depth == 0) return;

  // pop the current frame off the queue
  pthread_mutex_lock(&m_queue_mutex);
  frame_queue *top_frame = m_display_queue;
  m_display_queue = m_display_queue->nextframe;
  m_queue_depth--;
  pthread_mutex_unlock(&m_queue_mutex);

  // release the frame buffer
  CVPixelBufferRelease(top_frame->frame);
  free(top_frame);
}
#endif
