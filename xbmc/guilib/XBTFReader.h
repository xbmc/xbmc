/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
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
#include "utils/StdString.h"
#include "XBTF.h"

class CXBTFReader
{
public:
  CXBTFReader();
  bool IsOpen() const;
  bool Open(const CStdString& fileName);
  void Close();
  time_t GetLastModificationTimestamp();
  bool Exists(const CStdString& name);
  CXBTFFile* Find(const CStdString& name);
  bool Load(const CXBTFFrame& frame, unsigned char* buffer);
  std::vector<CXBTFFile>&  GetFiles();

private:
  CXBTF      m_xbtf;
  CStdString m_fileName;
  FILE*      m_file;
  std::map<CStdString, CXBTFFile> m_filesMap;
};

#endif
