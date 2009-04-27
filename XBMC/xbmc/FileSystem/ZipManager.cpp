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

#include "stdafx.h"
#include "ZipManager.h"
#include "Util.h"
#include "URL.h"
#include "FileSystem/File.h"

// All values are stored in little-endian byte order in .zip file
// Use SDL macros to perform byte swapping on big-endian systems
// This assumes that big-endian systems use SDL
// Macros do not do anything on little-endian systems
// SDL_endian.h is already included in PlatformDefs.h
#ifndef HAS_SDL
#define SDL_SwapLE16(X) (X)
#define SDL_SwapLE32(X) (X)
#endif

#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif

using namespace XFILE;
using namespace std;

CZipManager g_ZipManager;

CZipManager::CZipManager()
{
}

CZipManager::~CZipManager()
{

}

bool CZipManager::HasMultipleEntries(const CStdString& strPath)
{
  // no comments ;D
  CFile mFile;
  if (mFile.Open(strPath.c_str()))
  {
    char buffer[23];
    for (int i=22;i<1024;++i)
    {
      mFile.Seek(mFile.GetLength()-i,SEEK_SET);
      mFile.Read(buffer,4);
      if (SDL_SwapLE32(*((unsigned int*)buffer)) == ZIP_END_CENTRAL_HEADER)
      {
        mFile.Seek(6,SEEK_CUR);
        short iEntries;
        mFile.Read(&iEntries,2);
        iEntries = SDL_SwapLE16(iEntries);
        mFile.Close();
        return iEntries > 1;
      }
    }
    mFile.Close();
  }

  return true;
}

bool CZipManager::GetZipList(const CStdString& strPath, vector<SZipEntry>& items)
{
  CURL url(strPath);
  struct __stat64 m_StatData;

  CStdString strFile = url.GetHostName();

  map<CStdString,vector<SZipEntry> >::iterator it = mZipMap.find(strFile);
  if (it != mZipMap.end()) // already listed, just return it if not changed, else release and reread
  {
    map<CStdString,__int64>::iterator it2=mZipDate.find(strFile);
    if (CFile::Stat(strFile,&m_StatData))
#ifndef _LINUX
      CLog::Log(LOGDEBUG,"statdata: %i, new: %i",it2->second,m_StatData.st_mtime);
#else
      CLog::Log(LOGDEBUG,"statdata: %lld new: %lu",it2->second,m_StatData.st_mtime);
#endif
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

  SZipEntry ze;
  char hdr[4];
  mFile.Read(hdr,4);
  if( SDL_SwapLE32(*((unsigned int*)hdr)) != ZIP_LOCAL_HEADER )
  {
    CLog::Log(LOGDEBUG,"ZipManager: not a zip file!");
    mFile.Close();
    return false;
  }
  // push date for update detection
  CFile::Stat(strFile,&m_StatData);
  mZipDate.insert(make_pair(strFile,m_StatData.st_mtime));

  
  // Look for end of central directory record
  // Zipfile comment may be up to 65535 bytes
  // End of central directory record is 22 bytes (ECDREC_SIZE)
  // -> need to check the last 65557 bytes
  __int64 fileSize = mFile.GetLength();
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
      if ( SDL_SwapLE32(*((unsigned int*)(buffer+i))) == ZIP_END_CENTRAL_HEADER )
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
      if ( SDL_SwapLE32(*((unsigned int*)(buffer+i))) == ZIP_END_CENTRAL_HEADER )
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
  cdirSize = SDL_SwapLE32(cdirSize);  
  // Get Offset of start of central directory with respect to the starting disk number
  mFile.Read(&cdirOffset,4);
  cdirOffset = SDL_SwapLE32(cdirOffset);

  // Go to the start of central directory
  mFile.Seek(cdirOffset,SEEK_SET);

  char temp[CHDR_SIZE];
  while (mFile.GetPosition() < cdirOffset + cdirSize)
  {
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
    
    // Save the current position
    __int64 savePos = mFile.GetPosition();
    
    // Go to the local file header to get the extra field length
    // !! local header extra field length != central file header extra field length !!
    mFile.Seek(ze.lhdrOffset+28,SEEK_SET);
    mFile.Read(&(ze.elength),2);
    ze.elength = SDL_SwapLE16(ze.elength);
    
    // Compressed data offset = local header offset + size of local header + filename length + local file header extra field length
    ze.offset = ze.lhdrOffset + LHDR_SIZE + ze.flength + ze.elength;
        
    // Jump after central file header extra field and file comment
    mFile.Seek(savePos + ze.eclength + ze.clength,SEEK_SET);
	  
    items.push_back(ze);
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
  CUtil::CreateArchivePath(strZipPath, "zip", strArchive, "");
  GetZipList(strZipPath,entry);
  for (vector<SZipEntry>::iterator it=entry.begin();it != entry.end();++it)
  {
    if (it->name[strlen(it->name)-1] == '/') // skip dirs
      continue;
    CStdString strFilePath(it->name);


    CUtil::CreateArchivePath(strZipPath, "zip", strArchive, strFilePath);
    if (!CFile::Cache(strZipPath.c_str(),(strPath+strFilePath).c_str()))
      return false;
  }
  return true;
}

void CZipManager::CleanUp(const CStdString& strArchive, const CStdString& strPath)
{
  vector<SZipEntry> entry;
  CStdString strZipPath;
  CUtil::CreateArchivePath(strZipPath, "zip", strArchive, "");

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
  info.header = SDL_SwapLE32(*(unsigned int*)buffer);
  info.version = SDL_SwapLE16(*(unsigned short*)(buffer+4));
  info.flags = SDL_SwapLE16(*(unsigned short*)(buffer+6));
  info.method = SDL_SwapLE16(*(unsigned short*)(buffer+8));
  info.mod_time = SDL_SwapLE16(*(unsigned short*)(buffer+10));
  info.mod_date = SDL_SwapLE16(*(unsigned short*)(buffer+12));
  info.crc32 = SDL_SwapLE32(*(unsigned int*)(buffer+14));
  info.csize = SDL_SwapLE32(*(unsigned int*)(buffer+18));
  info.usize = SDL_SwapLE32(*(unsigned int*)(buffer+22));
  info.flength = SDL_SwapLE16(*(unsigned short*)(buffer+26));
  info.elength = SDL_SwapLE16(*(unsigned short*)(buffer+28));
}

// Read central file header (from central directory)
void CZipManager::readCHeader(const char* buffer, SZipEntry& info)
{
  info.header = SDL_SwapLE32(*(unsigned int*)buffer);
  // Skip version made by
  info.version = SDL_SwapLE16(*(unsigned short*)(buffer+6));
  info.flags = SDL_SwapLE16(*(unsigned short*)(buffer+8));
  info.method = SDL_SwapLE16(*(unsigned short*)(buffer+10));
  info.mod_time = SDL_SwapLE16(*(unsigned short*)(buffer+12));
  info.mod_date = SDL_SwapLE16(*(unsigned short*)(buffer+14));
  info.crc32 = SDL_SwapLE32(*(unsigned int*)(buffer+16));
  info.csize = SDL_SwapLE32(*(unsigned int*)(buffer+20));
  info.usize = SDL_SwapLE32(*(unsigned int*)(buffer+24));
  info.flength = SDL_SwapLE16(*(unsigned short*)(buffer+28));
  info.eclength = SDL_SwapLE16(*(unsigned short*)(buffer+30));
  info.clength = SDL_SwapLE16(*(unsigned short*)(buffer+32));
  // Skip disk number start, internal/external file attributes
  info.lhdrOffset = SDL_SwapLE32(*(unsigned int*)(buffer+42));
  
}

void CZipManager::release(const CStdString& strPath)
{
  CURL url(strPath);
  map<CStdString,vector<SZipEntry> >::iterator it= mZipMap.find(url.GetHostName());
  if (it != mZipMap.end())
  {
    map<CStdString,__int64>::iterator it2=mZipDate.find(url.GetHostName());
    mZipMap.erase(it);
    mZipDate.erase(it2);
  }
}


