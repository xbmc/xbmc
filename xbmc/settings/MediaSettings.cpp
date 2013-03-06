/*
 *      Copyright (C) 2013 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include <limits.h>

#include "MediaSettings.h"
#include "guilib/Key.h"
#include "utils/XMLUtils.h"

using namespace std;

CMediaSettings::CMediaSettings()
{
  Clear();
}

CMediaSettings::~CMediaSettings()
{
  Clear();
}

CMediaSettings& CMediaSettings::Get()
{
  static CMediaSettings sMediaSettings;
  return sMediaSettings;
}

bool CMediaSettings::Load(const TiXmlNode *settings)
{
  if (settings == NULL)
    return false;

  // mymusic settings
  const TiXmlElement *pElement = settings->FirstChildElement("mymusic");
  if (pElement != NULL)
  {
      const TiXmlElement *pChild = pElement->FirstChildElement("playlist");
    if (pChild != NULL)
    {
      XMLUtils::GetBoolean(pChild, "repeat", m_bMyMusicPlaylistRepeat);
      XMLUtils::GetBoolean(pChild, "shuffle", m_bMyMusicPlaylistShuffle);
    }
    XMLUtils::GetInt(pElement, "startwindow", m_iMyMusicStartWindow, WINDOW_MUSIC_FILES, WINDOW_MUSIC_NAV); //501; view songs
    XMLUtils::GetBoolean(pElement, "songthumbinvis", m_bMyMusicSongThumbInVis);
    XMLUtils::GetInt(pElement, "needsupdate", m_musicNeedsUpdate, 0, INT_MAX);
    XMLUtils::GetString(pElement, "defaultlibview", m_defaultMusicLibSource);
  }

  // myvideos settings
  pElement = settings->FirstChildElement("myvideos");
  if (pElement != NULL)
  {
    XMLUtils::GetInt(pElement, "startwindow", m_iVideoStartWindow, WINDOW_VIDEO_FILES, WINDOW_VIDEO_NAV);
    XMLUtils::GetBoolean(pElement, "stackvideos", m_videoStacking);

    XMLUtils::GetBoolean(pElement, "flatten", m_bMyVideoNavFlatten);
    XMLUtils::GetInt(pElement, "needsupdate", m_videoNeedsUpdate, 0, INT_MAX);

    const TiXmlElement *pChild = pElement->FirstChildElement("playlist");
    if (pChild)
    { // playlist
      XMLUtils::GetBoolean(pChild, "repeat", m_bMyVideoPlaylistRepeat);
      XMLUtils::GetBoolean(pChild, "shuffle", m_bMyVideoPlaylistShuffle);
    }
  }

  pElement = settings->FirstChildElement("defaultvideosettings");
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
      {
        deinterlaceMode = VS_DEINTERLACEMODE_AUTO;
      }
      else
      {
        deinterlaceMode = VS_DEINTERLACEMODE_FORCE;
      }
    }
    m_defaultVideoSettings.m_DeinterlaceMode = (EDEINTERLACEMODE)deinterlaceMode;
    m_defaultVideoSettings.m_InterlaceMethod = (EINTERLACEMETHOD)interlaceMethod;
    int scalingMethod;
    if (!XMLUtils::GetInt(pElement, "scalingmethod", scalingMethod, VS_SCALINGMETHOD_NEAREST, VS_SCALINGMETHOD_MAX))
      scalingMethod = (int)VS_SCALINGMETHOD_LINEAR;
    m_defaultVideoSettings.m_ScalingMethod = (ESCALINGMETHOD)scalingMethod;

    XMLUtils::GetInt(pElement, "viewmode", m_defaultVideoSettings.m_ViewMode, VIEW_MODE_NORMAL, VIEW_MODE_CUSTOM);
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
    XMLUtils::GetBoolean(pElement, "autocrop", m_defaultVideoSettings.m_Crop);
    XMLUtils::GetBoolean(pElement, "nonlinstretch", m_defaultVideoSettings.m_CustomNonLinStretch);

    m_defaultVideoSettings.m_SubtitleCached = false;
  }

  // Read the watchmode settings for the various media views
  int tmp;
  if (XMLUtils::GetInt(settings, "watchmodemovies", tmp, (int)WatchedModeAll, (int)WatchedModeWatched))
    m_watchedModes["movies"] = (WatchedMode)tmp;
  if (XMLUtils::GetInt(settings, "watchmodetvshows", tmp, (int)WatchedModeAll, (int)WatchedModeWatched))
    m_watchedModes["tvshows"] = (WatchedMode)tmp;
  if (XMLUtils::GetInt(settings, "watchmodemusicvideos", tmp, (int)WatchedModeAll, (int)WatchedModeWatched))
    m_watchedModes["musicvideos"] = (WatchedMode)tmp;

  return true;
}

bool CMediaSettings::Save(TiXmlNode *settings) const
{
  if (settings == NULL)
    return false;

  // mymusic settings
  TiXmlElement musicNode("mymusic");
  TiXmlNode *pNode = settings->InsertEndChild(musicNode);
  if (pNode == NULL)
    return false;

  {
    TiXmlElement childNode("playlist");
    TiXmlNode *pChild = pNode->InsertEndChild(childNode);
    if (!pChild) return false;
    XMLUtils::SetBoolean(pChild, "repeat", m_bMyMusicPlaylistRepeat);
    XMLUtils::SetBoolean(pChild, "shuffle", m_bMyMusicPlaylistShuffle);
  }

  XMLUtils::SetInt(pNode, "needsupdate", m_musicNeedsUpdate);
  XMLUtils::SetInt(pNode, "startwindow", m_iMyMusicStartWindow);
  XMLUtils::SetBoolean(pNode, "songthumbinvis", m_bMyMusicSongThumbInVis);
  XMLUtils::SetPath(pNode, "defaultlibview", m_defaultMusicLibSource);

  // myvideos settings
  TiXmlElement videosNode("myvideos");
  pNode = settings->InsertEndChild(videosNode);
  if (pNode == NULL)
    return false;

  XMLUtils::SetInt(pNode, "startwindow", m_iVideoStartWindow);

  XMLUtils::SetBoolean(pNode, "stackvideos", m_videoStacking);

  XMLUtils::SetInt(pNode, "needsupdate", m_videoNeedsUpdate);
  XMLUtils::SetBoolean(pNode, "flatten", m_bMyVideoNavFlatten);

  { // playlist window
    TiXmlElement childNode("playlist");
    TiXmlNode *pChild = pNode->InsertEndChild(childNode);
    if (!pChild) return false;
    XMLUtils::SetBoolean(pChild, "repeat", m_bMyVideoPlaylistRepeat);
    XMLUtils::SetBoolean(pChild, "shuffle", m_bMyVideoPlaylistShuffle);
  }

  // default video settings
  TiXmlElement videoSettingsNode("defaultvideosettings");
  pNode = settings->InsertEndChild(videoSettingsNode);
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
  XMLUtils::SetBoolean(pNode, "autocrop", m_defaultVideoSettings.m_Crop);
  XMLUtils::SetBoolean(pNode, "nonlinstretch", m_defaultVideoSettings.m_CustomNonLinStretch);

  XMLUtils::SetInt(settings, "watchmodemovies", (int)m_watchedModes.find("movies")->second);
  XMLUtils::SetInt(settings, "watchmodetvshows", (int)m_watchedModes.find("tvshows")->second);
  XMLUtils::SetInt(settings, "watchmodemusicvideos", (int)m_watchedModes.find("musicvideos")->second);

  return true;
}

void CMediaSettings::Clear()
{
  m_pictureExtensions = ".png|.jpg|.jpeg|.bmp|.gif|.ico|.tif|.tiff|.tga|.pcx|.cbz|.zip|.cbr|.rar|.m3u|.dng|.nef|.cr2|.crw|.orf|.arw|.erf|.3fr|.dcr|.x3f|.mef|.raf|.mrw|.pef|.sr2|.rss";
  m_musicExtensions = ".nsv|.m4a|.flac|.aac|.strm|.pls|.rm|.rma|.mpa|.wav|.wma|.ogg|.mp3|.mp2|.m3u|.mod|.amf|.669|.dmf|.dsm|.far|.gdm|.imf|.it|.m15|.med|.okt|.s3m|.stm|.sfx|.ult|.uni|.xm|.sid|.ac3|.dts|.cue|.aif|.aiff|.wpl|.ape|.mac|.mpc|.mp+|.mpp|.shn|.zip|.rar|.wv|.nsf|.spc|.gym|.adx|.dsp|.adp|.ymf|.ast|.afc|.hps|.xsp|.xwav|.waa|.wvs|.wam|.gcm|.idsp|.mpdsp|.mss|.spt|.rsd|.mid|.kar|.sap|.cmc|.cmr|.dmc|.mpt|.mpd|.rmt|.tmc|.tm8|.tm2|.oga|.url|.pxml|.tta|.rss|.cm3|.cms|.dlt|.brstm|.wtv|.mka";
  m_videoExtensions = ".m4v|.3g2|.3gp|.nsv|.tp|.ts|.ty|.strm|.pls|.rm|.rmvb|.m3u|.m3u8|.ifo|.mov|.qt|.divx|.xvid|.bivx|.vob|.nrg|.img|.iso|.pva|.wmv|.asf|.asx|.ogm|.m2v|.avi|.bin|.dat|.mpg|.mpeg|.mp4|.mkv|.avc|.vp3|.svq3|.nuv|.viv|.dv|.fli|.flv|.rar|.001|.wpl|.zip|.vdr|.dvr-ms|.xsp|.mts|.m2t|.m2ts|.evo|.ogv|.sdp|.avs|.rec|.url|.pxml|.vc1|.h264|.rcv|.rss|.mpls|.webm|.bdmv|.wtv";
  m_discStubExtensions = ".disc";
  // internal music extensions
  m_musicExtensions += "|.sidstream|.oggstream|.nsfstream|.asapstream|.cdda";
  // internal video extensions
  m_videoExtensions += "|.pvr";

  m_bMyMusicSongThumbInVis = false;  // used for music info in vis screen
  m_bMyMusicPlaylistRepeat = false;
  m_bMyMusicPlaylistShuffle = false;
  m_iMyMusicStartWindow = WINDOW_MUSIC_FILES;

  m_bMyVideoPlaylistRepeat = false;
  m_bMyVideoPlaylistShuffle = false;
  m_bMyVideoNavFlatten = false;
  m_bStartVideoWindowed = false;
  m_iVideoStartWindow = WINDOW_VIDEO_FILES;
  m_videoStacking = false;

  iAdditionalSubtitleDirectoryChecked = 0;

  m_musicNeedsUpdate = 0;
  m_videoNeedsUpdate = 0;

  m_watchedModes["movies"] = WatchedModeAll;
  m_watchedModes["tvshows"] = WatchedModeAll;
  m_watchedModes["musicvideos"] = WatchedModeAll;
}

WatchedMode CMediaSettings::GetWatchedMode(const std::string& content) const
{
  map<string, WatchedMode>::const_iterator it = m_watchedModes.find(GetWatchedContent(content));
  if (it != m_watchedModes.end())
    return it->second;

  return WatchedModeAll;
}

void CMediaSettings::SetWatchedMode(const std::string& content, WatchedMode value)
{
  map<string, WatchedMode>::iterator it = m_watchedModes.find(GetWatchedContent(content));
  if (it != m_watchedModes.end())
    it->second = value;
}

void CMediaSettings::CycleWatchedMode(const std::string& content)
{
  map<string, WatchedMode>::iterator it = m_watchedModes.find(GetWatchedContent(content));
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