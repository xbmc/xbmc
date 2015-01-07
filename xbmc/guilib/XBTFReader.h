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

#ifndef XBTFREADER_H_
#define XBTFREADER_H_

#include <vector>
#include <map>
#include <string>
#include "XBTF.h"

class CXBTFReader
{
public:
  CXBTFReader();
  bool IsOpen() const;
  bool Open(const std::string& fileName);
  void Close();
  time_t GetLastModificationTimestamp();
  bool Exists(const std::string& name);
  CXBTFFile* Find(const std::string& name);
  bool Load(const CXBTFFrame& frame, unsigned char* buffer);
  std::vector<CXBTFFile>&  GetFiles();

private:
  CXBTF      m_xbtf;
  std::string m_fileName;
  FILE*      m_file;
  std::map<std::string, CXBTFFile> m_filesMap;
};

#endif
