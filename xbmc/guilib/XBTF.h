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

static const std::string XBTF_MAGIC = "XBTF";
static const std::string XBTF_VERSION = "2";

#include "TextureFormats.h"

class CXBTFFrame
{
public:
  CXBTFFrame();

  uint32_t GetWidth() const;
  void SetWidth(uint32_t width);

  XB_FMT GetFormat(bool raw = false) const;
  void SetFormat(XB_FMT format);

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
  XB_FMT m_format;
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
