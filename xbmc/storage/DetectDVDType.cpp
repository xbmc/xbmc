/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DetectDVDType.h"

#include "GUIUserMessages.h"
#include "ServiceBroker.h"
#include "application/ApplicationComponents.h"
#include "application/ApplicationPlayer.h"
#include "cdioSupport.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "resources/LocalizeStrings.h"
#include "resources/ResourcesComponent.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "storage/MediaManager.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

#include <atomic>
#include <mutex>

using namespace MEDIA_DETECT;
using namespace std::chrono_literals;

CCriticalSection CDetectDVDMedia::m_muReadingMedia;
CCriticalSection CDetectDVDMedia::m_muDetect;
CEvent CDetectDVDMedia::m_evAutorun;
DriveState CDetectDVDMedia::m_DriveState{DriveState::CLOSED_NO_MEDIA};
CCdInfo* CDetectDVDMedia::m_pCdInfo = NULL;
time_t CDetectDVDMedia::m_LastPoll = 0;
std::atomic<bool> CDetectDVDMedia::m_bInstanceExists{false};
std::string CDetectDVDMedia::m_diskLabel = "";
std::string CDetectDVDMedia::m_diskPath = "";
UTILS::DISCS::DiscInfo CDetectDVDMedia::m_discInfo;

CDetectDVDMedia::CDetectDVDMedia() : CThread("DetectDVDMedia"),
  m_cdio(CLibcdio::GetInstance())
{
  m_bStop = false;
  m_bInstanceExists = true;
}

CDetectDVDMedia::~CDetectDVDMedia()
{
  m_bInstanceExists = false;
}

void CDetectDVDMedia::OnStartup()
{
  // SetPriority( ThreadPriority::LOWEST );
  CLog::Log(LOGDEBUG, "Compiled with libcdio Version 0.{}", LIBCDIO_VERSION_NUM);
}

void CDetectDVDMedia::Process()
{
// for apple - currently disable this check since cdio will return null if no media is loaded
#if !defined(TARGET_DARWIN)
  //Before entering loop make sure we actually have a CDrom drive
  CdIo_t *p_cdio = m_cdio->cdio_open(NULL, DRIVER_DEVICE);
  if (p_cdio == NULL)
    return;
  else
    m_cdio->cdio_destroy(p_cdio);
#endif

  const auto& components = CServiceBroker::GetAppComponents();
  const auto appPlayer = components.GetComponent<CApplicationPlayer>();

  while (!m_bStop)
  {
    if (appPlayer->IsPlayingVideo())
    {
      CThread::Sleep(10000ms);
    }
    else
    {
      UpdateDvdrom();
      m_bStartup = false;
      CThread::Sleep(2000ms);
      if (m_bAutorun)
      {
        // Media in drive, wait 1.5s more to be sure the device is ready for playback
        CThread::Sleep(1500ms);
        m_evAutorun.Set();
        m_bAutorun = false;
      }
    }
  }
}

void CDetectDVDMedia::OnExit()
{
}

// Gets state of the DVD drive
void CDetectDVDMedia::UpdateDvdrom()
{
  // Serialise against DetectMediaType(): a probe that started before a state change
  // detected here would otherwise finish after it and republish the old disc
  std::unique_lock detectLock(m_muDetect);

  // Signal for WaitMediaReady()
  // that we are busy detecting the
  // newly inserted media.
  {
    std::unique_lock waitLock(m_muReadingMedia);
    switch (PollDriveState())
    {
      case DriveState::NONE:
        //! @todo reduce / stop polling for drive updates
        break;

      case DriveState::OPEN:
      {
        // Send Message to GUI that disc been ejected
        SetNewDVDShareUrl(CServiceBroker::GetMediaManager().TranslateDevicePath(m_diskPath), false,
                          CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(502));
        CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_REMOVED_MEDIA);
        CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg);
        // Discard the details of the disc that was in the drive
        Clear();
        // Update drive state
        waitLock.unlock();
        m_DriveState = DriveState::OPEN;
        return;
      }
      break;
      case DriveState::NOT_READY:
      {
        // Drive is not ready (closing, opening)
        SetNewDVDShareUrl(CServiceBroker::GetMediaManager().TranslateDevicePath(m_diskPath), false,
                          CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(503));
        m_DriveState = DriveState::NOT_READY;
        // DVD-ROM in undefined state - discard the details of the old disc
        Clear();
        waitLock.unlock();
        CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UPDATE_SOURCES);
        CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg);
        // Do we really need sleep here? This will fix: [ 1530771 ] "Open tray" problem
        // CThread::Sleep(6000ms);
        return;
      }
      break;

      case DriveState::CLOSED_NO_MEDIA:
      {
        // Nothing in there...
        SetNewDVDShareUrl(CServiceBroker::GetMediaManager().TranslateDevicePath(m_diskPath), false,
                          CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(504));
        m_DriveState = DriveState::CLOSED_NO_MEDIA;
        // Nothing in the drive, so discard the details of the old disc
        Clear();
        // Send Message to GUI that disc has changed
        CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UPDATE_SOURCES);
        waitLock.unlock();
        CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg);
        return;
      }
      break;
      case DriveState::READY:
#if !defined(TARGET_DARWIN)
        return ;
#endif
        break;
      case DriveState::CLOSED_MEDIA_PRESENT:
      {
        if (m_DriveState != DriveState::CLOSED_MEDIA_PRESENT)
        {
          m_DriveState = DriveState::CLOSED_MEDIA_PRESENT;
          waitLock.unlock(); // Locking occurs in DetectMediaType()
          // Detect ISO9660(mode1/mode2) or CDDA filesystem
          DetectMediaType();
          CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UPDATE_SOURCES);
          CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg);
          // Tell the application object that a new Cd is inserted
          // So autorun can be started.
          if (!m_bStartup)
            m_bAutorun = true;
        }
        return;
      }
      default:
        break;
    }

    // We have finished media detection
    // Signal for WaitMediaReady()
  }


}

// Generates the drive url, (like iso9660://)
// from the CCdInfo class
void CDetectDVDMedia::DetectMediaType()
{
  bool bCDDA(false);
  CLog::Log(LOGINFO, "Detecting DVD-ROM media filesystem...");

  // Lock before the path is read - the drive state (and with it m_diskPath) must
  // not be able to change between the snapshot and the probe that acts on it
  std::unique_lock detectLock(m_muDetect);

  std::string devicePath;
  {
    std::unique_lock waitLock(m_muReadingMedia);
    devicePath = CServiceBroker::GetMediaManager().TranslateDevicePath(m_diskPath);
  }

  // Detect new CD-Information. Scan the tracks first: this is comparatively
  // cheap and tells us whether we should then probe for a video disc
  CCdIoSupport cdio;
  CCdInfo* pCdInfo = cdio.GetCdInfo();

  // Probe and store DiscInfo result.
  // Even if no valid tracks are detected we might still be able to play the disc
  // via libdvdnav or libbluray as long as they can correctly detect the disc.
  // Only skip discs that cannot possibly be a DVD or Blu-ray, i.e. ones
  // where every track is audio.
  UTILS::DISCS::DiscInfo discInfo;
  bool haveDiscInfo = false;
  if (pCdInfo == nullptr || pCdInfo->HasDataTracks())
  {
    haveDiscInfo = UTILS::DISCS::GetDiscInfo(discInfo, devicePath);
  }
  else
  {
    CLog::Log(LOGDEBUG, "Not probing for a video disc: no data tracks on the disc");
  }

  // Probing is done - lock and publish the results
  std::unique_lock waitLock(m_muReadingMedia);

  // Publish the result of the probe, including a negative one (otherwise
  // previous disc info would be used)
  if (haveDiscInfo)
  {
    m_discInfo = discInfo;
  }
  else
  {
    m_discInfo.clear();
  }

  // Replace the old CD-Information
  delete m_pCdInfo;
  m_pCdInfo = pCdInfo;

  if (m_pCdInfo == nullptr)
  {
    CLog::Log(LOGERROR, "Detection of DVD-ROM media failed.");
    // Nothing could be read from the disc, so fall back to the default label rather
    // than returning here and leaving the previous disc's details in place for
    // GetDVDLabel()/GetDVDPath()
    SetNewDVDShareUrl(devicePath, false, "");
    return;
  }
  CLog::Log(LOGINFO, "Tracks overall:{}; Audio tracks:{}; Data tracks:{}",
            m_pCdInfo->GetTrackCount(), m_pCdInfo->GetAudioTrackCount(),
            m_pCdInfo->GetDataTrackCount());

  std::string strNewUrl;

  // Detect ISO9660(mode1/mode2), CDDA filesystem or UDF
  if (m_pCdInfo->IsISOHFS(1) || m_pCdInfo->IsIso9660(1) || m_pCdInfo->IsIso9660Interactive(1))
  {
    strNewUrl = "iso9660://";
  }
  else
  {
    if (m_pCdInfo->IsUDF(1))
      strNewUrl = devicePath;
    else if (m_pCdInfo->IsAudio(1))
    {
      strNewUrl = "cdda://local/";
      bCDDA = true;
    }
    else
      strNewUrl = devicePath;
  }

  if (m_pCdInfo->IsISOUDF(1))
  {
    if (!CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_detectAsUdf)
    {
      strNewUrl = "iso9660://";
    }
    else
    {
      strNewUrl = devicePath;
    }
  }

  CLog::Log(LOGINFO, "Using protocol {}", strNewUrl);

  if (m_pCdInfo->IsValidFs())
  {
    if (!m_pCdInfo->IsAudio(1))
      CLog::Log(LOGINFO, "Disc label: {}", m_pCdInfo->GetDiscLabel());
  }
  else
  {
    CLog::Log(LOGWARNING, "Filesystem is not supported");
  }

  std::string strLabel;
  if (bCDDA)
  {
    strLabel = "Audio-CD";
  }
  else
  {
    strLabel = m_pCdInfo->GetDiscLabel();
    StringUtils::TrimRight(strLabel);
  }

  SetNewDVDShareUrl( strNewUrl , bCDDA, strLabel);
}

void CDetectDVDMedia::SetNewDVDShareUrl(const std::string& strNewUrl,
                                        bool bCDDA,
                                        const std::string& strDiscLabel)
{
  std::string strDescription = "DVD";
  if (bCDDA) strDescription = "CD";

  if (!strDiscLabel.empty())
    strDescription = strDiscLabel;

  // Store it in case others want it
  m_diskLabel = strDescription;
  m_diskPath = strNewUrl;
}

DriveState CDetectDVDMedia::PollDriveState()
{
  const std::shared_ptr<IDiscDriveHandler> platformDiscDriveHandler =
      CServiceBroker::GetMediaManager().GetDiscDriveHandler();
  if (!platformDiscDriveHandler)
  {
    return DriveState::NONE;
  }

  const std::string discPath = CServiceBroker::GetMediaManager().TranslateDevicePath("");
  const DriveState driveState = platformDiscDriveHandler->GetDriveState(discPath);
  switch (driveState)
  {
    case DriveState::CLOSED_MEDIA_UNDEFINED:
      // We only poll for new traystatus when driveState has changed or if the last recorded
      // tray state is undefined
      if (driveState == DriveState::CLOSED_MEDIA_UNDEFINED &&
          (m_LastTrayState == TrayState::UNDEFINED || driveState != m_LastDriveState))
      {
        m_TrayState = platformDiscDriveHandler->GetTrayState(discPath);
      }
      break;
    case DriveState::OPEN:
      m_TrayState = TrayState::OPEN;
      break;
    default:
      m_TrayState = TrayState::UNDEFINED;
      break;
  }
  m_LastDriveState = driveState;

  if (m_TrayState == TrayState::CLOSED_MEDIA_PRESENT)
  {
    if (m_LastTrayState != TrayState::CLOSED_MEDIA_PRESENT)
    {
      m_LastTrayState = m_TrayState;
      return DriveState::CLOSED_MEDIA_PRESENT;
    }
    else
    {
      return DriveState::READY;
    }
  }
  else if (m_TrayState == TrayState::CLOSED_NO_MEDIA)
  {
    if ((m_LastTrayState != TrayState::CLOSED_NO_MEDIA) &&
        (m_LastTrayState != TrayState::CLOSED_MEDIA_PRESENT))
    {
      m_LastTrayState = m_TrayState;
      return DriveState::CLOSED_NO_MEDIA;
    }
    else
    {
      return DriveState::READY;
    }
  }
  else if (m_TrayState == TrayState::OPEN)
  {
    if (m_LastTrayState != TrayState::OPEN)
    {
      m_LastTrayState = m_TrayState;
      return DriveState::OPEN;
    }
    else
    {
      return DriveState::READY;
    }
  }
  else
  {
    m_LastTrayState = m_TrayState;
  }

#ifdef HAS_OPTICAL_DRIVE
  return DriveState::NOT_READY;
#else
  return DriveState::READY;
#endif
}

void CDetectDVDMedia::UpdateState()
{
  // Do nothing unless a detection thread exists
  if (!m_bInstanceExists)
    return;

  DetectMediaType();
}

// Static function
// Wait for drive, to finish media detection.
void CDetectDVDMedia::WaitMediaReady()
{
  std::unique_lock detectLock(m_muDetect);
  std::unique_lock waitLock(m_muReadingMedia);
}

DriveState CDetectDVDMedia::GetDriveState()
{
  return m_DriveState;
}

// Static function
// Whether a disc is in drive
bool CDetectDVDMedia::IsDiscInDrive()
{
  return m_DriveState == DriveState::CLOSED_MEDIA_PRESENT;
}

// Static function
// Returns a CCdInfo class, which contains
// Media information of the current inserted CD.
// Can be NULL
CCdInfo* CDetectDVDMedia::GetCdInfo()
{
  std::unique_lock waitLock(m_muReadingMedia);
  CCdInfo* pCdInfo = m_pCdInfo;
  return pCdInfo;
}

std::string CDetectDVDMedia::GetDVDLabel()
{
  std::unique_lock waitLock(m_muReadingMedia);
  if (!m_discInfo.empty())
  {
    return m_discInfo.name;
  }

  return m_diskLabel;
}

std::string CDetectDVDMedia::GetDVDPath()
{
  std::unique_lock waitLock(m_muReadingMedia);
  return m_diskPath;
}


void CDetectDVDMedia::Clear()
{
  if (!m_discInfo.empty())
  {
    m_discInfo.clear();
  }

  delete m_pCdInfo;
  m_pCdInfo = nullptr;
}
