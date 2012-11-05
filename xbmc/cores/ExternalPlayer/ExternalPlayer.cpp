/*
 *      Copyright (C) 2005-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "threads/SystemClock.h"
#include "system.h"
#include "signal.h"
#include "limits.h"
#include "threads/SingleLock.h"
#include "ExternalPlayer.h"
#include "windowing/WindowingFactory.h"
#include "dialogs/GUIDialogOK.h"
#include "guilib/GUIWindowManager.h"
#include "Application.h"
#include "filesystem/MusicDatabaseFile.h"
#include "FileItem.h"
#include "utils/RegExp.h"
#include "utils/StringUtils.h"
#include "URL.h"
#include "utils/XMLUtils.h"
#include "utils/TimeUtils.h"
#include "utils/log.h"
#include "cores/AudioEngine/AEFactory.h"
#if defined(_WIN32)
  #include "Windows.h"
  #ifdef HAS_IRSERVERSUITE
    #include "input/windows/IRServerSuite.h"
  #endif
#endif
#if defined(HAS_LIRC)
  #include "input/linux/LIRC.h"
#endif

// If the process ends in less than this time (ms), we assume it's a launcher
// and wait for manual intervention before continuing
#define LAUNCHER_PROCESS_TIME 2000
// Time (ms) we give a process we sent a WM_QUIT to close before terminating
#define PROCESS_GRACE_TIME 3000
// Default time after which the item's playcount is incremented
#define DEFAULT_PLAYCOUNT_MIN_TIME 10

using namespace XFILE;

#if defined(_WIN32)
extern HWND g_hWnd;
#endif

CExternalPlayer::CExternalPlayer(IPlayerCallback& callback)
    : IPlayer(callback),
      CThread("CExternalPlayer")
{
  m_bAbortRequest = false;
  m_bIsPlaying = false;
  m_paused = false;
  m_playbackStartTime = 0;
  m_speed = 1;
  m_totalTime = 1;
  m_time = 0;

  m_hideconsole = false;
  m_warpcursor = WARP_NONE;
  m_hidexbmc = false;
  m_islauncher = false;
  m_playCountMinTime = DEFAULT_PLAYCOUNT_MIN_TIME;
  m_playOneStackItem = false;

  m_dialog = NULL;

#if defined(_WIN32)
  memset(&m_processInfo, 0, sizeof(m_processInfo));
#endif
}

CExternalPlayer::~CExternalPlayer()
{
  CloseFile();
}

bool CExternalPlayer::OpenFile(const CFileItem& file, const CPlayerOptions &options)
{
  try
  {
    m_bIsPlaying = true;
    m_launchFilename = file.GetPath();
    CLog::Log(LOGNOTICE, "%s: %s", __FUNCTION__, m_launchFilename.c_str());
    Create();

    return true;
  }
  catch(...)
  {
    m_bIsPlaying = false;
    CLog::Log(LOGERROR,"%s - Exception thrown", __FUNCTION__);
    return false;
  }
}

bool CExternalPlayer::CloseFile()
{
  m_bAbortRequest = true;

  if (m_dialog && m_dialog->IsActive()) m_dialog->Close();

#if defined(_WIN32)
  if (m_bIsPlaying && m_processInfo.hProcess)
  {
    TerminateProcess(m_processInfo.hProcess, 1);
  }
#endif

  return true;
}

bool CExternalPlayer::IsPlaying() const
{
  return m_bIsPlaying;
}

void CExternalPlayer::Process()
{
  CStdString mainFile = m_launchFilename;
  CStdString archiveContent = "";

  if (m_args.find("{0}") == std::string::npos)
  {
    // Unwind archive names
    CURL url(m_launchFilename);
    CStdString protocol = url.GetProtocol();
    if (protocol == "zip" || protocol == "rar"/* || protocol == "iso9660" ??*/)
    {
      mainFile = url.GetHostName();
      archiveContent = url.GetFileName();
    }
    if (protocol == "musicdb")
      mainFile = CMusicDatabaseFile::TranslateUrl(url);
  }

  if (m_filenameReplacers.size() > 0)
  {
    for (unsigned int i = 0; i < m_filenameReplacers.size(); i++)
    {
      std::vector<CStdString> vecSplit;
      StringUtils::SplitString(m_filenameReplacers[i], " , ", vecSplit);

      // something is wrong, go to next substitution
      if (vecSplit.size() != 4)
        continue;

      CStdString strMatch = vecSplit[0];
      strMatch.Replace(",,",",");
      bool bCaseless = vecSplit[3].Find('i') > -1;
      CRegExp regExp(bCaseless);

      if (!regExp.RegComp(strMatch.c_str()))
      { // invalid regexp - complain in logs
        CLog::Log(LOGERROR, "%s: Invalid RegExp:'%s'", __FUNCTION__, strMatch.c_str());
        continue;
      }

      if (regExp.RegFind(mainFile) > -1)
      {
        CStdString strPat = vecSplit[1];
        strPat.Replace(",,",",");

        if (!regExp.RegComp(strPat.c_str()))
        { // invalid regexp - complain in logs
          CLog::Log(LOGERROR, "%s: Invalid RegExp:'%s'", __FUNCTION__, strPat.c_str());
          continue;
        }

        CStdString strRep = vecSplit[2];
        strRep.Replace(",,",",");
        bool bGlobal = vecSplit[3].Find('g') > -1;
        bool bStop = vecSplit[3].Find('s') > -1;
        int iStart = 0;
        while ((iStart = regExp.RegFind(mainFile, iStart)) > -1)
        {
          int iLength = regExp.GetFindLen();
          mainFile = mainFile.Left(iStart) + regExp.GetReplaceString(strRep.c_str()).c_str() + mainFile.Mid(iStart+iLength);
          if (!bGlobal)
            break;
        }
        CLog::Log(LOGINFO, "%s: File matched:'%s' (RE='%s',Rep='%s') new filename:'%s'.", __FUNCTION__, strMatch.c_str(), strPat.c_str(), strRep.c_str(), mainFile.c_str());
        if (bStop) break;
      }
    }
  }

  CLog::Log(LOGNOTICE, "%s: Player : %s", __FUNCTION__, m_filename.c_str());
  CLog::Log(LOGNOTICE, "%s: File   : %s", __FUNCTION__, mainFile.c_str());
  CLog::Log(LOGNOTICE, "%s: Content: %s", __FUNCTION__, archiveContent.c_str());
  CLog::Log(LOGNOTICE, "%s: Args   : %s", __FUNCTION__, m_args.c_str());
  CLog::Log(LOGNOTICE, "%s: Start", __FUNCTION__);

  // make sure we surround the arguments with quotes where necessary
  CStdString strFName;
  CStdString strFArgs;
#if defined(_WIN32)
  // W32 batch-file handline
  if (m_filename.Right(4) == ".bat" || m_filename.Right(4) == ".cmd")
  {
    // MSDN says you just need to do this, but cmd's handing of spaces and
    // quotes is soo broken it seems to work much better if you just omit
    // lpApplicationName and enclose the module in lpCommandLine in quotes
    //strFName = "cmd.exe";
    //strFArgs = "/c ";
  }
  else
#endif
    strFName = m_filename;

  strFArgs.append("\"");
  strFArgs.append(m_filename);
  strFArgs.append("\" ");
  strFArgs.append(m_args);

  int nReplaced = strFArgs.Replace("{0}", mainFile);

  if (!nReplaced)
    nReplaced = strFArgs.Replace("{1}", mainFile) + strFArgs.Replace("{2}", archiveContent);

  if (!nReplaced)
  {
    strFArgs.append(" \"");
    strFArgs.append(mainFile);
    strFArgs.append("\"");
  }

#if defined(_WIN32)
  if (m_warpcursor)
  {
    GetCursorPos(&m_ptCursorpos);
    int x = 0;
    int y = 0;
    switch (m_warpcursor)
    {
      case WARP_BOTTOM_RIGHT:
        x = GetSystemMetrics(SM_CXSCREEN);
      case WARP_BOTTOM_LEFT:
        y = GetSystemMetrics(SM_CYSCREEN);
        break;
      case WARP_TOP_RIGHT:
        x = GetSystemMetrics(SM_CXSCREEN);
        break;
      case WARP_CENTER:
        x = GetSystemMetrics(SM_CXSCREEN) / 2;
        y = GetSystemMetrics(SM_CYSCREEN) / 2;
        break;
    }
    CLog::Log(LOGNOTICE, "%s: Warping cursor to (%d,%d)", __FUNCTION__, x, y);
    SetCursorPos(x,y);
  }

  LONG currentStyle = GetWindowLong(g_hWnd, GWL_EXSTYLE);
#endif

  if (m_hidexbmc && !m_islauncher)
  {
    CLog::Log(LOGNOTICE, "%s: Hiding XBMC window", __FUNCTION__);
    g_Windowing.Hide();
  }
#if defined(_WIN32)
  else if (currentStyle & WS_EX_TOPMOST)
  {
    CLog::Log(LOGNOTICE, "%s: Lowering XBMC window", __FUNCTION__);
    SetWindowPos(g_hWnd,HWND_BOTTOM,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOREDRAW);
  }

  CLog::Log(LOGDEBUG, "%s: Unlocking foreground window", __FUNCTION__);
  LockSetForegroundWindow(LSFW_UNLOCK);
#endif

  m_playbackStartTime = XbmcThreads::SystemClockMillis();

  /* Suspend AE temporarily so exclusive or hog-mode sinks */
  /* don't block external player's access to audio device  */
  if (!CAEFactory::Suspend())
  {
    CLog::Log(LOGNOTICE,"%s: Failed to suspend AudioEngine before launching external player", __FUNCTION__);
  }


  BOOL ret = TRUE;
#if defined(_WIN32)
  ret = ExecuteAppW32(strFName.c_str(),strFArgs.c_str());
#elif defined(_LINUX) || defined(TARGET_DARWIN_OSX)
  ret = ExecuteAppLinux(strFArgs.c_str());
#endif
  int64_t elapsedMillis = XbmcThreads::SystemClockMillis() - m_playbackStartTime;

  if (ret && (m_islauncher || elapsedMillis < LAUNCHER_PROCESS_TIME))
  {
    if (m_hidexbmc)
    {
      CLog::Log(LOGNOTICE, "%s: XBMC cannot stay hidden for a launcher process", __FUNCTION__);
      g_Windowing.Show(false);
    }

    {
      CSingleLock lock(g_graphicsContext);
      m_dialog = (CGUIDialogOK *)g_windowManager.GetWindow(WINDOW_DIALOG_OK);
      m_dialog->SetHeading(23100);
      m_dialog->SetLine(1, 23104);
      m_dialog->SetLine(2, 23105);
      m_dialog->SetLine(3, 23106);
    }

    if (!m_bAbortRequest) m_dialog->DoModal();
  }

  m_bIsPlaying = false;
  CLog::Log(LOGNOTICE, "%s: Stop", __FUNCTION__);

#if defined(_WIN32)
  g_Windowing.Restore();

  if (currentStyle & WS_EX_TOPMOST)
  {
    CLog::Log(LOGNOTICE, "%s: Showing XBMC window TOPMOST", __FUNCTION__);
    SetWindowPos(g_hWnd,HWND_TOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_SHOWWINDOW);
    SetForegroundWindow(g_hWnd);
  }
  else
#endif
  {
    CLog::Log(LOGNOTICE, "%s: Showing XBMC window", __FUNCTION__);
    g_Windowing.Show();
  }

#if defined(_WIN32)
  if (m_warpcursor)
  {
    m_xPos = 0;
    m_yPos = 0;
    if (&m_ptCursorpos != 0)
    {
      m_xPos = (m_ptCursorpos.x);
      m_yPos = (m_ptCursorpos.y);
    }
    CLog::Log(LOGNOTICE, "%s: Restoring cursor to (%d,%d)", __FUNCTION__, m_xPos, m_yPos);
    SetCursorPos(m_xPos,m_yPos);
  }
#endif

  /* Resume AE processing of XBMC native audio */
  if (!CAEFactory::Resume())
  {
    CLog::Log(LOGFATAL, "%s: Failed to restart AudioEngine after return from external player",__FUNCTION__);
  }

  // We don't want to come back to an active screensaver
  g_application.ResetScreenSaver();
  g_application.WakeUpScreenSaverAndDPMS();

  if (!ret || (m_playOneStackItem && g_application.CurrentFileItem().IsStack()))
    m_callback.OnPlayBackStopped();
  else
    m_callback.OnPlayBackEnded();
}

#if defined(_WIN32)
BOOL CExternalPlayer::ExecuteAppW32(const char* strPath, const char* strSwitches)
{
  CLog::Log(LOGNOTICE, "%s: %s %s", __FUNCTION__, strPath, strSwitches);

  STARTUPINFOW si;
  memset(&si, 0, sizeof(si));
  si.cb = sizeof(si);
  si.dwFlags = STARTF_USESHOWWINDOW;
  si.wShowWindow = m_hideconsole ? SW_HIDE : SW_SHOW;

  CStdStringW WstrPath, WstrSwitches;
  g_charsetConverter.utf8ToW(strPath, WstrPath);
  g_charsetConverter.utf8ToW(strSwitches, WstrSwitches);

  if (m_bAbortRequest) return false;

  BOOL ret = CreateProcessW(WstrPath.IsEmpty() ? NULL : WstrPath.c_str(),
                            (LPWSTR) WstrSwitches.c_str(), NULL, NULL, FALSE, NULL,
                            NULL, NULL, &si, &m_processInfo);

  if (ret == FALSE)
  {
    DWORD lastError = GetLastError();
    CLog::Log(LOGNOTICE, "%s - Failure: %d", __FUNCTION__, lastError);
  }
  else
  {
    int res = WaitForSingleObject(m_processInfo.hProcess, INFINITE);

    switch (res)
    {
      case WAIT_OBJECT_0:
        CLog::Log(LOGNOTICE, "%s: WAIT_OBJECT_0", __FUNCTION__);
        break;
      case WAIT_ABANDONED:
        CLog::Log(LOGNOTICE, "%s: WAIT_ABANDONED", __FUNCTION__);
        break;
      case WAIT_TIMEOUT:
        CLog::Log(LOGNOTICE, "%s: WAIT_TIMEOUT", __FUNCTION__);
        break;
      case WAIT_FAILED:
        CLog::Log(LOGNOTICE, "%s: WAIT_FAILED (%d)", __FUNCTION__, GetLastError());
        ret = FALSE;
        break;
    }

    CloseHandle(m_processInfo.hThread);
    m_processInfo.hThread = 0;
    CloseHandle(m_processInfo.hProcess);
    m_processInfo.hProcess = 0;
  }

  return ret;
}
#endif

#if defined(_LINUX) || defined(TARGET_DARWIN_OSX)
BOOL CExternalPlayer::ExecuteAppLinux(const char* strSwitches)
{
  CLog::Log(LOGNOTICE, "%s: %s", __FUNCTION__, strSwitches);
#ifdef HAS_LIRC
  bool remoteused = g_RemoteControl.IsInUse();
  g_RemoteControl.Disconnect();
  g_RemoteControl.setUsed(false);
#endif

  int ret = system(strSwitches);

#ifdef HAS_LIRC
  g_RemoteControl.setUsed(remoteused);
  g_RemoteControl.Initialize();
#endif

  if (ret != 0)
  {
    CLog::Log(LOGNOTICE, "%s: Failure: %d", __FUNCTION__, ret);
  }

  return ret == 0;
}
#endif

void CExternalPlayer::Pause()
{
}

bool CExternalPlayer::IsPaused() const
{
  return false;
}

bool CExternalPlayer::HasVideo() const
{
  return true;
}

bool CExternalPlayer::HasAudio() const
{
  return false;
}

void CExternalPlayer::SwitchToNextLanguage()
{
}

void CExternalPlayer::ToggleSubtitles()
{
}

bool CExternalPlayer::CanSeek()
{
  return false;
}

void CExternalPlayer::Seek(bool bPlus, bool bLargeStep)
{
}

void CExternalPlayer::GetAudioInfo(CStdString& strAudioInfo)
{
  strAudioInfo = "CExternalPlayer:GetAudioInfo";
}

void CExternalPlayer::GetVideoInfo(CStdString& strVideoInfo)
{
  strVideoInfo = "CExternalPlayer:GetVideoInfo";
}

void CExternalPlayer::GetGeneralInfo(CStdString& strGeneralInfo)
{
  strGeneralInfo = "CExternalPlayer:GetGeneralInfo";
}

void CExternalPlayer::SwitchToNextAudioLanguage()
{
}

void CExternalPlayer::SeekPercentage(float iPercent)
{
}

float CExternalPlayer::GetPercentage()
{
  int64_t iTime = GetTime();
  int64_t iTotalTime = GetTotalTime();

  if (iTotalTime != 0)
  {
    CLog::Log(LOGDEBUG, "Percentage is %f", (iTime * 100 / (float)iTotalTime));
    return iTime * 100 / (float)iTotalTime;
  }

  return 0.0f;
}

void CExternalPlayer::SetAVDelay(float fValue)
{
}

float CExternalPlayer::GetAVDelay()
{
  return 0.0f;
}

void CExternalPlayer::SetSubTitleDelay(float fValue)
{
}

float CExternalPlayer::GetSubTitleDelay()
{
  return 0.0;
}

void CExternalPlayer::SeekTime(int64_t iTime)
{
}

int64_t CExternalPlayer::GetTime() // in millis
{
  if ((XbmcThreads::SystemClockMillis() - m_playbackStartTime) / 1000 > m_playCountMinTime)
  {
    m_time = m_totalTime * 1000;
  }

  return m_time;
}

int64_t CExternalPlayer::GetTotalTime() // in milliseconds
{
  return (int64_t)m_totalTime * 1000;
}

void CExternalPlayer::ToFFRW(int iSpeed)
{
  m_speed = iSpeed;
}

void CExternalPlayer::ShowOSD(bool bOnoff)
{
}

CStdString CExternalPlayer::GetPlayerState()
{
  return "";
}

bool CExternalPlayer::SetPlayerState(CStdString state)
{
  return true;
}

bool CExternalPlayer::Initialize(TiXmlElement* pConfig)
{
  XMLUtils::GetString(pConfig, "filename", m_filename);
  if (m_filename.length() > 0)
  {
    CLog::Log(LOGNOTICE, "ExternalPlayer Filename: %s", m_filename.c_str());
  }
  else
  {
    CStdString xml;
    xml<<*pConfig;
    CLog::Log(LOGERROR, "ExternalPlayer Error: filename element missing from: %s", xml.c_str());
    return false;
  }

  XMLUtils::GetString(pConfig, "args", m_args);
  XMLUtils::GetBoolean(pConfig, "playonestackitem", m_playOneStackItem);
  XMLUtils::GetBoolean(pConfig, "islauncher", m_islauncher);
  XMLUtils::GetBoolean(pConfig, "hidexbmc", m_hidexbmc);
  if (!XMLUtils::GetBoolean(pConfig, "hideconsole", m_hideconsole))
  {
#ifdef _WIN32
    // Default depends on whether player is a batch file
    m_hideconsole = m_filename.Right(4) == ".bat";
#endif
  }

  bool bHideCursor;
  if (XMLUtils::GetBoolean(pConfig, "hidecursor", bHideCursor) && bHideCursor)
    m_warpcursor = WARP_BOTTOM_RIGHT;

  CStdString warpCursor;
  if (XMLUtils::GetString(pConfig, "warpcursor", warpCursor))
  {
    if (warpCursor == "bottomright") m_warpcursor = WARP_BOTTOM_RIGHT;
    else if (warpCursor == "bottomleft") m_warpcursor = WARP_BOTTOM_LEFT;
    else if (warpCursor == "topleft") m_warpcursor = WARP_TOP_LEFT;
    else if (warpCursor == "topright") m_warpcursor = WARP_TOP_RIGHT;
    else if (warpCursor == "center") m_warpcursor = WARP_CENTER;
    else
    {
      warpCursor = "none";
      CLog::Log(LOGWARNING, "ExternalPlayer: invalid value for warpcursor: %s", warpCursor.c_str());
    }
  }

  XMLUtils::GetInt(pConfig, "playcountminimumtime", m_playCountMinTime, 1, INT_MAX);

  CLog::Log(LOGNOTICE, "ExternalPlayer Tweaks: hideconsole (%s), hidexbmc (%s), islauncher (%s), warpcursor (%s)",
          m_hideconsole ? "true" : "false",
          m_hidexbmc ? "true" : "false",
          m_islauncher ? "true" : "false",
          warpCursor.c_str());

#ifdef _WIN32
  m_filenameReplacers.push_back("^smb:// , / , \\\\ , g");
  m_filenameReplacers.push_back("^smb:\\\\\\\\ , smb:(\\\\\\\\[^\\\\]*\\\\) , \\1 , ");
#endif

  TiXmlElement* pReplacers = pConfig->FirstChildElement("replacers");
  while (pReplacers)
  {
    GetCustomRegexpReplacers(pReplacers, m_filenameReplacers);
    pReplacers = pReplacers->NextSiblingElement("replacers");
  }

  return true;
}

void CExternalPlayer::GetCustomRegexpReplacers(TiXmlElement *pRootElement,
                                               CStdStringArray& settings)
{
  int iAction = 0; // overwrite
  // for backward compatibility
  const char* szAppend = pRootElement->Attribute("append");
  if ((szAppend && stricmp(szAppend, "yes") == 0))
    iAction = 1;
  // action takes precedence if both attributes exist
  const char* szAction = pRootElement->Attribute("action");
  if (szAction)
  {
    iAction = 0; // overwrite
    if (stricmp(szAction, "append") == 0)
      iAction = 1; // append
    else if (stricmp(szAction, "prepend") == 0)
      iAction = 2; // prepend
  }
  if (iAction == 0)
    settings.clear();

  TiXmlElement* pReplacer = pRootElement->FirstChildElement("replacer");
  int i = 0;
  while (pReplacer)
  {
    if (pReplacer->FirstChild())
    {
      const char* szGlobal = pReplacer->Attribute("global");
      const char* szStop = pReplacer->Attribute("stop");
      bool bGlobal = szGlobal && stricmp(szGlobal, "true") == 0;
      bool bStop = szStop && stricmp(szStop, "true") == 0;

      CStdString strMatch;
      CStdString strPat;
      CStdString strRep;
      XMLUtils::GetString(pReplacer,"match",strMatch);
      XMLUtils::GetString(pReplacer,"pat",strPat);
      XMLUtils::GetString(pReplacer,"rep",strRep);

      if (!strPat.IsEmpty() && !strRep.IsEmpty())
      {
        CLog::Log(LOGDEBUG,"  Registering replacer:");
        CLog::Log(LOGDEBUG,"    Match:[%s] Pattern:[%s] Replacement:[%s]", strMatch.c_str(), strPat.c_str(), strRep.c_str());
        CLog::Log(LOGDEBUG,"    Global:[%s] Stop:[%s]", bGlobal?"true":"false", bStop?"true":"false");
        // keep literal commas since we use comma as a seperator
        strMatch.Replace(",",",,");
        strPat.Replace(",",",,");
        strRep.Replace(",",",,");

        CStdString strReplacer = strMatch + " , " + strPat + " , " + strRep + " , " + (bGlobal ? "g" : "") + (bStop ? "s" : "");
        if (iAction == 2)
          settings.insert(settings.begin() + i++, 1, strReplacer);
        else
          settings.push_back(strReplacer);
      }
      else
      {
        // error message about missing tag
        if (strPat.IsEmpty())
          CLog::Log(LOGERROR,"  Missing <Pat> tag");
        else
          CLog::Log(LOGERROR,"  Missing <Rep> tag");
      }
    }

    pReplacer = pReplacer->NextSiblingElement("replacer");
  }
}
