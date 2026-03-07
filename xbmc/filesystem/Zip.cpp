/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Zip.h"

#include "File.h"
#include "URL.h"
#if defined(TARGET_POSIX)
#include "PlatformDefs.h"
#endif
#include "utils/CharsetConverter.h"
#include "utils/EndianSwap.h"
#include "utils/RegExp.h"
#include "utils/log.h"

#include <algorithm>
#include <array>
#include <cstdint>

using namespace XFILE;

static constexpr size_t ZC_FLAG_EFS = 1 << 11; // general purpose bit 11 - zip holds utf-8 filenames

namespace
{
template<typename T>
T ReadUnaligned(const void* mem)
{
  T var;
  std::memcpy(&var, mem, sizeof(T));
  return var;
}

bool ReadZip64EOCD(CFile& file, uint64_t& cdirOffset, uint64_t& cdirSize)
{
  // At this point, file position is just after reading 32-bit cdirOffset in EOCD.
  const int64_t curPos = file.GetPosition();
  const int64_t eocdStart = curPos - 20; // 12 (skipped) + 4 (size) + 4 (offset) = 20
  const int64_t locatorPos = eocdStart - 20; // ZIP64 locator is 20 bytes just before EOCD

  if (locatorPos < 0)
    return false;

  uint32_t sig = 0;
  if (file.Seek(locatorPos, SEEK_SET) == -1)
    return false;
  if (file.Read(&sig, 4) != 4)
    return false;
  sig = Endian_SwapLE32(sig);
  if (sig != ZIP64_END_CENTRAL_LOCATOR)
    return false;

  uint64_t zip64EocdOffset = 0;

  if (file.Seek(4, SEEK_CUR) == -1) // skip diskNumber
    return false;
  if (file.Read(&zip64EocdOffset, 8) != 8)
    return false;
  if (file.Seek(4, SEEK_CUR) == -1) // skip totalDiscs
    return false;

  zip64EocdOffset = Endian_SwapLE64(zip64EocdOffset);

  // Seek to ZIP64 EOCD record
  if (file.Seek(static_cast<int64_t>(zip64EocdOffset), SEEK_SET) == -1)
    return false;
  if (file.Read(&sig, 4) != 4)
    return false;
  sig = Endian_SwapLE32(sig);
  if (sig != ZIP64_END_CENTRAL_HEADER)
    return false;

  // Skip size of ZIP64 EOCD record (8 bytes)
  if (file.Seek(8, SEEK_CUR) == -1)
    return false;

  // Skip: version made by (2), version needed (2), disk number (4), CD start disk (4),
  // number of entries on this disk (8), total entries (8) = 28 bytes
  if (file.Seek(28, SEEK_CUR) == -1)
    return false;

  uint64_t cdirSize64 = 0;
  uint64_t cdirOffset64 = 0;
  if (file.Read(&cdirSize64, 8) != 8)
    return false;
  if (file.Read(&cdirOffset64, 8) != 8)
    return false;

  cdirSize64 = Endian_SwapLE64(cdirSize64);
  cdirOffset64 = Endian_SwapLE64(cdirOffset64);

  cdirSize = cdirSize64;
  cdirOffset = cdirOffset64;

  return true;
}
} // anonymous namespace

bool Zip::ParseZipCentralDirectory(const std::string& zipPath, std::vector<SZipEntry>& entries)
{
  CFile mFile;
  if (!mFile.Open(zipPath))
  {
    CLog::LogF(LOGERROR, "Unable to open file {}!", zipPath);
    return false;
  }

  unsigned int hdr;
  if (mFile.Read(&hdr, 4) != 4 ||
      (Endian_SwapLE32(hdr) != ZIP_LOCAL_HEADER && Endian_SwapLE32(hdr) != ZIP_DATA_RECORD_HEADER &&
       Endian_SwapLE32(hdr) != ZIP_SPLIT_ARCHIVE_HEADER))
  {
    CLog::LogF(LOGERROR, "Not a zip file!");
    mFile.Close();
    return false;
  }

  if (Endian_SwapLE32(hdr) == ZIP_SPLIT_ARCHIVE_HEADER)
    CLog::LogF(LOGWARNING,
               "ZIP split archive header found. Trying to process as a single archive..");

  const bool Is64{Zip::IsZip64(mFile)};

  // Look for end of central directory record
  // Zipfile comment may be up to 65535 bytes
  // End of central directory record is 22 bytes (ECDREC_SIZE)
  // -> need to check the last 65557 bytes
  int64_t fileSize = mFile.GetLength();
  // Don't need to look in the last 18 bytes (ECDREC_SIZE-4)
  // But as we need to do overlapping between blocks (3 bytes),
  // we start the search at ECDREC_SIZE-1 from the end of file
  if (fileSize < ECDREC_SIZE - 1)
  {
    CLog::LogF(LOGERROR, "Invalid zip file length: {}", fileSize);
    return false;
  }
  int searchSize =
      static_cast<int>(std::min(static_cast<int64_t>(65557), fileSize - ECDREC_SIZE + 1));
  int blockSize = std::min(1024, searchSize);
  int nbBlock = searchSize / blockSize;
  int extraBlockSize = searchSize % blockSize;
  // Signature is on 4 bytes
  // It could be between 2 blocks, so we need to read 3 extra bytes
  std::vector<char> buffer(blockSize + 3);
  bool found = false;

  // Loop through blocks starting at the end of the file (minus ECDREC_SIZE-1)
  for (int nb = 1; !found && (nb <= nbBlock); nb++)
  {
    if (mFile.Seek(fileSize - ECDREC_SIZE + 1 - (static_cast<int64_t>(blockSize) * nb), SEEK_SET) ==
        -1)
      return false;
    if (mFile.Read(buffer.data(), blockSize + 3) != blockSize + 3)
      return false;
    for (int i = blockSize - 1; !found && (i >= 0); i--)
    {
      if (Endian_SwapLE32(ReadUnaligned<uint32_t>(buffer.data() + i)) == ZIP_END_CENTRAL_HEADER)
      {
        // Set current position to start of end of central directory
        if (mFile.Seek(fileSize - ECDREC_SIZE + 1 - (static_cast<int64_t>(blockSize) * nb) + i,
                       SEEK_SET) == -1)
          return false;
        found = true;
      }
    }
  }

  // If not found, look in the last block left...
  if (!found && (extraBlockSize > 0))
  {
    if (mFile.Seek(fileSize - ECDREC_SIZE + 1 - searchSize, SEEK_SET) == -1)
      return false;
    if (mFile.Read(buffer.data(), extraBlockSize + 3) != extraBlockSize + 3)
      return false;
    for (int i = extraBlockSize - 1; !found && (i >= 0); i--)
    {
      if (Endian_SwapLE32(ReadUnaligned<uint32_t>(buffer.data() + i)) == ZIP_END_CENTRAL_HEADER)
      {
        // Set current position to start of end of central directory
        if (mFile.Seek(fileSize - ECDREC_SIZE + 1 - searchSize + i, SEEK_SET) == -1)
          return false;
        found = true;
      }
    }
  }

  buffer.clear();

  if (!found)
  {
    CLog::LogF(LOGERROR, "Broken file {}!", zipPath);
    mFile.Close();
    return false;
  }

  unsigned int cdirOffset;
  unsigned int cdirSize;
  // Get size of the central directory
  if (mFile.Seek(12, SEEK_CUR) == -1)
    return false;
  if (mFile.Read(&cdirSize, 4) != 4)
    return false;
  cdirSize = Endian_SwapLE32(cdirSize);
  // Get Offset of start of central directory with respect to the starting disk number
  if (mFile.Read(&cdirOffset, 4) != 4)
    return false;
  cdirOffset = Endian_SwapLE32(cdirOffset);

  // Handle ZIP64 if needed
  uint64_t cdirOffset64 = cdirOffset;
  uint64_t cdirSize64 = cdirSize;
  if (Is64 && !ReadZip64EOCD(mFile, cdirOffset64, cdirSize64))
  {
    CLog::LogF(LOGERROR, "ZIP64 EOCD invalid in {}", zipPath);
    return false;
  }

  // Go to the start of central directory
  if (mFile.Seek(static_cast<int64_t>(cdirOffset64), SEEK_SET) == -1)
    return false;

  CRegExp pathTraversal;
  pathTraversal.RegComp(PATH_TRAVERSAL);

  char temp[CHDR_SIZE];
  while (mFile.GetPosition() < static_cast<int64_t>(cdirOffset64 + cdirSize64))
  {
    SZipEntry ze;
    if (mFile.Read(temp, CHDR_SIZE) != CHDR_SIZE)
      return false;
    Zip::ReadCentralHeader(temp, ze);
    if (ze.header != ZIP_CENTRAL_HEADER)
    {
      CLog::LogF(LOGERROR, "Broken file {}!", zipPath);
      mFile.Close();
      return false;
    }

    // Get the filename just after the central file header
    std::vector<char> bufName(ze.flength);
    if (mFile.Read(bufName.data(), ze.flength) != ze.flength)
      return false;
    std::string strName(bufName.data(), bufName.size());
    bufName.clear();
    if ((ze.flags & ZC_FLAG_EFS) == 0)
    {
      std::string tmp(strName);
      g_charsetConverter.ToUtf8("CP437", tmp, strName);
    }
    memset(ze.name, 0, 255);
    const size_t copyLen = std::min(strName.size(), size_t{254});
    std::memcpy(ze.name, strName.data(), copyLen);
    ze.name[copyLen] = '\0';

    // Read central extra field so we can parse ZIP64 extra
    std::vector<char> extraField;
    if (ze.eclength > 0)
    {
      extraField.resize(ze.eclength);
      if (mFile.Read(extraField.data(), ze.eclength) != ze.eclength)
        return false;

      // If any 32-bit fields are maxed, pull real values from ZIP64 extra
      if (Is64 &&
          (ze.csize == 0xFFFFFFFFu || ze.usize == 0xFFFFFFFFu || ze.lhdrOffset == 0xFFFFFFFFu))
        Zip::ParseZip64ExtraField(extraField.data(), ze.eclength, ze);
    }

    // Jump after central file header extra field and file comment
    if (mFile.Seek(ze.clength, SEEK_CUR) == -1)
      return false;

    if (pathTraversal.RegFind(strName) < 0)
      entries.push_back(ze);
  }

  /* go through list and figure out file header lengths */
  for (auto& ze : entries)
  {
    // Go to the local file header to get the extra field length
    // !! local header extra field length != central file header extra field length !!
    const int64_t localFilenameLengthPos = static_cast<int64_t>(ze.lhdrOffset) + 26;
    if (localFilenameLengthPos + 2 > mFile.GetLength())
      return false;

    if (mFile.Seek(localFilenameLengthPos, SEEK_SET) == -1)
      return false;
    uint16_t flength;
    if (mFile.Read(&flength, 2) != 2)
      return false;
    flength = Endian_SwapLE16(flength);
    if (mFile.Read(&ze.elength, 2) != 2)
      return false;
    ze.elength = Endian_SwapLE16(ze.elength);

    mFile.Seek(flength, SEEK_CUR); // Skip filename

    std::vector<char> localExtra(ze.elength);
    if (ze.elength && mFile.Read(localExtra.data(), ze.elength) != ze.elength)
      return false;

    if (Is64 &&
        (ze.csize == 0xFFFFFFFFu || ze.usize == 0xFFFFFFFFu || ze.lhdrOffset == 0xFFFFFFFFu) &&
        !localExtra.empty())
      Zip::ParseZip64ExtraField(localExtra.data(), ze.elength, ze);

    // Compressed data offset = local header offset + size of local header + filename length + local file header extra field length
    ze.offset = static_cast<int64_t>(ze.lhdrOffset) + LHDR_SIZE + ze.flength + ze.elength;
  }

  mFile.Close();
  return true;
}

// Read local file header
void Zip::ReadLocalHeader(const char* buffer, SZipEntry& info)
{
  info.header = Endian_SwapLE32(ReadUnaligned<uint32_t>(buffer));
  info.version = Endian_SwapLE16(ReadUnaligned<uint16_t>(buffer + 4));
  info.flags = Endian_SwapLE16(ReadUnaligned<uint16_t>(buffer + 6));
  info.method = Endian_SwapLE16(ReadUnaligned<uint16_t>(buffer + 8));
  info.mod_time = Endian_SwapLE16(ReadUnaligned<uint16_t>(buffer + 10));
  info.mod_date = Endian_SwapLE16(ReadUnaligned<uint16_t>(buffer + 12));
  info.crc32 = Endian_SwapLE32(ReadUnaligned<uint32_t>(buffer + 14));
  info.csize = Endian_SwapLE32(ReadUnaligned<uint32_t>(buffer + 18));
  info.usize = Endian_SwapLE32(ReadUnaligned<uint32_t>(buffer + 22));
  info.flength = Endian_SwapLE16(ReadUnaligned<uint16_t>(buffer + 26));
  info.elength = Endian_SwapLE16(ReadUnaligned<uint16_t>(buffer + 28));
}

// Read central file header (from central directory)
void Zip::ReadCentralHeader(const char* buffer, SZipEntry& info)
{
  info.header = Endian_SwapLE32(ReadUnaligned<uint32_t>(buffer));
  // Skip version made by
  info.version = Endian_SwapLE16(ReadUnaligned<uint16_t>(buffer + 6));
  info.flags = Endian_SwapLE16(ReadUnaligned<uint16_t>(buffer + 8));
  info.method = Endian_SwapLE16(ReadUnaligned<uint16_t>(buffer + 10));
  info.mod_time = Endian_SwapLE16(ReadUnaligned<uint16_t>(buffer + 12));
  info.mod_date = Endian_SwapLE16(ReadUnaligned<uint16_t>(buffer + 14));
  info.crc32 = Endian_SwapLE32(ReadUnaligned<uint32_t>(buffer + 16));
  info.csize = Endian_SwapLE32(ReadUnaligned<uint32_t>(buffer + 20));
  info.usize = Endian_SwapLE32(ReadUnaligned<uint32_t>(buffer + 24));
  info.flength = Endian_SwapLE16(ReadUnaligned<uint16_t>(buffer + 28));
  info.eclength = Endian_SwapLE16(ReadUnaligned<uint16_t>(buffer + 30));
  info.clength = Endian_SwapLE16(ReadUnaligned<uint16_t>(buffer + 32));
  // Skip disk number start, internal/external file attributes
  info.lhdrOffset = Endian_SwapLE32(ReadUnaligned<uint32_t>(buffer + 42));
}

bool Zip::IsZip64(CFile& file)
{
  const int64_t size{std::min<int64_t>(file.GetLength(), 1024)};
  if (file.Seek(-size, SEEK_END) == -1)
    return false;

  std::vector<char> buffer(size);
  if (file.Read(buffer.data(), size) != size)
    return false;

  constexpr std::array<char, 4> centralHeaderSignature{
      {static_cast<char>(ZIP64_END_CENTRAL_HEADER & 0xFF),
       static_cast<char>((ZIP64_END_CENTRAL_HEADER >> 8) & 0xFF),
       static_cast<char>((ZIP64_END_CENTRAL_HEADER >> 16) & 0xFF),
       static_cast<char>((ZIP64_END_CENTRAL_HEADER >> 24) & 0xFF)}};

  return !std::ranges::search(buffer.begin(), buffer.end(), centralHeaderSignature.begin(),
                              centralHeaderSignature.end())
              .empty();
}

void Zip::ParseZip64ExtraField(const char* buf, uint16_t length, SZipEntry& info)
{
  uint16_t offset = 0;
  while (offset + 4 <= length)
  {
    uint16_t headerId = Endian_SwapLE16(ReadUnaligned<uint16_t>(buf + offset));
    uint16_t dataSize = Endian_SwapLE16(ReadUnaligned<uint16_t>(buf + offset + 2));
    offset += 4;

    if (offset + dataSize > length)
      break;

    if (headerId == 0x0001) // ZIP64 extended information extra field
    {
      const char* p = buf + offset;
      uint16_t remaining = dataSize;

      // Order: [usize(8)?][csize(8)?][lhdrOffset(8)?][diskStart(4)?]
      if (info.usize == 0xFFFFFFFFu && remaining >= 8)
      {
        const uint64_t usize64 = Endian_SwapLE64(ReadUnaligned<uint64_t>(p));
        info.usize = usize64;
        p += 8;
        remaining -= 8;
      }
      if (info.csize == 0xFFFFFFFFu && remaining >= 8)
      {
        const uint64_t csize64 = Endian_SwapLE64(ReadUnaligned<uint64_t>(p));
        info.csize = csize64;
        p += 8;
        remaining -= 8;
      }
      if (info.lhdrOffset == 0xFFFFFFFFu && remaining >= 8)
      {
        const uint64_t hdrOffset64 = Endian_SwapLE64(ReadUnaligned<uint64_t>(p));
        info.lhdrOffset = hdrOffset64;
      }
      // diskStart (4) is ignored here if present
      break; // we don't need other extra fields
    }

    offset += dataSize;
  }
}
