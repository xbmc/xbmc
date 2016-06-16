#pragma once
/*
 *      Copyright (C) 2015 Team XBMC
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

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "guilib/XBTFReader.h"

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
  CXbtManager(const CXbtManager&);
  CXbtManager& operator=(const CXbtManager&);

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
};
}
