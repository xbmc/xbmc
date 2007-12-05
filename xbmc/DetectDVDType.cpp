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
#include "DetectDVDType.h"
#include "FileSystem/cdioSupport.h"
#include "FileSystem/iso9660.h"
#ifdef HAS_UNDOCUMENTED
#include "xbox/Undocumented.h"
#endif
#ifdef _LINUX
#include <sys/types.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <linux/cdrom.h>
#endif
#include "Application.h"
#include "Util.h"
#include "Picture.h"
#if defined (LIBCDIO_VERSION_NUM) && (LIBCDIO_VERSION_NUM > 77)
#define USING_CDIO78
#include <cdio/mmc.h>
#endif

using namespace XFILE;
using namespace MEDIA_DETECT;

CCriticalSection CDetectDVDMedia::m_muReadingMedia;
CEvent CDetectDVDMedia::m_evAutorun;
int CDetectDVDMedia::m_DriveState = DRIVE_CLOSED_NO_MEDIA;
CCdInfo* CDetectDVDMedia::m_pCdInfo = NULL;
time_t CDetectDVDMedia::m_LastPoll = 0;
CDetectDVDMedia* CDetectDVDMedia::m_pInstance = NULL;
CStdString CDetectDVDMedia::m_diskLabel = "";
CStdString CDetectDVDMedia::m_diskPath = "";

CDetectDVDMedia::CDetectDVDMedia()
{
  m_bAutorun = false;
  m_bStop = false;
  m_dwLastTrayState = 0;
  m_bStartup = true;  // Do not autorun on startup
  m_pInstance = this;
#ifdef _LINUX
  // This fails on win32 due to the critical section (static) in CLog not being
  // initialized yet :(
  CLog::Log(LOGDEBUG, "Compiled with libcdio Version 0.%d", LIBCDIO_VERSION_NUM);
#endif
}

CDetectDVDMedia::~CDetectDVDMedia()
{
}

void CDetectDVDMedia::OnStartup()
{
  // SetPriority( THREAD_PRIORITY_LOWEST );
}

void CDetectDVDMedia::Process()
{
  if (g_advancedSettings.m_usePCDVDROM)
  {
    m_DriveState = DRIVE_CLOSED_MEDIA_PRESENT;
  }

  while (( !m_bStop ) && (!g_advancedSettings.m_usePCDVDROM))
  {
    Sleep(500);
    UpdateDvdrom();
    m_bStartup = false;
    if ( m_bAutorun )
    {
      m_evAutorun.Set();
      m_bAutorun = false;
    }
  }
}

void CDetectDVDMedia::OnExit()
{
}

// Gets state of the DVD drive
VOID CDetectDVDMedia::UpdateDvdrom()
{
  // Signal for WaitMediaReady()
  // that we are busy detecting the
  // newly inserted media.
  {
    CSingleLock waitLock(m_muReadingMedia);
    switch (GetTrayState())
    {
      case DRIVE_OPEN:
        {
          // Send Message to GUI that disc been ejected
          SetNewDVDShareUrl("D:\\", false, g_localizeStrings.Get(502));
          CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_REMOVED_MEDIA, 0, NULL);
          m_gWindowManager.SendThreadMessage( msg );
          m_isoReader.Reset();
          waitLock.Leave();
          m_DriveState = DRIVE_OPEN;
          return;
        }
        break;

      case DRIVE_NOT_READY:
        {
          // drive is not ready (closing, opening)
          m_isoReader.Reset();
          SetNewDVDShareUrl("D:\\", false, g_localizeStrings.Get(503));
          m_DriveState = DRIVE_NOT_READY;
          // DVD-ROM in undefined state
          // better delete old CD Information
          if ( m_pCdInfo != NULL )
          {
            delete m_pCdInfo;
            m_pCdInfo = NULL;
          }
          waitLock.Leave();
          CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UPDATE_SOURCES, 0, NULL);
          m_gWindowManager.SendThreadMessage( msg );
          // Do we really need sleep here? This will fix: [ 1530771 ] "Open tray" problem
          // Sleep(6000);
          return ;
        }
        break;

      case DRIVE_READY:
        // drive is ready
        //m_DriveState = DRIVE_READY;
        return ;
        break;
      case DRIVE_CLOSED_NO_MEDIA:
        {
          // nothing in there...
          m_isoReader.Reset();
          m_DriveState = DRIVE_CLOSED_NO_MEDIA;
          SetNewDVDShareUrl("D:\\", false, g_localizeStrings.Get(504));
          // Send Message to GUI that disc has changed
          CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UPDATE_SOURCES, 0, NULL);
          waitLock.Leave();
          m_gWindowManager.SendThreadMessage( msg );
          return ;
        }
        break;
      case DRIVE_CLOSED_MEDIA_PRESENT:
        {
          m_DriveState = DRIVE_CLOSED_MEDIA_PRESENT;
          // drive has been closed and is ready
          OutputDebugString("Drive closed media present, remounting...\n");
          CIoSupport::Dismount("Cdrom0");
          CIoSupport::RemapDriveLetter('D', "Cdrom0");
          // Detect ISO9660(mode1/mode2) or CDDA filesystem
          DetectMediaType();
          CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UPDATE_SOURCES, 0, NULL);
          waitLock.Leave();
          m_gWindowManager.SendThreadMessage( msg );
          // Tell the application object that a new Cd is inserted
          // So autorun can be started.
          if ( !m_bStartup )
            m_bAutorun = true;
          return ;
        }
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

  CStdString strNewUrl;
  CCdIoSupport cdio;
  // Delete old CD-Information
  if ( m_pCdInfo != NULL )
  {
    delete m_pCdInfo;
    m_pCdInfo = NULL;
  }

  // Detect new CD-Information
  m_pCdInfo = cdio.GetCdInfo();
  if (m_pCdInfo == NULL)
  {
    CLog::Log(LOGERROR, "Detection of DVD-ROM media failed.");
    return ;
  }
  CLog::Log(LOGINFO, "Tracks overall:%i; Audio tracks:%i; Data tracks:%i",
            m_pCdInfo->GetTrackCount(),
            m_pCdInfo->GetAudioTrackCount(),
            m_pCdInfo->GetDataTrackCount() );

  // Detect ISO9660(mode1/mode2), CDDA filesystem or UDF
  if (m_pCdInfo->IsISOHFS(1) || m_pCdInfo->IsIso9660(1) || m_pCdInfo->IsIso9660Interactive(1))
  {
    strNewUrl = "iso9660://";
    m_isoReader.Scan();
  }
  else
  {
    if (m_pCdInfo->IsUDF(1) || m_pCdInfo->IsUDFX(1))
      strNewUrl = "D:\\";
    else if (m_pCdInfo->IsAudio(1))
    {
      strNewUrl = "cdda://local/";
      bCDDA = true;
    }
    else
      strNewUrl = "D:\\";
  }

  if (m_pCdInfo->IsISOUDF(1))
  {
    if (!g_advancedSettings.m_detectAsUdf)
    {
      strNewUrl = "iso9660://";
      m_isoReader.Scan();
    }
    else
    {
      strNewUrl = "D:\\";
    }
  }

  CLog::Log(LOGINFO, "Using protocol %s", strNewUrl.c_str());

  if (m_pCdInfo->IsValidFs())
  {
    if (!m_pCdInfo->IsAudio(1))
      CLog::Log(LOGINFO, "Disc label: %s", m_pCdInfo->GetDiscLabel().c_str());
  }
  else
  {
    CLog::Log(LOGWARNING, "Filesystem is not supported");
  }

  CStdString strLabel = "";
  if (bCDDA)
  {
    strLabel = "Audio-CD";
  }
  else
  {
    strLabel = m_pCdInfo->GetDiscLabel();
    strLabel.TrimRight(" ");
  }

  SetNewDVDShareUrl( strNewUrl , bCDDA, strLabel);
}

void CDetectDVDMedia::SetNewDVDShareUrl( const CStdString& strNewUrl, bool bCDDA, const CStdString& strDiscLabel )
{
  CStdString strDescription = "DVD";
  if (bCDDA) strDescription = "CD";

  if (strDiscLabel != "") strDescription = strDiscLabel;

  // store it in case others want it
  m_diskLabel = strDescription;
  m_diskPath = strNewUrl;

  // delete any previously cached disc thumbnail
  CStdString strCache = "Z:\\dvdicon.tbn";
  if (CFile::Exists(strCache))
    CFile::Delete(strCache);

  // find and cache disc thumbnail, and update label to xbe label if applicable
  if ((g_advancedSettings.m_usePCDVDROM || IsDiscInDrive()) && !bCDDA)
  {
    // update disk label to xbe label if we have that info
    if (CFile::Exists("D:\\default.xbe"))
      CUtil::GetXBEDescription("D:\\default.xbe", m_diskLabel);

    // and get the thumb
    CStdString strThumb;
    CStdStringArray thumbs;
    StringUtils::SplitString(g_advancedSettings.m_dvdThumbs, "|", thumbs);
    for (unsigned int i = 0; i < thumbs.size(); ++i)
    {
      CUtil::AddFileToFolder(m_diskPath, thumbs[i], strThumb);
      CLog::Log(LOGDEBUG,"%s: looking for disc thumb:[%s]", __FUNCTION__, strThumb.c_str());
      if (CFile::Exists(strThumb))
      {
        CLog::Log(LOGDEBUG,"%s: found disc thumb:[%s], caching as:[%s]", __FUNCTION__, strThumb.c_str(), strCache.c_str());
        CPicture pic;
        pic.DoCreateThumbnail(strThumb, strCache);
        break;
      }
    }
  }
}

DWORD CDetectDVDMedia::GetTrayState()
{
#ifdef HAS_UNDOCUMENTED
  HalReadSMCTrayState(&m_dwTrayState, &m_dwTrayCount);
#endif
#ifdef _LINUX

  char* dvdDevice = CCdIoSupport::GetDeviceFileName();
  if (strlen(dvdDevice) == 0)
    return DRIVE_NOT_READY;

#ifndef USING_CDIO78

  int fd = 0;

  fd = open(dvdDevice, O_RDONLY | O_NONBLOCK);
  if (fd<0)
  {
    CLog::Log(LOGERROR, "Unable to open CD-ROM device %s for polling.", dvdDevice);
    return DRIVE_NOT_READY;
  }

  int drivestatus = ioctl(fd, CDROM_DRIVE_STATUS, 0);

  switch(drivestatus)
  {
  case CDS_NO_INFO:
    m_dwTrayState = TRAY_CLOSED_NO_MEDIA;
    break;

  case CDS_NO_DISC:
    m_dwTrayState = TRAY_CLOSED_NO_MEDIA;
    break;

  case CDS_TRAY_OPEN:
    m_dwTrayState = TRAY_OPEN;
    break;

  case CDS_DISC_OK:
    m_dwTrayState = TRAY_CLOSED_MEDIA_PRESENT;
    break;

  case CDS_DRIVE_NOT_READY:
    close(fd);
    return DRIVE_NOT_READY;

  default:
    m_dwTrayState = TRAY_CLOSED_NO_MEDIA;
  }

  close(fd);

#else

  // The following code works with libcdio >= 0.78
  // To enable it, download and install the latest version from
  // http://www.gnu.org/software/libcdio/
  // -d4rk 06/27/07


  m_dwTrayState = TRAY_CLOSED_MEDIA_PRESENT;
  CdIo_t* cdio = cdio_open(dvdDevice, DRIVER_UNKNOWN);
  if (cdio)
  {
    discmode_t discmode = CDIO_DISC_MODE_NO_INFO;
    int status = mmc_get_tray_status(cdio);
    if (status==0)
      discmode = cdio_get_discmode(cdio);
    switch(status)
    {
    case 0: //closed
      if (discmode==CDIO_DISC_MODE_NO_INFO || discmode==CDIO_DISC_MODE_ERROR)
        m_dwTrayState = TRAY_CLOSED_NO_MEDIA;
      else
	m_dwTrayState = TRAY_CLOSED_MEDIA_PRESENT;
      break;

    case 1: //open
      m_dwTrayState = TRAY_OPEN;
      break;
    }
    cdio_destroy(cdio);
  }

  
#endif // USING_CDIO78
#endif // _LINUX

  if (m_dwTrayState == TRAY_CLOSED_MEDIA_PRESENT)
  {
    if (m_dwLastTrayState != TRAY_CLOSED_MEDIA_PRESENT)
    {
      m_dwLastTrayState = m_dwTrayState;
      return DRIVE_CLOSED_MEDIA_PRESENT;
    }
    else
    {
      return DRIVE_READY;
    }
  }
  else if (m_dwTrayState == TRAY_CLOSED_NO_MEDIA)
  {
    if ( (m_dwLastTrayState != TRAY_CLOSED_NO_MEDIA) && (m_dwLastTrayState != TRAY_CLOSED_MEDIA_PRESENT) )
    {
      m_dwLastTrayState = m_dwTrayState;
      return DRIVE_CLOSED_NO_MEDIA;
    }
    else
    {
      return DRIVE_READY;
    }
  }
  else if (m_dwTrayState == TRAY_OPEN)
  {
    if (m_dwLastTrayState != TRAY_OPEN)
    {
      m_dwLastTrayState = m_dwTrayState;
      return DRIVE_OPEN;
    }
    else
    {
      return DRIVE_READY;
    }
  }
  else
  {
    m_dwLastTrayState = m_dwTrayState;
  }

#ifdef HAS_DVD_DRIVE
  return DRIVE_NOT_READY;
#else
  return DRIVE_READY;
#endif
}

void CDetectDVDMedia::UpdateState()
{
  CSingleLock waitLock(m_muReadingMedia);
  m_pInstance->DetectMediaType();
}

// Static function
// Wait for drive, to finish media detection.
void CDetectDVDMedia::WaitMediaReady()
{
  CSingleLock waitLock(m_muReadingMedia);
}

// Static function
// Returns status of the DVD Drive
int CDetectDVDMedia::DriveReady()
{
  return m_DriveState;
}

// Static function
// Whether a disc is in drive
bool CDetectDVDMedia::IsDiscInDrive()
{
  CSingleLock waitLock(m_muReadingMedia);
  bool bResult = true;
  if ( m_DriveState != DRIVE_CLOSED_MEDIA_PRESENT )
  {
    bResult = false;
  }

  if (g_advancedSettings.m_usePCDVDROM)
  {
    // allow the application to poll once every five seconds
    if ((clock() - m_LastPoll) > 5000)
    {
      // only poll if we're not playing media from the drive
      if (!(g_application.IsPlaying() && g_application.CurrentFileItem().IsOnDVD()))
      {
        CLog::Log(LOGINFO, "Polling PC-DVDROM...");

        m_isoReader.Reset();

        CIoSupport::Dismount("Cdrom0");
        if (CIoSupport::RemapDriveLetter('D', "Cdrom0") == S_OK)
        {
          if (m_pInstance)
          {
            m_pInstance->DetectMediaType();
          }
        }
      }
      m_LastPoll = clock();
    }
  }

  return bResult;
}

// Static function
// Returns a CCdInfo class, which contains
// Media information of the current
// inserted CD.
// Can be NULL
CCdInfo* CDetectDVDMedia::GetCdInfo()
{
  CSingleLock waitLock(m_muReadingMedia);
  CCdInfo* pCdInfo = m_pCdInfo;
  return pCdInfo;
}

const CStdString &CDetectDVDMedia::GetDVDLabel()
{
  return m_diskLabel;
}

const CStdString &CDetectDVDMedia::GetDVDPath()
{
  return m_diskPath;
}
