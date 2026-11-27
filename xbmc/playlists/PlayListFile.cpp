/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PlayListFile.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "filesystem/File.h"
#include "utils/ItemProperties.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

#include <iostream>
#include <sstream>

using namespace XFILE;

namespace KODI::PLAYLIST
{

bool CPlayListFile::Load(const std::string& strFileName)
{
  Clear();
  m_strBasePath = URIUtils::GetDirectory(strFileName);

  CFileStream file;
  if (!file.Open(strFileName))
    return false;

  if (file.GetLength() > 1024 * 1024)
  {
    CLog::LogF(LOGWARNING, "File is larger than 1 MB, most likely not a playlist");
    return false;
  }

  return LoadData(file);
}

bool CPlayListFile::LoadData(std::istream& stream)
{
  // try to read as a string
  std::ostringstream ostr;
  ostr << stream.rdbuf();
  return LoadData(ostr.str());
}

bool CPlayListFile::LoadData(const std::string& strData)
{
  return false;
}

void CPlayListFile::Add(const std::shared_ptr<CFileItem>& item)
{
  auto owned = std::make_shared<CFileItem>(*item);

  // set 'IsPlayable' property - needed for properly handling plugin:// URLs
  owned->SetProperty(ITEM::PROPERTY::IS_PLAYABLE, true);

  // set 'BasePath' property - needed for properly handling browse for subtitles
  if (!owned->HasProperty("BasePath"))
    owned->SetProperty("BasePath", m_strBasePath);

  m_items.push_back(std::move(owned));
}

void CPlayListFile::Add(const CFileItemList& items)
{
  for (const auto& item : items)
    Add(item);
}

void CPlayListFile::Add(const CPlayListFile& playlist)
{
  for (const auto& item : playlist.m_items)
    Add(item);
}

void CPlayListFile::Clear()
{
  m_items.clear();
  m_strPlayListName.clear();
}

std::shared_ptr<CFileItem> CPlayListFile::operator[](int iItem) const
{
  if (iItem < 0 || iItem >= Size())
  {
    CLog::Log(LOGERROR, "Error trying to retrieve an item that's out of range");
    return {};
  }
  return m_items[iItem];
}

void CPlayListFile::GetItems(CFileItemList& items) const
{
  items.Reserve(items.Size() + Size());
  for (const auto& item : m_items)
    items.Add(item);
}

} // namespace KODI::PLAYLIST
