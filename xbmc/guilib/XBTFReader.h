#pragma once
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

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "guilib/XBTF.h"
#include "utils/ScopeGuard.h"

class CURL;

class CXBTFReader
{
public:
  bool Open(const CURL& url);
  void Close();

  bool HasFiles() const { return !m_files.empty(); }
  bool Exists(const std::string& name) const;
  bool Get(const std::string& name, CXBTFFile& file) const;
  std::vector<CXBTFFile> GetFiles() const;
  size_t GetFileCount() const;

  bool Load(const CXBTFFrame& frame, unsigned char* buffer) const;

private:
  bool Init();
  uint64_t GetHeaderSize() const;

  std::string m_path;
  std::map<std::string, CXBTFFile> m_files;

  using FileGuard = KODI::UTILS::CScopeGuard<FILE*, nullptr, decltype(fclose)>;
  FileGuard m_file{fclose};
};

typedef std::shared_ptr<CXBTFReader> CXBTFReaderPtr;
