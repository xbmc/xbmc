/*
 *      Copyright (C) 2005-2013 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "system.h"
#include "utils/log.h"

#include "DVDFactoryCodec.h"
#include "Video/DVDVideoCodec.h"
#include "Audio/DVDAudioCodec.h"
#include "Overlay/DVDOverlayCodec.h"
#include "cores/dvdplayer/DVDCodecs/DVDCodecs.h"

#if defined(TARGET_DARWIN_OSX)
#include "Video/DVDVideoCodecVDA.h"
#endif
#if defined(HAVE_VIDEOTOOLBOXDECODER)
#include "Video/DVDVideoCodecVideoToolBox.h"
#include "utils/SystemInfo.h"
#endif
#include "Video/DVDVideoCodecFFmpeg.h"
#include "Video/DVDVideoCodecOpenMax.h"
#include "Video/DVDVideoCodecLibMpeg2.h"
#if defined(HAS_IMXVPU)
#include "Video/DVDVideoCodecIMX.h"
#endif
#include "Video/MMALCodec.h"
#include "Video/DVDVideoCodecStageFright.h"
#if defined(HAS_LIBAMCODEC)
#include "utils/AMLUtils.h"
#include "Video/DVDVideoCodecAmlogic.h"
#endif
#if defined(TARGET_ANDROID)
#include "Video/DVDVideoCodecAndroidMediaCodec.h"
#include "android/activity/AndroidFeatures.h"
#endif
#include "Audio/DVDAudioCodecFFmpeg.h"
#include "Audio/DVDAudioCodecPassthrough.h"
#include "Overlay/DVDOverlayCodecSSA.h"
#include "Overlay/DVDOverlayCodecText.h"
#include "Overlay/DVDOverlayCodecTX3G.h"
#include "Overlay/DVDOverlayCodecFFmpeg.h"


#include "DVDStreamInfo.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "settings/VideoSettings.h"
#include "utils/StringUtils.h"

CDVDVideoCodec* CDVDFactoryCodec::OpenCodec(CDVDVideoCodec* pCodec, CDVDStreamInfo &hints, CDVDCodecOptions &options )
{
  try
  {
    CLog::Log(LOGDEBUG, "FactoryCodec - Video: %s - Opening", pCodec->GetName());
    if( pCodec->Open( hints, options ) )
    {
      CLog::Log(LOGDEBUG, "FactoryCodec - Video: %s - Opened", pCodec->GetName());
      return pCodec;
    }

    CLog::Log(LOGDEBUG, "FactoryCodec - Video: %s - Failed", pCodec->GetName());
    pCodec->Dispose();
    delete pCodec;
  }
  catch(...)
  {
    CLog::Log(LOGERROR, "FactoryCodec - Video: Failed with exception");
  }
  return NULL;
}

CDVDAudioCodec* CDVDFactoryCodec::OpenCodec(CDVDAudioCodec* pCodec, CDVDStreamInfo &hints, CDVDCodecOptions &options )
{
  try
  {
    CLog::Log(LOGDEBUG, "FactoryCodec - Audio: %s - Opening", pCodec->GetName());
    if( pCodec->Open( hints, options ) )
    {
      CLog::Log(LOGDEBUG, "FactoryCodec - Audio: %s - Opened", pCodec->GetName());
      return pCodec;
    }

    CLog::Log(LOGDEBUG, "FactoryCodec - Audio: %s - Failed", pCodec->GetName());
    pCodec->Dispose();
    delete pCodec;
  }
  catch(...)
  {
    CLog::Log(LOGERROR, "FactoryCodec - Audio: Failed with exception");
  }
  return NULL;
}

CDVDOverlayCodec* CDVDFactoryCodec::OpenCodec(CDVDOverlayCodec* pCodec, CDVDStreamInfo &hints, CDVDCodecOptions &options )
{
  try
  {
    CLog::Log(LOGDEBUG, "FactoryCodec - Overlay: %s - Opening", pCodec->GetName());
    if( pCodec->Open( hints, options ) )
    {
      CLog::Log(LOGDEBUG, "FactoryCodec - Overlay: %s - Opened", pCodec->GetName());
      return pCodec;
    }

    CLog::Log(LOGDEBUG, "FactoryCodec - Overlay: %s - Failed", pCodec->GetName());
    pCodec->Dispose();
    delete pCodec;
  }
  catch(...)
  {
    CLog::Log(LOGERROR, "FactoryCodec - Audio: Failed with exception");
  }
  return NULL;
}


CDVDVideoCodec* CDVDFactoryCodec::CreateVideoCodec(CDVDStreamInfo &hint, const CRenderInfo &info)
{
  CDVDVideoCodec* pCodec = NULL;
  CDVDCodecOptions options;

  if(info.formats.empty())
    options.m_formats.push_back(RENDER_FMT_YUV420P);
  else
    options.m_formats = info.formats;

  options.m_opaque_pointer = info.opaque_pointer;

  //when support for a hardware decoder is not compiled in
  //only print it if it's actually available on the platform
  std::string hwSupport;
#if defined(TARGET_DARWIN_OSX)
  hwSupport += "VDADecoder:yes ";
#endif
#if defined(HAVE_VIDEOTOOLBOXDECODER) && defined(TARGET_DARWIN)
  hwSupport += "VideoToolBoxDecoder:yes ";
#elif defined(TARGET_DARWIN)
  hwSupport += "VideoToolBoxDecoder:no ";
#endif
#if defined(HAS_LIBAMCODEC)
  hwSupport += "AMCodec:yes ";
#else
  hwSupport += "AMCodec:no ";
#endif
#if defined(TARGET_ANDROID)
  hwSupport += "MediaCodec:yes ";
#else
  hwSupport += "MediaCodec:no ";
#endif
#if defined(HAVE_LIBOPENMAX)
  hwSupport += "OpenMax:yes ";
#elif defined(TARGET_POSIX)
  hwSupport += "OpenMax:no ";
#endif
#if defined(HAS_LIBSTAGEFRIGHT)
  hwSupport += "libstagefright:yes ";
#elif defined(_LINUX)
  hwSupport += "libstagefright:no ";
#endif
#if defined(HAVE_LIBVDPAU) && defined(TARGET_POSIX)
  hwSupport += "VDPAU:yes ";
#elif defined(TARGET_POSIX) && !defined(TARGET_DARWIN)
  hwSupport += "VDPAU:no ";
#endif
#if defined(TARGET_WINDOWS) && defined(HAS_DX)
  hwSupport += "DXVA:yes ";
#elif defined(TARGET_WINDOWS)
  hwSupport += "DXVA:no ";
#endif
#if defined(HAVE_LIBVA) && defined(TARGET_POSIX)
  hwSupport += "VAAPI:yes ";
#elif defined(TARGET_POSIX) && !defined(TARGET_DARWIN)
  hwSupport += "VAAPI:no ";
#endif
#if defined(HAS_IMXVPU)
  hwSupport += "iMXVPU:yes ";
#else
  hwSupport += "iMXVPU:no ";
#endif
#if defined(HAS_MMAL)
  hwSupport += "MMAL:yes ";
#else
  hwSupport += "MMAL:no ";
#endif
  CLog::Log(LOGDEBUG, "CDVDFactoryCodec: compiled in hardware support: %s", hwSupport.c_str());

  if (hint.stills && (hint.codec == AV_CODEC_ID_MPEG2VIDEO || hint.codec == AV_CODEC_ID_MPEG1VIDEO))
  {
     // If dvd is an mpeg2 and hint.stills
     if ( (pCodec = OpenCodec(new CDVDVideoCodecLibMpeg2(), hint, options)) ) return pCodec;
  }

#if defined(HAS_LIBAMCODEC)
  // amcodec can handle dvd playback.
  if (!hint.software && CSettings::Get().GetBool("videoplayer.useamcodec"))
  {
    switch(hint.codec)
    {
      case AV_CODEC_ID_MPEG4:
      case AV_CODEC_ID_MSMPEG4V2:
      case AV_CODEC_ID_MSMPEG4V3:
        // Avoid h/w decoder for SD; Those files might use features
        // not supported and can easily be soft-decoded
        if (hint.width <= 800)
          break;
      default:
        if ( (pCodec = OpenCodec(new CDVDVideoCodecAmlogic(), hint, options)) ) return pCodec;
    }
  }
#endif

#if defined(HAS_IMXVPU)
  if (!hint.software)
  {
    if ( (pCodec = OpenCodec(new CDVDVideoCodecIMX(), hint, options)) ) return pCodec;
  }
#endif

#if defined(TARGET_DARWIN_OSX)
  if (!hint.software && CSettings::Get().GetBool("videoplayer.usevda") && !g_advancedSettings.m_useFfmpegVda)
  {
    if (hint.codec == AV_CODEC_ID_H264 && !hint.ptsinvalid)
    {
      if ( (pCodec = OpenCodec(new CDVDVideoCodecVDA(), hint, options)) ) return pCodec;
    }
  }
#endif

#if defined(HAVE_VIDEOTOOLBOXDECODER)
  if (!hint.software && CSettings::Get().GetBool("videoplayer.usevideotoolbox"))
  {
    if (g_sysinfo.HasVideoToolBoxDecoder())
    {
      switch(hint.codec)
      {
        case AV_CODEC_ID_H264:
          if (hint.codec == AV_CODEC_ID_H264 && hint.ptsinvalid)
            break;
          if ( (pCodec = OpenCodec(new CDVDVideoCodecVideoToolBox(), hint, options)) ) return pCodec;
          break;
        default:
          break;
      }
    }
  }
#endif

#if defined(TARGET_ANDROID)
  if (!hint.software && CSettings::Get().GetBool("videoplayer.usemediacodec"))
  {
    switch(hint.codec)
    {
      case AV_CODEC_ID_MPEG4:
      case AV_CODEC_ID_MSMPEG4V2:
      case AV_CODEC_ID_MSMPEG4V3:
        // Avoid h/w decoder for SD; Those files might use features
        // not supported and can easily be soft-decoded
        if (hint.width <= 800)
          break;
      default:
        CLog::Log(LOGINFO, "MediaCodec Video Decoder...");
        if ( (pCodec = OpenCodec(new CDVDVideoCodecAndroidMediaCodec(), hint, options)) ) return pCodec;
    }
  }
#endif

#if defined(HAVE_LIBOPENMAX)
    if (CSettings::Get().GetBool("videoplayer.useomx") && !hint.software )
    {
      if (hint.codec == AV_CODEC_ID_H264 || hint.codec == AV_CODEC_ID_MPEG2VIDEO || hint.codec == AV_CODEC_ID_VC1)
      {
        if ( (pCodec = OpenCodec(new CDVDVideoCodecOpenMax(), hint, options)) ) return pCodec;
      }
    }
#endif

#if defined(HAS_MMAL)
    if (CSettings::Get().GetBool("videoplayer.usemmal") && !hint.software )
    {
      if (hint.codec == AV_CODEC_ID_H264 || hint.codec == AV_CODEC_ID_H263 || hint.codec == AV_CODEC_ID_MPEG4 ||
          hint.codec == AV_CODEC_ID_MPEG1VIDEO || hint.codec == AV_CODEC_ID_MPEG2VIDEO ||
          hint.codec == AV_CODEC_ID_VP6 || hint.codec == AV_CODEC_ID_VP6F || hint.codec == AV_CODEC_ID_VP6A || hint.codec == AV_CODEC_ID_VP8 ||
          hint.codec == AV_CODEC_ID_THEORA || hint.codec == AV_CODEC_ID_MJPEG || hint.codec == AV_CODEC_ID_MJPEGB || hint.codec == AV_CODEC_ID_VC1 || hint.codec == AV_CODEC_ID_WMV3)
      {
        if ( (pCodec = OpenCodec(new CMMALVideo(), hint, options)) ) return pCodec;
      }
    }
#endif

#if defined(HAS_LIBSTAGEFRIGHT)
    if (!hint.software && CSettings::Get().GetBool("videoplayer.usestagefright"))
    {
      switch(hint.codec)
      {
        case AV_CODEC_ID_MPEG4:
        case AV_CODEC_ID_MSMPEG4V2:
        case AV_CODEC_ID_MSMPEG4V3:
          // Avoid h/w decoder for SD; Those files might use features
          // not supported and can easily be soft-decoded
          if (hint.width <= 800)
            break;
        default:
          if ( (pCodec = OpenCodec(new CDVDVideoCodecStageFright(), hint, options)) ) return pCodec;
      }
    }
#endif


  // try to decide if we want to try halfres decoding
#if !defined(TARGET_POSIX) && !defined(TARGET_WINDOWS)
  float pixelrate = (float)hint.width*hint.height*hint.fpsrate/hint.fpsscale;
  if( pixelrate > 1400.0f*720.0f*30.0f )
  {
    CLog::Log(LOGINFO, "CDVDFactoryCodec - High video resolution detected %dx%d, trying half resolution decoding ", hint.width, hint.height);
    options.m_keys.push_back(CDVDCodecOption("lowres","1"));
  }
#endif

  std::string value = StringUtils::Format("%d", info.max_buffer_size);
  options.m_keys.push_back(CDVDCodecOption("surfaces", value));
  if( (pCodec = OpenCodec(new CDVDVideoCodecFFmpeg(), hint, options)) ) return pCodec;

  return NULL;
}

CDVDAudioCodec* CDVDFactoryCodec::CreateAudioCodec( CDVDStreamInfo &hint)
{
  CDVDAudioCodec* pCodec = NULL;
  CDVDCodecOptions options;

  // try passthrough first
  pCodec = OpenCodec( new CDVDAudioCodecPassthrough(), hint, options );
  if( pCodec ) return pCodec;

  pCodec = OpenCodec( new CDVDAudioCodecFFmpeg(), hint, options );
  if( pCodec ) return pCodec;

  return NULL;
}

CDVDOverlayCodec* CDVDFactoryCodec::CreateOverlayCodec( CDVDStreamInfo &hint )
{
  CDVDOverlayCodec* pCodec = NULL;
  CDVDCodecOptions options;

  switch (hint.codec)
  {
    case AV_CODEC_ID_TEXT:
    case AV_CODEC_ID_SUBRIP:
      pCodec = OpenCodec(new CDVDOverlayCodecText(), hint, options);
      if( pCodec ) return pCodec;
      break;

    case AV_CODEC_ID_SSA:
    case AV_CODEC_ID_ASS:
      pCodec = OpenCodec(new CDVDOverlayCodecSSA(), hint, options);
      if( pCodec ) return pCodec;

      pCodec = OpenCodec(new CDVDOverlayCodecText(), hint, options);
      if( pCodec ) return pCodec;
      break;

    case AV_CODEC_ID_MOV_TEXT:
      pCodec = OpenCodec(new CDVDOverlayCodecTX3G(), hint, options);
      if( pCodec ) return pCodec;
      break;

    default:
      pCodec = OpenCodec(new CDVDOverlayCodecFFmpeg(), hint, options);
      if( pCodec ) return pCodec;
      break;
  }

  return NULL;
}
