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
#include "Filesystem/cdiosupport.h"
#include "Filesystem/iso9660.h"
#ifdef HAS_UNDOCUMENTED
#include "xbox/undocumented.h"
#endif
#include "application.h"
#include "util.h"
#include "picture.h"

#ifdef AFTER2_0
#include "utils/LED.h"
#endif


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
#ifdef AFTER2_0
          //On NOT READY MODE! LED State: LED_COLOUR_CYCLE 
          ILED::CLEDControl(g_guiSettings.GetInt("system.ledcolour"));
#endif
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
          CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UPDATE_BOOKMARKS, 0, NULL);
          m_gWindowManager.SendThreadMessage( msg );
          // Do we really need sleep here? This will fix: [ 1530771 ] "Open tray" problem
          // Sleep(6000);
#ifdef AFTER2_0          
          //On NOT READY MODE! LED State: LED_COLOUR_CYCLE 
          ILED::CLEDControl(LED_COLOUR_CYCLE);
#endif
          return ;
        }
        break;

      case DRIVE_READY:
        // drive is ready
        //m_DriveState = DRIVE_READY;
#ifdef AFTER2_0
        //On NOT READY MODE! LED State: LED_COLOUR_CYCLE 
        ILED::CLEDControl(g_guiSettings.GetInt("system.ledcolour"));
#endif
        return ;
        break;
      case DRIVE_CLOSED_NO_MEDIA:
        {
          // nothing in there...
          m_isoReader.Reset();
          m_DriveState = DRIVE_CLOSED_NO_MEDIA;
          SetNewDVDShareUrl("D:\\", false, g_localizeStrings.Get(504));
          // Send Message to GUI that disc has changed
          CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UPDATE_BOOKMARKS, 0, NULL);
          waitLock.Leave();
          m_gWindowManager.SendThreadMessage( msg );
#ifdef AFTER2_0
          //On NOT READY MODE! LED State: LED_COLOUR_CYCLE 
          LED::CLEDControl(g_guiSettings.GetInt("system.ledcolour"));
#endif
          return ;
        }
        break;
      case DRIVE_CLOSED_MEDIA_PRESENT:
        {
          m_DriveState = DRIVE_CLOSED_MEDIA_PRESENT;
          // drive has been closed and is ready
          OutputDebugString("Drive closed media present, remounting...\n");
          m_helper.Remount("D:", "Cdrom0");
          // Detect ISO9660(mode1/mode2) or CDDA filesystem
          DetectMediaType();
          CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UPDATE_BOOKMARKS, 0, NULL);
          waitLock.Leave();
          m_gWindowManager.SendThreadMessage( msg );
          // Tell the application object that a new Cd is inserted
          // So autorun can be started.
          if ( !m_bStartup )
            m_bAutorun = true;
#ifdef AFTER2_0
          ILED::CLEDControl(g_guiSettings.GetInt("system.ledcolour"));
#endif
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

  // find and cache disc thumbnail
  if (IsDiscInDrive() && !bCDDA)
  {
    CStdString strThumb;
    CStdStringArray thumbs;
    StringUtils::SplitString(g_advancedSettings.m_dvdThumbs, "|", thumbs);
    for (unsigned int i = 0; i < thumbs.size(); ++i)
    {
      CUtil::AddFileToFolder(m_diskPath, thumbs[i], strThumb);
      CLog::Log(LOGDEBUG,__FUNCTION__": looking for disc thumb:[%s]", strThumb.c_str());
      if (CFile::Exists(strThumb))
      {
        CLog::Log(LOGDEBUG,__FUNCTION__": found disc thumb:[%s], caching as:[%s]", strThumb.c_str(), strCache.c_str());
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
        CIoSupport helper;
        if (helper.Remount("D:", "Cdrom0") == S_OK)
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