/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <cstdint>
#include <cstring>
#include <map>
#include <string>
#include <vector>

// See http://www.pkware.com/documents/casestudies/APPNOTE.TXT
static constexpr uint32_t ZIP_LOCAL_HEADER = 0x04034b50;
static constexpr uint32_t ZIP_DATA_RECORD_HEADER = 0x08074b50;
static constexpr uint32_t ZIP_CENTRAL_HEADER = 0x02014b50;
static constexpr uint32_t ZIP_END_CENTRAL_HEADER = 0x06054b50;
static constexpr uint32_t ZIP_SPLIT_ARCHIVE_HEADER = 0x30304b50;

// ZIP64 signatures
static constexpr uint32_t ZIP64_END_CENTRAL_HEADER = 0x06064b50;
static constexpr uint32_t ZIP64_END_CENTRAL_LOCATOR = 0x07064b50;

static constexpr int LHDR_SIZE = 30;
static constexpr int DREC_SIZE = 16;
static constexpr int CHDR_SIZE = 46;
static constexpr int ECDREC_SIZE = 22;

class CURL;
namespace XFILE
{
class CFile;
}

inline const std::string PATH_TRAVERSAL(R"_((^|\/|\\)\.{2}($|\/|\\))_");

struct SZipEntry {
  unsigned int header = 0;
  unsigned short version = 0;
  unsigned short flags = 0;
  unsigned short method = 0;
  unsigned short mod_time = 0;
  unsigned short mod_date = 0;
  unsigned int crc32 = 0;
  uint64_t csize = 0; // compressed size
  uint64_t usize = 0; // uncompressed size
  unsigned short flength = 0; // filename length
  unsigned short elength = 0; // extra field length (local file header)
  unsigned short eclength = 0; // extra field length (central file header)
  unsigned short clength = 0; // file comment length (central file header)
  uint64_t lhdrOffset = 0; // Relative offset of local header
  int64_t offset = 0;         // offset in file to compressed data
  char name[255];

  SZipEntry()
  {
    name[0] = '\0';
  }
};

class CZipManager
{
public:
  CZipManager();
  ~CZipManager();

  bool GetZipList(const CURL& url, std::vector<SZipEntry>& items);
  bool GetZipEntry(const CURL& url, SZipEntry& item);
  bool ExtractArchive(const std::string& strArchive, const std::string& strPath);
  bool ExtractArchive(const CURL& archive, const std::string& strPath);
  void release(const std::string& strPath); // release resources used by list zip
  static void readHeader(const char* buffer, SZipEntry& info);
  static void readCHeader(const char* buffer, SZipEntry& info);

  static bool IsZip64(XFILE::CFile& file);
  static void ParseZip64ExtraField(const char* buf, uint16_t length, SZipEntry& info);

private:
  std::map<std::string,std::vector<SZipEntry> > mZipMap;
  std::map<std::string, int64_t> mZipDate;

  static bool ReadZip64EOCD(XFILE::CFile& file, uint64_t& cdirOffset, uint64_t& cdirSize);

  template<typename T>
  static T ReadUnaligned(const void* mem)
  {
    T var;
    std::memcpy(&var, mem, sizeof(T));
    return var;
  }
};

extern CZipManager g_ZipManager;
