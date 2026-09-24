/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "MediaManager.h"

#include "Autorun.h"
#include "AutorunMediaJob.h"
#include "FileItem.h"
#include "GUIInfoManager.h"
#include "GUIUserMessages.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "addons/VFSEntry.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "dialogs/GUIDialogPlayEject.h"
#ifdef HAVE_LIBBLURAY
#include "filesystem/BlurayDirectory.h"
#include "filesystem/BlurayDiscCache.h"
#endif
#include "filesystem/File.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "jobs/JobManager.h"
#include "messaging/ApplicationMessenger.h"
#include "messaging/helpers/DialogOKHelper.h"
#ifdef TARGET_WINDOWS
#include "utils/CharsetConverter.h"
#endif
#include "resources/LocalizeStrings.h"
#include "resources/ResourcesComponent.h"
#include "settings/AdvancedSettings.h"
#include "settings/MediaSourceSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/FileUtils.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/XBMCTinyXML2.h"
#include "utils/XMLUtils.h"
#include "utils/log.h"
#include "video/VideoDatabase.h"

#ifdef HAS_OPTICAL_DRIVE
#ifndef TARGET_WINDOWS
//! @todo switch all ports to use auto sources
#include "DetectDVDType.h"

#include <map>
#include <utility>
#endif
#endif

#include <cctype>
#include <chrono>
#include <string>
#include <thread>
#include <vector>

#ifdef HAS_OPTICAL_DRIVE
using namespace MEDIA_DETECT;
#endif

const char MEDIA_SOURCES_XML[] = { "special://profile/mediasources.xml" };

#if defined(TARGET_WINDOWS) && defined(HAS_OPTICAL_DRIVE)
namespace
{
constexpr auto DRIVE_STATUS_RETRY_INTERVAL{std::chrono::seconds(2)};
/*! Windows does not always report a disc change made with the drive's own eject button, so even a
    stable state is confirmed now and then. One verify IOCTL, which does not spin the disc up */
constexpr auto DRIVE_STATUS_REFRESH_INTERVAL{std::chrono::seconds(10)};
/*! A disc with no label yet is still mounting, so it is read again soon */
constexpr auto MOUNTING_DISC_RETRY_INTERVAL{std::chrono::seconds(5)};
/*! A labelled disc that did not identify itself is usually a data disc, or a Blu-ray libbluray
    cannot open - a stable condition, and each attempt can cost seconds on the main thread */
constexpr auto UNIDENTIFIED_DISC_RETRY_INTERVAL{std::chrono::seconds(60)};
/*! How long a disc whose TOC could not be read is left alone before trying again - see GetCdInfo */
constexpr auto CDINFO_RETRY_INTERVAL{std::chrono::seconds(30)};
/*! A drive answers NOT_READY while it is busy, and goes on answering it once the drive itself
    has gone, so an arrival is given this long to resolve into a disc */
constexpr auto DRIVE_SETTLE_TIMEOUT{std::chrono::seconds(10)};
constexpr auto DRIVE_SETTLE_INTERVAL{std::chrono::milliseconds(500)};

const char* DriveStateName(DriveState state)
{
  switch (state)
  {
    using enum DriveState;
    case OPEN:
      return "open";
    case NOT_READY:
      return "not ready";
    case READY:
      return "ready";
    case CLOSED_NO_MEDIA:
      return "closed, no media";
    case CLOSED_MEDIA_PRESENT:
      return "closed, media present";
    case NONE:
      return "no drive";
    case CLOSED_MEDIA_UNDEFINED:
      return "closed, media undefined";
  }
  return "unknown";
}
} // namespace
#endif

#ifdef HAS_OPTICAL_DRIVE
namespace
{
/*! Elsewhere CDetectDVDMedia notices a new disc itself and raises the autorun that
    CAutorun::HandleAutorun() executes, so the storage event must not run it as well */
#ifdef TARGET_WINDOWS
constexpr bool AUTORUN_FROM_STORAGE_EVENT{true};
#else
constexpr bool AUTORUN_FROM_STORAGE_EVENT{false};
#endif

struct PostedWork
{
  KODI::MESSAGING::ThreadMessageCallback message;
  std::function<void()> work;
};

void RunCallback(void* userptr)
{
  const std::unique_ptr<PostedWork> posted{static_cast<PostedWork*>(userptr)};
  posted->work();
}

/*! \brief Run something on the application thread, without waiting for it
 *
 * The work is posted rather than sent, so a job thread never blocks on the application thread -
 * which would deadlock at shutdown, where the job manager is drained before the message queue.
 */
void RunOnAppThread(std::function<void()> work)
{
  if (CServiceBroker::GetAppMessenger()->IsProcessThread())
  {
    work();
    return;
  }

  auto* posted{new PostedWork};
  posted->work = std::move(work);
  posted->message = {&RunCallback, posted};
  CServiceBroker::GetAppMessenger()->PostMsg(TMSG_CALLBACK, -1, -1,
                                             static_cast<void*>(&posted->message));
}

/*! \brief Run autorun for a disc on the application thread
 *
 * CAutorun::RunDisc() reaches CApplication::PlayFile() directly rather than through the
 * messenger, so it has to be marshalled back rather than run on the job thread.
 */
void ExecuteAutorunOnAppThread(
    const std::string& path, const std::string& label, bool audio, bool hasToc, uint64_t generation)
{
  RunOnAppThread(
      [path, label, audio, hasToc, generation]()
      {
        // The disc may have gone while this waited for the application thread
        if (!CServiceBroker::GetMediaManager().IsDiscCurrent(path, generation))
          return;

        // No TOC means ExecuteAutorun() would read the disc again, on this thread
        if (hasToc && MEDIA_DETECT::CAutorun::ExecuteAutorun(path))
          return;

        if (audio)
          CLog::LogF(LOGDEBUG, "Could not execute autorun (rip/play) for audio CD with path {}",
                     path);
        else
          CLog::LogF(LOGDEBUG, "Could not execute autorun for video disc with path {}", path);

        CGUIDialogKaiToast::QueueNotification(
            CGUIDialogKaiToast::Info,
            CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(13019), label,
            TOAST_DISPLAY_TIME, false);
      });
}
} // namespace
#endif

CMediaManager::CMediaManager()
{
  m_bOpticalDrivePresent = false;
}

void CMediaManager::Stop()
{
  if (m_platformStorage)
    m_platformStorage->Stop();

  m_platformStorage.reset();
}

void CMediaManager::ScanForPresentMedia()
{
  std::unique_lock lock(m_CritSecStorageProvider);
  if (m_platformStorage)
    m_platformStorage->ScanForPresentMedia();
}

void CMediaManager::Initialize()
{
  if (!m_platformStorage)
  {
    m_platformStorage = IStorageProvider::CreateInstance();
  }
#ifdef HAS_OPTICAL_DRIVE
  m_platformDiscDriveHander = IDiscDriveHandler::CreateInstance();
  m_strFirstAvailDrive = m_platformStorage->GetFirstOpticalDeviceFileName();
#endif
  m_platformStorage->Initialize();
#ifndef TARGET_WINDOWS
  {
    // Discs already in the drive(s)
    std::unique_lock lock(m_CritSecStorageProvider);
    m_removableDrivePaths = GetRemovableDrivePaths();
  }
#endif
}

void CMediaManager::LoadSources()
{
  // clear our location list
  m_locations.clear();

  // Add-on sources are always present (and marked as "ignored" when saving XML)
  LoadAddonSources();

  // No more work to do if mediasources.xml doesn't exist
  if (!XFILE::CFile::Exists(MEDIA_SOURCES_XML))
  {
    CLog::Log(LOGDEBUG, "No media sources file at {}", MEDIA_SOURCES_XML);
    return;
  }

  // load xml file...
  CXBMCTinyXML2 xmlDoc;
  if (!xmlDoc.LoadFile(MEDIA_SOURCES_XML))
  {
    CLog::Log(LOGERROR, "Error loading {}, Line {} ({})", MEDIA_SOURCES_XML, xmlDoc.ErrorLineNum(),
              xmlDoc.ErrorStr());
    return;
  }

  auto* pRootElement = xmlDoc.RootElement();
  if (!pRootElement || StringUtils::CompareNoCase(pRootElement->Value(), "mediasources") != 0)
  {
    CLog::Log(LOGERROR, "Error loading {}, missing root <mediasources> element", MEDIA_SOURCES_XML);
    return;
  }

  // load the <network> block
  auto* pNetwork = pRootElement->FirstChildElement("network");
  if (pNetwork)
  {
    auto* pLocation = pNetwork->FirstChildElement("location");
    while (pLocation)
    {
      CNetworkLocation location;
      location.id = pLocation->IntAttribute("id");
      if (pLocation->FirstChild())
      {
        location.path = pLocation->FirstChild()->Value();
        m_locations.push_back(location);
      }
      pLocation = pLocation->NextSiblingElement("location");
    }
  }
}

bool CMediaManager::SaveSources()
{
  CXBMCTinyXML2 doc;
  auto* xmlRootElement = doc.NewElement("mediasources");
  auto* rootNode = doc.InsertFirstChild(xmlRootElement);

  if (!rootNode)
    return false;

  auto* networkElement = doc.NewElement("network");
  auto* networkNode = rootNode->InsertEndChild(networkElement);
  if (networkNode)
  {
    for (std::vector<CNetworkLocation>::iterator it = m_locations.begin(); it != m_locations.end(); ++it)
    {
      auto* locationNode = doc.NewElement("location");
      locationNode->SetAttribute("id", (*it).id);
      auto* value = doc.NewText((*it).path.c_str());
      locationNode->InsertEndChild(value);
      networkNode->InsertEndChild(locationNode);
    }
  }
  return doc.SaveFile(MEDIA_SOURCES_XML);
}

void CMediaManager::GetLocalDrives(std::vector<CMediaSource>& localDrives, bool includeQ)
{
  std::unique_lock lock(m_CritSecStorageProvider);
  m_platformStorage->GetLocalDrives(localDrives);
}

void CMediaManager::GetRemovableDrives(std::vector<CMediaSource>& removableDrives)
{
  std::unique_lock lock(m_CritSecStorageProvider);
  if (m_platformStorage)
    m_platformStorage->GetRemovableDrives(removableDrives);
}

void CMediaManager::GetNetworkLocations(std::vector<CMediaSource>& locations, bool autolocations)
{
  for (unsigned int i = 0; i < m_locations.size(); i++)
  {
    CMediaSource share;
    share.strPath = m_locations[i].path;
    CURL url(share.strPath);
    share.strName = url.GetWithoutUserDetails();
    locations.push_back(share);
  }
  if (autolocations)
  {
    CMediaSource share;
    share.m_ignore = true;
#ifdef HAS_FILESYSTEM_SMB
    share.strPath = "smb://";
    share.strName = CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(20171);
    locations.push_back(share);
#endif

#ifdef HAS_FILESYSTEM_NFS
    share.strPath = "nfs://";
    share.strName = CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(20259);
    locations.push_back(share);
#endif// HAS_FILESYSTEM_NFS

#ifdef HAS_UPNP
    if (CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_SERVICES_UPNP))
    {
      const std::string& strDevices =
          CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(33040); //"% Devices"
      share.strPath = "upnp://";
      share.strName = StringUtils::Format(strDevices, "UPnP"); //"UPnP Devices"
      locations.push_back(share);
    }
#endif

#ifdef HAS_ZEROCONF
    share.strPath = "zeroconf://";
    share.strName = CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(20262);
    locations.push_back(share);
#endif

    if (CServiceBroker::IsAddonInterfaceUp())
    {
      for (const auto& addon : CServiceBroker::GetVFSAddonCache().GetAddonInstances())
      {
        const auto& info = addon->GetProtocolInfo();
        if (!info.type.empty() && info.supportBrowsing)
        {
          share.strPath = info.type + "://";
          share.strName =
              CServiceBroker::GetResourcesComponent().GetLocalizeStrings().GetAddonString(
                  addon->ID(), info.label);
          if (share.strName.empty())
            share.strName =
                CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(info.label);
          locations.push_back(share);
        }
      }
    }
  }
}

bool CMediaManager::AddNetworkLocation(const std::string &path)
{
  CNetworkLocation location;
  location.path = path;
  location.id = (int)m_locations.size();
  m_locations.push_back(location);
  return SaveSources();
}

bool CMediaManager::HasLocation(const std::string& path) const
{
  for (unsigned int i=0;i<m_locations.size();++i)
  {
    if (URIUtils::CompareWithoutSlashAtEnd(m_locations[i].path, path))
      return true;
  }

  return false;
}


bool CMediaManager::RemoveLocation(const std::string& path)
{
  for (unsigned int i=0;i<m_locations.size();++i)
  {
    if (URIUtils::CompareWithoutSlashAtEnd(m_locations[i].path, path))
    {
      // prompt for sources, remove, cancel,
      m_locations.erase(m_locations.begin()+i);
      return SaveSources();
    }
  }

  return false;
}

bool CMediaManager::SetLocationPath(const std::string& oldPath, const std::string& newPath)
{
  for (unsigned int i=0;i<m_locations.size();++i)
  {
    if (URIUtils::CompareWithoutSlashAtEnd(m_locations[i].path, oldPath))
    {
      m_locations[i].path = newPath;
      return SaveSources();
    }
  }

  return false;
}

void CMediaManager::LoadAddonSources() const
{
  if (CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_bVirtualShares)
  {
    CMediaSourceSettings::GetInstance().AddShare("video", GetRootAddonTypeSource("video"));
    CMediaSourceSettings::GetInstance().AddShare("programs", GetRootAddonTypeSource("programs"));
    CMediaSourceSettings::GetInstance().AddShare("pictures", GetRootAddonTypeSource("pictures"));
    CMediaSourceSettings::GetInstance().AddShare("music", GetRootAddonTypeSource("music"));
    CMediaSourceSettings::GetInstance().AddShare("games", GetRootAddonTypeSource("games"));
  }
}

CMediaSource CMediaManager::GetRootAddonTypeSource(const std::string& type) const
{
  if (type == "programs" || type == "myprograms")
  {
    return ComputeRootAddonTypeSource(
        "executable", CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(1043),
        "DefaultAddonProgram.png");
  }
  else if (type == "video" || type == "videos")
  {
    return ComputeRootAddonTypeSource(
        "video", CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(1037),
        "DefaultAddonVideo.png");
  }
  else if (type == "music")
  {
    return ComputeRootAddonTypeSource(
        "audio", CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(1038),
        "DefaultAddonMusic.png");
  }
  else if (type == "pictures")
  {
    return ComputeRootAddonTypeSource(
        "image", CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(1039),
        "DefaultAddonPicture.png");
  }
  else if (type == "games")
  {
    return ComputeRootAddonTypeSource(
        "game", CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(35049),
        "DefaultAddonGame.png");
  }
  else
  {
    CLog::LogF(LOGERROR, "Invalid type {} provided", type);
    return {};
  }
}

CMediaSource CMediaManager::ComputeRootAddonTypeSource(const std::string& type,
                                                       const std::string& label,
                                                       const std::string& thumb) const
{
  CMediaSource source;
  source.strPath = "addons://sources/" + type + "/";
  source.strName = label;
  source.m_strThumbnailImage = thumb;
  source.m_iDriveType = SourceType::VPATH;
  source.m_ignore = true;
  return source;
}

void CMediaManager::AddAutoSource(const CMediaSource &share, bool bAutorun)
{
#ifdef HAS_OPTICAL_DRIVE
  if (!CServiceBroker::GetAppMessenger()->IsProcessThread())
  {
    RunOnAppThread([this, share, bAutorun]() { AddAutoSource(share, bAutorun); });
    return;
  }
#endif

  CMediaSourceSettings::GetInstance().AddShare("files", share);
  CMediaSourceSettings::GetInstance().AddShare("video", share);
  CMediaSourceSettings::GetInstance().AddShare("pictures", share);
  CMediaSourceSettings::GetInstance().AddShare("music", share);
  CMediaSourceSettings::GetInstance().AddShare("programs", share);
  CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UPDATE_SOURCES);
  CGUIComponent *gui = CServiceBroker::GetGUI();
  if (gui)
    gui->GetWindowManager().SendThreadMessage( msg );

#ifdef HAS_OPTICAL_DRIVE
  if(bAutorun)
    MEDIA_DETECT::CAutorun::ExecuteAutorun(share.strPath);
#endif
}

void CMediaManager::RemoveAutoSource(const CMediaSource &share)
{
#ifdef HAS_OPTICAL_DRIVE
  if (!CServiceBroker::GetAppMessenger()->IsProcessThread())
  {
    RunOnAppThread([this, share]() { RemoveAutoSource(share); });
    return;
  }
#endif

  DeleteAutoSource(share);

#ifdef HAS_OPTICAL_DRIVE
  // delete cached CdInfo if any
  RemoveCdInfo(TranslateDevicePath(share.strPath, true));
  RemoveDiscInfo(TranslateDevicePath(share.strPath, true));
#endif
}

void CMediaManager::DeleteAutoSource(const CMediaSource& share)
{
  CMediaSourceSettings::GetInstance().DeleteSource("files", share.strName, share.strPath, true);
  CMediaSourceSettings::GetInstance().DeleteSource("video", share.strName, share.strPath, true);
  CMediaSourceSettings::GetInstance().DeleteSource("pictures", share.strName, share.strPath, true);
  CMediaSourceSettings::GetInstance().DeleteSource("music", share.strName, share.strPath, true);
  CMediaSourceSettings::GetInstance().DeleteSource("programs", share.strName, share.strPath, true);
  CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UPDATE_SOURCES);
  CGUIComponent* gui = CServiceBroker::GetGUI();
  if (gui)
    gui->GetWindowManager().SendThreadMessage(msg);
}

/////////////////////////////////////////////////////////////
// AutoSource status functions:

std::string CMediaManager::TranslateDevicePath(const std::string& devicePath, bool bReturnAsDevice)
{
  std::unique_lock waitLock(m_muAutoSource);
  std::string strDevice = devicePath;
#ifdef HAS_OPTICAL_DRIVE
  // cdda://<device>/ names the drive, cdda://local/ means the first available one
  if (URIUtils::IsProtocol(devicePath, "cdda"))
    strDevice = CURL(devicePath).GetHostName();
  if (strDevice.empty() || strDevice == "local")
    strDevice = m_strFirstAvailDrive;
#endif

#ifdef TARGET_WINDOWS
  if (!m_bOpticalDrivePresent)
    return "";

  if(bReturnAsDevice == false)
    StringUtils::Replace(strDevice, "\\\\.\\","");
  else if(!strDevice.empty() && strDevice[1]==':')
    strDevice = StringUtils::Format("\\\\.\\{}:", strDevice[0]);

  URIUtils::RemoveSlashAtEnd(strDevice);

  // Drive letters are case insensitive on Windows, but the translated path is used as a map key
  // (drive state cache, cd info)
  const size_t colon{strDevice.find(':')};
  if (colon == 1 || (colon == 5 && StringUtils::StartsWith(strDevice, "\\\\.\\")))
    strDevice[colon - 1] =
        static_cast<char>(std::toupper(static_cast<unsigned char>(strDevice[colon - 1])));
#endif
  return strDevice;
}

bool CMediaManager::IsDiscInDrive(const std::string& devicePath)
{
#ifdef HAS_OPTICAL_DRIVE
#ifdef TARGET_WINDOWS
  return GetDriveStatus(devicePath) == DriveState::CLOSED_MEDIA_PRESENT;
#else
  if(URIUtils::IsDVD(devicePath) || devicePath.empty())
    return MEDIA_DETECT::CDetectDVDMedia::IsDiscInDrive();   //! @todo switch all ports to use auto sources
  else
    return true; // Assume other paths to be mounted already
#endif
#else
  return false;
#endif
}

bool CMediaManager::IsAudio(const std::string& devicePath, bool allowCachedFailure, bool polling)
{
#ifdef HAS_OPTICAL_DRIVE
#ifdef TARGET_WINDOWS
  if (!IsOpticalDrivePresent())
    return false;

  const std::shared_ptr<CCdInfo> pCdInfo{GetCdInfo(devicePath, allowCachedFailure, polling)};
  if (pCdInfo && pCdInfo->IsAudio(1))
    return true;

  return false;
#else
  //! @todo switch all ports to use auto sources
  const std::shared_ptr<MEDIA_DETECT::CCdInfo> pInfo{MEDIA_DETECT::CDetectDVDMedia::GetCdInfo()};
  if (pInfo && pInfo->IsAudio(1))
    return true;
#endif
#endif
  return false;
}

bool CMediaManager::HasOpticalDrive()
{
#ifdef HAS_OPTICAL_DRIVE
  if (!m_strFirstAvailDrive.empty())
    return true;
#endif
  return false;
}

DriveState CMediaManager::GetDriveStatus(const std::string& devicePath)
{
#ifdef HAS_OPTICAL_DRIVE
#ifdef TARGET_WINDOWS
  if (!IsOpticalDrivePresent() || !m_platformDiscDriveHander)
    return DriveState::NOT_READY;

  const std::string translatedDevicePath{TranslateDevicePath(devicePath, true)};

  // GUI labels query this every frame, and asking a drive that is spinning up blocks for
  // seconds, so this never waits. It answers with the last state seen and has a job refresh it
  // once that has expired. \sa RefreshDriveStatus
  bool refresh{false};
  DriveState state{DriveState::NOT_READY};
  {
    std::unique_lock lock(m_driveStatusSection);
    const auto it{m_driveStatusCache.find(translatedDevicePath)};
    if (it != m_driveStatusCache.end())
    {
      state = it->second.state;
      if (std::chrono::steady_clock::now() < it->second.expires)
        return state;
    }

    refresh = m_refreshingDrives.insert(translatedDevicePath).second;
  }

  if (refresh)
    CServiceBroker::GetJobManager()->Submit(
        [this, translatedDevicePath]()
        {
          // GetDriveStatusNow() may have asked the drive while this was queued
          {
            std::unique_lock lock(m_driveStatusSection);
            if (!m_refreshingDrives.contains(translatedDevicePath))
              return;
          }
          RefreshDriveStatus(translatedDevicePath);
        },
        CJob::PRIORITY_HIGH);

  return state;
#else
  return MEDIA_DETECT::CDetectDVDMedia::GetDriveState();
#endif
#else
  return DriveState::NOT_READY;
#endif
}

#if defined(TARGET_WINDOWS) && defined(HAS_OPTICAL_DRIVE)
DriveState CMediaManager::RefreshDriveStatus(const std::string& translatedDevicePath)
{
  uint64_t generation{0};
  {
    std::unique_lock lock(m_driveStatusSection);
    generation = m_driveStatusGeneration;
  }

  // Deliberately queried without holding m_driveStatusSection - this can block for seconds on a
  // drive that is spinning up, and no other caller should have to wait behind it.
  const DriveState state{m_platformDiscDriveHander->GetDriveState(translatedDevicePath)};

  std::unique_lock lock(m_driveStatusSection);
  m_refreshingDrives.erase(translatedDevicePath);

  // A probe invalidated by a storage or tray event while it was in flight describes the drive
  // before that event, so nothing is kept from it
  if (generation != m_driveStatusGeneration)
    return state;

  // Failed probes and no-media answers may recover without an event. Throttle retries,
  // including when an empty drive cannot be distinguished from a disc spinning up.
  const auto expires{std::chrono::steady_clock::now() +
                     (state == DriveState::NOT_READY || state == DriveState::CLOSED_NO_MEDIA
                          ? DRIVE_STATUS_RETRY_INTERVAL
                          : DRIVE_STATUS_REFRESH_INTERVAL)};
  m_driveStatusCache.insert_or_assign(translatedDevicePath, DriveStatusCacheEntry{state, expires});

  const auto logged{m_driveStatusLogged.find(translatedDevicePath)};
  const bool changed{logged == m_driveStatusLogged.end() || logged->second != state};

  // The drive returns NOT_READY while its tray moves (neither a disc-present nor an empty state)
  bool discGone{false};
  if (state == DriveState::CLOSED_MEDIA_PRESENT)
    m_discSeen.insert(translatedDevicePath);
  else if (state == DriveState::OPEN || state == DriveState::CLOSED_NO_MEDIA)
    discGone = m_discSeen.erase(translatedDevicePath) != 0;
  if (changed)
  {
    m_driveStatusLogged.insert_or_assign(translatedDevicePath, state);
    CLog::LogF(LOGDEBUG, "Drive {} state is now {}", translatedDevicePath, DriveStateName(state));
  }
  lock.unlock();

  if (discGone)
    RunOnAppThread([this, translatedDevicePath]() { BumpDiscGeneration(translatedDevicePath); });

  return state;
}

DriveState CMediaManager::GetDriveStatusNow(const std::string& devicePath)
{
  if (!IsOpticalDrivePresent() || !m_platformDiscDriveHander)
    return DriveState::NOT_READY;

  const std::string translatedDevicePath{TranslateDevicePath(devicePath, true)};
  {
    std::unique_lock lock(m_driveStatusSection);
    m_refreshingDrives.insert(translatedDevicePath);
  }

  return RefreshDriveStatus(translatedDevicePath);
}
#endif

#ifdef HAS_OPTICAL_DRIVE
uint64_t CMediaManager::BumpDiscGeneration(const std::string& devicePath)
{
  const std::string strDevice{TranslateDevicePath(devicePath, false)};

  std::unique_lock lock(m_muAutoSource);
  return ++m_discGeneration[strDevice];
}

uint64_t CMediaManager::DiscGeneration(const std::string& devicePath)
{
  const std::string strDevice{TranslateDevicePath(devicePath, false)};

  std::unique_lock lock(m_muAutoSource);
  return DiscGenerationLocked(strDevice);
}

uint64_t CMediaManager::DiscGenerationLocked(const std::string& translatedDevicePath) const
{
  const auto it{m_discGeneration.find(translatedDevicePath)};
  return it != m_discGeneration.end() ? it->second : 0;
}

bool CMediaManager::IsDiscCurrent(const std::string& devicePath, uint64_t generation)
{
  return DiscGeneration(devicePath) == generation;
}
#endif

bool CMediaManager::IsOpticalDrivePresent()
{
  std::unique_lock waitLock(m_muAutoSource);
  return m_bOpticalDrivePresent;
}

void CMediaManager::ResetDriveCaches(const std::string& devicePath)
{
#if defined(TARGET_WINDOWS) && defined(HAS_OPTICAL_DRIVE)
  // The drive state is keyed by the device (eg. "\\.\D:"), the disc identity by the drive ("D:")
  const std::string translatedDevicePath{
      devicePath.empty() ? std::string{} : TranslateDevicePath(devicePath, true)};
  const std::string drivePath{devicePath.empty() ? std::string{}
                                                 : TranslateDevicePath(devicePath, false)};

  // Each cache has its own lock, and they are never nested
  {
    std::unique_lock lock(m_driveStatusSection);
    ++m_driveStatusGeneration;
    // Expired rather than dropped, so the last answer stands while a job asks the drive again
    const auto now{std::chrono::steady_clock::now()};
    if (translatedDevicePath.empty())
    {
      for (auto& entry : m_driveStatusCache)
        entry.second.expires = now;
    }
    else
    {
      const auto it{m_driveStatusCache.find(translatedDevicePath)};
      if (it != m_driveStatusCache.end())
        it->second.expires = now;
    }
  }

  {
    std::unique_lock lock(m_discInfoSection);
    ++m_discInfoGeneration;
    if (drivePath.empty())
      m_mapDiscInfo.clear();
    else
      m_mapDiscInfo.erase(drivePath);
  }

  // Dropping a TOC a caller is still reading is safe - it holds a share of it
  {
    std::unique_lock waitLock(m_muAutoSource);
    ++m_cdInfoGeneration;
    if (drivePath.empty())
    {
      m_mapCdInfo.clear();
      m_cdInfoUnavailable.clear();
    }
    else
    {
      m_mapCdInfo.erase(drivePath);
      m_cdInfoUnavailable.erase(drivePath);
    }
  }
#endif
}

#ifdef HAS_OPTICAL_DRIVE
std::shared_ptr<CCdInfo> CMediaManager::GetCdInfo(const std::string& devicePath,
                                                  bool allowCachedFailure,
                                                  bool polling)
{
#ifdef TARGET_WINDOWS
  if (!IsOpticalDrivePresent())
    return {};

  const std::string strDevice{TranslateDevicePath(devicePath, false)};

  // A read invalidated by a storage or tray event while in flight describes the previous disc,
  // so it is discarded and the disc that is there now is read once more
  for (int attempt = 0; attempt < 2; ++attempt)
  {
    uint64_t generation{0};
    {
      std::unique_lock waitLock(m_muAutoSource);
      const auto it{m_mapCdInfo.find(strDevice)};
      if (it != m_mapCdInfo.end())
        return it->second;

      // Only polling callers reuse failures; classification and playback must be able to retry.
      const auto unavailable{m_cdInfoUnavailable.find(strDevice)};
      if (allowCachedFailure && unavailable != m_cdInfoUnavailable.end() &&
          std::chrono::steady_clock::now() < unavailable->second)
        return {};

      if (polling)
      {
        if (m_cdInfoFilling.insert(strDevice).second)
          CServiceBroker::GetJobManager()->Submit(
              [this, strDevice]()
              {
                GetCdInfo(strDevice);
                std::unique_lock lock(m_muAutoSource);
                m_cdInfoFilling.erase(strDevice);
              },
              CJob::PRIORITY_HIGH);
        return {};
      }
      generation = m_cdInfoGeneration;
    }

    // Reading the TOC is slow, so this is done without holding m_muAutoSource
    std::shared_ptr<CCdInfo> pCdInfo{CacheCdInfo(strDevice, ReadToc(strDevice), generation)};
    if (pCdInfo)
      return pCdInfo;

    std::unique_lock waitLock(m_muAutoSource);
    if (generation == m_cdInfoGeneration)
      return {}; // The disc really could not be read
  }
  return {};
#else
  return MEDIA_DETECT::CDetectDVDMedia::GetCdInfo();
#endif
}

#ifdef TARGET_WINDOWS
std::unique_ptr<CCdInfo> CMediaManager::ReadToc(const std::string& devicePath)
{
  if (m_readToc)
    return m_readToc(devicePath);

  CCdIoSupport cdio;
  return std::unique_ptr<CCdInfo>{cdio.GetCdInfo(const_cast<char*>(devicePath.c_str()))};
}

std::shared_ptr<CCdInfo> CMediaManager::CacheCdInfo(const std::string& devicePath,
                                                    std::unique_ptr<CCdInfo> info,
                                                    uint64_t generation)
{
  std::unique_lock waitLock(m_muAutoSource);
  if (generation != m_cdInfoGeneration)
  {
    CLog::LogF(LOGDEBUG, "Discarding invalidated TOC read for {}", devicePath);
    return {};
  }

  const auto cached{m_mapCdInfo.find(devicePath)};
  if (cached != m_mapCdInfo.end())
    return cached->second;

  if (info)
  {
    const auto it{m_mapCdInfo.try_emplace(devicePath, std::move(info)).first};
    m_cdInfoUnavailable.erase(devicePath);
    return it->second;
  }

  m_cdInfoUnavailable.insert_or_assign(devicePath,
                                       std::chrono::steady_clock::now() + CDINFO_RETRY_INTERVAL);
  return {};
}
#endif

bool CMediaManager::RemoveCdInfo(const std::string& devicePath)
{
  if (!IsOpticalDrivePresent())
    return false;

  std::string strDevice = TranslateDevicePath(devicePath, false);

  std::unique_lock waitLock(m_muAutoSource);
#ifdef TARGET_WINDOWS
  ++m_cdInfoGeneration;
  m_cdInfoUnavailable.erase(strDevice);
#endif
  const auto it = m_mapCdInfo.find(strDevice);
  if (it != m_mapCdInfo.end())
  {
    // Any caller still holding this keeps it alive until it is done with it
    m_mapCdInfo.erase(it);
    return true;
  }
  return false;
}

CMediaManager::DiscInfoCacheEntry CMediaManager::GetCachedDiscInfo(const std::string& mediaPath,
                                                                   bool polling)
{
#if defined(TARGET_WINDOWS) && defined(HAS_OPTICAL_DRIVE)
  uint64_t generation{0};
  {
    std::unique_lock lock(m_discInfoSection);
    const auto cached{m_mapDiscInfo.find(mediaPath)};
    if (cached != m_mapDiscInfo.end() && std::chrono::steady_clock::now() < cached->second.expires)
      return cached->second;

    if (polling)
    {
      if (m_discInfoFilling.insert(mediaPath).second)
        CServiceBroker::GetJobManager()->Submit(
            [this, mediaPath]()
            {
              GetCachedDiscInfo(mediaPath);
              std::unique_lock lock(m_discInfoSection);
              m_discInfoFilling.erase(mediaPath);
            },
            CJob::PRIORITY_HIGH);
      return cached != m_mapDiscInfo.end() ? cached->second : DiscInfoCacheEntry{};
    }
    generation = m_discInfoGeneration;
  }
#endif

  // Reading the disc is slow, so this is done without holding m_discInfoSection
  DiscInfoCacheEntry entry;
  entry.info = GetDiscInfo(mediaPath);
  entry.label = entry.info.name;

#if defined(TARGET_WINDOWS) && !defined(TARGET_WINDOWS_STORE)
  if (entry.label.empty())
  {
    // Nothing identified the disc, fall back to the volume label. Deliberately kept out of
    // entry.info, which has to stay exactly what the disc said - GetDiskUniqueId() reads an
    // empty DiscInfo as "this disc cannot be identified"
    std::string volumeRoot{mediaPath};
    URIUtils::AddSlashAtEnd(volumeRoot);
    std::wstring volumeRootW;
    g_charsetConverter.utf8ToW(volumeRoot, volumeRootW);
    WCHAR cVolumeName[128];
    WCHAR cFSName[128];
    if (GetVolumeInformationW(volumeRootW.c_str(), cVolumeName, 127, NULL, NULL, NULL, cFSName,
                              127) != 0)
    {
      std::string volumeName;
      g_charsetConverter.wToUTF8(cVolumeName, volumeName);
      entry.label = StringUtils::TrimRight(volumeName, " ");
    }
  }
#endif

#if defined(TARGET_WINDOWS) && defined(HAS_OPTICAL_DRIVE)
  CacheDiscInfo(mediaPath, entry, generation);
#endif

  return entry;
}

#ifdef TARGET_WINDOWS
void CMediaManager::CacheDiscInfo(const std::string& mediaPath,
                                  DiscInfoCacheEntry& entry,
                                  uint64_t generation)
{
  std::unique_lock lock(m_discInfoSection);
  if (generation == m_discInfoGeneration)
  {
    // A volume label does not prove that disc identification succeeded
    if (entry.label.empty())
      entry.expires = std::chrono::steady_clock::now() + MOUNTING_DISC_RETRY_INTERVAL;
    else if (entry.info.empty())
      entry.expires = std::chrono::steady_clock::now() + UNIDENTIFIED_DISC_RETRY_INTERVAL;
    m_mapDiscInfo.insert_or_assign(mediaPath, entry);
  }
}
#endif

std::string CMediaManager::GetDiskLabel(const std::string& devicePath, bool polling)
{
#ifdef TARGET_WINDOWS_STORE
  return ""; // GetVolumeInformationW nut support in UWP app
#elif defined(TARGET_WINDOWS)
  if (!IsOpticalDrivePresent())
    return "";

  const std::string mediaPath{TranslateDevicePath(devicePath)};

  // try to minimize the chance of a "device not ready" dialog
  if (GetDriveStatus(mediaPath) != DriveState::CLOSED_MEDIA_PRESENT)
    return "";

  return GetCachedDiscInfo(mediaPath, polling).label;
#else
  return MEDIA_DETECT::CDetectDVDMedia::GetDVDLabel();
#endif
}

std::string CMediaManager::GetDiskUniqueId(const std::string& devicePath)
{
  std::string mediaPath;

  const std::shared_ptr<CCdInfo> pInfo{CServiceBroker::GetMediaManager().GetCdInfo(devicePath)};
  if (!pInfo)
    return "";

  if (pInfo->IsAudio(1))
    mediaPath = "cdda://local/";

  if (mediaPath.empty() && (pInfo->IsISOUDF(1) || pInfo->IsISOHFS(1) || pInfo->IsIso9660(1) || pInfo->IsIso9660Interactive(1)))
    mediaPath = "iso9660://";

  if (mediaPath.empty())
    mediaPath = devicePath;

#ifdef TARGET_WINDOWS
  if (mediaPath.empty() || mediaPath == "iso9660://")
  {
    mediaPath = CServiceBroker::GetMediaManager().TranslateDevicePath(devicePath);
  }
#endif

  // Shares the read with GetDiskLabel(), which AddOpticalSource() calls immediately before this
  UTILS::DISCS::DiscInfo info{GetCachedDiscInfo(mediaPath).info};
  if (info.empty())
  {
    CLog::Log(LOGDEBUG, "GetDiskUniqueId: Retrieving ID for path {} failed, ID is empty.",
              CURL::GetRedacted(mediaPath));
    return "";
  }

  std::string strID{StringUtils::Format("removable://{}_{}", info.name, info.serial)};
  if (info.type == UTILS::DISCS::DiscType::BLURAY)
  {
    CURL url("bluray://");
    url.SetHostName(strID);
    url.SetFileName(URIUtils::AddFileToFolder("BDMV", "index.bdmv"));
    strID = url.Get();
  }
  CLog::Log(LOGDEBUG, "GetDiskUniqueId: Got ID {} for disc with path {}", strID,
            CURL::GetRedacted(mediaPath));

  return strID;
}

bool CMediaManager::HasMediaBlurayPlaylist(const std::string& devicePath)
{
#ifdef HAVE_LIBBLURAY
  // When the disc node is displayed, this gets called by the GUI via SYSTEM_MEDIA_BLURAY_PLAYLIST
  // in CSystemCGUIInfo at every refresh - so cache result until eject.
  if (m_hasBlurayPlaylist != HasBlurayPlaylist::UNKNOWN)
    return m_hasBlurayPlaylist == HasBlurayPlaylist::YES;

  const std::string mediaPath{TranslateDevicePath(devicePath)};
  UTILS::DISCS::DiscInfo info{GetCachedDiscInfo(mediaPath, true).info};
#ifdef TARGET_WINDOWS
  // Let the timed identification cache retry before remembering that no playlist exists.
  if (info.empty())
    return false;
#endif
  if (!info.empty() && info.type == UTILS::DISCS::DiscType::BLURAY)
  {
#ifdef TARGET_WINDOWS
    // GetDiskUniqueId() reads the TOC, so leave that to a job and ask again once it is cached
    if (!GetCdInfo("", true, true))
      return false;
#endif
    const std::string blurayPath{GetDiskUniqueId()};
    CVideoDatabase db;
    if (db.Open())
    {
      const std::string path{db.GetRemovableBlurayPath(blurayPath)};
      db.Close();
      m_hasBlurayPlaylist = path.empty() ? HasBlurayPlaylist::NO : HasBlurayPlaylist::YES;
      return !path.empty();
    }
  }
  m_hasBlurayPlaylist = HasBlurayPlaylist::NO;
#endif
  return false;
}

void CMediaManager::ResetBlurayPlaylistStatus()
{
#ifdef HAVE_LIBBLURAY
  m_hasBlurayPlaylist = HasBlurayPlaylist::UNKNOWN;
#endif
}

std::string CMediaManager::GetDiscPath()
{
#ifdef TARGET_WINDOWS
  return CServiceBroker::GetMediaManager().TranslateDevicePath("");
#else

  std::unique_lock lock(m_CritSecStorageProvider);
  std::vector<CMediaSource> drives;
  m_platformStorage->GetRemovableDrives(drives);
  for(unsigned i = 0; i < drives.size(); ++i)
  {
    if (drives[i].m_iDriveType == SourceType::OPTICAL_DISC && !drives[i].strPath.empty())
      return drives[i].strPath;
  }

  // iso9660://, cdda://local/ or D:\ depending on disc type
  return MEDIA_DETECT::CDetectDVDMedia::GetDVDPath();
#endif
}

std::shared_ptr<IDiscDriveHandler> CMediaManager::GetDiscDriveHandler()
{
  return m_platformDiscDriveHander;
}
#endif

void CMediaManager::SetHasOpticalDrive(bool bstatus)
{
  bool changed{false};
  {
    std::unique_lock waitLock(m_muAutoSource);
    changed = m_bOpticalDrivePresent != bstatus;
    m_bOpticalDrivePresent = bstatus;
  }
  if (changed)
    ResetDriveCaches();
}

bool CMediaManager::Eject(const std::string& mountpath)
{
  std::unique_lock lock(m_CritSecStorageProvider);
#ifdef HAVE_LIBBLURAY
  m_hasBlurayPlaylist = HasBlurayPlaylist::UNKNOWN;
#endif
  const bool ejected{m_platformStorage->Eject(mountpath)};
#ifdef HAS_OPTICAL_DRIVE
  if (ejected)
    BumpDiscGeneration(mountpath);
#endif
  ResetDriveCaches(mountpath);
  return ejected;
}

void CMediaManager::EjectTray(const bool bEject, const std::string& devicePath)
{
#ifdef HAS_OPTICAL_DRIVE
  OperateTray(
      devicePath,
      [](IDiscDriveHandler& handler, const std::string& path, bool eject)
      {
        if (eject)
          return handler.EjectDriveTray(path);
        return handler.CloseDriveTray(path);
      },
      bEject);
#endif
}

void CMediaManager::CloseTray(const std::string& devicePath)
{
#ifdef HAS_OPTICAL_DRIVE
  OperateTray(devicePath, [](IDiscDriveHandler& handler, const std::string& path, bool)
              { return handler.CloseDriveTray(path); });
#endif
}

void CMediaManager::ToggleTray(const std::string& devicePath)
{
#ifdef HAS_OPTICAL_DRIVE
  OperateTray(devicePath, [](IDiscDriveHandler& handler, const std::string& path, bool)
              { return handler.ToggleDriveTray(path); });
#endif
}

#ifndef TARGET_WINDOWS
std::set<std::string> CMediaManager::GetRemovableDrivePaths() const
{
  std::vector<CMediaSource> drives;
  m_platformStorage->GetRemovableDrives(drives);
  std::set<std::string> paths;
  for (const auto& drive : drives)
  {
    paths.insert(drive.strPath);
    if (!drive.strDevicePath.empty())
      paths.insert(drive.strDevicePath);
  }
  return paths;
}
#endif

#ifdef HAS_OPTICAL_DRIVE
void CMediaManager::OperateTray(
    const std::string& devicePath,
    const std::function<bool(IDiscDriveHandler&, const std::string&, bool)>& operation,
    bool eject)
{
  if (!m_platformDiscDriveHander)
    return;

  // Working a tray asks the drive what state it is in and then tells it to move. Both wait on
  // the hardware, and a drive that is loading media does not answer for tens of seconds.
  const std::string trayDevicePath{TranslateDevicePath(devicePath)};
  const std::shared_ptr<IDiscDriveHandler> handler{m_platformDiscDriveHander};

  // One at a time per drive. A second request while the tray is moving would see the state
  // it was in before the first, and move it the same way instead of back
  {
    std::unique_lock lock(m_muAutoSource);
    if (!m_trayBusy.insert(trayDevicePath).second)
      return;
  }

  CServiceBroker::GetJobManager()->Submit(
      [this, handler, trayDevicePath, operation, eject]()
      {
        // Only when the drive actually moved - a rejected eject leaves the disc where it was
        if (operation(*handler, trayDevicePath, eject))
        {
          // Bumped on the application thread, so it cannot land between a job's generation check
          // and the work that check guards
          RunOnAppThread(
              [this, trayDevicePath]()
              {
                BumpDiscGeneration(trayDevicePath);
                ResetBlurayPlaylistStatus();
              });
          ResetDriveCaches(trayDevicePath);
        }

        std::unique_lock lock(m_muAutoSource);
        m_trayBusy.erase(trayDevicePath);
      },
      CJob::PRIORITY_HIGH);
}
#endif

void CMediaManager::ProcessEvents()
{
  std::unique_lock lock(m_CritSecStorageProvider);
  if (m_platformStorage->PumpDriveChangeEvents(this))
  {
#ifndef TARGET_WINDOWS
    // Windows learns which drive changed through the storage callbacks and forgets its disc
    // there
    // A disc the OS has not mounted is never listed, but is still probed by its device node
    if (const std::string opticalDevice{m_platformStorage->GetFirstOpticalDeviceFileName()};
        !opticalDevice.empty())
      RemoveDiscInfo(opticalDevice);

    std::set<std::string> current{GetRemovableDrivePaths()};
    for (const auto& path : m_removableDrivePaths)
      RemoveDiscInfo(path);
    for (const auto& path : current)
      RemoveDiscInfo(path);
    m_removableDrivePaths = std::move(current);
#endif

#if defined(HAS_OPTICAL_DRIVE)
#if defined(TARGET_DARWIN_OSX)
    // darwins GetFirstOpticalDeviceFileName only gives us something
    // when a disc is inserted
    // so we have to refresh m_strFirstAvailDrive when this happens after Initialize
    // was called (e.x. the disc was inserted after the start of xbmc)
    // else TranslateDevicePath wouldn't give the correct device
    {
      const std::string firstDrive{m_platformStorage->GetFirstOpticalDeviceFileName()};
      std::unique_lock waitLock(m_muAutoSource);
      m_strFirstAvailDrive = firstDrive;
    }
#elif defined(TARGET_WINDOWS)
    // On Windows, virtual drives can appear or disappear at any time.
    // Re-scan to get the current state of optical drives and update
    // m_bhasoptical and m_strFirstAvailDrive accordingly.
    const std::string firstDrive{m_platformStorage->GetFirstOpticalDeviceFileName()};
    {
      std::unique_lock waitLock(m_muAutoSource);
      m_strFirstAvailDrive = firstDrive;
    }
    SetHasOpticalDrive(!firstDrive.empty());

    // A device change reported only as DBT_DEVTYP_DEVICEINTERFACE gives us no drive letter, so
    // this rescan is the sole invalidation point for it. SetHasOpticalDrive() only clears the
    // cache when the overall presence changes
    ResetDriveCaches();

#if !defined(TARGET_WINDOWS_STORE)
    // Nor does it say which drive went, so stop any job still reading a drive that is no longer
    // there. A drive that is still there keeps its generation, and with it any job an arrival
    // above has just queued
    {
      std::unique_lock waitLock(m_muAutoSource);
      for (auto& [drive, generation] : m_discGeneration)
      {
        std::wstring driveRootW;
        g_charsetConverter.utf8ToW(drive + "\\", driveRootW);
        if (GetDriveTypeW(driveRootW.c_str()) != DRIVE_CDROM)
          ++generation;
      }
    }
#endif
#endif
#endif

    CGUIMessage msg(GUI_MSG_NOTIFY_ALL,0,0,GUI_MSG_UPDATE_SOURCES);
    CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg);
  }
}

std::vector<std::string> CMediaManager::GetDiskUsage()
{
  std::unique_lock lock(m_CritSecStorageProvider);
  return m_platformStorage->GetDiskUsage();
}

#if defined(TARGET_WINDOWS) && defined(HAS_OPTICAL_DRIVE)
void CMediaManager::AddOpticalSource(const std::string& devicePath, uint64_t generation)
{
  CMediaSource share;
  share.strPath = devicePath;
  share.strDevicePath = devicePath;
  share.strName = devicePath;

  // Cleared here, before the disc is read, so what the reads store is what stays cached. Only
  // the sources need the application thread, guarded like the add below so a job for a disc
  // that has since been swapped cannot remove the source the current disc has published
  RemoveCdInfo(TranslateDevicePath(devicePath, true));
  RemoveDiscInfo(TranslateDevicePath(devicePath, true));
  RunOnAppThread(
      [this, share, devicePath, generation]()
      {
        if (IsDiscCurrent(devicePath, generation))
          DeleteAutoSource(share);
      });

  share.strStatus = GetDiskLabel(share.strPath);
  share.strDiskUniqueId = GetDiskUniqueId(share.strPath);
  // The TOC was just read for the unique ID, so a failure there need not be repeated here
  if (IsAudio(share.strPath, true))
    share.strStatus = "Audio-CD";
  else if (share.strStatus.empty())
    share.strStatus = CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(446);

  share.m_ignore = true;
  share.m_iDriveType = SourceType::OPTICAL_DISC;

  // Reading the disc above waited on it, so only add what still describes what is in the drive
  RunOnAppThread(
      [this, share, devicePath, generation]()
      {
        if (IsDiscCurrent(devicePath, generation))
          AddAutoSource(share, false);
      });
}
#endif

void CMediaManager::OnStorageAdded(const MEDIA_DETECT::STORAGE::StorageDevice& device)
{
  ResetDriveCaches(device.path);
#ifdef HAS_OPTICAL_DRIVE
  if (device.type == MEDIA_DETECT::STORAGE::Type::OPTICAL)
  {
#ifdef TARGET_WINDOWS
    SetHasOpticalDrive(true); // In case drive appeared after startup (eg. virtual drive)
    {
      std::unique_lock waitLock(m_muAutoSource);
      if (m_strFirstAvailDrive.empty())
        m_strFirstAvailDrive = device.path;
    }
#endif

    // Identifying the disc reads it, and on an AACS Blu-ray loading libaacs alone takes well
    // over a second. This is called from CApplication::ProcessSlow(), so doing it here freezes
    // the GUI for as long as the read takes - hand it to a job instead.
    const uint64_t generation{BumpDiscGeneration(device.path)};
    // A disc swapped using the drive's own button never passes through Eject()
    ResetBlurayPlaylistStatus();
    CServiceBroker::GetJobManager()->Submit([this, device, generation]()
                                            { ProcessAddedOpticalDevice(device, generation); },
                                            CJob::PRIORITY_HIGH);
  }
  else
#endif
  {
    // Non-optical (or no optical support) - eg. USB stick
    CGUIDialogKaiToast::QueueNotification(
        CGUIDialogKaiToast::Info,
        CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(13021), device.label,
        TOAST_DISPLAY_TIME, false);
  }
}

#ifdef HAS_OPTICAL_DRIVE
void CMediaManager::ProcessAddedOpticalDevice(const MEDIA_DETECT::STORAGE::StorageDevice& device,
                                              uint64_t generation)
{
#ifdef TARGET_WINDOWS
  // Windows reports a tray closing on nothing as an arrival, and a busy drive answers neither
  // yes nor no, so wait for an answer rather than announce a disc on the strength of the event
  // alone. On a job, so it can wait.
  DriveState state{GetDriveStatusNow(device.path)};
  for (std::chrono::milliseconds waited{0};
       state == DriveState::NOT_READY && waited < DRIVE_SETTLE_TIMEOUT;
       waited += DRIVE_SETTLE_INTERVAL)
  {
    std::this_thread::sleep_for(DRIVE_SETTLE_INTERVAL);
    state = GetDriveStatusNow(device.path);
  }

  // Asking the drive waited on it, long enough for the disc to have been taken out again
  if (!IsDiscCurrent(device.path, generation))
    return;

  // A drive that never says it holds a disc has none, or is no longer there at all
  if (state != DriveState::CLOSED_MEDIA_PRESENT)
    return;

  AddOpticalSource(device.path, generation);
#endif

  // Source creation clears the previous TOC; classification retries any failed read.
  const std::shared_ptr<CCdInfo> pInfo{GetCdInfo(device.path)};

  // Reading the table of contents waits on the drive as well
  if (!IsDiscCurrent(device.path, generation))
    return;

  // No TOC is likely to mean a protected video disc
  const bool isAudioDisc{pInfo && pInfo->IsAudio(1)};

  const std::shared_ptr<CSettings> settings{CServiceBroker::GetSettingsComponent()->GetSettings()};

  if (isAudioDisc)
  {
    const auto cdAutoAction{
        static_cast<AutoCDAction>(settings->GetInt(CSettings::SETTING_AUDIOCDS_AUTOACTION))};
    CLog::LogF(LOGDEBUG, "Audio CD detected with path {}, auto action is {}", device.path,
               static_cast<int>(cdAutoAction));

    using enum AutoCDAction;
    if (AUTORUN_FROM_STORAGE_EVENT && (cdAutoAction == RIP || cdAutoAction == PLAY))
    {
      // Will fallback to play if HAS_CDDA_RIPPER not defined
      ExecuteAutorunOnAppThread(device.path, device.label, true, pInfo != nullptr, generation);
    }
    else if (cdAutoAction == NONE)
    {
      CGUIDialogKaiToast::QueueNotification(
          CGUIDialogKaiToast::Info,
          CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(13019), device.label,
          TOAST_DISPLAY_TIME, false);
    }
  }
  else
  {
    const auto dvdAutoAction{
        static_cast<AutoDVDAction>(settings->GetInt(CSettings::SETTING_DVDS_AUTOACTION))};
    CLog::LogF(LOGDEBUG, "Video disc detected with path {}, auto action is {}", device.path,
               static_cast<int>(dvdAutoAction));

    using enum AutoDVDAction;
    if (dvdAutoAction == BROWSE)
    {
      CServiceBroker::GetJobManager()->AddJob(
          new CAutorunMediaJob(device.label, device.path, generation), this, CJob::PRIORITY_HIGH);
    }
    else if (AUTORUN_FROM_STORAGE_EVENT && dvdAutoAction == PLAY)
    {
      ExecuteAutorunOnAppThread(device.path, device.label, false, pInfo != nullptr, generation);
    }
    else if (dvdAutoAction == NONE)
    {
      CGUIDialogKaiToast::QueueNotification(
          CGUIDialogKaiToast::Info,
          CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(13019), device.label,
          TOAST_DISPLAY_TIME, false);
    }
  }
}
#endif

void CMediaManager::OnStorageSafelyRemoved(const MEDIA_DETECT::STORAGE::StorageDevice& device)
{
  ResetDriveCaches(device.path);
#ifdef HAS_OPTICAL_DRIVE
  // Every platform, so that a job still reading this disc knows to stop
  if (device.type == MEDIA_DETECT::STORAGE::Type::OPTICAL)
    BumpDiscGeneration(device.path);
#endif
#ifdef TARGET_WINDOWS
  if (device.type == MEDIA_DETECT::STORAGE::Type::OPTICAL)
  {
    CMediaSource share;
    share.strPath = device.path;
    share.strName = device.path;
    RemoveAutoSource(share);
  }
#endif
  CGUIDialogKaiToast::QueueNotification(
      CGUIDialogKaiToast::Info,
      CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(13023), device.label,
      TOAST_DISPLAY_TIME, false);
}

void CMediaManager::OnStorageUnsafelyRemoved(const MEDIA_DETECT::STORAGE::StorageDevice& device)
{
  ResetDriveCaches(device.path);
#ifdef HAS_OPTICAL_DRIVE
  if (device.type == MEDIA_DETECT::STORAGE::Type::OPTICAL)
    BumpDiscGeneration(device.path);
#endif
#ifdef TARGET_WINDOWS
  if (device.type == MEDIA_DETECT::STORAGE::Type::OPTICAL)
  {
    CMediaSource share;
    share.strPath = device.path;
    share.strName = device.path;
    RemoveAutoSource(share);
  }
#endif
  CGUIDialogKaiToast::QueueNotification(
      CGUIDialogKaiToast::Warning,
      CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(13022), device.label);
}

UTILS::DISCS::DiscInfo CMediaManager::GetDiscInfo(const std::string& mediaPath)
{
  UTILS::DISCS::DiscInfo info;

  if (mediaPath.empty())
    return info;

  // Try finding VIDEO_TS/VIDEO_TS.IFO - this indicates a DVD disc is inserted
  std::string pathVideoTS = URIUtils::AddFileToFolder(mediaPath, "VIDEO_TS", "VIDEO_TS.IFO");
  // correct the filename if needed
  if (StringUtils::StartsWith(mediaPath, "dvd://") ||
      StringUtils::StartsWith(mediaPath, "iso9660://"))
  {
    pathVideoTS = TranslateDevicePath("");
  }

  // check for DVD discs
  if (CFileUtils::Exists(pathVideoTS))
  {
    info = UTILS::DISCS::ProbeDVDDiscInfo(pathVideoTS);
    if (!info.empty())
      return info;
  }
#ifdef HAVE_LIBBLURAY
  // check for Blu-ray discs
  if (CFileUtils::Exists(URIUtils::AddFileToFolder(mediaPath, "BDMV", "index.bdmv")))
  {
    info = XFILE::CBlurayDirectory::ProbeDisc(mediaPath);
  }
#endif

  return info;
}

void CMediaManager::RemoveDiscInfo(const std::string& devicePath)
{
  const std::string strDevice{TranslateDevicePath(devicePath, false)};

  {
    std::unique_lock lock(m_discInfoSection);
    ++m_discInfoGeneration;
    m_mapDiscInfo.erase(strDevice);
  }

#ifdef HAVE_LIBBLURAY
  CServiceBroker::GetBlurayDiscCache()->ClearDisc(strDevice);
#endif
}

bool CMediaManager::playStubFile(const CFileItem& item)
{
  // Figure out Lines 1 and 2 of the dialog
  std::string strLine1, strLine2;

  // use generic message by default
  strLine1 = CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(435).c_str();
  strLine2 = CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(436).c_str();

  CXBMCTinyXML2 discStubXML;
  if (discStubXML.LoadFile(item.GetPath()))
  {
    auto* pRootElement = discStubXML.RootElement();
    if (!pRootElement || StringUtils::CompareNoCase(pRootElement->Value(), "discstub") != 0)
      CLog::Log(LOGINFO, "No <discstub> node found for {}. Using default info dialog message",
                item.GetPath());
    else
    {
      XMLUtils::GetString(pRootElement, "title", strLine1);
      XMLUtils::GetString(pRootElement, "message", strLine2);
      // no title? use the label of the CFileItem as line 1
      if (strLine1.empty())
        strLine1 = item.GetLabel();
    }
  }

  if (HasOpticalDrive())
  {
#ifdef HAS_OPTICAL_DRIVE
    if (CGUIDialogPlayEject::ShowAndGetInput(strLine1, strLine2))
      return MEDIA_DETECT::CAutorun::PlayDiscAskResume();
#endif
  }
  else
  {
    KODI::MESSAGING::HELPERS::ShowOKDialogText(strLine1, strLine2);
  }
  return true;
}
