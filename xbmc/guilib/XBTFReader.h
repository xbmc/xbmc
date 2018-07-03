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

#include <memory>
#include <string>
#include <vector>

#include <stdint.h>

#include "XBTF.h"

class CXBTFReader : public CXBTFBase
{
public:
  CXBTFReader();
  ~CXBTFReader() override;

  bool Open(const std::string& path);
  bool IsOpen() const;
  void Close();

  time_t GetLastModificationTimestamp() const;

  bool Load(const CXBTFFrame& frame, unsigned char* buffer) const;

private:
  std::string m_path;
  FILE* m_file = nullptr;
};

typedef std::shared_ptr<CXBTFReader> CXBTFReaderPtr;
