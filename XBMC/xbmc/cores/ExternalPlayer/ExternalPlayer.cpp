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
 
#include "stdafx.h"
#include "AudioContext.h"
#include "ExternalPlayer.h"
#include "GUIDialogOK.h"
#include "GUIFontManager.h"
#include "GUITextLayout.h"
#include "GUIWindowManager.h"
#include "Application.h"
#include "Settings.h"
#include "FileItem.h"
#include "RegExp.h"
#if defined(_WIN32PC)
  #include "Windows.h"
#endif
#if defined(HAS_LIRC)
  #include "common/LIRC.h"
#endif

// If the process ends in less than this time (ms), we assume it's a launcher
// and wait for manual intervention before continuing
#define LAUNCHER_PROCESS_TIME 2000
// Time (ms) we give a process we sent a WM_QUIT to close before terminating
#define PROCESS_GRACE_TIME 3000

CExternalPlayer::CExternalPlayer(IPlayerCallback& callback)
    : IPlayer(callback),
      CThread()
{
  m_bIsPlaying = false;
  m_paused = false;
  m_playbackStartTime = 0;
  m_speed = 1;
  m_totalTime = 1000;
  m_time = 0;

  m_hideconsole = false;
  m_warpcursor = WARP_NONE;
  m_hidexbmc = false;
  m_islauncher = false;

  m_dialog = NULL;
}

CExternalPlayer::~CExternalPlayer()
{
  CloseFile();
}

bool CExternalPlayer::OpenFile(const CFileItem& file, const CPlayerOptions &options)
{
  try
  {
    m_launchFilename = file.m_strPath;
    CLog::Log(LOGNOTICE, "%s: %s", __FUNCTION__, m_launchFilename.c_str());

    Create();

    return true;
  }
  catch(...)
  {
    CLog::Log(LOGERROR,"%s - Exception thrown", __FUNCTION__);
    return false;
  }
}

bool CExternalPlayer::CloseFile()
{
  StopThread();
  return true;
}

bool CExternalPlayer::IsPlaying() const
{
  return m_bIsPlaying;
}

void CExternalPlayer::Process()
{
  SetName("CExternalPlayer");

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
          mainFile = mainFile.Left(iStart) + regExp.GetReplaceString(strRep.c_str()) + mainFile.Mid(iStart+iLength);
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
#if defined(_WIN32PC)
  // W32 batch-file handline
  if (m_filename.Right(4) == ".bat")
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

  int iActiveDevice = g_audioContext.GetActiveDevice();
  if (iActiveDevice != CAudioContext::NONE)
  {
    CLog::Log(LOGNOTICE, "%s: Releasing audio device %d", __FUNCTION__, iActiveDevice);
    g_audioContext.SetActiveDevice(CAudioContext::NONE);
  }

  m_bIsPlaying = true;

#if defined(_WIN32PC)
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
  
  m_hwndXbmc = GetForegroundWindow();

  LONG currentStyle;
  if (m_hwndXbmc)
  {
    currentStyle = GetWindowLong(m_hwndXbmc, GWL_EXSTYLE);

    if (m_hidexbmc && !m_islauncher)
    {
      CLog::Log(LOGNOTICE, "%s: Hiding XBMC window", __FUNCTION__);
      ShowWindow(m_hwndXbmc,SW_HIDE);
    }
    else if (currentStyle & WS_EX_TOPMOST)
    {
      CLog::Log(LOGNOTICE, "%s: Lowering XBMC window", __FUNCTION__);
      SetWindowPos(m_hwndXbmc,HWND_BOTTOM,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOREDRAW);
    }
  }

  CLog::Log(LOGDEBUG, "%s: Unlocking foreground window", __FUNCTION__);
  LockSetForegroundWindow(LSFW_UNLOCK);
#endif

  m_playbackStartTime = timeGetTime();

  BOOL ret = TRUE;
#if defined(_WIN32PC)
  ret = ExecuteAppW32(strFName.c_str(),strFArgs.c_str());
#elif defined(_LINUX)
  ret = ExecuteAppLinux(strFArgs.c_str());
#endif

  m_time = m_totalTime;
  m_bIsPlaying = false;
  CLog::Log(LOGNOTICE, "%s: Stop", __FUNCTION__);

#if defined(_WIN32PC)
  if (m_hwndXbmc)
  {
    if (currentStyle & WS_EX_TOPMOST)
    {
      CLog::Log(LOGNOTICE, "%s: Showing XBMC window TOPMOST", __FUNCTION__);
      SetWindowPos(m_hwndXbmc,HWND_TOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_SHOWWINDOW);
    }
    else
    {
      CLog::Log(LOGNOTICE, "%s: Showing XBMC window TOP", __FUNCTION__);
      SetWindowPos(m_hwndXbmc,HWND_TOP,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_SHOWWINDOW);
    }
  }

  CLog::Log(LOGDEBUG, "%s: Focus XBMC window and lock foreground window", __FUNCTION__);
  SetFocus(m_hwndXbmc);
  SetForegroundWindow(m_hwndXbmc);
  LockSetForegroundWindow(LSFW_LOCK);

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

  // We don't want to come back to an active screensaver
  g_application.ResetScreenSaver();
  g_application.WakeUpScreenSaverAndDPMS();

  if (iActiveDevice != CAudioContext::NONE)
  {
    CLog::Log(LOGNOTICE, "%s: Reclaiming audio device %d", __FUNCTION__, iActiveDevice);
    g_audioContext.SetActiveDevice(iActiveDevice);
  }

  if (ret) 
    m_callback.OnPlayBackEnded();
  else
    m_callback.OnPlayBackStopped();
}

#if defined(_WIN32PC)
BOOL CExternalPlayer::ExecuteAppW32(const char* strPath, const char* strSwitches)
{
  CLog::Log(LOGNOTICE, "%s: %s %s", __FUNCTION__, strPath, strSwitches);

  STARTUPINFOW si;
  PROCESS_INFORMATION pi;
  memset(&si, 0, sizeof(si));
  memset(&pi, 0, sizeof(pi));
  si.cb = sizeof(si);
  si.dwFlags = STARTF_USESHOWWINDOW;
  si.wShowWindow = m_hideconsole ? SW_HIDE : SW_SHOW;

  CStdStringW WstrPath, WstrSwitches;
  g_charsetConverter.utf8ToW(strPath, WstrPath);
  g_charsetConverter.utf8ToW(strSwitches, WstrSwitches);

  m_dialog = (CGUIDialogOK *)m_gWindowManager.GetWindow(WINDOW_DIALOG_OK);
  m_dialog->SetHeading(23100);

  BOOL ret = CreateProcessW(WstrPath.IsEmpty() ? NULL : WstrPath.c_str(),
                            (LPWSTR) WstrSwitches.c_str(), NULL, NULL, FALSE, NULL, 
                            NULL, NULL, &si, &pi);

  if (ret == FALSE)
  {
    DWORD lastError = GetLastError(); 
    CLog::Log(LOGNOTICE, "%s - Failure: %d", __FUNCTION__, lastError);
  }
  else
  {
    m_playbackStartTime = timeGetTime();
    BOOL bIsLauncher = m_islauncher;

    if (m_hidexbmc && !m_islauncher)
    {
      int res = WaitForSingleObject(pi.hProcess, INFINITE);
      if (timeGetTime() - m_playbackStartTime < LAUNCHER_PROCESS_TIME)
      {
        CLog::Log(LOGNOTICE, "%s: External player process ended too quickly, assuming it's a launcher process", __FUNCTION__);
        bIsLauncher = true;

        if (m_hwndXbmc && m_hidexbmc)
        {
          CLog::Log(LOGNOTICE, "%s: XBMC cannot stay hidden for a launcher process", __FUNCTION__);
          SetWindowPos(m_hwndXbmc,HWND_BOTTOM,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_SHOWWINDOW);
        }
      }
    }

    if (!m_hidexbmc || bIsLauncher)
    {
      BOOL waiting = false;
      HANDLE hWait;
      if (!bIsLauncher)
      {
        waiting = RegisterWaitForSingleObject(&hWait, pi.hProcess, AppFinished, this, INFINITE, WT_EXECUTEDEFAULT|WT_EXECUTEONLYONCE);

        if (!waiting) 
          CLog::Log(LOGERROR,"%s: Failed to register wait callback (%d)", __FUNCTION__, GetLastError());
      }

      if (waiting)
      {
        m_dialog->SetLine(1, 23101);
        m_dialog->SetLine(2, 23102);
        m_dialog->SetLine(3, 23103);
      }
      else
      {
        m_dialog->SetLine(1, 23104);
        m_dialog->SetLine(2, 23105);
        m_dialog->SetLine(3, 23106);
      }

      // Carry on if we failed to register the wait callback,
      // the user will have to manually dismiss the dialog
      m_dialog->DoModal();

      if (waiting)
        UnregisterWait(hWait);

      if (m_dialog->IsConfirmed())
        // Post a message telling the player to stop
        PostThreadMessage(pi.dwThreadId, WM_QUIT, 0, 0);
    }

    // In case we've just posted a WM_QUIT we'll give the process a short while
    // to comply before using TerminateProcess
    int res = WaitForSingleObject(pi.hProcess, PROCESS_GRACE_TIME);
    switch (res)
    {
      case WAIT_OBJECT_0:
        CLog::Log(LOGNOTICE, "%s: WAIT_OBJECT_0", __FUNCTION__);
        break;
      case WAIT_ABANDONED:
        CLog::Log(LOGNOTICE, "%s: WAIT_ABANDONED", __FUNCTION__);
        break;
      case WAIT_TIMEOUT:
        CLog::Log(LOGNOTICE, "%s: WAIT_TIMEOUT, terminating process", __FUNCTION__);
        // The player hasn't stopped gracefully, kill it
        TerminateProcess(pi.hProcess, 1);
        break;
      case WAIT_FAILED:
        CLog::Log(LOGNOTICE, "%s: WAIT_FAILED (%d), terminating process", __FUNCTION__, GetLastError());
        // The wait failed, give the player a few seconds to stop gracefully before killing it
        Sleep(PROCESS_GRACE_TIME);
        TerminateProcess(pi.hProcess, 1);
        break;
    }
    // else the process exited normally and we closed the dialog programatically

    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
  }

  return ret;
}

void CALLBACK CExternalPlayer::AppFinished(void* closure, BOOLEAN TimerOrWaitFired)
{
  CExternalPlayer *player = (CExternalPlayer *)closure;
  if (timeGetTime() - player->m_playbackStartTime < LAUNCHER_PROCESS_TIME)
  {
    CLog::Log(LOGNOTICE, "%s: Process ran for <%dms, probably a launcher, not closing dialog", __FUNCTION__, LAUNCHER_PROCESS_TIME);
  }
  else
  {
    CLog::Log(LOGNOTICE, "%s: Process ended, closing dialog", __FUNCTION__);
    if (player->m_dialog->IsActive())
      player->m_dialog->Close();
  }
}
#endif

#if defined(_LINUX)
BOOL CExternalPlayer::ExecuteAppLinux(const char* strSwitches)
{
  CLog::Log(LOGNOTICE, "%s: %s", __FUNCTION__, strSwitches);
#ifdef HAS_LIRC
  g_RemoteControl.Disconnect();
#endif
  g_graphicsContext.Lock();

  int ret = system(strSwitches);

  g_graphicsContext.Unlock();
#ifdef HAS_LIRC
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
  return (m_totalTime * 100.0f) / m_time;
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

void CExternalPlayer::SeekTime(__int64 iTime)
{
}

__int64 CExternalPlayer::GetTime()
{
  return m_time;
}

int CExternalPlayer::GetTotalTime()
{
  return m_totalTime;
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
  XMLUtils::GetBoolean(pConfig, "islauncher", m_islauncher);
  XMLUtils::GetBoolean(pConfig, "hidexbmc", m_hidexbmc);
  if (!XMLUtils::GetBoolean(pConfig, "hideconsole", m_hideconsole))
  {
#ifdef _WIN32PC
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

  CLog::Log(LOGNOTICE, "ExternalPlayer Tweaks: hideconsole (%s), hidexbmc (%s), islauncher (%s), warpcursor (%s)", 
          m_hideconsole ? "true" : "false",
          m_hidexbmc ? "true" : "false",
          m_islauncher ? "true" : "false",
          warpCursor.c_str());

#ifdef _WIN32PC
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
