#ifndef ZIP_MANAGER_H_
#define ZIP_MANAGER_H_

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

// See http://www.pkware.com/documents/casestudies/APPNOTE.TXT
#define ZIP_LOCAL_HEADER 0x04034b50
#define ZIP_DATA_RECORD_HEADER 0x08074b50
#define ZIP_CENTRAL_HEADER 0x02014b50
#define ZIP_END_CENTRAL_HEADER 0x06054b50
#define ZIP_SPLIT_ARCHIVE_HEADER 0x30304b50
#define LHDR_SIZE 30
#define CHDR_SIZE 46
#define ECDREC_SIZE 22

#include <memory.h>
#include <string>
#include <vector>
#include <map>

class CURL;

struct SZipEntry {
  unsigned int header;
  unsigned short version;
  unsigned short flags;
  unsigned short method;
  unsigned short mod_time;
  unsigned short mod_date;
  unsigned int crc32;
  unsigned int csize; // compressed size
  unsigned int usize; // uncompressed size
  unsigned short flength; // filename length
  unsigned short elength; // extra field length (local file header)
  unsigned short eclength; // extra field length (central file header)
  unsigned short clength; // file comment length (central file header)
  unsigned int lhdrOffset; // Relative offset of local header
  int64_t offset;         // offset in file to compressed data
  char name[255];

  SZipEntry()
  {
    header = 0;
    version = 0;
    flags = 0;
    method = 0;
    mod_time = 0;
    mod_date = 0;
    crc32 = 0;
    csize = 0;
    usize = 0;
    flength = 0;
    elength = 0;
    eclength = 0;
    clength = 0;
    lhdrOffset = 0;
    offset = 0;
    name[0] = '\0';
  }

  SZipEntry(const SZipEntry& SNewItem)
  {
    memcpy(&header,&SNewItem.header,sizeof(unsigned int));
    memcpy(&version,&SNewItem.version,sizeof(unsigned short));
    memcpy(&flags,&SNewItem.flags,sizeof(unsigned short));
    memcpy(&method,&SNewItem.method,sizeof(unsigned short));
    memcpy(&mod_time,&SNewItem.mod_time,sizeof(unsigned short));
    memcpy(&mod_date,&SNewItem.mod_date,sizeof(unsigned short));
    memcpy(&crc32,&SNewItem.crc32,sizeof(unsigned int));
    memcpy(&csize,&SNewItem.csize,sizeof(unsigned int));
    memcpy(&usize,&SNewItem.usize,sizeof(unsigned int));
    memcpy(&flength,&SNewItem.flength,sizeof(unsigned short));
    memcpy(&elength,&SNewItem.elength,sizeof(unsigned short));
    memcpy(&eclength,&SNewItem.eclength,sizeof(unsigned short));
    memcpy(&clength,&SNewItem.clength,sizeof(unsigned short));
    memcpy(&lhdrOffset,&SNewItem.lhdrOffset,sizeof(unsigned int));
    memcpy(&offset,&SNewItem.offset,sizeof(int64_t));
    memcpy(name,SNewItem.name,255*sizeof(char));
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
private:
  std::map<std::string,std::vector<SZipEntry> > mZipMap;
  std::map<std::string,int64_t> mZipDate;
};

extern CZipManager g_ZipManager;

#endif
