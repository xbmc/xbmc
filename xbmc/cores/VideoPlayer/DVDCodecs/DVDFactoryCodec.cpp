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


//#include "VPFactoryCodec_override.h"

//------------------------------------------------------------------------------
// Video
//------------------------------------------------------------------------------

CDVDVideoCodec* CDVDFactoryCodec::CreateVideoCodec(CDVDStreamInfo &hint, CProcessInfo &processInfo, const CRenderInfo &info)
{
  std::unique_ptr<CDVDVideoCodec> pCodec;
  CDVDCodecOptions options;

  if (info.formats.empty())
    options.m_formats.push_back(RENDER_FMT_YUV420P);
  else
    options.m_formats = info.formats;

  options.m_opaque_pointer = info.opaque_pointer;

  // platform specifig video decoders
  if (!(hint.codecOptions & CODEC_FORCE_SOFTWARE))
  {
    pCodec.reset(CreateVideoCodecHW(processInfo));
    if (pCodec && pCodec->Open(hint, options))
    {
      return pCodec.release();
    }
    if (!(hint.codecOptions & CODEC_ALLOW_FALLBACK))
      return nullptr;
  }

  std::string value = StringUtils::Format("%d", info.max_buffer_size);
  options.m_keys.push_back(CDVDCodecOption("surfaces", value));
  pCodec.reset(new CDVDVideoCodecFFmpeg(processInfo));
  if (pCodec->Open(hint, options))
  {
    return pCodec.release();
  }

  return nullptr;;
}

CDVDVideoCodec* CDVDFactoryCodec::CreateVideoCodec(CDVDStreamInfo &hint, CProcessInfo &processInfo)
{
  CRenderInfo renderInfo;
  return CreateVideoCodec(hint, processInfo, renderInfo);
}

//------------------------------------------------------------------------------
// Audio
//------------------------------------------------------------------------------

CDVDAudioCodec* CDVDFactoryCodec::CreateAudioCodec(CDVDStreamInfo &hint, CProcessInfo &processInfo, bool allowpassthrough, bool allowdtshddecode)
{
  std::unique_ptr<CDVDAudioCodec> pCodec;
  CDVDCodecOptions options;

  // platform specifig audio decoders
  pCodec.reset(CreateAudioCodecHW(processInfo));
  if (pCodec && pCodec->Open(hint, options))
  {
    return pCodec.release();
  }

  if (!allowdtshddecode)
    options.m_keys.push_back(CDVDCodecOption("allowdtshddecode", "0"));

  // we don't use passthrough if "sync playback to display" is enabled
  if (allowpassthrough)
  {
    pCodec.reset(new CDVDAudioCodecPassthrough(processInfo));
    if (pCodec->Open(hint, options))
    {
      return pCodec.release();
    }
  }

  pCodec.reset(new CDVDAudioCodecFFmpeg(processInfo));
  if (pCodec->Open(hint, options))
  {
    return pCodec.release();
  }

  return nullptr;
}

//------------------------------------------------------------------------------
// Overlay
//------------------------------------------------------------------------------

CDVDOverlayCodec* CDVDFactoryCodec::CreateOverlayCodec( CDVDStreamInfo &hint )
{
  std::unique_ptr<CDVDOverlayCodec> pCodec;
  CDVDCodecOptions options;

  switch (hint.codec)
  {
    case AV_CODEC_ID_TEXT:
    case AV_CODEC_ID_SUBRIP:
      pCodec.reset(new CDVDOverlayCodecText());
      if (pCodec->Open(hint, options))
      {
        return pCodec.release();
      }
      break;

    case AV_CODEC_ID_SSA:
    case AV_CODEC_ID_ASS:
      pCodec.reset(new CDVDOverlayCodecSSA());
      if (pCodec->Open(hint, options))
      {
        return pCodec.release();
      }

      pCodec.reset(new CDVDOverlayCodecText());
      if (pCodec->Open(hint, options))
      {
        return pCodec.release();
      }
      break;

    case AV_CODEC_ID_MOV_TEXT:
      pCodec.reset(new CDVDOverlayCodecTX3G());
      if (pCodec->Open(hint, options))
      {
        return pCodec.release();
      }
      break;

    default:
      pCodec.reset(new CDVDOverlayCodecFFmpeg());
      if (pCodec->Open(hint, options))
      {
        return pCodec.release();
      }
      break;
  }

  return nullptr;
}

//------------------------------------------------------------------------------
// Stubs for platform specific overrides
//------------------------------------------------------------------------------

// temp
#if defined(HAS_MMAL)
#define VP_VIDEOCODEC_HW
#include "Video/MMALCodec.h"
CDVDVideoCodec* CDVDFactoryCodec::CreateVideoCodecHW(CProcessInfo &processInfo)
{
  CDVDVideoCodec* pCodec = new CMMALVideo(processInfo);
  return pCodec;
}
#endif

#if defined(TARGET_ANDROID)
#define VP_VIDEOCODEC_HW
#include "Video/DVDVideoCodecAndroidMediaCodec.h"
CDVDVideoCodec* CDVDFactoryCodec::CreateVideoCodecHW(CProcessInfo &processInfo)
{
  CDVDVideoCodec* pCodec = new CDVDVideoCodecAndroidMediaCodec(processInfo);
  return pCodec;
}
#endif

#if !defined(VP_VIDEOCODEC_HW)
CDVDVideoCodec* CDVDFactoryCodec::CreateVideoCodecHW(CProcessInfo &processInfo)
{
  return nullptr;
}
#endif

#if !defined(VP_AUDIOCODEC_HW)
CDVDAudioCodec* CDVDFactoryCodec::CreateAudioCodecHW(CProcessInfo &processInfo)
{
  return nullptr;
}
#endif
