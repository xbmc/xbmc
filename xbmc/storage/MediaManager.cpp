/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "MediaManager.h"

#include "FileItem.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "guilib/GUIComponent.h"
#include "guilib/LocalizeStrings.h"
#include "utils/URIUtils.h"

#include <mutex>
#ifdef TARGET_WINDOWS
#include "platform/win32/WIN32Util.h"
#include "utils/CharsetConverter.h"
#endif
#include "guilib/GUIWindowManager.h"
#ifdef HAS_OPTICAL_DRIVE
#ifndef TARGET_WINDOWS
//! @todo switch all ports to use auto sources
#include <map>
#include <utility>
#include "DetectDVDType.h"
#endif
#endif
#include "Autorun.h"
#include "AutorunMediaJob.h"
#include "GUIUserMessages.h"
#include "addons/VFSEntry.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "dialogs/GUIDialogPlayEject.h"
#include "messaging/helpers/DialogOKHelper.h"
#include "settings/AdvancedSettings.h"
#include "settings/MediaSourceSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/FileUtils.h"
#include "utils/JobManager.h"
#include "utils/StringUtils.h"
#include "utils/XBMCTinyXML.h"
#include "utils/XMLUtils.h"
#include "utils/log.h"

#include <string>
#include <vector>

#ifdef HAS_OPTICAL_DRIVE
using namespace MEDIA_DETECT;
#endif

const char MEDIA_SOURCES_XML[] = { "special://profile/mediasources.xml" };

CMediaManager::CMediaManager()
{
  m_bhasoptical = false;
}

void CMediaManager::Stop()
{
  if (m_platformStorage)
    m_platformStorage->Stop();

  m_platformStorage.reset();
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
}

bool CMediaManager::LoadSources()
{
  // clear our location list
  m_locations.clear();

  // load xml file...
  CXBMCTinyXML xmlDoc;
  if ( !xmlDoc.LoadFile( MEDIA_SOURCES_XML ) )
    return false;

  TiXmlElement* pRootElement = xmlDoc.RootElement();
  if (!pRootElement || StringUtils::CompareNoCase(pRootElement->Value(), "mediasources") != 0)
  {
    CLog::Log(LOGERROR, "Error loading {}, Line {} ({})", MEDIA_SOURCES_XML, xmlDoc.ErrorRow(),
              xmlDoc.ErrorDesc());
    return false;
  }

  // load the <network> block
  TiXmlNode *pNetwork = pRootElement->FirstChild("network");
  if (pNetwork)
  {
    TiXmlElement *pLocation = pNetwork->FirstChildElement("location");
    while (pLocation)
    {
      CNetworkLocation location;
      pLocation->Attribute("id", &location.id);
      if (pLocation->FirstChild())
      {
        location.path = pLocation->FirstChild()->Value();
        m_locations.push_back(location);
      }
      pLocation = pLocation->NextSiblingElement("location");
    }
  }
  LoadAddonSources();
  return true;
}

bool CMediaManager::SaveSources()
{
  CXBMCTinyXML xmlDoc;
  TiXmlElement xmlRootElement("mediasources");
  TiXmlNode *pRoot = xmlDoc.InsertEndChild(xmlRootElement);
  if (!pRoot) return false;

  TiXmlElement networkNode("network");
  TiXmlNode *pNetworkNode = pRoot->InsertEndChild(networkNode);
  if (pNetworkNode)
  {
    for (std::vector<CNetworkLocation>::iterator it = m_locations.begin(); it != m_locations.end(); ++it)
    {
      TiXmlElement locationNode("location");
      locationNode.SetAttribute("id", (*it).id);
      TiXmlText value((*it).path);
      locationNode.InsertEndChild(value);
      pNetworkNode->InsertEndChild(locationNode);
    }
  }
  return xmlDoc.SaveFile(MEDIA_SOURCES_XML);
}

void CMediaManager::GetLocalDrives(VECSOURCES &localDrives, bool includeQ)
{
  std::unique_lock<CCriticalSection> lock(m_CritSecStorageProvider);
  m_platformStorage->GetLocalDrives(localDrives);
}

void CMediaManager::GetRemovableDrives(VECSOURCES &removableDrives)
{
  std::unique_lock<CCriticalSection> lock(m_CritSecStorageProvider);
  if (m_platformStorage)
    m_platformStorage->GetRemovableDrives(removableDrives);
}

void CMediaManager::GetNetworkLocations(VECSOURCES &locations, bool autolocations)
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
    share.strName = g_localizeStrings.Get(20171);
    locations.push_back(share);
#endif

#ifdef HAS_FILESYSTEM_NFS
    share.strPath = "nfs://";
    share.strName = g_localizeStrings.Get(20259);
    locations.push_back(share);
#endif// HAS_FILESYSTEM_NFS

#ifdef HAS_UPNP
    if (CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_SERVICES_UPNP))
    {
      const std::string& strDevices = g_localizeStrings.Get(33040); //"% Devices"
      share.strPath = "upnp://";
      share.strName = StringUtils::Format(strDevices, "UPnP"); //"UPnP Devices"
      locations.push_back(share);
    }
#endif

#ifdef HAS_ZEROCONF
    share.strPath = "zeroconf://";
    share.strName = g_localizeStrings.Get(20262);
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
          share.strName = g_localizeStrings.GetAddonString(addon->ID(), info.label);
          if (share.strName.empty())
            share.strName = g_localizeStrings.Get(info.label);
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
    return ComputeRootAddonTypeSource("executable", g_localizeStrings.Get(1043),
                                      "DefaultAddonProgram.png");
  }
  else if (type == "video" || type == "videos")
  {
    return ComputeRootAddonTypeSource("video", g_localizeStrings.Get(1037),
                                      "DefaultAddonVideo.png");
  }
  else if (type == "music")
  {
    return ComputeRootAddonTypeSource("audio", g_localizeStrings.Get(1038),
                                      "DefaultAddonMusic.png");
  }
  else if (type == "pictures")
  {
    return ComputeRootAddonTypeSource("image", g_localizeStrings.Get(1039),
                                      "DefaultAddonPicture.png");
  }
  else if (type == "games")
  {
    return ComputeRootAddonTypeSource("game", g_localizeStrings.Get(35049), "DefaultAddonGame.png");
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
  source.m_iDriveType = CMediaSource::SOURCE_TYPE_VPATH;
  source.m_ignore = true;
  return source;
}

void CMediaManager::AddAutoSource(const CMediaSource &share, bool bAutorun)
{
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
  CMediaSourceSettings::GetInstance().DeleteSource("files", share.strName, share.strPath, true);
  CMediaSourceSettings::GetInstance().DeleteSource("video", share.strName, share.strPath, true);
  CMediaSourceSettings::GetInstance().DeleteSource("pictures", share.strName, share.strPath, true);
  CMediaSourceSettings::GetInstance().DeleteSource("music", share.strName, share.strPath, true);
  CMediaSourceSettings::GetInstance().DeleteSource("programs", share.strName, share.strPath, true);
  CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UPDATE_SOURCES);
  CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage( msg );

#ifdef HAS_OPTICAL_DRIVE
  // delete cached CdInfo if any
  RemoveCdInfo(TranslateDevicePath(share.strPath, true));
  RemoveDiscInfo(TranslateDevicePath(share.strPath, true));
#endif
}

/////////////////////////////////////////////////////////////
// AutoSource status functions:
//! @todo translate cdda://<device>/

std::string CMediaManager::TranslateDevicePath(const std::string& devicePath, bool bReturnAsDevice)
{
  std::unique_lock<CCriticalSection> waitLock(m_muAutoSource);
  std::string strDevice = devicePath;
  // fallback for cdda://local/ and empty devicePath
#ifdef HAS_OPTICAL_DRIVE
  if(devicePath.empty() || StringUtils::StartsWith(devicePath, "cdda://local"))
    strDevice = m_strFirstAvailDrive;
#endif

#ifdef TARGET_WINDOWS
  if(!m_bhasoptical)
    return "";

  if(bReturnAsDevice == false)
    StringUtils::Replace(strDevice, "\\\\.\\","");
  else if(!strDevice.empty() && strDevice[1]==':')
    strDevice = StringUtils::Format("\\\\.\\{}:", strDevice[0]);

  URIUtils::RemoveSlashAtEnd(strDevice);
#endif
  return strDevice;
}

bool CMediaManager::IsDiscInDrive(const std::string& devicePath)
{
#ifdef HAS_OPTICAL_DRIVE
#ifdef TARGET_WINDOWS
  if(!m_bhasoptical)
    return false;

  std::string strDevice = TranslateDevicePath(devicePath, false);
  std::map<std::string,CCdInfo*>::iterator it;
  std::unique_lock<CCriticalSection> waitLock(m_muAutoSource);
  it = m_mapCdInfo.find(strDevice);
  if(it != m_mapCdInfo.end())
    return true;
  else
    return false;
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

bool CMediaManager::IsAudio(const std::string& devicePath)
{
#ifdef HAS_OPTICAL_DRIVE
#ifdef TARGET_WINDOWS
  if(!m_bhasoptical)
    return false;

  CCdInfo* pCdInfo = GetCdInfo(devicePath);
  if(pCdInfo != NULL && pCdInfo->IsAudio(1))
    return true;

  return false;
#else
  //! @todo switch all ports to use auto sources
  MEDIA_DETECT::CCdInfo* pInfo = MEDIA_DETECT::CDetectDVDMedia::GetCdInfo();
  if (pInfo != NULL && pInfo->IsAudio(1))
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
  if (!m_bhasoptical || !m_platformDiscDriveHander)
    return DriveState::NOT_READY;

  std::string translatedDevicePath = TranslateDevicePath(devicePath, true);
  return m_platformDiscDriveHander->GetDriveState(translatedDevicePath);
#else
  return MEDIA_DETECT::CDetectDVDMedia::GetDriveState();
#endif
#else
  return DriveState::NOT_READY;
#endif
}

#ifdef HAS_OPTICAL_DRIVE
CCdInfo* CMediaManager::GetCdInfo(const std::string& devicePath)
{
#ifdef TARGET_WINDOWS
  if(!m_bhasoptical)
    return NULL;

  std::string strDevice = TranslateDevicePath(devicePath, false);
  std::map<std::string,CCdInfo*>::iterator it;
  {
    std::unique_lock<CCriticalSection> waitLock(m_muAutoSource);
    it = m_mapCdInfo.find(strDevice);
    if(it != m_mapCdInfo.end())
      return it->second;
  }

  CCdInfo* pCdInfo=NULL;
  CCdIoSupport cdio;
  pCdInfo = cdio.GetCdInfo((char*)strDevice.c_str());
  if(pCdInfo!=NULL)
  {
    std::unique_lock<CCriticalSection> waitLock(m_muAutoSource);
    m_mapCdInfo.insert(std::pair<std::string,CCdInfo*>(strDevice,pCdInfo));
  }

  return pCdInfo;
#else
  return MEDIA_DETECT::CDetectDVDMedia::GetCdInfo();
#endif
}

bool CMediaManager::RemoveCdInfo(const std::string& devicePath)
{
  if(!m_bhasoptical)
    return false;

  std::string strDevice = TranslateDevicePath(devicePath, false);

  std::map<std::string,CCdInfo*>::iterator it;
  std::unique_lock<CCriticalSection> waitLock(m_muAutoSource);
  it = m_mapCdInfo.find(strDevice);
  if(it != m_mapCdInfo.end())
  {
    if(it->second != NULL)
      delete it->second;

    m_mapCdInfo.erase(it);
    return true;
  }
  return false;
}

std::string CMediaManager::GetDiskLabel(const std::string& devicePath)
{
#ifdef TARGET_WINDOWS_STORE
  return ""; // GetVolumeInformationW nut support in UWP app
#elif defined(TARGET_WINDOWS)
  if(!m_bhasoptical)
    return "";

  std::string mediaPath = CServiceBroker::GetMediaManager().TranslateDevicePath(devicePath);

  auto cached = m_mapDiscInfo.find(mediaPath);
  if (cached != m_mapDiscInfo.end())
    return cached->second.name;

  // try to minimize the chance of a "device not ready" dialog
  std::string drivePath = CServiceBroker::GetMediaManager().TranslateDevicePath(devicePath, true);
  if (CServiceBroker::GetMediaManager().GetDriveStatus(drivePath) !=
      DriveState::CLOSED_MEDIA_PRESENT)
    return "";

  UTILS::DISCS::DiscInfo info;
  info = GetDiscInfo(mediaPath);
  if (!info.name.empty())
  {
    m_mapDiscInfo[mediaPath] = info;
    return info.name;
  }

  std::string strDevice = TranslateDevicePath(devicePath);
  WCHAR cVolumenName[128];
  WCHAR cFSName[128];
  URIUtils::AddSlashAtEnd(strDevice);
  std::wstring strDeviceW;
  g_charsetConverter.utf8ToW(strDevice, strDeviceW);
  if(GetVolumeInformationW(strDeviceW.c_str(), cVolumenName, 127, NULL, NULL, NULL, cFSName, 127)==0)
    return "";
  g_charsetConverter.wToUTF8(cVolumenName, strDevice);
  info.name = StringUtils::TrimRight(strDevice, " ");
  if (!info.name.empty())
    m_mapDiscInfo[mediaPath] = info;

  return info.name;
#else
  return MEDIA_DETECT::CDetectDVDMedia::GetDVDLabel();
#endif
}

std::string CMediaManager::GetDiskUniqueId(const std::string& devicePath)
{
  std::string mediaPath;

  CCdInfo* pInfo = CServiceBroker::GetMediaManager().GetCdInfo(devicePath);
  if (pInfo == NULL)
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

  UTILS::DISCS::DiscInfo info = GetDiscInfo(mediaPath);
  if (info.empty())
  {
    CLog::Log(LOGDEBUG, "GetDiskUniqueId: Retrieving ID for path {} failed, ID is empty.",
              CURL::GetRedacted(mediaPath));
    return "";
  }

  std::string strID = StringUtils::Format("removable://{}_{}", info.name, info.serial);
  CLog::Log(LOGDEBUG, "GetDiskUniqueId: Got ID {} for disc with path {}", strID,
            CURL::GetRedacted(mediaPath));

  return strID;
}

std::string CMediaManager::GetDiscPath()
{
#ifdef TARGET_WINDOWS
  return CServiceBroker::GetMediaManager().TranslateDevicePath("");
#else

  std::unique_lock<CCriticalSection> lock(m_CritSecStorageProvider);
  VECSOURCES drives;
  m_platformStorage->GetRemovableDrives(drives);
  for(unsigned i = 0; i < drives.size(); ++i)
  {
    if(drives[i].m_iDriveType == CMediaSource::SOURCE_TYPE_DVD && !drives[i].strPath.empty())
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
  std::unique_lock<CCriticalSection> waitLock(m_muAutoSource);
  m_bhasoptical = bstatus;
}

bool CMediaManager::Eject(const std::string& mountpath)
{
  std::unique_lock<CCriticalSection> lock(m_CritSecStorageProvider);
  return m_platformStorage->Eject(mountpath);
}

void CMediaManager::EjectTray( const bool bEject, const char cDriveLetter )
{
#ifdef HAS_OPTICAL_DRIVE
  if (m_platformDiscDriveHander)
  {
    m_platformDiscDriveHander->EjectDriveTray(TranslateDevicePath(""));
  }
#endif
}

void CMediaManager::CloseTray(const char cDriveLetter)
{
#ifdef HAS_OPTICAL_DRIVE
  if (m_platformDiscDriveHander)
  {
    m_platformDiscDriveHander->ToggleDriveTray(TranslateDevicePath(""));
  }
#endif
}

void CMediaManager::ToggleTray(const char cDriveLetter)
{
#ifdef HAS_OPTICAL_DRIVE
  if (m_platformDiscDriveHander)
  {
    m_platformDiscDriveHander->ToggleDriveTray(TranslateDevicePath(""));
  }
#endif
}

void CMediaManager::ProcessEvents()
{
  std::unique_lock<CCriticalSection> lock(m_CritSecStorageProvider);
  if (m_platformStorage->PumpDriveChangeEvents(this))
  {
#if defined(HAS_OPTICAL_DRIVE) && defined(TARGET_DARWIN_OSX)
    // darwins GetFirstOpticalDeviceFileName only gives us something
    // when a disc is inserted
    // so we have to refresh m_strFirstAvailDrive when this happens after Initialize
    // was called (e.x. the disc was inserted after the start of xbmc)
    // else TranslateDevicePath wouldn't give the correct device
    m_strFirstAvailDrive = m_platformStorage->GetFirstOpticalDeviceFileName();
#endif

    CGUIMessage msg(GUI_MSG_NOTIFY_ALL,0,0,GUI_MSG_UPDATE_SOURCES);
    CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg);
  }
}

std::vector<std::string> CMediaManager::GetDiskUsage()
{
  std::unique_lock<CCriticalSection> lock(m_CritSecStorageProvider);
  return m_platformStorage->GetDiskUsage();
}

void CMediaManager::OnStorageAdded(const MEDIA_DETECT::STORAGE::StorageDevice& device)
{
#ifdef HAS_OPTICAL_DRIVE
  const std::shared_ptr<CSettings> settings = CServiceBroker::GetSettingsComponent()->GetSettings();
  if (settings->GetInt(CSettings::SETTING_AUDIOCDS_AUTOACTION) != AUTOCD_NONE || settings->GetBool(CSettings::SETTING_DVDS_AUTORUN))
  {
    if (settings->GetInt(CSettings::SETTING_AUDIOCDS_AUTOACTION) == AUTOCD_RIP)
    {
      CServiceBroker::GetJobManager()->AddJob(new CAutorunMediaJob(device.label, device.path), this,
                                              CJob::PRIORITY_LOW);
    }
    else
    {
      if (device.type == MEDIA_DETECT::STORAGE::Type::OPTICAL)
      {
        if (MEDIA_DETECT::CAutorun::ExecuteAutorun(device.path))
        {
          return;
        }
        CLog::Log(LOGDEBUG, "{}: Could not execute autorun for optical disc with path {}",
                  __FUNCTION__, device.path);
      }
      CServiceBroker::GetJobManager()->AddJob(new CAutorunMediaJob(device.label, device.path), this,
                                              CJob::PRIORITY_HIGH);
    }
  }
  else
  {
    CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info, g_localizeStrings.Get(13021),
                                          device.label, TOAST_DISPLAY_TIME, false);
  }
#endif
}

void CMediaManager::OnStorageSafelyRemoved(const MEDIA_DETECT::STORAGE::StorageDevice& device)
{
  CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info, g_localizeStrings.Get(13023),
                                        device.label, TOAST_DISPLAY_TIME, false);
}

void CMediaManager::OnStorageUnsafelyRemoved(const MEDIA_DETECT::STORAGE::StorageDevice& device)
{
  CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Warning, g_localizeStrings.Get(13022),
                                        device.label);
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
  // check for Blu-ray discs
  if (CFileUtils::Exists(URIUtils::AddFileToFolder(mediaPath, "BDMV", "index.bdmv")))
  {
    info = UTILS::DISCS::ProbeBlurayDiscInfo(mediaPath);
  }

  return info;
}

void CMediaManager::RemoveDiscInfo(const std::string& devicePath)
{
  std::string strDevice = TranslateDevicePath(devicePath, false);

  auto it = m_mapDiscInfo.find(strDevice);
  if (it != m_mapDiscInfo.end())
    m_mapDiscInfo.erase(it);
}

bool CMediaManager::playStubFile(const CFileItem& item)
{
  // Figure out Lines 1 and 2 of the dialog
  std::string strLine1, strLine2;

  // use generic message by default
  strLine1 = g_localizeStrings.Get(435).c_str();
  strLine2 = g_localizeStrings.Get(436).c_str();

  CXBMCTinyXML discStubXML;
  if (discStubXML.LoadFile(item.GetPath()))
  {
    TiXmlElement* pRootElement = discStubXML.RootElement();
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
