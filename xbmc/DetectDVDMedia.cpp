/*
 *      Copyright (C) 2005-2009 Team XBMC
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

#include "stdafx.h"
#include "DetectDVDMedia.h"
#ifdef _WIN32PC
#include "WIN32Util.h"
#endif

using namespace MEDIA_DETECT;

CEvent CDetectDVDMedia::m_evAutorun;

CDetectDVDMedia* CDetectDVDMedia::m_instance = NULL;


CDetectDVDMedia::CDetectDVDMedia()
{
  DetectInsertedMedia();
}

CDetectDVDMedia::~CDetectDVDMedia()
{
  RemoveAllMedia();
}
 
CDetectDVDMedia* CDetectDVDMedia::GetInstance()
{
  if( !m_instance )
    m_instance = new CDetectDVDMedia();
  return m_instance;
}

void CDetectDVDMedia::DetectInsertedMedia()
{
#ifdef _WIN32PC
  char* pcBuffer=NULL;
  DWORD dwStrLength= GetLogicalDriveStrings( 0, pcBuffer );
  if( dwStrLength != 0 )
  {
    dwStrLength+= 1;
    pcBuffer= new char [dwStrLength];
    GetLogicalDriveStrings( dwStrLength, pcBuffer );

    UINT uDriveType;
    int iPos= 0;
    do
    {
      CStdString strPath = pcBuffer + iPos;
      uDriveType= GetDriveType( strPath.c_str()  );
      if(uDriveType == DRIVE_CDROM && IsDiscInDrive(strPath))
        AddMedia(strPath);

      iPos += (strlen( pcBuffer + iPos) + 1 );
    } 
    while( strlen( pcBuffer + iPos ) > 0 );
  }
  if(pcBuffer != NULL)
    delete[] pcBuffer;
#endif
}

void CDetectDVDMedia::Destroy()
{
  if (m_instance)
  {
    delete m_instance;
    m_instance = NULL;
  }
}

CStdString CDetectDVDMedia::GetDrive(const CStdString& strDrive)
{
  CStdString strPath;
  if(!strDrive.empty() && strDrive[1]==':')
    strPath = strDrive.substr(0,2);
  else if(strDrive.Left(5).Equals("cdda:") && !strDrive.Left(12).Equals("cdda://local"))
    strPath.Format("%c:", strDrive[7]);
  else
    strPath = GetDevice(strDrive).c_str()+4;

  strPath.MakeLower();

  return strPath;
}

CStdString CDetectDVDMedia::GetDevice(const CStdString& strDrive)
{
  CStdString strDevice;
  if(!strDrive.empty() && strDrive[1]==':')
    strDevice.Format("\\\\.\\%c:", strDrive[0]);
  else if(strDrive.Left(5).Equals("cdda:") && !strDrive.Left(12).Equals("cdda://local"))
    strDevice.Format("\\\\.\\%c:", strDrive[7]);
  else
    strDevice = CLibcdio::GetInstance()->GetDeviceFileName();

  strDevice.MakeLower();

  return strDevice;
}

void CDetectDVDMedia::AddMedia(const CStdString& strDrive)
{
  CSingleLock waitLock(m_critsec);
  CStdString strPath = GetDrive(strDrive);
  std::map<char,CCdInfo*>::iterator it;
  it = m_mapCdInfo.find(strPath[0]);
  if(it == m_mapCdInfo.end())
  {
    CStdString strNewUrl;
    CCdIoSupport cdio;
    CLog::Log(LOGINFO, "Detecting DVD-ROM media filesystem...");
    CCdInfo* pCdInfo = cdio.GetCdInfo((char*)strPath.c_str());
    if (pCdInfo == NULL)
    {
      CLog::Log(LOGERROR, "Detection of DVD-ROM media failed.");
      return ;
    }
    else
      m_mapCdInfo.insert(std::pair<char,CCdInfo*>(strPath[0],pCdInfo));

    CLog::Log(LOGINFO, "Tracks overall:%i; Audio tracks:%i; Data tracks:%i",
              pCdInfo->GetTrackCount(),
              pCdInfo->GetAudioTrackCount(),
              pCdInfo->GetDataTrackCount() );

    if(pCdInfo->IsAudio(1))
      //strNewUrl.Format("cdda://%c/", strPath[0]); // we need to extend the cdda: protocol to support other drives (dvd:// as well?)
      strNewUrl = "cdda://local/";
    else
      strNewUrl = strPath;

    CLog::Log(LOGINFO, "Using protocol %s", strNewUrl.c_str());

    if (pCdInfo->IsValidFs())
    {
      if (!pCdInfo->IsAudio(1))
        CLog::Log(LOGINFO, "Disc label: %s", pCdInfo->GetDiscLabel().c_str());
    }
    else
    {
      CLog::Log(LOGWARNING, "Filesystem is not supported");
    }

    // workaround until we have support for multiple drives
    m_evAutorun.Set();

  }
}

void CDetectDVDMedia::RemoveMedia(const CStdString& strDrive)
{
  CSingleLock waitLock(m_critsec);
  CStdString strPath = GetDrive(strDrive);
  std::map<char,CCdInfo*>::iterator it;
  it = m_mapCdInfo.find(strPath[0]);
  if(it != m_mapCdInfo.end())
  {
    if(it->second != NULL)
      delete it->second;

    m_mapCdInfo.erase(it);
  }
}

void CDetectDVDMedia::RemoveAllMedia()
{
  CSingleLock waitLock(m_critsec);
  std::map<char,CCdInfo*>::iterator it;
  for (it=m_mapCdInfo.begin();it!=m_mapCdInfo.end();++it)
  {
    if(it->second != NULL)
      delete it->second;
  }
  m_mapCdInfo.clear();
}

CCdInfo* CDetectDVDMedia::GetCdInfo(const CStdString& strDrive)
{
  CSingleLock waitLock(m_critsec);
  CStdString strPath = GetDrive(strDrive);
  std::map<char,CCdInfo*>::iterator it;
  it = m_mapCdInfo.find(strPath[0]);
  if(it != m_mapCdInfo.end())
    return it->second;
  else
    return NULL;
}

bool CDetectDVDMedia::IsAudio(const CStdString& strDrive)
{
  CSingleLock waitLock(m_critsec);
  CCdInfo* pCdInfo = GetCdInfo(strDrive);
  if(pCdInfo != NULL)
    return pCdInfo->IsAudio(1);
  else
    return false;
}

bool CDetectDVDMedia::IsDiscInDrive(const CStdString& strDrive)
{
  return GetTrayState(strDrive) == DRIVE_CLOSED_MEDIA_PRESENT;
}

void CDetectDVDMedia::WaitMediaReady()
{
  CSingleLock waitLock(m_critsec);
}

CStdString CDetectDVDMedia::GetDVDLabel(const CStdString& strDrive)
{
  CSingleLock waitLock(m_critsec);
  CStdString strPath = GetDrive(strDrive);
  std::map<char,CCdInfo*>::iterator it;
  it = m_mapCdInfo.find(strPath[0]);
  if(it == m_mapCdInfo.end())
    return "";
  
  CStdString strLabel;
  if(it->second->IsAudio(1))
    strLabel = "Audio-CD";
  else
  {
    strLabel = it->second->GetDiscLabel();
    strLabel.TrimRight(" ");
  }

  return strLabel;
}

CStdString CDetectDVDMedia::GetDVDPath(const CStdString& strDrive)
{
  CSingleLock waitLock(m_critsec);
  CStdString strPath = GetDrive(strDrive);
  std::map<char,CCdInfo*>::iterator it;
  it = m_mapCdInfo.find(strPath[0]);
  if(it != m_mapCdInfo.end())
  {
    if(it->second->IsAudio(1))
      strPath = "cdda://local/";
  }
  return strPath;
}

DWORD CDetectDVDMedia::GetTrayState(const CStdString& strDrive)
{
  CSingleLock waitLock(m_critsec);
  
  DWORD dwDriveState=1;
  int status = CWIN32Util::GetDriveStatus(GetDevice(strDrive));
  switch(status)
  {
  case -1: // error
    dwDriveState = DRIVE_NOT_READY;
    break;
  case 0: // no media
    dwDriveState = DRIVE_CLOSED_NO_MEDIA;
    break;
  case 1: // tray open
    dwDriveState = DRIVE_OPEN;      
    break;
  case 2: // media accessible
    dwDriveState = DRIVE_CLOSED_MEDIA_PRESENT;
    break;
  default:
    dwDriveState = DRIVE_NOT_READY;
  }

#ifdef HAS_DVD_DRIVE
  return dwDriveState;
#else
  return DRIVE_READY;
#endif
}
