/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FileItemList.h"

#include "CueDocument.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "filesystem/MusicDatabaseDirectory.h"
#include "filesystem/StackDirectory.h"
#include "filesystem/VideoDatabaseDirectory.h"
#include "music/MusicFileItemClassify.h"
#include "network/NetworkFileItemClassify.h"
#include "playlists/PlayListFileItemClassify.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/Archive.h"
#include "utils/ArtUtils.h"
#include "utils/Crc32.h"
#include "utils/FileExtensionProvider.h"
#include "utils/Random.h"
#include "utils/RegExp.h"
#include "utils/URIUtils.h"
#include "utils/log.h"
#include "video/VideoFileItemClassify.h"
#include "video/VideoUtils.h"

#include <algorithm>

using namespace KODI;
using namespace XFILE;

CFileItemList::CFileItemList() : CFileItem("", true)
{
}

CFileItemList::CFileItemList(const std::string& strPath) : CFileItem(strPath, true)
{
}

CFileItemList::~CFileItemList()
{
  Clear();
}

CFileItemPtr CFileItemList::operator[](int iItem) const
{
  return Get(iItem);
}

CFileItemPtr CFileItemList::operator[](const std::string& strPath) const
{
  return Get(strPath);
}

void CFileItemList::SetIgnoreURLOptions(bool ignoreURLOptions)
{
  m_ignoreURLOptions = ignoreURLOptions;

  if (m_fastLookup)
  {
    m_fastLookup = false; // Force SetFastlookup to clear map
    SetFastLookup(true); // and regenerate map
  }
}

void CFileItemList::SetFastLookup(bool fastLookup)
{
  std::unique_lock lock(m_lock);

  if (fastLookup && !m_fastLookup)
  { // generate the map
    m_map.clear();
    AddFastLookupItems(m_items);
  }
  if (!fastLookup && m_fastLookup)
    m_map.clear();
  m_fastLookup = fastLookup;
}

bool CFileItemList::Contains(const std::string& fileName) const
{
  std::unique_lock lock(m_lock);

  const std::string fname = m_ignoreURLOptions ? CURL(fileName).GetWithoutOptions() : fileName;
  if (m_fastLookup)
    return m_map.contains(fname);

  // slow method...
  return std::ranges::any_of(m_items, [&fname](const auto& pItem) { return pItem->IsPath(fname); });
}

void CFileItemList::Clear()
{
  std::unique_lock lock(m_lock);

  ClearItems();
  m_sortDescription.sortBy = SortByNone;
  m_sortDescription.sortOrder = SortOrderNone;
  m_sortDescription.sortAttributes = SortAttributeNone;
  m_sortIgnoreFolders = false;
  m_cacheToDisc = CacheType::IF_SLOW;
  m_sortDetails.clear();
  m_replaceListing = false;
  m_content.clear();
}

void CFileItemList::ClearItems()
{
  std::unique_lock lock(m_lock);
  // make sure we free the memory of the items (these are GUIControls which may have allocated resources)
  FreeMemory();
  std::ranges::for_each(m_items, [](const auto& item) { item->FreeMemory(); });
  m_items.clear();
  m_map.clear();
}

void CFileItemList::AddFastLookupItem(const CFileItemPtr& item)
{
  m_map.try_emplace(m_ignoreURLOptions ? item->GetURL().GetWithoutOptions() : item->GetPath(),
                    item);
}

void CFileItemList::AddFastLookupItems(const std::vector<CFileItemPtr>& items)
{
  for (const auto& item : items)
    AddFastLookupItem(item);
}

void CFileItemList::Add(CFileItemPtr pItem)
{
  std::unique_lock lock(m_lock);
  if (m_fastLookup)
    AddFastLookupItem(pItem);
  m_items.emplace_back(std::move(pItem));
}

void CFileItemList::Add(CFileItem&& item)
{
  std::unique_lock lock(m_lock);
  auto ptr = std::make_shared<CFileItem>(std::move(item));
  if (m_fastLookup)
    AddFastLookupItem(ptr);
  m_items.emplace_back(std::move(ptr));
}

void CFileItemList::AddItems(const std::vector<CFileItemPtr>& items)
{
  std::unique_lock lock(m_lock);

  if (m_fastLookup)
    AddFastLookupItems(items);

  m_items.reserve(m_items.size() + items.size());
  std::ranges::copy(items, std::back_inserter(m_items));
}

void CFileItemList::AddItems(std::vector<CFileItemPtr>&& items)
{
  std::unique_lock lock(m_lock);

  if (m_fastLookup)
    AddFastLookupItems(items);

  m_items.reserve(m_items.size() + items.size());
  std::ranges::move(items, std::back_inserter(m_items));
}

void CFileItemList::AddFront(const CFileItemPtr& pItem, int itemPosition)
{
  std::unique_lock lock(m_lock);
  if (itemPosition >= 0)
  {
    m_items.insert(m_items.begin() + itemPosition, pItem);
  }
  else
  {
    m_items.insert(m_items.begin() + (m_items.size() + itemPosition), pItem);
  }

  if (m_fastLookup)
    AddFastLookupItem(pItem);
}

void CFileItemList::Remove(const CFileItem* pItem)
{
  std::unique_lock lock(m_lock);
  const auto it =
      std::ranges::find_if(m_items, [pItem](const auto& item) { return item.get() == pItem; });
  if (it != m_items.end())
  {
    m_items.erase(it);
    if (m_fastLookup)
    {
      m_map.erase(m_ignoreURLOptions ? pItem->GetURL().GetWithoutOptions() : pItem->GetPath());
    }
  }
}

CFileItemList::Iterator CFileItemList::erase(Iterator first, Iterator last)
{
  std::unique_lock lock(m_lock);
  return m_items.erase(first, last);
}

void CFileItemList::Remove(int iItem)
{
  std::unique_lock lock(m_lock);

  if (iItem >= 0 && iItem < Size())
  {
    CFileItemPtr pItem = *(m_items.begin() + iItem);
    if (m_fastLookup)
    {
      m_map.erase(m_ignoreURLOptions ? pItem->GetURL().GetWithoutOptions() : pItem->GetPath());
    }
    m_items.erase(m_items.begin() + iItem);
  }
}

void CFileItemList::Append(const CFileItemList& itemlist)
{
  std::unique_lock lock(m_lock);

  std::ranges::for_each(itemlist, [this](const auto& item) { Add(item); });
}

void CFileItemList::Assign(const CFileItemList& itemlist, bool append)
{
  std::unique_lock lock(m_lock);
  if (!append)
    Clear();

  Append(itemlist);

  //! @todo Is it intentional not to copy CFileItem properties, except path, label and property map?
  //! This is different from CFileItemList::Copy. Why?
  SetPath(itemlist.GetPath());
  SetLabel(itemlist.GetLabel());
  SetProperties(itemlist.GetProperties());

  //! @todo Is it intentional not to copy m_ignoreURLOptions, m_fastLookup, m_sortIgnoreFolders, m_content?
  //! This is (partly) different from CFileItemList::Copy. Why?
  m_sortDetails = itemlist.m_sortDetails;
  m_sortDescription = itemlist.m_sortDescription;
  m_replaceListing = itemlist.m_replaceListing;
  m_content = itemlist.m_content;
  m_cacheToDisc = itemlist.m_cacheToDisc;
}

bool CFileItemList::Copy(const CFileItemList& items, bool copyItems /* = true */)
{
  // assign all CFileItem parts
  *static_cast<CFileItem*>(this) = static_cast<const CFileItem&>(items);

  //! @todo Is it intentional not to copy m_ignoreURLOptions, m_fastLookup ?
  // assign the rest of the CFileItemList properties
  m_replaceListing = items.m_replaceListing;
  m_content = items.m_content;
  m_cacheToDisc = items.m_cacheToDisc;
  m_sortDetails = items.m_sortDetails;
  m_sortDescription = items.m_sortDescription;
  m_sortIgnoreFolders = items.m_sortIgnoreFolders;

  if (copyItems)
  {
    // make a copy of each item
    std::ranges::for_each(items,
                          [this](const auto& item) { Add(std::make_shared<CFileItem>(*item)); });
  }

  return true;
}

CFileItemPtr CFileItemList::Get(int iItem) const
{
  std::unique_lock lock(m_lock);

  if (iItem > -1 && iItem < static_cast<int>(m_items.size()))
    return m_items[iItem];

  return CFileItemPtr();
}

CFileItemPtr CFileItemList::Get(const std::string& strPath) const
{
  std::unique_lock lock(m_lock);

  if (m_fastLookup)
  {
    const auto it = m_map.find(m_ignoreURLOptions ? CURL(strPath).GetWithoutOptions() : strPath);
    if (it != m_map.end())
      return it->second;

    return {};
  }
  // slow method...
  const std::string fname = m_ignoreURLOptions ? CURL(strPath).GetWithoutOptions() : strPath;
  const auto it =
      std::ranges::find_if(m_items, [&fname](const auto& pItem) { return pItem->IsPath(fname); });
  return it != m_items.end() ? *it : CFileItemPtr();
}

int CFileItemList::Size() const
{
  std::unique_lock lock(m_lock);
  return static_cast<int>(m_items.size());
}

bool CFileItemList::IsEmpty() const
{
  std::unique_lock lock(m_lock);
  return m_items.empty();
}

void CFileItemList::Reserve(size_t iCount)
{
  std::unique_lock lock(m_lock);
  m_items.reserve(iCount);
}

void CFileItemList::Sort(SortBy sortBy,
                         SortOrder sortOrder,
                         SortAttribute sortAttributes /* = SortAttributeNone */)
{
  if (sortBy == SortByNone ||
      (m_sortDescription.sortBy == sortBy && m_sortDescription.sortOrder == sortOrder &&
       m_sortDescription.sortAttributes == sortAttributes))
    return;

  SortDescription sorting;
  sorting.sortBy = sortBy;
  sorting.sortOrder = sortOrder;
  sorting.sortAttributes = sortAttributes;

  Sort(sorting);
  m_sortDescription = sorting;
}

void CFileItemList::Sort(SortDescription sortDescription)
{
  if (sortDescription.sortBy == SortByNone ||
      (m_sortDescription.sortBy == sortDescription.sortBy &&
       m_sortDescription.sortOrder == sortDescription.sortOrder &&
       m_sortDescription.sortAttributes == sortDescription.sortAttributes))
    return;

  if (sortDescription.sortAttributes & SortAttributeForceConsiderFolders)
  {
    sortDescription.sortAttributes =
        static_cast<SortAttribute>(sortDescription.sortAttributes & ~SortAttributeIgnoreFolders);
  }
  else if ((sortDescription.sortBy == SortByFile || sortDescription.sortBy == SortBySortTitle ||
            sortDescription.sortBy == SortByOriginalTitle ||
            sortDescription.sortBy == SortByDateAdded || sortDescription.sortBy == SortByRating ||
            sortDescription.sortBy == SortByYear || sortDescription.sortBy == SortByPlaylistOrder ||
            sortDescription.sortBy == SortByLastPlayed ||
            sortDescription.sortBy == SortByPlaycount) ||
           m_sortIgnoreFolders)
  {
    sortDescription.sortAttributes =
        static_cast<SortAttribute>(sortDescription.sortAttributes | SortAttributeIgnoreFolders);
  }

  const Fields fields = SortUtils::GetFieldsForSorting(sortDescription.sortBy);
  SortItems sortItems(static_cast<size_t>(Size()));
  for (int index = 0; index < Size(); index++)
  {
    sortItems[index] = std::make_shared<SortItem>();
    m_items[index]->ToSortable(*sortItems[index], fields);
    (*sortItems[index])[FieldId] = index;
  }

  // do the sorting
  SortUtils::Sort(sortDescription, sortItems);

  // apply the new order to the existing CFileItems
  std::vector<std::shared_ptr<CFileItem>> sortedFileItems;
  sortedFileItems.reserve(Size());
  for (const auto& sortItem : sortItems)
  {
    std::shared_ptr<CFileItem> item = m_items[static_cast<int>(sortItem->at(FieldId).asInteger())];
    // Set the sort label in the CFileItem
    item->SetSortLabel(sortItem->at(FieldSort).asWideString());

    sortedFileItems.emplace_back(std::move(item));
  }

  // replace the current list with the re-ordered one
  m_items = std::move(sortedFileItems);
}

void CFileItemList::Randomize()
{
  std::unique_lock lock(m_lock);
  KODI::UTILS::RandomShuffle(m_items.begin(), m_items.end());
}

void CFileItemList::Archive(CArchive& ar)
{
  std::unique_lock lock(m_lock);
  if (ar.IsStoring())
  {
    CFileItem::Archive(ar);

    int i = 0;
    if (!m_items.empty() && m_items[0]->IsParentFolder())
      i = 1;

    ar << static_cast<int>(m_items.size() - i);

    ar << m_ignoreURLOptions;

    ar << m_fastLookup;

    ar << static_cast<int>(m_sortDescription.sortBy);
    ar << static_cast<int>(m_sortDescription.sortOrder);
    ar << static_cast<int>(m_sortDescription.sortAttributes);
    ar << m_sortIgnoreFolders;
    ar << static_cast<int>(m_cacheToDisc);

    ar << static_cast<int>(m_sortDetails.size());
    for (const auto& details : m_sortDetails)
    {
      ar << static_cast<int>(details.m_sortDescription.sortBy);
      ar << static_cast<int>(details.m_sortDescription.sortOrder);
      ar << static_cast<int>(details.m_sortDescription.sortAttributes);
      ar << details.m_buttonLabel;
      ar << details.m_labelMasks.m_strLabelFile;
      ar << details.m_labelMasks.m_strLabelFolder;
      ar << details.m_labelMasks.m_strLabel2File;
      ar << details.m_labelMasks.m_strLabel2Folder;
    }

    ar << m_content;

    for (; i < static_cast<int>(m_items.size()); ++i)
    {
      const CFileItemPtr pItem = m_items[i];
      ar << *pItem;
    }
  }
  else
  {
    CFileItemPtr pParent;
    if (!IsEmpty())
    {
      const CFileItemPtr pItem = m_items[0];
      if (pItem->IsParentFolder())
        pParent = std::make_shared<CFileItem>(*pItem);
    }

    SetIgnoreURLOptions(false);
    SetFastLookup(false);
    Clear();

    CFileItem::Archive(ar);

    int iSize = 0;
    ar >> iSize;
    if (iSize <= 0)
      return;

    if (pParent)
    {
      m_items.reserve(iSize + 1);
      m_items.emplace_back(std::move(pParent));
    }
    else
      m_items.reserve(iSize);

    bool ignoreURLOptions = false;
    ar >> ignoreURLOptions;

    bool fastLookup = false;
    ar >> fastLookup;

    int tempint;
    ar >> tempint;
    m_sortDescription.sortBy = static_cast<SortBy>(tempint);
    ar >> tempint;
    m_sortDescription.sortOrder = static_cast<SortOrder>(tempint);
    ar >> tempint;
    m_sortDescription.sortAttributes = static_cast<SortAttribute>(tempint);
    ar >> m_sortIgnoreFolders;
    ar >> tempint;
    m_cacheToDisc = static_cast<CacheType>(tempint);

    unsigned int detailSize = 0;
    ar >> detailSize;
    for (unsigned int j = 0; j < detailSize; ++j)
    {
      GUIViewSortDetails details;
      ar >> tempint;
      details.m_sortDescription.sortBy = static_cast<SortBy>(tempint);
      ar >> tempint;
      details.m_sortDescription.sortOrder = static_cast<SortOrder>(tempint);
      ar >> tempint;
      details.m_sortDescription.sortAttributes = static_cast<SortAttribute>(tempint);
      ar >> details.m_buttonLabel;
      ar >> details.m_labelMasks.m_strLabelFile;
      ar >> details.m_labelMasks.m_strLabelFolder;
      ar >> details.m_labelMasks.m_strLabel2File;
      ar >> details.m_labelMasks.m_strLabel2Folder;
      m_sortDetails.emplace_back(std::move(details));
    }

    ar >> m_content;

    for (int i = 0; i < iSize; ++i)
    {
      const auto pItem{std::make_shared<CFileItem>()};
      ar >> *pItem;
      Add(pItem);
    }

    SetIgnoreURLOptions(ignoreURLOptions);
    SetFastLookup(fastLookup);
  }
}

void CFileItemList::FillInDefaultIcons()
{
  std::unique_lock lock(m_lock);
  std::ranges::for_each(m_items, [](const auto& pItem) { ART::FillInDefaultIcon(*pItem); });
}

int CFileItemList::GetFolderCount() const
{
  std::unique_lock lock(m_lock);
  return static_cast<int>(
      std::ranges::count_if(m_items, [](const auto& pItem) { return pItem->IsFolder(); }));
}

int CFileItemList::GetObjectCount() const
{
  std::unique_lock lock(m_lock);

  auto numObjects = static_cast<int>(m_items.size());
  if (numObjects && m_items[0]->IsParentFolder())
    numObjects--;

  return numObjects;
}

int CFileItemList::GetFileCount() const
{
  std::unique_lock lock(m_lock);
  return static_cast<int>(
      std::ranges::count_if(m_items, [](const auto& pItem) { return !pItem->IsFolder(); }));
}

int CFileItemList::GetSelectedCount() const
{
  std::unique_lock lock(m_lock);
  return static_cast<int>(
      std::ranges::count_if(m_items, [](const auto& pItem) { return pItem->IsSelected(); }));
}

void CFileItemList::FilterCueItems()
{
  std::unique_lock lock(m_lock);
  // Handle .CUE sheet files...
  std::vector<std::string> itemstodelete;
  for (const auto& pItem : m_items)
  {
    if (!pItem->IsFolder())
    { // see if it's a .CUE sheet
      if (MUSIC::IsCUESheet(*pItem))
      {
        const auto cuesheet(std::make_shared<CCueDocument>());
        if (cuesheet->ParseFile(pItem->GetPath()))
        {
          std::vector<std::string> MediaFileVec;
          cuesheet->GetMediaFiles(MediaFileVec);

          // queue the cue sheet and the underlying media file for deletion
          for (const auto& itMedia : MediaFileVec)
          {
            std::string strMediaFile = itMedia;
            std::string fileFromCue =
                strMediaFile; // save the file from the cue we're matching against,
            // as we're going to search for others here...
            bool bFoundMediaFile = CFile::Exists(strMediaFile);
            if (!bFoundMediaFile)
            {
              // try file in same dir, not matching case...
              if (Contains(strMediaFile))
              {
                bFoundMediaFile = true;
              }
              else
              {
                // try removing the .cue extension...
                strMediaFile = pItem->GetPath();
                URIUtils::RemoveExtension(strMediaFile);
                CFileItem item(strMediaFile, false);
                if (MUSIC::IsAudio(item) && Contains(strMediaFile))
                {
                  bFoundMediaFile = true;
                }
                else
                { // try replacing the extension with one of our allowed ones.
                  std::vector<std::string> extensions = StringUtils::Split(
                      CServiceBroker::GetFileExtensionProvider().GetMusicExtensions(), "|");
                  for (const auto& ext : extensions)
                  {
                    strMediaFile = URIUtils::ReplaceExtension(pItem->GetPath(), ext);
                    CFileItem media_item(strMediaFile, false);
                    if (!MUSIC::IsCUESheet(media_item) && !PLAYLIST::IsPlayList(media_item) &&
                        Contains(strMediaFile))
                    {
                      bFoundMediaFile = true;
                      break;
                    }
                  }
                }
              }
            }
            if (bFoundMediaFile)
            {
              cuesheet->UpdateMediaFile(fileFromCue, strMediaFile);
              // apply CUE for later processing
              for (const auto& inner_item : m_items)
              {
                if (StringUtils::CompareNoCase(inner_item->GetPath(), strMediaFile) == 0)
                  inner_item->SetCueDocument(cuesheet);
              }
            }
          }
        }
        itemstodelete.emplace_back(pItem->GetPath());
      }
    }
  }
  // now delete the .CUE files.
  for (const auto& delItem : itemstodelete)
  {
    const auto it = std::ranges::find_if(
        m_items, [&delItem](const auto& pItem)
        { return StringUtils::CompareNoCase(pItem->GetPath(), delItem) == 0; });
    if (it != m_items.end())
      m_items.erase(it);
  }
}

// Remove the extensions from the filenames
void CFileItemList::RemoveExtensions()
{
  std::unique_lock lock(m_lock);
  std::ranges::for_each(m_items, [](auto& item) { item->RemoveExtension(); });
}

void CFileItemList::Stack(bool stackFiles /* = true */)
{
  std::unique_lock lock(m_lock);

  // not allowed here
  if (IsVirtualDirectoryRoot() || IsLiveTV() || IsSourcesPath() || IsLibraryFolder())
    return;

  SetProperty("isstacked", true);

  // items needs to be sorted for stuff below to work properly
  Sort(SortByLabel, SortOrderAscending);

  StackFolders();

  if (stackFiles)
    StackFiles();
}

void CFileItemList::StackFolders()
{
  // Precompile our REs
  std::vector<CRegExp> folderRegExps =
      CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_folderStackRegExps;

  if (folderRegExps.empty())
  {
    CLog::LogF(LOGDEBUG, "No stack expressions available. Skipping folder stacking");
    return;
  }

  // stack folders
  for (const auto& item : m_items)
  {
    // combined the folder checks
    if (item->IsFolder())
    {
      // only check known fast sources?
      // NOTES:
      // 1. rars and zips may be on slow sources? is this supposed to be allowed?
      if (!NETWORK::IsRemote(*item) || item->IsSmb() || item->IsNfs() ||
          URIUtils::IsInRAR(item->GetPath()) || URIUtils::IsInZIP(item->GetPath()) ||
          URIUtils::IsOnLAN(item->GetPath()))
      {
        // stack cd# folders if contains only a single video file

        bool bMatch(false);

        auto expr = folderRegExps.begin();
        while (!bMatch && expr != folderRegExps.end())
        {
          //CLog::LogF(LOGDEBUG,"Running expression {} on {}", expr->GetPattern(), item->GetLabel());
          bMatch = (expr->RegFind(item->GetLabel().c_str()) != -1);
          if (bMatch)
          {
            CFileItemList items;
            CDirectory::GetDirectory(
                item->GetPath(), items,
                CServiceBroker::GetFileExtensionProvider().GetVideoExtensions(), DIR_FLAG_DEFAULTS);
            // optimized to only traverse listing once by checking for filecount
            // and recording last file item for later use
            int nFiles = 0;
            int index = -1;
            for (int j = 0; j < items.Size(); j++)
            {
              if (!items[j]->IsFolder())
              {
                nFiles++;
                index = j;
              }

              if (nFiles > 1)
                break;
            }

            if (nFiles == 1)
              *item = *items[index];
          }
          ++expr;
        }

        // check for dvd folders
        if (!bMatch)
        {
          std::string dvdPath = VIDEO::UTILS::GetOpticalMediaPath(*item);

          if (!dvdPath.empty())
          {
            // NOTE: should this be done for the CD# folders too?
            item->SetFolder(false);
            item->SetPath(std::move(dvdPath));
            item->SetLabel2("");
            item->SetLabelPreformatted(true);
            m_sortDescription.sortBy = SortByNone; /* sorting is now broken */
          }
        }
      }
    }
  }
}

void CFileItemList::StackFiles()
{
  std::vector<CRegExp> stackRegExps =
      CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_videoStackRegExps;

  if (stackRegExps.empty())
  {
    CLog::LogF(LOGDEBUG, "No stack expressions available. Skipping file stacking");
    return;
  }

  // now stack the files, some of which may be from the previous stack iteration
  int i = 0;
  while (i < Size())
  {
    CFileItemPtr item1 = Get(i);

    // skip folders, nfo files, playlists
    if (item1->IsFolder() || item1->IsParentFolder() || item1->IsNFO() ||
        PLAYLIST::IsPlayList(*item1))
    {
      // increment index
      i++;
      continue;
    }

    int64_t size = 0;
    size_t offset = 0;
    std::string stackName;
    std::string file1;
    std::string filePath;
    std::vector<int> stack;
    auto expr = stackRegExps.begin();

    URIUtils::Split(item1->GetPath(), filePath, file1);
    if (URIUtils::HasEncodedFilename(CURL(filePath)))
      file1 = CURL::Decode(file1);

    int j;
    while (expr != stackRegExps.end())
    {
      if (expr->RegFind(file1, offset) != -1)
      {
        std::string title1 = expr->GetMatch(1);
        const std::string volume1 = expr->GetMatch(2);
        const std::string ignore1 = expr->GetMatch(3);
        const std::string extension1 = expr->GetMatch(4);
        if (offset)
          title1 = file1.substr(0, expr->GetSubStart(2));
        j = i + 1;
        while (j < Size())
        {
          const CFileItemPtr item2 = Get(j);

          // skip folders, nfo files, playlists
          if (item2->IsFolder() || item2->IsParentFolder() || item2->IsNFO() ||
              PLAYLIST::IsPlayList(*item2))
          {
            // increment index
            j++;
            continue;
          }

          std::string file2;
          std::string filePath2;
          URIUtils::Split(item2->GetPath(), filePath2, file2);
          if (URIUtils::HasEncodedFilename(CURL(filePath2)))
            file2 = CURL::Decode(file2);

          if (expr->RegFind(file2, offset) != -1)
          {
            std::string title2 = expr->GetMatch(1);
            const std::string volume2 = expr->GetMatch(2);
            const std::string ignore2 = expr->GetMatch(3);
            const std::string extension2 = expr->GetMatch(4);
            if (offset)
              title2 = file2.substr(0, expr->GetSubStart(2));
            if (StringUtils::EqualsNoCase(title1, title2))
            {
              if (!StringUtils::EqualsNoCase(volume1, volume2))
              {
                if (StringUtils::EqualsNoCase(ignore1, ignore2) &&
                    StringUtils::EqualsNoCase(extension1, extension2))
                {
                  if (stack.empty())
                  {
                    stackName = title1 + ignore1 + extension1;
                    stack.emplace_back(i);
                    size += item1->GetSize();
                  }
                  stack.emplace_back(j);
                  size += item2->GetSize();
                }
                else // Sequel
                {
                  offset = 0;
                  ++expr;
                  break;
                }
              }
              else if (!StringUtils::EqualsNoCase(ignore1,
                                                  ignore2)) // False positive, try again with offset
              {
                offset = expr->GetSubStart(3);
                break;
              }
              else // Extension mismatch
              {
                offset = 0;
                ++expr;
                break;
              }
            }
            else // Title mismatch
            {
              offset = 0;
              ++expr;
              break;
            }
          }
          else // No match 2, next expression
          {
            offset = 0;
            ++expr;
            break;
          }
          j++;
        }
        if (j == Size())
          expr = stackRegExps.end();
      }
      else // No match 1
      {
        offset = 0;
        ++expr;
      }
      if (stack.size() > 1)
      {
        // have a stack, remove the items and add the stacked item
        // dont actually stack a multipart rar set, just remove all items but the first
        std::string stackPath;
        if (Get(stack[0])->IsRAR())
          stackPath = Get(stack[0])->GetPath();
        else
        {
          stackPath = CStackDirectory::ConstructStackPath(*this, stack);
        }
        item1->SetPath(std::move(stackPath));
        // clean up list
        for (size_t k = 1; k < stack.size(); k++)
          Remove(i + 1);
        // item->SetFolder(true);  // don't treat stacked files as folders
        // the label may be in a different char set from the filename (eg over smb
        // the label is converted from utf8, but the filename is not)
        if (!CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
                CSettings::SETTING_FILELISTS_SHOWEXTENSIONS))
          URIUtils::RemoveExtension(stackName);

        item1->SetLabel(stackName);
        item1->SetSize(size);
        break;
      }
    }
    i++;
  }
}

bool CFileItemList::Load(int windowID)
{
  CFile file;
  auto path = GetDiscFileCache(windowID);
  try
  {
    if (file.Open(path))
    {
      CArchive ar(&file, CArchive::load);
      ar >> *this;
      CLog::Log(LOGDEBUG, "Loading items: {}, directory: {} sort method: {}, ascending: {}", Size(),
                CURL::GetRedacted(GetPath()), m_sortDescription.sortBy,
                m_sortDescription.sortOrder == SortOrderAscending ? "true" : "false");
      ar.Close();
      file.Close();
      return true;
    }
  }
  catch (const std::out_of_range&)
  {
    CLog::Log(LOGERROR, "Corrupt archive: {}", CURL::GetRedacted(path));
  }

  return false;
}

bool CFileItemList::Save(int windowID)
{
  int iSize = Size();
  if (iSize <= 0)
    return false;

  CLog::Log(LOGDEBUG, "Saving fileitems [{}]", CURL::GetRedacted(GetPath()));

  CFile file;
  std::string cachefile = GetDiscFileCache(windowID);
  if (file.OpenForWrite(cachefile, true)) // overwrite always
  {
    // Before caching save simplified cache file name in every item so the cache file can be
    // identified and removed if the item is updated. List path and options (used for file
    // name when list cached) can not be accurately derived from item path.
    StringUtils::Replace(cachefile, "special://temp/archive_cache/", "");
    StringUtils::Replace(cachefile, ".fi", "");
    for (const auto& item : m_items)
      item->SetProperty("cachefilename", cachefile);

    CArchive ar(&file, CArchive::store);
    ar << *this;
    CLog::Log(LOGDEBUG, "  -- items: {}, sort method: {}, ascending: {}", iSize,
              m_sortDescription.sortBy,
              m_sortDescription.sortOrder == SortOrderAscending ? "true" : "false");
    ar.Close();
    file.Close();
    return true;
  }

  return false;
}

void CFileItemList::RemoveDiscCache(int windowID) const
{
  RemoveDiscCache(GetDiscFileCache(windowID));
}

void CFileItemList::RemoveDiscCache(const std::string& cacheFile) const
{
  if (CFile::Exists(cacheFile))
  {
    CLog::Log(LOGDEBUG, "Clearing cached fileitems [{}]", CURL::GetRedacted(GetPath()));
    CFile::Delete(cacheFile);
  }
}

void CFileItemList::RemoveDiscCacheCRC(const std::string& crc) const
{
  std::string cachefile = StringUtils::Format("special://temp/archive_cache/{}.fi", crc);
  RemoveDiscCache(cachefile);
}

std::string CFileItemList::GetDiscFileCache(int windowID) const
{
  std::string strPath(GetPath());
  URIUtils::RemoveSlashAtEnd(strPath);

  uint32_t crc = Crc32::ComputeFromLowerCase(strPath);

  if (MUSIC::IsCDDA(*this) || IsOnDVD())
    return StringUtils::Format("special://temp/archive_cache/r-{:08x}.fi", crc);

  if (MUSIC::IsMusicDb(*this))
    return StringUtils::Format("special://temp/archive_cache/mdb-{:08x}.fi", crc);

  if (VIDEO::IsVideoDb(*this))
    return StringUtils::Format("special://temp/archive_cache/vdb-{:08x}.fi", crc);

  if (PLAYLIST::IsSmartPlayList(*this))
    return StringUtils::Format("special://temp/archive_cache/sp-{:08x}.fi", crc);

  if (windowID)
    return StringUtils::Format("special://temp/archive_cache/{}-{:08x}.fi", windowID, crc);

  return StringUtils::Format("special://temp/archive_cache/{:08x}.fi", crc);
}

bool CFileItemList::AlwaysCache() const
{
  // some database folders are always cached
  if (MUSIC::IsMusicDb(*this))
    return CMusicDatabaseDirectory::CanCache(GetPath());
  if (VIDEO::IsVideoDb(*this))
    return CVideoDatabaseDirectory::CanCache(GetPath());
  if (IsEPG())
    return true; // always cache
  return false;
}

void CFileItemList::Swap(unsigned int item1, unsigned int item2)
{
  if (item1 != item2 && item1 < m_items.size() && item2 < m_items.size())
    std::swap(m_items[item1], m_items[item2]);
}

bool CFileItemList::UpdateItem(const CFileItem* item)
{
  if (!item)
    return false;

  std::unique_lock lock(m_lock);
  const auto it =
      std::ranges::find_if(m_items, [&item](const auto& pItem) { return pItem->IsSamePath(item); });
  if (it != m_items.end())
    (*it)->UpdateInfo(*item);

  return it != m_items.end();
}

void CFileItemList::AddSortMethod(SortBy sortBy,
                                  int buttonLabel,
                                  const LABEL_MASKS& labelMasks,
                                  SortAttribute sortAttributes /* = SortAttributeNone */)
{
  AddSortMethod(sortBy, sortAttributes, buttonLabel, labelMasks);
}

void CFileItemList::AddSortMethod(SortBy sortBy,
                                  SortAttribute sortAttributes,
                                  int buttonLabel,
                                  const LABEL_MASKS& labelMasks)
{
  SortDescription sorting;
  sorting.sortBy = sortBy;
  sorting.sortAttributes = sortAttributes;

  AddSortMethod(sorting, buttonLabel, labelMasks);
}

void CFileItemList::AddSortMethod(const SortDescription& sortDescription,
                                  int buttonLabel,
                                  const LABEL_MASKS& labelMasks)
{
  GUIViewSortDetails sort;
  sort.m_sortDescription = sortDescription;
  sort.m_buttonLabel = buttonLabel;
  sort.m_labelMasks = labelMasks;

  m_sortDetails.emplace_back(std::move(sort));
}

void CFileItemList::SetReplaceListing(bool replace)
{
  m_replaceListing = replace;
}

void CFileItemList::ClearSortState()
{
  m_sortDescription.sortBy = SortByNone;
  m_sortDescription.sortOrder = SortOrderNone;
  m_sortDescription.sortAttributes = SortAttributeNone;
}
