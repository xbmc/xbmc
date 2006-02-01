#include "stdafx.h"
#include "GUIDialogAudioSubtitleSettings.h"
#include "GUIDialogFileBrowser.h"
#include "util.h"
#include "application.h"
#include "VideoDatabase.h"

extern void xbox_audio_switch_channel(int iAudioStream, bool bAudioOnAllSpeakers); //lowlevel audio

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
// separator 5
#define SUBTITLE_SETTINGS_ENABLE          6
#define SUBTITLE_SETTINGS_DELAY           7
#define SUBTITLE_SETTINGS_STREAM          8
#define SUBTITLE_SETTINGS_BROWSER         9
#define AUDIO_SETTINGS_MAKE_DEFAULT      10

void CGUIDialogAudioSubtitleSettings::CreateSettings()
{
  // clear out any old settings
  m_settings.clear();
  // create our settings
  m_volume = g_stSettings.m_nVolumeLevel * 0.01f;
  AddSlider(AUDIO_SETTINGS_VOLUME, 13376, &m_volume, VOLUME_MINIMUM * 0.01f, (VOLUME_MAXIMUM - VOLUME_MINIMUM) * 0.0001f, VOLUME_MAXIMUM * 0.01f, "%2.1f dB");
  AddSlider(AUDIO_SETTINGS_VOLUME_AMPLIFICATION, 290, &g_stSettings.m_currentVideoSettings.m_VolumeAmplification, 0, 5, 30, "%2.1f dB");
  AddSlider(AUDIO_SETTINGS_DELAY, 297, &g_stSettings.m_currentVideoSettings.m_AudioDelay, -g_advancedSettings.m_videoAudioDelayRange, 0.1f, g_advancedSettings.m_videoAudioDelayRange, "%2.1fs");
  AddAudioStreams(AUDIO_SETTINGS_STREAM);
  AddSeparator(5);
  AddBool(SUBTITLE_SETTINGS_ENABLE, 13397, &g_stSettings.m_currentVideoSettings.m_SubtitleOn);
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
    g_application.m_pPlayer->GetAudioStreamName(i, strItem);
    if (strItem.length() == 0)
      strItem.Format("%2i", i + 1);
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
    g_application.m_pPlayer->GetSubtitleName(i, strItem);
    if (strItem.length() == 0)
      strItem.Format("%2i", i + 1);
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
    g_application.Restart(true);
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
        bool bAudioOnAllSpeakers = (g_guiSettings.GetInt("AudioOutput.Mode") == AUDIO_DIGITAL) && g_guiSettings.GetBool("VideoPlayer.OutputToAllSpeakers");
        xbox_audio_switch_channel(m_audioStream, bAudioOnAllSpeakers);
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
  else if (setting.id == SUBTITLE_SETTINGS_ENABLE)
  {
    g_application.m_pPlayer->SetSubtitleVisible(g_stSettings.m_currentVideoSettings.m_SubtitleOn);
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
    CStdString strPath="";
    const CStdString strMask = ".utf|.utf8|.utf-8|.sub|.srt|.smi|.rt|.txt|.ssa|.aqt|.jss|.ass|.idx|.ifo|.rar";
    if (CGUIDialogFileBrowser::ShowAndGetFile(g_settings.m_vecMyVideoShares,strMask,g_localizeStrings.Get(293),strPath)) // "subtitles"
    {
      CStdString strExt;
      CUtil::GetExtension(strPath,strExt);
      if (strExt.CompareNoCase(".rar") == 0)
      {
        std::vector<CStdString> vecExtensionsCached;
        CUtil::CacheRarSubtitles(vecExtensionsCached,strPath,"",".keep");
        g_application.Restart(true); // to reread subtitles
        CreateSettings();
        SetupPage();
      }
      else
      {
        if (CFile::Cache(strPath,"z:\\subtitle"+strExt+".keep"))
        {
          CStdString strPath2;
          if (strExt.CompareNoCase(".idx") == 0)
          {
            CUtil::ReplaceExtension(strPath,".sub",strPath2);
            if (!CFile::Cache(strPath2,"z:\\subtitle.sub.keep"))
            {
              CUtil::ReplaceExtension(strPath,".rar",strPath2);
              std::vector<CStdString> vecExtensionsCached;
              CUtil::CacheRarSubtitles(vecExtensionsCached,strPath2,CUtil::GetFileName(strPath),".keep");
            }
          }
          if (strExt.CompareNoCase(".sub") == 0)
          {
            CUtil::ReplaceExtension(strPath,".idx",strPath2);
            CFile::Cache(strPath2,"z:\\subtitle.idx.keep");
          }
          g_application.Restart(true); // to reread subtitles
          CreateSettings();
          SetupPage();
        }
      }
    }
  }
  else if (setting.id == AUDIO_SETTINGS_MAKE_DEFAULT)
  {
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