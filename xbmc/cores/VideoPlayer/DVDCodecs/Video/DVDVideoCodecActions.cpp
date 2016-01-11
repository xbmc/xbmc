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
// i.e. encoder/decoder components. (android.media.MediaCodec). Requires
// SDK16+ which is 4.1 Jellybean and above.
//

#include "DVDVideoCodecActions.h"


#include "DVDClock.h"
#include "utils/BitstreamConverter.h"
#include "utils/log.h"
#include "utils/ActsUtils.h"
#include "settings/AdvancedSettings.h"

#include "platform/android/activity/XBMCApp.h"
#include "platform/android/jni/MediaCodec.h"
#include "platform/android/jni/MediaCodecList.h"

static bool CanSurfaceRenderBlackList(const std::string &name)
{
  // All devices 'should' be capiable of surface rendering
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
    CJNIMediaCodecInfoCodecCapabilities::COLOR_FormatYUV420SemiPlanar,
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
CDVDVideoCodecActions::CDVDVideoCodecActions(bool surface_render)
: CDVDVideoCodecAndroidMediaCodec(surface_render){
	CLog::Log(LOGDEBUG,"create actvideocodec\n");
}

CDVDVideoCodecActions::~CDVDVideoCodecActions()
{
 	 Dispose();
}

bool CDVDVideoCodecActions::Open(CDVDStreamInfo &hints, CDVDCodecOptions &options)
{
  // mediacodec crashes with null size. Trap this...
  if (!hints.width || !hints.height)
  {
    CLog::Log(LOGERROR, "CDVDVideoCodecActions::Open - %s\n", "null size, cannot handle");
    return false;
  }

  m_drop = false;
  m_hints = hints;

  switch(m_hints.codec)
  {
    case AV_CODEC_ID_MPEG2VIDEO:
      m_mime = "video/mpeg2";
      m_formatname = "acts-mpeg2";
      break;
    case AV_CODEC_ID_MPEG4:
      m_mime = "video/mp4v-es";
      m_formatname = "acts-mpeg4";
      break;
    case AV_CODEC_ID_MSMPEG4V3:
    	m_mime = "video/div3";
      m_formatname = "acts-msmpeg4v3";
    	break;
    case AV_CODEC_ID_H263:
    case AV_CODEC_ID_H263P:
    case AV_CODEC_ID_H263I:
      m_mime = "video/3gpp";
      m_formatname = "acts-h263";
      break;
    case AV_CODEC_ID_FLV1:
      m_mime = "video/flv1";
      m_formatname = "acts-flv1";
      break;;
    case AV_CODEC_ID_VP6:
    case AV_CODEC_ID_VP6F:
    	m_mime = "video/vp6";
      m_formatname = "acts-vp6";
      break;   
    case AV_CODEC_ID_VP8:
      m_mime = "video/x-vnd.on2.vp8";
      m_formatname = "acts-vpX";
      break;
    case AV_CODEC_ID_RV30:
    	m_mime = "video/rv30";
    	m_formatname = "acts-rv30";
    	break;
    case AV_CODEC_ID_RV40:
    	m_mime = "video/rv40";
    	m_formatname = "acts-rv40";
    	break;
    case AV_CODEC_ID_AVS:
    case AV_CODEC_ID_CAVS:
    	if(!acts_support_avs()){
    		CLog::Log(LOGDEBUG,"not support avs hardware decode,use software\n");
    		return false;
    	}
    	m_mime = "video/avs";
      m_formatname = "acts-avs";
    	break;
    case AV_CODEC_ID_H264:
      switch(hints.profile)
      {
        case FF_PROFILE_H264_HIGH_10:
        case FF_PROFILE_H264_HIGH_10_INTRA:
          // No known h/w decoder supporting Hi10P
          return false;
      }
      m_mime = "video/avc";
      m_formatname = "acts-h264";
      // check for h264-avcC and convert to h264-annex-b
      if (m_hints.extradata)
      {
       	CLog::Log(LOGDEBUG,"h264 has extradata\n");
        m_bitstream = new CBitstreamConverter;
        if (!m_bitstream->Open(m_hints.codec, (uint8_t*)m_hints.extradata, m_hints.extrasize, true))
        {
          SAFE_DELETE(m_bitstream);
        }
      }
      break;
    case AV_CODEC_ID_HEVC:
    	if(!acts_support_hevc()){
    		CLog::Log(LOGDEBUG,"not support hevc hardware decode,use software\n");
    		return false;
    	}
      m_mime = "video/hevc";
      m_formatname = "acts-h265";
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
      m_mime = "video/wmv3";
      m_formatname = "acts-wmv3";
      break;
    case AV_CODEC_ID_VC1:
    {
      
      m_mime = "video/vc1";
      m_formatname = "acts-vc1";
      break;
    }
    default:
      CLog::Log(LOGDEBUG, "CDVDVideoCodecActions:: Unknown hints.codec(%d)", hints.codec);
      return false;
      break;
  }

  if (m_render_surface)
  {
    m_videosurface = CXBMCApp::get()->getVideoViewSurface();
    if (!m_videosurface)
      return false;
  }

  if (m_render_surface)
    m_formatname += "(S)";
  

  // CJNIMediaCodec::createDecoderByXXX doesn't handle errors nicely,
  // it crashes if the codec isn't found. This is fixed in latest AOSP,
  // but not in current 4.1 devices. So 1st search for a matching codec, then create it.
  m_colorFormat = -1;
  int num_codecs = CJNIMediaCodecList::getCodecCount();
  for (int i = 0; i < num_codecs; i++)
  {
    CJNIMediaCodecInfo codec_info = CJNIMediaCodecList::getCodecInfoAt(i);
    if (codec_info.isEncoder())
      continue;
    m_codecname = codec_info.getName();
    if (IsBlacklisted(m_codecname))
      continue;

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
          CLog::Log(LOGERROR, "CDVDVideoCodecActions::Open ExceptionCheck");
          xbmc_jnienv()->ExceptionClear();
          m_codec.reset();
          continue;
        }

        for (size_t k = 0; k < color_formats.size(); ++k)
        {
          CLog::Log(LOGDEBUG, "CDVDVideoCodecActions::Open "
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
    CLog::Log(LOGERROR, "CDVDVideoCodecActions:: Failed to create Android MediaCodec");
    SAFE_DELETE(m_bitstream);
    return false;
  }

  // blacklist of devices that cannot surface render.
  m_render_sw = CanSurfaceRenderBlackList(m_codecname) || g_advancedSettings.m_mediacodecForceSoftwareRendring;
  if (m_render_sw)
  {
    if (m_colorFormat == -1)
    {
      CLog::Log(LOGERROR, "CDVDVideoCodecActions:: No supported color format");
      m_codec.reset();
      SAFE_DELETE(m_bitstream);
      return false;
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
  {
    m_codec.reset();
    SAFE_DELETE(m_bitstream);
    return false;
  }

  CLog::Log(LOGINFO, "CDVDVideoCodecActions:: "
    "Open Android MediaCodec %s", m_codecname.c_str());

  m_opened = true;
  memset(&m_demux_pkt, 0, sizeof(m_demux_pkt));

  return m_opened;
}

void CDVDVideoCodecActions::Dispose()
{
  	CDVDVideoCodecAndroidMediaCodec::Dispose();
}

int CDVDVideoCodecActions::Decode(uint8_t *pData, int iSize, double dts, double pts)
{
  // Handle input, add demuxer packet to input queue, we must accept it or
  // it will be discarded as VideoPlayerVideo has no concept of "try again".
  // we must return VC_BUFFER or VC_PICTURE, default to VC_BUFFER.
  int rtn = VC_BUFFER;

  if (!m_opened)
    return VC_ERROR;

  if (m_hints.ptsinvalid)
    pts = DVD_NOPTS_VALUE;

  // must check for an output picture 1st,
  // otherwise, mediacodec can stall on some devices.
  if (GetOutputPicture() > 0)
    rtn |= VC_PICTURE;

  if (!pData)
  {
    // Check if we have a saved buffer
    if (m_demux_pkt.pData)
    {
      pData = m_demux_pkt.pData;
      iSize = m_demux_pkt.iSize;
      pts = m_demux_pkt.pts;
      dts = m_demux_pkt.dts;
    }
  }

  if (pData)
  {
    // try to fetch an input buffer
    int64_t timeout_us = 5000;
    int index = m_codec->dequeueInputBuffer(timeout_us);
    if (xbmc_jnienv()->ExceptionCheck())
    {
      CLog::Log(LOGERROR, "CDVDVideoCodecActions::Decode ExceptionCheck");
      xbmc_jnienv()->ExceptionDescribe();
      xbmc_jnienv()->ExceptionClear();
      rtn = VC_ERROR;
    }
    else if (index >= 0)
    {
      // docs lie, getInputBuffers should be good after
      // m_codec->start() but the internal refs are not
      // setup until much later on some devices.
      if (m_input.empty())
      {
        m_input = m_codec->getInputBuffers();
        if (xbmc_jnienv()->ExceptionCheck())
        {
          CLog::Log(LOGERROR, "CDVDMediaCodecInfo::getInputBuffers "
            "ExceptionCheck");
          xbmc_jnienv()->ExceptionDescribe();
          xbmc_jnienv()->ExceptionClear();
        }
      }

      // we have an input buffer, fill it.
      if (m_bitstream)
      {
        m_bitstream->Convert(pData, iSize);
        iSize = m_bitstream->GetConvertSize();
        pData = m_bitstream->GetConvertBuffer();
      }
      int size = m_input[index].capacity();
      if (iSize > size)
      {
        CLog::Log(LOGERROR, "CDVDVideoCodecActions::Decode, iSize(%d) > size(%d)", iSize, size);
        iSize = size;
      }
      // fetch a pointer to the ByteBuffer backing store
      uint8_t *dst_ptr = (uint8_t*)xbmc_jnienv()->GetDirectBufferAddress(m_input[index].get_raw());
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
/*
      CLog::Log(LOGDEBUG, "CDVDVideoCodecActions:: "
        "pts(%f), ipts(%lld), iSize(%d), GetDataSize(%d), loop_cnt(%d)",
        presentationTimeUs, pts_dtoi(presentationTimeUs), iSize, GetDataSize(), loop_cnt);
*/
      int flags = 0;
      int offset = 0;
      m_codec->queueInputBuffer(index, offset, iSize, presentationTimeUs, flags);
      // clear any jni exceptions, jni gets upset if we do not.
      if (xbmc_jnienv()->ExceptionCheck())
      {
        CLog::Log(LOGERROR, "CDVDVideoCodecActions::Decode ExceptionCheck");
        xbmc_jnienv()->ExceptionClear();
      }

      // Free saved buffer it there was one
      if (m_demux_pkt.pData)
      {
        free(m_demux_pkt.pData);
        memset(&m_demux_pkt, 0, sizeof(m_demux_pkt));
      }
    }
    else
    {
      // We couldn't get an input buffer. Save the packet for next iteration, if it wasn't already
      if (!m_demux_pkt.pData)
      {
        m_demux_pkt.dts = dts;
        m_demux_pkt.pts = pts;
        m_demux_pkt.iSize = iSize;
        m_demux_pkt.pData = (uint8_t*)malloc(iSize);
        memcpy(m_demux_pkt.pData, pData, iSize);
      }

      rtn &= ~VC_BUFFER;
    }
  }

  return rtn;
}

void CDVDVideoCodecActions::Reset()
{
	CDVDVideoCodecAndroidMediaCodec::Reset();
 
}
