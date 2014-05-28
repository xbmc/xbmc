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

#ifndef DVDCODECS_SYSTEM_H_INCLUDED
#define DVDCODECS_SYSTEM_H_INCLUDED
#include "system.h"
#endif

#ifndef DVDCODECS_UTILS_LOG_H_INCLUDED
#define DVDCODECS_UTILS_LOG_H_INCLUDED
#include "utils/log.h"
#endif


#ifndef DVDCODECS_DVDFACTORYCODEC_H_INCLUDED
#define DVDCODECS_DVDFACTORYCODEC_H_INCLUDED
#include "DVDFactoryCodec.h"
#endif

#ifndef DVDCODECS_VIDEO_DVDVIDEOCODEC_H_INCLUDED
#define DVDCODECS_VIDEO_DVDVIDEOCODEC_H_INCLUDED
#include "Video/DVDVideoCodec.h"
#endif

#ifndef DVDCODECS_AUDIO_DVDAUDIOCODEC_H_INCLUDED
#define DVDCODECS_AUDIO_DVDAUDIOCODEC_H_INCLUDED
#include "Audio/DVDAudioCodec.h"
#endif

#ifndef DVDCODECS_OVERLAY_DVDOVERLAYCODEC_H_INCLUDED
#define DVDCODECS_OVERLAY_DVDOVERLAYCODEC_H_INCLUDED
#include "Overlay/DVDOverlayCodec.h"
#endif


#if defined(TARGET_DARWIN_OSX)
#include "Video/DVDVideoCodecVDA.h"
#endif
#if defined(HAVE_VIDEOTOOLBOXDECODER)
#include "Video/DVDVideoCodecVideoToolBox.h"
#endif
#ifndef DVDCODECS_VIDEO_DVDVIDEOCODECFFMPEG_H_INCLUDED
#define DVDCODECS_VIDEO_DVDVIDEOCODECFFMPEG_H_INCLUDED
#include "Video/DVDVideoCodecFFmpeg.h"
#endif

#ifndef DVDCODECS_VIDEO_DVDVIDEOCODECOPENMAX_H_INCLUDED
#define DVDCODECS_VIDEO_DVDVIDEOCODECOPENMAX_H_INCLUDED
#include "Video/DVDVideoCodecOpenMax.h"
#endif

#ifndef DVDCODECS_VIDEO_DVDVIDEOCODECLIBMPEG2_H_INCLUDED
#define DVDCODECS_VIDEO_DVDVIDEOCODECLIBMPEG2_H_INCLUDED
#include "Video/DVDVideoCodecLibMpeg2.h"
#endif

#ifndef DVDCODECS_VIDEO_DVDVIDEOCODECSTAGEFRIGHT_H_INCLUDED
#define DVDCODECS_VIDEO_DVDVIDEOCODECSTAGEFRIGHT_H_INCLUDED
#include "Video/DVDVideoCodecStageFright.h"
#endif

#if defined(HAVE_LIBCRYSTALHD)
#include "Video/DVDVideoCodecCrystalHD.h"
#endif
#if defined(HAS_LIBAMCODEC)
#include "utils/AMLUtils.h"
#include "Video/DVDVideoCodecAmlogic.h"
#endif
#if defined(TARGET_ANDROID)
#include "Video/DVDVideoCodecAndroidMediaCodec.h"
#include "android/activity/AndroidFeatures.h"
#endif
#ifndef DVDCODECS_AUDIO_DVDAUDIOCODECFFMPEG_H_INCLUDED
#define DVDCODECS_AUDIO_DVDAUDIOCODECFFMPEG_H_INCLUDED
#include "Audio/DVDAudioCodecFFmpeg.h"
#endif

#ifndef DVDCODECS_AUDIO_DVDAUDIOCODECLIBMAD_H_INCLUDED
#define DVDCODECS_AUDIO_DVDAUDIOCODECLIBMAD_H_INCLUDED
#include "Audio/DVDAudioCodecLibMad.h"
#endif

#ifndef DVDCODECS_AUDIO_DVDAUDIOCODECPCM_H_INCLUDED
#define DVDCODECS_AUDIO_DVDAUDIOCODECPCM_H_INCLUDED
#include "Audio/DVDAudioCodecPcm.h"
#endif

#ifndef DVDCODECS_AUDIO_DVDAUDIOCODECLPCM_H_INCLUDED
#define DVDCODECS_AUDIO_DVDAUDIOCODECLPCM_H_INCLUDED
#include "Audio/DVDAudioCodecLPcm.h"
#endif

#ifndef DVDCODECS_AUDIO_DVDAUDIOCODECPASSTHROUGH_H_INCLUDED
#define DVDCODECS_AUDIO_DVDAUDIOCODECPASSTHROUGH_H_INCLUDED
#include "Audio/DVDAudioCodecPassthrough.h"
#endif

#ifndef DVDCODECS_OVERLAY_DVDOVERLAYCODECSSA_H_INCLUDED
#define DVDCODECS_OVERLAY_DVDOVERLAYCODECSSA_H_INCLUDED
#include "Overlay/DVDOverlayCodecSSA.h"
#endif

#ifndef DVDCODECS_OVERLAY_DVDOVERLAYCODECTEXT_H_INCLUDED
#define DVDCODECS_OVERLAY_DVDOVERLAYCODECTEXT_H_INCLUDED
#include "Overlay/DVDOverlayCodecText.h"
#endif

#ifndef DVDCODECS_OVERLAY_DVDOVERLAYCODECTX3G_H_INCLUDED
#define DVDCODECS_OVERLAY_DVDOVERLAYCODECTX3G_H_INCLUDED
#include "Overlay/DVDOverlayCodecTX3G.h"
#endif

#ifndef DVDCODECS_OVERLAY_DVDOVERLAYCODECFFMPEG_H_INCLUDED
#define DVDCODECS_OVERLAY_DVDOVERLAYCODECFFMPEG_H_INCLUDED
#include "Overlay/DVDOverlayCodecFFmpeg.h"
#endif



#ifndef DVDCODECS_DVDSTREAMINFO_H_INCLUDED
#define DVDCODECS_DVDSTREAMINFO_H_INCLUDED
#include "DVDStreamInfo.h"
#endif

#ifndef DVDCODECS_SETTINGS_SETTINGS_H_INCLUDED
#define DVDCODECS_SETTINGS_SETTINGS_H_INCLUDED
#include "settings/Settings.h"
#endif

#ifndef DVDCODECS_UTILS_SYSTEMINFO_H_INCLUDED
#define DVDCODECS_UTILS_SYSTEMINFO_H_INCLUDED
#include "utils/SystemInfo.h"
#endif

#ifndef DVDCODECS_UTILS_STRINGUTILS_H_INCLUDED
#define DVDCODECS_UTILS_STRINGUTILS_H_INCLUDED
#include "utils/StringUtils.h"
#endif


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


CDVDVideoCodec* CDVDFactoryCodec::CreateVideoCodec(CDVDStreamInfo &hint, unsigned int surfaces, const std::vector<ERenderFormat>& formats)
{
  CDVDVideoCodec* pCodec = NULL;
  CDVDCodecOptions options;

  if(formats.empty())
    options.m_formats.push_back(RENDER_FMT_YUV420P);
  else
    options.m_formats = formats;

  //when support for a hardware decoder is not compiled in
  //only print it if it's actually available on the platform
  CStdString hwSupport;
#if defined(TARGET_DARWIN_OSX)
  hwSupport += "VDADecoder:yes ";
#endif
#if defined(HAVE_VIDEOTOOLBOXDECODER) && defined(TARGET_DARWIN)
  hwSupport += "VideoToolBoxDecoder:yes ";
#elif defined(TARGET_DARWIN)
  hwSupport += "VideoToolBoxDecoder:no ";
#endif
#ifdef HAVE_LIBCRYSTALHD
  hwSupport += "CrystalHD:yes ";
#else
  hwSupport += "CrystalHD:no ";
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
#if defined(HAVE_LIBOPENMAX) && defined(TARGET_POSIX)
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
     if ( (pCodec = OpenCodec(new CDVDVideoCodecAmlogic(), hint, options)) ) return pCodec;
  }
#endif

#if defined(TARGET_DARWIN_OSX)
  if (!hint.software && CSettings::Get().GetBool("videoplayer.usevda"))
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

#if defined(HAVE_LIBCRYSTALHD)
  if (!hint.software && CSettings::Get().GetBool("videoplayer.usechd"))
  {
    if (CCrystalHD::GetInstance()->DevicePresent())
    {
      switch(hint.codec)
      {
        case AV_CODEC_ID_VC1:
        case AV_CODEC_ID_WMV3:
        case AV_CODEC_ID_H264:
        case AV_CODEC_ID_MPEG2VIDEO:
          if (hint.codec == AV_CODEC_ID_H264 && hint.ptsinvalid)
            break;
          if (hint.codec == AV_CODEC_ID_MPEG2VIDEO && hint.width <= 720)
            break;
          if ( (pCodec = OpenCodec(new CDVDVideoCodecCrystalHD(), hint, options)) ) return pCodec;
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
    CLog::Log(LOGINFO, "MediaCodec Video Decoder...");
    if ( (pCodec = OpenCodec(new CDVDVideoCodecAndroidMediaCodec(), hint, options)) ) return pCodec;
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

#if defined(HAS_LIBSTAGEFRIGHT)
  if (!hint.software && CSettings::Get().GetBool("videoplayer.usestagefright"))
  {
    switch(hint.codec)
    {
      case CODEC_ID_H264:
      case CODEC_ID_MPEG4:
      case CODEC_ID_MPEG2VIDEO:
      case CODEC_ID_VC1:
      case CODEC_ID_WMV3:
      case CODEC_ID_VP3:
      case CODEC_ID_VP6:
      case CODEC_ID_VP6F:
      case CODEC_ID_VP8:
        if ( (pCodec = OpenCodec(new CDVDVideoCodecStageFright(), hint, options)) ) return pCodec;
        break;
      default:
        break;
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

  CStdString value = StringUtils::Format("%d", surfaces);
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

  switch (hint.codec)
  {
  case AV_CODEC_ID_MP2:
  case AV_CODEC_ID_MP3:
    {
      pCodec = OpenCodec( new CDVDAudioCodecLibMad(), hint, options );
      if( pCodec ) return pCodec;
      break;
    }
  case AV_CODEC_ID_PCM_S32LE:
  case AV_CODEC_ID_PCM_S32BE:
  case AV_CODEC_ID_PCM_U32LE:
  case AV_CODEC_ID_PCM_U32BE:
  case AV_CODEC_ID_PCM_S24LE:
  case AV_CODEC_ID_PCM_S24BE:
  case AV_CODEC_ID_PCM_U24LE:
  case AV_CODEC_ID_PCM_U24BE:
  case AV_CODEC_ID_PCM_S24DAUD:
  case AV_CODEC_ID_PCM_S16LE:
  case AV_CODEC_ID_PCM_S16BE:
  case AV_CODEC_ID_PCM_U16LE:
  case AV_CODEC_ID_PCM_U16BE:
  case AV_CODEC_ID_PCM_S8:
  case AV_CODEC_ID_PCM_U8:
  case AV_CODEC_ID_PCM_ALAW:
  case AV_CODEC_ID_PCM_MULAW:
    {
      pCodec = OpenCodec( new CDVDAudioCodecPcm(), hint, options );
      if( pCodec ) return pCodec;
      break;
    }
#if 0
  //case AV_CODEC_ID_LPCM_S16BE:
  //case AV_CODEC_ID_LPCM_S20BE:
  case AV_CODEC_ID_LPCM_S24BE:
    {
      pCodec = OpenCodec( new CDVDAudioCodecLPcm(), hint, options );
      if( pCodec ) return pCodec;
      break;
    }
#endif
  default:
    {
      pCodec = NULL;
      break;
    }
  }

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
