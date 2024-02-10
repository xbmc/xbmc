/*
 *  Copyright (C) 2015-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "XbtManager.h"

#include "URL.h"
#include "guilib/XBTF.h"
#include "guilib/XBTFReader.h"

#include <utility>

namespace XFILE
{

CXbtManager::CXbtManager() = default;

CXbtManager::~CXbtManager() = default;

CXbtManager& CXbtManager::GetInstance()
{
  static CXbtManager xbtManager;
  return xbtManager;
}

bool CXbtManager::HasFiles(const CURL& path) const
{
  std::lock_guard l(m_lock);
  return ProcessFile(path) != m_readers.end();
}

bool CXbtManager::GetFiles(const CURL& path, std::vector<CXBTFFile>& files) const
{
  std::lock_guard l(m_lock);
  const auto& reader = ProcessFile(path);
  if (reader == m_readers.end())
    return false;

  files = reader->second.reader->GetFiles();
  return true;
}

bool CXbtManager::GetReader(const CURL& path, CXBTFReaderPtr& reader) const
{
  std::lock_guard l(m_lock);
  const auto& it = ProcessFile(path);
  if (it == m_readers.end())
    return false;

  reader = it->second.reader;
  return true;
}

void CXbtManager::Release(const CURL& path)
{
  std::lock_guard l(m_lock);
  const auto& it = GetReader(path);
  if (it == m_readers.end())
    return;

  RemoveReader(it);
}

CXbtManager::XBTFReaders::iterator CXbtManager::GetReader(const CURL& path) const
{
  return GetReader(NormalizePath(path));
}

CXbtManager::XBTFReaders::iterator CXbtManager::GetReader(const std::string& path) const
{
  if (path.empty())
    return m_readers.end();

  return m_readers.find(path);
}

void CXbtManager::RemoveReader(XBTFReaders::iterator readerIterator) const
{
  if (readerIterator == m_readers.end())
    return;

  // close the reader
  readerIterator->second.reader->Close();

  // and remove it from the map
  m_readers.erase(readerIterator);
}

CXbtManager::XBTFReaders::const_iterator CXbtManager::ProcessFile(const CURL& path) const
{
  std::string filePath = NormalizePath(path);

  // check if the file is known
  auto it = GetReader(filePath);
  if (it != m_readers.end())
  {
    // check if the XBT file has been modified
    if (it->second.reader->GetLastModificationTimestamp() <= it->second.lastModification)
      return it;

    // the XBT file has been modified so close and remove it from the cache
    // it will be re-opened by the following logic
    RemoveReader(it);
  }

  // try to read the file
  CXBTFReaderPtr reader(new CXBTFReader());
  if (!reader->Open(filePath))
    return m_readers.end();

  XBTFReader xbtfReader = {
    reader,
    reader->GetLastModificationTimestamp()
  };
  std::pair<XBTFReaders::iterator, bool> result = m_readers.insert(std::make_pair(filePath, xbtfReader));
  return result.first;
}

std::string CXbtManager::NormalizePath(const CURL& path)
{
  if (path.IsProtocol("xbt"))
    return path.GetHostName();

  return path.Get();
}

}
