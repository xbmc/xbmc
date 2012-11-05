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

#include "system.h"
#include "ZipManager.h"
#include "URL.h"
#include "File.h"
#include "utils/CharsetConverter.h"
#include "utils/log.h"
#include "utils/EndianSwap.h"
#include "utils/URIUtils.h"
#include "SpecialProtocol.h"


#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif

using namespace XFILE;
using namespace std;

CZipManager::CZipManager()
{
}

CZipManager::~CZipManager()
{

}

bool CZipManager::GetZipList(const CStdString& strPath, vector<SZipEntry>& items)
{
  CLog::Log(LOGDEBUG, "%s - Processing %s", __FUNCTION__, strPath.c_str());

  CURL url(strPath);
  struct __stat64 m_StatData = {};

  CStdString strFile = url.GetHostName();

  if (CFile::Stat(strFile,&m_StatData))
  {
    CLog::Log(LOGDEBUG,"CZipManager::GetZipList: failed to stat file %s", strPath.c_str());
    return false;
  }

  map<CStdString,vector<SZipEntry> >::iterator it = mZipMap.find(strFile);
  if (it != mZipMap.end()) // already listed, just return it if not changed, else release and reread
  {
    map<CStdString,int64_t>::iterator it2=mZipDate.find(strFile);
    CLog::Log(LOGDEBUG,"statdata: %"PRId64" new: %"PRIu64, it2->second, (uint64_t)m_StatData.st_mtime);

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
    CLog::Log(LOGDEBUG,"ZipManager: unable to open file %s!",strFile.c_str());
    return false;
  }

  unsigned int hdr;
  mFile.Read(&hdr, 4);
  if( Endian_SwapLE32(hdr) != ZIP_LOCAL_HEADER )
  {
    CLog::Log(LOGDEBUG,"ZipManager: not a zip file!");
    mFile.Close();
    return false;
  }
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
  int searchSize = (int) min(65557, fileSize-ECDREC_SIZE+1);
  int blockSize = (int) min(1024, searchSize);
  int nbBlock = searchSize / blockSize;
  int extraBlockSize = searchSize % blockSize;
  // Signature is on 4 bytes
  // It could be between 2 blocks, so we need to read 3 extra bytes
  char *buffer = new char[blockSize+3];
  bool found = false;

  // Loop through blocks starting at the end of the file (minus ECDREC_SIZE-1)
  for (int nb=1; !found && (nb <= nbBlock); nb++)
  {
    mFile.Seek(fileSize-ECDREC_SIZE+1-(blockSize*nb),SEEK_SET);
    mFile.Read(buffer,blockSize+3);
    for (int i=blockSize-1; !found && (i >= 0); i--)
    {
      if ( Endian_SwapLE32(*((unsigned int*)(buffer+i))) == ZIP_END_CENTRAL_HEADER )
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
    mFile.Read(buffer,extraBlockSize+3);
    for (int i=extraBlockSize-1; !found && (i >= 0); i--)
    {
      if ( Endian_SwapLE32(*((unsigned int*)(buffer+i))) == ZIP_END_CENTRAL_HEADER )
      {
        // Set current position to start of end of central directory
        mFile.Seek(fileSize-ECDREC_SIZE+1-searchSize+i,SEEK_SET);
        found = true;
      }
    }
  }

  delete [] buffer;

  if ( !found )
  {
    CLog::Log(LOGDEBUG,"ZipManager: broken file %s!",strFile.c_str());
    mFile.Close();
    return false;
  }

  unsigned int cdirOffset, cdirSize;
  // Get size of the central directory
  mFile.Seek(12,SEEK_CUR);
  mFile.Read(&cdirSize,4);
  cdirSize = Endian_SwapLE32(cdirSize);
  // Get Offset of start of central directory with respect to the starting disk number
  mFile.Read(&cdirOffset,4);
  cdirOffset = Endian_SwapLE32(cdirOffset);

  // Go to the start of central directory
  mFile.Seek(cdirOffset,SEEK_SET);

  char temp[CHDR_SIZE];
  while (mFile.GetPosition() < cdirOffset + cdirSize)
  {
    SZipEntry ze;
    mFile.Read(temp,CHDR_SIZE);
    readCHeader(temp, ze);
    if (ze.header != ZIP_CENTRAL_HEADER)
    {
      CLog::Log(LOGDEBUG,"ZipManager: broken file %s!",strFile.c_str());
      mFile.Close();
      return false;
    }

    // Get the filename just after the central file header
    CStdString strName;
    mFile.Read(strName.GetBuffer(ze.flength), ze.flength);
    strName.ReleaseBuffer();
    g_charsetConverter.unknownToUTF8(strName);
    ZeroMemory(ze.name, 255);
    strncpy(ze.name, strName.c_str(), strName.size()>254 ? 254 : strName.size());

    // Jump after central file header extra field and file comment
    mFile.Seek(ze.eclength + ze.clength,SEEK_CUR);

    items.push_back(ze);
  }

  /* go through list and figure out file header lengths */
  for(vector<SZipEntry>::iterator it = items.begin(); it != items.end(); ++it)
  {
    SZipEntry& ze = *it;
    // Go to the local file header to get the extra field length
    // !! local header extra field length != central file header extra field length !!
    mFile.Seek(ze.lhdrOffset+28,SEEK_SET);
    mFile.Read(&(ze.elength),2);
    ze.elength = Endian_SwapLE16(ze.elength);

    // Compressed data offset = local header offset + size of local header + filename length + local file header extra field length
    ze.offset = ze.lhdrOffset + LHDR_SIZE + ze.flength + ze.elength;

  }

  mZipMap.insert(make_pair(strFile,items));
  mFile.Close();
  return true;
}

bool CZipManager::GetZipEntry(const CStdString& strPath, SZipEntry& item)
{
  CURL url(strPath);

  CStdString strFile = url.GetHostName();

  map<CStdString,vector<SZipEntry> >::iterator it = mZipMap.find(strFile);
  vector<SZipEntry> items;
  if (it == mZipMap.end()) // we need to list the zip
  {
    GetZipList(strPath,items);
  }
  else
  {
    items = it->second;
  }

  CStdString strFileName = url.GetFileName();
  for (vector<SZipEntry>::iterator it2=items.begin();it2 != items.end();++it2)
  {
    if (CStdString(it2->name) == strFileName)
    {
      memcpy(&item,&(*it2),sizeof(SZipEntry));
      return true;
    }
  }
  return false;
}

bool CZipManager::ExtractArchive(const CStdString& strArchive, const CStdString& strPath)
{
  vector<SZipEntry> entry;
  CStdString strZipPath;
  URIUtils::CreateArchivePath(strZipPath, "zip", strArchive, "");
  GetZipList(strZipPath,entry);
  for (vector<SZipEntry>::iterator it=entry.begin();it != entry.end();++it)
  {
    if (it->name[strlen(it->name)-1] == '/') // skip dirs
      continue;
    CStdString strFilePath(it->name);


    URIUtils::CreateArchivePath(strZipPath, "zip", strArchive, strFilePath);
    if (!CFile::Cache(strZipPath.c_str(),(strPath+strFilePath).c_str()))
      return false;
  }
  return true;
}

void CZipManager::CleanUp(const CStdString& strArchive, const CStdString& strPath)
{
  vector<SZipEntry> entry;
  CStdString strZipPath;
  URIUtils::CreateArchivePath(strZipPath, "zip", strArchive, "");

  GetZipList(strZipPath,entry);
  for (vector<SZipEntry>::iterator it=entry.begin();it != entry.end();++it)
  {
    if (it->name[strlen(it->name)-1] == '/') // skip dirs
      continue;
    CStdString strFilePath(it->name);
    CLog::Log(LOGDEBUG,"delete file: %s",(strPath+strFilePath).c_str());
    CFile::Delete((strPath+strFilePath).c_str());
  }
}

// Read local file header
void CZipManager::readHeader(const char* buffer, SZipEntry& info)
{
  info.header = Endian_SwapLE32(*(unsigned int*)buffer);
  info.version = Endian_SwapLE16(*(unsigned short*)(buffer+4));
  info.flags = Endian_SwapLE16(*(unsigned short*)(buffer+6));
  info.method = Endian_SwapLE16(*(unsigned short*)(buffer+8));
  info.mod_time = Endian_SwapLE16(*(unsigned short*)(buffer+10));
  info.mod_date = Endian_SwapLE16(*(unsigned short*)(buffer+12));
  info.crc32 = Endian_SwapLE32(*(unsigned int*)(buffer+14));
  info.csize = Endian_SwapLE32(*(unsigned int*)(buffer+18));
  info.usize = Endian_SwapLE32(*(unsigned int*)(buffer+22));
  info.flength = Endian_SwapLE16(*(unsigned short*)(buffer+26));
  info.elength = Endian_SwapLE16(*(unsigned short*)(buffer+28));
}

// Read central file header (from central directory)
void CZipManager::readCHeader(const char* buffer, SZipEntry& info)
{
  info.header = Endian_SwapLE32(*(unsigned int*)buffer);
  // Skip version made by
  info.version = Endian_SwapLE16(*(unsigned short*)(buffer+6));
  info.flags = Endian_SwapLE16(*(unsigned short*)(buffer+8));
  info.method = Endian_SwapLE16(*(unsigned short*)(buffer+10));
  info.mod_time = Endian_SwapLE16(*(unsigned short*)(buffer+12));
  info.mod_date = Endian_SwapLE16(*(unsigned short*)(buffer+14));
  info.crc32 = Endian_SwapLE32(*(unsigned int*)(buffer+16));
  info.csize = Endian_SwapLE32(*(unsigned int*)(buffer+20));
  info.usize = Endian_SwapLE32(*(unsigned int*)(buffer+24));
  info.flength = Endian_SwapLE16(*(unsigned short*)(buffer+28));
  info.eclength = Endian_SwapLE16(*(unsigned short*)(buffer+30));
  info.clength = Endian_SwapLE16(*(unsigned short*)(buffer+32));
  // Skip disk number start, internal/external file attributes
  info.lhdrOffset = Endian_SwapLE32(*(unsigned int*)(buffer+42));

}

void CZipManager::release(const CStdString& strPath)
{
  CURL url(strPath);
  map<CStdString,vector<SZipEntry> >::iterator it= mZipMap.find(url.GetHostName());
  if (it != mZipMap.end())
  {
    map<CStdString,int64_t>::iterator it2=mZipDate.find(url.GetHostName());
    mZipMap.erase(it);
    mZipDate.erase(it2);
  }
}


