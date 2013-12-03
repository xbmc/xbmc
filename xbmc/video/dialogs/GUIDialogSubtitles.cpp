/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#include "system.h"
#include "GUIUserMessages.h"
#include "Application.h"
#include "GUIDialogSubtitles.h"
#include "addons/AddonManager.h"
#include "cores/IPlayer.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "filesystem/AddonsDirectory.h"
#include "filesystem/File.h"
#include "filesystem/PluginDirectory.h"
#include "filesystem/SpecialProtocol.h"
#include "guilib/GUIImage.h"
#include "settings/MediaSettings.h"
#include "settings/Settings.h"
#include "settings/VideoSettings.h"
#include "settings/lib/Setting.h"
#include "utils/JobManager.h"
#include "utils/LangCodeExpander.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "URL.h"
#include "Util.h"
#include "video/VideoDatabase.h"

using namespace ADDON;
using namespace XFILE;

#define CONTROL_NAMELABEL            100
#define CONTROL_NAMELOGO             110
#define CONTROL_SUBLIST              120
#define CONTROL_SUBSEXIST            130
#define CONTROL_SUBSTATUS            140
#define CONTROL_SERVICELIST          150

/*! \brief simple job to retrieve a directory and store a string (language)
 */
class CSubtitlesJob: public CJob
{
public:
  CSubtitlesJob(const CURL &url, const std::string &language) : m_url(url), m_language(language)
  {
    m_items = new CFileItemList;
  }
  virtual ~CSubtitlesJob()
  {
    delete m_items;
  }
  virtual bool DoWork()
  {
    CDirectory::GetDirectory(m_url.Get(), *m_items);
    return true;
  }
  virtual bool operator==(const CJob *job) const
  {
    if (strcmp(job->GetType(),GetType()) == 0)
    {
      const CSubtitlesJob* rjob = dynamic_cast<const CSubtitlesJob*>(job);
      if (rjob)
      {
        return m_url.Get() == rjob->m_url.Get() &&
               m_language == rjob->m_language;
      }
    }
    return false;
  }
  const CFileItemList *GetItems() const { return m_items; }
  const CURL &GetURL() const { return m_url; }
  const std::string &GetLanguage() const { return m_language; }
private:
  CURL           m_url;
  CFileItemList *m_items;
  std::string    m_language;
};

CGUIDialogSubtitles::CGUIDialogSubtitles(void)
    : CGUIDialog(WINDOW_DIALOG_SUBTITLES, "DialogSubtitles.xml")
{
  m_loadType  = KEEP_IN_MEMORY;
  m_subtitles = new CFileItemList;
  m_serviceItems = new CFileItemList;
  m_pausedOnRun = false;
  m_updateSubsList = false;
}

CGUIDialogSubtitles::~CGUIDialogSubtitles(void)
{
  CancelJobs();
  delete m_subtitles;
  delete m_serviceItems;
}

bool CGUIDialogSubtitles::OnMessage(CGUIMessage& message)
{
  if  (message.GetMessage() == GUI_MSG_CLICKED)
  {
    int iControl = message.GetSenderId();

    if (iControl == CONTROL_SUBLIST)
    {
      CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), CONTROL_SUBLIST);
      OnMessage(msg);

      int item = msg.GetParam1();
      if (item >= 0 && item < m_subtitles->Size())
        Download(*m_subtitles->Get(item));
      return true;
    }
    else if (iControl == CONTROL_SERVICELIST)
    {
      CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), CONTROL_SERVICELIST);
      OnMessage(msg);

      int item = msg.GetParam1();
      if (item >= 0 && item < m_serviceItems->Size() &&
          SetService(m_serviceItems->Get(item)->GetProperty("Addon.ID").asString()))
        Search();

      return true;
    }
  }
  else if (message.GetMessage() == GUI_MSG_WINDOW_DEINIT)
  {
    // Resume the video if the user has requested it
    if (g_application.m_pPlayer->IsPaused() && m_pausedOnRun)
      g_application.m_pPlayer->Pause();

    CGUIDialog::OnMessage(message);

    ClearSubtitles();
    ClearServices();
    return true;
  }
  return CGUIDialog::OnMessage(message);
}

void CGUIDialogSubtitles::OnInitWindow()
{
  // Pause the video if the user has requested it
  m_pausedOnRun = false;
  if (CSettings::Get().GetBool("subtitles.pauseonsearch") && !g_application.m_pPlayer->IsPaused())
  {
    g_application.m_pPlayer->Pause();
    m_pausedOnRun = true;
  }

  FillServices();
  CGUIWindow::OnInitWindow();
  Search();
}

void CGUIDialogSubtitles::Process(unsigned int currentTime, CDirtyRegionList &dirtyregions)
{
  if (m_bInvalidated)
  {
    // take copies of our variables to ensure we don't hold the lock for long.
    std::string status;
    CFileItemList subs;
    {
      CSingleLock lock(m_section);
      status = m_status;
      subs.Assign(*m_subtitles);
    }
    SET_CONTROL_LABEL(CONTROL_SUBSTATUS, status);

    if (m_updateSubsList)
    {
      CGUIMessage message(GUI_MSG_LABEL_BIND, GetID(), CONTROL_SUBLIST, 0, 0, &subs);
      OnMessage(message);
      m_updateSubsList = false;
    }

    if (!m_subtitles->IsEmpty())
    { // set focus to the list
      CGUIMessage msg(GUI_MSG_SETFOCUS, GetID(), CONTROL_SUBLIST);
      OnMessage(msg);
    }
    else
    { // set focus to the service list if no subs are found
      CGUIMessage msg(GUI_MSG_SETFOCUS, GetID(), CONTROL_SERVICELIST);
      OnMessage(msg);
    }
  }
  CGUIDialog::Process(currentTime, dirtyregions);
}

void CGUIDialogSubtitles::FillServices()
{
  ClearServices();

  VECADDONS addons;
  ADDON::CAddonMgr::Get().GetAddons(ADDON_SUBTITLE_MODULE, addons, true);

  if (addons.empty())
  {
    UpdateStatus(NO_SERVICES);
    return;
  }

  std::string defaultService;
  const CFileItem &item = g_application.CurrentFileItem();
  if (item.GetVideoContentType() == VIDEODB_CONTENT_TVSHOWS ||
      item.GetVideoContentType() == VIDEODB_CONTENT_EPISODES)
    // Set default service for tv shows
    defaultService = CSettings::Get().GetString("subtitles.tv");
  else
    // Set default service for filemode and movies
    defaultService = CSettings::Get().GetString("subtitles.movie");
  
  std::string service = addons.front()->ID();
  for (VECADDONS::const_iterator addonIt = addons.begin(); addonIt != addons.end(); addonIt++)
  {
    CFileItemPtr item(CAddonsDirectory::FileItemFromAddon(*addonIt, "plugin://", false));
    m_serviceItems->Add(item);
    if ((*addonIt)->ID() == defaultService)
      service = (*addonIt)->ID();
  }

  // Bind our services to the UI
  CGUIMessage msg(GUI_MSG_LABEL_BIND, GetID(), CONTROL_SERVICELIST, 0, 0, m_serviceItems);
  OnMessage(msg);

  SetService(service);
}

bool CGUIDialogSubtitles::SetService(const std::string &service)
{
  if (service != m_currentService)
  {
    m_currentService = service;
    CLog::Log(LOGDEBUG, "New Service [%s] ", m_currentService.c_str());

    CFileItemPtr currentService = GetService();
    // highlight this item in the skin
    for (int i = 0; i < m_serviceItems->Size(); i++)
    {
      CFileItemPtr pItem = m_serviceItems->Get(i);
      pItem->Select(pItem == currentService);
    }

    SET_CONTROL_LABEL(CONTROL_NAMELABEL, currentService->GetLabel());

    CGUIImage* image = (CGUIImage*)GetControl(CONTROL_NAMELOGO);
    if (image)
    {
      std::string icon = URIUtils::AddFileToFolder(currentService->GetProperty("Addon.Path").asString(), "logo.png");
      image->SetFileName(icon);
    }
    if (g_application.m_pPlayer->GetSubtitleCount() == 0)
      SET_CONTROL_HIDDEN(CONTROL_SUBSEXIST);
    else
      SET_CONTROL_VISIBLE(CONTROL_SUBSEXIST);

    return true;
  }
  return false;
}

const CFileItemPtr CGUIDialogSubtitles::GetService() const
{
  for (int i = 0; i < m_serviceItems->Size(); i++)
  {
    if (m_serviceItems->Get(i)->GetProperty("Addon.ID") == m_currentService)
      return m_serviceItems->Get(i);
  }
  return CFileItemPtr();
}

void CGUIDialogSubtitles::Search()
{
  if (m_currentService.empty())
    return; // no services available

  UpdateStatus(SEARCHING);
  ClearSubtitles();

  CURL url("plugin://" + m_currentService + "/");
  url.SetOption("action", "search");

  const CSetting *setting = CSettings::Get().GetSetting("subtitles.languages");
  if (setting)
    url.SetOption("languages", setting->ToString());

  AddJob(new CSubtitlesJob(url, ""));
}

void CGUIDialogSubtitles::OnJobComplete(unsigned int jobID, bool success, CJob *job)
{
  const CURL &url             = ((CSubtitlesJob *)job)->GetURL();
  const CFileItemList *items  = ((CSubtitlesJob *)job)->GetItems();
  const std::string &language = ((CSubtitlesJob *)job)->GetLanguage();
  if (url.GetOption("action") == "search")
    OnSearchComplete(items);
  else
    OnDownloadComplete(items, language);
  CJobQueue::OnJobComplete(jobID, success, job);
}

void CGUIDialogSubtitles::OnSearchComplete(const CFileItemList *items)
{
  CSingleLock lock(m_section);
  m_subtitles->Assign(*items);
  UpdateStatus(SEARCH_COMPLETE);
  m_updateSubsList = true;
  SetInvalid();
}

void CGUIDialogSubtitles::UpdateStatus(STATUS status)
{
  CSingleLock lock(m_section);
  std::string label;
  switch (status)
  {
    case NO_SERVICES:
      label = g_localizeStrings.Get(24114);
      break;
    case SEARCHING:
      label = g_localizeStrings.Get(24107);
      break;
    case SEARCH_COMPLETE:
      if (!m_subtitles->IsEmpty())
        label = StringUtils::Format(g_localizeStrings.Get(24108).c_str(), m_subtitles->Size());
      else
        label = g_localizeStrings.Get(24109);
      break;
    case DOWNLOADING:
      label = g_localizeStrings.Get(24110);
      break;
    default:
      break;
  }
  if (label != m_status)
  {
    m_status = label;
    SetInvalid();
  }
}

void CGUIDialogSubtitles::Download(const CFileItem &subtitle)
{
  UpdateStatus(DOWNLOADING);

  // subtitle URL should be of the form plugin://<addonid>/?param=foo&param=bar
  // we just append (if not already present) the action=download parameter.
  CURL url(subtitle.GetAsUrl());
  if (url.GetOption("action").empty())
    url.SetOption("action", "download");

  AddJob(new CSubtitlesJob(url, subtitle.GetLabel()));
}

void CGUIDialogSubtitles::OnDownloadComplete(const CFileItemList *items, const std::string &language)
{
  if (items->IsEmpty())
  {
    CFileItemPtr service = GetService();
    if (service)
      CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Error, service->GetLabel(), g_localizeStrings.Get(24113));
    UpdateStatus(SEARCH_COMPLETE);
    return;
  }

  CStdString strFileName;
  CStdString strDestPath;
  if (g_application.CurrentFileItem().IsStack())
  {
    for (int i = 0; i < items->Size(); i++)
    {
//    check for all stack items and match to given subs, item [0] == CD1, item [1] == CD2
//    CLog::Log(LOGDEBUG, "Stack Subs [%s} Found", vecItems[i]->GetLabel().c_str());
    }
  }
  else if (StringUtils::StartsWith(g_application.CurrentFile(), "http://"))
  {
    strFileName = "TemporarySubs";
    strDestPath = "special://temp/";
  }
  else
  {
    strFileName = URIUtils::GetFileName(g_application.CurrentFile());
    if (CSettings::Get().GetBool("subtitles.savetomoviefolder"))
    {
      strDestPath = URIUtils::GetDirectory(g_application.CurrentFile());
      if (!CUtil::SupportsWriteFileOperations(strDestPath))
        strDestPath.clear();
    }
    if (strDestPath.empty())
    {
      if (CSpecialProtocol::TranslatePath("special://subtitles").empty())
        strDestPath = "special://temp";
      else
        strDestPath = "special://subtitles";
    }
  }
  // Extract the language and appropriate extension
  CStdString strSubLang;
  g_LangCodeExpander.ConvertToTwoCharCode(strSubLang, language);
  CStdString strUrl = items->Get(0)->GetPath();
  CStdString strSubExt = URIUtils::GetExtension(strUrl);

  // construct subtitle path
  URIUtils::RemoveExtension(strFileName);
  CStdString strSubName = StringUtils::Format("%s.%s%s", strFileName.c_str(), strSubLang.c_str(), strSubExt.c_str());
  CStdString strSubPath = URIUtils::AddFileToFolder(strDestPath, strSubName);

  // and copy the file across
  CFile::Cache(strUrl, strSubPath);
  SetSubtitles(strSubPath);
  // Close the window
  Close();
}

void CGUIDialogSubtitles::ClearSubtitles()
{
  CGUIMessage msg(GUI_MSG_LABEL_RESET, GetID(), CONTROL_SUBLIST);
  OnMessage(msg);
  CSingleLock lock(m_section);
  m_subtitles->Clear();
}

void CGUIDialogSubtitles::ClearServices()
{
  CGUIMessage msg(GUI_MSG_LABEL_RESET, GetID(), CONTROL_SERVICELIST);
  OnMessage(msg);
  m_serviceItems->Clear();
  m_currentService.clear();
}

void CGUIDialogSubtitles::SetSubtitles(const std::string &subtitle)
{
  if (g_application.m_pPlayer)
  {
    int nStream = g_application.m_pPlayer->AddSubtitle(subtitle);
    if(nStream >= 0)
    {
      g_application.m_pPlayer->SetSubtitle(nStream);
      g_application.m_pPlayer->SetSubtitleVisible(true);
      CMediaSettings::Get().GetCurrentVideoSettings().m_SubtitleDelay = 0.0f;
      g_application.m_pPlayer->SetSubTitleDelay(0);
    }
  }
}
