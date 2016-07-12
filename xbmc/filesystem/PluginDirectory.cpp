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

#include "Application.h"
#include "threads/SystemClock.h"
#include "system.h"
#include "PluginDirectory.h"
#include "addons/AddonManager.h"
#include "addons/AddonInstaller.h"
#include "addons/IAddon.h"
#include "interfaces/generic/ScriptInvocationManager.h"
#include "threads/SingleLock.h"
#include "guilib/GUIWindowManager.h"
#include "dialogs/GUIDialogBusy.h"
#include "settings/Settings.h"
#include "FileItem.h"
#include "video/VideoInfoTag.h"
#include "utils/log.h"
#include "utils/JobManager.h"
#include "utils/StringUtils.h"
#include "messaging/ApplicationMessenger.h"
#include "URL.h"

using namespace XFILE;
using namespace ADDON;
using namespace KODI::MESSAGING;

std::map<int, CPluginDirectory *> CPluginDirectory::globalHandles;
int CPluginDirectory::handleCounter = 0;
CCriticalSection CPluginDirectory::m_handleLock;

CPluginDirectory::CScriptObserver::CScriptObserver(int scriptId, CEvent &event) :
  CThread("scriptobs"), m_scriptId(scriptId), m_event(event)
{
  Create();
}

void CPluginDirectory::CScriptObserver::Process()
{
  while (!m_bStop)
  {
    if (!CScriptInvocationManager::GetInstance().IsRunning(m_scriptId))
    {
      m_event.Set();
      break;
    }
    Sleep(20);
  }
}

void CPluginDirectory::CScriptObserver::Abort()
{
  m_bStop = true;
}

CPluginDirectory::CPluginDirectory()
  : m_fetchComplete(true)
  , m_cancelled(false)
  , m_success(false)
  , m_totalItems(0)
{
  m_listItems = new CFileItemList;
  m_fileResult = new CFileItem;
}

CPluginDirectory::~CPluginDirectory(void)
{
  delete m_listItems;
  delete m_fileResult;
}

int CPluginDirectory::getNewHandle(CPluginDirectory *cp)
{
  CSingleLock lock(m_handleLock);
  int handle = ++handleCounter;
  globalHandles[handle] = cp;
  return handle;
}

void CPluginDirectory::removeHandle(int handle)
{
  CSingleLock lock(m_handleLock);
  if (!globalHandles.erase(handle))
    CLog::Log(LOGWARNING, "Attempt to erase invalid handle %i", handle);
}

CPluginDirectory *CPluginDirectory::dirFromHandle(int handle)
{
  CSingleLock lock(m_handleLock);
  std::map<int, CPluginDirectory *>::iterator i = globalHandles.find(handle);
  if (i != globalHandles.end())
    return i->second;
  CLog::Log(LOGWARNING, "Attempt to use invalid handle %i", handle);
  return NULL;
}

bool CPluginDirectory::StartScript(const std::string& strPath, bool retrievingDir)
{
  CURL url(strPath);

  // try the plugin type first, and if not found, try an unknown type
  if (!CAddonMgr::GetInstance().GetAddon(url.GetHostName(), m_addon, ADDON_PLUGIN) &&
      !CAddonMgr::GetInstance().GetAddon(url.GetHostName(), m_addon, ADDON_UNKNOWN) &&
      !CAddonInstaller::GetInstance().InstallModal(url.GetHostName(), m_addon))
  {
    CLog::Log(LOGERROR, "Unable to find plugin %s", url.GetHostName().c_str());
    return false;
  }

  // get options
  std::string options = url.GetOptions();
  url.SetOptions(""); // do this because we can then use the url to generate the basepath
                      // which is passed to the plugin (and represents the share)

  std::string basePath(url.Get());
  // reset our wait event, and grab a new handle
  m_fetchComplete.Reset();
  int handle = getNewHandle(this);

  // clear out our status variables
  m_fileResult->Reset();
  m_listItems->Clear();
  m_listItems->SetPath(strPath);
  m_listItems->SetLabel(m_addon->Name());
  m_cancelled = false;
  m_success = false;
  m_totalItems = 0;

  // setup our parameters to send the script
  std::string strHandle = StringUtils::Format("%i", handle);
  std::vector<std::string> argv;
  argv.push_back(basePath);
  argv.push_back(strHandle);
  argv.push_back(options);

  // run the script
  CLog::Log(LOGDEBUG, "%s - calling plugin %s('%s','%s','%s')", __FUNCTION__, m_addon->Name().c_str(), argv[0].c_str(), argv[1].c_str(), argv[2].c_str());
  bool success = false;
  std::string file = m_addon->LibPath();
  int id = CScriptInvocationManager::GetInstance().ExecuteAsync(file, m_addon, argv);
  if (id >= 0)
  { // wait for our script to finish
    std::string scriptName = m_addon->Name();
    success = WaitOnScriptResult(file, id, scriptName, retrievingDir);
  }
  else
    CLog::Log(LOGERROR, "Unable to run plugin %s", m_addon->Name().c_str());

  // free our handle
  removeHandle(handle);

  return success;
}

bool CPluginDirectory::GetPluginResult(const std::string& strPath, CFileItem &resultItem)
{
  CURL url(strPath);
  CPluginDirectory newDir;

  bool success = newDir.StartScript(strPath, false);

  if (success)
  { // update the play path and metadata, saving the old one as needed
    if (!resultItem.HasProperty("original_listitem_url"))
      resultItem.SetProperty("original_listitem_url", resultItem.GetPath());
    resultItem.SetPath(newDir.m_fileResult->GetPath());
    resultItem.SetMimeType(newDir.m_fileResult->GetMimeType());
    resultItem.UpdateInfo(*newDir.m_fileResult);
    if (newDir.m_fileResult->HasVideoInfoTag() && newDir.m_fileResult->GetVideoInfoTag()->m_resumePoint.IsSet())
      resultItem.m_lStartOffset = STARTOFFSET_RESUME; // resume point set in the resume item, so force resume
  }

  return success;
}

bool CPluginDirectory::AddItem(int handle, const CFileItem *item, int totalItems)
{
  CSingleLock lock(m_handleLock);
  CPluginDirectory *dir = dirFromHandle(handle);
  if (!dir)
    return false;

  CFileItemPtr pItem(new CFileItem(*item));
  dir->m_listItems->Add(pItem);
  dir->m_totalItems = totalItems;

  return !dir->m_cancelled;
}

bool CPluginDirectory::AddItems(int handle, const CFileItemList *items, int totalItems)
{
  CSingleLock lock(m_handleLock);
  CPluginDirectory *dir = dirFromHandle(handle);
  if (!dir)
    return false;

  CFileItemList pItemList;
  pItemList.Copy(*items);
  dir->m_listItems->Append(pItemList);
  dir->m_totalItems = totalItems;

  return !dir->m_cancelled;
}

void CPluginDirectory::EndOfDirectory(int handle, bool success, bool replaceListing, bool cacheToDisc)
{
  CSingleLock lock(m_handleLock);
  CPluginDirectory *dir = dirFromHandle(handle);
  if (!dir)
    return;

  // set cache to disc
  dir->m_listItems->SetCacheToDisc(cacheToDisc ? CFileItemList::CACHE_IF_SLOW : CFileItemList::CACHE_NEVER);

  dir->m_success = success;
  dir->m_listItems->SetReplaceListing(replaceListing);

  if (!dir->m_listItems->HasSortDetails())
    dir->m_listItems->AddSortMethod(SortByNone, 552, LABEL_MASKS("%L", "%D"));

  // set the event to mark that we're done
  dir->m_fetchComplete.Set();
}

void CPluginDirectory::AddSortMethod(int handle, SORT_METHOD sortMethod, const std::string &label2Mask)
{
  CSingleLock lock(m_handleLock);
  CPluginDirectory *dir = dirFromHandle(handle);
  if (!dir)
    return;

  //! @todo Add all sort methods and fix which labels go on the right or left
  switch(sortMethod)
  {
    case SORT_METHOD_LABEL:
    case SORT_METHOD_LABEL_IGNORE_THE:
      {
        dir->m_listItems->AddSortMethod(SortByLabel, 551, LABEL_MASKS("%T", label2Mask), CSettings::GetInstance().GetBool(CSettings::SETTING_FILELISTS_IGNORETHEWHENSORTING) ? SortAttributeIgnoreArticle : SortAttributeNone);
        break;
      }
    case SORT_METHOD_TITLE:
    case SORT_METHOD_TITLE_IGNORE_THE:
      {
        dir->m_listItems->AddSortMethod(SortByTitle, 556, LABEL_MASKS("%T", label2Mask), CSettings::GetInstance().GetBool(CSettings::SETTING_FILELISTS_IGNORETHEWHENSORTING) ? SortAttributeIgnoreArticle : SortAttributeNone);
        break;
      }
    case SORT_METHOD_ARTIST:
    case SORT_METHOD_ARTIST_IGNORE_THE:
      {
        dir->m_listItems->AddSortMethod(SortByArtist, 557, LABEL_MASKS("%T", "%A"), CSettings::GetInstance().GetBool(CSettings::SETTING_FILELISTS_IGNORETHEWHENSORTING) ? SortAttributeIgnoreArticle : SortAttributeNone);
        break;
      }
    case SORT_METHOD_ALBUM:
    case SORT_METHOD_ALBUM_IGNORE_THE:
      {
        dir->m_listItems->AddSortMethod(SortByAlbum, 558, LABEL_MASKS("%T", "%B"), CSettings::GetInstance().GetBool(CSettings::SETTING_FILELISTS_IGNORETHEWHENSORTING) ? SortAttributeIgnoreArticle : SortAttributeNone);
        break;
      }
    case SORT_METHOD_DATE:
      {
        dir->m_listItems->AddSortMethod(SortByDate, 552, LABEL_MASKS("%T", "%J"));
        break;
      }
    case SORT_METHOD_BITRATE:
      {
        dir->m_listItems->AddSortMethod(SortByBitrate, 623, LABEL_MASKS("%T", "%X"));
        break;
      }             
    case SORT_METHOD_SIZE:
      {
        dir->m_listItems->AddSortMethod(SortBySize, 553, LABEL_MASKS("%T", "%I"));
        break;
      }
    case SORT_METHOD_FILE:
      {
        dir->m_listItems->AddSortMethod(SortByFile, 561, LABEL_MASKS("%T", label2Mask));
        break;
      }
    case SORT_METHOD_TRACKNUM:
      {
        dir->m_listItems->AddSortMethod(SortByTrackNumber, 554, LABEL_MASKS("[%N. ]%T", label2Mask));
        break;
      }
    case SORT_METHOD_DURATION:
    case SORT_METHOD_VIDEO_RUNTIME:
      {
        dir->m_listItems->AddSortMethod(SortByTime, 180, LABEL_MASKS("%T", "%D"));
        break;
      }
    case SORT_METHOD_VIDEO_RATING:
    case SORT_METHOD_SONG_RATING:
      {
        dir->m_listItems->AddSortMethod(SortByRating, 563, LABEL_MASKS("%T", "%R"));
        break;
      }
    case SORT_METHOD_YEAR:
      {
        dir->m_listItems->AddSortMethod(SortByYear, 562, LABEL_MASKS("%T", "%Y"));
        break;
      }
    case SORT_METHOD_GENRE:
      {
        dir->m_listItems->AddSortMethod(SortByGenre, 515, LABEL_MASKS("%T", "%G"));
        break;
      }
    case SORT_METHOD_COUNTRY:
      {
        dir->m_listItems->AddSortMethod(SortByCountry, 574, LABEL_MASKS("%T", "%G"));
        break;
      }
    case SORT_METHOD_VIDEO_TITLE:
      {
        dir->m_listItems->AddSortMethod(SortByTitle, 369, LABEL_MASKS("%T", label2Mask));
        break;
      }
    case SORT_METHOD_VIDEO_SORT_TITLE:
    case SORT_METHOD_VIDEO_SORT_TITLE_IGNORE_THE:
      {
        dir->m_listItems->AddSortMethod(SortBySortTitle, 556, LABEL_MASKS("%T", label2Mask), CSettings::GetInstance().GetBool(CSettings::SETTING_FILELISTS_IGNORETHEWHENSORTING) ? SortAttributeIgnoreArticle : SortAttributeNone);
        break;
      }
    case SORT_METHOD_MPAA_RATING:
      {
        dir->m_listItems->AddSortMethod(SortByMPAA, 20074, LABEL_MASKS("%T", "%O"));
        break;
      }
    case SORT_METHOD_STUDIO:
    case SORT_METHOD_STUDIO_IGNORE_THE:
      {
        dir->m_listItems->AddSortMethod(SortByStudio, 572, LABEL_MASKS("%T", "%U"), CSettings::GetInstance().GetBool(CSettings::SETTING_FILELISTS_IGNORETHEWHENSORTING) ? SortAttributeIgnoreArticle : SortAttributeNone);
        break;
      }
    case SORT_METHOD_PROGRAM_COUNT:
      {
        dir->m_listItems->AddSortMethod(SortByProgramCount, 567, LABEL_MASKS("%T", "%C"));
        break;
      }
    case SORT_METHOD_UNSORTED:
      {
        dir->m_listItems->AddSortMethod(SortByNone, 571, LABEL_MASKS("%T", label2Mask));
        break;
      }
    case SORT_METHOD_NONE:
      {
        dir->m_listItems->AddSortMethod(SortByNone, 552, LABEL_MASKS("%T", label2Mask));
        break;
      }
    case SORT_METHOD_DRIVE_TYPE:
      {
        dir->m_listItems->AddSortMethod(SortByDriveType, 564, LABEL_MASKS()); // Preformatted
        break;
      }
    case SORT_METHOD_PLAYLIST_ORDER:
      {
        std::string strTrack=CSettings::GetInstance().GetString(CSettings::SETTING_MUSICFILES_TRACKFORMAT);
        dir->m_listItems->AddSortMethod(SortByPlaylistOrder, 559, LABEL_MASKS(strTrack, "%D"));
        break;
      }
    case SORT_METHOD_EPISODE:
      {
        dir->m_listItems->AddSortMethod(SortByEpisodeNumber, 20359, LABEL_MASKS("%E. %T","%R"));
        break;
      }
    case SORT_METHOD_PRODUCTIONCODE:
      {
        //dir->m_listItems.AddSortMethod(SORT_METHOD_PRODUCTIONCODE,20368,LABEL_MASKS("%E. %T","%P", "%E. %T","%P"));
        dir->m_listItems->AddSortMethod(SortByProductionCode, 20368, LABEL_MASKS("%H. %T","%P", "%H. %T","%P"));
        break;
      }
    case SORT_METHOD_LISTENERS:
      {
       dir->m_listItems->AddSortMethod(SortByListeners, 20455, LABEL_MASKS("%T","%W"));
       break;
      }
    case SORT_METHOD_DATEADDED:
      {
        dir->m_listItems->AddSortMethod(SortByDateAdded, 570, LABEL_MASKS("%T", "%a"));
        break;
      }
    case SORT_METHOD_FULLPATH:
      {
        dir->m_listItems->AddSortMethod(SortByPath, 573, LABEL_MASKS("%T", label2Mask));
        break;
      }
    case SORT_METHOD_LABEL_IGNORE_FOLDERS:
      {
        dir->m_listItems->AddSortMethod(SortByLabel, SortAttributeIgnoreFolders, 551, LABEL_MASKS("%T", label2Mask));
        break;
      }
    case SORT_METHOD_LASTPLAYED:
      {
        dir->m_listItems->AddSortMethod(SortByLastPlayed, 568, LABEL_MASKS("%T", "%G"));
        break;
      }
    case SORT_METHOD_PLAYCOUNT:
      {
        dir->m_listItems->AddSortMethod(SortByPlaycount, 567, LABEL_MASKS("%T", "%V"));
        break;
      }
    case SORT_METHOD_CHANNEL:
      {
        dir->m_listItems->AddSortMethod(SortByChannel, 19029, LABEL_MASKS("%T", label2Mask));
        break;
      }
   
    default:
      break;
  }
}

bool CPluginDirectory::GetDirectory(const CURL& url, CFileItemList& items)
{
  const std::string pathToUrl(url.Get());
  bool success = StartScript(pathToUrl, true);

  // append the items to the list
  items.Assign(*m_listItems, true); // true to keep the current items
  m_listItems->Clear();
  return success;
}

bool CPluginDirectory::RunScriptWithParams(const std::string& strPath)
{
  CURL url(strPath);
  if (url.GetHostName().empty()) // called with no script - should never happen
    return false;

  AddonPtr addon;
  if (!CAddonMgr::GetInstance().GetAddon(url.GetHostName(), addon, ADDON_PLUGIN) && !CAddonInstaller::GetInstance().InstallModal(url.GetHostName(), addon))
  {
    CLog::Log(LOGERROR, "Unable to find plugin %s", url.GetHostName().c_str());
    return false;
  }

  // options
  std::string options = url.GetOptions();
  url.SetOptions(""); // do this because we can then use the url to generate the basepath
                      // which is passed to the plugin (and represents the share)

  std::string basePath(url.Get());

  // setup our parameters to send the script
  std::string strHandle = StringUtils::Format("%i", -1);
  std::vector<std::string> argv;
  argv.push_back(basePath);
  argv.push_back(strHandle);
  argv.push_back(options);

  // run the script
  CLog::Log(LOGDEBUG, "%s - calling plugin %s('%s','%s','%s')", __FUNCTION__, addon->Name().c_str(), argv[0].c_str(), argv[1].c_str(), argv[2].c_str());
  if (CScriptInvocationManager::GetInstance().ExecuteAsync(addon->LibPath(), addon, argv) >= 0)
    return true;
  else
    CLog::Log(LOGERROR, "Unable to run plugin %s", addon->Name().c_str());

  return false;
}

bool CPluginDirectory::WaitOnScriptResult(const std::string &scriptPath, int scriptId, const std::string &scriptName, bool retrievingDir)
{
  // CPluginDirectory::GetDirectory can be called from the main and other threads.
  // If called form the main thread, we need to bring up the BusyDialog in order to
  // keep the render loop alive
  if (g_application.IsCurrentThread())
  {
    if (!m_fetchComplete.WaitMSec(20))
    {
      CScriptObserver scriptObs(scriptId, m_fetchComplete);
      if (!CGUIDialogBusy::WaitOnEvent(m_fetchComplete, 200))
      {
        m_cancelled = true;
      }
      scriptObs.Abort();
    }
  }
  else
  {
    // Wait for directory fetch to complete, end, or be cancelled
    while (!m_cancelled
        && CScriptInvocationManager::GetInstance().IsRunning(scriptId)
        && !m_fetchComplete.WaitMSec(20));

    // Give the script 30 seconds to exit before we attempt to stop it
    XbmcThreads::EndTime timer(30000);
    while (!timer.IsTimePast()
          && CScriptInvocationManager::GetInstance().IsRunning(scriptId)
          && !m_fetchComplete.WaitMSec(20));
  }

  if (m_cancelled)
  { // cancel our script
    if (scriptId != -1 && CScriptInvocationManager::GetInstance().IsRunning(scriptId))
    {
      CLog::Log(LOGDEBUG, "%s- cancelling plugin %s (id=%d)", __FUNCTION__, scriptName.c_str(), scriptId);
      CScriptInvocationManager::GetInstance().Stop(scriptId);
    }
  }

  return !m_cancelled && m_success;
}

void CPluginDirectory::SetResolvedUrl(int handle, bool success, const CFileItem *resultItem)
{
  CSingleLock lock(m_handleLock);
  CPluginDirectory *dir = dirFromHandle(handle);
  if (!dir)
    return;

  dir->m_success = success;
  *dir->m_fileResult = *resultItem;

  // set the event to mark that we're done
  dir->m_fetchComplete.Set();
}

std::string CPluginDirectory::GetSetting(int handle, const std::string &strID)
{
  CSingleLock lock(m_handleLock);
  CPluginDirectory *dir = dirFromHandle(handle);
  if(dir && dir->m_addon)
    return dir->m_addon->GetSetting(strID);
  else
    return "";
}

void CPluginDirectory::SetSetting(int handle, const std::string &strID, const std::string &value)
{
  CSingleLock lock(m_handleLock);
  CPluginDirectory *dir = dirFromHandle(handle);
  if(dir && dir->m_addon)
    dir->m_addon->UpdateSetting(strID, value);
}

void CPluginDirectory::SetContent(int handle, const std::string &strContent)
{
  CSingleLock lock(m_handleLock);
  CPluginDirectory *dir = dirFromHandle(handle);
  if (dir)
    dir->m_listItems->SetContent(strContent);
}

void CPluginDirectory::SetProperty(int handle, const std::string &strProperty, const std::string &strValue)
{
  CSingleLock lock(m_handleLock);
  CPluginDirectory *dir = dirFromHandle(handle);
  if (!dir)
    return;
  if (strProperty == "fanart_image")
    dir->m_listItems->SetArt("fanart", strValue);
  else
    dir->m_listItems->SetProperty(strProperty, strValue);
}

void CPluginDirectory::CancelDirectory()
{
  m_cancelled = true;
}

float CPluginDirectory::GetProgress() const
{
  if (m_totalItems > 0)
    return (m_listItems->Size() * 100.0f) / m_totalItems;
  return 0.0f;
}
