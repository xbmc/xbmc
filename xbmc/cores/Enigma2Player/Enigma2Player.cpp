/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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
#include "CompileInfo.h"
#include "threads/SingleLock.h"
#include "Enigma2Player.h"
#include "windowing/WindowingFactory.h"
#include "dialogs/GUIDialogOK.h"
#include "guilib/GUIWindowManager.h"
#include "Application.h"
#include "filesystem/MusicDatabaseFile.h"
#include "FileItem.h"
#include "utils/RegExp.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "URL.h"
#include "utils/XMLUtils.h"
#include "utils/log.h"
#include "utils/JSONVariantWriter.h"
#include "utils/JSONVariantParser.h"
#include "cores/AudioEngine/AEFactory.h"
#include "input/InputManager.h"

// If the process ends in less than this time (ms), we assume it's a launcher
// and wait for manual intervention before continuing
#define LAUNCHER_PROCESS_TIME 2000
// Time (ms) we give a process we sent a WM_QUIT to close before terminating
#define PROCESS_GRACE_TIME 3000
// Default time after which the item's playcount is incremented
#define DEFAULT_PLAYCOUNT_MIN_TIME 10

using namespace XFILE;

CEnigma2Player::CEnigma2Player(IPlayerCallback& callback)
    : IPlayer(callback),
      CThread("Enigma2Player")
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
  m_hwndXbmc = NULL;
  m_xPos = 0;
  m_yPos = 0;

}

CEnigma2Player::~CEnigma2Player()
{
  CloseFile();
}

bool CEnigma2Player::OpenFile(const CFileItem& file, const CPlayerOptions &options)
{
  try
  {
    m_bIsPlaying = true;
    m_time = 0;
    m_playbackStartTime = XbmcThreads::SystemClockMillis();
    m_launchFilename = file.GetPath();
    m_item = file;

    if (m_filenameMeta.length() > 0)
    {
      CVariant variant;
      file.Serialize(variant);
      CVariant playerOptionsVariant;
      playerOptionsVariant["startPercent"] = options.startpercent;
      playerOptionsVariant["startTime"] = options.starttime;
      variant["playerOptions"] = playerOptionsVariant;

      CVariant listItemVariant;
      listItemVariant["label"] = file.GetLabel();
      listItemVariant["label2"] = file.GetLabel2();
      listItemVariant["iconImage"] = file.GetIconImage();
      variant["listItem"] = listItemVariant;

      std::string jsonStr;
      jsonStr = CJSONVariantWriter::Write(variant, false);
      CLog::Log(LOGNOTICE, "%s json: %s", __FUNCTION__, jsonStr.c_str());

      CFile file;
      if (file.OpenForWrite(m_filenameMeta, true))
      {
        ssize_t ret;
        if ((ret = file.Write(jsonStr.c_str(), jsonStr.length())) == -1)
        {
          CLog::Log(LOGDEBUG, "%s ex write error occurred(json)!", __FUNCTION__);
        }
        else if (ret != jsonStr.length())
        {
          CLog::Log(LOGDEBUG, "%s write error occurred(json)!", __FUNCTION__);
        }
        file.Close();
      }
    }

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

bool CEnigma2Player::CloseFile(bool reopen)
{
  m_bAbortRequest = true;

  if (m_dialog && m_dialog->IsActive()) m_dialog->Close();

  return true;
}

bool CEnigma2Player::IsPlaying() const
{
  return m_bIsPlaying;
}

void CEnigma2Player::Process()
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

  if (m_filenameReplacers.size() > 0)
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
        CLog::Log(LOGERROR, "%s: Invalid RegExp:'%s'", __FUNCTION__, strMatch.c_str());
        continue;
      }

      if (regExp.RegFind(mainFile) > -1)
      {
        std::string strPat = vecSplit[1];
        StringUtils::Replace(strPat, ",,",",");

        if (!regExp.RegComp(strPat.c_str()))
        { // invalid regexp - complain in logs
          CLog::Log(LOGERROR, "%s: Invalid RegExp:'%s'", __FUNCTION__, strPat.c_str());
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
        CLog::Log(LOGINFO, "%s: File matched:'%s' (RE='%s',Rep='%s') new filename:'%s'.", __FUNCTION__, strMatch.c_str(), strPat.c_str(), strRep.c_str(), mainFile.c_str());
        if (bStop) break;
      }
    }
  }
  
  // add subtitles from file item
  std::string key("subtitle:1");
  for(unsigned s = 1; m_item.HasProperty(key); key = StringUtils::Format("subtitle:%u", ++s))
  {
    CLog::Log(LOGDEBUG, "%s: found subtitle: %s", __FUNCTION__, m_item.GetProperty(key).asString().c_str());
    m_subs.push_back(m_item.GetProperty(key).asString());
  }
  
  /*
   * wait a moment in case external subtitles
   * are loaded pre-HELIX way
   */
  
  CLog::Log(LOGDEBUG, "%s: waiting for subtitles", __FUNCTION__);
  Sleep(200);
  CLog::Log(LOGDEBUG, "%s: waiting for subtitles ended", __FUNCTION__);
  
  std::string strSubs;
  for (std::vector<std::string>::const_iterator iter = m_subs.begin(); iter != m_subs.end(); iter++)
  {
    if(!strSubs.empty())
      strSubs += "|" + *iter;
    else
      strSubs = *iter;
  }
  
  CLog::Log(LOGNOTICE, "%s: Player : %s", __FUNCTION__, m_filename.c_str());
  CLog::Log(LOGNOTICE, "%s: File   : %s", __FUNCTION__, mainFile.c_str());
  CLog::Log(LOGNOTICE, "%s: Content: %s", __FUNCTION__, archiveContent.c_str());
  CLog::Log(LOGNOTICE, "%s: Args   : %s", __FUNCTION__, m_args.c_str());
  CLog::Log(LOGNOTICE, "%s: Subs   : %s", __FUNCTION__, strSubs.c_str());
  CLog::Log(LOGNOTICE, "%s: Start", __FUNCTION__);

  // make sure we surround the arguments with quotes where necessary
  std::string strFName;
  std::string strFArgs;
  
    strFName = m_filename;

  strFArgs.append("\"");
  strFArgs.append(m_filename);
  strFArgs.append("\" ");
  strFArgs.append(m_args);

  int nReplaced = StringUtils::Replace(strFArgs, "{0}", mainFile);

  if (!nReplaced)
    nReplaced = StringUtils::Replace(strFArgs, "{1}", mainFile) + StringUtils::Replace(strFArgs, "{2}", archiveContent) + StringUtils::Replace(strFArgs, "{3}", strSubs);

  if (!nReplaced)
  {
    strFArgs.append(" \"");
    strFArgs.append(mainFile);
    strFArgs.append("\"");
  }

  if (m_hidexbmc && !m_islauncher)
  {
    CLog::Log(LOGNOTICE, "%s: Hiding %s window", __FUNCTION__, CCompileInfo::GetAppName());
    g_Windowing.Hide();
  }

  m_playbackStartTime = XbmcThreads::SystemClockMillis();

  /* Suspend AE temporarily so exclusive or hog-mode sinks */
  /* don't block external player's access to audio device  */
  CAEFactory::Suspend();
  // wait for AE has completed suspended
  XbmcThreads::EndTime timer(2000);
  while (!timer.IsTimePast() && !CAEFactory::IsSuspended())
  {
    Sleep(50);
  }
  if (timer.IsTimePast())
  {
    CLog::Log(LOGERROR,"%s: AudioEngine did not suspend before launching external player", __FUNCTION__);
  }

  m_callback.OnPlayBackStarted();

  BOOL ret = TRUE;
  ret = ExecuteAppLinux(strFArgs.c_str());
  int64_t elapsedMillis = XbmcThreads::SystemClockMillis() - m_playbackStartTime;

  if (ret && (m_islauncher || elapsedMillis < LAUNCHER_PROCESS_TIME))
  {
    if (m_hidexbmc)
    {
      CLog::Log(LOGNOTICE, "%s: %s cannot stay hidden for a launcher process", __FUNCTION__, CCompileInfo::GetAppName());
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

    if (!m_bAbortRequest)
      m_dialog->Open();
  }

  m_bIsPlaying = false;
  CLog::Log(LOGNOTICE, "%s: Stop", __FUNCTION__);

  {
    CLog::Log(LOGNOTICE, "%s: Showing %s window", __FUNCTION__, CCompileInfo::GetAppName());
    g_Windowing.Show();
  }

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


BOOL CEnigma2Player::ExecuteAppLinux(const char* strSwitches)
{
  CLog::Log(LOGNOTICE, "%s: %s", __FUNCTION__, strSwitches);

  bool remoteUsed = CInputManager::GetInstance().IsRemoteControlEnabled();
  CInputManager::GetInstance().DisableRemoteControl();

  FILE *f;
  if ((f = popen(strSwitches, "r")) == NULL)
  {
    CLog::Log(LOGNOTICE, "%s: Failure popen: %s", __FUNCTION__, strerror(errno));
    return false;
  }

  char line[1024];
  while (fgets(line, sizeof(line), f) != NULL)
  {
    handleAppLinuxOutput(line);
  }
  int ret = pclose(f);

  if (remoteUsed)
    CInputManager::GetInstance().EnableRemoteControl();

  if (ret != 0)
  {
    CLog::Log(LOGNOTICE, "%s: Failure: %d", __FUNCTION__, ret);
  }

  return ret == 0;
}

void CEnigma2Player::Pause()
{
}

bool CEnigma2Player::IsPaused() const
{
  return false;
}

bool CEnigma2Player::HasVideo() const
{
  return true;
}

bool CEnigma2Player::HasAudio() const
{
  return false;
}

void CEnigma2Player::SwitchToNextLanguage()
{
}

void CEnigma2Player::ToggleSubtitles()
{
}

bool CEnigma2Player::CanSeek()
{
  return false;
}

void CEnigma2Player::Seek(bool bPlus, bool bLargeStep, bool bChapterOverride)
{
}

void CEnigma2Player::GetAudioInfo(std::string& strAudioInfo)
{
  strAudioInfo = "CEnigma2Player:GetAudioInfo";
}

void CEnigma2Player::GetVideoInfo(std::string& strVideoInfo)
{
  strVideoInfo = "CEnigma2Player:GetVideoInfo";
}

void CEnigma2Player::GetGeneralInfo(std::string& strGeneralInfo)
{
  strGeneralInfo = "CEnigma2Player:GetGeneralInfo";
}

void CEnigma2Player::SwitchToNextAudioLanguage()
{
}

void CEnigma2Player::SeekPercentage(float iPercent)
{
}

float CEnigma2Player::GetPercentage()
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

void CEnigma2Player::SetAVDelay(float fValue)
{
}

float CEnigma2Player::GetAVDelay()
{
  return 0.0f;
}

void CEnigma2Player::SetSubTitleDelay(float fValue)
{
}

float CEnigma2Player::GetSubTitleDelay()
{
  return 0.0;
}

void CEnigma2Player::AddSubtitle(const std::string& strSubPath)
{
    CLog::Log(LOGDEBUG, "%s: found subtitle : %s", __FUNCTION__, strSubPath.c_str());
    m_subs.push_back(strSubPath.c_str());
};


void CEnigma2Player::SeekTime(int64_t iTime)
{
}

int64_t CEnigma2Player::GetTime() // in millis
{
  return (int64_t)m_time * 1000;
}

int64_t CEnigma2Player::GetTotalTime() // in milliseconds
{
  return (int64_t)m_totalTime * 1000;
}

void CEnigma2Player::ToFFRW(int iSpeed)
{
  m_speed = iSpeed;
}

void CEnigma2Player::ShowOSD(bool bOnoff)
{
}

std::string CEnigma2Player::GetPlayerState()
{
  return "";
}

bool CEnigma2Player::SetPlayerState(const std::string& state)
{
  return true;
}

bool CEnigma2Player::Initialize(TiXmlElement* pConfig)
{
  XMLUtils::GetString(pConfig, "filename", m_filename);
  if (m_filename.length() > 0)
  {
    CLog::Log(LOGNOTICE, "ExternalPlayer Filename: %s", m_filename.c_str());
  }
  else
  {
    std::string xml;
    xml<<*pConfig;
    CLog::Log(LOGERROR, "ExternalPlayer Error: filename element missing from: %s", xml.c_str());
    return false;
  }

  XMLUtils::GetString(pConfig, "filenamemeta", m_filenameMeta);
  if (m_filenameMeta.length() > 0)
  {
    CLog::Log(LOGNOTICE, "ExternalPlayer FilenameMeta: %s", m_filenameMeta.c_str());
  }

  XMLUtils::GetString(pConfig, "args", m_args);
  XMLUtils::GetBoolean(pConfig, "playonestackitem", m_playOneStackItem);
  XMLUtils::GetBoolean(pConfig, "islauncher", m_islauncher);
  XMLUtils::GetBoolean(pConfig, "hidexbmc", m_hidexbmc);
  if (!XMLUtils::GetBoolean(pConfig, "hideconsole", m_hideconsole))
  {
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
      CLog::Log(LOGWARNING, "ExternalPlayer: invalid value for warpcursor: %s", warpCursor.c_str());
    }
  }

  XMLUtils::GetInt(pConfig, "playcountminimumtime", m_playCountMinTime, 1, INT_MAX);

  CLog::Log(LOGNOTICE, "ExternalPlayer Tweaks: hideconsole (%s), hidexbmc (%s), islauncher (%s), warpcursor (%s)",
          m_hideconsole ? "true" : "false",
          m_hidexbmc ? "true" : "false",
          m_islauncher ? "true" : "false",
          warpCursor.c_str());

  TiXmlElement* pReplacers = pConfig->FirstChildElement("replacers");
  while (pReplacers)
  {
    GetCustomRegexpReplacers(pReplacers, m_filenameReplacers);
    pReplacers = pReplacers->NextSiblingElement("replacers");
  }

  return true;
}

void CEnigma2Player::GetCustomRegexpReplacers(TiXmlElement *pRootElement,
                                               std::vector<std::string>& settings)
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

      std::string strMatch;
      std::string strPat;
      std::string strRep;
      XMLUtils::GetString(pReplacer,"match",strMatch);
      XMLUtils::GetString(pReplacer,"pat",strPat);
      XMLUtils::GetString(pReplacer,"rep",strRep);

      if (!strPat.empty() && !strRep.empty())
      {
        CLog::Log(LOGDEBUG,"  Registering replacer:");
        CLog::Log(LOGDEBUG,"    Match:[%s] Pattern:[%s] Replacement:[%s]", strMatch.c_str(), strPat.c_str(), strRep.c_str());
        CLog::Log(LOGDEBUG,"    Global:[%s] Stop:[%s]", bGlobal?"true":"false", bStop?"true":"false");
        // keep literal commas since we use comma as a seperator
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

void CEnigma2Player::handleAppLinuxOutput(const std::string& output)
{
  //CLog::Log(LOGDEBUG, "%s: output: %s", __FUNCTION__, output.c_str());
  const CVariant variantOutput = CJSONVariantParser::Parse((unsigned char*)output.c_str(), output.length());
  if (!variantOutput["position"].isNull())
    m_time = variantOutput["position"].asInteger(0);
  if (!variantOutput["duration"].isNull())
    m_totalTime = variantOutput["duration"].asInteger(0);
  //CLog::Log(LOGDEBUG, "%s: position: %d, duration: %d", __FUNCTION__, m_time, m_totalTime);
}
