/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PictureInfoLoader.h"

#include "CompileInfo.h"
#include "FileItem.h"
#include "FileItemList.h"
#include "PictureInfoTag.h"
#include "ServiceBroker.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "network/NetworkFileItemClassify.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/Archive.h"
#include "utils/ArtTypes.h"
#include "utils/Crc32.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "video/VideoFileItemClassify.h"

#include <exception>

using namespace KODI;

namespace
{
// Unlike special://temp/archive_cache, this survives a restart. Each cached item is only used
// while its file's date matches, and a cache is only read by the build that wrote it.
constexpr const char* CACHE_FOLDER = "special://temp/picture_cache/";

std::string GetCacheFile(const std::string& folder)
{
  std::string path = folder;
  URIUtils::RemoveSlashAtEnd(path);
  return StringUtils::Format("{}{:08x}.fi", CACHE_FOLDER, Crc32::ComputeFromLowerCase(path));
}
} // namespace

CPictureInfoLoader::CPictureInfoLoader()
{
  m_mapFileItems = new CFileItemList;
  m_tagReads = 0;
}

CPictureInfoLoader::~CPictureInfoLoader()
{
  StopThread();
  delete m_mapFileItems;
}

void CPictureInfoLoader::OnLoaderStart()
{
  // Load previously cached items from HD
  m_mapFileItems->SetPath(m_pVecItems->GetPath());
  XFILE::CFile file;
  if (file.Open(GetCacheFile(m_pVecItems->GetPath())))
  {
    try
    {
      CArchive ar(&file, CArchive::load);
      std::string build;
      ar >> build;
      if (build == CCompileInfo::GetSCMID())
        ar >> *m_mapFileItems;
      ar.Close();
    }
    catch (const std::exception&)
    {
      m_mapFileItems->Clear();
    }
  }
  m_mapFileItems->SetFastLookup(true);

  m_tagReads = 0;
  m_loadTags = CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_PICTURES_USETAGS);

  if (m_pProgressCallback)
    m_pProgressCallback->SetProgressMax(m_pVecItems->GetFileCount());
}

bool CPictureInfoLoader::LoadItem(CFileItem* pItem)
{
  bool result  = LoadItemCached(pItem);
       result |= LoadItemLookup(pItem);

  return result;
}

bool CPictureInfoLoader::LoadItemCached(CFileItem* pItem)
{
  if (!pItem->IsPicture() || pItem->IsZIP() || pItem->IsRAR() || pItem->IsCBR() || pItem->IsCBZ() ||
      NETWORK::IsInternetStream(*pItem) || VIDEO::IsVideo(*pItem))
    return false;

  if (pItem->HasPictureInfoTag())
    return true;

  // Check the cached item
  CFileItemPtr mapItem = (*m_mapFileItems)[pItem->GetPath()];
  if (mapItem && mapItem->HasPictureInfoTag() && mapItem->GetPictureInfoTag()->Loaded() &&
      mapItem->GetDateTime() == pItem->GetDateTime())
  { // Query map if we previously cached the file on HD
    *pItem->GetPictureInfoTag() = *mapItem->GetPictureInfoTag();
    pItem->SetArt(ART::TYPE::THUMB, mapItem->GetArt(ART::TYPE::THUMB));
    return true;
  }

  return true;
}

bool CPictureInfoLoader::LoadItemLookup(CFileItem* pItem)
{
  if (m_pProgressCallback && !pItem->IsFolder())
    m_pProgressCallback->SetProgressAdvance();

  if (!pItem->IsPicture() || pItem->IsZIP() || pItem->IsRAR() || pItem->IsCBR() || pItem->IsCBZ() ||
      NETWORK::IsInternetStream(*pItem) || VIDEO::IsVideo(*pItem))
    return false;

  if (pItem->HasPictureInfoTag())
    return false;

  if (m_loadTags)
  { // Nothing found, load tag from file
    pItem->GetPictureInfoTag()->Load(pItem->GetPath());
    m_tagReads++;
  }

  return true;
}

void CPictureInfoLoader::OnLoaderFinish()
{
  // cleanup cache loaded from HD
  m_mapFileItems->Clear();

  // Save loaded items to HD
  if (!m_bStop && m_tagReads > 0)
  {
    // Written aside first, so an interrupted write never leaves part of a cache behind. The old
    // one is deleted before the rename because Windows won't rename over it.
    const std::string cacheFile = GetCacheFile(m_pVecItems->GetPath());
    const std::string partFile = cacheFile + ".part";
    XFILE::CFile file;
    if (XFILE::CDirectory::Create(CACHE_FOLDER) && file.OpenForWrite(partFile, true))
    {
      CArchive ar(&file, CArchive::store);
      ar << std::string(CCompileInfo::GetSCMID());
      ar << *m_pVecItems;
      ar.Close();
      file.Close();
      XFILE::CFile::Delete(cacheFile);
      if (!XFILE::CFile::Rename(partFile, cacheFile))
        XFILE::CFile::Delete(partFile);
    }
  }
}
