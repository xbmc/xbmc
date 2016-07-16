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

#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "XBTFReader.h"
#include "guilib/XBTF.h"
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

CXBTFReader::CXBTFReader()
  : CXBTFBase(),
    m_path(),
    m_file(nullptr)
{ }

CXBTFReader::~CXBTFReader()
{
  Close();
}

bool CXBTFReader::Open(const std::string& path)
{
  if (path.empty())
    return false;

  m_path = path;

#ifdef TARGET_WINDOWS
  std::wstring strPathW;
  g_charsetConverter.utf8ToW(CSpecialProtocol::TranslatePath(m_path), strPathW, false);
  m_file = _wfopen(strPathW.c_str(), L"rb");
#else
  m_file = fopen(m_path.c_str(), "rb");
#endif
  if (m_file == nullptr)
    return false;

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

    AddFile(xbtfFile);
  }

  // Sanity check
  uint64_t pos = static_cast<uint64_t>(ftell(m_file));
  if (pos != GetHeaderSize())
    return false;

  return true;
}

bool CXBTFReader::IsOpen() const
{
  return m_file != nullptr;
}

void CXBTFReader::Close()
{
  if (m_file != nullptr)
  {
    fclose(m_file);
    m_file = nullptr;
  }

  m_path.clear();
  m_files.clear();
}

time_t CXBTFReader::GetLastModificationTimestamp() const
{
  if (m_file == nullptr)
    return 0;

  struct stat fileStat;
  if (fstat(fileno(m_file), &fileStat) == -1)
    return 0;

  return fileStat.st_mtime;
}

bool CXBTFReader::Load(const CXBTFFrame& frame, unsigned char* buffer) const
{
  if (m_file == nullptr)
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
