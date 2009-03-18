#ifndef ZIP_MANAGER_H_
#define ZIP_MANAGER_H_

#define ZIP_LOCAL_HEADER 0x04034b50
#define ZIP_CENTRAL_HEADER 0x02014b50
/*
 *      Copyright (C) 2005-2008 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */


#include  "StdString.h"

#include <memory.h>
#include <vector>
#include <map>

struct SZipEntry { // sizeof(zipentry) == 30 + flength + elength
  unsigned int header;
  unsigned short version;
  unsigned short flags;
  unsigned short method;
  unsigned short mod_time;
  unsigned short mod_date;
  int crc32;
  unsigned int csize; // compressed size
  unsigned int usize; // uncompressed size
  unsigned short flength; // filename length
  unsigned short elength; // length of extra field
  __int64 offset;         // offset in file to compressed data
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
    memcpy(&crc32,&SNewItem.crc32,sizeof(int));
    memcpy(&csize,&SNewItem.csize,sizeof(unsigned int));
    memcpy(&usize,&SNewItem.usize,sizeof(unsigned int));
    memcpy(&flength,&SNewItem.flength,sizeof(unsigned short));
    memcpy(&elength,&SNewItem.elength,sizeof(unsigned short));
    memcpy(&offset,&SNewItem.offset,sizeof(__int64));
    memcpy(name,SNewItem.name,255*sizeof(char));
  }
};

class CZipManager
{
public:
  CZipManager();
  ~CZipManager();

  bool GetZipList(const CStdString& strPath, std::vector<SZipEntry>& items);
  bool HasMultipleEntries(const CStdString& strPath);
  bool GetZipEntry(const CStdString& strPath, SZipEntry& item);
  bool ExtractArchive(const CStdString& strArchive, const CStdString& strPath);
  void CleanUp(const CStdString& strArchive, const CStdString& strPath); // deletes extracted archive. use with care!
  void release(const CStdString& strPath); // release resources used by list zip
  static void readHeader(const char* buffer, SZipEntry& info);
private:
  std::map<CStdString,std::vector<SZipEntry> > mZipMap;
  std::map<CStdString,__int64> mZipDate;
};

extern CZipManager g_ZipManager;

#endif
