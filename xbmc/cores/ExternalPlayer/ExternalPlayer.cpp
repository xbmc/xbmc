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

CExternalPlayer::CExternalPlayer(IPlayerCallback& callback)
    : IPlayer(callback),
      CThread()
{
  m_paused = false;
  m_clock = 0;
  m_lastTime = 0;
  m_speed = 1;
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

    if (g_advancedSettings.m_externalPlayerFilenameReplacers.size() > 0) 
    {
      CRegExp regExp;
      for (unsigned int i = 0; i < g_advancedSettings.m_externalPlayerFilenameReplacers.size(); i++)
      {
        std::vector<CStdString> vecSplit;
        StringUtils::SplitString(g_advancedSettings.m_externalPlayerFilenameReplacers[i], " , ", vecSplit);

        // something is wrong, go to next substitution
        if (vecSplit.size() != 4)
          continue;

        CStdString strMatch = vecSplit[0];
        strMatch.Replace(",,",",");
        //bool bCaseless = vecSplit[3].Find('i') > -1;
        //if (bCaseless)
        //  regExp.m_iOptions |= PCRE_CASELESS;
        //else
        //  regExp.m_iOptions &= ^PCRE_CASELESS;

        if (!regExp.RegComp(strMatch.c_str()))
        { // invalid regexp - complain in logs
          CLog::Log(LOGERROR, "%s: Invalid RegExp:'%s'", __FUNCTION__, strMatch.c_str());
          continue;
        }

        if (regExp.RegFind(m_launchFilename) > -1) 
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
          while ((iStart = regExp.RegFind(m_launchFilename, iStart)) > -1)
          {
            int iLength = regExp.GetFindLen();
            m_launchFilename = m_launchFilename.Left(iStart) + regExp.GetReplaceString(strRep.c_str()) + m_launchFilename.Mid(iStart+iLength);
            if (!bGlobal)
              break;
          }
          CLog::Log(LOGINFO, "%s: File matched:'%s' (RE='%s',Rep='%s') new filename:'%s'.", __FUNCTION__, strMatch.c_str(), strPat.c_str(), strRep.c_str(), m_launchFilename.c_str());
          if (bStop) break;
        }
      }
    }

    Create();
    if( options.starttime > 0 )
      SeekTime( (__int64)(options.starttime * 1000) );
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
  return !m_bStop;
}

void CExternalPlayer::Process()
{
  m_clock = 0;
  m_lastTime = timeGetTime();

  CLog::Log(LOGNOTICE, "%s: Filename: %s", __FUNCTION__, g_advancedSettings.m_externalPlayerFilename.c_str());
  CLog::Log(LOGNOTICE, "%s: Args: %s", __FUNCTION__, g_advancedSettings.m_externalPlayerArgs.c_str());
  CLog::Log(LOGNOTICE, "%s: Default Audio Player: %s", __FUNCTION__, g_advancedSettings.m_audioDefaultPlayer.c_str());
  CLog::Log(LOGNOTICE, "%s: Default Video Player: %s", __FUNCTION__, g_advancedSettings.m_videoDefaultPlayer.c_str());
  CLog::Log(LOGNOTICE, "%s: Start", __FUNCTION__);

  // make sure we surround the arguments with quotes where necessary
  CStdString strFName = g_advancedSettings.m_externalPlayerFilename.c_str();
  CStdString strFArgs = "\"";
  strFArgs.append(g_advancedSettings.m_externalPlayerFilename.c_str());
  strFArgs.append("\" ");
  strFArgs.append(g_advancedSettings.m_externalPlayerArgs.c_str());
  strFArgs.append(" \"");
  strFArgs.append(m_launchFilename.c_str());
  strFArgs.append("\"");

  int iActiveDevice = g_audioContext.GetActiveDevice();
  g_audioContext.SetActiveDevice(CAudioContext::NONE);

  BOOL ret = TRUE;
#if defined(_WIN32PC)
  if (g_advancedSettings.m_externalPlayerHidecursor)
  {
    CLog::Log(LOGNOTICE, "%s: Hiding cursor", __FUNCTION__);
    GetCursorPos(&m_ptCursorpos);
    SetCursorPos(GetSystemMetrics(SM_CXSCREEN),GetSystemMetrics(SM_CYSCREEN));
  }
  
  m_hwndXbmc = GetForegroundWindow();

  if (g_advancedSettings.m_externalPlayerForceontop && m_hwndXbmc)
  {
    CLog::Log(LOGNOTICE, "%s: Making XBMC window NOTOPMOST", __FUNCTION__);
    SetWindowPos(m_hwndXbmc,HWND_NOTOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOREDRAW);
  }  
  if (g_advancedSettings.m_externalPlayerHidexbmc && m_hwndXbmc)
  {
    CLog::Log(LOGNOTICE, "%s: Hiding XBMC window", __FUNCTION__);
    ShowWindow(m_hwndXbmc,SW_HIDE);
  }

  LockSetForegroundWindow(LSFW_UNLOCK);

  ret = ExecuteAppW32(strFName.c_str(),strFArgs.c_str());

  if ((g_advancedSettings.m_externalPlayerForceontop || g_advancedSettings.m_externalPlayerHidexbmc) && m_hwndXbmc)
  {
    if (g_advancedSettings.m_externalPlayerHidexbmc)
    {
      CLog::Log(LOGNOTICE, "%s: Showing XBMC window", __FUNCTION__);
      ShowWindow(m_hwndXbmc,SW_SHOW);
    }
    if (g_advancedSettings.m_externalPlayerForceontop)
    {
      CLog::Log(LOGNOTICE, "%s: Making XBMC window TOPMOST", __FUNCTION__);
      SetWindowPos(m_hwndXbmc,HWND_TOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE);
    }
  }
  SetFocus(m_hwndXbmc);
  SetForegroundWindow(m_hwndXbmc);
  LockSetForegroundWindow(LSFW_LOCK);
  if (g_advancedSettings.m_externalPlayerHidecursor)
  {
    m_xPos = 0;
    m_yPos = 0;
    if (&m_ptCursorpos != 0)
    {
      m_xPos = (m_ptCursorpos.x);
      m_yPos = (m_ptCursorpos.y);
    }
    SetCursorPos(m_xPos,m_yPos);
  }
#endif

#if defined(_LINUX)
  ret = ExecuteAppLinux(strFArgs.c_str());
#endif

  CLog::Log(LOGNOTICE, "%s: Stop", __FUNCTION__);

  // We don't want to come back to an active screensaver
  g_application.ResetScreenSaver();
  g_application.ResetScreenSaverWindow();
  g_audioContext.SetActiveDevice(iActiveDevice);

  if (ret) 
  {
    // Deactivate any library window, we reactivate it later in order to refresh the
    // watched-state of the item.
    DWORD activeWindow = m_gWindowManager.GetActiveWindow();
    if (g_stSettings.m_iMyVideoWatchMode == VIDEO_SHOW_UNWATCHED &&
        (activeWindow == WINDOW_VIDEO_NAV || activeWindow == WINDOW_MUSIC_NAV))
    {
      CGUIMessage msg(GUI_MSG_WINDOW_DEINIT, 0, 0, activeWindow);
      m_gWindowManager.SendThreadMessage(msg, activeWindow);
    }

    m_callback.OnPlayBackEnded();

    // Re-activate any library window in order to refresh the watched-state of the item.
    // This isn't exactly quick, but is the same as when you use the builtin player.
    if (g_stSettings.m_iMyVideoWatchMode == VIDEO_SHOW_UNWATCHED &&
        (activeWindow == WINDOW_VIDEO_NAV || activeWindow == WINDOW_MUSIC_NAV))
    {
      CGUIMessage msg(GUI_MSG_WINDOW_INIT, 0, 0, WINDOW_INVALID, activeWindow);
      msg.SetStringParam(",return");
      m_gWindowManager.SendThreadMessage(msg, activeWindow);
    }
  }
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
  if (g_advancedSettings.m_externalPlayerHideconsole)
  {
    si.wShowWindow=SW_HIDE;
  }
  else
  {
    si.wShowWindow=SW_SHOW;
  }

  CStdStringW WstrPath, WstrSwitches;
  g_charsetConverter.utf8ToW(strPath, WstrPath);
  g_charsetConverter.utf8ToW(strSwitches, WstrSwitches);

  BOOL ret = CreateProcessW(WstrPath.c_str(), (LPWSTR) WstrSwitches.c_str(), NULL, NULL, FALSE, NULL, 
                          NULL, NULL, &si, &pi);
  if (ret == FALSE)
  {
    CLog::Log(LOGNOTICE, "%s - Failure: %d", __FUNCTION__, ret);
  }
  else
  {
    if (g_advancedSettings.m_externalPlayerStartupTime > 0) 
    {
      int res = WaitForSingleObject(pi.hProcess, g_advancedSettings.m_externalPlayerStartupTime);
      if (res == WAIT_TIMEOUT) 
      {
        // Hopefully by now the player window is now visible above ours; time to lock the GraphicsContext to save CPU cycles,
        g_graphicsContext.Lock();
        // wait for the player to finish
        res = WaitForSingleObject(pi.hProcess, INFINITE);
        g_application.ResetScreenSaverTimer();
        // and unlock it again
        g_graphicsContext.Unlock();
      }
      switch (res) 
      {
        case WAIT_OBJECT_0:
          CLog::Log(LOGNOTICE, "%s: WAIT_OBJECT_0", __FUNCTION__);
          break;
        case WAIT_ABANDONED:
          CLog::Log(LOGNOTICE, "%s: WAIT_ABANDONED", __FUNCTION__);
          break;
        case WAIT_FAILED:
          CLog::Log(LOGNOTICE, "%s: WAIT_FAILED (%d)", __FUNCTION__, GetLastError());
          break;
      }
    }
    else
    {
      CGUIDialogOK *dialog = (CGUIDialogOK *)m_gWindowManager.GetWindow(WINDOW_DIALOG_OK);
      dialog->SetHeading("External Player Active");
      dialog->SetLine(0, "Click OK to terminate player,");
      dialog->SetLine(1, "only if you cannot close it normally");

      HANDLE hWait;
      BOOL waiting = RegisterWaitForSingleObject(&hWait, pi.hProcess, AppFinished, dialog, INFINITE, WT_EXECUTEDEFAULT|WT_EXECUTEONLYONCE);

      if (!waiting) 
        CLog::Log(LOGERROR,"%s: Failed to register wait callback (%d)", __FUNCTION__, GetLastError());

      dialog->DoModal();

      if (dialog->IsConfirmed())
      {
        if (waiting)
          UnregisterWait(hWait);

        // Post a message telling the player to stop
        PostThreadMessage(pi.dwThreadId, WM_QUIT, 0, 0);

        // and wait a short while for it to do so
        int res = WaitForSingleObject(pi.hProcess, 3000);
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
            Sleep(3000);
            TerminateProcess(pi.hProcess, 1);
            break;
        }
      } // else the process exited normally and we closed the dialog programatically
    }

    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
  }

  return ret;
}

void CALLBACK CExternalPlayer::AppFinished(void* closure, BOOLEAN TimerOrWaitFired)
{
    CGUIDialogOK *dialog = (CGUIDialogOK *)closure;
    dialog->Close();
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
  return false;
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

void CExternalPlayer::ToggleFrameDrop()
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

void CExternalPlayer::SeekTime(__int64 iTime)
{
}

__int64 CExternalPlayer::GetTime()
{
  return 0;
}

int CExternalPlayer::GetTotalTime()
{
  return 1000;
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

