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

#include "XBTFReader.h"

#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "URL.h"
#include "utils/EndianSwap.h"

#ifdef TARGET_WINDOWS
#include "filesystem/SpecialProtocol.h"
#include "utils/CharsetConverter.h"
#include "platform/win32/PlatformDefs.h"
#endif

static bool ReadString(FILE* file, char* str, size_t max_length)
{
  if (file == nullptr || str == nullptr || max_length <= 0)
    return false;

  return (fread(str, max_length, 1, file) == 1);
}

static bool ReadUInt32(FILE* file, uint32_t& value)
{
  if (file == nullptr)
    return false;

  if (fread(&value, sizeof(uint32_t), 1, file) != 1)
    return false;

  value = Endian_SwapLE32(value);
  return true;
}

static bool ReadUInt64(FILE* file, uint64_t& value)
{
  if (file == nullptr)
    return false;

  if (fread(&value, sizeof(uint64_t), 1, file) != 1)
    return false;

  value = Endian_SwapLE64(value);
  return true;
}

bool CXBTFReader::Open(const CURL& url)
{
  m_path = url.IsProtocol("xbt") ? url.GetHostName() : url.Get();

  if (m_path.empty())
    return false;

#ifdef TARGET_WINDOWS
  std::wstring strPathW;
  g_charsetConverter.utf8ToW(CSpecialProtocol::TranslatePath(m_path), strPathW, false);
  m_file.attach(_wfopen(strPathW.c_str(), L"rb"));
#else
  m_file.attach(fopen(m_path.c_str(), "rb"));
#endif
  if (!m_file)
    return false;

  if (!HasFiles())
    return Init();

  return true;
}

void CXBTFReader::Close()
{
  m_file.reset();
}

bool CXBTFReader::Load(const CXBTFFrame& frame, unsigned char* buffer) const
{
  if (!m_file)
    return false;

#if defined(TARGET_DARWIN) || defined(TARGET_FREEBSD) || defined(TARGET_ANDROID)
  if (fseeko(m_file, static_cast<off_t>(frame.GetOffset()), SEEK_SET) == -1)
#else
  if (fseeko64(m_file, static_cast<off_t>(frame.GetOffset()), SEEK_SET) == -1)
#endif
    return false;

  if (fread(buffer, 1, static_cast<size_t>(frame.GetPackedSize()), m_file) != frame.GetPackedSize())
    return false;

  return true;
}

bool CXBTFReader::Init()
{
  // read the magic word
  char magic[4];
  if (!ReadString(m_file, magic, sizeof(magic)))
    return false;

  if (strncmp(XBTF_MAGIC.c_str(), magic, sizeof(magic)) != 0)
    return false;

  // read the version
  char version[1];
  if (!ReadString(m_file, version, sizeof(version)))
    return false;

  if (strncmp(XBTF_VERSION.c_str(), version, sizeof(version)) != 0)
    return false;

  unsigned int nofFiles;
  if (!ReadUInt32(m_file, nofFiles))
    return false;

  for (uint32_t i = 0; i < nofFiles; i++)
  {
    CXBTFFile xbtfFile;
    uint32_t u32;
    uint64_t u64;

    char path[CXBTFFile::MaximumPathLength];
    memset(path, 0, sizeof(path));
    if (!ReadString(m_file, path, sizeof(path)))
      return false;
    xbtfFile.SetPath(path);

    if (!ReadUInt32(m_file, u32))
      return false;
    xbtfFile.SetLoop(u32);

    unsigned int nofFrames;
    if (!ReadUInt32(m_file, nofFrames))
      return false;

    for (uint32_t j = 0; j < nofFrames; j++)
    {
      CXBTFFrame frame;

      if (!ReadUInt32(m_file, u32))
        return false;
      frame.SetWidth(u32);

      if (!ReadUInt32(m_file, u32))
        return false;
      frame.SetHeight(u32);

      if (!ReadUInt32(m_file, u32))
        return false;
      frame.SetFormat(u32);

      if (!ReadUInt64(m_file, u64))
        return false;
      frame.SetPackedSize(u64);

      if (!ReadUInt64(m_file, u64))
        return false;
      frame.SetUnpackedSize(u64);

      if (!ReadUInt32(m_file, u32))
        return false;
      frame.SetDuration(u32);

      if (!ReadUInt64(m_file, u64))
        return false;
      frame.SetOffset(u64);

      xbtfFile.GetFrames().push_back(frame);
    }

    m_files.insert(std::make_pair(xbtfFile.GetPath(), xbtfFile));
  }

  // Sanity check
  uint64_t pos = static_cast<uint64_t>(ftell(m_file));
  if (pos != GetHeaderSize())
    return false;

  return true;
}

uint64_t CXBTFReader::GetHeaderSize() const
{
  uint64_t result = XBTF_MAGIC.size() + XBTF_VERSION.size() +
    sizeof(uint32_t) /* number of files */;

  for (const auto& file : m_files)
    result += file.second.GetHeaderSize();

  return result;
}

bool CXBTFReader::Exists(const std::string& name) const
{
  CXBTFFile dummy;
  return Get(name, dummy);
}

bool CXBTFReader::Get(const std::string& name, CXBTFFile& file) const
{
  const auto& iter = m_files.find(name);
  if (iter == m_files.end())
    return false;

  file = iter->second;
  return true;
}

std::vector<CXBTFFile> CXBTFReader::GetFiles() const
{
  std::vector<CXBTFFile> files;
  files.reserve(m_files.size());

  for (const auto& file : m_files)
    files.push_back(file.second);

  return files;
}

size_t CXBTFReader::GetFileCount() const
{
  return m_files.size();
}
