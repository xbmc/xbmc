/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PluginDirectory.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "addons/AddonInstaller.h"
#include "addons/AddonManager.h"
#include "addons/IAddon.h"
#include "addons/PluginSource.h"
#include "addons/addoninfo/AddonType.h"
#include "interfaces/generic/RunningScriptObserver.h"
#include "messaging/ApplicationMessenger.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/URIUtils.h"
#include "utils/log.h"
#include "video/VideoInfoTag.h"

#include <mutex>

using namespace XFILE;
using namespace ADDON;
using namespace KODI::MESSAGING;

namespace
{
const unsigned int maxPluginResolutions = 5;

/*!
  \brief Get the plugin path from a CFileItem.

  \param item CFileItem where to get the path.
  \return The plugin path if found otherwise an empty string.
*/
std::string GetOriginalPluginPath(const CFileItem& item)
{
  std::string currentPath = item.GetPath();
  if (URIUtils::IsPlugin(currentPath))
    return currentPath;

  currentPath = item.GetDynPath();
  if (URIUtils::IsPlugin(currentPath))
    return currentPath;

  return std::string();
}
} // unnamed namespace

CPluginDirectory::CPluginDirectory()
  : m_listItems(new CFileItemList), m_fileResult(new CFileItem), m_cancelled(false)

{
}

CPluginDirectory::~CPluginDirectory(void)
{
}

bool CPluginDirectory::StartScript(const std::string& strPath, bool resume)
{
  CURL url(strPath);

  ADDON::AddonPtr addon;
  // try the plugin type first, and if not found, try an unknown type
  if (!CServiceBroker::GetAddonMgr().GetAddon(url.GetHostName(), addon, AddonType::PLUGIN,
                                              OnlyEnabled::CHOICE_YES) &&
      !CServiceBroker::GetAddonMgr().GetAddon(url.GetHostName(), addon, AddonType::UNKNOWN,
                                              OnlyEnabled::CHOICE_YES) &&
      !CAddonInstaller::GetInstance().InstallModal(url.GetHostName(), addon,
                                                   InstallModalPrompt::CHOICE_YES))
  {
    CLog::Log(LOGERROR, "Unable to find plugin {}", url.GetHostName());
    return false;
  }

  // clear out our status variables
  m_fileResult->Reset();
  m_listItems->Clear();
  m_listItems->SetPath(strPath);
  m_listItems->SetLabel(addon->Name());
  m_cancelled = false;
  m_success = false;
  m_totalItems = 0;

  // run the script
  return RunScript(this, addon, strPath, resume);
}

bool CPluginDirectory::GetResolvedPluginResult(CFileItem& resultItem)
{
  std::string lastResolvedPath;
  if (resultItem.HasProperty("ForceResolvePlugin") &&
      resultItem.GetProperty("ForceResolvePlugin").asBoolean())
  {
    // ensures that a plugin have the callback to resolve the paths in any case
    // also when the same items in the playlist are played more times
    lastResolvedPath = GetOriginalPluginPath(resultItem);
  }
  else
  {
    lastResolvedPath = resultItem.GetDynPath();
  }

  if (!lastResolvedPath.empty())
  {
    // we try to resolve recursively up to n. (maxPluginResolutions) nested plugin paths
    // to avoid deadlocks (plugin:// paths can resolve to plugin:// paths)
    for (unsigned int i = 0; URIUtils::IsPlugin(lastResolvedPath) && i < maxPluginResolutions; ++i)
    {
      bool resume = resultItem.GetStartOffset() == STARTOFFSET_RESUME;

      // we modify the item so that it becomes a real URL
      if (!XFILE::CPluginDirectory::GetPluginResult(lastResolvedPath, resultItem, resume) ||
          resultItem.GetDynPath() ==
              resultItem.GetPath()) // GetPluginResult resolved to an empty path
      {
        return false;
      }
      lastResolvedPath = resultItem.GetDynPath();
    }
    // if after the maximum allowed resolution attempts the item is still a plugin just return, it isn't playable
    if (URIUtils::IsPlugin(resultItem.GetDynPath()))
      return false;
  }

  return true;
}

bool CPluginDirectory::GetPluginResult(const std::string& strPath, CFileItem &resultItem, bool resume)
{
  CURL url(strPath);
  CPluginDirectory newDir;

  bool success = newDir.StartScript(strPath, resume);

  if (success)
  { // update the play path and metadata, saving the old one as needed
    if (!resultItem.HasProperty("original_listitem_url"))
      resultItem.SetProperty("original_listitem_url", resultItem.GetPath());
    resultItem.SetDynPath(newDir.m_fileResult->GetPath());
    resultItem.SetMimeType(newDir.m_fileResult->GetMimeType());
    resultItem.SetContentLookup(newDir.m_fileResult->ContentLookup());

    if (resultItem.HasProperty("OverrideInfotag") &&
        resultItem.GetProperty("OverrideInfotag").asBoolean())
      resultItem.UpdateInfo(*newDir.m_fileResult);
    else
      resultItem.MergeInfo(*newDir.m_fileResult);

    if (newDir.m_fileResult->HasVideoInfoTag() && newDir.m_fileResult->GetVideoInfoTag()->GetResumePoint().IsSet())
      resultItem.SetStartOffset(
          STARTOFFSET_RESUME); // resume point set in the resume item, so force resume
  }

  return success;
}

bool CPluginDirectory::AddItem(int handle, const CFileItem *item, int totalItems)
{
  std::unique_lock<CCriticalSection> lock(GetScriptsLock());
  CPluginDirectory* dir = GetScriptFromHandle(handle);
  if (!dir)
    return false;

  CFileItemPtr pItem(new CFileItem(*item));
  dir->m_listItems->Add(pItem);
  dir->m_totalItems = totalItems;

  return !dir->m_cancelled;
}

bool CPluginDirectory::AddItems(int handle, const CFileItemList *items, int totalItems)
{
  std::unique_lock<CCriticalSection> lock(GetScriptsLock());
  CPluginDirectory* dir = GetScriptFromHandle(handle);
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
  std::unique_lock<CCriticalSection> lock(GetScriptsLock());
  CPluginDirectory* dir = GetScriptFromHandle(handle);
  if (!dir)
    return;

  // set cache to disc
  dir->m_listItems->SetCacheToDisc(cacheToDisc ? CFileItemList::CACHE_IF_SLOW : CFileItemList::CACHE_NEVER);

  dir->m_success = success;
  dir->m_listItems->SetReplaceListing(replaceListing);

  if (!dir->m_listItems->HasSortDetails())
    dir->m_listItems->AddSortMethod(SortByNone, 552, LABEL_MASKS("%L", "%D"));

  // set the event to mark that we're done
  dir->SetDone();
}

void CPluginDirectory::AddSortMethod(int handle, SORT_METHOD sortMethod, const std::string &labelMask, const std::string &label2Mask)
{
  std::unique_lock<CCriticalSection> lock(GetScriptsLock());
  CPluginDirectory* dir = GetScriptFromHandle(handle);
  if (!dir)
    return;

  //! @todo Add all sort methods and fix which labels go on the right or left
  switch(sortMethod)
  {
    case SORT_METHOD_LABEL:
    case SORT_METHOD_LABEL_IGNORE_THE:
      {
        dir->m_listItems->AddSortMethod(SortByLabel, 551, LABEL_MASKS(labelMask, label2Mask, labelMask, label2Mask), CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_FILELISTS_IGNORETHEWHENSORTING) ? SortAttributeIgnoreArticle : SortAttributeNone);
        break;
      }
    case SORT_METHOD_TITLE:
    case SORT_METHOD_TITLE_IGNORE_THE:
      {
        dir->m_listItems->AddSortMethod(SortByTitle, 556, LABEL_MASKS(labelMask, label2Mask, labelMask, label2Mask), CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_FILELISTS_IGNORETHEWHENSORTING) ? SortAttributeIgnoreArticle : SortAttributeNone);
        break;
      }
    case SORT_METHOD_ARTIST:
    case SORT_METHOD_ARTIST_IGNORE_THE:
      {
        dir->m_listItems->AddSortMethod(SortByArtist, 557, LABEL_MASKS(labelMask, "%A", labelMask, "%A"), CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_FILELISTS_IGNORETHEWHENSORTING) ? SortAttributeIgnoreArticle : SortAttributeNone);
        break;
      }
    case SORT_METHOD_ALBUM:
    case SORT_METHOD_ALBUM_IGNORE_THE:
      {
        dir->m_listItems->AddSortMethod(SortByAlbum, 558, LABEL_MASKS(labelMask, "%B", labelMask, "%B"), CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_FILELISTS_IGNORETHEWHENSORTING) ? SortAttributeIgnoreArticle : SortAttributeNone);
        break;
      }
    case SORT_METHOD_DATE:
      {
        dir->m_listItems->AddSortMethod(SortByDate, 552, LABEL_MASKS(labelMask, "%J", labelMask, "%J"));
        break;
      }
    case SORT_METHOD_BITRATE:
      {
        dir->m_listItems->AddSortMethod(SortByBitrate, 623, LABEL_MASKS(labelMask, "%X", labelMask, "%X"));
        break;
      }
    case SORT_METHOD_SIZE:
      {
        dir->m_listItems->AddSortMethod(SortBySize, 553, LABEL_MASKS(labelMask, "%I", labelMask, "%I"));
        break;
      }
    case SORT_METHOD_FILE:
      {
        dir->m_listItems->AddSortMethod(SortByFile, 561, LABEL_MASKS(labelMask, label2Mask, labelMask, label2Mask));
        break;
      }
    case SORT_METHOD_TRACKNUM:
      {
        dir->m_listItems->AddSortMethod(SortByTrackNumber, 554, LABEL_MASKS(labelMask, label2Mask, labelMask, label2Mask));
        break;
      }
    case SORT_METHOD_DURATION:
    case SORT_METHOD_VIDEO_RUNTIME:
      {
        dir->m_listItems->AddSortMethod(SortByTime, 180, LABEL_MASKS(labelMask, "%D", labelMask, "%D"));
        break;
      }
    case SORT_METHOD_VIDEO_RATING:
    case SORT_METHOD_SONG_RATING:
      {
        dir->m_listItems->AddSortMethod(SortByRating, 563, LABEL_MASKS(labelMask, "%R", labelMask, "%R"));
        break;
      }
    case SORT_METHOD_YEAR:
      {
        dir->m_listItems->AddSortMethod(SortByYear, 562, LABEL_MASKS(labelMask, "%Y", labelMask, "%Y"));
        break;
      }
    case SORT_METHOD_GENRE:
      {
        dir->m_listItems->AddSortMethod(SortByGenre, 515, LABEL_MASKS(labelMask, "%G", labelMask, "%G"));
        break;
      }
    case SORT_METHOD_COUNTRY:
      {
        dir->m_listItems->AddSortMethod(SortByCountry, 574, LABEL_MASKS(labelMask, "%G", labelMask, "%G"));
        break;
      }
    case SORT_METHOD_VIDEO_TITLE:
      {
        dir->m_listItems->AddSortMethod(SortByTitle, 369, LABEL_MASKS(labelMask, "%M", labelMask, "%M"));
        break;
      }
    case SORT_METHOD_VIDEO_SORT_TITLE:
    case SORT_METHOD_VIDEO_SORT_TITLE_IGNORE_THE:
      {
        dir->m_listItems->AddSortMethod(SortBySortTitle, 556, LABEL_MASKS(labelMask, "%M", labelMask, "%M"), CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_FILELISTS_IGNORETHEWHENSORTING) ? SortAttributeIgnoreArticle : SortAttributeNone);
        break;
      }
      case SORT_METHOD_VIDEO_ORIGINAL_TITLE:
      case SORT_METHOD_VIDEO_ORIGINAL_TITLE_IGNORE_THE:
      {
        dir->m_listItems->AddSortMethod(
            SortByOriginalTitle, 20376, LABEL_MASKS(labelMask, "%M", labelMask, "%M"),
            CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
                CSettings::SETTING_FILELISTS_IGNORETHEWHENSORTING)
                ? SortAttributeIgnoreArticle
                : SortAttributeNone);
        break;
      }
      case SORT_METHOD_MPAA_RATING:
      {
        dir->m_listItems->AddSortMethod(SortByMPAA, 20074, LABEL_MASKS(labelMask, "%O", labelMask, "%O"));
        break;
      }
    case SORT_METHOD_STUDIO:
    case SORT_METHOD_STUDIO_IGNORE_THE:
      {
        dir->m_listItems->AddSortMethod(SortByStudio, 572, LABEL_MASKS(labelMask, "%U", labelMask, "%U"), CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_FILELISTS_IGNORETHEWHENSORTING) ? SortAttributeIgnoreArticle : SortAttributeNone);
        break;
      }
    case SORT_METHOD_PROGRAM_COUNT:
      {
        dir->m_listItems->AddSortMethod(SortByProgramCount, 567, LABEL_MASKS(labelMask, "%C", labelMask, "%C"));
        break;
      }
    case SORT_METHOD_UNSORTED:
      {
        dir->m_listItems->AddSortMethod(SortByNone, 571, LABEL_MASKS(labelMask, label2Mask, labelMask, label2Mask));
        break;
      }
    case SORT_METHOD_NONE:
      {
        dir->m_listItems->AddSortMethod(SortByNone, 552, LABEL_MASKS(labelMask, label2Mask, labelMask, label2Mask));
        break;
      }
    case SORT_METHOD_DRIVE_TYPE:
      {
        dir->m_listItems->AddSortMethod(SortByDriveType, 564, LABEL_MASKS()); // Preformatted
        break;
      }
    case SORT_METHOD_PLAYLIST_ORDER:
      {
        std::string strTrack=CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(CSettings::SETTING_MUSICFILES_TRACKFORMAT);
        dir->m_listItems->AddSortMethod(SortByPlaylistOrder, 559, LABEL_MASKS(strTrack, "%D"));
        break;
      }
    case SORT_METHOD_EPISODE:
      {
        dir->m_listItems->AddSortMethod(SortByEpisodeNumber, 20359, LABEL_MASKS(labelMask, "%R", labelMask, "%R"));
        break;
      }
    case SORT_METHOD_PRODUCTIONCODE:
      {
        //dir->m_listItems.AddSortMethod(SORT_METHOD_PRODUCTIONCODE,20368,LABEL_MASKS("%E. %T","%P", "%E. %T","%P"));
        dir->m_listItems->AddSortMethod(SortByProductionCode, 20368, LABEL_MASKS(labelMask, "%P", labelMask, "%P"));
        break;
      }
    case SORT_METHOD_LISTENERS:
      {
       dir->m_listItems->AddSortMethod(SortByListeners, 20455, LABEL_MASKS(labelMask, "%W"));
       break;
      }
    case SORT_METHOD_DATEADDED:
      {
        dir->m_listItems->AddSortMethod(SortByDateAdded, 570, LABEL_MASKS(labelMask, "%a"));
        break;
      }
    case SORT_METHOD_FULLPATH:
      {
        dir->m_listItems->AddSortMethod(SortByPath, 573, LABEL_MASKS(labelMask, label2Mask, labelMask, label2Mask));
        break;
      }
    case SORT_METHOD_LABEL_IGNORE_FOLDERS:
      {
        dir->m_listItems->AddSortMethod(SortByLabel, SortAttributeIgnoreFolders, 551, LABEL_MASKS(labelMask, label2Mask, labelMask, label2Mask));
        break;
      }
    case SORT_METHOD_LASTPLAYED:
      {
        dir->m_listItems->AddSortMethod(SortByLastPlayed, 568, LABEL_MASKS(labelMask, "%G"));
        break;
      }
    case SORT_METHOD_PLAYCOUNT:
      {
        dir->m_listItems->AddSortMethod(SortByPlaycount, 567, LABEL_MASKS(labelMask, "%V", labelMask, "%V"));
        break;
      }
    case SORT_METHOD_CHANNEL:
      {
        dir->m_listItems->AddSortMethod(SortByChannel, 19029, LABEL_MASKS(labelMask, label2Mask, labelMask, label2Mask));
        break;
      }

    default:
      break;
  }
}

bool CPluginDirectory::GetDirectory(const CURL& url, CFileItemList& items)
{
  const std::string pathToUrl(url.Get());
  bool success = StartScript(pathToUrl, false);

  // append the items to the list
  items.Assign(*m_listItems, true); // true to keep the current items
  m_listItems->Clear();
  return success;
}

bool CPluginDirectory::RunScriptWithParams(const std::string& strPath, bool resume)
{
  CURL url(strPath);
  if (url.GetHostName().empty()) // called with no script - should never happen
    return false;

  AddonPtr addon;
  if (!CServiceBroker::GetAddonMgr().GetAddon(url.GetHostName(), addon, AddonType::PLUGIN,
                                              OnlyEnabled::CHOICE_YES) &&
      !CAddonInstaller::GetInstance().InstallModal(url.GetHostName(), addon,
                                                   InstallModalPrompt::CHOICE_YES))
  {
    CLog::Log(LOGERROR, "Unable to find plugin {}", url.GetHostName());
    return false;
  }

  return ExecuteScript(addon, strPath, resume) >= 0;
}

void CPluginDirectory::SetResolvedUrl(int handle, bool success, const CFileItem *resultItem)
{
  std::unique_lock<CCriticalSection> lock(GetScriptsLock());
  CPluginDirectory* dir = GetScriptFromHandle(handle);
  if (!dir)
    return;

  dir->m_success = success;
  *dir->m_fileResult = *resultItem;

  // set the event to mark that we're done
  dir->SetDone();
}

std::string CPluginDirectory::GetSetting(int handle, const std::string &strID)
{
  std::unique_lock<CCriticalSection> lock(GetScriptsLock());
  CPluginDirectory* dir = GetScriptFromHandle(handle);
  if (dir && dir->GetAddon())
    return dir->GetAddon()->GetSetting(strID);
  else
    return "";
}

void CPluginDirectory::SetSetting(int handle, const std::string &strID, const std::string &value)
{
  std::unique_lock<CCriticalSection> lock(GetScriptsLock());
  CPluginDirectory* dir = GetScriptFromHandle(handle);
  if (dir && dir->GetAddon())
    dir->GetAddon()->UpdateSetting(strID, value);
}

void CPluginDirectory::SetContent(int handle, const std::string &strContent)
{
  std::unique_lock<CCriticalSection> lock(GetScriptsLock());
  CPluginDirectory* dir = GetScriptFromHandle(handle);
  if (dir)
    dir->m_listItems->SetContent(strContent);
}

void CPluginDirectory::SetProperty(int handle, const std::string &strProperty, const std::string &strValue)
{
  std::unique_lock<CCriticalSection> lock(GetScriptsLock());
  CPluginDirectory* dir = GetScriptFromHandle(handle);
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

bool CPluginDirectory::IsMediaLibraryScanningAllowed(const std::string& content, const std::string& strPath)
{
  if (content.empty())
    return false;

  CURL url(strPath);
  if (url.GetHostName().empty())
    return false;
  AddonPtr addon;
  if (!CServiceBroker::GetAddonMgr().GetAddon(url.GetHostName(), addon, AddonType::PLUGIN,
                                              OnlyEnabled::CHOICE_YES))
  {
    CLog::Log(LOGERROR, "Unable to find plugin {}", url.GetHostName());
    return false;
  }
  CPluginSource* plugin = dynamic_cast<CPluginSource*>(addon.get());
  if (!plugin)
    return false;

  auto& paths = plugin->MediaLibraryScanPaths();
  if (paths.empty())
    return false;
  auto it = paths.find(content);
  if (it == paths.end())
    return false;
  const std::string& path = url.GetFileName();
  for (const auto& p : it->second)
    if (p.empty() || p == "/" || URIUtils::PathHasParent(path, p))
      return true;
  return false;
}

bool CPluginDirectory::CheckExists(const std::string& content, const std::string& strPath)
{
  if (!IsMediaLibraryScanningAllowed(content, strPath))
    return false;
  // call the plugin at specified path with option "kodi_action=check_exists"
  // url exists if the plugin returns any fileitem with setResolvedUrl
  CURL url(strPath);
  url.SetOption("kodi_action", "check_exists");
  CFileItem item;
  return CPluginDirectory::GetPluginResult(url.Get(), item, false);
}

bool CPluginDirectory::Resolve(CFileItem& item) const
{
  return GetResolvedPluginResult(item);
}
