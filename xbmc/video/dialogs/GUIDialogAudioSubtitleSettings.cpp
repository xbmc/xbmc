/*
 *      Copyright (C) 2005-2012 Team XBMC
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

#include "system.h"
#include "GUIDialogAudioSubtitleSettings.h"
#include "dialogs/GUIDialogFileBrowser.h"
#include "GUIPassword.h"
#include "utils/URIUtils.h"
#include "Application.h"
#include "video/VideoDatabase.h"
#include "dialogs/GUIDialogYesNo.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "URL.h"
#include "FileItem.h"
#include "addons/Skin.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "settings/GUISettings.h"
#include "guilib/LocalizeStrings.h"
#include "pvr/PVRManager.h"
#include "cores/AudioEngine/Utils/AEUtil.h"

using namespace std;
using namespace XFILE;
using namespace PVR;

#ifdef HAS_VIDEO_PLAYBACK
extern void xbox_audio_switch_channel(int iAudioStream, bool bAudioOnAllSpeakers); //lowlevel audio
#endif

CGUIDialogAudioSubtitleSettings::CGUIDialogAudioSubtitleSettings(void)
    : CGUIDialogSettings(WINDOW_DIALOG_AUDIO_OSD_SETTINGS, "VideoOSDSettings.xml")
{
}

CGUIDialogAudioSubtitleSettings::~CGUIDialogAudioSubtitleSettings(void)
{
}

#define AUDIO_SETTINGS_VOLUME             1
#define AUDIO_SETTINGS_VOLUME_AMPLIFICATION 2
#define AUDIO_SETTINGS_DELAY              3
#define AUDIO_SETTINGS_STREAM             4
#define AUDIO_SETTINGS_OUTPUT_TO_ALL_SPEAKERS 5
#define AUDIO_SETTINGS_DIGITAL_ANALOG     6

// separator 7
#define SUBTITLE_SETTINGS_ENABLE          8
#define SUBTITLE_SETTINGS_DELAY           9
#define SUBTITLE_SETTINGS_STREAM          10
#define SUBTITLE_SETTINGS_BROWSER         11
#define AUDIO_SETTINGS_MAKE_DEFAULT       12

void CGUIDialogAudioSubtitleSettings::CreateSettings()
{
  m_usePopupSliders = g_SkinInfo->HasSkinFile("DialogSlider.xml");

  if (g_application.m_pPlayer)
  {
    g_application.m_pPlayer->GetAudioCapabilities(m_audioCaps);
    g_application.m_pPlayer->GetSubtitleCapabilities(m_subCaps);
  }

  // clear out any old settings
  m_settings.clear();
  // create our settings
  m_volume = g_settings.m_fVolumeLevel;
  AddSlider(AUDIO_SETTINGS_VOLUME, 13376, &m_volume, VOLUME_MINIMUM, VOLUME_MAXIMUM / 100.0f, VOLUME_MAXIMUM, PercentAsDecibel, false);
  if (SupportsAudioFeature(IPC_AUD_AMP))
    AddSlider(AUDIO_SETTINGS_VOLUME_AMPLIFICATION, 660, &g_settings.m_currentVideoSettings.m_VolumeAmplification, VOLUME_DRC_MINIMUM * 0.01f, (VOLUME_DRC_MAXIMUM - VOLUME_DRC_MINIMUM) / 6000.0f, VOLUME_DRC_MAXIMUM * 0.01f, FormatDecibel, false);
  if (g_application.m_pPlayer && g_application.m_pPlayer->IsPassthrough())
  {
    EnableSettings(AUDIO_SETTINGS_VOLUME,false);
    EnableSettings(AUDIO_SETTINGS_VOLUME_AMPLIFICATION,false);
  }
  if (SupportsAudioFeature(IPC_AUD_OFFSET))
    AddSlider(AUDIO_SETTINGS_DELAY, 297, &g_settings.m_currentVideoSettings.m_AudioDelay, -g_advancedSettings.m_videoAudioDelayRange, .025f, g_advancedSettings.m_videoAudioDelayRange, FormatDelay);
  if (SupportsAudioFeature(IPC_AUD_SELECT_STREAM))
    AddAudioStreams(AUDIO_SETTINGS_STREAM);

  // only show stuff available in digital mode if we have digital output
  if (SupportsAudioFeature(IPC_AUD_OUTPUT_STEREO))
    AddBool(AUDIO_SETTINGS_OUTPUT_TO_ALL_SPEAKERS, 252, &g_settings.m_currentVideoSettings.m_OutputToAllSpeakers, AUDIO_IS_BITSTREAM(g_guiSettings.GetInt("audiooutput.mode")));

  int settings[3] = { 338, 339, 420 }; //ANALOG, IEC958, HDMI
  m_outputmode = g_guiSettings.GetInt("audiooutput.mode");
  if (SupportsAudioFeature(IPC_AUD_SELECT_OUTPUT))
    AddSpin(AUDIO_SETTINGS_DIGITAL_ANALOG, 337, &m_outputmode, 3, settings);

  AddSeparator(7);
  m_subtitleVisible = g_application.m_pPlayer->GetSubtitleVisible();
  AddBool(SUBTITLE_SETTINGS_ENABLE, 13397, &m_subtitleVisible);
  if (SupportsSubtitleFeature(IPC_SUBS_OFFSET))
    AddSlider(SUBTITLE_SETTINGS_DELAY, 22006, &g_settings.m_currentVideoSettings.m_SubtitleDelay, -g_advancedSettings.m_videoSubsDelayRange, 0.1f, g_advancedSettings.m_videoSubsDelayRange, FormatDelay);
  if (SupportsSubtitleFeature(IPC_SUBS_SELECT))
    AddSubtitleStreams(SUBTITLE_SETTINGS_STREAM);
  if (SupportsSubtitleFeature(IPC_SUBS_EXTERNAL))
    AddButton(SUBTITLE_SETTINGS_BROWSER,13250);
  AddButton(AUDIO_SETTINGS_MAKE_DEFAULT, 12376);
}

void CGUIDialogAudioSubtitleSettings::AddAudioStreams(unsigned int id)
{
  SettingInfo setting;
  setting.id = id;
  setting.name = g_localizeStrings.Get(460);
  setting.type = SettingInfo::SPIN;
  setting.min = 0;
  setting.data = &m_audioStream;

  // get the number of audio strams for the current movie
  setting.max = (float)g_application.m_pPlayer->GetAudioStreamCount() - 1;
  m_audioStream = g_application.m_pPlayer->GetAudioStream();

  if( m_audioStream < 0 ) m_audioStream = 0;

  // check if we have a single, stereo stream, and if so, allow us to split into
  // left, right or both
  if (!setting.max)
  {
    CStdString strAudioInfo;
    g_application.m_pPlayer->GetAudioInfo(strAudioInfo);
    int iNumChannels = atoi(strAudioInfo.Right(strAudioInfo.size() - strAudioInfo.Find("chns:") - 5).c_str());
    CStdString strAudioCodec = strAudioInfo.Mid(7, strAudioInfo.Find(") VBR") - 5);
    bool bDTS = strstr(strAudioCodec.c_str(), "DTS") != 0;
    bool bAC3 = strstr(strAudioCodec.c_str(), "AC3") != 0;
    if (iNumChannels == 2 && !(bDTS || bAC3))
    { // ok, enable these options
/*      if (g_settings.m_currentVideoSettings.m_AudioStream == -1)
      { // default to stereo stream
        g_settings.m_currentVideoSettings.m_AudioStream = 0;
      }*/
      setting.max = 2;
      for (int i = 0; i <= setting.max; i++)
        setting.entry.push_back(make_pair(setting.entry.size(), g_localizeStrings.Get(13320 + i)));
      m_audioStream = -g_settings.m_currentVideoSettings.m_AudioStream - 1;
      m_settings.push_back(setting);
      return;
    }
  }

  // cycle through each audio stream and add it to our list control
  for (int i = 0; i <= setting.max; ++i)
  {
    CStdString strItem;
    CStdString strName;
    g_application.m_pPlayer->GetAudioStreamName(i, strName);
    if (strName.length() == 0)
      strName = "Unnamed";

    strItem.Format("%s (%i/%i)", strName.c_str(), i + 1, (int)setting.max + 1);
    setting.entry.push_back(make_pair(setting.entry.size(), strItem));
  }

  if( setting.max < 0 )
  {
    setting.max = 0;
    setting.entry.push_back(make_pair(setting.entry.size(), g_localizeStrings.Get(231)));
  }

  m_settings.push_back(setting);
}

void CGUIDialogAudioSubtitleSettings::AddSubtitleStreams(unsigned int id)
{
  SettingInfo setting;

  setting.id = id;
  setting.name = g_localizeStrings.Get(462);
  setting.type = SettingInfo::SPIN;
  setting.min = 0;
  setting.data = &m_subtitleStream;
  m_subtitleStream = g_application.m_pPlayer->GetSubtitle();

  if(m_subtitleStream < 0) m_subtitleStream = 0;

  // get the number of audio strams for the current movie
  setting.max = (float)g_application.m_pPlayer->GetSubtitleCount() - 1;

  // cycle through each subtitle and add it to our entry list
  for (int i = 0; i <= setting.max; ++i)
  {
    CStdString strItem;
    CStdString strName;
    g_application.m_pPlayer->GetSubtitleName(i, strName);
    if (strName.length() == 0)
      strName = "Unnamed";

    CStdString strLanguage;
    g_application.m_pPlayer->GetSubtitleLanguage(i, strLanguage);

    if (strName != strLanguage)
      strName.Format("%s [%s]", strName.c_str(), strLanguage.c_str());

    strItem.Format("%s (%i/%i)", strName.c_str(), i + 1, (int)setting.max + 1);

    setting.entry.push_back(make_pair(setting.entry.size(), strItem));
  }

  if (setting.max < 0)
  { // no subtitle streams - just add a "None" entry
    m_subtitleStream = 0;
    setting.max = 0;
    setting.entry.push_back(make_pair(setting.entry.size(), g_localizeStrings.Get(231)));
  }
  m_settings.push_back(setting);
}

void CGUIDialogAudioSubtitleSettings::OnSettingChanged(SettingInfo &setting)
{
  // check and update anything that needs it
  if (setting.id == AUDIO_SETTINGS_VOLUME)
  {
    g_settings.m_fVolumeLevel = m_volume;
    g_application.SetVolume(m_volume, false);//false - value is not in percent
  }
  else if (setting.id == AUDIO_SETTINGS_VOLUME_AMPLIFICATION)
  {
    if (g_application.m_pPlayer)
      g_application.m_pPlayer->SetDynamicRangeCompression((long)(g_settings.m_currentVideoSettings.m_VolumeAmplification * 100));
  }
  else if (setting.id == AUDIO_SETTINGS_DELAY)
  {
    if (g_application.m_pPlayer)
      g_application.m_pPlayer->SetAVDelay(g_settings.m_currentVideoSettings.m_AudioDelay);
  }
  else if (setting.id == AUDIO_SETTINGS_STREAM)
  {
    // first check if it's a stereo track that we can change between stereo, left and right
    if (g_application.m_pPlayer->GetAudioStreamCount() == 1)
    {
      if (setting.max == 2)
      { // we're in the case we want - call the code to switch channels etc.
        // update the screen setting...
        g_settings.m_currentVideoSettings.m_AudioStream = -1 - m_audioStream;
        // call monkeyh1's code here...
        //bool bAudioOnAllSpeakers = (g_guiSettings.GetInt("audiooutput.mode") == AUDIO_IEC958) && g_settings.m_currentVideoSettings.m_OutputToAllSpeakers;
        return;
      }
    }
    // only change the audio stream if a different one has been asked for
    if (g_application.m_pPlayer->GetAudioStream() != m_audioStream)
    {
      g_settings.m_currentVideoSettings.m_AudioStream = m_audioStream;
      g_application.m_pPlayer->SetAudioStream(m_audioStream);    // Set the audio stream to the one selected
      EnableSettings(AUDIO_SETTINGS_VOLUME, !g_application.m_pPlayer->IsPassthrough());
    }
  }
  else if (setting.id == AUDIO_SETTINGS_OUTPUT_TO_ALL_SPEAKERS)
  {
    g_application.Restart();
  }
  else if (setting.id == AUDIO_SETTINGS_DIGITAL_ANALOG)
  {
    bool bitstream = false;

    switch(m_outputmode)
    {
      case 0: g_guiSettings.SetInt("audiooutput.mode", AUDIO_ANALOG ); break;
      case 1: g_guiSettings.SetInt("audiooutput.mode", AUDIO_IEC958 ); bitstream = true; break;
      case 2: g_guiSettings.SetInt("audiooutput.mode", AUDIO_HDMI   ); bitstream = true; break;
    }

    EnableSettings(AUDIO_SETTINGS_OUTPUT_TO_ALL_SPEAKERS, bitstream);
    g_application.Restart();
    EnableSettings(AUDIO_SETTINGS_VOLUME, !g_application.m_pPlayer->IsPassthrough());
  }
  else if (setting.id == SUBTITLE_SETTINGS_ENABLE)
  {
    g_settings.m_currentVideoSettings.m_SubtitleOn = m_subtitleVisible;
    g_application.m_pPlayer->SetSubtitleVisible(g_settings.m_currentVideoSettings.m_SubtitleOn);
  }
  else if (setting.id == SUBTITLE_SETTINGS_DELAY)
  {
    g_application.m_pPlayer->SetSubTitleDelay(g_settings.m_currentVideoSettings.m_SubtitleDelay);
  }
  else if (setting.id == SUBTITLE_SETTINGS_STREAM && setting.max > 0)
  {
    g_settings.m_currentVideoSettings.m_SubtitleStream = m_subtitleStream;
    g_application.m_pPlayer->SetSubtitle(m_subtitleStream);
  }
  else if (setting.id == SUBTITLE_SETTINGS_BROWSER)
  {
    CStdString strPath;
    if (URIUtils::IsInRAR(g_application.CurrentFileItem().GetPath()) || URIUtils::IsInZIP(g_application.CurrentFileItem().GetPath()))
    {
      CURL url(g_application.CurrentFileItem().GetPath());
      strPath = url.GetHostName();
    }
    else
      strPath = g_application.CurrentFileItem().GetPath();

    CStdString strMask = ".utf|.utf8|.utf-8|.sub|.srt|.smi|.rt|.txt|.ssa|.aqt|.jss|.ass|.idx|.rar|.zip";
    if (g_application.GetCurrentPlayer() == EPC_DVDPLAYER)
      strMask = ".srt|.rar|.zip|.ifo|.smi|.sub|.idx|.ass|.ssa|.txt";
    VECSOURCES shares(g_settings.m_videoSources);
    if (g_settings.iAdditionalSubtitleDirectoryChecked != -1 && !g_guiSettings.GetString("subtitles.custompath").IsEmpty())
    {
      CMediaSource share;
      std::vector<CStdString> paths;
      CStdString strPath1;
      URIUtils::GetDirectory(strPath,strPath1);
      paths.push_back(strPath1);
      strPath1 = g_guiSettings.GetString("subtitles.custompath");
      paths.push_back(g_guiSettings.GetString("subtitles.custompath"));
      share.FromNameAndPaths("video",g_localizeStrings.Get(21367),paths);
      shares.push_back(share);
      strPath = share.strPath;
      URIUtils::AddSlashAtEnd(strPath);
    }
    if (CGUIDialogFileBrowser::ShowAndGetFile(shares,strMask,g_localizeStrings.Get(293),strPath,false,true)) // "subtitles"
    {
      if (URIUtils::GetExtension(strPath) == ".sub")
        if (CFile::Exists(URIUtils::ReplaceExtension(strPath, ".idx")))
          strPath = URIUtils::ReplaceExtension(strPath, ".idx");
      
      int id = g_application.m_pPlayer->AddSubtitle(strPath);
      if(id >= 0)
      {
        m_subtitleStream = id;
        g_application.m_pPlayer->SetSubtitle(m_subtitleStream);
        g_application.m_pPlayer->SetSubtitleVisible(true);
      }
      g_settings.m_currentVideoSettings.m_SubtitleCached = true;
      Close();
    }
  }
  else if (setting.id == AUDIO_SETTINGS_MAKE_DEFAULT)
  {
    if (g_settings.GetCurrentProfile().settingsLocked() &&
        g_settings.GetMasterProfile().getLockMode() != ::LOCK_MODE_EVERYONE)
      if (!g_passwordManager.IsMasterLockUnlocked(true))
        return;

    // prompt user if they are sure
    if (CGUIDialogYesNo::ShowAndGetInput(12376, 750, 0, 12377))
    { // reset the settings
      CVideoDatabase db;
      db.Open();
      db.EraseVideoSettings();
      db.Close();
      g_settings.m_defaultVideoSettings = g_settings.m_currentVideoSettings;
      g_settings.m_defaultVideoSettings.m_SubtitleStream = -1;
      g_settings.m_defaultVideoSettings.m_AudioStream = -1;
      g_settings.Save();
    }
  }

  if (g_PVRManager.IsPlayingRadio() || g_PVRManager.IsPlayingTV())
    g_PVRManager.TriggerSaveChannelSettings();
}

void CGUIDialogAudioSubtitleSettings::FrameMove()
{
  m_volume = g_settings.m_fVolumeLevel;
  UpdateSetting(AUDIO_SETTINGS_VOLUME);
  if (g_application.m_pPlayer)
  {
    // these settings can change on the fly
    UpdateSetting(AUDIO_SETTINGS_DELAY);
    UpdateSetting(SUBTITLE_SETTINGS_ENABLE);
    UpdateSetting(SUBTITLE_SETTINGS_DELAY);
  }
  CGUIDialogSettings::FrameMove();
}

CStdString CGUIDialogAudioSubtitleSettings::PercentAsDecibel(float value, float interval)
{
  CStdString text;
  text.Format("%2.1f dB", CAEUtil::PercentToGain(value));
  return text;
}

CStdString CGUIDialogAudioSubtitleSettings::FormatDecibel(float value, float interval)
{
  CStdString text;
  text.Format("%2.1f dB", value);
  return text;
}

CStdString CGUIDialogAudioSubtitleSettings::FormatDelay(float value, float interval)
{
  CStdString text;
  if (fabs(value) < 0.5f*interval)
    text.Format(g_localizeStrings.Get(22003).c_str(), 0.0);
  else if (value < 0)
    text.Format(g_localizeStrings.Get(22004).c_str(), fabs(value));
  else
    text.Format(g_localizeStrings.Get(22005).c_str(), value);
  return text;
}

bool CGUIDialogAudioSubtitleSettings::SupportsAudioFeature(int feature)
{
  for (Features::iterator itr = m_audioCaps.begin(); itr != m_audioCaps.end(); itr++)
  {
    if(*itr == feature || *itr == IPC_AUD_ALL)
      return true;
  }
  return false;
}

bool CGUIDialogAudioSubtitleSettings::SupportsSubtitleFeature(int feature)
{
  for (Features::iterator itr = m_subCaps.begin(); itr != m_subCaps.end(); itr++)
  {
    if(*itr == feature || *itr == IPC_SUBS_ALL)
      return true;
  }
  return false;
}
