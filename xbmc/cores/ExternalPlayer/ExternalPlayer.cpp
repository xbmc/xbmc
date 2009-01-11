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
#include "ExternalPlayer.h"
#include "GUIFontManager.h"
#include "GUITextLayout.h"
#include "Application.h"
#include "Settings.h"
#include "FileItem.h"
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
#ifdef _WIN32PC
    if (m_launchFilename.Left(6).Equals("smb://"))
    { // mayaswell attempt to use win32 smb stuff
      m_launchFilename.Replace("smb://", "//");
      m_launchFilename.Replace('/', '\\');
    }
#endif
    CLog::Log(LOGNOTICE, "%s: %s", __FUNCTION__, m_launchFilename.c_str());
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

  CLog::Log(LOGNOTICE, "CExternalPlayer:Filename: %s", g_advancedSettings.m_externalPlayerFilename.c_str());
  CLog::Log(LOGNOTICE, "CExternalPlayer:Args: %s", g_advancedSettings.m_externalPlayerArgs.c_str());
  CLog::Log(LOGNOTICE, "CExternalPlayer:Default Audio Player: %s", g_advancedSettings.m_audioDefaultPlayer.c_str());
  CLog::Log(LOGNOTICE, "CExternalPlayer:Default Video Player: %s", g_advancedSettings.m_videoDefaultPlayer.c_str());
  CLog::Log(LOGNOTICE, "CExternalPlayer:Process: Start");

  // make sure we surround the arguments with quotes where necessary
  CStdString strFName = g_advancedSettings.m_externalPlayerFilename.c_str();
  CStdString strFArgs = "\"";
  strFArgs.append(g_advancedSettings.m_externalPlayerFilename.c_str());
  strFArgs.append("\" ");
  strFArgs.append(g_advancedSettings.m_externalPlayerArgs.c_str());
  strFArgs.append(" \"");
  strFArgs.append(m_launchFilename.c_str());
  strFArgs.append("\"");

#if defined(_WIN32PC)
  if (g_advancedSettings.m_externalPlayerHidecursor)
  {
    GetCursorPos(&m_ptCursorpos);
    SetCursorPos(GetSystemMetrics(SM_CXSCREEN),GetSystemMetrics(SM_CYSCREEN));
  }
  if (g_advancedSettings.m_externalPlayerForceontop)
  {
    m_hwndXbmc = GetForegroundWindow();
    if (m_hwndXbmc)
    {
      SetWindowPos(m_hwndXbmc,HWND_TOPMOST,0,0,0,0,SWP_HIDEWINDOW|SWP_NOMOVE|SWP_NOSIZE|SWP_NOREDRAW);
    }
  }  
  LockSetForegroundWindow(LSFW_UNLOCK);
  ExecuteAppW32(strFName.c_str(),strFArgs.c_str());

  if (g_advancedSettings.m_externalPlayerForceontop)
  {
    if (m_hwndXbmc)
    {
      SetWindowPos(m_hwndXbmc,HWND_TOPMOST,0,0,0,0,SWP_SHOWWINDOW|SWP_NOMOVE|SWP_NOSIZE|SWP_NOREDRAW);
      SetForegroundWindow(m_hwndXbmc);
      SetFocus(m_hwndXbmc);
    }
  }
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
  ExecuteAppLinux(strFArgs.c_str());
#endif

  CLog::Log(LOGNOTICE, "CExternalPlayer:Stop");
  m_callback.OnPlayBackEnded();
}

#if defined(_WIN32PC)
void CExternalPlayer::ExecuteAppW32(const char* strPath, const char* strSwitches)
{
  CLog::Log(LOGNOTICE, "%s: %s %s", __FUNCTION__, strPath, strSwitches);

  STARTUPINFO si;
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
  int ret = CreateProcess(strPath, (LPTSTR) strSwitches, NULL, NULL, FALSE, NULL, 
                          NULL, NULL, &si, &pi);
  if (ret == FALSE) 
  {
    CLog::Log(LOGNOTICE, "%s - Failure: %d", __FUNCTION__, ret);
  }
  else 
  {
    Sleep(0);
    g_graphicsContext.Lock();
    int res = WaitForSingleObject(pi.hProcess, INFINITE);
    Sleep(0);
    g_graphicsContext.Unlock();
    switch (res) 
    {
      case WAIT_FAILED:
        CLog::Log(LOGNOTICE, "CExternalPlayer:WAIT_FAILED");
        break;
      case WAIT_ABANDONED:
        CLog::Log(LOGNOTICE, "CExternalPlayer:WAIT_ABANDONED");
        break;
      case WAIT_OBJECT_0:
        CLog::Log(LOGNOTICE, "CExternalPlayer:WAIT_OBJECT_0");
      break;
      case WAIT_TIMEOUT:
        CLog::Log(LOGNOTICE, "CExternalPlayer:WAIT_TIMEOUT");
        break;
      default:
        CLog::Log(LOGNOTICE, "CExternalPlayer:...");
        break;
      }
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
  }
}
#endif

#if defined(_LINUX)
void CExternalPlayer::ExecuteAppLinux(const char* strSwitches)
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
    CLog::Log(LOGNOTICE, "%s - Failure: %d", __FUNCTION__, ret);
  }
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
