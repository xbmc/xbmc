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

CFileItemPtr CFileItemList::operator[](int iItem)
{
  return Get(iItem);
}

const CFileItemPtr CFileItemList::operator[](int iItem) const
{
  return Get(iItem);
}

CFileItemPtr CFileItemList::operator[](const std::string& strPath)
{
  return Get(strPath);
}

const CFileItemPtr CFileItemList::operator[](const std::string& strPath) const
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
  std::unique_lock<CCriticalSection> lock(m_lock);

  if (fastLookup && !m_fastLookup)
  { // generate the map
    m_map.clear();
    for (unsigned int i = 0; i < m_items.size(); i++)
    {
      CFileItemPtr pItem = m_items[i];
      m_map.insert(MAPFILEITEMSPAIR(m_ignoreURLOptions ? CURL(pItem->GetPath()).GetWithoutOptions()
                                                       : pItem->GetPath(),
                                    pItem));
    }
  }
  if (!fastLookup && m_fastLookup)
    m_map.clear();
  m_fastLookup = fastLookup;
}

bool CFileItemList::Contains(const std::string& fileName) const
{
  std::unique_lock<CCriticalSection> lock(m_lock);

  if (m_fastLookup)
    return m_map.find(m_ignoreURLOptions ? CURL(fileName).GetWithoutOptions() : fileName) !=
           m_map.end();

  // slow method...
  for (unsigned int i = 0; i < m_items.size(); i++)
  {
    const CFileItemPtr pItem = m_items[i];
    if (pItem->IsPath(m_ignoreURLOptions ? CURL(fileName).GetWithoutOptions() : fileName))
      return true;
  }
  return false;
}

void CFileItemList::Clear()
{
  std::unique_lock<CCriticalSection> lock(m_lock);

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
  std::unique_lock<CCriticalSection> lock(m_lock);
  // make sure we free the memory of the items (these are GUIControls which may have allocated resources)
  FreeMemory();
  for (unsigned int i = 0; i < m_items.size(); i++)
  {
    CFileItemPtr item = m_items[i];
    item->FreeMemory();
  }
  m_items.clear();
  m_map.clear();
}

void CFileItemList::Add(CFileItemPtr pItem)
{
  std::unique_lock<CCriticalSection> lock(m_lock);
  if (m_fastLookup)
    m_map.insert(MAPFILEITEMSPAIR(
        m_ignoreURLOptions ? CURL(pItem->GetPath()).GetWithoutOptions() : pItem->GetPath(), pItem));
  m_items.emplace_back(std::move(pItem));
}

void CFileItemList::Add(CFileItem&& item)
{
  std::unique_lock<CCriticalSection> lock(m_lock);
  auto ptr = std::make_shared<CFileItem>(std::move(item));
  if (m_fastLookup)
    m_map.insert(MAPFILEITEMSPAIR(
        m_ignoreURLOptions ? CURL(ptr->GetPath()).GetWithoutOptions() : ptr->GetPath(), ptr));
  m_items.emplace_back(std::move(ptr));
}

void CFileItemList::AddFront(const CFileItemPtr& pItem, int itemPosition)
{
  std::unique_lock<CCriticalSection> lock(m_lock);

  if (itemPosition >= 0)
  {
    m_items.insert(m_items.begin() + itemPosition, pItem);
  }
  else
  {
    m_items.insert(m_items.begin() + (m_items.size() + itemPosition), pItem);
  }
  if (m_fastLookup)
  {
    m_map.insert(MAPFILEITEMSPAIR(
        m_ignoreURLOptions ? CURL(pItem->GetPath()).GetWithoutOptions() : pItem->GetPath(), pItem));
  }
}

void CFileItemList::Remove(CFileItem* pItem)
{
  std::unique_lock<CCriticalSection> lock(m_lock);

  for (IVECFILEITEMS it = m_items.begin(); it != m_items.end(); ++it)
  {
    if (pItem == it->get())
    {
      m_items.erase(it);
      if (m_fastLookup)
      {
        m_map.erase(m_ignoreURLOptions ? CURL(pItem->GetPath()).GetWithoutOptions()
                                       : pItem->GetPath());
      }
      break;
    }
  }
}

VECFILEITEMS::iterator CFileItemList::erase(VECFILEITEMS::iterator first,
                                            VECFILEITEMS::iterator last)
{
  std::unique_lock<CCriticalSection> lock(m_lock);
  return m_items.erase(first, last);
}

void CFileItemList::Remove(int iItem)
{
  std::unique_lock<CCriticalSection> lock(m_lock);

  if (iItem >= 0 && iItem < Size())
  {
    CFileItemPtr pItem = *(m_items.begin() + iItem);
    if (m_fastLookup)
    {
      m_map.erase(m_ignoreURLOptions ? CURL(pItem->GetPath()).GetWithoutOptions()
                                     : pItem->GetPath());
    }
    m_items.erase(m_items.begin() + iItem);
  }
}

void CFileItemList::Append(const CFileItemList& itemlist)
{
  std::unique_lock<CCriticalSection> lock(m_lock);

  for (int i = 0; i < itemlist.Size(); ++i)
    Add(itemlist[i]);
}

void CFileItemList::Assign(const CFileItemList& itemlist, bool append)
{
  std::unique_lock<CCriticalSection> lock(m_lock);
  if (!append)
    Clear();
  Append(itemlist);
  SetPath(itemlist.GetPath());
  SetLabel(itemlist.GetLabel());
  m_sortDetails = itemlist.m_sortDetails;
  m_sortDescription = itemlist.m_sortDescription;
  m_replaceListing = itemlist.m_replaceListing;
  m_content = itemlist.m_content;
  m_mapProperties = itemlist.m_mapProperties;
  m_cacheToDisc = itemlist.m_cacheToDisc;
}

bool CFileItemList::Copy(const CFileItemList& items, bool copyItems /* = true */)
{
  // assign all CFileItem parts
  *static_cast<CFileItem*>(this) = static_cast<const CFileItem&>(items);

  // assign the rest of the CFileItemList properties
  m_replaceListing = items.m_replaceListing;
  m_content = items.m_content;
  m_mapProperties = items.m_mapProperties;
  m_cacheToDisc = items.m_cacheToDisc;
  m_sortDetails = items.m_sortDetails;
  m_sortDescription = items.m_sortDescription;
  m_sortIgnoreFolders = items.m_sortIgnoreFolders;

  if (copyItems)
  {
    // make a copy of each item
    for (int i = 0; i < items.Size(); i++)
    {
      CFileItemPtr newItem(new CFileItem(*items[i]));
      Add(newItem);
    }
  }

  return true;
}

CFileItemPtr CFileItemList::Get(int iItem) const
{
  std::unique_lock<CCriticalSection> lock(m_lock);

  if (iItem > -1 && iItem < (int)m_items.size())
    return m_items[iItem];

  return CFileItemPtr();
}

CFileItemPtr CFileItemList::Get(const std::string& strPath) const
{
  std::unique_lock<CCriticalSection> lock(m_lock);

  if (m_fastLookup)
  {
    MAPFILEITEMS::const_iterator it =
        m_map.find(m_ignoreURLOptions ? CURL(strPath).GetWithoutOptions() : strPath);
    if (it != m_map.end())
      return it->second;

    return CFileItemPtr();
  }
  // slow method...
  for (unsigned int i = 0; i < m_items.size(); i++)
  {
    CFileItemPtr pItem = m_items[i];
    if (pItem->IsPath(m_ignoreURLOptions ? CURL(strPath).GetWithoutOptions() : strPath))
      return pItem;
  }

  return CFileItemPtr();
}

int CFileItemList::Size() const
{
  std::unique_lock<CCriticalSection> lock(m_lock);
  return (int)m_items.size();
}

bool CFileItemList::IsEmpty() const
{
  std::unique_lock<CCriticalSection> lock(m_lock);
  return m_items.empty();
}

void CFileItemList::Reserve(size_t iCount)
{
  std::unique_lock<CCriticalSection> lock(m_lock);
  m_items.reserve(iCount);
}

void CFileItemList::Sort(FILEITEMLISTCOMPARISONFUNC func)
{
  std::unique_lock<CCriticalSection> lock(m_lock);
  std::stable_sort(m_items.begin(), m_items.end(), func);
}

void CFileItemList::FillSortFields(FILEITEMFILLFUNC func)
{
  std::unique_lock<CCriticalSection> lock(m_lock);
  std::for_each(m_items.begin(), m_items.end(), func);
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
  if (sortDescription.sortBy == SortByFile || sortDescription.sortBy == SortBySortTitle ||
      sortDescription.sortBy == SortByOriginalTitle || sortDescription.sortBy == SortByDateAdded ||
      sortDescription.sortBy == SortByRating || sortDescription.sortBy == SortByYear ||
      sortDescription.sortBy == SortByPlaylistOrder || sortDescription.sortBy == SortByLastPlayed ||
      sortDescription.sortBy == SortByPlaycount)
    sortDescription.sortAttributes =
        (SortAttribute)((int)sortDescription.sortAttributes | SortAttributeIgnoreFolders);

  if (sortDescription.sortBy == SortByNone ||
      (m_sortDescription.sortBy == sortDescription.sortBy &&
       m_sortDescription.sortOrder == sortDescription.sortOrder &&
       m_sortDescription.sortAttributes == sortDescription.sortAttributes))
    return;

  if (m_sortIgnoreFolders)
    sortDescription.sortAttributes =
        (SortAttribute)((int)sortDescription.sortAttributes | SortAttributeIgnoreFolders);

  const Fields fields = SortUtils::GetFieldsForSorting(sortDescription.sortBy);
  SortItems sortItems((size_t)Size());
  for (int index = 0; index < Size(); index++)
  {
    sortItems[index] = std::make_shared<SortItem>();
    m_items[index]->ToSortable(*sortItems[index], fields);
    (*sortItems[index])[FieldId] = index;
  }

  // do the sorting
  SortUtils::Sort(sortDescription, sortItems);

  // apply the new order to the existing CFileItems
  VECFILEITEMS sortedFileItems;
  sortedFileItems.reserve(Size());
  for (SortItems::const_iterator it = sortItems.begin(); it != sortItems.end(); ++it)
  {
    CFileItemPtr item = m_items[(int)(*it)->at(FieldId).asInteger()];
    // Set the sort label in the CFileItem
    item->SetSortLabel((*it)->at(FieldSort).asWideString());

    sortedFileItems.push_back(item);
  }

  // replace the current list with the re-ordered one
  m_items = std::move(sortedFileItems);
}

void CFileItemList::Randomize()
{
  std::unique_lock<CCriticalSection> lock(m_lock);
  KODI::UTILS::RandomShuffle(m_items.begin(), m_items.end());
}

void CFileItemList::Archive(CArchive& ar)
{
  std::unique_lock<CCriticalSection> lock(m_lock);
  if (ar.IsStoring())
  {
    CFileItem::Archive(ar);

    int i = 0;
    if (!m_items.empty() && m_items[0]->IsParentFolder())
      i = 1;

    ar << (int)(m_items.size() - i);

    ar << m_ignoreURLOptions;

    ar << m_fastLookup;

    ar << (int)m_sortDescription.sortBy;
    ar << (int)m_sortDescription.sortOrder;
    ar << (int)m_sortDescription.sortAttributes;
    ar << m_sortIgnoreFolders;
    ar << (int)m_cacheToDisc;

    ar << (int)m_sortDetails.size();
    for (unsigned int j = 0; j < m_sortDetails.size(); ++j)
    {
      const GUIViewSortDetails& details = m_sortDetails[j];
      ar << (int)details.m_sortDescription.sortBy;
      ar << (int)details.m_sortDescription.sortOrder;
      ar << (int)details.m_sortDescription.sortAttributes;
      ar << details.m_buttonLabel;
      ar << details.m_labelMasks.m_strLabelFile;
      ar << details.m_labelMasks.m_strLabelFolder;
      ar << details.m_labelMasks.m_strLabel2File;
      ar << details.m_labelMasks.m_strLabel2Folder;
    }

    ar << m_content;

    for (; i < (int)m_items.size(); ++i)
    {
      CFileItemPtr pItem = m_items[i];
      ar << *pItem;
    }
  }
  else
  {
    CFileItemPtr pParent;
    if (!IsEmpty())
    {
      CFileItemPtr pItem = m_items[0];
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
      m_items.push_back(pParent);
    }
    else
      m_items.reserve(iSize);

    bool ignoreURLOptions = false;
    ar >> ignoreURLOptions;

    bool fastLookup = false;
    ar >> fastLookup;

    int tempint;
    ar >> tempint;
    m_sortDescription.sortBy = (SortBy)tempint;
    ar >> tempint;
    m_sortDescription.sortOrder = (SortOrder)tempint;
    ar >> tempint;
    m_sortDescription.sortAttributes = (SortAttribute)tempint;
    ar >> m_sortIgnoreFolders;
    ar >> tempint;
    m_cacheToDisc = CacheType(tempint);

    unsigned int detailSize = 0;
    ar >> detailSize;
    for (unsigned int j = 0; j < detailSize; ++j)
    {
      GUIViewSortDetails details;
      ar >> tempint;
      details.m_sortDescription.sortBy = (SortBy)tempint;
      ar >> tempint;
      details.m_sortDescription.sortOrder = (SortOrder)tempint;
      ar >> tempint;
      details.m_sortDescription.sortAttributes = (SortAttribute)tempint;
      ar >> details.m_buttonLabel;
      ar >> details.m_labelMasks.m_strLabelFile;
      ar >> details.m_labelMasks.m_strLabelFolder;
      ar >> details.m_labelMasks.m_strLabel2File;
      ar >> details.m_labelMasks.m_strLabel2Folder;
      m_sortDetails.push_back(details);
    }

    ar >> m_content;

    for (int i = 0; i < iSize; ++i)
    {
      CFileItemPtr pItem(new CFileItem);
      ar >> *pItem;
      Add(pItem);
    }

    SetIgnoreURLOptions(ignoreURLOptions);
    SetFastLookup(fastLookup);
  }
}

void CFileItemList::FillInDefaultIcons()
{
  std::unique_lock<CCriticalSection> lock(m_lock);
  for (int i = 0; i < (int)m_items.size(); ++i)
  {
    CFileItemPtr pItem = m_items[i];
    ART::FillInDefaultIcon(*pItem);
  }
}

int CFileItemList::GetFolderCount() const
{
  std::unique_lock<CCriticalSection> lock(m_lock);
  int nFolderCount = 0;
  for (int i = 0; i < (int)m_items.size(); i++)
  {
    CFileItemPtr pItem = m_items[i];
    if (pItem->m_bIsFolder)
      nFolderCount++;
  }

  return nFolderCount;
}

int CFileItemList::GetObjectCount() const
{
  std::unique_lock<CCriticalSection> lock(m_lock);

  int numObjects = (int)m_items.size();
  if (numObjects && m_items[0]->IsParentFolder())
    numObjects--;

  return numObjects;
}

int CFileItemList::GetFileCount() const
{
  std::unique_lock<CCriticalSection> lock(m_lock);
  int nFileCount = 0;
  for (int i = 0; i < (int)m_items.size(); i++)
  {
    CFileItemPtr pItem = m_items[i];
    if (!pItem->m_bIsFolder)
      nFileCount++;
  }

  return nFileCount;
}

int CFileItemList::GetSelectedCount() const
{
  std::unique_lock<CCriticalSection> lock(m_lock);
  int count = 0;
  for (int i = 0; i < (int)m_items.size(); i++)
  {
    CFileItemPtr pItem = m_items[i];
    if (pItem->IsSelected())
      count++;
  }

  return count;
}

void CFileItemList::FilterCueItems()
{
  std::unique_lock<CCriticalSection> lock(m_lock);
  // Handle .CUE sheet files...
  std::vector<std::string> itemstodelete;
  for (int i = 0; i < (int)m_items.size(); i++)
  {
    CFileItemPtr pItem = m_items[i];
    if (!pItem->m_bIsFolder)
    { // see if it's a .CUE sheet
      if (MUSIC::IsCUESheet(*pItem))
      {
        CCueDocumentPtr cuesheet(new CCueDocument);
        if (cuesheet->ParseFile(pItem->GetPath()))
        {
          std::vector<std::string> MediaFileVec;
          cuesheet->GetMediaFiles(MediaFileVec);

          // queue the cue sheet and the underlying media file for deletion
          for (std::vector<std::string>::iterator itMedia = MediaFileVec.begin();
               itMedia != MediaFileVec.end(); ++itMedia)
          {
            std::string strMediaFile = *itMedia;
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
                  for (std::vector<std::string>::const_iterator i = extensions.begin();
                       i != extensions.end(); ++i)
                  {
                    strMediaFile = URIUtils::ReplaceExtension(pItem->GetPath(), *i);
                    CFileItem item(strMediaFile, false);
                    if (!MUSIC::IsCUESheet(item) && !PLAYLIST::IsPlayList(item) &&
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
              for (int j = 0; j < (int)m_items.size(); j++)
              {
                CFileItemPtr pItem = m_items[j];
                if (StringUtils::CompareNoCase(pItem->GetPath(), strMediaFile) == 0)
                  pItem->SetCueDocument(cuesheet);
              }
            }
          }
        }
        itemstodelete.push_back(pItem->GetPath());
      }
    }
  }
  // now delete the .CUE files.
  for (int i = 0; i < (int)itemstodelete.size(); i++)
  {
    for (int j = 0; j < (int)m_items.size(); j++)
    {
      CFileItemPtr pItem = m_items[j];
      if (StringUtils::CompareNoCase(pItem->GetPath(), itemstodelete[i]) == 0)
      { // delete this item
        m_items.erase(m_items.begin() + j);
        break;
      }
    }
  }
}

// Remove the extensions from the filenames
void CFileItemList::RemoveExtensions()
{
  std::unique_lock<CCriticalSection> lock(m_lock);
  for (int i = 0; i < Size(); ++i)
    m_items[i]->RemoveExtension();
}

void CFileItemList::ConvertDiscFoldersToFiles()
{
  for (const auto& item : m_items)
  {
    const std::string playPath = VIDEO::UTILS::GetOpticalMediaPath(*item);
    if (!playPath.empty())
      ConvertDiscFolderToFile(item, playPath);
  }
}

void CFileItemList::ConvertDiscFolderToFile(const std::shared_ptr<CFileItem>& item,
                                            const std::string& playPath)
{

  item->SetPath(playPath); // updated path (for DVD/Bluray files)
  item->m_bIsFolder = false;
  item->SetLabel2("");
  item->SetLabelPreformatted(true);
  m_sortDescription.sortBy = SortByNone; // sorting is now broken
}

static constexpr int FOLDER_CANDIDATE = 1;
static constexpr int FILE_CANDIDATE = 2;

void CFileItemList::Stack()
{
  std::unique_lock<CCriticalSection> lock(m_lock);

  // not allowed here
  if (IsVirtualDirectoryRoot() || IsLiveTV() || IsSourcesPath() || IsLibraryFolder())
    return;

  SetProperty("isstacked", true);

  // items needs to be sorted for stuff below to work properly
  Sort(SortByLabel, SortOrderAscending);

  // Convert folder paths containing disc images to files (INDEX.BDMV or VIDEO_TS.IFO)
  ConvertDiscFoldersToFiles();

  // Get REs
  VECCREGEXP folderRegExps;
  VECCREGEXP fileRegExps;
  CStackDirectory::LoadRegExps(fileRegExps, folderRegExps);

  if (fileRegExps.empty() || folderRegExps.empty())
    CLog::LogF(LOGDEBUG, "No stack expressions available. Skipping stacking");
  else
  {
    // Find stack candidates (ie. any file or directory that matches a RegEx)
    std::vector<std::tuple<int, std::string, std::string, int, int64_t>> stackCandidates;

    for (const auto& item : m_items)
    {
      // Fast sources
      // RARs and ZIPs may be on slow sources? is this supposed to be allowed?
      if ((item->m_bIsFolder || VIDEO::IsDVDFile(*item) || VIDEO::IsBDFile(*item)) &&
          (!NETWORK::IsRemote(*item) || item->IsSmb() || item->IsNfs() ||
           URIUtils::IsInRAR(item->GetPath()) || URIUtils::IsInZIP(item->GetPath()) ||
           URIUtils::IsOnLAN(item->GetPath())))
      {
        // Folder stacking (for BD/DVD files/images)
        std::string folder{StringUtils::ToLower(item->GetLabel())};
        URIUtils::RemoveSlashAtEnd(folder);

        // Test each item against each RegExp
        for (auto& regExp : folderRegExps)
        {
          if (regExp.RegFind(folder) != -1)
          {
            // Directory matches RegEx so get image files
            // BD/DVD files (not ISOs) are dealt with in ConvertDiscFoldersToFiles()
            bool fileFound{true};
            if (item->m_bIsFolder)
            {
              // Look for disc images here (by extension)
              CFileItemList items;
              CDirectory::GetDirectory(
                  item->GetPath(), items,
                  CServiceBroker::GetFileExtensionProvider().GetVideoExtensions(),
                  DIR_FLAG_DEFAULTS);

              // Only expect one disc image per folder (if >1 should be a file stack)
              if (items.GetFileCount() == 1)
                ConvertDiscFolderToFile(item, items[0]->GetPath());
              else
                fileFound = false;
            }
            if (fileFound)
            {
              // Add to stack vector
              stackCandidates.emplace_back(FOLDER_CANDIDATE, regExp.GetMatch(1) /* title */,
                                           regExp.GetMatch(2) /* volume */,
                                           static_cast<int>(&item - &m_items[0]), item->m_dwSize);
              break;
            }
          }
        }
      }
      else if (!(item->m_bIsFolder || item->IsParentFolder() || item->IsNFO() ||
                 PLAYLIST::IsPlayList(*item)))
      {
        // File stacking
        std::string file{};
        std::string filePath{};
        URIUtils::Split(StringUtils::ToLower(item->GetPath()), filePath, file);
        if (URIUtils::HasEncodedFilename(CURL(filePath)))
          file = CURL::Decode(file);

        // Test each item against each RegExp
        for (auto& regExp : fileRegExps)
        {
          if (regExp.RegFind(file) != -1)
          {
            // Get components of file name
            stackCandidates.emplace_back(FILE_CANDIDATE, regExp.GetMatch(1) /* Title */,
                                         regExp.GetMatch(2) /* Volume */,
                                         static_cast<int>(&item - &m_items[0]), item->m_dwSize);
            break;
          }
        }
      }
    }

    // Sort stack candidates
    std::sort(stackCandidates.begin(), stackCandidates.end(),
              [](const auto& s, const auto& t)
              {
                if (std::get<0>(s) == std::get<0>(t)) // type (file/folder)
                {
                  if (std::get<1>(s) == std::get<1>(t)) // title
                    return std::get<2>(s) < std::get<2>(t); // volume
                  else
                    return std::get<1>(s) < std::get<1>(t);
                }
                else
                  return std::get<0>(s) < std::get<0>(t);
              });

    // Count stack candidates
    std::map<std::pair<int, std::string>, int> countCandidates;
    for (const auto& s : stackCandidates)
      ++countCandidates[std::make_pair(std::get<0>(s), std::get<1>(s))];

    // Find stacks (and collect items for deletion)
    std::vector<int> deleteItems;
    for (const auto& stackBase : countCandidates)
    {
      const auto& stackBaseItem = stackBase.first;
      if (stackBase.second > 1) // more than one item to stack
      {
        // Find all items in stack
        std::vector<int> stack;
        int64_t size{0};

        for (const auto& stackItem : stackCandidates)
        {
          if (stackBaseItem.first == std::get<0>(stackItem) && // type
              stackBaseItem.second == std::get<1>(stackItem)) // title
          {
            const int index{std::get<3>(stackItem)}; // index
            stack.emplace_back(index);
            size += std::get<4>(stackItem); // size
            if (stack.size() > 1)
              deleteItems.emplace_back(index); // delete all but first
          }
        }

        // Create stack
        // Generate combined stack path
        std::string stackPath{};
        if (Get(stack[0])->IsRAR())
          stackPath = Get(stack[0])->GetPath();
        else
          stackPath = CStackDirectory::ConstructStackPath(*this, stack);

        // First item in stack becomes the stack
        std::string stackName = std::get<1>(stackBaseItem); // title
        if (!CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
                CSettings::SETTING_FILELISTS_SHOWEXTENSIONS))
          URIUtils::RemoveExtension(stackName);

        // Update item
        const std::shared_ptr<CFileItem> stackItem{Get(stack[0])};
        stackItem->SetPath(stackPath);
        stackItem->SetLabel(stackName);
        stackItem->m_dwSize = size;
      }
    }

    // Delete unneeded items
    std::sort(deleteItems.begin(), deleteItems.end(),
              [](const int i, const int j) { return i > j; });
    for (int i : deleteItems)
      Remove(i);
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
    // identifed and removed if the item is updated. List path and options (used for file
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

  std::unique_lock<CCriticalSection> lock(m_lock);
  for (unsigned int i = 0; i < m_items.size(); i++)
  {
    CFileItemPtr pItem = m_items[i];
    if (pItem->IsSamePath(item))
    {
      pItem->UpdateInfo(*item);
      return true;
    }
  }
  return false;
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

  m_sortDetails.push_back(sort);
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
