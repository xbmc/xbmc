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

#include <vector>

#include "IFile.h"
#include "URL.h"
#include "guilib/XBTF.h"
#include "guilib/XBTFReader.h"

namespace XFILE
{
class CXbtFile : public IFile
{
public:
  CXbtFile();
  ~CXbtFile() override;

  bool Open(const CURL& url) override;
  void Close() override;
  bool Exists(const CURL& url) override;

  int64_t GetPosition() override;
  int64_t GetLength() override;

  int Stat(struct __stat64* buffer) override;
  int Stat(const CURL& url, struct __stat64* buffer) override;

  ssize_t Read(void* lpBuf, size_t uiBufSize) override;
  int64_t Seek(int64_t iFilePosition, int iWhence = SEEK_SET) override;

  uint32_t GetImageWidth() const;
  uint32_t GetImageHeight() const;
  uint32_t GetImageFormat() const;
  bool HasImageAlpha() const;

private:
  bool GetFirstFrame(CXBTFFrame& frame) const;

  static bool GetReader(const CURL& url, CXBTFReaderPtr& reader);
  static bool GetReaderAndFile(const CURL& url, CXBTFReaderPtr& reader, CXBTFFile& file);
  static bool GetFile(const CURL& url, CXBTFFile& file);

  CURL m_url;
  bool m_open;
  CXBTFReaderPtr m_xbtfReader;
  CXBTFFile m_xbtfFile;

  std::vector<uint64_t> m_frameStartPositions;
  size_t m_frameIndex;
  uint64_t m_positionWithinFrame;
  int64_t m_positionTotal;

  std::vector<uint8_t*> m_unpackedFrames;
};
}
