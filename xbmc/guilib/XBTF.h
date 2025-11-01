/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <ctime>
#include <map>
#include <string>
#include <vector>

#include <stdint.h>

inline const std::string XBTF_MAGIC = "XBTF";
inline const std::string XBTF_VERSION = "3";
static const char XBTF_VERSION_MIN = '2';

#include "TextureFormats.h"

class CXBTFFrame
{
public:
  CXBTFFrame();

  uint32_t GetWidth() const;
  void SetWidth(uint32_t width);

  XB_FMT GetFormat(bool raw = false) const;
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

  KD_TEX_FMT GetKDFormat() const;
  KD_TEX_FMT GetKDFormatType() const;
  KD_TEX_ALPHA GetKDAlpha() const;
  KD_TEX_SWIZ GetKDSwizzle() const;

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

  const std::string& GetPath() const;
  void SetPath(const std::string& path);

  const std::string& GetRealPath() const { return m_realPath; }
  void SetRealPath(const std::string& path) { m_realPath = path; }

  unsigned int GetSubstitutionPriority() const { return m_substitutionPriority; }
  void SetSubstitutionPriority(unsigned int substitutionPriority)
  {
    m_substitutionPriority = substitutionPriority;
  }

  uint32_t GetLoop() const;
  void SetLoop(uint32_t loop);

  const std::vector<CXBTFFrame>& GetFrames() const;
  std::vector<CXBTFFrame>& GetFrames();

  uint64_t GetPackedSize() const;
  uint64_t GetUnpackedSize() const;
  uint64_t GetHeaderSize() const;

  static const size_t MaximumPathLength = 256;

private:
  std::string m_path; // path used in skin, e.g. icon.png
  std::string m_realPath; // actual file path, might be icon.png.astc.ktx
  unsigned int m_substitutionPriority{0};
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
