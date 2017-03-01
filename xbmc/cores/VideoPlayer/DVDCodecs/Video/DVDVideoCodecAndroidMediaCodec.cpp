/*
 *      Copyright (C) 2013 Team XBMC
 *      http://xbmc.org
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

// http://developer.android.com/reference/android/media/MediaCodec.html
//
// Android MediaCodec class can be used to access low-level media codec,
// i.e. encoder/decoder components. (android.media.MediaCodec). Requires SDK21+
//

#include "DVDVideoCodecAndroidMediaCodec.h"

#include "Application.h"
#include "ServiceBroker.h"
#include "messaging/ApplicationMessenger.h"
#include "TimingConstants.h"
#include "utils/BitstreamConverter.h"
#include "utils/BitstreamWriter.h"

#include "utils/CPUInfo.h"
#include "utils/log.h"
#include "settings/AdvancedSettings.h"
#include "platform/android/activity/XBMCApp.h"
#include "cores/VideoPlayer/VideoRenderers/RenderManager.h"
#include "cores/VideoPlayer/VideoRenderers/RenderFlags.h"
#include "cores/VideoPlayer/DVDDemuxers/DemuxCrypto.h"

#include "platform/android/jni/ByteBuffer.h"
#include "platform/android/jni/MediaCodec.h"
#include "platform/android/jni/MediaCrypto.h"
#include "platform/android/jni/MediaFormat.h"
#include "platform/android/jni/MediaCodecList.h"
#include "platform/android/jni/MediaCodecInfo.h"
#include "platform/android/jni/MediaCodecCryptoInfo.h"
#include "platform/android/jni/Surface.h"
#include "platform/android/jni/SurfaceTexture.h"
#include "platform/android/jni/UUID.h"
#include "platform/android/activity/AndroidFeatures.h"
#include "settings/Settings.h"

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <cassert>

using namespace KODI::MESSAGING;

enum MEDIACODEC_STATES
{
  MEDIACODEC_STATE_UNINITIALIZED,
  MEDIACODEC_STATE_CONFIGURED,
  MEDIACODEC_STATE_FLUSHED,
  MEDIACODEC_STATE_RUNNING,
  MEDIACODEC_STATE_ENDOFSTREAM,
  MEDIACODEC_STATE_ERROR
};

static bool CanSurfaceRenderBlackList(const std::string &name)
{
  // All devices 'should' be capable of surface rendering
  // but that seems to be hit or miss as most odd name devices
  // cannot surface render.
  static const char *cannotsurfacerender_decoders[] = {
    NULL
  };
  for (const char **ptr = cannotsurfacerender_decoders; *ptr; ptr++)
  {
    if (!strnicmp(*ptr, name.c_str(), strlen(*ptr)))
      return true;
  }
  return false;
}

static bool IsBlacklisted(const std::string &name)
{
  static const char *blacklisted_decoders[] = {
    // No software decoders
    "OMX.google",
    // For Rockchip non-standard components
    "AVCDecoder",
    "AVCDecoder_FLASH",
    "FLVDecoder",
    "M2VDecoder",
    "M4vH263Decoder",
    "RVDecoder",
    "VC1Decoder",
    "VPXDecoder",
    // End of Rockchip
    NULL
  };
  for (const char **ptr = blacklisted_decoders; *ptr; ptr++)
  {
    if (!strnicmp(*ptr, name.c_str(), strlen(*ptr)))
      return true;
  }
  return false;
}

static bool IsSupportedColorFormat(int color_format)
{
  static const int supported_colorformats[] = {
    CJNIMediaCodecInfoCodecCapabilities::COLOR_FormatYUV420Planar,
    CJNIMediaCodecInfoCodecCapabilities::COLOR_TI_FormatYUV420PackedSemiPlanar,
    CJNIMediaCodecInfoCodecCapabilities::COLOR_FormatYUV420SemiPlanar,
    CJNIMediaCodecInfoCodecCapabilities::COLOR_QCOM_FormatYUV420SemiPlanar,
    CJNIMediaCodecInfoCodecCapabilities::OMX_QCOM_COLOR_FormatYVU420SemiPlanarInterlace,
    -1
  };
  for (const int *ptr = supported_colorformats; *ptr != -1; ptr++)
  {
    if (color_format == *ptr)
      return true;
  }
  return false;
}

/*****************************************************************************/
/*****************************************************************************/
class CNULL_Listener : public CJNISurfaceTextureOnFrameAvailableListener
{
public:
  CNULL_Listener() : CJNISurfaceTextureOnFrameAvailableListener(jni::jhobject(NULL)) {};

protected:
  virtual void OnFrameAvailable(CJNISurfaceTexture &surface) {};
};

class CDVDMediaCodecOnFrameAvailable : public CEvent, CJNISurfaceTextureOnFrameAvailableListener
{
public:
  CDVDMediaCodecOnFrameAvailable(std::shared_ptr<CJNISurfaceTexture> &surfaceTexture)
  : m_surfaceTexture(surfaceTexture)
  {
    m_surfaceTexture->setOnFrameAvailableListener(*this);
  }

  virtual ~CDVDMediaCodecOnFrameAvailable()
  {
    // unhook the callback
    CNULL_Listener null_listener;
    m_surfaceTexture->setOnFrameAvailableListener(null_listener);
  }

protected:
  virtual void OnFrameAvailable(CJNISurfaceTexture &surface)
  {
    Set();
  }

private:
  std::shared_ptr<CJNISurfaceTexture> m_surfaceTexture;

};

/*****************************************************************************/
/*****************************************************************************/
CDVDMediaCodecInfo::CDVDMediaCodecInfo(
    int index
  , unsigned int texture
  , std::shared_ptr<CJNIMediaCodec> &codec
  , std::shared_ptr<CJNISurfaceTexture> &surfacetexture
  , std::shared_ptr<CDVDMediaCodecOnFrameAvailable> &frameready
)
: m_refs(1)
, m_valid(true)
, m_isReleased(true)
, m_index(index)
, m_texture(texture)
, m_timestamp(0)
, m_codec(codec)
, m_surfacetexture(surfacetexture)
, m_frameready(frameready)
{
  // paranoid checks
  assert(m_index >= 0);
  assert(m_codec != NULL);
}

CDVDMediaCodecInfo::~CDVDMediaCodecInfo()
{
  assert(m_refs == 0);
}

CDVDMediaCodecInfo* CDVDMediaCodecInfo::Retain()
{
  ++m_refs;
  m_isReleased = false;

  return this;
}

long CDVDMediaCodecInfo::Release()
{
  long count = --m_refs;
  if (count == 1)
    ReleaseOutputBuffer(false);
  if (count == 0)
    delete this;

  return count;
}

void CDVDMediaCodecInfo::Validate(bool state)
{
  CSingleLock lock(m_section);

  m_valid = state;
}

bool CDVDMediaCodecInfo::WaitForFrame(int millis)
{
  return m_frameready->WaitMSec(millis);
}

void CDVDMediaCodecInfo::ReleaseOutputBuffer(bool render)
{
  CSingleLock lock(m_section);

  if (!m_valid || m_isReleased)
    return;

  // release OutputBuffer and render if indicated
  // then wait for rendered frame to become available.

  if (render)
    if (m_frameready)
      m_frameready->Reset();

  m_codec->releaseOutputBuffer(m_index, render);
  m_isReleased = true;

  if (xbmc_jnienv()->ExceptionCheck())
  {
    CLog::Log(LOGERROR, "CDVDMediaCodecInfo::ReleaseOutputBuffer "
      "ExceptionCheck index(%d), render(%d)", m_index, render);
    xbmc_jnienv()->ExceptionDescribe();
    xbmc_jnienv()->ExceptionClear();
  }
}

ssize_t CDVDMediaCodecInfo::GetIndex() const
{
  CSingleLock lock(m_section);

  return m_index;
}

int CDVDMediaCodecInfo::GetTextureID() const
{
  // since m_texture never changes,
  // we do not need a m_section lock here.
  return m_texture;
}

void CDVDMediaCodecInfo::GetTransformMatrix(float *textureMatrix)
{
  CSingleLock lock(m_section);

  if (!m_valid)
    return;

  m_surfacetexture->getTransformMatrix(textureMatrix);
}

void CDVDMediaCodecInfo::UpdateTexImage()
{
  CSingleLock lock(m_section);

  if (!m_valid)
    return;

  // updateTexImage will check and spew any prior gl errors,
  // clear them before we call updateTexImage.
  glGetError();

  // this is key, after calling releaseOutputBuffer, we must
  // wait a little for MediaCodec to render to the surface.
  // Then we can updateTexImage without delay. If we do not
  // wait, then video playback gets jerky. To optimize this,
  // we hook the SurfaceTexture OnFrameAvailable callback
  // using CJNISurfaceTextureOnFrameAvailableListener and wait
  // on a CEvent to fire. 50ms seems to be a good max fallback.
  WaitForFrame(50);

  m_surfacetexture->updateTexImage();
  if (xbmc_jnienv()->ExceptionCheck())
  {
    CLog::Log(LOGERROR, "CDVDMediaCodecInfo::UpdateTexImage updateTexImage:ExceptionCheck");
    xbmc_jnienv()->ExceptionDescribe();
    xbmc_jnienv()->ExceptionClear();
  }

  m_timestamp = m_surfacetexture->getTimestamp();
  if (xbmc_jnienv()->ExceptionCheck())
  {
    CLog::Log(LOGERROR, "CDVDMediaCodecInfo::UpdateTexImage getTimestamp:ExceptionCheck");
    xbmc_jnienv()->ExceptionDescribe();
    xbmc_jnienv()->ExceptionClear();
  }
}

void CDVDMediaCodecInfo::RenderUpdate(const CRect &SrcRect, const CRect &DestRect)
{
  CSingleLock lock(m_section);

  static CRect cur_rect;

  if (!m_valid)
    return;

  if (DestRect != cur_rect)
  {
    CRect adjRect = CXBMCApp::MapRenderToDroid(DestRect);
    CXBMCApp::get()->setVideoViewSurfaceRect(adjRect.x1, adjRect.y1, adjRect.x2, adjRect.y2);
    CLog::Log(LOGDEBUG, "RenderUpdate: Dest - %f+%f-%fx%f", DestRect.x1, DestRect.y1, DestRect.Width(), DestRect.Height());
    CLog::Log(LOGDEBUG, "RenderUpdate: Adj  - %f+%f-%fx%f", adjRect.x1, adjRect.y1, adjRect.Width(), adjRect.Height());
    cur_rect = DestRect;

    // setVideoViewSurfaceRect is async, so skip rendering this frame
    ReleaseOutputBuffer(false);
  }
  else
    ReleaseOutputBuffer(true);
}


/*****************************************************************************/
/*****************************************************************************/
CDVDVideoCodecAndroidMediaCodec::CDVDVideoCodecAndroidMediaCodec(CProcessInfo &processInfo, bool surface_render)
: CDVDVideoCodec(processInfo)
, m_formatname("mediacodec")
, m_opened(false)
, m_checkForPicture(false)
, m_surface(NULL)
, m_textureId(0)
, m_crypto(nullptr)
, m_bitstream(NULL)
, m_render_sw(false)
, m_render_surface(surface_render)
{
  memset(&m_videobuffer, 0x00, sizeof(DVDVideoPicture));
}

CDVDVideoCodecAndroidMediaCodec::~CDVDVideoCodecAndroidMediaCodec()
{
  Dispose();

  if (m_crypto)
    delete m_crypto;
}

std::atomic<bool> CDVDVideoCodecAndroidMediaCodec::m_InstanceGuard(false);

bool CDVDVideoCodecAndroidMediaCodec::Open(CDVDStreamInfo &hints, CDVDCodecOptions &options)
{
  int num_codecs;
  bool needSecureDecoder;

  m_opened = false;
  // allow only 1 instance here
  if (m_InstanceGuard.exchange(true))
  {
    CLog::Log(LOGERROR, "CDVDVideoCodecAndroidMediaCodec::Open - InstanceGuard locked\n");
    return false;
  }

  // mediacodec crashes with null size. Trap this...
  if (!hints.width || !hints.height)
  {
    CLog::Log(LOGERROR, "CDVDVideoCodecAndroidMediaCodec::Open - %s\n", "null size, cannot handle");
    goto FAIL;
  }
  else if (hints.stills || hints.dvd)
  {
    // Won't work reliably
    goto FAIL;
  }
  else if (hints.orientation && m_render_surface && CJNIMediaFormat::GetSDKVersion() < 23)
  {
    CLog::Log(LOGERROR, "CDVDVideoCodecAndroidMediaCodec::Open - %s\n", "Surface does not support orientation before API 23");
    goto FAIL;
  }
  else if (!CServiceBroker::GetSettings().GetBool(CSettings::SETTING_VIDEOPLAYER_USEMEDIACODEC) &&
           !CServiceBroker::GetSettings().GetBool(CSettings::SETTING_VIDEOPLAYER_USEMEDIACODECSURFACE))
    goto FAIL;

  m_render_surface = CServiceBroker::GetSettings().GetBool(CSettings::SETTING_VIDEOPLAYER_USEMEDIACODECSURFACE);
  m_drop = false;
  m_state = MEDIACODEC_STATE_UNINITIALIZED;
  m_noPictureLoop = 0;
  m_codecControlFlags = 0;
  m_hints = hints;
  m_indexInputBuffer = -1;

  switch(m_hints.codec)
  {
    case AV_CODEC_ID_MPEG2VIDEO:
      m_mime = "video/mpeg2";
      m_formatname = "amc-mpeg2";
      break;
    case AV_CODEC_ID_MPEG4:
      if (hints.width <= 800)
        goto FAIL;
      m_mime = "video/mp4v-es";
      m_formatname = "amc-mpeg4";
      break;
    case AV_CODEC_ID_H263:
      m_mime = "video/3gpp";
      m_formatname = "amc-h263";
      break;
    case AV_CODEC_ID_VP3:
    case AV_CODEC_ID_VP6:
    case AV_CODEC_ID_VP6F:
    case AV_CODEC_ID_VP8:
      //m_mime = "video/x-vp6";
      //m_mime = "video/x-vp7";
      m_mime = "video/x-vnd.on2.vp8";
      m_formatname = "amc-vpX";
      break;
    case AV_CODEC_ID_VP9:
      m_mime = "video/x-vnd.on2.vp9";
      m_formatname = "amc-vp9";
      break;
    case AV_CODEC_ID_AVS:
    case AV_CODEC_ID_CAVS:
    case AV_CODEC_ID_H264:
      switch(hints.profile)
      {
        case FF_PROFILE_H264_HIGH_10:
        case FF_PROFILE_H264_HIGH_10_INTRA:
          // No known h/w decoder supporting Hi10P
          goto FAIL;
      }
      m_mime = "video/avc";
      m_formatname = "amc-h264";
      // check for h264-avcC and convert to h264-annex-b
      if (m_hints.extradata)
      {
        m_bitstream = new CBitstreamConverter;
        if (!m_bitstream->Open(m_hints.codec, (uint8_t*)m_hints.extradata, m_hints.extrasize, true))
        {
          SAFE_DELETE(m_bitstream);
        }
      }
      break;
    case AV_CODEC_ID_HEVC:
      m_mime = "video/hevc";
      m_formatname = "amc-h265";
      // check for hevc-hvcC and convert to h265-annex-b
      if (m_hints.extradata)
      {
        m_bitstream = new CBitstreamConverter;
        if (!m_bitstream->Open(m_hints.codec, (uint8_t*)m_hints.extradata, m_hints.extrasize, true))
        {
          SAFE_DELETE(m_bitstream);
        }
      }
      break;
    case AV_CODEC_ID_WMV3:
      if (m_hints.extrasize == 4 || m_hints.extrasize == 5)
      {
        // Convert to SMPTE 421M-2006 Annex-L
        static char annexL_hdr1[] = {0x8e, 0x01, 0x00, 0xc5, 0x04, 0x00, 0x00, 0x00};
        static char annexL_hdr2[] = {0x0c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
        free(m_hints.extradata);
        m_hints.extrasize = 36;
        m_hints.extradata = malloc(m_hints.extrasize);

        unsigned int offset = 0;
        char buf[4];
        memcpy(m_hints.extradata, annexL_hdr1, sizeof(annexL_hdr1));
        offset += sizeof(annexL_hdr1);
        memcpy(&((char *)(m_hints.extradata))[offset], hints.extradata, 4);
        offset += 4;
        BS_WL32(buf, hints.height);
        memcpy(&((char *)(m_hints.extradata))[offset], buf, 4);
        offset += 4;
        BS_WL32(buf, hints.width);
        memcpy(&((char *)(m_hints.extradata))[offset], buf, 4);
        offset += 4;
        memcpy(&((char *)(m_hints.extradata))[offset], annexL_hdr2, sizeof(annexL_hdr2));
      }

      m_mime = "video/x-ms-wmv";
      m_formatname = "amc-wmv";
      break;
    case AV_CODEC_ID_VC1:
    {
      if (m_hints.extrasize < 16)
        goto FAIL;

      // Reduce extradata to first SEQ header
      unsigned int seq_offset = 0;
      for (; seq_offset <= m_hints.extrasize-4; ++seq_offset)
      {
        char *ptr = &((char*)m_hints.extradata)[seq_offset];
        if (ptr[0] == 0x00 && ptr[1] == 0x00 && ptr[2] == 0x01 && ptr[3] == 0x0f)
          break;
      }
      if (seq_offset > m_hints.extrasize-4)
        goto FAIL;

      if (seq_offset)
      {
        free(m_hints.extradata);
        m_hints.extrasize -= seq_offset;
        m_hints.extradata = malloc(m_hints.extrasize);
        memcpy(m_hints.extradata, &((char *)(hints.extradata))[seq_offset], m_hints.extrasize);
      }

      m_mime = "video/wvc1";
      m_formatname = "amc-vc1";
      break;
    }
    default:
      CLog::Log(LOGDEBUG, "CDVDVideoCodecAndroidMediaCodec::Open Unknown hints.codec(%d)", hints.codec);
      goto FAIL;
      break;
  }

  if (m_crypto)
    delete m_crypto;

  if (m_hints.cryptoSession)
  {
    CLog::Log(LOGDEBUG, "CDVDVideoCodecAndroidMediaCodec::Open Initializing MediaCrypto");

    CJNIUUID uuid(jni::jhobject(NULL));
    if (m_hints.cryptoSession->keySystem == CRYPTO_SESSION_SYSTEM_WIDEVINE)
      uuid = CJNIUUID(0xEDEF8BA979D64ACEULL, 0xA3C827DCD51D21EDULL);
    else if (m_hints.cryptoSession->keySystem == CRYPTO_SESSION_SYSTEM_PLAYREADY)
      uuid = CJNIUUID(0x9A04F07998404286ULL, 0xAB92E65BE0885F95ULL);
    else
    {
      CLog::Log(LOGERROR, "CDVDVideoCodecAndroidMediaCodec::Open Unsupported crypto-keysystem %u", m_hints.cryptoSession->keySystem);
      goto FAIL;
    }

    m_crypto = new CJNIMediaCrypto(uuid, std::vector<char>(m_hints.cryptoSession->sessionId, m_hints.cryptoSession->sessionId + m_hints.cryptoSession->sessionIdSize));

    if (xbmc_jnienv()->ExceptionCheck())
    {
      CLog::Log(LOGERROR, "MediaCrypto::ExceptionCheck: <init>");
      xbmc_jnienv()->ExceptionDescribe();
      xbmc_jnienv()->ExceptionClear();
      goto FAIL;
    }
  }
  else
    m_crypto = new CJNIMediaCrypto(jni::jhobject(NULL));

  if (m_render_surface)
  {
    m_videosurface = CXBMCApp::get()->getVideoViewSurface();
    if (!m_videosurface)
      goto FAIL;
  }

  // CJNIMediaCodec::createDecoderByXXX doesn't handle errors nicely,
  // it crashes if the codec isn't found. This is fixed in latest AOSP,
  // but not in current 4.1 devices. So 1st search for a matching codec, then create it.
  m_colorFormat = -1;
  num_codecs = CJNIMediaCodecList::getCodecCount();
  needSecureDecoder = m_crypto->requiresSecureDecoderComponent(m_mime);

  for (int i = 0; i < num_codecs; i++)
  {
    CJNIMediaCodecInfo codec_info = CJNIMediaCodecList::getCodecInfoAt(i);
    if (codec_info.isEncoder())
      continue;

    m_codecname = codec_info.getName();
    if (IsBlacklisted(m_codecname))
      continue;

    if (needSecureDecoder)
      m_codecname += ".secure";

    CJNIMediaCodecInfoCodecCapabilities codec_caps = codec_info.getCapabilitiesForType(m_mime);
    if (xbmc_jnienv()->ExceptionCheck())
    {
      // Unsupported type?
      xbmc_jnienv()->ExceptionClear();
      continue;
    }

    std::vector<int> color_formats = codec_caps.colorFormats();

    std::vector<std::string> types = codec_info.getSupportedTypes();
    // return the 1st one we find, that one is typically 'the best'
    for (size_t j = 0; j < types.size(); ++j)
    {
      if (types[j] == m_mime)
      {
        m_codec = std::shared_ptr<CJNIMediaCodec>(new CJNIMediaCodec(CJNIMediaCodec::createByCodecName(m_codecname)));

        // clear any jni exceptions, jni gets upset if we do not.
        if (xbmc_jnienv()->ExceptionCheck())
        {
          CLog::Log(LOGERROR, "CDVDVideoCodecAndroidMediaCodec::Open ExceptionCheck");
          xbmc_jnienv()->ExceptionClear();
          m_codec.reset();
          continue;
        }

        for (size_t k = 0; k < color_formats.size(); ++k)
        {
          CLog::Log(LOGDEBUG, "CDVDVideoCodecAndroidMediaCodec::Open "
            "m_codecname(%s), colorFormat(%d)", m_codecname.c_str(), color_formats[k]);
          if (IsSupportedColorFormat(color_formats[k]))
            m_colorFormat = color_formats[k]; // Save color format for initial output configuration
        }
        break;
      }
    }
    if (m_codec)
      break;
  }
  if (!m_codec)
  {
    CLog::Log(LOGERROR, "CDVDVideoCodecAndroidMediaCodec:: Failed to create Android MediaCodec");
    goto FAIL;
  }

  // blacklist of devices that cannot surface render.
  m_render_sw = CanSurfaceRenderBlackList(m_codecname) || g_advancedSettings.m_mediacodecForceSoftwareRendering;
  if (m_render_sw)
  {
    if (m_colorFormat == -1)
    {
      CLog::Log(LOGERROR, "CDVDVideoCodecAndroidMediaCodec:: No supported color format");
      goto FAIL;
    }
    m_render_surface = false;
  }

  // setup a YUV420P DVDVideoPicture buffer.
  // first make sure all properties are reset.
  memset(&m_videobuffer, 0x00, sizeof(DVDVideoPicture));

  m_videobuffer.dts = DVD_NOPTS_VALUE;
  m_videobuffer.pts = DVD_NOPTS_VALUE;
  m_videobuffer.color_range  = 0;
  m_videobuffer.color_matrix = 4;
  m_videobuffer.iFlags  = DVP_FLAG_ALLOCATED;
  m_videobuffer.iWidth  = m_hints.width;
  m_videobuffer.iHeight = m_hints.height;
  // these will get reset to crop values later
  m_videobuffer.iDisplayWidth  = m_hints.width;
  m_videobuffer.iDisplayHeight = m_hints.height;

  if (!ConfigureMediaCodec())
    goto FAIL;

  CLog::Log(LOGINFO, "CDVDVideoCodecAndroidMediaCodec:: "
    "Open Android MediaCodec %s", m_codecname.c_str());

  m_opened = true;

  m_processInfo.SetVideoDecoderName(m_formatname, true );
  m_processInfo.SetVideoPixelFormat(m_render_surface ? "Surface" : (m_render_sw ? "YUV" : "EGL"));
  m_processInfo.SetVideoDimensions(m_hints.width, m_hints.height);
  m_processInfo.SetVideoDeintMethod("hardware");
  m_processInfo.SetVideoDAR(m_hints.aspect);

  if (m_hints.cryptoSession)
    SAFE_DELETE(m_bitstream);

  return true;

FAIL:
  m_InstanceGuard.exchange(false);
  if (m_codec)
    m_codec.reset();
  SAFE_DELETE(m_bitstream);

  return false;
}

void CDVDVideoCodecAndroidMediaCodec::Dispose()
{
  if (!m_opened)
    return;

  // invalidate any inflight outputbuffers
  FlushInternal();

  // clear m_videobuffer bits
  if (m_render_sw)
  {
    free(m_videobuffer.data[0]), m_videobuffer.data[0] = NULL;
    free(m_videobuffer.data[1]), m_videobuffer.data[1] = NULL;
    free(m_videobuffer.data[2]), m_videobuffer.data[2] = NULL;
  }
  m_videobuffer.iFlags = 0;
  // m_videobuffer.mediacodec is unioned with m_videobuffer.data[0]
  // so be very careful when and how you touch it.
  m_videobuffer.mediacodec = NULL;

  if (m_codec)
  {
    m_codec->stop();
    m_state = MEDIACODEC_STATE_UNINITIALIZED;
    m_codec->release();
    m_codec.reset();
    if (xbmc_jnienv()->ExceptionCheck())
      xbmc_jnienv()->ExceptionClear();
  }
  ReleaseSurfaceTexture();

  m_InstanceGuard.exchange(false);

  //TODO: investigate why this one crashes
  //if (m_render_surface)
  //  CXBMCApp::get()->clearVideoView();

  SAFE_DELETE(m_bitstream);

  m_opened = false;
}

bool CDVDVideoCodecAndroidMediaCodec::AddData(const DemuxPacket &packet)
{
  if (!m_opened)
    return 0;

  double pts(packet.pts), dts(packet.dts);

  if (g_advancedSettings.CanLogComponent(LOGVIDEO))
    CLog::Log(LOGDEBUG, "CDVDVideoCodecAndroidMediaCodec::AddData dts:%0.2lf pts:%0.2lf sz:%d indexBuffer:%d current state (%d)", dts, pts, packet.iSize, m_indexInputBuffer, m_state);
  else if (m_state != MEDIACODEC_STATE_RUNNING)
    CLog::Log(LOGDEBUG, "CDVDVideoCodecAndroidMediaCodec::AddData current state (%d)", m_state);

  if (m_hints.ptsinvalid)
    pts = DVD_NOPTS_VALUE;

  uint8_t *pData(packet.pData);
  int iSize(packet.iSize);

  if (m_state == MEDIACODEC_STATE_ENDOFSTREAM)
  {
    // We received a packet but already reached EOS. Flush...
    FlushInternal();
    m_codec->flush();
    m_state = MEDIACODEC_STATE_FLUSHED;
  }

  if (pData && iSize)
  {
    if (m_indexInputBuffer >= 0)
    {
      if (m_state == MEDIACODEC_STATE_FLUSHED)
        m_state = MEDIACODEC_STATE_RUNNING;
      if (!(m_state == MEDIACODEC_STATE_FLUSHED || m_state == MEDIACODEC_STATE_RUNNING))
        CLog::Log(LOGERROR, "CDVDVideoCodecAndroidMediaCodec::AddData: Wrong state (%d)", m_state);

      // we have an input buffer, fill it.
      if (pData && m_bitstream)
      {
        m_bitstream->Convert(pData, iSize);
        iSize = m_bitstream->GetConvertSize();
        pData = m_bitstream->GetConvertBuffer();
      }
      CJNIByteBuffer buffer = m_codec->getInputBuffer(m_indexInputBuffer);
      int size = buffer.capacity();
      if (iSize + 4 > size)
      {
        CLog::Log(LOGERROR, "CDVDVideoCodecAndroidMediaCodec::AddData, iSize(%d) > size(%d)", iSize, size);
        return false;
      }

      CJNIMediaCodecCryptoInfo *cryptoInfo(0);
      if (!!m_crypto->get_raw() && packet.cryptoInfo)
      {
        cryptoInfo = new CJNIMediaCodecCryptoInfo();
        cryptoInfo->set(
          packet.cryptoInfo->numSubSamples,
          std::vector<int>(packet.cryptoInfo->clearBytes, packet.cryptoInfo->clearBytes + packet.cryptoInfo->numSubSamples),
          std::vector<int>(packet.cryptoInfo->cipherBytes, packet.cryptoInfo->cipherBytes + packet.cryptoInfo->numSubSamples),
          std::vector<char>(packet.cryptoInfo->kid, packet.cryptoInfo->kid + 16),
          std::vector<char>(packet.cryptoInfo->iv, packet.cryptoInfo->iv + 16),
          CJNIMediaCodec::CRYPTO_MODE_AES_CTR);
      }

      // fetch a pointer to the ByteBuffer backing store
      uint8_t *dst_ptr = (uint8_t*)xbmc_jnienv()->GetDirectBufferAddress(buffer.get_raw());
      if (dst_ptr)
      {
        // Codec specifics
        switch(m_hints.codec)
        {
          case AV_CODEC_ID_VC1:
          {
            if (iSize >= 4 && pData[0] == 0x00 && pData[1] == 0x00 && pData[2] == 0x01 && (pData[3] == 0x0d || pData[3] == 0x0f))
              memcpy(dst_ptr, pData, iSize);
            else
            {
              dst_ptr[0] = 0x00;
              dst_ptr[1] = 0x00;
              dst_ptr[2] = 0x01;
              dst_ptr[3] = 0x0d;
              memcpy(dst_ptr+4, pData, iSize);
              iSize += 4;
            }

            break;
          }

          default:
            memcpy(dst_ptr, pData, iSize);
            break;
        }
      }

      // Translate from VideoPlayer dts/pts to MediaCodec pts,
      // pts WILL get re-ordered by MediaCodec if needed.
      // Do not try to pass pts as a unioned double/int64_t,
      // some android devices will diddle with presentationTimeUs
      // and you will get NaN back and VideoPlayerVideo will barf.
      int64_t presentationTimeUs = 0;
      if (pts != DVD_NOPTS_VALUE)
        presentationTimeUs = pts;
      else if (dts != DVD_NOPTS_VALUE)
        presentationTimeUs = dts;

      int flags = 0;
      int offset = 0;

      if (!cryptoInfo)
        m_codec->queueInputBuffer(m_indexInputBuffer, offset, iSize, presentationTimeUs, flags);
      else
      {
        m_codec->queueSecureInputBuffer(m_indexInputBuffer, offset, *cryptoInfo, presentationTimeUs, flags);
        delete cryptoInfo;
      }
      m_indexInputBuffer = -1;
      // clear any jni exceptions, jni gets upset if we do not.
      if (xbmc_jnienv()->ExceptionCheck())
      {
        CLog::Log(LOGERROR, "CDVDVideoCodecAndroidMediaCodec::AddData ExceptionCheck");
        xbmc_jnienv()->ExceptionClear();
      }
    }
    else
      return false;
  }
  return true;
}

void CDVDVideoCodecAndroidMediaCodec::Reset()
{
  if (!m_opened)
    return;

  if (m_codec)
  {
    // flush all outputbuffers inflight, they will
    // become invalid on m_codec->flush and generate
    // a spew of java exceptions if used
    FlushInternal();

    // now we can flush the actual MediaCodec object
    CLog::Log(LOGDEBUG, "CDVDVideoCodecAndroidMediaCodec::Reset Current state (%d)", m_state);
    m_codec->flush();
    m_state = MEDIACODEC_STATE_FLUSHED;
    if (xbmc_jnienv()->ExceptionCheck())
    {
      CLog::Log(LOGERROR, "CDVDVideoCodecAndroidMediaCodec::Reset ExceptionCheck");
      xbmc_jnienv()->ExceptionClear();
    }

    // Invalidate our local DVDVideoPicture bits
    m_videobuffer.pts = DVD_NOPTS_VALUE;
    if (!m_render_sw)
      m_videobuffer.mediacodec = NULL;

    m_indexInputBuffer = -1;
    m_checkForPicture = false;
  }
}

bool CDVDVideoCodecAndroidMediaCodec::Reconfigure(CDVDStreamInfo &hints)
{
  CLog::Log(LOGDEBUG, "CDVDVideoCodecAndroidMediaCodec::Reconfigure called");
  return false;
}

CDVDVideoCodec::VCReturn CDVDVideoCodecAndroidMediaCodec::GetPicture(DVDVideoPicture* pDvdVideoPicture)
{
  if (!m_opened)
    return VC_NONE;

  if (m_checkForPicture)
  {
    int retgp = GetOutputPicture();
    if (retgp > 0)
    {
      m_checkForPicture = false;
      m_noPictureLoop = 0;
      *pDvdVideoPicture = m_videobuffer;

      // Invalidate our local DVDVideoPicture bits
      m_videobuffer.pts = DVD_NOPTS_VALUE;
      if (!m_render_sw)
        m_videobuffer.mediacodec = NULL;

      if (g_advancedSettings.CanLogComponent(LOGVIDEO))
        CLog::Log(LOGDEBUG, "CDVDVideoCodecAndroidMediaCodec::GetPicture pts:%0.4lf", pDvdVideoPicture->pts);

      return VC_PICTURE;
    }
    else if (retgp == -1 || ((m_codecControlFlags & DVD_CODEC_CTRL_DRAIN)!=0 && ++m_noPictureLoop == 10))  // EOS
    {
      m_state = MEDIACODEC_STATE_ENDOFSTREAM;
      m_noPictureLoop = 0;
      return VC_NONE;
    }
  }

  m_checkForPicture = true;

  // try to fetch an input buffer
  if (m_indexInputBuffer < 0)
    m_indexInputBuffer = m_codec->dequeueInputBuffer(5000 /*timout*/);

  if (xbmc_jnienv()->ExceptionCheck())
  {
    CLog::Log(LOGERROR, "CDVDVideoCodecAndroidMediaCodec::Decode ExceptionCheck");
    xbmc_jnienv()->ExceptionDescribe();
    xbmc_jnienv()->ExceptionClear();
    return VC_ERROR;
  }
  else if (m_indexInputBuffer >= 0)
    return VC_BUFFER;

  return VC_NONE;
}

bool CDVDVideoCodecAndroidMediaCodec::ClearPicture(DVDVideoPicture* pDvdVideoPicture)
{
  if (pDvdVideoPicture->format == RENDER_FMT_MEDIACODEC || pDvdVideoPicture->format == RENDER_FMT_MEDIACODECSURFACE)
    SAFE_RELEASE(pDvdVideoPicture->mediacodec);
  memset(pDvdVideoPicture, 0x00, sizeof(DVDVideoPicture));

  return true;
}

void CDVDVideoCodecAndroidMediaCodec::SetCodecControl(int flags)
{
  if (m_codecControlFlags != flags)
  {
    if (g_advancedSettings.CanLogComponent(LOGVIDEO))
      CLog::Log(LOGDEBUG, "%s %x->%x",  __func__, m_codecControlFlags, flags);
    m_codecControlFlags = flags;

    m_drop = (flags & DVD_CODEC_CTRL_DROP) != 0;

    if (m_drop)
      m_videobuffer.iFlags |= DVP_FLAG_DROPPED;
    else
      m_videobuffer.iFlags &= ~DVP_FLAG_DROPPED;
  }
}

unsigned CDVDVideoCodecAndroidMediaCodec::GetAllowedReferences()
{
  return 4;
}

void CDVDVideoCodecAndroidMediaCodec::FlushInternal()
{
  // invalidate any existing inflight buffers and create
  // new ones to match the number of output buffers

  if (m_render_sw)
    return;

  for (size_t i = 0; i < m_inflight.size(); i++)
  {
    m_inflight[i]->Validate(false);
    m_inflight[i]->Release();
  }
  m_inflight.clear();

}

bool CDVDVideoCodecAndroidMediaCodec::ConfigureMediaCodec(void)
{
  // setup a MediaFormat to match the video content,
  // used by codec during configure
  CJNIMediaFormat mediaformat = CJNIMediaFormat::createVideoFormat(
    m_mime.c_str(), m_hints.width, m_hints.height);
  // some android devices forget to default the demux input max size
  mediaformat.setInteger(CJNIMediaFormat::KEY_MAX_INPUT_SIZE, 0);

  if (CJNIBase::GetSDKVersion() >= 23 && m_render_surface)
  {
    // Handle rotation
    mediaformat.setInteger(CJNIMediaFormat::KEY_ROTATION, m_hints.orientation);
  }


  // handle codec extradata
  if (m_hints.extrasize)
  {
    size_t size = m_hints.extrasize;
    void  *src_ptr = m_hints.extradata;
    if (m_bitstream)
    {
      size = m_bitstream->GetExtraSize();
      src_ptr = m_bitstream->GetExtraData();
    }
    // Allocate a byte buffer via allocateDirect in java instead of NewDirectByteBuffer,
    // since the latter doesn't allocate storage of its own, and we don't know how long
    // the codec uses the buffer.
    CJNIByteBuffer bytebuffer = CJNIByteBuffer::allocateDirect(size);
    void *dts_ptr = xbmc_jnienv()->GetDirectBufferAddress(bytebuffer.get_raw());
    memcpy(dts_ptr, src_ptr, size);
    // codec will automatically handle buffers as extradata
    // using entries with keys "csd-0", "csd-1", etc.
    mediaformat.setByteBuffer("csd-0", bytebuffer);
  }

  InitSurfaceTexture();

  // configure and start the codec.
  // use the MediaFormat that we have setup.
  // use a null Surface, we will extract the video picture data manually.
  int flags = 0;
  // our jni gets upset if we do this a different way, do not mess with it.
  if (m_render_sw)
  {
    CJNISurface surface(jni::jhobject(NULL));
    m_codec->configure(mediaformat, surface, *m_crypto, flags);
  }
  else
  {
    if (m_render_surface)
      m_codec->configure(mediaformat, m_videosurface, *m_crypto, flags);
    else
      m_codec->configure(mediaformat, *m_surface, *m_crypto, flags);
  }
  // always, check/clear jni exceptions.
  if (xbmc_jnienv()->ExceptionCheck())
  {
    CLog::Log(LOGERROR, "CDVDVideoCodecAndroidMediaCodec::ExceptionCheck: configure");
    xbmc_jnienv()->ExceptionDescribe();
    xbmc_jnienv()->ExceptionClear();
    return false;
  }
  m_state = MEDIACODEC_STATE_CONFIGURED;


  m_codec->start();

  // always, check/clear jni exceptions.
  if (xbmc_jnienv()->ExceptionCheck())
  {
    CLog::Log(LOGERROR, "CDVDVideoCodecAndroidMediaCodec::ExceptionCheck: start");
    xbmc_jnienv()->ExceptionDescribe();
    xbmc_jnienv()->ExceptionClear();
    return false;
  }
  m_state = MEDIACODEC_STATE_FLUSHED;

  // There is no guarantee we'll get an INFO_OUTPUT_FORMAT_CHANGED (up to Android 4.3)
  // Configure the output with defaults
  ConfigureOutputFormat(&mediaformat);

  return true;
}

int CDVDVideoCodecAndroidMediaCodec::GetOutputPicture(void)
{
  int rtn = 0;

  int64_t timeout_us = 10000;
  CJNIMediaCodecBufferInfo bufferInfo;
  int index = m_codec->dequeueOutputBuffer(bufferInfo, timeout_us);
  if (xbmc_jnienv()->ExceptionCheck())
  {
    CLog::Log(LOGERROR, "CDVDVideoCodecAndroidMediaCodec::GetOutputPicture ExceptionCheck; dequeueOutputBuffer");
    xbmc_jnienv()->ExceptionDescribe();
    xbmc_jnienv()->ExceptionClear();
    return -2;
  }
  if (index >= 0)
  {
    int64_t pts= bufferInfo.presentationTimeUs();
    m_videobuffer.dts = DVD_NOPTS_VALUE;
    m_videobuffer.pts = DVD_NOPTS_VALUE;
    if (pts != AV_NOPTS_VALUE)
      m_videobuffer.pts = pts;

    int flags = bufferInfo.flags();
    if (flags & CJNIMediaCodec::BUFFER_FLAG_SYNC_FRAME)
      CLog::Log(LOGDEBUG, "CDVDVideoCodecAndroidMediaCodec:: BUFFER_FLAG_SYNC_FRAME");

    if (flags & CJNIMediaCodec::BUFFER_FLAG_CODEC_CONFIG)
      CLog::Log(LOGDEBUG, "CDVDVideoCodecAndroidMediaCodec:: BUFFER_FLAG_CODEC_CONFIG");

    if (flags & CJNIMediaCodec::BUFFER_FLAG_END_OF_STREAM)
    {
      CLog::Log(LOGDEBUG, "CDVDVideoCodecAndroidMediaCodec:: BUFFER_FLAG_END_OF_STREAM");
      m_codec->releaseOutputBuffer(index, false);
      if (xbmc_jnienv()->ExceptionCheck())
      {
        CLog::Log(LOGERROR, "CDVDVideoCodecAndroidMediaCodec::GetOutputPicture ExceptionCheck: releaseOutputBuffer");
        xbmc_jnienv()->ExceptionDescribe();
        xbmc_jnienv()->ExceptionClear();
        return -2;
      }
      return -1;
    }

    if (m_drop)
    {
      m_codec->releaseOutputBuffer(index, false);
      if (xbmc_jnienv()->ExceptionCheck())
      {
        CLog::Log(LOGERROR, "CDVDVideoCodecAndroidMediaCodec::GetOutputPicture ExceptionCheck: releaseOutputBuffer");
        xbmc_jnienv()->ExceptionDescribe();
        xbmc_jnienv()->ExceptionClear();
        return -2;
      }
      m_videobuffer.mediacodec = nullptr;
      return 1;
    }

    if (!m_render_sw)
    {
      size_t i = 0;
      for (; i < m_inflight.size(); ++i)
      {
        if (m_inflight[i]->GetIndex() == index)
          break;
      }
      if (i == m_inflight.size())
        m_inflight.push_back(
          new CDVDMediaCodecInfo(index, m_textureId, m_codec, m_surfaceTexture, m_frameAvailable)
        );
      m_videobuffer.mediacodec = m_inflight[i]->Retain();
      m_videobuffer.mediacodec->Validate(true);
    }
    else
    {
      int size = bufferInfo.size();
      int offset = bufferInfo.offset();

      CJNIByteBuffer buffer = m_codec->getOutputBuffer(index);;
      if (!buffer.isDirect())
        CLog::Log(LOGWARNING, "CDVDVideoCodecAndroidMediaCodec:: buffer.isDirect == false");

      if (!buffer.isDirect())
        CLog::Log(LOGWARNING, "CDVDVideoCodecAndroidMediaCodec:: m_output[index].isDirect == false");

      if (size && buffer.capacity())
      {
        uint8_t *src_ptr = (uint8_t*)xbmc_jnienv()->GetDirectBufferAddress(buffer.get_raw());
        src_ptr += offset;

        int loop_end = 0;
        if (m_videobuffer.format == RENDER_FMT_NV12)
          loop_end = 2;
        else if (m_videobuffer.format == RENDER_FMT_YUV420P)
          loop_end = 3;

        for (int i = 0; i < loop_end; i++)
        {
          uint8_t *src   = src_ptr + m_src_offset[i];
          int src_stride = m_src_stride[i];
          uint8_t *dst   = m_videobuffer.data[i];
          int dst_stride = m_videobuffer.iLineSize[i];

          int height = m_videobuffer.iHeight;
          if (i > 0)
            height = (m_videobuffer.iHeight + 1) / 2;

          if (src_stride == dst_stride)
            memcpy(dst, src, dst_stride * height);
          else
            for (int j = 0; j < height; j++, src += src_stride, dst += dst_stride)
              memcpy(dst, src, dst_stride);
        }
      }
      m_codec->releaseOutputBuffer(index, false);
    }

    // always, check/clear jni exceptions.
    if (xbmc_jnienv()->ExceptionCheck())
      xbmc_jnienv()->ExceptionClear();

    rtn = 1;
  }
  else if (index == CJNIMediaCodec::INFO_OUTPUT_FORMAT_CHANGED)
  {
    CJNIMediaFormat mediaformat = m_codec->getOutputFormat();
    if (xbmc_jnienv()->ExceptionCheck())
    {
      xbmc_jnienv()->ExceptionDescribe();
      xbmc_jnienv()->ExceptionClear();
    }
    ConfigureOutputFormat(&mediaformat);
  }
  else if (index == CJNIMediaCodec::INFO_TRY_AGAIN_LATER || index == CJNIMediaCodec::INFO_OUTPUT_BUFFERS_CHANGED)
  {
    // ignore
    rtn = 0;
  }
  else
  {
    // we should never get here
    CLog::Log(LOGERROR, "CDVDVideoCodecAndroidMediaCodec::GetOutputPicture unknown index(%d)", index);
    rtn = -2;
  }

  return rtn;
}

void CDVDVideoCodecAndroidMediaCodec::ConfigureOutputFormat(CJNIMediaFormat* mediaformat)
{
  int width       = 0;
  int height      = 0;
  int stride      = 0;
  int slice_height= 0;
  int color_format= 0;
  int crop_left   = 0;
  int crop_top    = 0;
  int crop_right  = 0;
  int crop_bottom = 0;

  if (mediaformat->containsKey("width"))
    width       = mediaformat->getInteger("width");
  if (mediaformat->containsKey("height"))
    height      = mediaformat->getInteger("height");
  if (mediaformat->containsKey("stride"))
    stride      = mediaformat->getInteger("stride");
  if (mediaformat->containsKey("slice-height"))
    slice_height= mediaformat->getInteger("slice-height");
  if (mediaformat->containsKey("color-format"))
    color_format= mediaformat->getInteger("color-format");
  if (mediaformat->containsKey("crop-left"))
    crop_left   = mediaformat->getInteger("crop-left");
  if (mediaformat->containsKey("crop-top"))
    crop_top    = mediaformat->getInteger("crop-top");
  if (mediaformat->containsKey("crop-right"))
    crop_right  = mediaformat->getInteger("crop-right");
  if (mediaformat->containsKey("crop-bottom"))
    crop_bottom = mediaformat->getInteger("crop-bottom");

  // clear any jni exceptions
  if (xbmc_jnienv()->ExceptionCheck())
    xbmc_jnienv()->ExceptionClear();

  CLog::Log(LOGDEBUG, "CDVDVideoCodecAndroidMediaCodec:: "
    "width(%d), height(%d), stride(%d), slice-height(%d), color-format(%d)",
    width, height, stride, slice_height, color_format);
  CLog::Log(LOGDEBUG, "CDVDVideoCodecAndroidMediaCodec:: "
    "crop-left(%d), crop-top(%d), crop-right(%d), crop-bottom(%d)",
    crop_left, crop_top, crop_right, crop_bottom);

  if (!m_render_sw)
  {
    if (m_render_surface)
    {
      CLog::Log(LOGDEBUG, "CDVDVideoCodecAndroidMediaCodec:: Multi-Surface Rendering");
      m_videobuffer.format = RENDER_FMT_MEDIACODECSURFACE;
    }
    else
    {
      CLog::Log(LOGDEBUG, "CDVDVideoCodecAndroidMediaCodec:: Direct Surface Rendering");
      m_videobuffer.format = RENDER_FMT_MEDIACODEC;
    }
  }
  else
  {
    // Android device quirks and fixes

    // Samsung Quirk: ignore width/height/stride/slice: http://code.google.com/p/android/issues/detail?id=37768#c3
    if (strstr(m_codecname.c_str(), "OMX.SEC.avc.dec") != NULL || strstr(m_codecname.c_str(), "OMX.SEC.avcdec") != NULL)
    {
      width = stride = m_hints.width;
      height = slice_height = m_hints.height;
    }
    // No color-format? Initialize with the one we detected as valid earlier
    if (color_format == 0)
      color_format = m_colorFormat;
    if (stride <= width)
      stride = width;
    if (!crop_right)
      crop_right = width-1;
    if (!crop_bottom)
      crop_bottom = height-1;
    if (slice_height <= height)
    {
      slice_height = height;
      if (color_format == CJNIMediaCodecInfoCodecCapabilities::COLOR_FormatYUV420Planar)
      {
        // NVidia Tegra 3 on Nexus 7 does not set slice_heights
        if (strstr(m_codecname.c_str(), "OMX.Nvidia.") != NULL)
        {
          slice_height = (((height) + 15) & ~15);
          CLog::Log(LOGDEBUG, "CDVDVideoCodecAndroidMediaCodec:: NVidia Tegra 3 quirk, slice_height(%d)", slice_height);
        }
      }
    }
    if (color_format == CJNIMediaCodecInfoCodecCapabilities::COLOR_TI_FormatYUV420PackedSemiPlanar)
    {
      slice_height -= crop_top / 2;
      // set crop top/left here, since the offset parameter already includes this.
      // if we would ignore the offset parameter in the BufferInfo, we could just keep
      // the original slice height and apply the top/left cropping instead.
      crop_top = 0;
      crop_left = 0;
    }

    // default picture format to none
    for (int i = 0; i < 4; i++)
      m_src_offset[i] = m_src_stride[i] = 0;
    // delete any existing buffers
    for (int i = 0; i < 4; i++)
      free(m_videobuffer.data[i]);

    // setup picture format and data offset vectors
    if (color_format == CJNIMediaCodecInfoCodecCapabilities::COLOR_FormatYUV420Planar)
    {
      CLog::Log(LOGDEBUG, "CDVDVideoCodecAndroidMediaCodec:: COLOR_FormatYUV420Planar");

      // Y plane
      m_src_stride[0] = stride;
      m_src_offset[0] = crop_top * stride;
      m_src_offset[0]+= crop_left;

      // U plane
      m_src_stride[1] = (stride + 1) / 2;
      //  skip over the Y plane
      m_src_offset[1] = slice_height * stride;
      //  crop_top/crop_left divided by two
      //  because one byte of the U/V planes
      //  corresponds to two pixels horizontally/vertically
      m_src_offset[1]+= crop_top  / 2 * m_src_stride[1];
      m_src_offset[1]+= crop_left / 2;

      // V plane
      m_src_stride[2] = (stride + 1) / 2;
      //  skip over the Y plane
      m_src_offset[2] = slice_height * stride;
      //  skip over the U plane
      m_src_offset[2]+= ((slice_height + 1) / 2) * ((stride + 1) / 2);
      //  crop_top/crop_left divided by two
      //  because one byte of the U/V planes
      //  corresponds to two pixels horizontally/vertically
      m_src_offset[2]+= crop_top  / 2 * m_src_stride[2];
      m_src_offset[2]+= crop_left / 2;

      m_videobuffer.iLineSize[0] =  width;         // Y
      m_videobuffer.iLineSize[1] = (width + 1) /2; // U
      m_videobuffer.iLineSize[2] = (width + 1) /2; // V
      m_videobuffer.iLineSize[3] = 0;

      unsigned int iPixels = width * height;
      unsigned int iChromaPixels = iPixels/4;
      m_videobuffer.data[0] = (uint8_t*)malloc(16 + iPixels);
      m_videobuffer.data[1] = (uint8_t*)malloc(16 + iChromaPixels);
      m_videobuffer.data[2] = (uint8_t*)malloc(16 + iChromaPixels);
      m_videobuffer.data[3] = NULL;
      m_videobuffer.format  = RENDER_FMT_YUV420P;
    }
    else if (color_format == CJNIMediaCodecInfoCodecCapabilities::COLOR_FormatYUV420SemiPlanar
          || color_format == CJNIMediaCodecInfoCodecCapabilities::COLOR_QCOM_FormatYUV420SemiPlanar
          || color_format == CJNIMediaCodecInfoCodecCapabilities::COLOR_TI_FormatYUV420PackedSemiPlanar
          || color_format == CJNIMediaCodecInfoCodecCapabilities::OMX_QCOM_COLOR_FormatYVU420SemiPlanarInterlace)

    {
      CLog::Log(LOGDEBUG, "CDVDVideoCodecAndroidMediaCodec:: COLOR_FormatYUV420SemiPlanar");

      // Y plane
      m_src_stride[0] = stride;
      m_src_offset[0] = crop_top * stride;
      m_src_offset[0]+= crop_left;

      // UV plane
      m_src_stride[1] = stride;
      //  skip over the Y plane
      m_src_offset[1] = slice_height * stride;
      m_src_offset[1]+= crop_top * stride;
      m_src_offset[1]+= crop_left;

      m_videobuffer.iLineSize[0] = width;  // Y
      m_videobuffer.iLineSize[1] = width;  // UV
      m_videobuffer.iLineSize[2] = 0;
      m_videobuffer.iLineSize[3] = 0;

      unsigned int iPixels = width * height;
      unsigned int iChromaPixels = iPixels;
      m_videobuffer.data[0] = (uint8_t*)malloc(16 + iPixels);
      m_videobuffer.data[1] = (uint8_t*)malloc(16 + iChromaPixels);
      m_videobuffer.data[2] = NULL;
      m_videobuffer.data[3] = NULL;
      m_videobuffer.format  = RENDER_FMT_NV12;
    }
    else
    {
      CLog::Log(LOGERROR, "CDVDVideoCodecAndroidMediaCodec:: Fixme unknown color_format(%d)", color_format);
      return;
    }
  }

  if (width)
    m_videobuffer.iWidth  = width;
  if (height)
    m_videobuffer.iHeight = height;

  // picture display width/height include the cropping.
  m_videobuffer.iDisplayWidth  = crop_right  + 1 - crop_left;
  m_videobuffer.iDisplayHeight = crop_bottom + 1 - crop_top;
  if (m_hints.aspect > 1.0 && !m_hints.forced_aspect)
  {
    m_videobuffer.iDisplayWidth  = ((int)lrint(m_videobuffer.iHeight * m_hints.aspect)) & ~3;
    if (m_videobuffer.iDisplayWidth > m_videobuffer.iWidth)
    {
      m_videobuffer.iDisplayWidth  = m_videobuffer.iWidth;
      m_videobuffer.iDisplayHeight = ((int)lrint(m_videobuffer.iWidth / m_hints.aspect)) & ~3;
    }
  }
}

void CDVDVideoCodecAndroidMediaCodec::CallbackInitSurfaceTexture(void *userdata)
{
  CDVDVideoCodecAndroidMediaCodec *ctx = static_cast<CDVDVideoCodecAndroidMediaCodec*>(userdata);
  ctx->InitSurfaceTexture();
}

void CDVDVideoCodecAndroidMediaCodec::InitSurfaceTexture(void)
{
  if (m_render_sw || m_render_surface)
    return;

  // We MUST create the GLES texture on the main thread
  // to match where the valid GLES context is located.
  // It would be nice to move this out of here, we would need
  // to create/fetch/create from g_RenderManager. But g_RenderManager
  // does not know we are using MediaCodec until Configure and we
  // we need m_surfaceTexture valid before then. Chicken, meet Egg.
  if (g_application.IsCurrentThread())
  {
    // localize GLuint so we do not spew gles includes in our header
    GLuint texture_id;

    glGenTextures(1, &texture_id);
    glBindTexture(  GL_TEXTURE_EXTERNAL_OES, texture_id);
    glTexParameterf(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(  GL_TEXTURE_EXTERNAL_OES, 0);
    m_textureId = texture_id;

    m_surfaceTexture = std::shared_ptr<CJNISurfaceTexture>(new CJNISurfaceTexture(m_textureId));
    // hook the surfaceTexture OnFrameAvailable callback
    m_frameAvailable = std::shared_ptr<CDVDMediaCodecOnFrameAvailable>(new CDVDMediaCodecOnFrameAvailable(m_surfaceTexture));
    m_surface = new CJNISurface(*m_surfaceTexture);
  }
  else
  {
    ThreadMessageCallback callbackData;
    callbackData.callback = &CallbackInitSurfaceTexture;
    callbackData.userptr  = (void*)this;

    // wait for it.
    CApplicationMessenger::GetInstance().SendMsg(TMSG_CALLBACK, -1, -1, static_cast<void*>(&callbackData));
  }

  return;
}

void CDVDVideoCodecAndroidMediaCodec::ReleaseSurfaceTexture(void)
{
  if (m_render_sw || m_render_surface)
    return;

  // it is safe to delete here even though these items
  // were created in the main thread instance
  SAFE_DELETE(m_surface);
  m_frameAvailable.reset();
  m_surfaceTexture.reset();

  if (m_textureId > 0)
  {
    GLuint texture_id = m_textureId;
    glDeleteTextures(1, &texture_id);
    m_textureId = 0;
  }
}
