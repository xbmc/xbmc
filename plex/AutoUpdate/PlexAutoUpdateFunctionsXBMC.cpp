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
#include "filesystem/File.h"
#include "GUIWindowManager.h"
#include "filesystem/SpecialProtocol.h"

#include "URL.h"
#include "dialogs/GUIDialogYesNo.h"
#include "JobManager.h"
#include "Utility/PlexDownloadFileJob.h"
#include "ApplicationMessenger.h"

#include "filesystem/ZipFile.h"

#ifdef _LINUX
#include <unistd.h>
#include <stdlib.h>
#endif

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

std::string CAutoUpdateFunctionsXBMC::GetLocalFileName(const std::string& baseName)
{
  return CSpecialProtocol::TranslatePath(string("special://home/") + baseName);
}

void
CAutoUpdateFunctionsXBMC::OnJobComplete(unsigned int jobID, bool success, CJob *job)
{
  if (jobID == m_jobId)
  {
    m_downloadingDone = true;
    m_downloadSuccess = success;
  }
}

void
CAutoUpdateFunctionsXBMC::OnJobProgress(unsigned int jobID, unsigned int progress, unsigned int total, const CJob *job)
{
  if (jobID == m_jobId && m_progressDialog)
    m_progressDialog->SetPercentage((float)((float)progress/(float)total) * 100.0);
}

bool
CAutoUpdateFunctionsXBMC::DownloadFile(const string &url, string &localPath)
{
  CURL theUrl(url);
  localPath = GetLocalFileName(theUrl.GetFileNameWithoutPath());
  
  struct stat mStat;
  if (CFile::Stat(string("file://") + localPath, &mStat) != -1)
  {
    return true;
  }
  
  m_downloadingDone = false;
  CPlexDownloadFileJob* downloadJob = new CPlexDownloadFileJob(url, localPath);
  m_jobId = CJobManager::GetInstance().AddJob(downloadJob, this);

  m_progressDialog = (CGUIDialogProgress*)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
  if (m_progressDialog)
  {
    m_progressDialog->SetHeading(13413); // Downloading
    m_progressDialog->SetLine(0, "");
    m_progressDialog->SetLine(1, "Downloading update");
    m_progressDialog->SetLine(2, "");
    m_progressDialog->SetPercentage(0);
    m_progressDialog->StartModal();
    m_progressDialog->ShowProgressBar(true);
  }

  while(!m_downloadingDone)
  {
    m_progressDialog->Progress();
    if (m_progressDialog->IsCanceled())
    {
      CJobManager::GetInstance().CancelJob(m_jobId);
      m_downloadSuccess = false;
      break;
    }
  }

  CApplicationMessenger::Get().Close(m_progressDialog, false, false);
  m_progressDialog = NULL;

  return m_downloadSuccess;
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

bool
CAutoUpdateFunctionsXBMC::ShouldWeInstall(const std::string &localPath)
{
  bool cancel;
  CGUIDialogYesNo::ShowAndGetInput("Download finished", "", "Do you want to quit Plex and show the download folder?", "", cancel, "No", "Yes, do it!");
  return !cancel;
}

std::string
CAutoUpdateFunctionsXBMC::GetResourcePath() const
{
  return CSpecialProtocol::TranslatePath("special://xbmcbin");
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
  CStdString line1("Version " + m_updater->GetNewVersion().m_enclosureVersion.GetVersionString() + " is available, would you like to download it?");
  CStdString line2("You current version is " + m_updater->GetCurrentVersion().GetVersionString());
  bool cancel;
  CGUIDialogYesNo::ShowAndGetInput("New version available", line1, line2, "", cancel, "Cancel", "Download");
  if (!cancel)
  {
    m_updater->DownloadNewVersion();
  }
}
