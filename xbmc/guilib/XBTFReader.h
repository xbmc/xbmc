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

#include <stdint.h>

class CXBTFFile;
class CXBTFFrame;

class CXBTFReader
{
public:
  CXBTFReader();
  ~CXBTFReader();

  bool Open(const std::string& path);
  bool IsOpen() const;
  void Close();

  time_t GetLastModificationTimestamp() const;
  uint64_t GetHeaderSize() const;

  bool Exists(const std::string& name) const;
  bool Get(const std::string& name, CXBTFFile& file) const;
  std::vector<CXBTFFile> GetFiles() const;
  void AddFile(const CXBTFFile& file);

  bool Load(const CXBTFFrame& frame, unsigned char* buffer) const;

private:
  std::string m_path;
  FILE* m_file;
  std::map<std::string, CXBTFFile> m_files;
};

typedef std::shared_ptr<CXBTFReader> CXBTFReaderPtr;
