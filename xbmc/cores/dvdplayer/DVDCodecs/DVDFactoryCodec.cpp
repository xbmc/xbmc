/*
 *      Copyright (C) 2005-2008 Team XBMC
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

#include "system.h"
#include "utils/log.h"

#include "DVDFactoryCodec.h"
#include "Video/DVDVideoCodec.h"
#include "Audio/DVDAudioCodec.h"
#include "Overlay/DVDOverlayCodec.h"

#include "Video/DVDVideoCodecVDA.h"
#if defined(HAVE_VIDEOTOOLBOXDECODER)
#include "Video/DVDVideoCodecVideoToolBox.h"
#endif
#include "Video/DVDVideoCodecFFmpeg.h"
#include "Video/DVDVideoCodecOpenMax.h"
#include "Video/DVDVideoCodecLibMpeg2.h"
#if defined(HAVE_LIBCRYSTALHD)
#include "Video/DVDVideoCodecCrystalHD.h"
#endif
#include "Audio/DVDAudioCodecFFmpeg.h"
#include "Audio/DVDAudioCodecLibMad.h"
#include "Audio/DVDAudioCodecPcm.h"
#include "Audio/DVDAudioCodecLPcm.h"
#include "Audio/DVDAudioCodecPassthroughFFmpeg.h"
#include "Overlay/DVDOverlayCodecSSA.h"
#include "Overlay/DVDOverlayCodecText.h"
#include "Overlay/DVDOverlayCodecTX3G.h"
#include "Overlay/DVDOverlayCodecFFmpeg.h"


#include "DVDStreamInfo.h"
#include "settings/GUISettings.h"
#include "utils/SystemInfo.h"

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


CDVDVideoCodec* CDVDFactoryCodec::CreateVideoCodec(CDVDStreamInfo &hint, unsigned int surfaces)
{
  CDVDVideoCodec* pCodec = NULL;
  CDVDCodecOptions options;

  //when support for a hardware decoder is not compiled in
  //only print it if it's actually available on the platform
  CStdString hwSupport;
#if defined(HAVE_LIBVDADECODER) && defined(__APPLE__)
  hwSupport += "VDADecoder:yes ";
#elif defined(__APPLE__)
  hwSupport += "VDADecoder:no ";
#endif
#if defined(HAVE_VIDEOTOOLBOXDECODER) && defined(__APPLE__)
  hwSupport += "VideoToolBoxDecoder:yes ";
#elif defined(__APPLE__)
  hwSupport += "VideoToolBoxDecoder:no ";
#endif
#ifdef HAVE_LIBCRYSTALHD
  hwSupport += "CrystalHD:yes ";
#else
  hwSupport += "CrystalHD:no ";
#endif
#if defined(HAVE_LIBOPENMAX) && defined(_LINUX)
  hwSupport += "OpenMax:yes ";
#elif defined(_LINUX)
  hwSupport += "OpenMax:no ";
#endif
#if defined(HAVE_LIBVDPAU) && defined(_LINUX)
  hwSupport += "VDPAU:yes ";
#elif defined(_LINUX) && !defined(__APPLE__)
  hwSupport += "VDPAU:no ";
#endif
#if defined(_WIN32) && defined(HAS_DX)
  hwSupport += "DXVA:yes ";
#elif defined(_WIN32)
  hwSupport += "DXVA:no ";
#endif
#if defined(HAVE_LIBVA) && defined(_LINUX)
  hwSupport += "VAAPI:yes ";
#elif defined(_LINUX) && !defined(__APPLE__)
  hwSupport += "VAAPI:no ";
#endif

  CLog::Log(LOGDEBUG, "CDVDFactoryCodec: compiled in hardware support: %s", hwSupport.c_str());

  // dvd's have weird still-frames in it, which is not fully supported in ffmpeg
  if(hint.stills && (hint.codec == CODEC_ID_MPEG2VIDEO || hint.codec == CODEC_ID_MPEG1VIDEO))
  {
    if( (pCodec = OpenCodec(new CDVDVideoCodecLibMpeg2(), hint, options)) ) return pCodec;
  }
#if defined(HAVE_LIBVDADECODER)
  if (!hint.software && g_guiSettings.GetBool("videoplayer.usevda"))
  {
    if (g_sysinfo.HasVDADecoder())
    {
      if (hint.codec == CODEC_ID_H264 && !hint.ptsinvalid)
      {
        CLog::Log(LOGINFO, "Trying Apple VDA Decoder...");
        if ( (pCodec = OpenCodec(new CDVDVideoCodecVDA(), hint, options)) ) return pCodec;
      }
    }
  }
#endif

#if defined(HAVE_VIDEOTOOLBOXDECODER)
  if (!hint.software && g_guiSettings.GetBool("videoplayer.usevideotoolbox"))
  {
    if (g_sysinfo.HasVideoToolBoxDecoder())
    {
      switch(hint.codec)
      {
        case CODEC_ID_H264:
          if (hint.codec == CODEC_ID_H264 && hint.ptsinvalid)
            break;
          CLog::Log(LOGINFO, "Apple VideoToolBox Decoder...");
          if ( (pCodec = OpenCodec(new CDVDVideoCodecVideoToolBox(), hint, options)) ) return pCodec;
        break;
        default:
        break;
      }
    }
  }
#endif

#if defined(HAVE_LIBCRYSTALHD)
  if (!hint.software && g_guiSettings.GetBool("videoplayer.usechd"))
  {
    if (CCrystalHD::GetInstance()->DevicePresent())
    {
      switch(hint.codec)
      {
        case CODEC_ID_VC1:
        case CODEC_ID_WMV3:
        case CODEC_ID_H264:
        case CODEC_ID_MPEG2VIDEO:
          if (hint.codec == CODEC_ID_H264 && hint.ptsinvalid)
            break;
          if (hint.codec == CODEC_ID_MPEG2VIDEO && hint.width <= 720)
            break;
          CLog::Log(LOGINFO, "Trying Broadcom Crystal HD Decoder...");
          if ( (pCodec = OpenCodec(new CDVDVideoCodecCrystalHD(), hint, options)) ) return pCodec;
        break;
        default:
        break;
      }
    }
  }
#endif

#if defined(HAVE_LIBOPENMAX)
  if (g_guiSettings.GetBool("videoplayer.useomx") && !hint.software )
  {
      if (hint.codec == CODEC_ID_H264 || hint.codec == CODEC_ID_MPEG2VIDEO || hint.codec == CODEC_ID_VC1)
    {
      CLog::Log(LOGINFO, "Trying OpenMax Decoder...");
      if ( (pCodec = OpenCodec(new CDVDVideoCodecOpenMax(), hint, options)) ) return pCodec;
    }
  }
#endif

  // try to decide if we want to try halfres decoding
#if !defined(_LINUX) && !defined(_WIN32)
  float pixelrate = (float)hint.width*hint.height*hint.fpsrate/hint.fpsscale;
  if( pixelrate > 1400.0f*720.0f*30.0f )
  {
    CLog::Log(LOGINFO, "CDVDFactoryCodec - High video resolution detected %dx%d, trying half resolution decoding ", hint.width, hint.height);
    options.push_back(CDVDCodecOption("lowres","1"));
  }
#endif

  CStdString value;
  value.Format("%d", surfaces);
  options.push_back(CDVDCodecOption("surfaces", value));
  if( (pCodec = OpenCodec(new CDVDVideoCodecFFmpeg(), hint, options)) ) return pCodec;

  return NULL;
}

CDVDAudioCodec* CDVDFactoryCodec::CreateAudioCodec( CDVDStreamInfo &hint, bool passthrough /* = true */)
{
  CDVDAudioCodec* pCodec = NULL;
  CDVDCodecOptions options;

  if (passthrough)
  {
    pCodec = OpenCodec( new CDVDAudioCodecPassthroughFFmpeg(), hint, options);
    if ( pCodec ) return pCodec;
  }

  switch (hint.codec)
  {
  case CODEC_ID_MP2:
  case CODEC_ID_MP3:
    {
      pCodec = OpenCodec( new CDVDAudioCodecLibMad(), hint, options );
      if( pCodec ) return pCodec;
      break;
    }
  case CODEC_ID_PCM_S32LE:
  case CODEC_ID_PCM_S32BE:
  case CODEC_ID_PCM_U32LE:
  case CODEC_ID_PCM_U32BE:
  case CODEC_ID_PCM_S24LE:
  case CODEC_ID_PCM_S24BE:
  case CODEC_ID_PCM_U24LE:
  case CODEC_ID_PCM_U24BE:
  case CODEC_ID_PCM_S24DAUD:
  case CODEC_ID_PCM_S16LE:
  case CODEC_ID_PCM_S16BE:
  case CODEC_ID_PCM_U16LE:
  case CODEC_ID_PCM_U16BE:
  case CODEC_ID_PCM_S8:
  case CODEC_ID_PCM_U8:
  case CODEC_ID_PCM_ALAW:
  case CODEC_ID_PCM_MULAW:
    {
      pCodec = OpenCodec( new CDVDAudioCodecPcm(), hint, options );
      if( pCodec ) return pCodec;
      break;
    }
#if 0
  //case CODEC_ID_LPCM_S16BE:
  //case CODEC_ID_LPCM_S20BE:
  case CODEC_ID_LPCM_S24BE:
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
    case CODEC_ID_TEXT:
      pCodec = OpenCodec(new CDVDOverlayCodecText(), hint, options);
      if( pCodec ) return pCodec;

    case CODEC_ID_SSA:
      pCodec = OpenCodec(new CDVDOverlayCodecSSA(), hint, options);
      if( pCodec ) return pCodec;

      pCodec = OpenCodec(new CDVDOverlayCodecText(), hint, options);
      if( pCodec ) return pCodec;

    case CODEC_ID_MOV_TEXT:
      pCodec = OpenCodec(new CDVDOverlayCodecTX3G(), hint, options);
      if( pCodec ) return pCodec;

    default:
      pCodec = OpenCodec(new CDVDOverlayCodecFFmpeg(), hint, options);
      if( pCodec ) return pCodec;
  }

  return NULL;
}
