/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ExternalPlayer.h"

#include "CompileInfo.h"
#include "FileItem.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "application/Application.h"
#include "application/ApplicationComponents.h"
#include "application/ApplicationPowerHandling.h"
#include "cores/AudioEngine/Interfaces/AE.h"
#include "cores/DataCacheCore.h"
#include "dialogs/GUIDialogOK.h"
#include "filesystem/MusicDatabaseFile.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "threads/SystemClock.h"
#include "utils/RegExp.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "utils/XMLUtils.h"
#include "utils/log.h"
#include "video/Bookmark.h"
#include "windowing/WinSystem.h"
#if defined(TARGET_WINDOWS)
  #include "utils/CharsetConverter.h"
  #include <Windows.h>
#endif
#if defined(TARGET_ANDROID)
  #include "platform/android/activity/XBMCApp.h"
#endif

// If the process ends in less than this time (ms), we assume it's a launcher
// and wait for manual intervention before continuing
#define LAUNCHER_PROCESS_TIME 2000
// Time (ms) we give a process we sent a WM_QUIT to close before terminating
#define PROCESS_GRACE_TIME 3000
// Default time after which the item's playcount is incremented
#define DEFAULT_PLAYCOUNT_MIN_TIME 10

using namespace XFILE;
using namespace std::chrono_literals;

#if defined(TARGET_WINDOWS_DESKTOP)
extern HWND g_hWnd;
#endif

CExternalPlayer::CExternalPlayer(IPlayerCallback& callback)
  : IPlayer(callback), CThread("ExternalPlayer"), m_playbackStartTime{}
{
  m_bAbortRequest = false;
  m_bIsPlaying = false;
  m_speed = 1;
  m_time = 0;

  m_hideconsole = false;
  m_warpcursor = WARP_NONE;
  m_hidexbmc = false;
  m_islauncher = false;
  m_playCountMinTime = DEFAULT_PLAYCOUNT_MIN_TIME;
  m_playOneStackItem = false;

  m_dialog = NULL;
#if defined(TARGET_WINDOWS_DESKTOP)
  m_xPos = 0;
  m_yPos = 0;

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
    m_file = file;
    m_bIsPlaying = true;
    m_time = 0;
    m_playbackStartTime = std::chrono::steady_clock::now();
    m_launchFilename = file.GetDynPath();
    CLog::Log(LOGINFO, "{}: {}", __FUNCTION__, m_launchFilename);
    Create();

    return true;
  }
  catch(...)
  {
    m_bIsPlaying = false;
    CLog::Log(LOGERROR, "{} - Exception thrown", __FUNCTION__);
    return false;
  }
}

bool CExternalPlayer::CloseFile(bool reopen)
{
  m_bAbortRequest = true;

  if (m_dialog && m_dialog->IsActive()) m_dialog->Close();

#if defined(TARGET_WINDOWS_DESKTOP)
  if (m_bIsPlaying && m_processInfo.hProcess)
  {
    TerminateProcess(m_processInfo.hProcess, 1);
  }
#endif
  CServiceBroker::GetDataCacheCore().Reset();
  return true;
}

bool CExternalPlayer::IsPlaying() const
{
  return m_bIsPlaying;
}

void CExternalPlayer::Process()
{
  std::string mainFile = m_launchFilename;
  std::string archiveContent;

  if (m_args.find("{0}") == std::string::npos)
  {
    // Unwind archive names
    CURL url(m_launchFilename);
    if (url.IsProtocol("zip") || url.IsProtocol("rar") /* || url.IsProtocol("iso9660") ??*/ || url.IsProtocol("udf"))
    {
      mainFile = url.GetHostName();
      archiveContent = url.GetFileName();
    }
    if (url.IsProtocol("musicdb"))
      mainFile = CMusicDatabaseFile::TranslateUrl(url);
    if (url.IsProtocol("bluray"))
    {
      CURL base(url.GetHostName());
      if (base.IsProtocol("udf"))
      {
        mainFile = base.GetHostName(); /* image file */
        archiveContent = base.GetFileName();
      }
      else
        mainFile = URIUtils::AddFileToFolder(base.Get(), url.GetFileName());
    }
  }

  if (!m_filenameReplacers.empty())
  {
    for (unsigned int i = 0; i < m_filenameReplacers.size(); i++)
    {
      std::vector<std::string> vecSplit = StringUtils::Split(m_filenameReplacers[i], " , ");

      // something is wrong, go to next substitution
      if (vecSplit.size() != 4)
        continue;

      std::string strMatch = vecSplit[0];
      StringUtils::Replace(strMatch, ",,",",");
      bool bCaseless = vecSplit[3].find('i') != std::string::npos;
      CRegExp regExp(bCaseless, CRegExp::autoUtf8);

      if (!regExp.RegComp(strMatch.c_str()))
      { // invalid regexp - complain in logs
        CLog::Log(LOGERROR, "{}: Invalid RegExp:'{}'", __FUNCTION__, strMatch);
        continue;
      }

      if (regExp.RegFind(mainFile) > -1)
      {
        std::string strPat = vecSplit[1];
        StringUtils::Replace(strPat, ",,",",");

        if (!regExp.RegComp(strPat.c_str()))
        { // invalid regexp - complain in logs
          CLog::Log(LOGERROR, "{}: Invalid RegExp:'{}'", __FUNCTION__, strPat);
          continue;
        }

        std::string strRep = vecSplit[2];
        StringUtils::Replace(strRep, ",,",",");
        bool bGlobal = vecSplit[3].find('g') != std::string::npos;
        bool bStop = vecSplit[3].find('s') != std::string::npos;
        int iStart = 0;
        while ((iStart = regExp.RegFind(mainFile, iStart)) > -1)
        {
          int iLength = regExp.GetFindLen();
          mainFile = mainFile.substr(0, iStart) + regExp.GetReplaceString(strRep) + mainFile.substr(iStart + iLength);
          if (!bGlobal)
            break;
        }
        CLog::Log(LOGINFO, "{}: File matched:'{}' (RE='{}',Rep='{}') new filename:'{}'.",
                  __FUNCTION__, strMatch, strPat, strRep, mainFile);
        if (bStop) break;
      }
    }
  }

  CLog::Log(LOGINFO, "{}: Player : {}", __FUNCTION__, m_filename);
  CLog::Log(LOGINFO, "{}: File   : {}", __FUNCTION__, mainFile);
  CLog::Log(LOGINFO, "{}: Content: {}", __FUNCTION__, archiveContent);
  CLog::Log(LOGINFO, "{}: Args   : {}", __FUNCTION__, m_args);
  CLog::Log(LOGINFO, "{}: Start", __FUNCTION__);

  // make sure we surround the arguments with quotes where necessary
  std::string strFName;
  std::string strFArgs;
#if defined(TARGET_WINDOWS_DESKTOP)
  // W32 batch-file handline
  if (StringUtils::EndsWith(m_filename, ".bat") || StringUtils::EndsWith(m_filename, ".cmd"))
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

  int nReplaced = StringUtils::Replace(strFArgs, "{0}", mainFile);

  if (!nReplaced)
    nReplaced = StringUtils::Replace(strFArgs, "{1}", mainFile) + StringUtils::Replace(strFArgs, "{2}", archiveContent);

  if (!nReplaced)
  {
    strFArgs.append(" \"");
    strFArgs.append(mainFile);
    strFArgs.append("\"");
  }

#if defined(TARGET_WINDOWS_DESKTOP)
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
    CLog::Log(LOGINFO, "{}: Warping cursor to ({},{})", __FUNCTION__, x, y);
    SetCursorPos(x,y);
  }

  LONG currentStyle = GetWindowLong(g_hWnd, GWL_EXSTYLE);
#endif

  if (m_hidexbmc && !m_islauncher)
  {
    CLog::Log(LOGINFO, "{}: Hiding {} window", __FUNCTION__, CCompileInfo::GetAppName());
    CServiceBroker::GetWinSystem()->Hide();
  }
#if defined(TARGET_WINDOWS_DESKTOP)
  else if (currentStyle & WS_EX_TOPMOST)
  {
    CLog::Log(LOGINFO, "{}: Lowering {} window", __FUNCTION__, CCompileInfo::GetAppName());
    SetWindowPos(g_hWnd, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOREDRAW | SWP_ASYNCWINDOWPOS);
  }

  CLog::Log(LOGDEBUG, "{}: Unlocking foreground window", __FUNCTION__);
  LockSetForegroundWindow(LSFW_UNLOCK);
#endif

  m_playbackStartTime = std::chrono::steady_clock::now();

  /* Suspend AE temporarily so exclusive or hog-mode sinks */
  /* don't block external player's access to audio device  */
  CServiceBroker::GetActiveAE()->Suspend();
  // wait for AE has completed suspended
  XbmcThreads::EndTime<> timer(2000ms);
  while (!timer.IsTimePast() && !CServiceBroker::GetActiveAE()->IsSuspended())
  {
    CThread::Sleep(50ms);
  }
  if (timer.IsTimePast())
  {
    CLog::Log(LOGERROR, "{}: AudioEngine did not suspend before launching external player",
              __FUNCTION__);
  }

  m_callback.OnPlayBackStarted(m_file);
  m_callback.OnAVStarted(m_file);

  bool ret = true;
#if defined(TARGET_WINDOWS_DESKTOP)
  ret = ExecuteAppW32(strFName.c_str(),strFArgs.c_str());
#elif defined(TARGET_ANDROID)
  ret = ExecuteAppAndroid(m_filename.c_str(), mainFile.c_str());
#elif defined(TARGET_POSIX) && !defined(TARGET_DARWIN_EMBEDDED)
  ret = ExecuteAppLinux(strFArgs.c_str());
#endif
  auto end = std::chrono::steady_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - m_playbackStartTime);

  if (ret && (m_islauncher || duration.count() < LAUNCHER_PROCESS_TIME))
  {
    if (m_hidexbmc)
    {
      CLog::Log(LOGINFO, "{}: {} cannot stay hidden for a launcher process", __FUNCTION__,
                CCompileInfo::GetAppName());
      CServiceBroker::GetWinSystem()->Show(false);
    }

    {
      m_dialog = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogOK>(WINDOW_DIALOG_OK);
      m_dialog->SetHeading(CVariant{23100});
      m_dialog->SetLine(1, CVariant{23104});
      m_dialog->SetLine(2, CVariant{23105});
      m_dialog->SetLine(3, CVariant{23106});
    }

    if (!m_bAbortRequest)
      m_dialog->Open();
  }

  m_bIsPlaying = false;
  CLog::Log(LOGINFO, "{}: Stop", __FUNCTION__);

#if defined(TARGET_WINDOWS_DESKTOP)
  CServiceBroker::GetWinSystem()->Restore();

  if (currentStyle & WS_EX_TOPMOST)
  {
    CLog::Log(LOGINFO, "{}: Showing {} window TOPMOST", __FUNCTION__, CCompileInfo::GetAppName());
    SetWindowPos(g_hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW | SWP_ASYNCWINDOWPOS);
    SetForegroundWindow(g_hWnd);
  }
  else
#endif
  {
    CLog::Log(LOGINFO, "{}: Showing {} window", __FUNCTION__, CCompileInfo::GetAppName());
    CServiceBroker::GetWinSystem()->Show();
  }

#if defined(TARGET_WINDOWS_DESKTOP)
  if (m_warpcursor)
  {
    m_xPos = 0;
    m_yPos = 0;
    if (&m_ptCursorpos != 0)
    {
      m_xPos = (m_ptCursorpos.x);
      m_yPos = (m_ptCursorpos.y);
    }
    CLog::Log(LOGINFO, "{}: Restoring cursor to ({},{})", __FUNCTION__, m_xPos, m_yPos);
    SetCursorPos(m_xPos,m_yPos);
  }
#endif

  CBookmark bookmark;
  bookmark.totalTimeInSeconds = 1;
  bookmark.timeInSeconds = (duration.count() / 1000 >= m_playCountMinTime) ? 1 : 0;
  bookmark.player = m_name;
  m_callback.OnPlayerCloseFile(m_file, bookmark);

  /* Resume AE processing of XBMC native audio */
  if (!CServiceBroker::GetActiveAE()->Resume())
  {
    CLog::Log(LOGFATAL, "{}: Failed to restart AudioEngine after return from external player",
              __FUNCTION__);
  }

  // We don't want to come back to an active screensaver
  auto& components = CServiceBroker::GetAppComponents();
  const auto appPower = components.GetComponent<CApplicationPowerHandling>();
  appPower->ResetScreenSaver();
  appPower->WakeUpScreenSaverAndDPMS();

  if (!ret || (m_playOneStackItem && g_application.CurrentFileItem().IsStack()))
    m_callback.OnPlayBackStopped();
  else
    m_callback.OnPlayBackEnded();
}

#if defined(TARGET_WINDOWS_DESKTOP)
bool CExternalPlayer::ExecuteAppW32(const char* strPath, const char* strSwitches)
{
  CLog::Log(LOGINFO, "{}: {} {}", __FUNCTION__, strPath, strSwitches);

  STARTUPINFOW si = {};
  si.cb = sizeof(si);
  si.dwFlags = STARTF_USESHOWWINDOW;
  si.wShowWindow = m_hideconsole ? SW_HIDE : SW_SHOW;

  std::wstring WstrPath, WstrSwitches;
  g_charsetConverter.utf8ToW(strPath, WstrPath, false);
  g_charsetConverter.utf8ToW(strSwitches, WstrSwitches, false);

  if (m_bAbortRequest) return false;

  BOOL ret = CreateProcessW(WstrPath.empty() ? NULL : WstrPath.c_str(),
                            (LPWSTR) WstrSwitches.c_str(), NULL, NULL, FALSE, NULL,
                            NULL, NULL, &si, &m_processInfo);

  if (ret == FALSE)
  {
    DWORD lastError = GetLastError();
    CLog::Log(LOGINFO, "{} - Failure: {}", __FUNCTION__, lastError);
  }
  else
  {
    int res = WaitForSingleObject(m_processInfo.hProcess, INFINITE);

    switch (res)
    {
      case WAIT_OBJECT_0:
        CLog::Log(LOGINFO, "{}: WAIT_OBJECT_0", __FUNCTION__);
        break;
      case WAIT_ABANDONED:
        CLog::Log(LOGINFO, "{}: WAIT_ABANDONED", __FUNCTION__);
        break;
      case WAIT_TIMEOUT:
        CLog::Log(LOGINFO, "{}: WAIT_TIMEOUT", __FUNCTION__);
        break;
      case WAIT_FAILED:
        CLog::Log(LOGINFO, "{}: WAIT_FAILED ({})", __FUNCTION__, GetLastError());
        ret = FALSE;
        break;
    }

    CloseHandle(m_processInfo.hThread);
    m_processInfo.hThread = 0;
    CloseHandle(m_processInfo.hProcess);
    m_processInfo.hProcess = 0;
  }
  return (ret == TRUE);
}
#endif

#if !defined(TARGET_ANDROID) && !defined(TARGET_DARWIN_EMBEDDED) && defined(TARGET_POSIX)
bool CExternalPlayer::ExecuteAppLinux(const char* strSwitches)
{
  CLog::Log(LOGINFO, "{}: {}", __FUNCTION__, strSwitches);

  int ret = system(strSwitches);
  if (ret != 0)
  {
    CLog::Log(LOGINFO, "{}: Failure: {}", __FUNCTION__, ret);
  }

  return (ret == 0);
}
#endif

#if defined(TARGET_ANDROID)
bool CExternalPlayer::ExecuteAppAndroid(const char* strSwitches,const char* strPath)
{
  CLog::Log(LOGINFO, "{}: {}", __FUNCTION__, strSwitches);

  bool ret = CXBMCApp::StartActivity(strSwitches, "android.intent.action.VIEW", "video/*", strPath);

  if (!ret)
  {
    CLog::Log(LOGINFO, "{}: Failure", __FUNCTION__);
  }

  return (ret == 0);
}
#endif

void CExternalPlayer::Pause()
{
}

bool CExternalPlayer::HasVideo() const
{
  return true;
}

bool CExternalPlayer::HasAudio() const
{
  return false;
}

bool CExternalPlayer::CanSeek() const
{
  return false;
}

void CExternalPlayer::Seek(bool bPlus, bool bLargeStep, bool bChapterOverride)
{
}

void CExternalPlayer::SeekPercentage(float iPercent)
{
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

void CExternalPlayer::SetSpeed(float speed)
{
  m_speed = speed;
  CDataCacheCore::GetInstance().SetSpeed(1.0, speed);
}

bool CExternalPlayer::SetPlayerState(const std::string& state)
{
  return true;
}

bool CExternalPlayer::Initialize(TiXmlElement* pConfig)
{
  XMLUtils::GetString(pConfig, "filename", m_filename);
  if (m_filename.length() > 0)
  {
    CLog::Log(LOGINFO, "ExternalPlayer Filename: {}", m_filename);
  }
  else
  {
    std::string xml;
    xml<<*pConfig;
    CLog::Log(LOGERROR, "ExternalPlayer Error: filename element missing from: {}", xml);
    return false;
  }

  XMLUtils::GetString(pConfig, "args", m_args);
  XMLUtils::GetBoolean(pConfig, "playonestackitem", m_playOneStackItem);
  XMLUtils::GetBoolean(pConfig, "islauncher", m_islauncher);
  XMLUtils::GetBoolean(pConfig, "hidexbmc", m_hidexbmc);
  if (!XMLUtils::GetBoolean(pConfig, "hideconsole", m_hideconsole))
  {
#ifdef TARGET_WINDOWS_DESKTOP
    // Default depends on whether player is a batch file
    m_hideconsole = StringUtils::EndsWith(m_filename, ".bat");
#endif
  }

  bool bHideCursor;
  if (XMLUtils::GetBoolean(pConfig, "hidecursor", bHideCursor) && bHideCursor)
    m_warpcursor = WARP_BOTTOM_RIGHT;

  std::string warpCursor;
  if (XMLUtils::GetString(pConfig, "warpcursor", warpCursor) && !warpCursor.empty())
  {
    if (warpCursor == "bottomright") m_warpcursor = WARP_BOTTOM_RIGHT;
    else if (warpCursor == "bottomleft") m_warpcursor = WARP_BOTTOM_LEFT;
    else if (warpCursor == "topleft") m_warpcursor = WARP_TOP_LEFT;
    else if (warpCursor == "topright") m_warpcursor = WARP_TOP_RIGHT;
    else if (warpCursor == "center") m_warpcursor = WARP_CENTER;
    else
    {
      warpCursor = "none";
      CLog::Log(LOGWARNING, "ExternalPlayer: invalid value for warpcursor: {}", warpCursor);
    }
  }

  XMLUtils::GetInt(pConfig, "playcountminimumtime", m_playCountMinTime, 1, INT_MAX);

  CLog::Log(
      LOGINFO,
      "ExternalPlayer Tweaks: hideconsole ({}), hidexbmc ({}), islauncher ({}), warpcursor ({})",
      m_hideconsole ? "true" : "false", m_hidexbmc ? "true" : "false",
      m_islauncher ? "true" : "false", warpCursor);

#ifdef TARGET_WINDOWS_DESKTOP
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
                                               std::vector<std::string>& settings)
{
  int iAction = 0; // overwrite
  // for backward compatibility
  const char* szAppend = pRootElement->Attribute("append");
  if ((szAppend && StringUtils::CompareNoCase(szAppend, "yes") == 0))
    iAction = 1;
  // action takes precedence if both attributes exist
  const char* szAction = pRootElement->Attribute("action");
  if (szAction)
  {
    iAction = 0; // overwrite
    if (StringUtils::CompareNoCase(szAction, "append") == 0)
      iAction = 1; // append
    else if (StringUtils::CompareNoCase(szAction, "prepend") == 0)
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
      bool bGlobal = szGlobal && StringUtils::CompareNoCase(szGlobal, "true") == 0;
      bool bStop = szStop && StringUtils::CompareNoCase(szStop, "true") == 0;

      std::string strMatch;
      std::string strPat;
      std::string strRep;
      XMLUtils::GetString(pReplacer,"match",strMatch);
      XMLUtils::GetString(pReplacer,"pat",strPat);
      XMLUtils::GetString(pReplacer,"rep",strRep);

      if (!strPat.empty() && !strRep.empty())
      {
        CLog::Log(LOGDEBUG,"  Registering replacer:");
        CLog::Log(LOGDEBUG, "    Match:[{}] Pattern:[{}] Replacement:[{}]", strMatch, strPat,
                  strRep);
        CLog::Log(LOGDEBUG, "    Global:[{}] Stop:[{}]", bGlobal ? "true" : "false",
                  bStop ? "true" : "false");
        // keep literal commas since we use comma as a separator
        StringUtils::Replace(strMatch, ",",",,");
        StringUtils::Replace(strPat, ",",",,");
        StringUtils::Replace(strRep, ",",",,");

        std::string strReplacer = strMatch + " , " + strPat + " , " + strRep + " , " + (bGlobal ? "g" : "") + (bStop ? "s" : "");
        if (iAction == 2)
          settings.insert(settings.begin() + i++, 1, strReplacer);
        else
          settings.push_back(strReplacer);
      }
      else
      {
        // error message about missing tag
        if (strPat.empty())
          CLog::Log(LOGERROR,"  Missing <Pat> tag");
        else
          CLog::Log(LOGERROR,"  Missing <Rep> tag");
      }
    }

    pReplacer = pReplacer->NextSiblingElement("replacer");
  }
}
