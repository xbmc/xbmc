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
#ifndef XBTF_H_
#define XBTF_H_

#include <string>
#include <vector>
#include <stdint.h>

#define XBTF_MAGIC "XBTF"
#define XBTF_VERSION "2"

#define XB_FMT_MASK   0xffff ///< mask for format info - other flags are outside this
#define XB_FMT_DXT_MASK   15
#define XB_FMT_UNKNOWN     0
#define XB_FMT_DXT1        1
#define XB_FMT_DXT3        2
#define XB_FMT_DXT5        4
#define XB_FMT_DXT5_YCoCg  8
#define XB_FMT_A8R8G8B8   16 // texture.xbt byte order (matches BGRA8)
#define XB_FMT_A8         32
#define XB_FMT_RGBA8      64
#define XB_FMT_RGB8      128
#define XB_FMT_OPAQUE  65536

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
  char* GetPath();
  void SetPath(const std::string& path);
  uint32_t GetLoop() const;
  void SetLoop(uint32_t loop);
  std::vector<CXBTFFrame>& GetFrames();
  uint64_t GetHeaderSize() const;

private:
  char         m_path[256];
  uint32_t     m_loop;
  std::vector<CXBTFFrame> m_frames;
};

class CXBTF
{
public:
  CXBTF();
  uint64_t GetHeaderSize() const;
  std::vector<CXBTFFile>& GetFiles();

private:
  std::vector<CXBTFFile> m_files;
};

#endif
