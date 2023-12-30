/*
 *  Copyright (C) 2015-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "guilib/XBTFReader.h"
#include "threads/CriticalSection.h"

#include <map>
#include <memory>
#include <string>
#include <vector>

class CURL;
class CXBTFFile;

namespace XFILE
{
class CXbtManager
{
public:
  ~CXbtManager();

  static CXbtManager& GetInstance();

  bool HasFiles(const CURL& path) const;
  bool GetFiles(const CURL& path, std::vector<CXBTFFile>& files) const;

  bool GetReader(const CURL& path, CXBTFReaderPtr& reader) const;

  void Release(const CURL& path);

private:
  CXbtManager();
  CXbtManager(const CXbtManager&) = delete;
  CXbtManager& operator=(const CXbtManager&) = delete;

  struct XBTFReader
  {
    CXBTFReaderPtr reader;
    time_t lastModification;
  };
  using XBTFReaders = std::map<std::string, XBTFReader>;

  XBTFReaders::iterator GetReader(const CURL& path) const;
  XBTFReaders::iterator GetReader(const std::string& path) const;
  void RemoveReader(XBTFReaders::iterator readerIterator) const;
  XBTFReaders::const_iterator ProcessFile(const CURL& path) const;

  static std::string NormalizePath(const CURL& path);

  mutable XBTFReaders m_readers;
  mutable CCriticalSection m_lock;
};
}
