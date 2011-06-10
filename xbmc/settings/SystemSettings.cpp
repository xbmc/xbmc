/*
 *      Copyright (C) 2005-2011 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "SystemSettings.h"
#include "filesystem/SpecialProtocol.h"
#include "settings/Settings.h"
#include "settings/GUISettings.h"
#include "utils/LangCodeExpander.h"
#include "utils/log.h"
#include "utils/URIUtils.h"
#include "utils/XMLUtils.h"

CSystemSettings::CSystemSettings(bool isStandalone)
{
  Initialise(isStandalone);
}

CSystemSettings::CSystemSettings(bool isStandalone, TiXmlElement *pRootElement)
{
  Initialise(isStandalone);
  TiXmlElement *pElement = pRootElement->FirstChildElement("network");
  if (pElement)
  {
    XMLUtils::GetInt(pElement, "autodetectpingtime", m_autoDetectPingTime, 1, 240);
    XMLUtils::GetInt(pElement, "curlclienttimeout", m_curlConnectTimeout, 1, 1000);
    XMLUtils::GetInt(pElement, "curllowspeedtime", m_curlLowSpeedTime, 1, 1000);
    XMLUtils::GetInt(pElement, "curlretries", m_curlRetries, 0, 10);
    XMLUtils::GetBoolean(pElement,"disableipv6", m_curlDisableIPV6);
    XMLUtils::GetUInt(pElement, "cachemembuffersize", m_cacheMemBufferSize);
  }

  pElement = pRootElement->FirstChildElement("jsonrpc");
  if (pElement)
  {
    XMLUtils::GetBoolean(pElement, "compactoutput", m_jsonOutputCompact);
    XMLUtils::GetUInt(pElement, "tcpport", m_jsonTcpPort);
  }

  pElement = pRootElement->FirstChildElement("samba");
  if (pElement)
  {
    XMLUtils::GetString(pElement,  "doscodepage", m_sambaDOSCodePage);
    XMLUtils::GetInt(pElement, "clienttimeout", m_sambaClientTimeout, 5, 100);
    XMLUtils::GetBoolean(pElement, "statfiles", m_sambaStatFiles);
  }

  pElement = pRootElement->FirstChildElement("httpdirectory");
  if (pElement)
    XMLUtils::GetBoolean(pElement, "statfilesize", m_HTTPDirectoryStatFilesize);

  pElement = pRootElement->FirstChildElement("ftp");
  if (pElement)
  {
    XMLUtils::GetBoolean(pElement, "remotethumbs", m_FTPThumbs);
  }

  pElement = pRootElement->FirstChildElement("loglevel");
  if (pElement)
  { // read the loglevel setting, so set the setting advanced to hide it in GUI
    // as altering it will do nothing - we don't write to advancedsettings.xml
    XMLUtils::GetInt(pRootElement, "loglevel", m_logLevelHint, LOG_LEVEL_NONE, LOG_LEVEL_MAX);
    CSettingBool *setting = (CSettingBool *)g_guiSettings.GetSetting("debug.showloginfo");
    if (setting)
    {
      const char* hide;
      if (!((hide = pElement->Attribute("hide")) && strnicmp("false", hide, 4) == 0))
        setting->SetAdvanced();
    }
    m_logLevel = std::max(m_logLevel, m_logLevelHint);
    CLog::SetLogLevel(m_logLevel);
  }

  XMLUtils::GetBoolean(pRootElement, "handlemounting", m_handleMounting);
  XMLUtils::GetBoolean(pRootElement, "nodvdrom", m_noDVDROM);

#ifdef HAS_SDL
  XMLUtils::GetBoolean(pRootElement, "fullscreen", m_startFullScreen);
#endif
  XMLUtils::GetBoolean(pRootElement, "showsplash", m_showSplash);
  XMLUtils::GetBoolean(pRootElement, "canquit", m_canQuit);
  XMLUtils::GetBoolean(pRootElement, "canwindowed", m_canWindowed);
  XMLUtils::GetUInt(pRootElement, "restrictcapsmask", m_restrictCapsMask);
  XMLUtils::GetFloat(pRootElement, "sleepbeforeflip", m_sleepBeforeFlip, 0.0f, 1.0f);
  XMLUtils::GetBoolean(pRootElement, "virtualshares", m_useVirtualShares);
  XMLUtils::GetBoolean(pRootElement, "rootovershoot",m_useEvilB);
  // TODO: Should cache path be given in terms of our predefined paths??
  //       Are we even going to have predefined paths??
  CSettings::GetPath(pRootElement, "cachepath", m_cachePath);
  URIUtils::AddSlashAtEnd(m_cachePath);
  g_LangCodeExpander.LoadUserCodes(pRootElement->FirstChildElement("languagecodes"));

  // path substitutions
  TiXmlElement* pPathSubstitution = pRootElement->FirstChildElement("pathsubstitution");
  if (pPathSubstitution)
  {
    m_pathSubstitutions.clear();
    CLog::Log(LOGDEBUG,"Configuring path substitutions");
    TiXmlNode* pSubstitute = pPathSubstitution->FirstChildElement("substitute");
    while (pSubstitute)
    {
      CStdString strFrom, strTo;
      TiXmlNode* pFrom = pSubstitute->FirstChild("from");
      if (pFrom)
        strFrom = _P(pFrom->FirstChild()->Value()).c_str();
      TiXmlNode* pTo = pSubstitute->FirstChild("to");
      if (pTo)
        strTo = pTo->FirstChild()->Value();

      if (!strFrom.IsEmpty() && !strTo.IsEmpty())
      {
        CLog::Log(LOGDEBUG,"  Registering substition pair:");
        CLog::Log(LOGDEBUG,"    From: [%s]", strFrom.c_str());
        CLog::Log(LOGDEBUG,"    To:   [%s]", strTo.c_str());
        m_pathSubstitutions.push_back(make_pair(strFrom,strTo));
      }
      else
      {
        // error message about missing tag
        if (strFrom.IsEmpty())
          CLog::Log(LOGERROR,"  Missing <from> tag");
        else
          CLog::Log(LOGERROR,"  Missing <to> tag");
      }

      // get next one
      pSubstitute = pSubstitute->NextSiblingElement("substitute");
    }
  }

  XMLUtils::GetString(pRootElement, "cputempcommand", m_cpuTempCmd);
  XMLUtils::GetString(pRootElement, "gputempcommand", m_gpuTempCmd);

  XMLUtils::GetBoolean(pRootElement, "alwaysontop", m_alwaysOnTop);

  XMLUtils::GetInt(pRootElement, "bginfoloadermaxthreads", m_bgInfoLoaderMaxThreads);
  m_bgInfoLoaderMaxThreads = std::max(1, m_bgInfoLoaderMaxThreads);

  pElement = pRootElement->FirstChildElement("enablemultimediakeys");
  if (pElement)
  {
    XMLUtils::GetBoolean(pRootElement, "enablemultimediakeys", m_enableMultimediaKeys);
  }
}

int CSystemSettings::AutoDetectPingTime()
{
  return m_autoDetectPingTime;
}

int CSystemSettings::CurlConnectTimeout()
{
  return m_curlConnectTimeout;
}

int CSystemSettings::CurlLowSpeedTime()
{
  return m_curlLowSpeedTime;
}

int CSystemSettings::CurlRetries()
{
  return m_curlRetries;
}

int CSystemSettings::SambaClientTimeout()
{
  return m_sambaClientTimeout;
}

void CSystemSettings::SetLogLevel(int logLevel)
{
  m_logLevel = logLevel;
}

int CSystemSettings::LogLevel()
{
  return m_logLevel;
}

void CSystemSettings::SetLogLevelAndHint(int logLevelHint)
{
  SetLogLevel(logLevelHint);
  m_logLevelHint = logLevelHint;
}

int CSystemSettings::LogLevelHint()
{
  return m_logLevelHint;
}

int CSystemSettings::BGInfoLoaderMaxThreads()
{
  return m_bgInfoLoaderMaxThreads;
}

unsigned int CSystemSettings::CacheMemBufferSize()
{
  return m_cacheMemBufferSize;
}

unsigned int CSystemSettings::JSONTCPPort()
{
  return m_jsonTcpPort;
}

unsigned int CSystemSettings::RestrictCapsMask()
{
  return m_restrictCapsMask;
}

float CSystemSettings::SleepBeforeSlip()
{
  return m_sleepBeforeFlip;
}

bool CSystemSettings::CurlDisableIPV6()
{
  return m_curlDisableIPV6;
}

bool CSystemSettings::OutputCompactJSON()
{
  return m_jsonOutputCompact;
}

bool CSystemSettings::SambaStatFiles()
{
  return m_sambaStatFiles;
}

bool CSystemSettings::HTTPDirectoryStatFilesize()
{
  return m_HTTPDirectoryStatFilesize;
}

bool CSystemSettings::FTPThumbs()
{
  return m_FTPThumbs;
}

void CSystemSettings::SetHandleMounting(bool handleMounting)
{
  m_handleMounting = handleMounting;
}

bool CSystemSettings::HandleMounting()
{
  return m_handleMounting;
}

bool CSystemSettings::NoDVDROM()
{
  return m_noDVDROM;
}

bool CSystemSettings::CanQuit()
{
  return m_canQuit;
}

bool CSystemSettings::CanWindowed()
{
  return m_canWindowed;
}

bool CSystemSettings::ShowSplash()
{
  return m_showSplash;
}

bool CSystemSettings::StartFullScreen()
{
  return m_startFullScreen;
}

void CSystemSettings::SetStartFullScreen(bool startFullScreen)
{
  m_startFullScreen = startFullScreen;
}

bool CSystemSettings::UseVirtualShares()
{
  return m_useVirtualShares;
}

bool CSystemSettings::UseEvilB()
{
  return m_useEvilB;
}

bool CSystemSettings::AlwaysOnTop()
{
  return m_alwaysOnTop;
}

bool CSystemSettings::EnableMultimediaKeys()
{
  return m_enableMultimediaKeys;
}

CStdString CSystemSettings::SambaDOSCodePage()
{
  return m_sambaDOSCodePage;
}

CStdString CSystemSettings::CachePath()
{
  return m_cachePath;
}

CStdString CSystemSettings::CPUTempCMD()
{
  return m_cpuTempCmd;
}

CStdString CSystemSettings::GPUTempCMD()
{
  return m_gpuTempCmd;
}

void CSystemSettings::Initialise(bool isStandalone)
{
  m_cacheMemBufferSize = 1024 * 1024 * 20;
  m_jsonOutputCompact = true;
  m_jsonTcpPort = 9090;
  m_enableMultimediaKeys = false;
  m_canQuit = true;
  m_canWindowed = true;
  m_showSplash = true;
  m_cpuTempCmd = "";
  m_gpuTempCmd = "";
#ifdef __APPLE__
  // default for osx is fullscreen always on top
  m_alwaysOnTop = true;
#else
  // default for windows is not always on top
  m_alwaysOnTop = false;
#endif

  m_bgInfoLoaderMaxThreads = 5;
  m_restrictCapsMask = 0;
  m_sleepBeforeFlip = 0;
  m_useVirtualShares = true;
  m_startFullScreen = false;

  m_curlConnectTimeout = 10;
  m_curlLowSpeedTime = 20;
  m_curlRetries = 2;
  m_curlDisableIPV6 = false;      //Certain hardware/OS combinations have trouble
                                  //with ipv6.

  m_useEvilB = true;
  m_sambaClientTimeout = 10;
  m_sambaDOSCodePage = "";
  m_sambaStatFiles = true;
  m_HTTPDirectoryStatFilesize = false;
  m_FTPThumbs = false;

  m_handleMounting = isStandalone;
  m_noDVDROM = false;
  m_cachePath = "special://temp/";
  m_autoDetectPingTime = 30;
}