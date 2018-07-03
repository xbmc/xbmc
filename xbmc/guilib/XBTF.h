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

#include <ctime>
#include <map>
#include <string>
#include <vector>

#include <stdint.h>

static const std::string XBTF_MAGIC = "XBTF";
static const std::string XBTF_VERSION = "2";

#include "TextureFormats.h"

class CXBTFFrame
{
public:
  CXBTFFrame();

  uint32_t GetWidth() const;
  void SetWidth(uint32_t width);

  uint32_t GetFormat(bool raw = false) const;
  void SetFormat(uint32_t format);

  uint32_t GetHeight() const;
  void SetHeight(uint32_t height);

  uint64_t GetUnpackedSize() const;
  void SetUnpackedSize(uint64_t size);

  uint64_t GetPackedSize() const;
  void SetPackedSize(uint64_t size);

  uint64_t GetOffset() const;
  void SetOffset(uint64_t offset);

  uint64_t GetHeaderSize() const;

  uint32_t GetDuration() const;
  void SetDuration(uint32_t duration);

  bool IsPacked() const;
  bool HasAlpha() const;

private:
  uint32_t m_width;
  uint32_t m_height;
  uint32_t m_format;
  uint64_t m_packedSize;
  uint64_t m_unpackedSize;
  uint64_t m_offset;
  uint32_t m_duration;
};

class CXBTFFile
{
public:
  CXBTFFile();
  CXBTFFile(const CXBTFFile& ref);

  const std::string& GetPath() const;
  void SetPath(const std::string& path);

  uint32_t GetLoop() const;
  void SetLoop(uint32_t loop);

  const std::vector<CXBTFFrame>& GetFrames() const;
  std::vector<CXBTFFrame>& GetFrames();

  uint64_t GetPackedSize() const;
  uint64_t GetUnpackedSize() const;
  uint64_t GetHeaderSize() const;

  static const size_t MaximumPathLength = 256;

private:
  std::string m_path;
  uint32_t m_loop = 0;
  std::vector<CXBTFFrame> m_frames;
};

class CXBTFBase
{
public:
  virtual ~CXBTFBase() = default;

  uint64_t GetHeaderSize() const;

  bool Exists(const std::string& name) const;
  bool Get(const std::string& name, CXBTFFile& file) const;
  std::vector<CXBTFFile> GetFiles() const;
  void AddFile(const CXBTFFile& file);
  void UpdateFile(const CXBTFFile& file);

protected:
  CXBTFBase() = default;

  std::map<std::string, CXBTFFile> m_files;
};
