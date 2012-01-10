/*
 *      Copyright (C) 2005-2008 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "system.h"
#include "MediaManager.h"
#include "guilib/LocalizeStrings.h"
#include "IoSupport.h"
#include "URL.h"
#include "Util.h"
#include "utils/URIUtils.h"
#ifdef _WIN32
#include "WIN32Util.h"
#endif
#include "guilib/GUIWindowManager.h"
#ifdef HAS_DVD_DRIVE
#include "cdioSupport.h"
#ifndef _WIN32
// TODO: switch all ports to use auto sources
#include "DetectDVDType.h"
#endif
#endif
#include "Autorun.h"
#include "GUIUserMessages.h"
#include "settings/Settings.h"
#include "tinyXML/tinyxml.h"
#include "threads/SingleLock.h"
#include "utils/log.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "utils/JobManager.h"
#include "AutorunMediaJob.h"
#include "settings/GUISettings.h"

#include "FileItem.h"
#include "filesystem/File.h"
#include "filesystem/FactoryDirectory.h"
#include "filesystem/Directory.h"
#include "utils/Crc32.h"

#ifdef __APPLE__
#include "osx/DarwinStorageProvider.h"
#elif defined(_LINUX)
#include "linux/LinuxStorageProvider.h"
#elif _WIN32
#include "windows/Win32StorageProvider.h"
#endif

using namespace std;
using namespace XFILE;

const char MEDIA_SOURCES_XML[] = { "special://profile/mediasources.xml" };

class CMediaManager g_mediaManager;

CMediaManager::CMediaManager()
{
  m_bhasoptical = false;
  m_platformStorage = NULL;
}

void CMediaManager::Stop()
{
  m_platformStorage->Stop();

  delete m_platformStorage;
  m_platformStorage = NULL;
}

void CMediaManager::Initialize()
{
  if (!m_platformStorage)
  {
    #ifdef __APPLE__
      m_platformStorage = new CDarwinStorageProvider();
    #elif defined(_LINUX)
      m_platformStorage = new CLinuxStorageProvider();
    #elif _WIN32
      m_platformStorage = new CWin32StorageProvider();
    #endif
  }
  m_platformStorage->Initialize();
}

bool CMediaManager::LoadSources()
{
  // clear our location list
  m_locations.clear();

  // load xml file...
  TiXmlDocument xmlDoc;
  if ( !xmlDoc.LoadFile( MEDIA_SOURCES_XML ) )
    return false;

  TiXmlElement* pRootElement = xmlDoc.RootElement();
  if ( !pRootElement || strcmpi(pRootElement->Value(), "mediasources") != 0)
  {
    CLog::Log(LOGERROR, "Error loading %s, Line %d (%s)", MEDIA_SOURCES_XML, xmlDoc.ErrorRow(), xmlDoc.ErrorDesc());
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
  return true;
}

bool CMediaManager::SaveSources()
{
  TiXmlDocument xmlDoc;
  TiXmlElement xmlRootElement("mediasources");
  TiXmlNode *pRoot = xmlDoc.InsertEndChild(xmlRootElement);
  if (!pRoot) return false;

  TiXmlElement networkNode("network");
  TiXmlNode *pNetworkNode = pRoot->InsertEndChild(networkNode);
  if (pNetworkNode)
  {
    for (vector<CNetworkLocation>::iterator it = m_locations.begin(); it != m_locations.end(); it++)
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
  CSingleLock lock(m_CritSecStorageProvider);
  m_platformStorage->GetLocalDrives(localDrives);
}

void CMediaManager::GetRemovableDrives(VECSOURCES &removableDrives)
{
  CSingleLock lock(m_CritSecStorageProvider);
  m_platformStorage->GetRemovableDrives(removableDrives);
}

void CMediaManager::GetNetworkLocations(VECSOURCES &locations)
{
  // Load our xml file
  LoadSources();
  for (unsigned int i = 0; i < m_locations.size(); i++)
  {
    CMediaSource share;
    share.strPath = m_locations[i].path;
    CURL url(share.strPath);
    share.strName = url.GetWithoutUserDetails();
    locations.push_back(share);
  }
}

bool CMediaManager::AddNetworkLocation(const CStdString &path)
{
  CNetworkLocation location;
  location.path = path;
  location.id = (int)m_locations.size();
  m_locations.push_back(location);
  return SaveSources();
}

bool CMediaManager::HasLocation(const CStdString& path) const
{
  for (unsigned int i=0;i<m_locations.size();++i)
  {
    if (m_locations[i].path == path)
      return true;
  }

  return false;
}


bool CMediaManager::RemoveLocation(const CStdString& path)
{
  for (unsigned int i=0;i<m_locations.size();++i)
  {
    if (m_locations[i].path == path)
    {
      // prompt for sources, remove, cancel,
      m_locations.erase(m_locations.begin()+i);
      return SaveSources();
    }
  }

  return false;
}

bool CMediaManager::SetLocationPath(const CStdString& oldPath, const CStdString& newPath)
{
  for (unsigned int i=0;i<m_locations.size();++i)
  {
    if (m_locations[i].path == oldPath)
    {
      m_locations[i].path = newPath;
      return SaveSources();
    }
  }

  return false;
}

void CMediaManager::AddAutoSource(const CMediaSource &share, bool bAutorun)
{
  g_settings.AddShare("files",share);
  g_settings.AddShare("video",share);
  g_settings.AddShare("pictures",share);
  g_settings.AddShare("music",share);
  g_settings.AddShare("programs",share);
  CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UPDATE_SOURCES);
  g_windowManager.SendThreadMessage( msg );

#ifdef HAS_DVD_DRIVE
  if(bAutorun)
    MEDIA_DETECT::CAutorun::ExecuteAutorun(share.strPath);
#endif
}

void CMediaManager::RemoveAutoSource(const CMediaSource &share)
{
  g_settings.DeleteSource("files", share.strName, share.strPath, true);
  g_settings.DeleteSource("video", share.strName, share.strPath, true);
  g_settings.DeleteSource("pictures", share.strName, share.strPath, true);
  g_settings.DeleteSource("music", share.strName, share.strPath, true);
  g_settings.DeleteSource("programs", share.strName, share.strPath, true);
  CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UPDATE_SOURCES);
  g_windowManager.SendThreadMessage( msg );

#ifdef HAS_DVD_DRIVE
  // delete cached CdInfo if any
  RemoveCdInfo(TranslateDevicePath(share.strPath, true));
#endif
}

/////////////////////////////////////////////////////////////
// AutoSource status functions:
// - TODO: translate cdda://<device>/

CStdString CMediaManager::TranslateDevicePath(const CStdString& devicePath, bool bReturnAsDevice)
{
  CSingleLock waitLock(m_muAutoSource);
  CStdString strDevice = devicePath;
  // fallback for cdda://local/ and empty devicePath
#ifdef HAS_DVD_DRIVE
  if(devicePath.empty() || devicePath.Left(12).Compare("cdda://local")==0)
    strDevice = MEDIA_DETECT::CLibcdio::GetInstance()->GetDeviceFileName();
#endif

#ifdef _WIN32
  if(!m_bhasoptical)
    return "";

  if(bReturnAsDevice == false)
    strDevice.Replace("\\\\.\\","");
  else if(!strDevice.empty() && strDevice[1]==':')
    strDevice.Format("\\\\.\\%c:", strDevice[0]);

  URIUtils::RemoveSlashAtEnd(strDevice);
#endif
  return strDevice;
}

bool CMediaManager::IsDiscInDrive(const CStdString& devicePath)
{
#ifdef HAS_DVD_DRIVE
#ifdef _WIN32
  if(!m_bhasoptical)
    return false;

  CSingleLock waitLock(m_muAutoSource);
  CStdString strDevice = TranslateDevicePath(devicePath, true);
  std::map<CStdString,CCdInfo*>::iterator it;
  it = m_mapCdInfo.find(strDevice);
  if(it != m_mapCdInfo.end())
    return true;
  else
    return false;
#else
  if(URIUtils::IsDVD(devicePath) || devicePath.IsEmpty())
    return MEDIA_DETECT::CDetectDVDMedia::IsDiscInDrive();   // TODO: switch all ports to use auto sources
  else
    return true; // Assume other paths to be mounted already
#endif
#else
  return false;
#endif
}

bool CMediaManager::IsAudio(const CStdString& devicePath)
{
#ifdef HAS_DVD_DRIVE
#ifdef _WIN32
  if(!m_bhasoptical)
    return false;

  CCdInfo* pCdInfo = GetCdInfo(devicePath);
  if(pCdInfo != NULL && pCdInfo->IsAudio(1))
    return true;

  return false;
#else
  // TODO: switch all ports to use auto sources
  MEDIA_DETECT::CCdInfo* pInfo = MEDIA_DETECT::CDetectDVDMedia::GetCdInfo();
  if (pInfo != NULL && pInfo->IsAudio(1))
    return true;
#endif
#endif
  return false;
}

DWORD CMediaManager::GetDriveStatus(const CStdString& devicePath)
{
#ifdef HAS_DVD_DRIVE
#ifdef _WIN32
  if(!m_bhasoptical)
    return DRIVE_NOT_READY;

  CSingleLock waitLock(m_muAutoSource);
  CStdString strDevice = TranslateDevicePath(devicePath, true);
  DWORD dwRet = DRIVE_NOT_READY;
  int status = CWIN32Util::GetDriveStatus(strDevice);

  switch(status)
  {
  case -1: // error
    dwRet = DRIVE_NOT_READY;
    break;
  case 0: // no media
    dwRet = DRIVE_CLOSED_NO_MEDIA;
    break;
  case 1: // tray open
    dwRet = DRIVE_OPEN;
    break;
  case 2: // media accessible
    dwRet = DRIVE_CLOSED_MEDIA_PRESENT;
    break;
  }
  return dwRet;
#else
  return MEDIA_DETECT::CDetectDVDMedia::DriveReady();
#endif
#else
  return DRIVE_NOT_READY;
#endif
}

#ifdef HAS_DVD_DRIVE
CCdInfo* CMediaManager::GetCdInfo(const CStdString& devicePath)
{
#ifdef _WIN32
  if(!m_bhasoptical)
    return NULL;

  CSingleLock waitLock(m_muAutoSource);
  CCdInfo* pCdInfo=NULL;
  CStdString strDevice = TranslateDevicePath(devicePath, true);
  std::map<CStdString,CCdInfo*>::iterator it;
  it = m_mapCdInfo.find(strDevice);
  if(it != m_mapCdInfo.end())
    return it->second;

  CCdIoSupport cdio;
  pCdInfo = cdio.GetCdInfo((char*)strDevice.c_str());
  if(pCdInfo!=NULL)
    m_mapCdInfo.insert(std::pair<CStdString,CCdInfo*>(strDevice,pCdInfo));

  return pCdInfo;
#else
  return MEDIA_DETECT::CDetectDVDMedia::GetCdInfo();
#endif
}

bool CMediaManager::RemoveCdInfo(const CStdString& devicePath)
{
  if(!m_bhasoptical)
    return false;

  CSingleLock waitLock(m_muAutoSource);
  CStdString strDevice = TranslateDevicePath(devicePath, true);

  std::map<CStdString,CCdInfo*>::iterator it;
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

CStdString CMediaManager::GetDiskLabel(const CStdString& devicePath)
{
#ifdef _WIN32
  if(!m_bhasoptical)
    return "";

  CSingleLock waitLock(m_muAutoSource);
  CStdString strDevice = TranslateDevicePath(devicePath);
  char cVolumenName[128];
  char cFSName[128];
  URIUtils::AddSlashAtEnd(strDevice);
  if(GetVolumeInformation(strDevice.c_str(), cVolumenName, 127, NULL, NULL, NULL, cFSName, 127)==0)
    return "";
  return CStdString(cVolumenName).TrimRight(" ");
#else
  return MEDIA_DETECT::CDetectDVDMedia::GetDVDLabel();
#endif
}

CStdString CMediaManager::GetDiskUniqueId(const CStdString& devicePath)
{
  CStdString strDevice = devicePath;

  if (strDevice.IsEmpty()) // if no value passed, use the current default disc path.
    strDevice = GetDiscPath();    // in case of non-Windows we must obtain the disc path

#ifdef _WIN32
  if (!m_bhasoptical)
    return "";
  strDevice = TranslateDevicePath(strDevice);
  URIUtils::AddSlashAtEnd(strDevice);
#endif

  CStdString strDrive = g_mediaManager.TranslateDevicePath(strDevice);

#ifndef _WIN32
  {
    CSingleLock waitLock(m_muAutoSource);  
    CCdInfo* pInfo = g_mediaManager.GetCdInfo();
    if ( pInfo  )
    {
      if (pInfo->IsISOUDF(1) || pInfo->IsISOHFS(1) || pInfo->IsIso9660(1) || pInfo->IsIso9660Interactive(1))
        strDrive = "iso9660://";
      else
        strDrive = "D:\\";
    }
    else
    {
      CLog::Log(LOGERROR, "GetDiskUniqueId: Failed getting CD info");
      return "";
    }
  }
#endif

  CStdString pathVideoTS = URIUtils::AddFileToFolder(strDrive, "VIDEO_TS");
  if(! CDirectory::Exists(pathVideoTS) )
    return ""; // return empty

  CLog::Log(LOGDEBUG, "GetDiskUniqueId: Trying to retrieve ID for path %s", pathVideoTS.c_str());
  uint32_t dvdcrc = 0;
  CStdString strID;

  if (HashDVD(pathVideoTS, dvdcrc))
  {
    strID.Format("removable://%s_%08x", GetDiskLabel(devicePath), dvdcrc);
    CLog::Log(LOGDEBUG, "GetDiskUniqueId: Got ID %s for DVD disk", strID.c_str());
  }

  return strID;
}

bool CMediaManager::HashDVD(const CStdString& dvdpath, uint32_t& crc)
{
  CFileItemList vecItemsTS;
  bool success = false;

  // first try to open the VIDEO_TS folder of the DVD
  if (!CDirectory::GetDirectory( dvdpath, vecItemsTS, ".ifo" ))
  {
    CLog::Log(LOGERROR, "%s - Cannot open dvd VIDEO_TS folder -- ABORTING", __FUNCTION__);
    return false;
  }

  Crc32 crc32;
  bool dataRead = false;

  vecItemsTS.Sort(SORT_METHOD_FILE, SORT_ORDER_ASC);
  for (int i = 0; i < vecItemsTS.Size(); i++) 
  {
    CFileItemPtr videoTSItem = vecItemsTS[i];
    success = true;

    // get the file name for logging purposes
    CStdString fileName = URIUtils::GetFileName(videoTSItem->GetPath());
    CLog::Log(LOGDEBUG, "%s - Adding file content for dvd file: %s", __FUNCTION__, fileName.c_str());
    CFile file;
    if(!file.Open(videoTSItem->GetPath()))
    {
      CLog::Log(LOGERROR, "%s - Cannot open dvd file: %s -- ABORTING", __FUNCTION__, fileName.c_str());
      return false;
    }
    int res;
    char buf[2048];
    while( (res = file.Read(buf, sizeof(buf))) > 0) 
    {
      dataRead = true;
      crc32.Compute(buf, res);
    }
    file.Close();
  }

  if (!dataRead)
  {
    CLog::Log(LOGERROR, "%s - Did not read any data from the IFO files -- ABORTING", __FUNCTION__);
    return false;
  }

  // put result back in reference parameter
  crc = (uint32_t) crc32;

  return success;
}


CStdString CMediaManager::GetDiscPath()
{
#ifdef _WIN32
  return g_mediaManager.TranslateDevicePath("");
#else

  CSingleLock lock(m_CritSecStorageProvider);
  VECSOURCES drives;
  m_platformStorage->GetRemovableDrives(drives);
  for(unsigned i = 0; i < drives.size(); ++i)
  {
    if(drives[i].m_iDriveType == CMediaSource::SOURCE_TYPE_DVD)
      return drives[i].strPath;
  }

  // iso9660://, cdda://local/ or D:\ depending on disc type
  return MEDIA_DETECT::CDetectDVDMedia::GetDVDPath();
#endif
}
#endif

void CMediaManager::SetHasOpticalDrive(bool bstatus)
{
  CSingleLock waitLock(m_muAutoSource);
  m_bhasoptical = bstatus;
}

bool CMediaManager::Eject(CStdString mountpath)
{
  CSingleLock lock(m_CritSecStorageProvider);
  return m_platformStorage->Eject(mountpath);
}

void CMediaManager::ProcessEvents()
{
  CSingleLock lock(m_CritSecStorageProvider);
  if (m_platformStorage->PumpDriveChangeEvents(this))
  {
    CGUIMessage msg(GUI_MSG_NOTIFY_ALL,0,0,GUI_MSG_UPDATE_SOURCES);
    g_windowManager.SendThreadMessage(msg);
  }
}

std::vector<CStdString> CMediaManager::GetDiskUsage()
{
  CSingleLock waitLock(m_muAutoSource);
  return m_platformStorage->GetDiskUsage();
}

void CMediaManager::OnStorageAdded(const CStdString &label, const CStdString &path)
{
  if (g_guiSettings.GetBool("audiocds.autorun") || g_guiSettings.GetBool("dvds.autorun"))
    CJobManager::GetInstance().AddJob(new CAutorunMediaJob(label, path), this, CJob::PRIORITY_HIGH);
  else
    CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info, g_localizeStrings.Get(13021), label, TOAST_DISPLAY_TIME, false);
}

void CMediaManager::OnStorageSafelyRemoved(const CStdString &label)
{
  CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info, g_localizeStrings.Get(13023), label, TOAST_DISPLAY_TIME, false);
}

void CMediaManager::OnStorageUnsafelyRemoved(const CStdString &label)
{
  CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Warning, g_localizeStrings.Get(13022), label);
}
