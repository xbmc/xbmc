//
//  PlexAutoUpdateFunctionsXBMC.cpp
//  Plex
//
//  Created by Tobias Hieta <tobias@plexapp.com> on 2012-12-09.
//  Copyright 2012 Plex Inc. All rights reserved.
//

#include "AutoUpdate/PlexAutoUpdateFunctionsXBMC.h"
#include "utils/XBMCTinyXML.h"
#include "filesystem/CurlFile.h"
#include "Utils/log.h"

#include "dialogs/GUIDialogYesNo.h"

using namespace XFILE;
using namespace std;

bool
CAutoUpdateFunctionsXBMC::FetchUrlData(const std::string &url, std::string &data)
{
  CCurlFile http;
  CStdString htmlData;
  http.SetRequestHeader("X-Plex-Client", "Plex Media Center");
  bool ret = http.Get(url, htmlData);
  data = htmlData;
  return ret;
}

bool
CAutoUpdateFunctionsXBMC::ParseItemElement(TiXmlElement *el, CAutoUpdateInfo& info)
{
  TiXmlElement* title = el->FirstChildElement("title");
  if (title)
    info.m_title = title->GetText();

  TiXmlElement* relnotes = el->FirstChildElement("sparkle:releasenoteslink");
  if (relnotes)
    info.m_relnoteUrl = relnotes->GetText();

  TiXmlElement* date = el->FirstChildElement("pubdate");
  if (date)
    info.m_date = date->GetText();

  /* Multiple enclosures ? */
  TiXmlElement* enclosure = el->FirstChildElement("enclosure");
  if (enclosure)
  {
    const char* url = enclosure->Attribute("url");
    if (url)
      info.m_enclosureUrl = url;

    const char* size = enclosure->Attribute("length");
    if (size)
      info.m_enclosureSize = boost::lexical_cast<int64_t>(size);

    const char* version = enclosure->Attribute("sparkle:version");
    if (version)
      info.m_enclosureVersion = CAutoUpdateInfoVersion(version);
  }

  return true;
}

bool
CAutoUpdateFunctionsXBMC::ParseXMLData(const std::string &xmlData, CAutoUpdateInfoList &infoList)
{
  TiXmlDocument doc;
  doc.Parse(xmlData.c_str());

  TiXmlElement *root = doc.RootElement();
  if (root == 0)
  {
    LogInfo("Failed to parse XML document...");
    return false;
  }

  /* Channel element */
  TiXmlElement *el = root->FirstChildElement("channel");
  if (el == 0)
  {
    LogInfo("Failed to find channel element");
    return false;
  }

  /* Find the item */
  TiXmlElement* item = el->FirstChildElement("item");
  if (item == 0)
  {
    LogInfo("Failed to find item element");
    return false;
  }

  while(item)
  {
    CAutoUpdateInfo info;
    if (ParseItemElement(item, info))
    {
      infoList.push_back(info);
    }
    item = item->NextSiblingElement();
  }

  return true;
}

void
CAutoUpdateFunctionsXBMC::LogDebug(const std::string& msg)
{
  CLog::Log(LOGDEBUG, "[AutoUpdate]: %s", msg.c_str());
}

void
CAutoUpdateFunctionsXBMC::LogInfo(const std::string& msg)
{
  CLog::Log(LOGINFO, "[AutoUpdate]: %s", msg.c_str());
}

void
CAutoUpdateFunctionsXBMC::NotifyNewVersion()
{
  CStdString line1("Version " + m_updater->GetNewVersion().m_enclosureVersion.GetVersionString() + " is available, would you like to install it?");
  CStdString line2("You current version is " + m_updater->GetCurrentVersion().GetVersionString());
  bool cancel;
  CGUIDialogYesNo::ShowAndGetInput("New version available", line1, line2, "", cancel, "Cancel", "Install");
  if (!cancel)
  {
    m_updater->DownloadNewVersion();
  }
}
