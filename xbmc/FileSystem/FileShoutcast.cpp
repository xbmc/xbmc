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


// FileShoutcast.cpp: implementation of the CFileShoutcast class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "FileShoutcast.h"
#include "GUISettings.h"
#include "GUIDialogProgress.h"
#include "GUIWindowManager.h"
#include "URL.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

#include "lib/libshout/rip_manager.h"
//#include "lib/libshout/util.h"
#include "lib/libshout/filelib.h"
#include "RingBuffer.h"
#include "ShoutcastRipFile.h"
#include "utils/GUIInfoManager.h"

using namespace std;
using namespace XFILE;
using namespace MUSIC_INFO;

#ifndef HAS_SHOUTCAST
extern "C"
{
  error_code rip_manager_start(void (*status_callback)(int message, void *data), RIP_MANAGER_OPTIONS *options) { return 0; }
  void       rip_manager_stop() { }
  void       set_rip_manager_options_defaults(RIP_MANAGER_OPTIONS*) {}
  int        rip_manager_get_content_type() { return 0; }
}
#endif

const int SHOUTCASTTIMEOUT = 100;
static CRingBuffer m_ringbuf;

static FileState m_fileState;
static CShoutcastRipFile m_ripFile;


static RIP_MANAGER_INFO m_ripInfo;
static ERROR_INFO m_errorInfo;



void rip_callback(int message, void *data)
{
  switch (message)
  {
    RIP_MANAGER_INFO *info;
  case RM_UPDATE:
    info = (RIP_MANAGER_INFO*)data;
    memcpy(&m_ripInfo, info, sizeof(m_ripInfo));
    if (info->status == RM_STATUS_BUFFERING)
    {
      m_fileState.bBuffering = true;
    }
    else if ( info->status == RM_STATUS_RIPPING)
    {
      m_ripFile.SetRipManagerInfo( &m_ripInfo );
      m_fileState.bBuffering = false;
    }
    else if (info->status == RM_STATUS_RECONNECTING)
  { }
    break;
  case RM_ERROR:
    ERROR_INFO *errInfo;
    errInfo = (ERROR_INFO*)data;
    memcpy(&m_errorInfo, errInfo, sizeof(m_errorInfo));
    m_fileState.bRipError = true;
    OutputDebugString("error\n");
    break;
  case RM_DONE:
    OutputDebugString("done\n");
    m_fileState.bRipDone = true;
    break;
  case RM_NEW_TRACK:
    char *trackName;
    trackName = (char*) data;
    m_ripFile.SetTrackname( trackName );
    break;
  case RM_STARTED:
    m_fileState.bRipStarted = true;
    OutputDebugString("Started\n");
    break;
  }

}

extern "C" {
error_code filelib_write_show(char *buf, u_long size)
{
  if ((int)size > m_ringbuf.Size())
  {
    CLog::Log(LOGERROR, "Shoutcast chunk too big: %lu", size);
    return SR_ERROR_BUFFER_FULL;
  }
  while (m_ringbuf.GetMaxWriteSize() < (int)size) Sleep(10);
  m_ringbuf.WriteBinary(buf, size);
  m_ripFile.Write( buf, size ); //will only write, if it has to
  if (m_fileState.bBuffering)
  {
    if (rip_manager_get_content_type() == CONTENT_TYPE_OGG)
    {
      if (m_ringbuf.GetMaxReadSize() > (m_ringbuf.Size() / 8) )
      {
        // hack because ogg streams are very broke, force it to go.
        m_fileState.bBuffering = false;
      }
    }
  }

  return SR_SUCCESS;
}
}

CFileShoutcast* m_pShoutCastRipper = NULL;

CFileShoutcast::CFileShoutcast()
{
  // FIXME: without this check
  // the playback stops when CFile::Stat()
  // or CFile::Exists() is called

  // Do we already have another file
  // using the ripper?
  if (!m_pShoutCastRipper)
  {
    m_fileState.bBuffering = true;
    m_fileState.bRipDone = false;
    m_fileState.bRipStarted = false;
    m_fileState.bRipError = false;
    m_ringbuf.Create(1024*1024); // must be big enough. some stations use 192kbps.
    m_pShoutCastRipper = this;
  }
}

CFileShoutcast::~CFileShoutcast()
{
  // FIXME: without this check
  // the playback stops when CFile::Stat()
  // or CFile::Exists() is called

  // Has this object initialized the ripper?
  if (m_pShoutCastRipper==this)
  {
    rip_manager_stop();
    m_pShoutCastRipper = NULL;
    m_ripFile.Reset();
    m_ringbuf.Destroy();
  }
}

bool CFileShoutcast::CanRecord()
{
  if ( !m_fileState.bRipStarted )
    return false;
  return m_ripFile.CanRecord();
}

bool CFileShoutcast::Record()
{
  return m_ripFile.Record();
}

void CFileShoutcast::StopRecording()
{
  m_ripFile.StopRecording();
}


__int64 CFileShoutcast::GetPosition()
{
  return 0;
}

__int64 CFileShoutcast::GetLength()
{
  return 0;
}


bool CFileShoutcast::Open(const CURL& url)
{
  m_dwLastTime = timeGetTime();
  int ret;

  CGUIDialogProgress* dlgProgress = NULL;
  
  // workaround to avoid deadlocks caused by dvdplayer halting app, only ogg is played by dvdplayer
  if (!url.GetFileType().Equals("ogg") )
  {
    dlgProgress = (CGUIDialogProgress*)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
  }

  set_rip_manager_options_defaults(&m_opt);

  strcpy(m_opt.output_directory, "./");
  m_opt.proxyurl[0] = '\0';

  // Use a proxy, if the GUI was configured as such
  bool bProxyEnabled = g_guiSettings.GetBool("network.usehttpproxy");
  if (bProxyEnabled)
  {
    const CStdString &strProxyServer = g_guiSettings.GetString("network.httpproxyserver");
    const CStdString &strProxyPort = g_guiSettings.GetString("network.httpproxyport");
    // Should we check for valid strings here
#ifndef _LINUX
    _snprintf( m_opt.proxyurl, MAX_URL_LEN, "http://%s:%s", strProxyServer.c_str(), strProxyPort.c_str() );
#else
    snprintf( m_opt.proxyurl, MAX_URL_LEN, "http://%s:%s", strProxyServer.c_str(), strProxyPort.c_str() );
#endif
  }

  CStdString strUrl;
  url.GetURL(strUrl);
  strUrl.Replace("shout://", "http://");
  strncpy(m_opt.url, strUrl.c_str(), MAX_URL_LEN);
  sprintf(m_opt.useragent, "x%s", url.GetFileName().c_str());
  if (dlgProgress)
  {
    dlgProgress->SetHeading(260);
    dlgProgress->SetLine(0, 259);
    dlgProgress->SetLine(1, strUrl);
    dlgProgress->SetLine(2, "");
    if (!dlgProgress->IsDialogRunning())
      dlgProgress->StartModal();
    dlgProgress->Progress();
  }

  if ((ret = rip_manager_start(rip_callback, &m_opt)) != SR_SUCCESS)
  {
    if (dlgProgress) dlgProgress->Close();
    return false;
  }
  int iShoutcastTimeout = 10 * SHOUTCASTTIMEOUT; //i.e: 10 * 10 = 100 * 100ms = 10s
  int iCount = 0;
  while (!m_fileState.bRipDone && !m_fileState.bRipStarted && !m_fileState.bRipError && (!dlgProgress || !dlgProgress->IsCanceled()))
  {
    if (iCount <= iShoutcastTimeout) //Normally, this isn't the problem,
      //because if RIP_MANAGER fails, this would be here
      //with m_fileState.bRipError
    {
      Sleep(100);
    }
    else
    {
      if (dlgProgress)
      {
        dlgProgress->SetLine(1, 257);
        dlgProgress->SetLine(2, "Connection timed out...");
        Sleep(1500);
        dlgProgress->Close();
      }
      return false;
    }
    iCount++;
  }

  if (dlgProgress && dlgProgress->IsCanceled())
  {
     Close();
     dlgProgress->Close();
     return false;
  }

  /* store content type of stream */
  m_contenttype = rip_manager_get_content_type();

  //CHANGED CODE: Don't reset timer anymore.

  while (!m_fileState.bRipDone && !m_fileState.bRipError && m_fileState.bBuffering && (!dlgProgress || !dlgProgress->IsCanceled()))
  {
    if (iCount <= iShoutcastTimeout) //Here is the real problem: Sometimes the buffer fills just to
      //slowly, thus the quality of the stream will be bad, and should be
      //aborted...
    {
      Sleep(100);
      char szTmp[1024];
      //g_dialog.SetCaption(0, "Shoutcast" );
      sprintf(szTmp, "Buffering %i bytes", m_ringbuf.GetMaxReadSize());
      if (dlgProgress)
      {
        dlgProgress->SetLine(2, szTmp );
        dlgProgress->Progress();
      }

      sprintf(szTmp, "%s", m_ripInfo.filename);
      for (int i = 0; i < (int)strlen(szTmp); i++)
        szTmp[i] = tolower((unsigned char)szTmp[i]);
      szTmp[50] = 0;
      if (dlgProgress)
      {
        dlgProgress->SetLine(1, szTmp );
        dlgProgress->Progress();
      }
    }
    else //it's not really a connection timeout, but it's here,
      //where things get boring, if connection is slow.
      //trust me, i did a lot of testing... Doesn't happen often,
      //but if it does it sucks to wait here forever.
      //CHANGED: Other message here
    {
      if (dlgProgress)
      {
        dlgProgress->SetLine(1, 257);
        dlgProgress->SetLine(2, "Connection to server too slow...");
        dlgProgress->Close();
      }
      return false;
    }
    iCount++;
  }
  if (dlgProgress && dlgProgress->IsCanceled())
  {
     Close();
     dlgProgress->Close();
     return false;
  }
  if ( m_fileState.bRipError )
  {
    if (dlgProgress)
    {
      dlgProgress->SetLine(1, 257);
      dlgProgress->SetLine(2, m_errorInfo.error_str);
      dlgProgress->Progress();

      Sleep(1500);
      dlgProgress->Close();
    }
    return false;
  }
  if (dlgProgress)
  {
    dlgProgress->SetLine(2, 261);
    dlgProgress->Progress();
    dlgProgress->Close();
  }
  return true;
}

unsigned int CFileShoutcast::Read(void* lpBuf, __int64 uiBufSize)
{
  if (m_fileState.bRipDone)
  {
    OutputDebugString("Read done\n");
    return 0;
  }
  while (m_ringbuf.GetMaxReadSize() <= 0) Sleep(10);
  int iRead = m_ringbuf.GetMaxReadSize();
  if (iRead > uiBufSize) iRead = (int)uiBufSize;
  m_ringbuf.ReadBinary((char*)lpBuf, iRead);

  if (timeGetTime() - m_dwLastTime > 500)
  {
    m_dwLastTime = timeGetTime();
    CMusicInfoTag tag;
    GetMusicInfoTag(tag);
    g_infoManager.SetCurrentSongTag(tag);
  }
  return iRead;
}

void CFileShoutcast::outputTimeoutMessage(const char* message)
{
  //g_dialog.SetCaption(0, "Shoutcast"  );
  //g_dialog.SetMessage(0,  message );
  //g_dialog.Render();
  Sleep(1500);
}

__int64 CFileShoutcast::Seek(__int64 iFilePosition, int iWhence)
{
  return -1;
}

void CFileShoutcast::Close()
{
  OutputDebugString("Shoutcast Stopping\n");
  if ( m_ripFile.IsRecording() )
    m_ripFile.StopRecording();
  m_ringbuf.Clear();
  rip_manager_stop();
  m_ripFile.Reset();
  OutputDebugString("Shoutcast Stopped\n");
}

bool CFileShoutcast::IsRecording()
{
  return m_ripFile.IsRecording();
}

bool CFileShoutcast::GetMusicInfoTag(CMusicInfoTag& tag)
{
  m_ripFile.GetMusicInfoTag(tag);
  return true;
}

CStdString CFileShoutcast::GetContent()
{
  switch (m_contenttype) 
  { 
    case CONTENT_TYPE_MP3: 
      return "audio/mpeg"; 
    case CONTENT_TYPE_OGG: 
      return "audio/ogg"; 
    case CONTENT_TYPE_AAC: 
      return "audio/aac"; 
    default: 
      return "application/octet-stream";  
  }
}
