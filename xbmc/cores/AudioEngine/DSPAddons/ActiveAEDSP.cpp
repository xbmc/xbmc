/*
 *      Copyright (C) 2010-2014 Team KODI
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

#include "ActiveAEDSP.h"

#include <utility>

extern "C" {
#include "libavutil/channel_layout.h"
}

#include "ActiveAEDSPProcess.h"
#include "addons/AddonInstaller.h"
#include "addons/GUIDialogAddonSettings.h"
#include "Application.h"
#include "cores/AudioEngine/Engines/ActiveAE/ActiveAEBuffer.h"
#include "cores/AudioEngine/Interfaces/AEResample.h"
#include "cores/AudioEngine/Utils/AEUtil.h"
#include "cores/IPlayer.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "dialogs/GUIDialogOK.h"
#include "dialogs/GUIDialogSelect.h"
#include "guiinfo/GUIInfoLabels.h"
#include "guilib/GUIWindowManager.h"
#include "GUIUserMessages.h"
#include "messaging/ApplicationMessenger.h"
#include "messaging/helpers/DialogHelper.h"
#include "settings/AdvancedSettings.h"
#include "settings/dialogs/GUIDialogAudioDSPManager.h"
#include "settings/MediaSettings.h"
#include "settings/MediaSourceSettings.h"
#include "settings/Settings.h"
#include "utils/JobManager.h"
#include "utils/StringUtils.h"


using namespace ADDON;
using namespace ActiveAE;
using namespace KODI::MESSAGING;

using KODI::MESSAGING::HELPERS::DialogResponse;

#define MIN_DSP_ARRAY_SIZE 4096

/*! @name Master audio dsp control class */
//@{
CActiveAEDSP::CActiveAEDSP()
  : CThread("ActiveAEDSP")
  , m_isActive(false)
  , m_noAddonWarningDisplayed(false)
  , m_usedProcessesCnt(0)
  , m_activeProcessId(-1)
  , m_isValidAudioDSPSettings(false)
{
  Cleanup();
}

CActiveAEDSP::~CActiveAEDSP()
{
  /* Deactivate all present dsp addons */
  Deactivate();
  CLog::Log(LOGDEBUG, "ActiveAE DSP - destroyed");
}

CActiveAEDSP &CActiveAEDSP::GetInstance()
{
  static CActiveAEDSP activeAEDSPManagerInstance;
  return activeAEDSPManagerInstance;
}
//@}

/*! @name initialization and configuration methods */
//@{
class CActiveAEDSPStartJob : public CJob
{
public:
  CActiveAEDSPStartJob() {}
  ~CActiveAEDSPStartJob(void) {}

  bool DoWork(void)
  {
    CActiveAEDSP::GetInstance().Activate(false);
    return true;
  }
};

void CActiveAEDSP::Activate(bool bAsync /* = false */)
{
  if (bAsync)
  {
    CActiveAEDSPStartJob *job = new CActiveAEDSPStartJob();
    CJobManager::GetInstance().AddJob(job, NULL);
    return;
  }

  /* first stop and remove any audio dsp add-on's */
  Deactivate();

  CSingleLock lock(m_critSection);

  CLog::Log(LOGNOTICE, "ActiveAE DSP - starting");

  /* don't start if Settings->System->Audio->Audio DSP isn't checked */
  if (!CSettings::GetInstance().GetBool(CSettings::SETTING_AUDIOOUTPUT_DSPADDONSENABLED))
    return;

  Cleanup();

  /* create and open database */
  m_databaseDSP.Open();

  Create();
  SetPriority(-1);
}

class CActiveAEDSPModeUpdateJob : public CJob
{
public:
  CActiveAEDSPModeUpdateJob() {}
  ~CActiveAEDSPModeUpdateJob(void) {}

  bool DoWork(void)
  {
    CActiveAEDSP::GetInstance().TriggerModeUpdate(false);
    return true;
  }
};

void CActiveAEDSP::TriggerModeUpdate(bool bAsync /* = true */)
{
  if (bAsync)
  {
    CActiveAEDSPModeUpdateJob *job = new CActiveAEDSPModeUpdateJob();
    CJobManager::GetInstance().AddJob(job, NULL);
    return;
  }

  CLog::Log(LOGINFO, "ActiveAE DSP - %s - Update mode selections", __FUNCTION__);

  if (!m_databaseDSP.IsOpen())
  {
    CLog::Log(LOGERROR, "ActiveAE DSP - failed to open the database");
    return;
  }

  for (unsigned int i = 0; i < AE_DSP_MODE_TYPE_MAX; ++i)
  {
    m_modes[i].clear();
    m_databaseDSP.GetModes(m_modes[i], i);
  }

  /*
   * if any dsp processing is active restart playback
   */
  if (m_usedProcessesCnt > 0)
  {
    CLog::Log(LOGNOTICE, "ActiveAE DSP - restarting playback after disabled dsp system");
    CApplicationMessenger::GetInstance().SendMsg(TMSG_MEDIA_RESTART);
  }
}

void CActiveAEDSP::Deactivate(void)
{
  /* check whether the audio dsp is loaded */
  if (!m_isActive)
    return;

  /* stop thread */
  StopThread();

  CSingleLock lock(m_critSection);

  CLog::Log(LOGNOTICE, "ActiveAE DSP - stopping");

  /* destroy all addons */
  for (AE_DSP_ADDONMAP_ITR itr = m_addonMap.begin(); itr != m_addonMap.end(); ++itr)
    itr->second->Destroy();

  m_addonMap.clear();

  /* unload all data */
  Cleanup();

  /* close database */
  if (m_databaseDSP.IsOpen())
    m_databaseDSP.Close();
}

void CActiveAEDSP::Cleanup(void)
{
  CActiveAEDSPProcessPtr tmp;
  for (unsigned int i = 0; i < AE_DSP_STREAM_MAX_STREAMS; ++i)
    m_usedProcesses[i] = tmp;

  m_isActive                  = false;
  m_usedProcessesCnt          = 0;
  m_isValidAudioDSPSettings   = false;
  m_noAddonWarningDisplayed   = false;
  m_outdatedAddons.clear();

  for (unsigned int i = 0; i < AE_DSP_MODE_TYPE_MAX; ++i)
    m_modes[i].clear();
}

bool CActiveAEDSP::InstallAddonAllowed(const std::string &strAddonId) const
{
  return !m_isActive ||
         !IsInUse(strAddonId) ||
         m_usedProcessesCnt == 0;
}

void CActiveAEDSP::MarkAsOutdated(const std::string& strAddonId)
{
  if (IsActivated() && CSettings::GetInstance().GetInt(CSettings::SETTING_GENERAL_ADDONUPDATES) == AUTO_UPDATES_ON)
  {
    CSingleLock lock(m_critSection);
    m_outdatedAddons.push_back(strAddonId);
  }
}

bool CActiveAEDSP::HasOutdatedAddons(std::vector<std::string> &outdatedAddons)
{
  CSingleLock lock(m_critSection);
  if (!m_outdatedAddons.empty())
  {
    outdatedAddons = m_outdatedAddons;
    return true;
  }
  return false;
}

void CActiveAEDSP::ResetDatabase(void)
{
  CLog::Log(LOGNOTICE, "ActiveAE DSP - clearing the audio DSP database");

  if (IsProcessing())
  {
    CLog::Log(LOGNOTICE, "ActiveAE DSP - stopping playback");
    CApplicationMessenger::GetInstance().PostMsg(TMSG_MEDIA_STOP);
  }

  /* stop the thread */
  Deactivate();

  if (m_databaseDSP.Open())
  {
    m_databaseDSP.DeleteModes();
    m_databaseDSP.DeleteActiveDSPSettings();
    m_databaseDSP.DeleteAddons();

    m_databaseDSP.Close();
  }

  CLog::Log(LOGNOTICE, "ActiveAE DSP - database cleared");

  if (CSettings::GetInstance().GetBool(CSettings::SETTING_AUDIOOUTPUT_DSPADDONSENABLED))
  {
    CLog::Log(LOGNOTICE, "ActiveAE DSP - restarting the audio DSP handler");
    m_databaseDSP.Open();
    Cleanup();
    Activate();
  }
}
//@}

/*! @name Settings and action callback methods (OnAction currently unused */
//@{
void CActiveAEDSP::OnSettingAction(const CSetting *setting)
{
  if (setting == NULL)
    return;

  const std::string &settingId = setting->GetId();
  if (settingId == CSettings::SETTING_AUDIOOUTPUT_DSPSETTINGS)
  {
    if (IsActivated())
    {
      CGUIDialogAudioDSPManager *dialog = (CGUIDialogAudioDSPManager *)g_windowManager.GetWindow(WINDOW_DIALOG_AUDIO_DSP_MANAGER);
      if (dialog)
        dialog->Open();
    }
  }
  else if (settingId == CSettings::SETTING_AUDIOOUTPUT_DSPRESETDB)
  {
    if (HELPERS::ShowYesNoDialogLines(CVariant{19098}, CVariant{36440}, CVariant{750}) ==
      DialogResponse::YES)
    {
      CDateTime::ResetTimezoneBias();
      ResetDatabase();
    }
  }
}
//@}

/*! @name addon installation callback methods */
//@{
bool CActiveAEDSP::RequestRestart(AddonPtr addon, bool bDataChanged)
{
  return StopAudioDSPAddon(addon, true);
}

bool CActiveAEDSP::RequestRemoval(AddonPtr addon)
{
  return StopAudioDSPAddon(addon, false);
}

bool CActiveAEDSP::IsInUse(const std::string &strAddonId) const
{
  CSingleLock lock(m_critSection);

  for (AE_DSP_ADDONMAP_CITR citr = m_addonMap.begin(); citr != m_addonMap.end(); ++citr)
    if (!CAddonMgr::GetInstance().IsAddonDisabled(citr->second->ID()) && citr->second->ID() == strAddonId)
      return true;
  return false;
}

bool CActiveAEDSP::IsKnownAudioDSPAddon(const AddonPtr &addon) const
{
  // database IDs start at 1
  return GetAudioDSPAddonId(addon) > 0;
}

int CActiveAEDSP::GetAudioDSPAddonId(const AddonPtr &addon) const
{
  CSingleLock lock(m_critUpdateSection);

  for (AE_DSP_ADDONMAP_CITR citr = m_addonMap.begin(); citr != m_addonMap.end(); ++citr)
  {
    if (citr->second->ID() == addon->ID())
      return citr->first;
  }

  return -1;
}
//@}

/*! @name GUIInfoManager calls */
//@{
bool CActiveAEDSP::TranslateBoolInfo(DWORD dwInfo) const
{
  bool bReturn(false);

  CSingleLock lock(m_critSection);

  if (!IsProcessing() || !m_usedProcesses[m_activeProcessId])
    return bReturn;

  switch (dwInfo)
  {
  case ADSP_IS_ACTIVE:
    bReturn = true;
    break;
  case ADSP_HAS_INPUT_RESAMPLE:
    bReturn = m_usedProcesses[m_activeProcessId]->HasActiveModes(AE_DSP_MODE_TYPE_INPUT_RESAMPLE);
    break;
  case ADSP_HAS_PRE_PROCESS:
    bReturn = m_usedProcesses[m_activeProcessId]->HasActiveModes(AE_DSP_MODE_TYPE_PRE_PROCESS);
    break;
  case ADSP_HAS_MASTER_PROCESS:
    bReturn = m_usedProcesses[m_activeProcessId]->HasActiveModes(AE_DSP_MODE_TYPE_MASTER_PROCESS);
    break;
  case ADSP_HAS_POST_PROCESS:
    bReturn = m_usedProcesses[m_activeProcessId]->HasActiveModes(AE_DSP_MODE_TYPE_POST_PROCESS);
    break;
  case ADSP_HAS_OUTPUT_RESAMPLE:
    bReturn = m_usedProcesses[m_activeProcessId]->HasActiveModes(AE_DSP_MODE_TYPE_OUTPUT_RESAMPLE);
    break;
  case ADSP_MASTER_ACTIVE:
    bReturn = m_usedProcesses[m_activeProcessId]->GetActiveMasterMode() != NULL;
    break;
  default:
    break;
  };

  return bReturn;
}

bool CActiveAEDSP::TranslateCharInfo(DWORD dwInfo, std::string &strValue) const
{
  bool bReturn(true);

  CSingleLock lock(m_critSection);

  if (!IsProcessing() || !m_usedProcesses[m_activeProcessId])
    return false;

  CActiveAEDSPModePtr activeMaster = m_usedProcesses[m_activeProcessId]->GetActiveMasterMode();
  if (activeMaster == NULL)
    return false;

  switch (dwInfo)
  {
  case ADSP_ACTIVE_STREAM_TYPE:
    strValue = g_localizeStrings.Get(GetStreamTypeName(m_usedProcesses[m_activeProcessId]->GetUsedStreamType()));
    break;
  case ADSP_DETECTED_STREAM_TYPE:
    strValue = g_localizeStrings.Get(GetStreamTypeName(m_usedProcesses[m_activeProcessId]->GetDetectedStreamType()));
    break;
  case ADSP_MASTER_NAME:
    {
      AE_DSP_ADDON addon;
      int modeId = activeMaster->ModeID();
      if (modeId == AE_DSP_MASTER_MODE_ID_PASSOVER || modeId >= AE_DSP_MASTER_MODE_ID_INTERNAL_TYPES)
        strValue = g_localizeStrings.Get(activeMaster->ModeName());
      else if (CActiveAEDSP::GetInstance().GetAudioDSPAddon(activeMaster->AddonID(), addon))
        strValue = addon->GetString(activeMaster->ModeName());
    }
    break;
  case ADSP_MASTER_INFO:
    bReturn = m_usedProcesses[m_activeProcessId]->GetMasterModeStreamInfoString(strValue);
    break;
  case ADSP_MASTER_OWN_ICON:
    strValue = activeMaster->IconOwnModePath();
    break;
  case ADSP_MASTER_OVERRIDE_ICON:
    strValue = activeMaster->IconOverrideModePath();
    break;
  default:
    strValue.clear();
    bReturn = false;
    break;
  };

  return bReturn;
}
//@}

/*! @name Current processing streams control function methods */
//@{
CAEChannelInfo CActiveAEDSP::GetInternalChannelLayout(AEStdChLayout stdLayout)
{
  uint64_t channelLayoutOut;
  switch (stdLayout)
  {
    default:
    case AE_CH_LAYOUT_2_0:
      channelLayoutOut = AV_CH_LAYOUT_STEREO;
      break;
    case AE_CH_LAYOUT_2_1:
      channelLayoutOut = AV_CH_LAYOUT_2POINT1;
      break;
    case AE_CH_LAYOUT_3_0:
      channelLayoutOut = AV_CH_LAYOUT_SURROUND;
      break;
    case AE_CH_LAYOUT_3_1:
      channelLayoutOut = AV_CH_LAYOUT_3POINT1;
      break;
    case AE_CH_LAYOUT_4_0:
      channelLayoutOut = AV_CH_LAYOUT_2_2;
      break;
    case AE_CH_LAYOUT_4_1:
      channelLayoutOut = AV_CH_LAYOUT_2_2|AV_CH_LOW_FREQUENCY;
      break;
    case AE_CH_LAYOUT_5_0:
      channelLayoutOut = AV_CH_LAYOUT_5POINT0;
      break;
    case AE_CH_LAYOUT_5_1:
      channelLayoutOut = AV_CH_LAYOUT_5POINT1;
      break;
    case AE_CH_LAYOUT_7_0:
      channelLayoutOut = AV_CH_LAYOUT_7POINT0;
      break;
    case AE_CH_LAYOUT_7_1:
      channelLayoutOut = AV_CH_LAYOUT_7POINT1;
      break;
  }
  return CAEUtil::GetAEChannelLayout(channelLayoutOut);
}

bool CActiveAEDSP::CreateDSPs(unsigned int &streamId, CActiveAEDSPProcessPtr &process, const AEAudioFormat &inputFormat, const AEAudioFormat &outputFormat, bool upmix,
                              AEQuality quality, enum AVMatrixEncoding matrix_encoding, enum AVAudioServiceType audio_service_type,
                              int profile, bool wasActive)
{
  if (!IsActivated() || m_usedProcessesCnt >= AE_DSP_STREAM_MAX_STREAMS)
    return false;

  CSingleLock lock(m_critSection);

  AE_DSP_STREAMTYPE requestedStreamType = LoadCurrentAudioSettings();

  CActiveAEDSPProcessPtr usedProc;
  if (wasActive && streamId != (unsigned int)-1 && streamId < AE_DSP_STREAM_MAX_STREAMS)
  {
    if (m_usedProcesses[streamId] != NULL)
    {
      usedProc = m_usedProcesses[streamId];
    }
  }
  else
  {
    for (unsigned int i = 0; i < AE_DSP_STREAM_MAX_STREAMS; ++i)
    {
      /* find a free position */
      if (m_usedProcesses[i] == NULL)
      {
        usedProc = CActiveAEDSPProcessPtr(new CActiveAEDSPProcess(i));
        streamId = i;
        break;
      }
    }
  }

  if (usedProc == NULL)
  {
    CLog::Log(LOGERROR, "ActiveAE DSP - %s - can't find active processing class", __FUNCTION__);
    return false;
  }

  if (!usedProc->Create(inputFormat, outputFormat, upmix, quality, requestedStreamType, matrix_encoding, audio_service_type, profile))
  {
    CLog::Log(LOGERROR, "ActiveAE DSP - %s - Creation of processing class failed", __FUNCTION__);
    return false;
  }

  if (!wasActive)
  {
    process = usedProc;
    m_activeProcessId = streamId;
    m_usedProcesses[streamId] = usedProc;
    m_usedProcessesCnt++;
  }
  return true;
}

void CActiveAEDSP::DestroyDSPs(unsigned int streamId)
{
  CSingleLock lock(m_critSection);

  if (streamId != (unsigned int)-1 && m_usedProcesses[streamId] != NULL)
  {
    m_usedProcesses[streamId]->Destroy();
    m_usedProcesses[streamId] = CActiveAEDSPProcessPtr();
    --m_usedProcessesCnt;
  }
  if (m_usedProcessesCnt == 0)
  {
    m_activeProcessId = -1;
  }
}

CActiveAEDSPProcessPtr CActiveAEDSP::GetDSPProcess(unsigned int streamId)
{
  CSingleLock lock(m_critSection);

  if (streamId != (unsigned int)-1 && m_usedProcesses[streamId])
    return m_usedProcesses[streamId];
  return CActiveAEDSPProcessPtr();
}

unsigned int CActiveAEDSP::GetProcessingStreamsAmount(void)
{
  CSingleLock lock(m_critSection);
  return m_usedProcessesCnt;
}

unsigned int CActiveAEDSP::GetActiveStreamId(void)
{
  CSingleLock lock(m_critSection);

  return m_activeProcessId;
}

const AE_DSP_MODELIST &CActiveAEDSP::GetAvailableModes(AE_DSP_MODE_TYPE modeType)
{
  static AE_DSP_MODELIST emptyArray;
  if (modeType < 0 || modeType >= AE_DSP_MODE_TYPE_MAX)
    return emptyArray;

  CSingleLock lock(m_critSection);
  return m_modes[modeType];
}

/*! @name addon update process methods */
//@{
bool CActiveAEDSP::StopAudioDSPAddon(AddonPtr addon, bool bRestart)
{
  CSingleLock lock(m_critSection);

  int iId = GetAudioDSPAddonId(addon);
  AE_DSP_ADDON mappedAddon;
  if (GetReadyAudioDSPAddon(iId, mappedAddon))
  {
    if (bRestart)
      mappedAddon->ReCreate();
    else
      mappedAddon->Destroy();

    return true;
  }

  return false;
}

bool CActiveAEDSP::UpdateAndInitialiseAudioDSPAddons(bool bInitialiseAllAudioDSPAddons /* = false */)
{
  bool bReturn(true);
  VECADDONS map;
  VECADDONS disableAddons;
  {
    CSingleLock lock(m_critUpdateSection);
    map = m_addons;
  }

  if (map.size() == 0)
    return false;

  for (unsigned iAddonPtr = 0; iAddonPtr < map.size(); ++iAddonPtr)
  {
    const AddonPtr dspAddon = map.at(iAddonPtr);
    bool bEnabled = !CAddonMgr::GetInstance().IsAddonDisabled(dspAddon->ID());

    if (!bEnabled && IsKnownAudioDSPAddon(dspAddon))
    {
      CSingleLock lock(m_critUpdateSection);
      /* stop the dsp addon and remove it from the db */
      StopAudioDSPAddon(dspAddon, false);
      VECADDONS::iterator addonPtr = std::find(m_addons.begin(), m_addons.end(), dspAddon);
      if (addonPtr != m_addons.end())
        m_addons.erase(addonPtr);

    }
    else if (bEnabled && (bInitialiseAllAudioDSPAddons || !IsKnownAudioDSPAddon(dspAddon) || !IsReadyAudioDSPAddon(dspAddon)))
    {
      bool bDisabled(false);

      /* register the add-on in the audio dsp db, and create the CActiveAEDSPAddon instance */
      int iAddonId = RegisterAudioDSPAddon(dspAddon);
      if (iAddonId < 0)
      {
        /* failed to register or create the add-on, disable it */
        CLog::Log(LOGWARNING, "ActiveAE DSP - %s - failed to register add-on %s, disabling it", __FUNCTION__, dspAddon->Name().c_str());
        disableAddons.push_back(dspAddon);
        bDisabled = true;
      }
      else
      {
        ADDON_STATUS status(ADDON_STATUS_UNKNOWN);
        AE_DSP_ADDON addon;
        {
          CSingleLock lock(m_critUpdateSection);
          if (!GetAudioDSPAddon(iAddonId, addon))
          {
            CLog::Log(LOGWARNING, "ActiveAE DSP - %s - failed to find add-on %s, disabling it", __FUNCTION__, dspAddon->Name().c_str());
            disableAddons.push_back(dspAddon);
            bDisabled = true;
          }
        }

        /* re-check the enabled status. newly installed dsps get disabled when they're added to the db */
        if (!bDisabled && !CAddonMgr::GetInstance().IsAddonDisabled(addon->ID()) && (status = addon->Create(iAddonId)) != ADDON_STATUS_OK)
        {
          CLog::Log(LOGWARNING, "ActiveAE DSP - %s - failed to create add-on %s, status = %d", __FUNCTION__, dspAddon->Name().c_str(), status);
          if (!addon.get() || !addon->DllLoaded() || status == ADDON_STATUS_PERMANENT_FAILURE)
          {
            /* failed to load the dll of this add-on, disable it */
            CLog::Log(LOGWARNING, "ActiveAE DSP - %s - failed to load the dll for add-on %s, disabling it", __FUNCTION__, dspAddon->Name().c_str());
            disableAddons.push_back(dspAddon);
            bDisabled = true;
          }
        }
      }

      if (bDisabled && IsActivated())
        CGUIDialogOK::ShowAndGetInput(24070, 24071, 16029, 0);
    }
  }

  /* disable add-ons that failed to initialise */
  if (disableAddons.size() > 0)
  {
    CSingleLock lock(m_critUpdateSection);
    for (VECADDONS::iterator itr = disableAddons.begin(); itr != disableAddons.end(); ++itr)
    {
      /* disable in the add-on db */
      CAddonMgr::GetInstance().DisableAddon((*itr)->ID());

      /* remove from the audio dsp add-on list */
      VECADDONS::iterator addonPtr = std::find(m_addons.begin(), m_addons.end(), *itr);
      if (addonPtr != m_addons.end())
        m_addons.erase(addonPtr);
    }
  }

  return bReturn;
}

bool CActiveAEDSP::UpdateAddons(void)
{
  VECADDONS addons;
  AE_DSP_ADDON dspAddon;
  bool bReturn(CAddonMgr::GetInstance().GetAddons(ADDON_ADSPDLL, addons, true));
  size_t usableAddons;

  if (bReturn)
  {
    CSingleLock lock(m_critUpdateSection);
    m_addons = addons;
  }

  usableAddons = m_addons.size();

  /* handle "new" addons which aren't yet in the db - these have to be added first */
  for (VECADDONS::const_iterator itr = addons.begin(); itr != addons.end(); ++itr)
  {
    dspAddon = std::dynamic_pointer_cast<CActiveAEDSPAddon>(*itr);

    if (RegisterAudioDSPAddon(dspAddon) < 0)
    {
      CAddonMgr::GetInstance().DisableAddon(dspAddon->ID());
      --usableAddons;
    }
  }

  if ((!bReturn || usableAddons == 0) && !m_noAddonWarningDisplayed &&
      !CAddonMgr::GetInstance().HasAddons(ADDON_ADSPDLL, false) &&
      IsActivated())
  {
    // No audio DSP add-ons could be found
    // You need a add-on installed for the process of audio DSP signal. System becomes disabled.
    m_noAddonWarningDisplayed = true;
    CGUIDialogOK::ShowAndGetInput(CVariant{19273}, CVariant{19274});
    CSettings::GetInstance().SetBool(CSettings::SETTING_AUDIOOUTPUT_DSPADDONSENABLED, false);
    CGUIMessage msg(GUI_MSG_UPDATE, WINDOW_SETTINGS_SYSTEM, 0);
    CApplicationMessenger::GetInstance().SendGUIMessage(msg);
    CApplicationMessenger::GetInstance().PostMsg(TMSG_SETAUDIODSPSTATE, ACTIVE_AE_DSP_STATE_OFF);
  }

  return bReturn;
}

void CActiveAEDSP::Notify(const Observable &obs, const ObservableMessage msg)
{
  if (msg == ObservableMessageAddons)
    UpdateAddons();
}

void CActiveAEDSP::Process(void)
{
  bool bCheckedEnabledAddonsOnStartup(false);

  CAddonMgr::GetInstance().RegisterAddonMgrCallback(ADDON_ADSPDLL, this);
  CAddonMgr::GetInstance().RegisterObserver(this);

  m_isActive = true;

  UpdateAddons();

  while (!g_application.m_bStop && !m_bStop)
  {
    UpdateAndInitialiseAudioDSPAddons();

    if (!bCheckedEnabledAddonsOnStartup)
    {
      bCheckedEnabledAddonsOnStartup = true;
      if (HasEnabledAudioDSPAddons())
        TriggerModeUpdate(false);
      else if (!m_noAddonWarningDisplayed)
        ShowDialogNoAddonsEnabled();
    }
    else
    {
      Sleep(1000);
    }
  }

  m_isActive = false;
}

void CActiveAEDSP::ShowDialogNoAddonsEnabled(void)
{
  if (!IsActivated())
    return;

  CGUIDialogOK::ShowAndGetInput(15048, 15049, 0, 0);

  std::vector<std::string> params;
  params.push_back("addons://disabled/kodi.adsp");
  params.push_back("return");
  g_windowManager.ActivateWindow(WINDOW_ADDON_BROWSER, params);
}

int CActiveAEDSP::RegisterAudioDSPAddon(AddonPtr addon)
{
  int iAddonId(-1);

  if (CAddonMgr::GetInstance().IsAddonDisabled(addon->ID()))
    return -1;

  CLog::Log(LOGDEBUG, "ActiveAE DSP - %s - registering add-on '%s'", __FUNCTION__, addon->Name().c_str());

  if (!m_databaseDSP.IsOpen())
  {
    CLog::Log(LOGERROR, "ActiveAE DSP - %s - failed to get the database", __FUNCTION__);
    return -1;
  }

  /* check whether we already know this dsp addon */
  iAddonId = m_databaseDSP.GetAudioDSPAddonId(addon->ID());

  /* try to register the new dsp addon in the db */
  if (iAddonId < 0)
  {
    if ((iAddonId = m_databaseDSP.Persist(addon)) < 0)
    {
      CLog::Log(LOGERROR, "ActiveAE DSP - %s - can't add dsp addon '%s' to the database", __FUNCTION__, addon->Name().c_str());
      return -1;
    }
  }

  AE_DSP_ADDON dspAddon;
  /* load and initialise the dsp addon libraries */
  {
    CSingleLock lock(m_critSection);
    AE_DSP_ADDONMAP_CITR existingAddon = m_addonMap.find(iAddonId);
    if (existingAddon != m_addonMap.end())
    {
      /* return existing addon */
      dspAddon = existingAddon->second;
    }
    else
    {
      /* create a new addon instance */
      dspAddon = std::dynamic_pointer_cast<CActiveAEDSPAddon> (addon);
      m_addonMap.insert(std::make_pair(iAddonId, dspAddon));
    }
  }

  if (iAddonId < 0)
    CLog::Log(LOGERROR, "ActiveAE DSP - %s - can't register dsp add-on '%s'", __FUNCTION__, addon->Name().c_str());

  return iAddonId;
}
//@}

/*! @name Played source settings methods
 *  @note for save of settings see CSaveFileStateJob */
//@{
AE_DSP_STREAMTYPE CActiveAEDSP::LoadCurrentAudioSettings(void)
{
  CSingleLock lock(m_critSection);

  AE_DSP_STREAMTYPE type = AE_DSP_ASTREAM_INVALID;

  if (g_application.m_pPlayer->HasPlayer())
  {
    CFileItem currentFile(g_application.CurrentFileItem());

    /* load the persisted audio settings and set them as current */
    CAudioSettings loadedAudioSettings = CMediaSettings::GetInstance().GetDefaultAudioSettings();
    m_databaseDSP.GetActiveDSPSettings(currentFile, loadedAudioSettings);

    CMediaSettings::GetInstance().GetCurrentAudioSettings() = loadedAudioSettings;
    type = (AE_DSP_STREAMTYPE) loadedAudioSettings.m_MasterStreamTypeSel;

    /* settings can be saved on next audio stream change */
    m_isValidAudioDSPSettings = true;
  }
  return type;
}
//@}

/*! @name Backend methods */
//@{

bool CActiveAEDSP::IsProcessing(void) const
{
  return m_isActive && m_usedProcessesCnt > 0;
}

bool CActiveAEDSP::IsActivated(void) const
{
  return m_isActive;
}

int CActiveAEDSP::EnabledAudioDSPAddonAmount(void) const
{
  int iReturn(0);
  CSingleLock lock(m_critUpdateSection);

  for (AE_DSP_ADDONMAP_CITR citr = m_addonMap.begin(); citr != m_addonMap.end(); ++citr)
  {
    if (!CAddonMgr::GetInstance().IsAddonDisabled(citr->second->ID()))
      ++iReturn;
  }

  return iReturn;
}

bool CActiveAEDSP::HasEnabledAudioDSPAddons(void) const
{
  return EnabledAudioDSPAddonAmount() > 0;
}

int CActiveAEDSP::GetEnabledAudioDSPAddons(AE_DSP_ADDONMAP &addons) const
{
  int iReturn(0);
  CSingleLock lock(m_critUpdateSection);

  for (AE_DSP_ADDONMAP_CITR citr = m_addonMap.begin(); citr != m_addonMap.end(); ++citr)
  {
    if (!CAddonMgr::GetInstance().IsAddonDisabled(citr->second->ID()))
    {
      addons.insert(std::make_pair(citr->second->GetID(), citr->second));
      ++iReturn;
    }
  }

  return iReturn;
}

int CActiveAEDSP::ReadyAudioDSPAddonAmount(void) const
{
  int iReturn(0);
  CSingleLock lock(m_critUpdateSection);

  for (AE_DSP_ADDONMAP_CITR citr = m_addonMap.begin(); citr != m_addonMap.end(); ++citr)
  {
    if (citr->second->ReadyToUse())
      ++iReturn;
  }

  return iReturn;
}

bool CActiveAEDSP::HasReadyAudioDSPAddons(void) const
{
  return ReadyAudioDSPAddonAmount() > 0;
}

bool CActiveAEDSP::IsReadyAudioDSPAddon(int iAddonId) const
{
  AE_DSP_ADDON addon;
  return GetReadyAudioDSPAddon(iAddonId, addon);
}

bool CActiveAEDSP::IsReadyAudioDSPAddon(const AddonPtr &addon)
{
  CSingleLock lock(m_critUpdateSection);

  for (AE_DSP_ADDONMAP_CITR citr = m_addonMap.begin(); citr != m_addonMap.end(); ++citr)
  {
    if (citr->second->ID() == addon->ID())
      return citr->second->ReadyToUse();
  }

  return false;
}

int CActiveAEDSP::GetReadyAddons(AE_DSP_ADDONMAP &addons) const
{
  int iReturn(0);
  CSingleLock lock(m_critSection);

  for (AE_DSP_ADDONMAP_CITR citr = m_addonMap.begin(); citr != m_addonMap.end(); ++citr)
  {
    if (citr->second->ReadyToUse())
    {
      addons.insert(std::make_pair(citr->second->GetID(), citr->second));
      ++iReturn;
    }
  }

  return iReturn;
}

bool CActiveAEDSP::GetReadyAudioDSPAddon(int iAddonId, AE_DSP_ADDON &addon) const
{
  if (GetAudioDSPAddon(iAddonId, addon))
    return addon->ReadyToUse();
  return false;
}

bool CActiveAEDSP::GetAudioDSPAddonName(int iAddonId, std::string &strName) const
{
  bool bReturn(false);
  AE_DSP_ADDON addon;
  if ((bReturn = GetReadyAudioDSPAddon(iAddonId, addon)) == true)
    strName = addon->GetAudioDSPName();

  return bReturn;
}

bool CActiveAEDSP::GetAudioDSPAddon(int iAddonId, AE_DSP_ADDON &addon) const
{
  bool bReturn(false);
  if (iAddonId <= AE_DSP_INVALID_ADDON_ID)
    return bReturn;

  CSingleLock lock(m_critUpdateSection);

  AE_DSP_ADDONMAP_CITR citr = m_addonMap.find(iAddonId);
  if (citr != m_addonMap.end())
  {
    addon = citr->second;
    bReturn = true;
  }

  return bReturn;
}

bool CActiveAEDSP::GetAudioDSPAddon(const std::string &strId, AddonPtr &addon) const
{
  CSingleLock lock(m_critUpdateSection);
  for (AE_DSP_ADDONMAP_CITR citr = m_addonMap.begin(); citr != m_addonMap.end(); ++citr)
  {
    if (citr->second->ID() == strId)
    {
      addon = citr->second;
      return true;
    }
  }
  return false;
}
//@}

/*! @name Menu hook methods */
//@{
bool CActiveAEDSP::HaveMenuHooks(AE_DSP_MENUHOOK_CAT cat, int iDSPAddonID)
{
  for (AE_DSP_ADDONMAP_CITR citr = m_addonMap.begin(); citr != m_addonMap.end(); ++citr)
  {
    if (citr->second->ReadyToUse())
    {
      if (citr->second->HaveMenuHooks(cat))
      {
        if (iDSPAddonID > 0 && citr->second->GetID() == iDSPAddonID)
          return true;
        else if (iDSPAddonID < 0)
          return true;
      }
      else if (cat == AE_DSP_MENUHOOK_SETTING)
      {
        AddonPtr addon;
        if (CAddonMgr::GetInstance().GetAddon(citr->second->ID(), addon) && addon->HasSettings())
          return true;
      }
    }
  }

  return false;
}

bool CActiveAEDSP::GetMenuHooks(int iDSPAddonID, AE_DSP_MENUHOOK_CAT cat, AE_DSP_MENUHOOKS &hooks)
{
  bool bReturn(false);

  if (iDSPAddonID < 0)
    return bReturn;

  AE_DSP_ADDON addon;
  if (GetReadyAudioDSPAddon(iDSPAddonID, addon) && addon->HaveMenuHooks(cat))
  {
    AE_DSP_MENUHOOKS *addonhooks = addon->GetMenuHooks();
    for (unsigned int i = 0; i < addonhooks->size(); ++i)
    {
      if (cat == AE_DSP_MENUHOOK_ALL || addonhooks->at(i).category == cat)
      {
        hooks.push_back(addonhooks->at(i));
        bReturn = true;
      }
    }
  }

  return bReturn;
}
//@}

/*! @name General helper functions */
//@{
enum AEChannel CActiveAEDSP::GetKODIChannel(AE_DSP_CHANNEL channel)
{
  switch (channel)
  {
  case AE_DSP_CH_FL:   return AE_CH_FL;
  case AE_DSP_CH_FR:   return AE_CH_FR;
  case AE_DSP_CH_FC:   return AE_CH_FC;
  case AE_DSP_CH_LFE:  return AE_CH_LFE;
  case AE_DSP_CH_BL:   return AE_CH_BL;
  case AE_DSP_CH_BR:   return AE_CH_BR;
  case AE_DSP_CH_FLOC: return AE_CH_FLOC;
  case AE_DSP_CH_FROC: return AE_CH_FROC;
  case AE_DSP_CH_BC:   return AE_CH_BC;
  case AE_DSP_CH_SL:   return AE_CH_SL;
  case AE_DSP_CH_SR:   return AE_CH_SR;
  case AE_DSP_CH_TC:   return AE_CH_TC;
  case AE_DSP_CH_TFL:  return AE_CH_TFL;
  case AE_DSP_CH_TFC:  return AE_CH_TFC;
  case AE_DSP_CH_TFR:  return AE_CH_TFR;
  case AE_DSP_CH_TBL:  return AE_CH_TBL;
  case AE_DSP_CH_TBC:  return AE_CH_TBC;
  case AE_DSP_CH_TBR:  return AE_CH_TBR;
  default:
    return AE_CH_NULL;
  }
}

AE_DSP_CHANNEL CActiveAEDSP::GetDSPChannel(enum AEChannel channel)
{
  switch (channel)
  {
  case AE_CH_FL:   return AE_DSP_CH_FL;
  case AE_CH_FR:   return AE_DSP_CH_FR;
  case AE_CH_FC:   return AE_DSP_CH_FC;
  case AE_CH_LFE:  return AE_DSP_CH_LFE;
  case AE_CH_BL:   return AE_DSP_CH_BL;
  case AE_CH_BR:   return AE_DSP_CH_BR;
  case AE_CH_FLOC: return AE_DSP_CH_FLOC;
  case AE_CH_FROC: return AE_DSP_CH_FROC;
  case AE_CH_BC:   return AE_DSP_CH_BC;
  case AE_CH_SL:   return AE_DSP_CH_SL;
  case AE_CH_SR:   return AE_DSP_CH_SR;
  case AE_CH_TC:   return AE_DSP_CH_TC;
  case AE_CH_TFL:  return AE_DSP_CH_TFL;
  case AE_CH_TFC:  return AE_DSP_CH_TFC;
  case AE_CH_TFR:  return AE_DSP_CH_TFR;
  case AE_CH_TBL:  return AE_DSP_CH_TBL;
  case AE_CH_TBC:  return AE_DSP_CH_TBC;
  case AE_CH_TBR:  return AE_DSP_CH_TBR;
  default:
    return AE_DSP_CH_INVALID;
  }
}

/*!
 * Contains string name id's related to the AE_DSP_ASTREAM_ values
 */
const int CActiveAEDSP::m_StreamTypeNameTable[] =
{
  15004,  //!< "Basic"
  249,    //!< "Music"
  157,    //!< "Video"
  15016,  //!< "Games"
  15005,  //!< "Application"
  15006,  //!< "Phone"
  15007,  //!< "Message"
  14061   //!< "Auto"
};

int CActiveAEDSP::GetStreamTypeName(unsigned int streamType)
{
  if (streamType > AE_DSP_ASTREAM_AUTO)
    return -1;
  return m_StreamTypeNameTable[streamType];
}
//@}
