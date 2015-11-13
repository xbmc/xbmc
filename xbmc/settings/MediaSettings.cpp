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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include <string>

#include <limits.h>

#include "MediaSettings.h"
#include "Application.h"
#include "PlayListPlayer.h"
#include "dialogs/GUIDialogContextMenu.h"
#include "dialogs/GUIDialogFileBrowser.h"
#include "interfaces/builtins/Builtins.h"
#include "music/MusicDatabase.h"
#include "messaging/ApplicationMessenger.h"
#include "messaging/helpers/DialogHelper.h"
#include "profiles/ProfilesManager.h"
#include "settings/lib/Setting.h"
#include "settings/Settings.h"
#include "storage/MediaManager.h"
#include "threads/SingleLock.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/XBMCTinyXML.h"
#include "utils/XMLUtils.h"
#include "utils/Variant.h"
#include "video/VideoDatabase.h"
#include "cores/AudioEngine/DSPAddons/ActiveAEDSP.h"

#ifdef HAS_DS_PLAYER
#include "cores/DSPlayer/Dialogs/GUIDialogDSRules.h"
#include "cores/DSPlayer/Dialogs/GUIDialogDSFilters.h"
#include "cores/DSPlayer/Dialogs/GUIDialogDSPlayercoreFactory.h"
#include "MadvrCallback.h"
#include "GraphFilters.h"
#endif
using namespace KODI::MESSAGING;

using KODI::MESSAGING::HELPERS::DialogResponse;

CMediaSettings::CMediaSettings()
{
  m_watchedModes["files"] = WatchedModeAll;
  m_watchedModes["movies"] = WatchedModeAll;
  m_watchedModes["tvshows"] = WatchedModeAll;
  m_watchedModes["musicvideos"] = WatchedModeAll;

  m_musicPlaylistRepeat = false;
  m_musicPlaylistShuffle = false;
  m_videoPlaylistRepeat = false;
  m_videoPlaylistShuffle = false;

  m_videoStartWindowed = false;
  m_additionalSubtitleDirectoryChecked = 0;

  m_musicNeedsUpdate = 0;
  m_videoNeedsUpdate = 0;
}

CMediaSettings::~CMediaSettings()
{ }

CMediaSettings& CMediaSettings::GetInstance()
{
  static CMediaSettings sMediaSettings;
  return sMediaSettings;
}

bool CMediaSettings::Load(const TiXmlNode *settings)
{
  if (settings == NULL)
    return false;

  CSingleLock lock(m_critical);
  const TiXmlElement *pElement = settings->FirstChildElement("defaultvideosettings");
  if (pElement != NULL)
  {
    int deinterlaceMode;
    bool deinterlaceModePresent = XMLUtils::GetInt(pElement, "deinterlacemode", deinterlaceMode, VS_DEINTERLACEMODE_OFF, VS_DEINTERLACEMODE_FORCE);
    int interlaceMethod;
    bool interlaceMethodPresent = XMLUtils::GetInt(pElement, "interlacemethod", interlaceMethod, VS_INTERLACEMETHOD_AUTO, VS_INTERLACEMETHOD_MAX);
    // For smooth conversion of settings stored before the deinterlaceMode existed
    if (!deinterlaceModePresent && interlaceMethodPresent)
    {
      if (interlaceMethod == VS_INTERLACEMETHOD_NONE)
      {
        deinterlaceMode = VS_DEINTERLACEMODE_OFF;
        interlaceMethod = VS_INTERLACEMETHOD_AUTO;
      }
      else if (interlaceMethod == VS_INTERLACEMETHOD_AUTO)
        deinterlaceMode = VS_DEINTERLACEMODE_AUTO;
      else
        deinterlaceMode = VS_DEINTERLACEMODE_FORCE;
    }
    m_defaultVideoSettings.m_DeinterlaceMode = (EDEINTERLACEMODE)deinterlaceMode;
    m_defaultVideoSettings.m_InterlaceMethod = (EINTERLACEMETHOD)interlaceMethod;
    int scalingMethod;
    if (!XMLUtils::GetInt(pElement, "scalingmethod", scalingMethod, VS_SCALINGMETHOD_NEAREST, VS_SCALINGMETHOD_MAX))
      scalingMethod = (int)VS_SCALINGMETHOD_LINEAR;
    m_defaultVideoSettings.m_ScalingMethod = (ESCALINGMETHOD)scalingMethod;

    XMLUtils::GetInt(pElement, "viewmode", m_defaultVideoSettings.m_ViewMode, ViewModeNormal, ViewModeCustom);
    if (!XMLUtils::GetFloat(pElement, "zoomamount", m_defaultVideoSettings.m_CustomZoomAmount, 0.5f, 2.0f))
      m_defaultVideoSettings.m_CustomZoomAmount = 1.0f;
    if (!XMLUtils::GetFloat(pElement, "pixelratio", m_defaultVideoSettings.m_CustomPixelRatio, 0.5f, 2.0f))
      m_defaultVideoSettings.m_CustomPixelRatio = 1.0f;
    if (!XMLUtils::GetFloat(pElement, "verticalshift", m_defaultVideoSettings.m_CustomVerticalShift, -2.0f, 2.0f))
      m_defaultVideoSettings.m_CustomVerticalShift = 0.0f;
    if (!XMLUtils::GetFloat(pElement, "volumeamplification", m_defaultVideoSettings.m_VolumeAmplification, VOLUME_DRC_MINIMUM * 0.01f, VOLUME_DRC_MAXIMUM * 0.01f))
      m_defaultVideoSettings.m_VolumeAmplification = VOLUME_DRC_MINIMUM * 0.01f;
    if (!XMLUtils::GetFloat(pElement, "noisereduction", m_defaultVideoSettings.m_NoiseReduction, 0.0f, 1.0f))
      m_defaultVideoSettings.m_NoiseReduction = 0.0f;
    XMLUtils::GetBoolean(pElement, "postprocess", m_defaultVideoSettings.m_PostProcess);
    if (!XMLUtils::GetFloat(pElement, "sharpness", m_defaultVideoSettings.m_Sharpness, -1.0f, 1.0f))
      m_defaultVideoSettings.m_Sharpness = 0.0f;
    XMLUtils::GetBoolean(pElement, "outputtoallspeakers", m_defaultVideoSettings.m_OutputToAllSpeakers);
    XMLUtils::GetBoolean(pElement, "showsubtitles", m_defaultVideoSettings.m_SubtitleOn);
    if (!XMLUtils::GetFloat(pElement, "brightness", m_defaultVideoSettings.m_Brightness, 0, 100))
      m_defaultVideoSettings.m_Brightness = 50;
    if (!XMLUtils::GetFloat(pElement, "contrast", m_defaultVideoSettings.m_Contrast, 0, 100))
      m_defaultVideoSettings.m_Contrast = 50;
    if (!XMLUtils::GetFloat(pElement, "gamma", m_defaultVideoSettings.m_Gamma, 0, 100))
      m_defaultVideoSettings.m_Gamma = 20;
    if (!XMLUtils::GetFloat(pElement, "audiodelay", m_defaultVideoSettings.m_AudioDelay, -10.0f, 10.0f))
      m_defaultVideoSettings.m_AudioDelay = 0.0f;
    if (!XMLUtils::GetFloat(pElement, "subtitledelay", m_defaultVideoSettings.m_SubtitleDelay, -10.0f, 10.0f))
      m_defaultVideoSettings.m_SubtitleDelay = 0.0f;
    XMLUtils::GetBoolean(pElement, "nonlinstretch", m_defaultVideoSettings.m_CustomNonLinStretch);
    if (!XMLUtils::GetInt(pElement, "stereomode", m_defaultVideoSettings.m_StereoMode))
      m_defaultVideoSettings.m_StereoMode = 0;

    m_defaultVideoSettings.m_SubtitleCached = false;
  }

  pElement = settings->FirstChildElement("defaultaudiosettings");
  if (pElement != NULL)
  {
    if (!XMLUtils::GetInt(pElement, "masterstreamtype", m_defaultAudioSettings.m_MasterStreamType))
      m_defaultAudioSettings.m_MasterStreamType = AE_DSP_ASTREAM_AUTO;
    if (!XMLUtils::GetInt(pElement, "masterstreamtypesel", m_defaultAudioSettings.m_MasterStreamTypeSel))
      m_defaultAudioSettings.m_MasterStreamTypeSel = AE_DSP_ASTREAM_AUTO;
    if (!XMLUtils::GetInt(pElement, "masterstreambase", m_defaultAudioSettings.m_MasterStreamBase))
      m_defaultAudioSettings.m_MasterStreamBase = AE_DSP_ABASE_STEREO;

    std::string strTag;
    for (int type = AE_DSP_ASTREAM_BASIC; type < AE_DSP_ASTREAM_MAX; type++)
    {
      for (int base = AE_DSP_ABASE_STEREO; base < AE_DSP_ABASE_MAX; base++)
      {
        int tmp;
        strTag = StringUtils::Format("masterprocess_%i_%i", type, base);
        if (XMLUtils::GetInt(pElement, strTag.c_str(), tmp))
          m_defaultAudioSettings.m_MasterModes[type][base] = tmp;
        else
          m_defaultAudioSettings.m_MasterModes[type][base] = 0;
      }
    }
  }

#ifdef HAS_DS_PLAYER
  // madvr settings
  pElement = settings->FirstChildElement("defaultmadvrsettings");
  if (pElement != NULL)
  {
    if (!XMLUtils::GetInt(pElement, "chromaupscaling", m_defaultMadvrSettings.m_ChromaUpscaling))
      m_defaultMadvrSettings.m_ChromaUpscaling = MADVR_DEFAULT_CHROMAUP;
    XMLUtils::GetBoolean(pElement, "chromaantiring", m_defaultMadvrSettings.m_ChromaAntiRing);
    XMLUtils::GetBoolean(pElement, "chromasuperres", m_defaultMadvrSettings.m_ChromaSuperRes);
    if (!XMLUtils::GetInt(pElement, "chromasuperrespasses", m_defaultMadvrSettings.m_ChromaSuperResPasses))
      m_defaultMadvrSettings.m_ChromaSuperResPasses = MADVR_DEFAULT_CHROMAUP_SUPERRESPASSES;
    if (!XMLUtils::GetFloat(pElement, "cchromasuperresstrength", m_defaultMadvrSettings.m_ChromaSuperResStrength))
      m_defaultMadvrSettings.m_ChromaSuperResStrength = MADVR_DEFAULT_CHROMAUP_SUPERRESSTRENGTH;
    if (!XMLUtils::GetFloat(pElement, "chromaupscalingsoftness", m_defaultMadvrSettings.m_ChromaSuperResSoftness))
      m_defaultMadvrSettings.m_ChromaSuperResSoftness = MADVR_DEFAULT_CHROMAUP_SUPERRESSOFTNESS;

    if (!XMLUtils::GetInt(pElement, "imageupscaling", m_defaultMadvrSettings.m_ImageUpscaling))
      m_defaultMadvrSettings.m_ImageUpscaling = MADVR_DEFAULT_LUMAUP;
    XMLUtils::GetBoolean(pElement, "imageupantiring", m_defaultMadvrSettings.m_ImageUpAntiRing);
    XMLUtils::GetBoolean(pElement, "imageuplinear", m_defaultMadvrSettings.m_ImageUpLinear);

    if (!XMLUtils::GetInt(pElement, "imagedownscaling", m_defaultMadvrSettings.m_ImageDownscaling))
      m_defaultMadvrSettings.m_ImageDownscaling = MADVR_DEFAULT_LUMADOWN;
    XMLUtils::GetBoolean(pElement, "imagedownantiring", m_defaultMadvrSettings.m_ImageDownAntiRing);
    XMLUtils::GetBoolean(pElement, "imagedownlinear", m_defaultMadvrSettings.m_ImageDownLinear);

    if (!XMLUtils::GetInt(pElement, "imagedoubleluma", m_defaultMadvrSettings.m_ImageDoubleLuma))
      m_defaultMadvrSettings.m_ImageDoubleLuma = -1;
    if (!XMLUtils::GetInt(pElement, "imagedoublechroma", m_defaultMadvrSettings.m_ImageDoubleChroma))
      m_defaultMadvrSettings.m_ImageDoubleChroma = -1;
    if (!XMLUtils::GetInt(pElement, "imagequadrupleluma", m_defaultMadvrSettings.m_ImageQuadrupleLuma))
      m_defaultMadvrSettings.m_ImageQuadrupleLuma = -1;
    if (!XMLUtils::GetInt(pElement, "imagequadruplechroma", m_defaultMadvrSettings.m_ImageQuadrupleChroma))
      m_defaultMadvrSettings.m_ImageQuadrupleChroma = -1;

    if (!XMLUtils::GetInt(pElement, "imagedoublelumafactor", m_defaultMadvrSettings.m_ImageDoubleLumaFactor))
      m_defaultMadvrSettings.m_ImageDoubleLumaFactor = MADVR_DEFAULT_DOUBLEFACTOR;
    if (!XMLUtils::GetInt(pElement, "imagedoublechromafactor", m_defaultMadvrSettings.m_ImageDoubleChromaFactor))
      m_defaultMadvrSettings.m_ImageDoubleChromaFactor = MADVR_DEFAULT_DOUBLEFACTOR;
    if (!XMLUtils::GetInt(pElement, "imagequadruplelumafactor", m_defaultMadvrSettings.m_ImageQuadrupleLumaFactor))
      m_defaultMadvrSettings.m_ImageQuadrupleLumaFactor = MADVR_DEFAULT_QUADRUPLEFACTOR;
    if (!XMLUtils::GetInt(pElement, "imagequadruplechromafactor", m_defaultMadvrSettings.m_ImageQuadrupleChromaFactor))
      m_defaultMadvrSettings.m_ImageQuadrupleChromaFactor = MADVR_DEFAULT_QUADRUPLEFACTOR;

    if (!XMLUtils::GetInt(pElement, "deintactive", m_defaultMadvrSettings.m_deintactive))
      m_defaultMadvrSettings.m_deintactive = MADVR_DEFAULT_DEINTACTIVE;
    if (!XMLUtils::GetInt(pElement, "deintforce", m_defaultMadvrSettings.m_deintforce))
      m_defaultMadvrSettings.m_deintforce = MADVR_DEFAULT_DEINTFORCE;
    XMLUtils::GetBoolean(pElement, "deintlookpixels", m_defaultMadvrSettings.m_deintlookpixels);

    if (!XMLUtils::GetInt(pElement, "smoothmotion", m_defaultMadvrSettings.m_smoothMotion))
      m_defaultMadvrSettings.m_smoothMotion = -1;
    if (!XMLUtils::GetInt(pElement, "dithering", m_defaultMadvrSettings.m_dithering))
      m_defaultMadvrSettings.m_dithering = MADVR_DEFAULT_DITHERING;
    XMLUtils::GetBoolean(pElement, "ditheringcolorednoise", m_defaultMadvrSettings.m_ditheringColoredNoise);
    XMLUtils::GetBoolean(pElement, "ditheringeveryframe", m_defaultMadvrSettings.m_ditheringEveryFrame);

    XMLUtils::GetBoolean(pElement, "deband", m_defaultMadvrSettings.m_deband);
    if (!XMLUtils::GetInt(pElement, "debandlevel", m_defaultMadvrSettings.m_debandLevel))
      m_defaultMadvrSettings.m_debandLevel = MADVR_DEFAULT_DEBAND_LEVEL;
    if (!XMLUtils::GetInt(pElement, "debandfadelevel", m_defaultMadvrSettings.m_debandFadeLevel))
      m_defaultMadvrSettings.m_debandFadeLevel = MADVR_DEFAULT_DEBAND_FADELEVEL;

    XMLUtils::GetBoolean(pElement, "sharpenedges", m_defaultMadvrSettings.m_sharpenEdges);
    if (!XMLUtils::GetFloat(pElement, "sharpenedgesstrength", m_defaultMadvrSettings.m_sharpenEdgesStrength))
      m_defaultMadvrSettings.m_sharpenEdgesStrength = MADVR_DEFAULT_SHARPENEDGES;
    XMLUtils::GetBoolean(pElement, "crispenedges", m_defaultMadvrSettings.m_crispenEdges);
    if (!XMLUtils::GetFloat(pElement, "crispenedgesstrength", m_defaultMadvrSettings.m_crispenEdgesStrength))
      m_defaultMadvrSettings.m_crispenEdgesStrength = MADVR_DEFAULT_CRISPENEDGES;
    XMLUtils::GetBoolean(pElement, "thinedges", m_defaultMadvrSettings.m_thinEdges);
    if (!XMLUtils::GetFloat(pElement, "thinedgesstrength", m_defaultMadvrSettings.m_thinEdgesStrength))
      m_defaultMadvrSettings.m_thinEdgesStrength = MADVR_DEFAULT_THINEDGES;
    XMLUtils::GetBoolean(pElement, "enhancedetail", m_defaultMadvrSettings.m_enhanceDetail);
    if (!XMLUtils::GetFloat(pElement, "enhancedetailstrength", m_defaultMadvrSettings.m_enhanceDetailStrength))
      m_defaultMadvrSettings.m_enhanceDetailStrength = MADVR_DEFAULT_ENHANCEDETAIL;
    XMLUtils::GetBoolean(pElement, "lumasharpen", m_defaultMadvrSettings.m_lumaSharpen);
    if (!XMLUtils::GetFloat(pElement, "lumasharpenstrength", m_defaultMadvrSettings.m_lumaSharpenStrength))
      m_defaultMadvrSettings.m_lumaSharpenStrength = MADVR_DEFAULT_LUMASHARPEN;
    XMLUtils::GetBoolean(pElement, "adaptivesharpen", m_defaultMadvrSettings.m_adaptiveSharpen);
    if (!XMLUtils::GetFloat(pElement, "adaptivesharpenstrength", m_defaultMadvrSettings.m_adaptiveSharpenStrength))
      m_defaultMadvrSettings.m_adaptiveSharpenStrength = MADVR_DEFAULT_ADAPTIVESHARPEN;

    if (!XMLUtils::GetInt(pElement, "nosmallscaling", m_defaultMadvrSettings.m_noSmallScaling))
      m_defaultMadvrSettings.m_noSmallScaling = -1;
    if (!XMLUtils::GetInt(pElement, "movesubs", m_defaultMadvrSettings.m_moveSubs))
      m_defaultMadvrSettings.m_moveSubs = 0;
    XMLUtils::GetBoolean(pElement, "detectbars", m_defaultMadvrSettings.m_detectBars);
    if (!XMLUtils::GetInt(pElement, "archange", m_defaultMadvrSettings.m_arChange))
      m_defaultMadvrSettings.m_arChange = -1;
    if (!XMLUtils::GetInt(pElement, "quickarchange", m_defaultMadvrSettings.m_quickArChange))
      m_defaultMadvrSettings.m_quickArChange = -1;
    if (!XMLUtils::GetInt(pElement, "shiftimage", m_defaultMadvrSettings.m_shiftImage))
      m_defaultMadvrSettings.m_shiftImage = -1;
    if (!XMLUtils::GetInt(pElement, "dontcropsubs", m_defaultMadvrSettings.m_dontCropSubs))
      m_defaultMadvrSettings.m_dontCropSubs = -1;
    if (!XMLUtils::GetInt(pElement, "cleanborders", m_defaultMadvrSettings.m_cleanBorders))
      m_defaultMadvrSettings.m_cleanBorders = -1;
    if (!XMLUtils::GetInt(pElement, "reducebigbars", m_defaultMadvrSettings.m_reduceBigBars))
      m_defaultMadvrSettings.m_reduceBigBars = -1;
    XMLUtils::GetBoolean(pElement, "cropsmallbars", m_defaultMadvrSettings.m_cropSmallBars);
    XMLUtils::GetBoolean(pElement, "cropbars", m_defaultMadvrSettings.m_cropBars);

    XMLUtils::GetBoolean(pElement, "uprefsharpenedges", m_defaultMadvrSettings.m_UpRefSharpenEdges);
    if (!XMLUtils::GetFloat(pElement, "uprefsharpenedgesstrength", m_defaultMadvrSettings.m_UpRefSharpenEdgesStrength))
      m_defaultMadvrSettings.m_UpRefSharpenEdgesStrength = MADVR_DEFAULT_UPSHARPENEDGES;
    XMLUtils::GetBoolean(pElement, "uprefcrispenedges", m_defaultMadvrSettings.m_UpRefCrispenEdges);
    if (!XMLUtils::GetFloat(pElement, "uprefcrispenedgesstrength", m_defaultMadvrSettings.m_UpRefCrispenEdgesStrength))
      m_defaultMadvrSettings.m_UpRefCrispenEdgesStrength = MADVR_DEFAULT_UPCRISPENEDGES;
    XMLUtils::GetBoolean(pElement, "uprefthinedges", m_defaultMadvrSettings.m_UpRefThinEdges);
    if (!XMLUtils::GetFloat(pElement, "upthinedgesstrength", m_defaultMadvrSettings.m_UpRefThinEdgesStrength))
      m_defaultMadvrSettings.m_UpRefThinEdgesStrength = MADVR_DEFAULT_UPTHINEDGES;
    XMLUtils::GetBoolean(pElement, "uprefenhancedetail", m_defaultMadvrSettings.m_UpRefEnhanceDetail);
    if (!XMLUtils::GetFloat(pElement, "uprefenhancedetailstrength", m_defaultMadvrSettings.m_UpRefEnhanceDetailStrength))
      m_defaultMadvrSettings.m_UpRefEnhanceDetailStrength = MADVR_DEFAULT_UPENHANCEDETAIL;
    XMLUtils::GetBoolean(pElement, "upreflumasharpen", m_defaultMadvrSettings.m_UpRefLumaSharpen);
    if (!XMLUtils::GetFloat(pElement, "upreflumasharpenstrength", m_defaultMadvrSettings.m_UpRefLumaSharpenStrength))
      m_defaultMadvrSettings.m_UpRefLumaSharpenStrength = MADVR_DEFAULT_UPLUMASHARPEN;
    XMLUtils::GetBoolean(pElement, "uprefadaptivesharpen", m_defaultMadvrSettings.m_UpRefAdaptiveSharpen);
    if (!XMLUtils::GetFloat(pElement, "uprefadaptivesharpenstrength", m_defaultMadvrSettings.m_UpRefAdaptiveSharpenStrength))
      m_defaultMadvrSettings.m_UpRefAdaptiveSharpenStrength = MADVR_DEFAULT_UPADAPTIVESHARPEN;
    XMLUtils::GetBoolean(pElement, "superres", m_defaultMadvrSettings.m_superRes);
    if (!XMLUtils::GetFloat(pElement, "superresstrength", m_defaultMadvrSettings.m_superResStrength))
      m_defaultMadvrSettings.m_superResStrength = MADVR_DEFAULT_SUPERRES;
    XMLUtils::GetBoolean(pElement, "superreslinear", m_defaultMadvrSettings.m_superResLinear);

  }
#endif
  // mymusic settings
  pElement = settings->FirstChildElement("mymusic");
  if (pElement != NULL)
  {
    const TiXmlElement *pChild = pElement->FirstChildElement("playlist");
    if (pChild != NULL)
    {
      XMLUtils::GetBoolean(pChild, "repeat", m_musicPlaylistRepeat);
      XMLUtils::GetBoolean(pChild, "shuffle", m_musicPlaylistShuffle);
    }
    if (!XMLUtils::GetInt(pElement, "needsupdate", m_musicNeedsUpdate, 0, INT_MAX))
      m_musicNeedsUpdate = 0;
  }
  
  // Read the watchmode settings for the various media views
  pElement = settings->FirstChildElement("myvideos");
  if (pElement != NULL)
  {
    int tmp;
    if (XMLUtils::GetInt(pElement, "watchmodemovies", tmp, (int)WatchedModeAll, (int)WatchedModeWatched))
      m_watchedModes["movies"] = (WatchedMode)tmp;
    if (XMLUtils::GetInt(pElement, "watchmodetvshows", tmp, (int)WatchedModeAll, (int)WatchedModeWatched))
      m_watchedModes["tvshows"] = (WatchedMode)tmp;
    if (XMLUtils::GetInt(pElement, "watchmodemusicvideos", tmp, (int)WatchedModeAll, (int)WatchedModeWatched))
      m_watchedModes["musicvideos"] = (WatchedMode)tmp;

    const TiXmlElement *pChild = pElement->FirstChildElement("playlist");
    if (pChild != NULL)
    {
      XMLUtils::GetBoolean(pChild, "repeat", m_videoPlaylistRepeat);
      XMLUtils::GetBoolean(pChild, "shuffle", m_videoPlaylistShuffle);
    }
    if (!XMLUtils::GetInt(pElement, "needsupdate", m_videoNeedsUpdate, 0, INT_MAX))
      m_videoNeedsUpdate = 0;
  }

  return true;
}

void CMediaSettings::OnSettingsLoaded()
{
  g_playlistPlayer.SetRepeat(PLAYLIST_MUSIC, m_musicPlaylistRepeat ? PLAYLIST::REPEAT_ALL : PLAYLIST::REPEAT_NONE);
  g_playlistPlayer.SetShuffle(PLAYLIST_MUSIC, m_musicPlaylistShuffle);
  g_playlistPlayer.SetRepeat(PLAYLIST_VIDEO, m_videoPlaylistRepeat ? PLAYLIST::REPEAT_ALL : PLAYLIST::REPEAT_NONE);
  g_playlistPlayer.SetShuffle(PLAYLIST_VIDEO, m_videoPlaylistShuffle);
}

bool CMediaSettings::Save(TiXmlNode *settings) const
{
  if (settings == NULL)
    return false;

  CSingleLock lock(m_critical);
  // default video settings
  TiXmlElement videoSettingsNode("defaultvideosettings");
  TiXmlNode *pNode = settings->InsertEndChild(videoSettingsNode);
  if (pNode == NULL)
    return false;

  XMLUtils::SetInt(pNode, "deinterlacemode", m_defaultVideoSettings.m_DeinterlaceMode);
  XMLUtils::SetInt(pNode, "interlacemethod", m_defaultVideoSettings.m_InterlaceMethod);
  XMLUtils::SetInt(pNode, "scalingmethod", m_defaultVideoSettings.m_ScalingMethod);
  XMLUtils::SetFloat(pNode, "noisereduction", m_defaultVideoSettings.m_NoiseReduction);
  XMLUtils::SetBoolean(pNode, "postprocess", m_defaultVideoSettings.m_PostProcess);
  XMLUtils::SetFloat(pNode, "sharpness", m_defaultVideoSettings.m_Sharpness);
  XMLUtils::SetInt(pNode, "viewmode", m_defaultVideoSettings.m_ViewMode);
  XMLUtils::SetFloat(pNode, "zoomamount", m_defaultVideoSettings.m_CustomZoomAmount);
  XMLUtils::SetFloat(pNode, "pixelratio", m_defaultVideoSettings.m_CustomPixelRatio);
  XMLUtils::SetFloat(pNode, "verticalshift", m_defaultVideoSettings.m_CustomVerticalShift);
  XMLUtils::SetFloat(pNode, "volumeamplification", m_defaultVideoSettings.m_VolumeAmplification);
  XMLUtils::SetBoolean(pNode, "outputtoallspeakers", m_defaultVideoSettings.m_OutputToAllSpeakers);
  XMLUtils::SetBoolean(pNode, "showsubtitles", m_defaultVideoSettings.m_SubtitleOn);
  XMLUtils::SetFloat(pNode, "brightness", m_defaultVideoSettings.m_Brightness);
  XMLUtils::SetFloat(pNode, "contrast", m_defaultVideoSettings.m_Contrast);
  XMLUtils::SetFloat(pNode, "gamma", m_defaultVideoSettings.m_Gamma);
  XMLUtils::SetFloat(pNode, "audiodelay", m_defaultVideoSettings.m_AudioDelay);
  XMLUtils::SetFloat(pNode, "subtitledelay", m_defaultVideoSettings.m_SubtitleDelay);
  XMLUtils::SetBoolean(pNode, "nonlinstretch", m_defaultVideoSettings.m_CustomNonLinStretch);
  XMLUtils::SetInt(pNode, "stereomode", m_defaultVideoSettings.m_StereoMode);

  // default audio settings for dsp addons
  TiXmlElement audioSettingsNode("defaultaudiosettings");
  pNode = settings->InsertEndChild(audioSettingsNode);
  if (pNode == NULL)
    return false;

  XMLUtils::SetInt(pNode, "masterstreamtype", m_defaultAudioSettings.m_MasterStreamType);
  XMLUtils::SetInt(pNode, "masterstreamtypesel", m_defaultAudioSettings.m_MasterStreamTypeSel);
  XMLUtils::SetInt(pNode, "masterstreambase", m_defaultAudioSettings.m_MasterStreamBase);

  std::string strTag;
  for (int type = AE_DSP_ASTREAM_BASIC; type < AE_DSP_ASTREAM_MAX; type++)
  {
    for (int base = AE_DSP_ABASE_STEREO; base < AE_DSP_ABASE_MAX; base++)
    {
      strTag = StringUtils::Format("masterprocess_%i_%i", type, base);
      XMLUtils::SetInt(pNode, strTag.c_str(), m_defaultAudioSettings.m_MasterModes[type][base]);
    }
  }

#ifdef HAS_DS_PLAYER
  //madvr settings
  TiXmlElement madvrSettingsNode("defaultmadvrsettings");
  pNode = settings->InsertEndChild(madvrSettingsNode);
  if (pNode == NULL)
    return false;

  XMLUtils::SetInt(pNode, "chromaupscaling", m_defaultMadvrSettings.m_ChromaUpscaling);
  XMLUtils::SetBoolean(pNode, "chromaantiring", m_defaultMadvrSettings.m_ChromaAntiRing);
  XMLUtils::SetBoolean(pNode, "chromasuperres", m_defaultMadvrSettings.m_ChromaSuperRes);
  XMLUtils::SetInt(pNode, "chromasuperrespasses", m_defaultMadvrSettings.m_ChromaSuperResPasses);
  XMLUtils::SetFloat(pNode, "chromasuperresstrength", m_defaultMadvrSettings.m_ChromaSuperResStrength);
  XMLUtils::SetFloat(pNode, "chromasuperressoftness", m_defaultMadvrSettings.m_ChromaSuperResSoftness);
  XMLUtils::SetInt(pNode, "imageupscaling", m_defaultMadvrSettings.m_ImageUpscaling);
  XMLUtils::SetBoolean(pNode, "imageupantiring", m_defaultMadvrSettings.m_ImageUpAntiRing);
  XMLUtils::SetBoolean(pNode, "imageuplinear", m_defaultMadvrSettings.m_ImageUpLinear);
  XMLUtils::SetInt(pNode, "imagedownscaling", m_defaultMadvrSettings.m_ImageDownscaling);
  XMLUtils::SetBoolean(pNode, "imagedownantiring", m_defaultMadvrSettings.m_ImageDownAntiRing);
  XMLUtils::SetBoolean(pNode, "imagedownlinear", m_defaultMadvrSettings.m_ImageDownLinear);

  XMLUtils::SetInt(pNode, "imagedoubleluma", m_defaultMadvrSettings.m_ImageDoubleLuma);
  XMLUtils::SetInt(pNode, "imagedoublechroma", m_defaultMadvrSettings.m_ImageDoubleChroma);
  XMLUtils::SetInt(pNode, "imagequadrupleluma", m_defaultMadvrSettings.m_ImageQuadrupleLuma);
  XMLUtils::SetInt(pNode, "imagequadruplechroma", m_defaultMadvrSettings.m_ImageQuadrupleChroma);

  XMLUtils::SetInt(pNode, "imagedoublelumafactor", m_defaultMadvrSettings.m_ImageDoubleLumaFactor);
  XMLUtils::SetInt(pNode, "imagedoublechromafactor", m_defaultMadvrSettings.m_ImageDoubleChromaFactor);
  XMLUtils::SetInt(pNode, "imagequadruplelumafactor", m_defaultMadvrSettings.m_ImageQuadrupleLumaFactor);
  XMLUtils::SetInt(pNode, "imagequadruplechromafactor", m_defaultMadvrSettings.m_ImageQuadrupleChromaFactor);

  XMLUtils::SetInt(pNode, "deintactive", m_defaultMadvrSettings.m_deintactive);
  XMLUtils::SetInt(pNode, "deintforce", m_defaultMadvrSettings.m_deintforce);
  XMLUtils::SetBoolean(pNode, "deintlookpixels", m_defaultMadvrSettings.m_deintlookpixels);

  XMLUtils::SetInt(pNode, "smoothmotion", m_defaultMadvrSettings.m_smoothMotion);

  XMLUtils::SetInt(pNode, "dithering", m_defaultMadvrSettings.m_dithering);
  XMLUtils::SetBoolean(pNode, "ditheringcolorednoise", m_defaultMadvrSettings.m_ditheringColoredNoise);
  XMLUtils::SetBoolean(pNode, "ditheringeveryframe", m_defaultMadvrSettings.m_ditheringEveryFrame);

  XMLUtils::SetBoolean(pNode, "deband", m_defaultMadvrSettings.m_deband); 
  XMLUtils::SetInt(pNode, "debandlevel", m_defaultMadvrSettings.m_debandLevel);
  XMLUtils::SetInt(pNode, "debandfadelevel", m_defaultMadvrSettings.m_debandFadeLevel);

  XMLUtils::SetBoolean(pNode, "sharpenedges", m_defaultMadvrSettings.m_sharpenEdges);
  XMLUtils::SetFloat(pNode, "sharpenedgesstrength", m_defaultMadvrSettings.m_sharpenEdgesStrength);
  XMLUtils::SetBoolean(pNode, "crispenedges", m_defaultMadvrSettings.m_crispenEdges);
  XMLUtils::SetFloat(pNode, "crispenedgesstrength", m_defaultMadvrSettings.m_crispenEdgesStrength);
  XMLUtils::SetBoolean(pNode, "thinedges", m_defaultMadvrSettings.m_thinEdges);
  XMLUtils::SetFloat(pNode, "thinedgesstrength", m_defaultMadvrSettings.m_thinEdgesStrength);
  XMLUtils::SetBoolean(pNode, "enhancedetail", m_defaultMadvrSettings.m_enhanceDetail);
  XMLUtils::SetFloat(pNode, "enhancedetailstrength", m_defaultMadvrSettings.m_enhanceDetailStrength);
  XMLUtils::SetBoolean(pNode, "lumasharpen", m_defaultMadvrSettings.m_lumaSharpen);
  XMLUtils::SetFloat(pNode, "lumasharpenstrength", m_defaultMadvrSettings.m_lumaSharpenStrength);
  XMLUtils::SetBoolean(pNode, "adpativesharpen", m_defaultMadvrSettings.m_adaptiveSharpen);
  XMLUtils::SetFloat(pNode, "adpativesharpenstrength", m_defaultMadvrSettings.m_adaptiveSharpenStrength);

  XMLUtils::SetInt(pNode, "nosmallscaling", m_defaultMadvrSettings.m_noSmallScaling);
  XMLUtils::SetInt(pNode, "movesubs", m_defaultMadvrSettings.m_moveSubs);
  XMLUtils::SetBoolean(pNode, "detectbars", m_defaultMadvrSettings.m_detectBars);
  XMLUtils::SetInt(pNode, "archange", m_defaultMadvrSettings.m_arChange);
  XMLUtils::SetInt(pNode, "quickarchange", m_defaultMadvrSettings.m_quickArChange);
  XMLUtils::SetInt(pNode, "shiftimage", m_defaultMadvrSettings.m_shiftImage);
  XMLUtils::SetInt(pNode, "dontcropsubs", m_defaultMadvrSettings.m_dontCropSubs);
  XMLUtils::SetInt(pNode, "cleanborders", m_defaultMadvrSettings.m_cleanBorders);
  XMLUtils::SetInt(pNode, "reducebigbars", m_defaultMadvrSettings.m_reduceBigBars);
  XMLUtils::SetBoolean(pNode, "cropsmallbars", m_defaultMadvrSettings.m_cropSmallBars);
  XMLUtils::SetBoolean(pNode, "cropbars", m_defaultMadvrSettings.m_cropBars);

  XMLUtils::SetBoolean(pNode, "uprefsharpenedges", m_defaultMadvrSettings.m_UpRefSharpenEdges);
  XMLUtils::SetFloat(pNode, "uprefsharpenedgesstrength", m_defaultMadvrSettings.m_UpRefSharpenEdgesStrength);
  XMLUtils::SetBoolean(pNode, "uprefcrispenedges", m_defaultMadvrSettings.m_UpRefCrispenEdges);
  XMLUtils::SetFloat(pNode, "uprefcrispenedgesstrength", m_defaultMadvrSettings.m_UpRefCrispenEdgesStrength);
  XMLUtils::SetBoolean(pNode, "uprefthinedges", m_defaultMadvrSettings.m_UpRefThinEdges);
  XMLUtils::SetFloat(pNode, "uprefthinedgesstrength", m_defaultMadvrSettings.m_UpRefThinEdgesStrength);
  XMLUtils::SetBoolean(pNode, "uprefenhancedetail", m_defaultMadvrSettings.m_UpRefEnhanceDetail);
  XMLUtils::SetFloat(pNode, "uprefenhancedetailstrength", m_defaultMadvrSettings.m_UpRefEnhanceDetailStrength);
  XMLUtils::SetBoolean(pNode, "upreflumasharpen", m_defaultMadvrSettings.m_UpRefLumaSharpen);
  XMLUtils::SetFloat(pNode, "upreflumasharpenstrength", m_defaultMadvrSettings.m_UpRefLumaSharpenStrength);
  XMLUtils::SetBoolean(pNode, "uprefadpativesharpen", m_defaultMadvrSettings.m_UpRefAdaptiveSharpen);
  XMLUtils::SetFloat(pNode, "uprefadpativesharpenstrength", m_defaultMadvrSettings.m_UpRefAdaptiveSharpenStrength);
  XMLUtils::SetBoolean(pNode, "superres", m_defaultMadvrSettings.m_superRes);
  XMLUtils::SetFloat(pNode, "superresstrength", m_defaultMadvrSettings.m_superResStrength);
  XMLUtils::SetBoolean(pNode, "superreslinear", m_defaultMadvrSettings.m_superResLinear);

  XMLUtils::SetBoolean(pNode, "refineonce", !m_defaultMadvrSettings.m_refineOnce);
#endif

  // mymusic
  pNode = settings->FirstChild("mymusic");
  if (pNode == NULL)
  {
    TiXmlElement videosNode("mymusic");
    pNode = settings->InsertEndChild(videosNode);
    if (pNode == NULL)
      return false;
  }

  TiXmlElement musicPlaylistNode("playlist");
  TiXmlNode *playlistNode = pNode->InsertEndChild(musicPlaylistNode);
  if (playlistNode == NULL)
    return false;
  XMLUtils::SetBoolean(playlistNode, "repeat", m_musicPlaylistRepeat);
  XMLUtils::SetBoolean(playlistNode, "shuffle", m_musicPlaylistShuffle);

  XMLUtils::SetInt(pNode, "needsupdate", m_musicNeedsUpdate);

  // myvideos
  pNode = settings->FirstChild("myvideos");
  if (pNode == NULL)
  {
    TiXmlElement videosNode("myvideos");
    pNode = settings->InsertEndChild(videosNode);
    if (pNode == NULL)
      return false;
  }

  XMLUtils::SetInt(pNode, "watchmodemovies", m_watchedModes.find("movies")->second);
  XMLUtils::SetInt(pNode, "watchmodetvshows", m_watchedModes.find("tvshows")->second);
  XMLUtils::SetInt(pNode, "watchmodemusicvideos", m_watchedModes.find("musicvideos")->second);

  TiXmlElement videoPlaylistNode("playlist");
  playlistNode = pNode->InsertEndChild(videoPlaylistNode);
  if (playlistNode == NULL)
    return false;
  XMLUtils::SetBoolean(playlistNode, "repeat", m_videoPlaylistRepeat);
  XMLUtils::SetBoolean(playlistNode, "shuffle", m_videoPlaylistShuffle);

  XMLUtils::SetInt(pNode, "needsupdate", m_videoNeedsUpdate);

  return true;
}

void CMediaSettings::OnSettingAction(const CSetting *setting)
{
  if (setting == NULL)
    return;

  const std::string &settingId = setting->GetId();
  if (settingId == CSettings::SETTING_MUSICLIBRARY_CLEANUP)
  {
    if (HELPERS::ShowYesNoDialogText(CVariant{313}, CVariant{333}) == DialogResponse::YES)
      g_application.StartMusicCleanup(true);
  }
  else if (settingId == CSettings::SETTING_MUSICLIBRARY_EXPORT)
    CBuiltins::GetInstance().Execute("exportlibrary(music)");
  else if (settingId == CSettings::SETTING_MUSICLIBRARY_IMPORT)
  {
    std::string path;
    VECSOURCES shares;
    g_mediaManager.GetLocalDrives(shares);
    g_mediaManager.GetNetworkLocations(shares);
    g_mediaManager.GetRemovableDrives(shares);

    if (CGUIDialogFileBrowser::ShowAndGetFile(shares, "musicdb.xml", g_localizeStrings.Get(651) , path))
    {
      CMusicDatabase musicdatabase;
      musicdatabase.Open();
      musicdatabase.ImportFromXML(path);
      musicdatabase.Close();
    }
  }
  else if (settingId == CSettings::SETTING_VIDEOLIBRARY_CLEANUP)
  {
    if (HELPERS::ShowYesNoDialogText(CVariant{313}, CVariant{333}) == DialogResponse::YES)
      g_application.StartVideoCleanup(true);
  }
  else if (settingId == CSettings::SETTING_VIDEOLIBRARY_EXPORT)
    CBuiltins::GetInstance().Execute("exportlibrary(video)");
#ifdef HAS_DS_PLAYER
  else if (settingId == CSettings::SETTING_DSPLAYER_RULES)
    CGUIDialogDSRules::ShowDSRulesList();
  else if (settingId == CSettings::SETTING_DSPLAYER_FILTERS)
    CGUIDialogDSFilters::ShowDSFiltersList();
  else if (settingId == CSettings::SETTING_DSPLAYER_PLAYCORE)
    CGUIDialogDSPlayercoreFactory::ShowDSPlayercoreFactory();
  else if (settingId == CSettings::SETTING_DSPLAYER_LAVSPLITTER)
    CGraphFilters::Get()->ShowInternalPPage(LAVSPLITTER, false);
  else if (settingId == CSettings::SETTING_DSPLAYER_LAVVIDEO)
    CGraphFilters::Get()->ShowInternalPPage(LAVVIDEO, false);
  else if (settingId == CSettings::SETTING_DSPLAYER_LAVAUDIO)
    CGraphFilters::Get()->ShowInternalPPage(LAVAUDIO, false);
  else if (settingId == CSettings::SETTING_DSPLAYER_XYSUBFILTER || settingId == CSettings::SETTING_DSPLAYER_XYVSFILTER)
    CGraphFilters::Get()->ShowInternalPPage(XYSUBFILTER, true);
#endif
  else if (settingId == CSettings::SETTING_VIDEOLIBRARY_IMPORT)
  {
    std::string path;
    VECSOURCES shares;
    g_mediaManager.GetLocalDrives(shares);
    g_mediaManager.GetNetworkLocations(shares);
    g_mediaManager.GetRemovableDrives(shares);

    if (CGUIDialogFileBrowser::ShowAndGetDirectory(shares, g_localizeStrings.Get(651) , path))
    {
      CVideoDatabase videodatabase;
      videodatabase.Open();
      videodatabase.ImportFromXML(path);
      videodatabase.Close();
    }
  }
}

int CMediaSettings::GetWatchedMode(const std::string &content) const
{
  CSingleLock lock(m_critical);
  WatchedModes::const_iterator it = m_watchedModes.find(GetWatchedContent(content));
  if (it != m_watchedModes.end())
    return it->second;

  return WatchedModeAll;
}

void CMediaSettings::SetWatchedMode(const std::string &content, WatchedMode mode)
{
  CSingleLock lock(m_critical);
  WatchedModes::iterator it = m_watchedModes.find(GetWatchedContent(content));
  if (it != m_watchedModes.end())
    it->second = mode;
}

void CMediaSettings::CycleWatchedMode(const std::string &content)
{
  CSingleLock lock(m_critical);
  WatchedModes::iterator it = m_watchedModes.find(GetWatchedContent(content));
  if (it != m_watchedModes.end())
  {
    it->second = (WatchedMode)((int)it->second + 1);
    if (it->second > WatchedModeWatched)
      it->second = WatchedModeAll;
  }
}

std::string CMediaSettings::GetWatchedContent(const std::string &content)
{
  if (content == "seasons" || content == "episodes")
    return "tvshows";

  return content;
}
