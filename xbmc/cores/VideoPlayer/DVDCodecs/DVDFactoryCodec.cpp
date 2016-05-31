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
#include "cores/VideoPlayer/DVDCodecs/DVDCodecs.h"

#include "Video/DVDVideoCodecFFmpeg.h"
#include "Video/DVDVideoCodecOpenMax.h"
#if defined(HAS_IMXVPU)
#include "Video/DVDVideoCodecIMX.h"
#endif
#include "Video/MMALCodec.h"
#if defined(HAS_LIBAMCODEC)
#include "utils/AMLUtils.h"
#include "Video/DVDVideoCodecAmlogic.h"
#endif
#if defined(TARGET_ANDROID)
#include "Video/DVDVideoCodecAndroidMediaCodec.h"
#include "platform/android/activity/AndroidFeatures.h"
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
    delete pCodec;
  }
  catch(...)
  {
    CLog::Log(LOGERROR, "FactoryCodec - Video: Failed with exception");
  }
  return nullptr;
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
    delete pCodec;
  }
  catch(...)
  {
    CLog::Log(LOGERROR, "FactoryCodec - Audio: Failed with exception");
  }
  return nullptr;
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
    delete pCodec;
  }
  catch(...)
  {
    CLog::Log(LOGERROR, "FactoryCodec - Audio: Failed with exception");
  }
  return nullptr;
}


CDVDVideoCodec* CDVDFactoryCodec::CreateVideoCodec(CDVDStreamInfo &hint, CProcessInfo &processInfo, const CRenderInfo &info)
{
  CDVDVideoCodec* pCodec = nullptr;
  CDVDCodecOptions options;

  if (info.formats.empty())
    options.m_formats.push_back(RENDER_FMT_YUV420P);
  else
    options.m_formats = info.formats;

  options.m_opaque_pointer = info.opaque_pointer;

  if (!hint.software)
  {
#if defined(HAS_LIBAMCODEC)
    // Amlogic can be present on multiple platforms (Linux, Android)
    // try this first. if it does not open, we still try other hw decoders
    pCodec = OpenCodec(new CDVDVideoCodecAmlogic(processInfo), hint, options);
    if (pCodec)
      return pCodec;
#endif

#if defined(HAS_IMXVPU)
    pCodec = OpenCodec(new CDVDVideoCodecIMX(processInfo), hint, options);
#elif defined(TARGET_ANDROID)
    pCodec = OpenCodec(new CDVDVideoCodecAndroidMediaCodec(processInfo), hint, options);
#elif defined(HAVE_LIBOPENMAX)
    pCodec = OpenCodec(new CDVDVideoCodecOpenMax(processInfo), hint, options);
#elif defined(HAS_MMAL)
    pCodec = OpenCodec(new CMMALVideo(processInfo), hint, options);
#endif
    if (pCodec)
      return pCodec;
  }

  // try to decide if we want to try halfres decoding
#if !defined(TARGET_POSIX) && !defined(TARGET_WINDOWS)
  float pixelrate = (float)hint.width*hint.height*hint.fpsrate/hint.fpsscale;
  if (pixelrate > 1400.0f*720.0f*30.0f)
  {
    CLog::Log(LOGINFO, "CDVDFactoryCodec - High video resolution detected %dx%d, trying half resolution decoding ", hint.width, hint.height);
    options.m_keys.push_back(CDVDCodecOption("lowres","1"));
  }
#endif

  std::string value = StringUtils::Format("%d", info.max_buffer_size);
  options.m_keys.push_back(CDVDCodecOption("surfaces", value));
  pCodec = OpenCodec(new CDVDVideoCodecFFmpeg(processInfo), hint, options);
  if (pCodec)
    return pCodec;

  return nullptr;;
}

CDVDAudioCodec* CDVDFactoryCodec::CreateAudioCodec(CDVDStreamInfo &hint, CProcessInfo &processInfo, bool allowpassthrough, bool allowdtshddecode)
{
  CDVDAudioCodec* pCodec = NULL;
  CDVDCodecOptions options;

  if (!allowdtshddecode)
    options.m_keys.push_back(CDVDCodecOption("allowdtshddecode", "0"));

  // we don't use passthrough if "sync playback to display" is enabled
  if (allowpassthrough)
  {
    pCodec = OpenCodec(new CDVDAudioCodecPassthrough(processInfo), hint, options);
    if (pCodec)
      return pCodec;
  }

  pCodec = OpenCodec(new CDVDAudioCodecFFmpeg(processInfo), hint, options);
  if (pCodec)
    return pCodec;

  return nullptr;
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
      if (pCodec)
        return pCodec;
      break;

    case AV_CODEC_ID_SSA:
    case AV_CODEC_ID_ASS:
      pCodec = OpenCodec(new CDVDOverlayCodecSSA(), hint, options);
      if (pCodec)
        return pCodec;

      pCodec = OpenCodec(new CDVDOverlayCodecText(), hint, options);
      if (pCodec)
        return pCodec;
      break;

    case AV_CODEC_ID_MOV_TEXT:
      pCodec = OpenCodec(new CDVDOverlayCodecTX3G(), hint, options);
      if (pCodec)
        return pCodec;
      break;

    default:
      pCodec = OpenCodec(new CDVDOverlayCodecFFmpeg(), hint, options);
      if (pCodec)
        return pCodec;
      break;
  }

  return nullptr;
}
