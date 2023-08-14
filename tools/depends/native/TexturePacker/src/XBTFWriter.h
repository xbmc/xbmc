/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://kodi.tv
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

#pragma once

#include "guilib/XBTF.h"

#include <cstdio>
#include <string>
#include <vector>

class CXBTFWriter : public CXBTFBase
{
public:
  CXBTFWriter(const std::string& outputFile);
  ~CXBTFWriter() override;

  bool Create();
  bool Close();
  bool AppendContent(unsigned char const* data, size_t length);
  bool UpdateHeader(const std::vector<unsigned int>& dupes);

private:
  void Cleanup();

  std::string m_outputFile;
  FILE* m_file = nullptr;
  std::vector<uint8_t> m_data;
};

