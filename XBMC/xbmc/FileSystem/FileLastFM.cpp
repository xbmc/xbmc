#include "../stdafx.h"
#include "FileLastFM.h"
#include "../Util.h"
#include "../utils/GUIInfoManager.h"
#include "../lib/libscrobbler/md5.h"

enum ACTION
{
  ACTION_Idle,
  ACTION_Handshaking,
  ACTION_ChangingStation,
  ACTION_RetreivingMetaData
};
typedef struct FileStateSt
{
  ACTION Action;
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
  
  m_pFile        = NULL;
  m_pSyncBuffer  = new char[6];
  m_bOpened      = false;
  m_bDirectSkip  = false;
  m_hWorkerEvent = CreateEvent(NULL, false, false, NULL);
}

CFileLastFM::~CFileLastFM()
{
  Close();
  CloseHandle(m_hWorkerEvent);
  delete[] m_pSyncBuffer;
  m_pSyncBuffer = NULL;
}

bool CFileLastFM::CanSeek()
{
  return false;
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

  CStdString strPassword = g_guiSettings.GetString("MyMusic.AudioScrobblerPassword");
  CStdString strUserName = g_guiSettings.GetString("MyMusic.AudioScrobblerUserName");
  if (strUserName.IsEmpty() || strPassword.IsEmpty())
  {
    CLog::Log(LOGERROR, "Last.fm stream selected but no username or password set.");
    return false;
  }
  md5_state_t md5state;
  unsigned char md5pword[16];
  md5_init(&md5state);
  md5_append(&md5state, (unsigned const char *)strPassword.c_str(), (int)strPassword.size());
  md5_close(&md5state, md5pword);
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
  CLog::DebugLog("Handshake: %s", html.c_str());

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
  CLog::DebugLog("RTP: %s", html.c_str());
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
  CLog::DebugLog("ChangeStation: %s", html.c_str());

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
  CGUIDialogProgress* dlgProgress = (CGUIDialogProgress*)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS);

  url.GetURL(m_Url);
  CStdString strUrl = m_Url;
  CUtil::UrlDecode(strUrl);

  Create();
  if (dlgProgress)
  {
    dlgProgress->SetHeading("Last.fm");
    dlgProgress->SetLine(0, 259);
    dlgProgress->SetLine(1, strUrl);
    dlgProgress->SetLine(2, "");
    if (!dlgProgress->IsRunning())
      dlgProgress->StartModal(m_gWindowManager.GetActiveWindow());
  }
  
  if (!m_fileState.bHandshakeDone)
  {
    m_fileState.Action      = ACTION_Handshaking;
    m_fileState.bActionDone = false;
    SetEvent(m_hWorkerEvent);
    dlgProgress->SetLine(2, 15251);//Connecting to Last.fm...
    while (!m_fileState.bError && !m_fileState.bActionDone && !dlgProgress->IsCanceled())
    {
      dlgProgress->Progress();
      Sleep(100);
    }
    if (dlgProgress->IsCanceled() || m_fileState.bError)
    {
      if (dlgProgress) dlgProgress->Close();
      Close();
      return false;
    }
  }
  m_fileState.Action      = ACTION_ChangingStation;
  m_fileState.bActionDone = false;
  SetEvent(m_hWorkerEvent);
  dlgProgress->SetLine(2, 15252); // Selecting station...
  while (!m_fileState.bError && !m_fileState.bActionDone && !dlgProgress->IsCanceled())
  {
    dlgProgress->Progress();
    Sleep(100);
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

bool CFileLastFM::OpenStream()
{
  m_pFile = new CFileCurl();
  if (!m_pFile) 
  {
    return false;
  }
  
  m_pFile->SetUserAgent("");
  m_pFile->SetBufferSize(8192);
  
  if (!m_pFile->Open(CURL(m_StreamUrl), true))
  {
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
  fast_memcpy(m_pSyncBuffer + 3, (char*)lpBuf, 3);
  int iSyncPos = -1;
  if ((iSyncPos = SyncReceived(m_pSyncBuffer, 6)) != -1)
  {
    m_bDirectSkip = false;
    memmove(data, data + iSyncPos + 1, read - (iSyncPos + 1));
    read -= (iSyncPos + 1);
    m_fileState.Action = ACTION_RetreivingMetaData;
    SetEvent(m_hWorkerEvent);
  }
  else if ((iSyncPos = SyncReceived(data, read)) != -1)
  {
    if (m_bDirectSkip)
    {
      read -= (iSyncPos + 4);
      memmove(data, data + iSyncPos + 4, read);
      m_bDirectSkip = false;
    }
    else
    {
      memmove(data + iSyncPos, data + iSyncPos + 4, read - (iSyncPos + 4));
      read -= 4;
    }
    m_fileState.Action = ACTION_RetreivingMetaData;
    SetEvent(m_hWorkerEvent);
  }
  //copy last 3 chars of data to first three chars of syncbuffer, might be "SYN"
  fast_memcpy(m_pSyncBuffer, (char*)lpBuf + max(0, read - 3), 3);
  if (m_bDirectSkip)
  {
    //send only silence while skipping track
    memset(lpBuf, 0, read);
  }
  return read;
}

bool CFileLastFM::ReadString(char *szLine, int iLineLength)
{
  return false;
}

__int64 CFileLastFM::Seek(__int64 iFilePosition, int iWhence)
{
  return 0;
}


bool CFileLastFM::SkipNext()
{
  m_bDirectSkip = true;
  CHTTP http;
  CStdString url;
  CStdString html;
  url.Format("http://" + m_BaseUrl + m_BasePath + "/control.php?session=%s&command=skip&debug=%i", m_Session, 0);
  if (!http.Get(url, html)) return false;
  CLog::DebugLog("Skip: %s", html.c_str());
  CStdString value;
  Parameter("response", html, value);
  if (value == "OK")
  {
    return true;
  }
  m_bDirectSkip = false;
  return false;
}

void CFileLastFM::Close()
{
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

  CLog::DebugLog("LastFM closed");
}

bool CFileLastFM::RetreiveMetaData()
{
  DWORD m_dwTime = timeGetTime();

  CHTTP http;
  CMusicInfoTag tag;
  
  CStdString url;
  CStdString html;
  CStdString value;
  CStdString convertedvalue;
  url.Format("http://" + m_BaseUrl + m_BasePath + "/np.php?session=%s&debug=%i", m_Session, 0 );
  if (!http.Get(url, html)) return false;
  CLog::DebugLog("MetaData: %s", html.c_str());
  Parameter("artist", html, value);
  g_charsetConverter.utf8ToStringCharset(value, convertedvalue);
  tag.SetArtist(convertedvalue);
  //CStdString station;
  //Parameter("station", html, station);
  //tag.SetArtist(station + " - " + value);
  Parameter("album", html, value);
  g_charsetConverter.utf8ToStringCharset(value, convertedvalue);
  tag.SetAlbum(convertedvalue);
  Parameter("track", html, value);
  g_charsetConverter.utf8ToStringCharset(value, convertedvalue);
  tag.SetTitle(convertedvalue);
  Parameter("trackduration", html, value);
  tag.SetDuration(atoi(value.c_str()));
  Parameter("albumcover_medium", html, value);
  CStdString coverUrl = value;

  const CMusicInfoTag &currenttag = g_infoManager.GetCurrentSongTag();
  if (
    (currenttag.GetAlbum() != tag.GetAlbum()) ||
    (currenttag.GetArtist() != tag.GetArtist()) ||
    (currenttag.GetTitle() != tag.GetTitle())
    )
  {
    CStdString cachedFile = "";
    if ((coverUrl != "") && (coverUrl.Find("noalbum") == -1))
    {
      CUtil::GetCachedThumbnail(coverUrl, cachedFile);
      if (!CFile::Exists(cachedFile))
      {
        http.Download(coverUrl, cachedFile);
      }
    }
    g_infoManager.SetCurrentAlbumThumb(cachedFile);
    tag.SetLoaded();
    g_infoManager.SetCurrentSongTag(tag);

    //check recordtoprofile, only update if server has wrong setting
    Parameter("recordtoprofile", html, value);
    bool bRTP = g_guiSettings.GetBool("MyMusic.LastFMRecordToProfile");
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
    switch (m_fileState.Action)
    {
    case ACTION_Handshaking:
      if (!HandShake())
        m_fileState.bError = true;
      else
        m_fileState.bActionDone = true;
      m_fileState.Action = ACTION_Idle;
      break;
    case ACTION_ChangingStation:
      if (!ChangeStation(m_Url))
        m_fileState.bError = true;
      else
        m_fileState.bActionDone = true;
      m_fileState.Action = ACTION_Idle;
      break;
    case ACTION_RetreivingMetaData:
      if (!RetreiveMetaData())
      {
        //Sleep(2000);
        //SetEvent(m_hWorkerEvent);
      }
      break;
    }
  }
}

