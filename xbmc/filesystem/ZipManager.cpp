/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ZipManager.h"

#include <algorithm>
#include <utility>

#include "File.h"
#include "URL.h"
#if defined(TARGET_POSIX)
#include "PlatformDefs.h"
#endif
#include "utils/CharsetConverter.h"
#include "utils/EndianSwap.h"
#include "utils/log.h"
#include "utils/RegExp.h"
#include "utils/URIUtils.h"

using namespace XFILE;

static const size_t ZC_FLAG_EFS = 1 << 11; // general purpose bit 11 - zip holds utf-8 filenames

CZipManager::CZipManager() = default;

CZipManager::~CZipManager() = default;

bool CZipManager::GetZipList(const CURL& url, std::vector<SZipEntry>& items)
{
  struct __stat64 m_StatData = {};

  std::string strFile = url.GetHostName();

  if (CFile::Stat(strFile,&m_StatData))
  {
    CLog::Log(LOGDEBUG, "CZipManager::GetZipList: failed to stat file {}", url.GetRedacted());
    return false;
  }

  std::map<std::string, std::vector<SZipEntry> >::iterator it = mZipMap.find(strFile);
  if (it != mZipMap.end()) // already listed, just return it if not changed, else release and reread
  {
    std::map<std::string,int64_t>::iterator it2=mZipDate.find(strFile);

    if (m_StatData.st_mtime == it2->second)
    {
      items = it->second;
      return true;
    }
    mZipMap.erase(it);
    mZipDate.erase(it2);
  }

  CFile mFile;
  if (!mFile.Open(strFile))
  {
    CLog::Log(LOGDEBUG, "ZipManager: unable to open file {}!", strFile);
    return false;
  }

  unsigned int hdr;
  if (mFile.Read(&hdr, 4)!=4 || (Endian_SwapLE32(hdr) != ZIP_LOCAL_HEADER &&
                                 Endian_SwapLE32(hdr) != ZIP_DATA_RECORD_HEADER &&
                                 Endian_SwapLE32(hdr) != ZIP_SPLIT_ARCHIVE_HEADER))
  {
    CLog::Log(LOGDEBUG,"ZipManager: not a zip file!");
    mFile.Close();
    return false;
  }

  if (Endian_SwapLE32(hdr) == ZIP_SPLIT_ARCHIVE_HEADER)
    CLog::LogF(LOGWARNING, "ZIP split archive header found. Trying to process as a single archive..");

  // push date for update detection
  mZipDate.insert(make_pair(strFile,m_StatData.st_mtime));


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
    CLog::Log(LOGERROR, "ZipManager: Invalid zip file length: {}", fileSize);
    return false;
  }
  int searchSize = (int) std::min(static_cast<int64_t>(65557), fileSize-ECDREC_SIZE+1);
  int blockSize = (int) std::min(1024, searchSize);
  int nbBlock = searchSize / blockSize;
  int extraBlockSize = searchSize % blockSize;
  // Signature is on 4 bytes
  // It could be between 2 blocks, so we need to read 3 extra bytes
  std::vector<char> buffer(blockSize + 3);
  bool found = false;

  // Loop through blocks starting at the end of the file (minus ECDREC_SIZE-1)
  for (int nb=1; !found && (nb <= nbBlock); nb++)
  {
    mFile.Seek(fileSize-ECDREC_SIZE+1-(blockSize*nb),SEEK_SET);
    if (mFile.Read(buffer.data(), blockSize + 3) != blockSize + 3)
      return false;
    for (int i=blockSize-1; !found && (i >= 0); i--)
    {
      if (Endian_SwapLE32(ReadUnaligned<uint32_t>(buffer.data() + i)) == ZIP_END_CENTRAL_HEADER)
      {
        // Set current position to start of end of central directory
        mFile.Seek(fileSize-ECDREC_SIZE+1-(blockSize*nb)+i,SEEK_SET);
        found = true;
      }
    }
  }

  // If not found, look in the last block left...
  if ( !found && (extraBlockSize > 0) )
  {
    mFile.Seek(fileSize-ECDREC_SIZE+1-searchSize,SEEK_SET);
    if (mFile.Read(buffer.data(), extraBlockSize + 3) != extraBlockSize + 3)
      return false;
    for (int i=extraBlockSize-1; !found && (i >= 0); i--)
    {
      if (Endian_SwapLE32(ReadUnaligned<uint32_t>(buffer.data() + i)) == ZIP_END_CENTRAL_HEADER)
      {
        // Set current position to start of end of central directory
        mFile.Seek(fileSize-ECDREC_SIZE+1-searchSize+i,SEEK_SET);
        found = true;
      }
    }
  }

  buffer.clear();

  if ( !found )
  {
    CLog::Log(LOGDEBUG, "ZipManager: broken file {}!", strFile);
    mFile.Close();
    return false;
  }

  unsigned int cdirOffset, cdirSize;
  // Get size of the central directory
  mFile.Seek(12,SEEK_CUR);
  if (mFile.Read(&cdirSize, 4) != 4)
    return false;
  cdirSize = Endian_SwapLE32(cdirSize);
  // Get Offset of start of central directory with respect to the starting disk number
  if (mFile.Read(&cdirOffset, 4) != 4)
    return false;
  cdirOffset = Endian_SwapLE32(cdirOffset);

  // Go to the start of central directory
  mFile.Seek(cdirOffset,SEEK_SET);

  CRegExp pathTraversal;
  pathTraversal.RegComp(PATH_TRAVERSAL);

  char temp[CHDR_SIZE];
  while (mFile.GetPosition() < cdirOffset + cdirSize)
  {
    SZipEntry ze;
    if (mFile.Read(temp, CHDR_SIZE) != CHDR_SIZE)
      return false;
    readCHeader(temp, ze);
    if (ze.header != ZIP_CENTRAL_HEADER)
    {
      CLog::Log(LOGDEBUG, "ZipManager: broken file {}!", strFile);
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
    strncpy(ze.name, strName.c_str(), strName.size() > 254 ? 254 : strName.size());

    // Jump after central file header extra field and file comment
    mFile.Seek(ze.eclength + ze.clength,SEEK_CUR);

    if (pathTraversal.RegFind(strName) < 0)
      items.push_back(ze);
  }

  /* go through list and figure out file header lengths */
  for (auto& ze : items)
  {
    // Go to the local file header to get the extra field length
    // !! local header extra field length != central file header extra field length !!
    mFile.Seek(ze.lhdrOffset+28,SEEK_SET);
    if (mFile.Read(&(ze.elength), 2) != 2)
      return false;
    ze.elength = Endian_SwapLE16(ze.elength);

    // Compressed data offset = local header offset + size of local header + filename length + local file header extra field length
    ze.offset = ze.lhdrOffset + LHDR_SIZE + ze.flength + ze.elength;

  }

  mZipMap.insert(make_pair(strFile,items));
  mFile.Close();
  return true;
}

bool CZipManager::GetZipEntry(const CURL& url, SZipEntry& item)
{
  const std::string& strFile = url.GetHostName();

  std::map<std::string, std::vector<SZipEntry> >::iterator it = mZipMap.find(strFile);
  std::vector<SZipEntry> items;
  if (it == mZipMap.end()) // we need to list the zip
  {
    GetZipList(url,items);
  }
  else
  {
    items = it->second;
  }

  const std::string& strFileName = url.GetFileName();
  for (const auto& it2 : items)
  {
    if (std::string(it2.name) == strFileName)
    {
      item = it2;
      return true;
    }
  }
  return false;
}

bool CZipManager::ExtractArchive(const std::string& strArchive, const std::string& strPath)
{
  const CURL pathToUrl(strArchive);
  return ExtractArchive(pathToUrl, strPath);
}

bool CZipManager::ExtractArchive(const CURL& archive, const std::string& strPath)
{
  std::vector<SZipEntry> entry;
  CURL url = URIUtils::CreateArchivePath("zip", archive);
  GetZipList(url, entry);
  for (const auto& it : entry)
  {
    if (it.name[strlen(it.name) - 1] == '/') // skip dirs
      continue;
    std::string strFilePath(it.name);

    CURL zipPath = URIUtils::CreateArchivePath("zip", archive, strFilePath);
    const CURL pathToUrl(strPath + strFilePath);
    if (!CFile::Copy(zipPath, pathToUrl))
      return false;
  }
  return true;
}

// Read local file header
void CZipManager::readHeader(const char* buffer, SZipEntry& info)
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
void CZipManager::readCHeader(const char* buffer, SZipEntry& info)
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

void CZipManager::release(const std::string& strPath)
{
  CURL url(strPath);
  std::map<std::string, std::vector<SZipEntry> >::iterator it= mZipMap.find(url.GetHostName());
  if (it != mZipMap.end())
  {
    std::map<std::string,int64_t>::iterator it2=mZipDate.find(url.GetHostName());
    mZipMap.erase(it);
    mZipDate.erase(it2);
  }
}


