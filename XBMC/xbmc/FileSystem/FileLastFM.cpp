#include "stdafx.h"
#include "FileLastFM.h"
#include "../Util.h"
#include "../utils/GUIInfoManager.h"
#include "../utils/md5.h"
#include "../Application.h"
#include "../BusyIndicator.h"

namespace XFILE
{

enum ACTION
{
  ACTION_Handshaking,
  ACTION_ChangingStation,
  ACTION_RetreivingMetaData,
  ACTION_SkipNext
};
typedef struct FileStateSt
{
  queue<ACTION> ActionQueue;
  bool bActionDone;
  bool bHandshakeDone;
  bool bError;
} FileState;

static FileState m_fileState;

CFileLastFM::CFileLastFM() : CThread()
{
  m_fileState.bHandshakeDone = false;
  m_fileState.bActionDone    = false;
  m_fileState.bError         = false;
  
  m_pFile          = NULL;
  m_pSyncBuffer    = new char[6];
  m_bOpened        = false;
  m_bDirectSkip    = false;
  m_bSkippingTrack = false;
  m_hWorkerEvent   = CreateEvent(NULL, false, false, NULL);
}

CFileLastFM::~CFileLastFM()
{
  Close();
  CloseHandle(m_hWorkerEvent);
  delete[] m_pSyncBuffer;
  m_pSyncBuffer = NULL;
}

__int64 CFileLastFM::GetPosition()
{
  return 0;
}

__int64 CFileLastFM::GetLength()
{
  return 0;
}

void CFileLastFM::Parameter(const CStdString& key, const CStdString& data, CStdString& value)
{
  value = "";
  vector<CStdString> params;
  int iNumItems = StringUtils::SplitString(data, "\n", params);
  for (int i = 0; i < (int)params.size(); i++)
  {
    CStdString tmp = params[i];
    if (int pos = tmp.Find(key) >= 0)
    {
      tmp.Delete(pos - 1, key.GetLength() + 1);
      value = tmp;
      break;
    }
  }
  CLog::Log(LOGDEBUG, "Parameter %s -> %s", key.c_str(), value.c_str());
}

bool CFileLastFM::HandShake()
{
  m_Session = "";
  m_fileState.bHandshakeDone = false;

  CHTTP http;
  CStdString html;

  CStdString strPassword = g_guiSettings.GetString("lastfm.password");
  CStdString strUserName = g_guiSettings.GetString("lastfm.username");
  if (strUserName.IsEmpty() || strPassword.IsEmpty())
  {
    CLog::Log(LOGERROR, "Last.fm stream selected but no username or password set.");
    return false;
  }
  MD5_CTX md5state;
  unsigned char md5pword[16];
  MD5Init(&md5state);
  MD5Update(&md5state, (unsigned char *)strPassword.c_str(), (int)strPassword.size());
  MD5Final(md5pword, &md5state);
  char tmp[33];
  strncpy(tmp, "\0", sizeof(tmp));
  for (int j = 0;j < 16;j++) 
  {
    char a[3];
    sprintf(a, "%02x", md5pword[j]);
    tmp[2*j] = a[0];
    tmp[2*j+1] = a[1];
  }
  CStdString passwordmd5 = tmp;

  CStdString url;
  CUtil::URLEncode(strUserName);
  url.Format("http://ws.audioscrobbler.com/radio/handshake.php?version=%s&platform=%s&username=%s&passwordmd5=%s&debug=%i&partner=%s", "0.1", "xbmc", strUserName, passwordmd5, 0, "");
  if (!http.Get(url, html))
  {
    CLog::Log(LOGERROR, "Connect to Last.fm failed.");
    return false;
  }
  CLog::Log(LOGDEBUG,"Handshake: %s", html.c_str());

  Parameter("session",    html, m_Session);
  Parameter("stream_url", html, m_StreamUrl);
  Parameter("base_url",   html, m_BaseUrl);
  Parameter("base_path",  html, m_BasePath);
  Parameter("subscriber", html, m_Subscriber);
  Parameter("banned",     html, m_Banned);

  if (m_Session == "failed")
  {
    CLog::Log(LOGERROR, "Last.fm return failed response, possible bad username or password?");
    m_Session = "";
    m_fileState.bHandshakeDone = false;
  }
  else 
  {
    m_fileState.bHandshakeDone = true;
  }
  return m_fileState.bHandshakeDone;
}

bool CFileLastFM::RecordToProfile(bool enabled)
{
  CHTTP http;
  CStdString url;
  CStdString html;
  url.Format("http://" + m_BaseUrl + m_BasePath + "/control.php?session=%s&command=%s&debug=%i", m_Session, enabled?"rtp":"nortp", 0);
  if (!http.Get(url, html)) return false;
  CLog::Log(LOGDEBUG,"RTP: %s", html.c_str());
  CStdString value;
  Parameter("response", html, value);
  return value == "OK";
}

bool CFileLastFM::ChangeStation(const CURL& stationUrl)
{
  CStdString strUrl;
  stationUrl.GetURL(strUrl);

  CHTTP http;
  CStdString url;
  CStdString html;
  url.Format("http://" + m_BaseUrl + m_BasePath + "/adjust.php?session=%s&url=%s&debug=%i", m_Session, strUrl, 0);
  if (!http.Get(url, html)) 
  {
    CLog::Log(LOGERROR, "Connect to Last.fm to change station failed.");
    return false;
  }
  CLog::Log(LOGDEBUG,"ChangeStation: %s", html.c_str());

  CStdString strErrorCode;
  Parameter("error", html,  strErrorCode);
  if (strErrorCode != "")
  {
    //int errCode = 
    //if ( errCode > 0 )
    //{
    //    errorCode( errCode );
    //}
    CLog::Log(LOGERROR, "Last.fm returned an error (%s) response for change station request.", strErrorCode.c_str());
    return false;
  }
  return true;
}

bool CFileLastFM::Open(const CURL& url, bool bBinary)
{
  Object = NULL;
  CGUIDialogProgress* dlgProgress = (CGUIDialogProgress*)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS);

  url.GetURL(m_Url);
  m_Url.Replace(" ", "%20");
  CStdString strUrl = m_Url;
  CUtil::UrlDecode(strUrl);

  Create();
  if (dlgProgress)
  {
    dlgProgress->SetHeading("Last.fm");
    dlgProgress->SetLine(0, 259);
    dlgProgress->SetLine(1, strUrl);
    dlgProgress->SetLine(2, "");
    if (!dlgProgress->IsDialogRunning())
      dlgProgress->StartModal();
  }
  
  if (!m_fileState.bHandshakeDone)
  {
    m_fileState.ActionQueue.push(ACTION_Handshaking);
    m_fileState.bActionDone = false;
    SetEvent(m_hWorkerEvent);
    dlgProgress->SetLine(2, 15251);//Connecting to Last.fm...
    while (!m_fileState.bError && !m_fileState.bActionDone && !dlgProgress->IsCanceled())
    {
      dlgProgress->Progress();
      Sleep(1);
    }
    if (dlgProgress->IsCanceled() || m_fileState.bError)
    {
      if (dlgProgress) dlgProgress->Close();
      Close();
      return false;
    }
  }
  m_fileState.ActionQueue.push(ACTION_ChangingStation);
  m_fileState.bActionDone = false;
  SetEvent(m_hWorkerEvent);
  dlgProgress->SetLine(2, 15252); // Selecting station...
  while (!m_fileState.bError && !m_fileState.bActionDone && !dlgProgress->IsCanceled())
  {
    dlgProgress->Progress();
    Sleep(1);
  }
  if (dlgProgress->IsCanceled() || m_fileState.bError)
  {
    if (dlgProgress) dlgProgress->Close();
    Close();
    return false;
  }

  dlgProgress->SetLine(2, 15107); //Buffering...
  if (!OpenStream())
  {
    CLog::Log(LOGERROR, "Last.fm could not open stream.");
    if (dlgProgress) dlgProgress->Close();
    Close();
    return false;
  }
  if (dlgProgress->IsCanceled() || m_fileState.bError)
  {
    if (m_fileState.bError)
      CLog::Log(LOGERROR, "Last.fm streaming failed.");
    if (dlgProgress) dlgProgress->Close();
    Close();
    return false;
  }
  
  if (dlgProgress)
  {
    dlgProgress->SetLine(2, 261);
    dlgProgress->Progress();
    dlgProgress->Close();
  }
  m_bOpened = true;
  return true;
}

bool CFileLastFM::Exists(const CURL& url)
{
  CURL url2(url);
  url2.SetProtocol("http");
  CStdString strURL;
  url2.GetURL(strURL);
  return CFile::Exists(strURL);
}

bool CFileLastFM::OpenStream()
{
  m_pFile = new CFileCurl();
  if (!m_pFile) 
  {
    CLog::Log(LOGERROR, "Last.fm could not create new CFileCurl.");
    return false;
  }
  
  m_pFile->SetUserAgent("");
  m_pFile->SetBufferSize(8192);
  
  if (!m_pFile->Open(CURL(m_StreamUrl), true))
  {
    CLog::Log(LOGERROR, "Last.fm could not open url %s.", m_StreamUrl.c_str());
    delete m_pFile;
    m_pFile = NULL;
    return false;
  }
  return true;
}

int CFileLastFM::SyncReceived(const char* data, unsigned int size)
{
  if (size < 4) return false;

  unsigned int i = 0;
  const char* pSync = data;
  while (i <= size - 4)
  {
    if ( 
      (pSync[0] == 'S') &&
      (pSync[1] == 'Y') && 
      (pSync[2] == 'N') &&
      (pSync[3] == 'C')
      )
    {
      CLog::Log(LOGDEBUG, "Last.fm SYNC found.");
      return i;
    }
    pSync++;
    i++;
  }
  return -1;
}

unsigned int CFileLastFM::Read(void* lpBuf, __int64 uiBufSize)
{
  if (!m_bOpened) return 0;
  unsigned int read = 0;
  unsigned int tms = 0;
  read = m_pFile->Read(lpBuf, uiBufSize);
  if (read == 0) return 0;
  char* data = (char*)lpBuf;
  //copy first 3 chars to syncbuffer, might be "YNC"
  memcpy(m_pSyncBuffer + 3, (char*)lpBuf, 3);
  int iSyncPos = -1;
  if ((iSyncPos = SyncReceived(m_pSyncBuffer, 6)) != -1)
  {
    m_bDirectSkip = false;
    memmove(data, data + iSyncPos + 1, read - (iSyncPos + 1));
    read -= (iSyncPos + 1);
    m_fileState.ActionQueue.push(ACTION_RetreivingMetaData);
    SetEvent(m_hWorkerEvent);
  }
  else if ((iSyncPos = SyncReceived(data, read)) != -1)
  {
    if (m_bDirectSkip)
    {
      read -= (iSyncPos + 4);
      memmove(data, data + iSyncPos + 4, read);
      //clear whatever is in the buffer and continue with the new track after a skip.
      if (Object != NULL)
      {
        CRingHoldBuffer* ringHoldBuffer = (CRingHoldBuffer*)Object;
        ringHoldBuffer->Clear();
      }
      m_bDirectSkip = false;
    }
    else
    {
      memmove(data + iSyncPos, data + iSyncPos + 4, read - (iSyncPos + 4));
      read -= 4;
    }
    m_fileState.ActionQueue.push(ACTION_RetreivingMetaData);
    SetEvent(m_hWorkerEvent);
  }
  //copy last 3 chars of data to first three chars of syncbuffer, might be "SYN"
  memcpy(m_pSyncBuffer, (char*)lpBuf + max((unsigned int)0, read - 3), 3);
  return read;
}

__int64 CFileLastFM::Seek(__int64 iFilePosition, int iWhence)
{
  return -1;
}


bool CFileLastFM::DoSkipNext()
{
  m_bDirectSkip = true;
  g_ApplicationRenderer.SetBusy(true);
  m_bSkippingTrack = true;
  CHTTP http;
  CStdString url;
  CStdString html;
  url.Format("http://" + m_BaseUrl + m_BasePath + "/control.php?session=%s&command=skip&debug=%i", m_Session, 0);
  if (!http.Get(url, html)) return false;
  CLog::Log(LOGDEBUG,"Skip: %s", html.c_str());
  CStdString value;
  Parameter("response", html, value);
  if (value == "OK")
  {
    return true;
  }
  g_ApplicationRenderer.SetBusy(false);
  m_bDirectSkip = false;
  m_bSkippingTrack = false;
  return false;
}

bool CFileLastFM::SkipNext()
{
  if (m_bDirectSkip) return true; //already skipping
  if (m_fileState.ActionQueue.size() == 0)
  {
    m_fileState.ActionQueue.push(ACTION_SkipNext);
    SetEvent(m_hWorkerEvent);
  }
  return true; //only to indicate we handle the skipnext
}

void CFileLastFM::Close()
{
  Object = NULL;
  if (m_ThreadHandle)
  {
    m_bStop = true;
    SetEvent(m_hWorkerEvent);
    StopThread();
  }

  if (m_pFile)
  {
    m_pFile->Close();
    delete m_pFile;
    m_pFile = NULL;
  }
  m_bOpened = false;

  if (m_bSkippingTrack)
  {
    m_bSkippingTrack = false;
    g_ApplicationRenderer.SetBusy(false);
  }
  CLog::Log(LOGDEBUG,"LastFM closed");
}

bool CFileLastFM::RetreiveMetaData()
{
  DWORD m_dwTime = timeGetTime();

  CHTTP http;
  CMusicInfoTag tag;
  
  CStdString url;
  CStdString html;
  CStdString value;
  //CStdString convertedvalue;
  url.Format("http://" + m_BaseUrl + m_BasePath + "/np.php?session=%s&debug=%i", m_Session, 0 );
  if (!http.Get(url, html)) return false;
  CLog::Log(LOGDEBUG,"MetaData: %s", html.c_str());
  Parameter("artist", html, value);
  tag.SetArtist(value);
  //CStdString station;
  //Parameter("station", html, station);
  //tag.SetArtist(station + " - " + value);
  Parameter("album", html, value);
  tag.SetAlbum(value);
  Parameter("track", html, value);
  tag.SetTitle(value);
  Parameter("trackduration", html, value);
  tag.SetDuration(atoi(value.c_str()));
  Parameter("albumcover_medium", html, value);
  CStdString coverUrl = value;

  const CMusicInfoTag* currenttag = g_infoManager.GetCurrentSongTag();
  if (!currenttag ||
    ((currenttag->GetAlbum() != tag.GetAlbum()) ||
    (currenttag->GetArtist() != tag.GetArtist()) ||
    (currenttag->GetTitle() != tag.GetTitle()))
    )
  {
    CStdString cachedFile = "";
    if ((coverUrl != "") && (coverUrl.Find("no_album") == -1) && (coverUrl.Find("noalbum") == -1) && (coverUrl.Right(1) != "/") && (coverUrl.Find(".gif") == -1))
    {
      Crc32 crc;
      crc.ComputeFromLowerCase(coverUrl);
      cachedFile.Format("%s\\%08x.tbn", g_settings.GetLastFMThumbFolder().c_str(), (unsigned __int32) crc);
      if (!CFile::Exists(cachedFile))
      {
        http.Download(coverUrl, cachedFile);
      }
    }
    CSingleLock lock(g_graphicsContext);
    g_infoManager.SetCurrentAlbumThumb(cachedFile);
    tag.SetLoaded();
    g_infoManager.SetCurrentSongTag(tag);
    lock.Leave();

    //inform app a new track has started, and reset our playtime
    CGUIMessage msg(GUI_MSG_PLAYBACK_STARTED, 0, 0, 0, 0, NULL);
    m_gWindowManager.SendThreadMessage(msg);
    g_application.ResetPlayTime();

    //check recordtoprofile, only update if server has wrong setting
    Parameter("recordtoprofile", html, value);
    bool bRTP = g_guiSettings.GetBool("lastfm.recordtoprofile");
    if ((value == "1" && !bRTP) || (value == "0" && bRTP))
      RecordToProfile(bRTP);
    return true;
  }
  return false;
}

void CFileLastFM::Process()
{
  while (!m_bStop)
  {
    WaitForSingleObject(m_hWorkerEvent, INFINITE);
    if (m_bStop)
      break;
    
    ACTION action = m_fileState.ActionQueue.front();
    m_fileState.ActionQueue.pop();
    switch (action)
    {
    case ACTION_Handshaking:
      if (!HandShake())
        m_fileState.bError = true;
      else
        m_fileState.bActionDone = true;
      break;
    case ACTION_ChangingStation:
      if (!ChangeStation(m_Url))
        m_fileState.bError = true;
      else
        m_fileState.bActionDone = true;
      break;
    case ACTION_RetreivingMetaData:
      if (!RetreiveMetaData())
        m_fileState.bError = true;
      else
        m_fileState.bActionDone = true;
      if (m_bSkippingTrack)
      {
        m_bSkippingTrack = false;
        g_ApplicationRenderer.SetBusy(false);
      }
      break;
    case ACTION_SkipNext:
      if (!DoSkipNext())
        m_fileState.bError = true;
      else
        m_fileState.bActionDone = true;
      break;
    }
  }
}

}
