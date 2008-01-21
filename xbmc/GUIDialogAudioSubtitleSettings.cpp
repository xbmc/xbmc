/*
 *      Copyright (C) 2005-2007 Team XboxMediaCenter
 *      http://www.xboxmediacenter.com
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
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include "GUIDialogAudioSubtitleSettings.h"
#include "GUIDialogFileBrowser.h"
#include "GUIPassword.h"
#include "Util.h"
#include "Application.h"
#include "VideoDatabase.h"
#include "XBAudioConfig.h"

using namespace XFILE;
using namespace DIRECTORY;

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
#define AUDIO_SETTINGS_DIGITAL_ANALOG 6

// separator 7
#define SUBTITLE_SETTINGS_ENABLE          8
#define SUBTITLE_SETTINGS_DELAY           9
#define SUBTITLE_SETTINGS_STREAM          10
#define SUBTITLE_SETTINGS_BROWSER        11
#define AUDIO_SETTINGS_MAKE_DEFAULT      12

void CGUIDialogAudioSubtitleSettings::CreateSettings()
{
  // clear out any old settings
  m_settings.clear();
  // create our settings
  m_volume = g_stSettings.m_nVolumeLevel * 0.01f;
  AddSlider(AUDIO_SETTINGS_VOLUME, 13376, &m_volume, VOLUME_MINIMUM * 0.01f, (VOLUME_MAXIMUM - VOLUME_MINIMUM) * 0.0001f, VOLUME_MAXIMUM * 0.01f, "%2.1f dB");
  AddSlider(AUDIO_SETTINGS_VOLUME_AMPLIFICATION, 660, &g_stSettings.m_currentVideoSettings.m_VolumeAmplification, VOLUME_DRC_MINIMUM * 0.01f, (VOLUME_DRC_MAXIMUM - VOLUME_DRC_MINIMUM) * 0.0005f, VOLUME_DRC_MAXIMUM * 0.01f, "%2.1f dB");
  AddSlider(AUDIO_SETTINGS_DELAY, 297, &g_stSettings.m_currentVideoSettings.m_AudioDelay, -g_advancedSettings.m_videoAudioDelayRange, 0.1f, g_advancedSettings.m_videoAudioDelayRange, "%2.1fs");
  AddAudioStreams(AUDIO_SETTINGS_STREAM);

  // only show stuff available in digital mode if we have digital output
  if(g_audioConfig.HasDigitalOutput())
  {
    AddBool(AUDIO_SETTINGS_OUTPUT_TO_ALL_SPEAKERS, 252, &g_stSettings.m_currentVideoSettings.m_OutputToAllSpeakers, g_guiSettings.GetInt("audiooutput.mode") == AUDIO_DIGITAL);

    int settings[2] = { 338, 339 }; //ANALOG, DIGITAL
    m_outputmode = g_guiSettings.GetInt("audiooutput.mode");
    AddSpin(AUDIO_SETTINGS_DIGITAL_ANALOG, 337, &m_outputmode, 2, settings);
  }

  AddSeparator(7);
  m_subtitleVisible = g_application.m_pPlayer->GetSubtitleVisible();
  AddBool(SUBTITLE_SETTINGS_ENABLE, 13397, &m_subtitleVisible);
  AddSlider(SUBTITLE_SETTINGS_DELAY, 303, &g_stSettings.m_currentVideoSettings.m_SubtitleDelay, -g_advancedSettings.m_videoSubsDelayRange, 0.1f, g_advancedSettings.m_videoSubsDelayRange, "%2.1fs");
  AddSubtitleStreams(SUBTITLE_SETTINGS_STREAM);
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
/*      if (g_stSettings.m_currentVideoSettings.m_AudioStream == -1)
      { // default to stereo stream
        g_stSettings.m_currentVideoSettings.m_AudioStream = 0;
      }*/
      setting.max = 2;
      for (int i = 0; i <= setting.max; i++)
        setting.entry.push_back(g_localizeStrings.Get(13320 + i));
      m_audioStream = -g_stSettings.m_currentVideoSettings.m_AudioStream - 1;
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
    setting.entry.push_back(strItem);
  }

  if( setting.max < 0 )
  {
    setting.max = 0;
    setting.entry.push_back(g_localizeStrings.Get(231).c_str());
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

    strItem.Format("%s (%i/%i)", strName.c_str(), i + 1, (int)setting.max + 1);

    setting.entry.push_back(strItem);
  }

  if (setting.max < 0)
  { // no subtitle streams - just add a "None" entry
    m_subtitleStream = 0;
    setting.max = 0;
    setting.entry.push_back(g_localizeStrings.Get(231).c_str());
  }
  m_settings.push_back(setting);
}

void CGUIDialogAudioSubtitleSettings::OnSettingChanged(unsigned int num)
{
  // setting has changed - update anything that needs it
  if (num >= m_settings.size()) return;
  SettingInfo &setting = m_settings.at(num);
  // check and update anything that needs it
  if (setting.id == AUDIO_SETTINGS_VOLUME)
  {
    g_stSettings.m_nVolumeLevel = (long)(m_volume * 100.0f);
    g_application.SetVolume(int(((float)(g_stSettings.m_nVolumeLevel - VOLUME_MINIMUM)) / (VOLUME_MAXIMUM - VOLUME_MINIMUM)*100.0f + 0.5f));
  }
  else if (setting.id == AUDIO_SETTINGS_VOLUME_AMPLIFICATION)
  {
    if (g_application.m_pPlayer)
      g_application.m_pPlayer->SetDynamicRangeCompression((long)(g_stSettings.m_currentVideoSettings.m_VolumeAmplification * 100));
  }
  else if (setting.id == AUDIO_SETTINGS_DELAY)
  {
    if (g_application.m_pPlayer)
      g_application.m_pPlayer->SetAVDelay(g_stSettings.m_currentVideoSettings.m_AudioDelay);
  }
  else if (setting.id == AUDIO_SETTINGS_STREAM)
  {
    // first check if it's a stereo track that we can change between stereo, left and right
    if (g_application.m_pPlayer->GetAudioStreamCount() == 1)
    {
      if (setting.max == 2)
      { // we're in the case we want - call the code to switch channels etc.
        // update the screen setting...
        g_stSettings.m_currentVideoSettings.m_AudioStream = -1 - m_audioStream;
        // call monkeyh1's code here...
        bool bAudioOnAllSpeakers = (g_guiSettings.GetInt("audiooutput.mode") == AUDIO_DIGITAL) && g_stSettings.m_currentVideoSettings.m_OutputToAllSpeakers;
#if defined(HAS_VIDEO_PLAYBACK) && defined(HAS_XBOX_HARDWARE)
        xbox_audio_switch_channel(m_audioStream, bAudioOnAllSpeakers);
#endif
        return;
      }
    }
    // only change the audio stream if a different one has been asked for
    if (g_application.m_pPlayer->GetAudioStream() != m_audioStream)
    {
      g_stSettings.m_currentVideoSettings.m_AudioStream = m_audioStream;
      g_application.m_pPlayer->SetAudioStream(m_audioStream);    // Set the audio stream to the one selected
    }
  }
  else if (setting.id == AUDIO_SETTINGS_OUTPUT_TO_ALL_SPEAKERS)
  {
    g_application.Restart();
  }
  else if (setting.id == AUDIO_SETTINGS_DIGITAL_ANALOG)
  {
    if(m_outputmode == 0) // might be unneccesary (indexes match), but just for clearity
      g_guiSettings.SetInt("audiooutput.mode", AUDIO_ANALOG);
    else
      g_guiSettings.SetInt("audiooutput.mode", AUDIO_DIGITAL);

    EnableSettings(AUDIO_SETTINGS_OUTPUT_TO_ALL_SPEAKERS, g_guiSettings.GetInt("audiooutput.mode") == AUDIO_DIGITAL);
    g_application.Restart();
  }
  else if (setting.id == SUBTITLE_SETTINGS_ENABLE)
  {
    g_stSettings.m_currentVideoSettings.m_SubtitleOn = m_subtitleVisible;
    g_application.m_pPlayer->SetSubtitleVisible(g_stSettings.m_currentVideoSettings.m_SubtitleOn);
    if (!g_stSettings.m_currentVideoSettings.m_SubtitleCached && g_stSettings.m_currentVideoSettings.m_SubtitleOn)
    {
      g_application.Restart(true); // cache subtitles
      Close();
    }
  }
  else if (setting.id == SUBTITLE_SETTINGS_DELAY)
    g_application.m_pPlayer->SetSubTitleDelay(g_stSettings.m_currentVideoSettings.m_SubtitleDelay);
  else if (setting.id == SUBTITLE_SETTINGS_STREAM && setting.max > 0)
  {
    g_stSettings.m_currentVideoSettings.m_SubtitleStream = m_subtitleStream;
    g_application.m_pPlayer->SetSubtitle(m_subtitleStream);
  }
  else if (setting.id == SUBTITLE_SETTINGS_BROWSER)
  {
    CStdString strPath;
    if (CUtil::IsInRAR(g_application.CurrentFileItem().m_strPath) || CUtil::IsInZIP(g_application.CurrentFileItem().m_strPath))
    {
      CURL url(g_application.CurrentFileItem().m_strPath);
      strPath = url.GetHostName();
    }
    else
      strPath = g_application.CurrentFileItem().m_strPath;

    CStdString strMask = ".utf|.utf8|.utf-8|.sub|.srt|.smi|.rt|.txt|.ssa|.aqt|.jss|.ass|.idx|.ifo|.rar|.zip";
    if (g_application.GetCurrentPlayer() == EPC_DVDPLAYER)
      strMask = ".srt|.rar|.zip";
    VECSHARES shares(g_settings.m_videoSources);
    if (g_stSettings.iAdditionalSubtitleDirectoryChecked != -1 && !g_guiSettings.GetString("subtitles.custompath").IsEmpty())
    {
      CShare share;
      std::vector<CStdString> paths;
      CStdString strPath1;
      CUtil::GetDirectory(strPath,strPath1);
      paths.push_back(strPath1);
      strPath1 = g_guiSettings.GetString("subtitles.custompath");
      paths.push_back(g_guiSettings.GetString("subtitles.custompath"));
      share.FromNameAndPaths("video",g_localizeStrings.Get(21367),paths);
      // hack
      g_settings.m_videoSources.push_back(share);
      strPath = share.strPath;
      CUtil::AddSlashAtEnd(strPath);
    }
    if (CGUIDialogFileBrowser::ShowAndGetFile(g_settings.m_videoSources,strMask,g_localizeStrings.Get(293),strPath,false,true)) // "subtitles"
    {
      CStdString strExt;
      CUtil::GetExtension(strPath,strExt);
      if (strExt.CompareNoCase(".idx") == 0 || strExt.CompareNoCase(".sub") == 0)
      {
        // else get current position
        double time = g_application.GetTime();

        // get player state, needed for dvd's
        CStdString state = g_application.m_pPlayer->GetPlayerState();

        g_application.m_pPlayer->CloseFile(); // to conserve memory if unraring
        if (CFile::Cache(strPath,"z:\\subtitle"+strExt+".keep"))
        {
          CStdString strPath2;
          CStdString strPath3;
          if (strExt.CompareNoCase(".idx") == 0)
          {
            CUtil::ReplaceExtension(strPath,".sub",strPath2);
            strPath3 = "z:\\subtitle.sub.keep";
          }
          else
          {
            CUtil::ReplaceExtension(strPath,".idx",strPath2);
            if (!CFile::Exists(strPath2) && (CUtil::IsInRAR(strPath2) || CUtil::IsInZIP(strPath2)))
            {
              CStdString strFileName = CUtil::GetFileName(strPath);
              CUtil::GetDirectory(strPath,strPath3);
              CUtil::GetParentPath(strPath3,strPath2);
              CUtil::AddFileToFolder(strPath2,strFileName,strPath2);
              CUtil::ReplaceExtension(strPath2,".idx",strPath2);
            }
            strPath3 = "z:\\subtitle.idx.keep";
          }
          if (CFile::Exists(strPath2))
            CFile::Cache(strPath2,strPath3);
          else
          {
            CFileItemList items;
            CStdString strDir,strFileNameNoExtNoCase;
            CUtil::Split(strPath,strDir,strPath3);
            CUtil::ReplaceExtension(strPath3,".",strFileNameNoExtNoCase);
            strFileNameNoExtNoCase.ToLower();
            CUtil::GetDirectory(strPath,strDir);
            CDirectory::GetDirectory(strDir,items,".rar|.zip",false);
            vector<CStdString> vecExts;
            for (int i=0;i<items.Size();++i)
              CUtil::CacheRarSubtitles(vecExts,items[i]->m_strPath,strFileNameNoExtNoCase,".keep");
          }
          g_stSettings.m_currentVideoSettings.m_SubtitleCached = false;
          g_stSettings.m_currentVideoSettings.m_SubtitleOn = true;

          // reopen the file
          if ( g_application.PlayFile(g_application.CurrentFileItem(), true) && g_application.m_pPlayer )
          {
            // and seek to the position
            g_application.m_pPlayer->SetPlayerState(state);
            g_application.SeekTime(time);
          }

          Close();
        }
      }
      else
      {
        m_subtitleStream = g_application.m_pPlayer->GetSubtitleCount();
        CStdString strExt;
        CUtil::GetExtension(strPath,strExt);
        if (CFile::Cache(strPath,"z:\\subtitle.browsed"+strExt))
        {
          g_stSettings.m_currentVideoSettings.m_SubtitleOn = true;
          g_application.m_pPlayer->SetSubtitleVisible(true);
          g_application.m_pPlayer->AddSubtitle("z:\\subtitle.browsed"+strExt);
          g_application.m_pPlayer->SetSubtitle(m_subtitleStream);
        }

        Close();
      }
      g_stSettings.m_currentVideoSettings.m_SubtitleCached = true;
    }
    g_settings.m_videoSources = shares;
  }
  else if (setting.id == AUDIO_SETTINGS_MAKE_DEFAULT)
  {
    if (g_settings.m_vecProfiles[g_settings.m_iLastLoadedProfileIndex].settingsLocked() && g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE)
      if (!g_passwordManager.IsMasterLockUnlocked(true))
        return;

    // prompt user if they are sure
    if (CGUIDialogYesNo::ShowAndGetInput(12376, 750, 0, 12377))
    { // reset the settings
      CVideoDatabase db;
      db.Open();
      db.EraseVideoSettings();
      db.Close();
      g_stSettings.m_defaultVideoSettings = g_stSettings.m_currentVideoSettings;
      g_settings.Save();
    }
  }
}

void CGUIDialogAudioSubtitleSettings::Render()
{
  m_volume = g_stSettings.m_nVolumeLevel * 0.01f;
  UpdateSetting(AUDIO_SETTINGS_VOLUME);
  if (g_application.m_pPlayer)
  {
    // these settings can change on the fly
    UpdateSetting(AUDIO_SETTINGS_DELAY);
    UpdateSetting(SUBTITLE_SETTINGS_ENABLE);
    UpdateSetting(SUBTITLE_SETTINGS_DELAY);
  }
  CGUIDialogSettings::Render();
}
