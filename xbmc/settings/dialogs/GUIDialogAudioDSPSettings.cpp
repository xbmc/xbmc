/*
 *      Copyright (C) 2005-2015 Team KODI
 *      http://kodi.tv
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
 *  along with KODI; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "GUIDialogAudioDSPSettings.h"
#include "Application.h"
#include "addons/Skin.h"
#include "cores/IPlayer.h"
#include "cores/AudioEngine/Engines/ActiveAE/AudioDSPAddons/ActiveAEDSPDatabase.h"
#include "cores/AudioEngine/Engines/ActiveAE/AudioDSPAddons/ActiveAEDSPMode.h"
#include "cores/AudioEngine/Utils/AEUtil.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "dialogs/GUIDialogYesNo.h"
#include "guilib/GUIMessage.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "profiles/ProfilesManager.h"
#include "settings/AdvancedSettings.h"
#include "settings/MediaSettings.h"
#include "settings/MediaSourceSettings.h"
#include "settings/Settings.h"
#include "settings/lib/SettingSection.h"
#include "settings/lib/Setting.h"
#include "settings/lib/SettingsManager.h"
#include "settings/windows/GUIControlSettings.h"
#include "utils/log.h"
#include "utils/StringUtils.h"

#define SETTING_AUDIO_CAT_MAIN                    "audiodspmainsettings"
#define SETTING_AUDIO_CAT_MASTER                  "audiodspmastersettings"
#define SETTING_AUDIO_CAT_POST_PROCESS            "audiodsppostsettings"
#define SETTING_AUDIO_CAT_RESAMPLING              "audiodspresamplesettings"
#define SETTING_AUDIO_CAT_PRE_PROCESS             "audiodsppresettings"
#define SETTING_AUDIO_CAT_MISC                    "audiodspmiscsettings"
#define SETTING_AUDIO_CAT_PROC_INFO               "audiodspprocinfo"

#define SETTING_AUDIO_MAIN_STREAMTYPE             "audiodsp.main.streamtype"
#define SETTING_AUDIO_MAIN_MODETYPE               "audiodsp.main.modetype"
#define SETTING_AUDIO_MAIN_VOLUME                 "audiodsp.main.volume"
#define SETTING_AUDIO_MAIN_VOLUME_AMPLIFICATION   "audiodsp.main.volumeamplification"
#define SETTING_AUDIO_MAIN_BUTTON_MASTER          "audiodsp.main.menumaster"
#define SETTING_AUDIO_MAIN_BUTTON_OUTPUT          "audiodsp.main.menupostproc"
#define SETTING_AUDIO_MAIN_BUTTON_RESAMPLE        "audiodsp.main.menuresample"
#define SETTING_AUDIO_MAIN_BUTTON_PRE_PROC        "audiodsp.main.menupreproc"
#define SETTING_AUDIO_MAIN_BUTTON_MISC            "audiodsp.main.menumisc"
#define SETTING_AUDIO_MAIN_BUTTON_INFO            "audiodsp.main.menuinfo"
#define SETTING_AUDIO_MAIN_MAKE_DEFAULT           "audiodsp.main.makedefault"
#define SETTING_AUDIO_MASTER_SETTINGS_MENUS       "audiodsp.master.menu_"
#define SETTING_AUDIO_POST_PROC_AUDIO_DELAY       "audiodsp.postproc.delay"
#define SETTING_AUDIO_PROC_SETTINGS_MENUS         "audiodsp.proc.menu_"

#define SETTING_STREAM_INFO_INPUT_CHANNELS        "audiodsp.info.inputchannels"
#define SETTING_STREAM_INFO_INPUT_CHANNEL_NAMES   "audiodsp.info.inputchannelnames"
#define SETTING_STREAM_INFO_INPUT_SAMPLERATE      "audiodsp.info.inputsamplerate"
#define SETTING_STREAM_INFO_OUTPUT_CHANNELS       "audiodsp.info.outputchannels"
#define SETTING_STREAM_INFO_OUTPUT_CHANNEL_NAMES  "audiodsp.info.outputchannelnames"
#define SETTING_STREAM_INFO_OUTPUT_SAMPLERATE     "audiodsp.info.outputsamplerate"
#define SETTING_STREAM_INFO_CPU_USAGE             "audiodsp.info.cpuusage"
#define SETTING_STREAM_INFO_TYPE_INPUT            "audiodsp.info.typeinput"
#define SETTING_STREAM_INFO_TYPE_PREPROC          "audiodsp.info.typepreproc"
#define SETTING_STREAM_INFO_TYPE_MASTER           "audiodsp.info.typemaster"
#define SETTING_STREAM_INFO_TYPE_POSTPROC         "audiodsp.info.typepostproc"
#define SETTING_STREAM_INFO_TYPE_OUTPUT           "audiodsp.info.typeoutput"
#define SETTING_STREAM_INFO_MODE_CPU_USAGE        "audiodsp.info.modecpuusage_"

using namespace ActiveAE;

CGUIDialogAudioDSPSettings::CGUIDialogAudioDSPSettings()
  : CGUIDialogSettingsManualBase(WINDOW_DIALOG_AUDIO_DSP_OSD_SETTINGS, "DialogSettings.xml")
{
  m_ActiveStreamId                                = 0;
  m_GetCPUUsage                                   = false;
  m_MenuPositions[SETTING_AUDIO_CAT_MAIN]         = CONTROL_SETTINGS_START_CONTROL;
  m_MenuPositions[SETTING_AUDIO_CAT_MASTER]       = CONTROL_SETTINGS_START_CONTROL;
  m_MenuPositions[SETTING_AUDIO_CAT_POST_PROCESS] = CONTROL_SETTINGS_START_CONTROL;
  m_MenuPositions[SETTING_AUDIO_CAT_RESAMPLING]   = CONTROL_SETTINGS_START_CONTROL;
  m_MenuPositions[SETTING_AUDIO_CAT_PRE_PROCESS]  = CONTROL_SETTINGS_START_CONTROL;
  m_MenuPositions[SETTING_AUDIO_CAT_MISC]         = CONTROL_SETTINGS_START_CONTROL;
  m_MenuPositions[SETTING_AUDIO_CAT_PROC_INFO]    = CONTROL_SETTINGS_START_CONTROL+2;
}

CGUIDialogAudioDSPSettings::~CGUIDialogAudioDSPSettings(void)
{ }

int CGUIDialogAudioDSPSettings::FindCategoryIndex(const std::string &catId)
{
  for (unsigned int i =  0; i < m_categories.size(); i++)
  {
    if (m_categories[i]->GetId() == catId)
      return i;
  }
  return 0;
}

void CGUIDialogAudioDSPSettings::OpenMenu(const std::string &id)
{
  m_GetCPUUsage = false;
  m_MenuPositions[m_categories[m_iCategory]->GetId()] = GetFocusedControlID();
  m_MenuHierarchy.push_back(m_iCategory);
  m_iCategory = FindCategoryIndex(id);

  /* Get menu name */
  m_MenuName = -1;
  if (id == SETTING_AUDIO_CAT_MAIN)
    m_MenuName = 15028;
  else if (id == SETTING_AUDIO_CAT_MASTER)
    m_MenuName = 15029;
  else if (id == SETTING_AUDIO_CAT_POST_PROCESS)
    m_MenuName = 15030;
  else if (id == SETTING_AUDIO_CAT_RESAMPLING)
    m_MenuName = 15035;
  else if (id == SETTING_AUDIO_CAT_PRE_PROCESS)
    m_MenuName = 15037;
  else if (id == SETTING_AUDIO_CAT_MISC)
    m_MenuName = 15038;
  else if (id == SETTING_AUDIO_CAT_PROC_INFO)
    m_MenuName = 15031;

  SetHeading(g_localizeStrings.Get(m_MenuName));
  CreateSettings();
  SET_CONTROL_FOCUS(m_MenuPositions[id], 0);
}

bool CGUIDialogAudioDSPSettings::OnMessage(CGUIMessage &message)
{
  switch (message.GetMessage())
  {
    case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();
      if (iControl >= CONTROL_SETTINGS_START_CONTROL && iControl < (int)(CONTROL_SETTINGS_START_CONTROL + m_settingControls.size()))
      {
        CSetting *setting = GetSettingControl(iControl)->GetSetting();
        if (setting != NULL)
        {
          if (setting->GetId() == SETTING_AUDIO_MAIN_BUTTON_MASTER)
            OpenMenu(SETTING_AUDIO_CAT_MASTER);
          else if (setting->GetId() == SETTING_AUDIO_MAIN_BUTTON_OUTPUT)
            OpenMenu(SETTING_AUDIO_CAT_POST_PROCESS);
          else if (setting->GetId() == SETTING_AUDIO_MAIN_BUTTON_RESAMPLE)
            OpenMenu(SETTING_AUDIO_CAT_RESAMPLING);
          else if (setting->GetId() == SETTING_AUDIO_MAIN_BUTTON_PRE_PROC)
            OpenMenu(SETTING_AUDIO_CAT_PRE_PROCESS);
          else if (setting->GetId() == SETTING_AUDIO_MAIN_BUTTON_MISC)
            OpenMenu(SETTING_AUDIO_CAT_MISC);
          else if (setting->GetId() == SETTING_AUDIO_MAIN_BUTTON_INFO)
          {
            SetupView();
            OpenMenu(SETTING_AUDIO_CAT_PROC_INFO);
            m_GetCPUUsage = true;
          }
          else
          {
            if (setting->GetId().substr(0, 19) == SETTING_AUDIO_PROC_SETTINGS_MENUS)
              OpenAudioDSPMenu(strtol(setting->GetId().substr(19).c_str(), NULL, 0));
            else if (setting->GetId().substr(0, 21) == SETTING_AUDIO_MASTER_SETTINGS_MENUS)
              OpenAudioDSPMenu(strtol(setting->GetId().substr(21).c_str(), NULL, 0));
            else if (setting->GetId().substr(0, 27) == SETTING_STREAM_INFO_MODE_CPU_USAGE)
            {
              if (!OpenAudioDSPMenu(m_ActiveModesData[strtol(setting->GetId().substr(27).c_str(), NULL, 0)].MenuListPtr))
                CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info, g_localizeStrings.Get(15031), g_localizeStrings.Get(416));
            }
          }
        }
      }
      break;
    }
    default:
      break;
  }

  return CGUIDialogSettingsManualBase::OnMessage(message);
}

bool CGUIDialogAudioDSPSettings::OnBack(int actionID)
{
  // if the setting dialog is not a window but a dialog we need to close differently
  int mainCategory = FindCategoryIndex(SETTING_AUDIO_CAT_MAIN);
  if (m_iCategory == mainCategory)
    return CGUIDialogSettingsManualBase::OnBack(actionID);

  m_MenuPositions[m_categories[m_iCategory]->GetId()] = GetFocusedControlID();
  if (!m_MenuHierarchy.empty())
  {
    m_iCategory = m_MenuHierarchy.back();
    m_MenuHierarchy.pop_back();
  }
  else
    m_iCategory = mainCategory;

  if (m_iCategory == mainCategory)
    SetHeading(15028);

  CreateSettings();
  SET_CONTROL_FOCUS(m_MenuPositions[m_categories[m_iCategory]->GetId()], 0);

  return true;
}

void CGUIDialogAudioDSPSettings::FrameMove()
{
  // update the volume setting if necessary
  float newVolume = g_application.GetVolume(false);
  if (newVolume != m_volume)
    m_settingsManager->SetNumber(SETTING_AUDIO_MAIN_VOLUME, newVolume);

  if (g_application.m_pPlayer->HasPlayer())
  {
    const CVideoSettings &videoSettings = CMediaSettings::GetInstance().GetCurrentVideoSettings();

    // these settings can change on the fly
    if (SupportsAudioFeature(IPC_AUD_OFFSET))
      m_settingsManager->SetNumber(SETTING_AUDIO_POST_PROC_AUDIO_DELAY, videoSettings.m_AudioDelay);

    bool forceReload = false;
    unsigned int  streamId = CServiceBroker::GetADSP().GetActiveStreamId();
    if (m_ActiveStreamId != streamId)
    {
      m_ActiveStreamId      = streamId;
      m_ActiveStreamProcess = CServiceBroker::GetADSP().GetDSPProcess(m_ActiveStreamId);
      if (m_ActiveStreamId == (unsigned int)-1 || !m_ActiveStreamProcess)
      {
        Close(true);
        return;
      }
      forceReload = true;
    }

    int               modeUniqueId;
    AE_DSP_BASETYPE   usedBaseType;
    AE_DSP_STREAMTYPE streamTypeUsed;
    m_ActiveStreamProcess->GetMasterModeTypeInformation(streamTypeUsed, usedBaseType, modeUniqueId);
    if (forceReload || m_baseTypeUsed != usedBaseType || m_streamTypeUsed != streamTypeUsed)
    {
      m_baseTypeUsed   = usedBaseType;
      m_streamTypeUsed = streamTypeUsed;

      /*!
      * Update settings
      */
      int selType = CMediaSettings::GetInstance().GetCurrentAudioSettings().m_MasterStreamTypeSel;
      CMediaSettings::GetInstance().GetCurrentAudioSettings().m_MasterModes[streamTypeUsed][usedBaseType] = modeUniqueId;
      CMediaSettings::GetInstance().GetCurrentAudioSettings().m_MasterModes[selType][usedBaseType]        = modeUniqueId;
      CMediaSettings::GetInstance().GetCurrentAudioSettings().m_MasterStreamBase                          = usedBaseType;
      CMediaSettings::GetInstance().GetCurrentAudioSettings().m_MasterStreamType                          = streamTypeUsed;

      m_settingsManager->SetInt(SETTING_AUDIO_MAIN_MODETYPE, modeUniqueId);
    }

    // these settings can change on the fly
    if (m_GetCPUUsage)
    {
      m_CPUUsage = StringUtils::Format("%.02f %%", m_ActiveStreamProcess->GetCPUUsage());
      m_settingsManager->SetString(SETTING_STREAM_INFO_CPU_USAGE, m_CPUUsage);
      for (unsigned int i = 0; i < m_ActiveModes.size(); i++)
      {
        std::string settingId = StringUtils::Format("%s%i", SETTING_STREAM_INFO_MODE_CPU_USAGE, i);
        m_ActiveModesData[i].CPUUsage = StringUtils::Format("%.02f %%", m_ActiveModes[i]->CPUUsage());
        m_settingsManager->SetString(settingId, m_ActiveModesData[i].CPUUsage);
      }
    }
  }

  CGUIDialogSettingsManualBase::FrameMove();
}

std::string CGUIDialogAudioDSPSettings::FormatDelay(float value, float interval)
{
  if (fabs(value) < 0.5f * interval)
    return StringUtils::Format(g_localizeStrings.Get(22003).c_str(), 0.0);
  if (value < 0)
    return StringUtils::Format(g_localizeStrings.Get(22004).c_str(), fabs(value));

  return StringUtils::Format(g_localizeStrings.Get(22005).c_str(), value);
}

std::string CGUIDialogAudioDSPSettings::FormatDecibel(float value)
{
  return StringUtils::Format(g_localizeStrings.Get(14054).c_str(), value);
}

std::string CGUIDialogAudioDSPSettings::FormatPercentAsDecibel(float value)
{
  return StringUtils::Format(g_localizeStrings.Get(14054).c_str(), CAEUtil::PercentToGain(value));
}

void CGUIDialogAudioDSPSettings::OnSettingChanged(const CSetting *setting)
{
  if (setting == NULL)
    return;

  CGUIDialogSettingsManualBase::OnSettingChanged(setting);

  CVideoSettings &videoSettings = CMediaSettings::GetInstance().GetCurrentVideoSettings();
  const std::string &settingId = setting->GetId();
  if (settingId == SETTING_AUDIO_MAIN_STREAMTYPE)
  {
    int type = (AE_DSP_STREAMTYPE)static_cast<const CSettingInt*>(setting)->GetValue();
    CMediaSettings::GetInstance().GetCurrentAudioSettings().m_MasterStreamTypeSel = type;
    if (type == AE_DSP_ASTREAM_AUTO)
      type = m_ActiveStreamProcess->GetDetectedStreamType();

    CMediaSettings::GetInstance().GetCurrentAudioSettings().m_MasterStreamType = type;

    /* Set the input stream type if any modes are available for this type */
    if (type >= AE_DSP_ASTREAM_BASIC && type < AE_DSP_ASTREAM_AUTO && !m_MasterModes[type].empty())
    {
      /* Find the master mode id for the selected stream type if it was not known before */
      if (CMediaSettings::GetInstance().GetCurrentAudioSettings().m_MasterModes[type][m_baseTypeUsed] < 0)
        CMediaSettings::GetInstance().GetCurrentAudioSettings().m_MasterModes[type][m_baseTypeUsed] = m_MasterModes[type][0]->ModeID();

      /* Switch now the master mode and stream type for audio dsp processing */
      m_ActiveStreamProcess->SetMasterMode((AE_DSP_STREAMTYPE)type,
                                           CMediaSettings::GetInstance().GetCurrentAudioSettings().m_MasterModes[type][m_baseTypeUsed],
                                           true);
    }
    else
    {
      CLog::Log(LOGERROR, "ActiveAE DSP Settings - %s - Change of audio stream type failed (type = %i)", __FUNCTION__, type);
    }
  }
  else if (settingId == SETTING_AUDIO_MAIN_MODETYPE)
  {
    m_modeTypeUsed = static_cast<const CSettingInt*>(setting)->GetValue();
    if (m_ActiveStreamProcess->SetMasterMode(m_streamTypeUsed, m_modeTypeUsed))
      CMediaSettings::GetInstance().GetCurrentAudioSettings().m_MasterModes[m_streamTypeUsed][m_baseTypeUsed] = m_modeTypeUsed;
  }
  else if (settingId == SETTING_AUDIO_MAIN_VOLUME)
  {
    m_volume = static_cast<float>(static_cast<const CSettingNumber*>(setting)->GetValue());
    g_application.SetVolume(m_volume, false); // false - value is not in percent
  }
  else if (settingId == SETTING_AUDIO_MAIN_VOLUME_AMPLIFICATION)
  {
    videoSettings.m_VolumeAmplification = static_cast<float>(static_cast<const CSettingNumber*>(setting)->GetValue());
    g_application.m_pPlayer->SetDynamicRangeCompression((long)(videoSettings.m_VolumeAmplification * 100));
  }
  else if (settingId == SETTING_AUDIO_POST_PROC_AUDIO_DELAY)
  {
    videoSettings.m_AudioDelay = static_cast<float>(static_cast<const CSettingNumber*>(setting)->GetValue());
    g_application.m_pPlayer->SetAVDelay(videoSettings.m_AudioDelay);
  }
}

void CGUIDialogAudioDSPSettings::OnSettingAction(const CSetting *setting)
{
  if (setting == NULL)
    return;

  CGUIDialogSettingsManualBase::OnSettingAction(setting);

  const std::string &settingId = setting->GetId();
  if (settingId == SETTING_AUDIO_MAIN_MAKE_DEFAULT)
    Save();
}

void CGUIDialogAudioDSPSettings::Save()
{
  if (!g_passwordManager.CheckSettingLevelLock(SettingLevelExpert) &&
      CProfilesManager::GetInstance().GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE)
    return;

  // prompt user if they are sure
  if (!CGUIDialogYesNo::ShowAndGetInput(CVariant{12376}, CVariant{12377}))
    return;

  // reset the settings
  CActiveAEDSPDatabase db;
  if (!db.Open())
    return;

  db.EraseActiveDSPSettings();
  db.Close();

  CMediaSettings::GetInstance().GetDefaultAudioSettings() = CMediaSettings::GetInstance().GetCurrentAudioSettings();
  CMediaSettings::GetInstance().GetDefaultAudioSettings().m_MasterStreamType = AE_DSP_ASTREAM_AUTO;
  CSettings::GetInstance().Save();
}

void CGUIDialogAudioDSPSettings::SetupView()
{
  CGUIDialogSettingsManualBase::SetupView();
  SetHeading(15028);
  SET_CONTROL_HIDDEN(CONTROL_SETTINGS_OKAY_BUTTON);
  SET_CONTROL_HIDDEN(CONTROL_SETTINGS_CUSTOM_BUTTON);
  SET_CONTROL_LABEL(CONTROL_SETTINGS_CANCEL_BUTTON, 15067);
}

void CGUIDialogAudioDSPSettings::InitializeSettings()
{
  CGUIDialogSettingsManualBase::InitializeSettings();

  CSettingCategory *category = AddCategory(SETTING_AUDIO_CAT_MAIN, -1);
  if (category == NULL)
  {
    CLog::Log(LOGERROR, "CGUIDialogAudioDSPSettings: unable to setup settings category 'audiodspmainsettings'");
    return;
  }

  // get all necessary setting groups
  CSettingGroup *groupAudioModeSel = AddGroup(category);
  if (groupAudioModeSel == NULL)
  {
    CLog::Log(LOGERROR, "CGUIDialogAudioDSPSettings: unable to setup settings group 'groupAudioModeSel'");
    return;
  }
  CSettingGroup *groupAudioVolumeSel = AddGroup(category);
  if (groupAudioVolumeSel == NULL)
  {
    CLog::Log(LOGERROR, "CGUIDialogAudioDSPSettings: unable to setup settings group 'groupAudioVolumeSel'");
    return;
  }
  CSettingGroup *groupAudioSubmenuSel = AddGroup(category);
  if (groupAudioSubmenuSel == NULL)
  {
    CLog::Log(LOGERROR, "CGUIDialogAudioDSPSettings: unable to setup settings group 'groupAudioSubmenuSel'");
    return;
  }
  CSettingGroup *groupSaveAsDefault = AddGroup(category);
  if (groupSaveAsDefault == NULL)
  {
    CLog::Log(LOGERROR, "CGUIDialogAudioDSPSettings: unable to setup settings group 'groupSaveAsDefault'");
    return;
  }

  bool usePopup = g_SkinInfo->HasSkinFile("DialogSlider.xml");

  CVideoSettings &videoSettings = CMediaSettings::GetInstance().GetCurrentVideoSettings();

  m_audioCaps.clear();
  if (g_application.m_pPlayer->HasPlayer())
    g_application.m_pPlayer->GetAudioCapabilities(m_audioCaps);

  m_ActiveStreamId      = CServiceBroker::GetADSP().GetActiveStreamId();
  m_ActiveStreamProcess = CServiceBroker::GetADSP().GetDSPProcess(m_ActiveStreamId);
  if (m_ActiveStreamId == (unsigned int)-1 || !m_ActiveStreamProcess)
  {
    m_iCategory = FindCategoryIndex(SETTING_AUDIO_CAT_MAIN);
    Close(true);
    return;
  }

  int modeUniqueId;
  m_ActiveStreamProcess->GetMasterModeTypeInformation(m_streamTypeUsed, m_baseTypeUsed, modeUniqueId);

  int modesAvailable = 0;
  for (int i = 0; i < AE_DSP_ASTREAM_AUTO; i++)
  {
    m_MasterModes[i].clear();
    m_ActiveStreamProcess->GetAvailableMasterModes((AE_DSP_STREAMTYPE)i, m_MasterModes[i]);
    if (!m_MasterModes[i].empty()) modesAvailable++;
  }

  if (modesAvailable > 0)
  {
    /* about size() > 1, it is always the fallback (ignore of master processing) present. */
    StaticIntegerSettingOptions modeEntries;
    if (m_MasterModes[AE_DSP_ASTREAM_BASIC].size() > 1)
      modeEntries.push_back(std::pair<int, int>(CServiceBroker::GetADSP().GetStreamTypeName(AE_DSP_ASTREAM_BASIC),   AE_DSP_ASTREAM_BASIC));
    if (m_MasterModes[AE_DSP_ASTREAM_MUSIC].size() > 1)
      modeEntries.push_back(std::pair<int, int>(CServiceBroker::GetADSP().GetStreamTypeName(AE_DSP_ASTREAM_MUSIC),   AE_DSP_ASTREAM_MUSIC));
    if (m_MasterModes[AE_DSP_ASTREAM_MOVIE].size() > 1)
      modeEntries.push_back(std::pair<int, int>(CServiceBroker::GetADSP().GetStreamTypeName(AE_DSP_ASTREAM_MOVIE),   AE_DSP_ASTREAM_MOVIE));
    if (m_MasterModes[AE_DSP_ASTREAM_GAME].size() > 1)
      modeEntries.push_back(std::pair<int, int>(CServiceBroker::GetADSP().GetStreamTypeName(AE_DSP_ASTREAM_GAME),    AE_DSP_ASTREAM_GAME));
    if (m_MasterModes[AE_DSP_ASTREAM_APP].size() > 1)
      modeEntries.push_back(std::pair<int, int>(CServiceBroker::GetADSP().GetStreamTypeName(AE_DSP_ASTREAM_APP),     AE_DSP_ASTREAM_APP));
    if (m_MasterModes[AE_DSP_ASTREAM_MESSAGE].size() > 1)
      modeEntries.push_back(std::pair<int, int>(CServiceBroker::GetADSP().GetStreamTypeName(AE_DSP_ASTREAM_MESSAGE), AE_DSP_ASTREAM_MESSAGE));
    if (m_MasterModes[AE_DSP_ASTREAM_PHONE].size() > 1)
      modeEntries.push_back(std::pair<int, int>(CServiceBroker::GetADSP().GetStreamTypeName(AE_DSP_ASTREAM_PHONE),   AE_DSP_ASTREAM_PHONE));
    if (modesAvailable > 1 && m_MasterModes[m_streamTypeUsed].size() > 1)
      modeEntries.insert(modeEntries.begin(), std::pair<int, int>(CServiceBroker::GetADSP().GetStreamTypeName(AE_DSP_ASTREAM_AUTO), AE_DSP_ASTREAM_AUTO));

    AddSpinner(groupAudioModeSel,
                SETTING_AUDIO_MAIN_STREAMTYPE, 15021, 0,
                (AE_DSP_STREAMTYPE)CMediaSettings::GetInstance().GetCurrentAudioSettings().m_MasterStreamTypeSel,
                modeEntries);
  }

  bool AddonMasterModeSetupPresent = false;
  m_ModeList.clear();
  for (unsigned int i = 0; i < m_MasterModes[m_streamTypeUsed].size(); i++)
  {
    if (m_MasterModes[m_streamTypeUsed][i])
    {
      AE_DSP_ADDON addon;
      int modeId = m_MasterModes[m_streamTypeUsed][i]->ModeID();
      if (modeId == AE_DSP_MASTER_MODE_ID_PASSOVER || modeId >= AE_DSP_MASTER_MODE_ID_INTERNAL_TYPES)
      {
        m_ModeList.push_back(make_pair(g_localizeStrings.Get(m_MasterModes[m_streamTypeUsed][i]->ModeName()), modeId));
      }
      else if (CServiceBroker::GetADSP().GetAudioDSPAddon(m_MasterModes[m_streamTypeUsed][i]->AddonID(), addon))
      {
        m_ModeList.push_back(make_pair(g_localizeStrings.GetAddonString(addon->ID(), m_MasterModes[m_streamTypeUsed][i]->ModeName()), modeId));
        if (!AddonMasterModeSetupPresent)
          AddonMasterModeSetupPresent = m_MasterModes[m_streamTypeUsed][i]->HasSettingsDialog();
      }
    }
  }

  m_modeTypeUsed = CMediaSettings::GetInstance().GetCurrentAudioSettings().m_MasterModes[m_streamTypeUsed][m_baseTypeUsed];
  CSettingInt *spinner = AddSpinner(groupAudioModeSel, SETTING_AUDIO_MAIN_MODETYPE, 15022, 0, m_modeTypeUsed, AudioModeOptionFiller);
  spinner->SetOptionsFiller(AudioModeOptionFiller, this);

  ///-----------------------

  // audio settings
  // audio volume setting
  m_volume = g_application.GetVolume(false);
  if (!g_windowManager.IsWindowActive(WINDOW_DIALOG_AUDIO_OSD_SETTINGS))
  {
    CSettingNumber *settingAudioVolume = AddSlider(groupAudioVolumeSel, SETTING_AUDIO_MAIN_VOLUME, 13376, 0, m_volume, 14054, VOLUME_MINIMUM, VOLUME_MAXIMUM / 100.0f, VOLUME_MAXIMUM);
    static_cast<CSettingControlSlider*>(settingAudioVolume->GetControl())->SetFormatter(SettingFormatterPercentAsDecibel);
  }

  // audio volume amplification setting
  if (SupportsAudioFeature(IPC_AUD_AMP))
    AddSlider(groupAudioVolumeSel, SETTING_AUDIO_MAIN_VOLUME_AMPLIFICATION, 660, 0, videoSettings.m_VolumeAmplification, 14054, VOLUME_DRC_MINIMUM * 0.01f, (VOLUME_DRC_MAXIMUM - VOLUME_DRC_MINIMUM) / 6000.0f, VOLUME_DRC_MAXIMUM * 0.01f);

  ///-----------------------

  AddButton(groupAudioSubmenuSel, SETTING_AUDIO_MAIN_BUTTON_MASTER,   15025, 0, false, AddonMasterModeSetupPresent, -1);
  AddButton(groupAudioSubmenuSel, SETTING_AUDIO_MAIN_BUTTON_OUTPUT,   15026, 0, false, HaveActiveMenuHooks(AE_DSP_MENUHOOK_POST_PROCESS) || SupportsAudioFeature(IPC_AUD_OFFSET), -1);
  AddButton(groupAudioSubmenuSel, SETTING_AUDIO_MAIN_BUTTON_RESAMPLE, 15033, 0, false, HaveActiveMenuHooks(AE_DSP_MENUHOOK_RESAMPLE), -1);
  AddButton(groupAudioSubmenuSel, SETTING_AUDIO_MAIN_BUTTON_PRE_PROC, 15039, 0, false, HaveActiveMenuHooks(AE_DSP_MENUHOOK_PRE_PROCESS), -1);
  AddButton(groupAudioSubmenuSel, SETTING_AUDIO_MAIN_BUTTON_MISC,     15034, 0, false, HaveActiveMenuHooks(AE_DSP_MENUHOOK_MISCELLANEOUS), -1);
  AddButton(groupAudioSubmenuSel, SETTING_AUDIO_MAIN_BUTTON_INFO,     15027, 0, false, true, -1);

  ///-----------------------

  AddButton(groupSaveAsDefault, SETTING_AUDIO_MAIN_MAKE_DEFAULT, 12376, 0);

  m_Menus.clear();

  /**
   * Audio Master mode settings Dialog init
   */
  {
    CSettingCategory *categoryMaster = AddCategory(SETTING_AUDIO_CAT_MASTER, -1);
    if (categoryMaster == NULL)
    {
      CLog::Log(LOGERROR, "CGUIDialogAudioDSPSettings: unable to setup settings category 'audiodspmastersettings'");
      return;
    }

    CSettingGroup *groupMasterMode = AddGroup(categoryMaster);
    if (groupMasterMode == NULL)
    {
      CLog::Log(LOGERROR, "CGUIDialogAudioDSPSettings: unable to setup settings group 'groupMasterMode'");
      return;
    }

    for (unsigned int i = 0; i < m_MasterModes[m_streamTypeUsed].size(); i++)
    {
      if (m_MasterModes[m_streamTypeUsed][i]->HasSettingsDialog())
      {
        AE_DSP_ADDON addon;
        if (CServiceBroker::GetADSP().GetAudioDSPAddon(m_MasterModes[m_streamTypeUsed][i]->AddonID(), addon))
        {
          AE_DSP_MENUHOOKS hooks;
          if (CServiceBroker::GetADSP().GetMenuHooks(m_MasterModes[m_streamTypeUsed][i]->AddonID(), AE_DSP_MENUHOOK_MASTER_PROCESS, hooks))
          {
            for (unsigned int j = 0; j < hooks.size(); j++)
            {
              if (hooks[j].iRelevantModeId != m_MasterModes[m_streamTypeUsed][i]->AddonModeNumber())
                continue;

              MenuHookMember menu;
              menu.addonId                  = m_MasterModes[m_streamTypeUsed][i]->AddonID();
              menu.hook.category            = hooks[j].category;
              menu.hook.iHookId             = hooks[j].iHookId;
              menu.hook.iLocalizedStringId  = hooks[j].iLocalizedStringId;
              menu.hook.iRelevantModeId     = hooks[j].iRelevantModeId;
              m_Menus.push_back(menu);

              std::string setting = StringUtils::Format("%s%i", SETTING_AUDIO_MASTER_SETTINGS_MENUS, (int)m_Menus.size()-1);
              AddButton(groupMasterMode, setting, 15041, 0);
              break;
            }
          }
        }
      }
    }
  }

  /**
   * Audio post processing settings Dialog init
   */
  {
    CSettingCategory *category = AddCategory(SETTING_AUDIO_CAT_POST_PROCESS, -1);
    if (category == NULL)
    {
      CLog::Log(LOGERROR, "CGUIDialogAudioDSPSettings: unable to setup settings category 'audiodsppostsettings'");
      return;
    }

    CSettingGroup *groupInternal = AddGroup(category);
    if (groupInternal == NULL)
    {
      CLog::Log(LOGERROR, "CGUIDialogAudioDSPSettings: unable to setup settings group 'groupInternal'");
      return;
    }

    CSettingGroup *groupAddon = AddGroup(category);
    if (groupAddon == NULL)
    {
      CLog::Log(LOGERROR, "CGUIDialogAudioDSPSettings: unable to setup settings group 'groupAddon'");
      return;
    }

    // audio delay setting
    if (SupportsAudioFeature(IPC_AUD_OFFSET))
    {
      CSettingNumber *settingAudioDelay = AddSlider(groupInternal, SETTING_AUDIO_POST_PROC_AUDIO_DELAY, 297, 0, videoSettings.m_AudioDelay, 0, -g_advancedSettings.m_videoAudioDelayRange, 0.025f, g_advancedSettings.m_videoAudioDelayRange, 297, usePopup);
      static_cast<CSettingControlSlider*>(settingAudioDelay->GetControl())->SetFormatter(SettingFormatterDelay);
    }
    GetAudioDSPMenus(groupAddon, AE_DSP_MENUHOOK_POST_PROCESS);
  }

  /**
   * Audio add-on resampling setting dialog's
   */
  {
    CSettingCategory *category = AddCategory(SETTING_AUDIO_CAT_RESAMPLING, -1);
    if (category == NULL)
    {
      CLog::Log(LOGERROR, "CGUIDialogAudioDSPSettings: unable to setup settings category 'audiodspresamplesettings'");
      return;
    }
    CSettingGroup *group = AddGroup(category);
    if (group == NULL)
    {
      CLog::Log(LOGERROR, "CGUIDialogAudioDSPSettings: unable to setup settings group 'group'");
      return;
    }
    GetAudioDSPMenus(group, AE_DSP_MENUHOOK_RESAMPLE);
  }

  /**
   * Audio add-on's pre processing setting dialog's
   */
  {
    CSettingCategory *category = AddCategory(SETTING_AUDIO_CAT_PRE_PROCESS, -1);
    if (category == NULL)
    {
      CLog::Log(LOGERROR, "CGUIDialogAudioDSPSettings: unable to setup settings category 'audiodsppresettings'");
      return;
    }
    CSettingGroup *group = AddGroup(category);
    if (group == NULL)
    {
      CLog::Log(LOGERROR, "CGUIDialogAudioDSPSettings: unable to setup settings group 'group'");
      return;
    }
    GetAudioDSPMenus(group, AE_DSP_MENUHOOK_PRE_PROCESS);
  }

  /**
   * Audio add-on's miscellaneous setting dialog's
   */
  {
    CSettingCategory *category = AddCategory(SETTING_AUDIO_CAT_MISC, -1);
    if (category == NULL)
    {
      CLog::Log(LOGERROR, "CGUIDialogAudioDSPSettings: unable to setup settings category 'audiodspmiscsettings'");
      return;
    }
    CSettingGroup *group = AddGroup(category);
    if (group == NULL)
    {
      CLog::Log(LOGERROR, "CGUIDialogAudioDSPSettings: unable to setup settings group 'group'");
      return;
    }
    GetAudioDSPMenus(group, AE_DSP_MENUHOOK_MISCELLANEOUS);
  }

  /**
   * Audio Information Dialog init
   */
  {
    CSettingGroup *group;
    CSettingCategory *category = AddCategory(SETTING_AUDIO_CAT_PROC_INFO, -1);
    if (category == NULL)
    {
      CLog::Log(LOGERROR, "CGUIDialogAudioDSPSettings: unable to setup settings category 'audiodspprocinfo'");
      return;
    }

    m_ActiveModes.clear();
    m_ActiveStreamProcess->GetActiveModes(AE_DSP_MODE_TYPE_UNDEFINED, m_ActiveModes);
    m_ActiveModesData.resize(m_ActiveModes.size());

    group = AddGroup(category, 15089);
    if (group == NULL)
    {
      CLog::Log(LOGERROR, "CGUIDialogAudioDSPSettings: unable to setup settings group for '%s'", g_localizeStrings.Get(15089).c_str());
      return;
    }
    m_InputChannels = StringUtils::Format("%i", m_ActiveStreamProcess->GetInputChannels());
    AddInfoLabelButton(group, SETTING_STREAM_INFO_INPUT_CHANNELS, 21444, 0, m_InputChannels);
    m_InputChannelNames = m_ActiveStreamProcess->GetInputChannelNames();
    AddInfoLabelButton(group, SETTING_STREAM_INFO_INPUT_CHANNEL_NAMES, 15091, 0, m_InputChannelNames);
    m_InputSamplerate = StringUtils::Format("%i Hz", (int)m_ActiveStreamProcess->GetInputSamplerate());
    AddInfoLabelButton(group, SETTING_STREAM_INFO_INPUT_SAMPLERATE, 613, 0, m_InputSamplerate);

    group = AddGroup(category, 15090);
    if (group == NULL)
    {
      CLog::Log(LOGERROR, "CGUIDialogAudioDSPSettings: unable to setup settings group for '%s'", g_localizeStrings.Get(15090).c_str());
      return;
    }
    m_OutputChannels = StringUtils::Format("%i", m_ActiveStreamProcess->GetOutputChannels());
    AddInfoLabelButton(group, SETTING_STREAM_INFO_OUTPUT_CHANNELS, 21444, 0, m_OutputChannels);
    m_OutputChannelNames = m_ActiveStreamProcess->GetOutputChannelNames();
    AddInfoLabelButton(group, SETTING_STREAM_INFO_OUTPUT_CHANNEL_NAMES, 15091, 0, m_OutputChannelNames);
    m_OutputSamplerate = StringUtils::Format("%i Hz", (int)m_ActiveStreamProcess->GetOutputSamplerate());
    AddInfoLabelButton(group, SETTING_STREAM_INFO_OUTPUT_SAMPLERATE, 613, 0, m_OutputSamplerate);

    group = AddGroup(category, 15081);
    if (group == NULL)
    {
      CLog::Log(LOGERROR, "CGUIDialogAudioDSPSettings: unable to setup settings group for '%s'", g_localizeStrings.Get(15081).c_str());
      return;
    }
    m_CPUUsage = StringUtils::Format("%.02f %%", m_ActiveStreamProcess->GetCPUUsage());
    AddInfoLabelButton(group, SETTING_STREAM_INFO_CPU_USAGE, 15092, 0, m_CPUUsage);

    bool foundPreProcess = false, foundPostProcess = false;
    for (unsigned int i = 0; i < m_ActiveModes.size(); i++)
    {
      AE_DSP_ADDON addon;
      if (CServiceBroker::GetADSP().GetAudioDSPAddon(m_ActiveModes[i]->AddonID(), addon))
      {
        std::string label;
        switch (m_ActiveModes[i]->ModeType())
        {
          case AE_DSP_MODE_TYPE_INPUT_RESAMPLE:
            group = AddGroup(category, 15087, -1, true, true);
            label = StringUtils::Format(g_localizeStrings.Get(15082).c_str(), m_ActiveStreamProcess->GetProcessSamplerate());
            break;
          case AE_DSP_MODE_TYPE_OUTPUT_RESAMPLE:
            group = AddGroup(category, 15088, -1, true, true);
            label = StringUtils::Format(g_localizeStrings.Get(15083).c_str(), m_ActiveStreamProcess->GetOutputSamplerate());
            break;
          case AE_DSP_MODE_TYPE_MASTER_PROCESS:
            group = AddGroup(category, 15084, -1, true, true);
            label = g_localizeStrings.GetAddonString(addon->ID(), m_ActiveModes[i]->ModeName());
            break;
          case AE_DSP_MODE_TYPE_PRE_PROCESS:
            if (!foundPreProcess)
            {
              foundPreProcess = true;
              group = AddGroup(category, 15085, -1, true, true);
            }
            label = g_localizeStrings.GetAddonString(addon->ID(), m_ActiveModes[i]->ModeName());
            break;
          case AE_DSP_MODE_TYPE_POST_PROCESS:
            if (!foundPostProcess)
            {
              foundPostProcess = true;
              group = AddGroup(category, 15086, -1, true, true);
            }
            label = g_localizeStrings.GetAddonString(addon->ID(), m_ActiveModes[i]->ModeName());
            break;
          default:
          {
            label += g_localizeStrings.GetAddonString(addon->ID(), m_ActiveModes[i]->ModeName());
            label += " - ";
            label += addon->GetFriendlyName();
          }
        };
        m_ActiveModesData[i].CPUUsage = StringUtils::Format("%.02f %%", m_ActiveModes[i]->CPUUsage());

        MenuHookMember menu;
        menu.addonId = -1;

        AE_DSP_MENUHOOKS hooks;
        m_ActiveModesData[i].MenuListPtr = -1;
        if (CServiceBroker::GetADSP().GetMenuHooks(m_ActiveModes[i]->AddonID(), AE_DSP_MENUHOOK_INFORMATION, hooks))
        {
          for (unsigned int j = 0; j < hooks.size(); j++)
          {
            if (hooks[j].iRelevantModeId != m_ActiveModes[i]->AddonModeNumber())
              continue;

            menu.addonId                  = m_ActiveModes[i]->AddonID();
            menu.hook.category            = hooks[j].category;
            menu.hook.iHookId             = hooks[j].iHookId;
            menu.hook.iLocalizedStringId  = hooks[j].iLocalizedStringId;
            menu.hook.iRelevantModeId     = hooks[j].iRelevantModeId;
            m_Menus.push_back(menu);
            m_ActiveModesData[i].MenuListPtr = m_Menus.size()-1;
            label += " ...";
            break;
          }
        }
        m_ActiveModesData[i].MenuName = label;

        std::string settingId = StringUtils::Format("%s%i", SETTING_STREAM_INFO_MODE_CPU_USAGE, i);
        AddInfoLabelButton(group, settingId, 15041, 0, m_ActiveModesData[i].CPUUsage);
      }
    }
  }
}

bool CGUIDialogAudioDSPSettings::SupportsAudioFeature(int feature)
{
  for (Features::iterator itr = m_audioCaps.begin(); itr != m_audioCaps.end(); ++itr)
  {
    if (*itr == feature || *itr == IPC_AUD_ALL)
      return true;
  }

  return false;
}

void CGUIDialogAudioDSPSettings::AudioModeOptionFiller(const CSetting *setting, std::vector< std::pair<std::string, int> > &list, int &current, void *data)
{
  CGUIDialogAudioDSPSettings *dialog  = (CGUIDialogAudioDSPSettings *)data;
  list = dialog->m_ModeList;

  if (list.empty())
  {
    list.push_back(make_pair(g_localizeStrings.Get(231), -1));
    current = -1;
  }
}

std::string CGUIDialogAudioDSPSettings::SettingFormatterDelay(const CSettingControlSlider *control, const CVariant &value, const CVariant &minimum, const CVariant &step, const CVariant &maximum)
{
  if (!value.isDouble())
    return "";

  float fValue = value.asFloat();
  float fStep = step.asFloat();

  if (fabs(fValue) < 0.5f * fStep)
    return StringUtils::Format(g_localizeStrings.Get(22003).c_str(), 0.0);
  if (fValue < 0)
    return StringUtils::Format(g_localizeStrings.Get(22004).c_str(), fabs(fValue));

  return StringUtils::Format(g_localizeStrings.Get(22005).c_str(), fValue);
}

std::string CGUIDialogAudioDSPSettings::SettingFormatterPercentAsDecibel(const CSettingControlSlider *control, const CVariant &value, const CVariant &minimum, const CVariant &step, const CVariant &maximum)
{
  if (control == NULL || !value.isDouble())
    return "";

  std::string formatString = control->GetFormatString();
  if (control->GetFormatLabel() > -1)
    formatString = g_localizeStrings.Get(control->GetFormatLabel());

  return StringUtils::Format(formatString.c_str(), CAEUtil::PercentToGain(value.asFloat()));
}

bool CGUIDialogAudioDSPSettings::HaveActiveMenuHooks(AE_DSP_MENUHOOK_CAT category)
{
  /*!> Check menus are active on current stream */
  AE_DSP_ADDONMAP addonMap;
  if (CServiceBroker::GetADSP().HaveMenuHooks(category) &&
      CServiceBroker::GetADSP().GetEnabledAudioDSPAddons(addonMap) > 0)
  {
    for (AE_DSP_ADDONMAP_ITR itr = addonMap.begin(); itr != addonMap.end(); itr++)
    {
      AE_DSP_MENUHOOKS hooks;
      if (CServiceBroker::GetADSP().GetMenuHooks(itr->second->GetID(), category, hooks))
      {
        for (unsigned int i = 0; i < hooks.size(); i++)
        {
          if (category != AE_DSP_MENUHOOK_MISCELLANEOUS &&
              !m_ActiveStreamProcess->IsMenuHookModeActive(hooks[i].category, itr->second->GetID(), hooks[i].iRelevantModeId))
            continue;

          return true;
        }
      }
    }
  }

  return false;
}

std::string CGUIDialogAudioDSPSettings::GetSettingsLabel(CSetting *pSetting)
{
  if (pSetting->GetLabel() == 15041)
  {
    const std::string &settingId = pSetting->GetId();

    int ptr = -1;
    AE_DSP_ADDON addon;
    if (settingId.substr(0, 27) == SETTING_STREAM_INFO_MODE_CPU_USAGE)
    {
      ptr = strtol(settingId.substr(27).c_str(), NULL, 0);
      if (ptr >= 0 && CServiceBroker::GetADSP().GetAudioDSPAddon(m_ActiveModes[ptr]->AddonID(), addon))
        return m_ActiveModesData[ptr].MenuName;
    }
    else
    {
      if (settingId.substr(0, 21) == SETTING_AUDIO_MASTER_SETTINGS_MENUS)
        ptr = strtol(settingId.substr(21).c_str(), NULL, 0);
      else if (settingId.substr(0, 19) == SETTING_AUDIO_PROC_SETTINGS_MENUS)
        ptr = strtol(settingId.substr(19).c_str(), NULL, 0);

      if (ptr >= 0 && CServiceBroker::GetADSP().GetAudioDSPAddon(m_Menus[ptr].addonId, addon))
        return g_localizeStrings.GetAddonString(addon->ID(), m_Menus[ptr].hook.iLocalizedStringId);
    }
  }

  return GetLocalizedString(pSetting->GetLabel());
}

void CGUIDialogAudioDSPSettings::GetAudioDSPMenus(CSettingGroup *group, AE_DSP_MENUHOOK_CAT category)
{
  AE_DSP_ADDONMAP addonMap;
  if (CServiceBroker::GetADSP().GetEnabledAudioDSPAddons(addonMap) > 0)
  {
    for (AE_DSP_ADDONMAP_ITR itr = addonMap.begin(); itr != addonMap.end(); itr++)
    {
      AE_DSP_MENUHOOKS hooks;
      if (CServiceBroker::GetADSP().GetMenuHooks(itr->second->GetID(), category, hooks))
      {
        for (unsigned int i = 0; i < hooks.size(); i++)
        {
          if (category != hooks[i].category || (category != AE_DSP_MENUHOOK_MISCELLANEOUS &&
              !m_ActiveStreamProcess->IsMenuHookModeActive(hooks[i].category, itr->second->GetID(), hooks[i].iRelevantModeId)))
            continue;

          MenuHookMember menu;
          menu.addonId                  = itr->second->GetID();
          menu.hook.category            = hooks[i].category;
          menu.hook.iHookId             = hooks[i].iHookId;
          menu.hook.iLocalizedStringId  = hooks[i].iLocalizedStringId;
          menu.hook.iRelevantModeId     = hooks[i].iRelevantModeId;
          m_Menus.push_back(menu);
        }
      }
    }
  }

  for (unsigned int i = 0; i < m_Menus.size(); i++)
  {
    AE_DSP_ADDON addon;
    if (CServiceBroker::GetADSP().GetAudioDSPAddon(m_Menus[i].addonId, addon) && category == m_Menus[i].hook.category)
    {
      std::string modeName = g_localizeStrings.GetAddonString(addon->ID(), m_Menus[i].hook.iLocalizedStringId);
      if (modeName.empty())
        modeName = g_localizeStrings.Get(15041);

      std::string setting = StringUtils::Format("%s%i", SETTING_AUDIO_PROC_SETTINGS_MENUS, i);
      AddButton(group, setting, 15041, 0);
    }
  }
}

bool CGUIDialogAudioDSPSettings::OpenAudioDSPMenu(unsigned int setupEntry)
{
  if (setupEntry >= m_Menus.size())
    return false;

  AE_DSP_ADDON addon;
  if (!CServiceBroker::GetADSP().GetAudioDSPAddon(m_Menus[setupEntry].addonId, addon))
    return false;

  AE_DSP_MENUHOOK       hook;
  AE_DSP_MENUHOOK_DATA  hookData;

  hook.category           = m_Menus[setupEntry].hook.category;
  hook.iHookId            = m_Menus[setupEntry].hook.iHookId;
  hook.iLocalizedStringId = m_Menus[setupEntry].hook.iLocalizedStringId;
  hook.iRelevantModeId    = m_Menus[setupEntry].hook.iRelevantModeId;
  hookData.category       = m_Menus[setupEntry].hook.category;
  switch (hookData.category)
  {
    case AE_DSP_MENUHOOK_PRE_PROCESS:
    case AE_DSP_MENUHOOK_MASTER_PROCESS:
    case AE_DSP_MENUHOOK_RESAMPLE:
    case AE_DSP_MENUHOOK_POST_PROCESS:
      hookData.data.iStreamId = m_ActiveStreamId;
      break;
    default:
      break;
  }

  /*!
   * @note the addon dialog becomes always opened on the back of Kodi ones for this reason a
   * "<animation effect="fade" start="100" end="0" time="400" condition="Window.IsVisible(Addon)">Conditional</animation>"
   * on skin is needed to hide dialog.
   */
  addon->CallMenuHook(hook, hookData);

  return true;
}
