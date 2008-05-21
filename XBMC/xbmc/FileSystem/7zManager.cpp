/*
* XBMC
* 7z Filesystem
* Copyright (c) 2008 topfs2
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "stdafx.h"
#include "7zManager.h"
#include "../Util.h"
#include "../utils/SingleLock.h"
#include "../FileItem.h"

#include <set>

using namespace std;

// Write callback for the temp file
bool Write(void *object, unsigned char *buffer, int size)
{
  XFILE::CFile *file = (XFILE::CFile *)object;

  file->Seek(file->GetLength(), SEEK_SET);
  int i = 0;
  i = file->Write(buffer, size);
  if (i != size)
    CLog::Log(LOGERROR, "7z: Write failure");
  file->Flush();
}

// Read callback for the temp file
Byte Read(void *object, size_t position)
{
  XFILE::CFile *file = (XFILE::CFile *)object;
  if (file->Seek(position, SEEK_SET) == -1)
    CLog::Log(LOGERROR, "7z: Can't seek temp out file");

  Byte t[1];
  file->Read(t, 1);

  return t[0];
}

CFile7zExtractThread::CFile7zExtractThread(CStdString path, CStdString pathInArchive, Byte **Buff, size_t *BuffSize)
{
  str7zPath    = path;
  strIn7z      = pathInArchive;
  m_NowPos     = 0;
  m_Ready      = false;

  m_TempOut    = NULL;
}

CFile7zExtractThread::~CFile7zExtractThread()
{
  if (m_TempOut)
    delete m_TempOut;
}

void CFile7zExtractThread::Run()
{
  m_Error = false;
  CFileInStream archiveStream;
  CArchiveDatabaseEx db;
  SZ_RESULT res;
  ISzAlloc allocImp;
  ISzAlloc allocTempImp;

  archiveStream.File = new XFILE::CFile();
  if (!archiveStream.File->Open(str7zPath, READ_BUFFERED))
  {
    delete archiveStream.File;
    m_Error = true;
    return;
  }

  archiveStream.InStream.Read = C7zManager::FileRead;
  archiveStream.InStream.Seek = C7zManager::FileSeek;

  allocImp.Alloc = SzAlloc;
  allocImp.Free = SzFree;

  allocTempImp.Alloc = SzAllocTemp;
  allocTempImp.Free = SzFreeTemp;

  CrcGenerateTable();

  SzArDbExInit(&db);
  res = SzArchiveOpen(&archiveStream.InStream, &db, &allocImp, &allocTempImp);
  if (res == SZ_OK)
  {
    UInt32 i = -1;
    for (unsigned int j = 0; j < db.Database.NumFiles; j++)
    {
      C7zFileItem *f = db.Database.Files + j;
      if (strIn7z.Equals(f->Name))
      {
        i = j; 
        break;
      }
    }

    if (i == -1) //This can't really happen but good to be safe.
      return;

    m_blockIndex = 0xFFFFFFFF; /* it can have any value before first call (if m_Buffer = 0) */

    m_Buffer = 0;
    m_BufferSize =0;

    C7zFileItem *f = db.Database.Files + i;    if (f->IsDirectory)
    {
      CLog::Log(LOGERROR, "7z: We Can't unpack a dir");
      return;
    }
    m_NowPos     = 0;
    m_Ready = true;
    m_outFile = new XFILE::CFile();
    m_inFile  = new XFILE::CFile();
    m_TempOut = new XFILE::CFile(); //Is this needed?
    CStdString tempfile;
    tempfile.Format("z:\\%s", f->Name);
    CLog::Log(LOGNOTICE, "7z: Going to write temp to %s", tempfile.c_str());
    if (!m_outFile->OpenForWrite(tempfile.c_str(), true, true))
    {
      CLog::Log(LOGERROR, "7z: Can't open temp out file for write");
      delete m_outFile;
      delete m_inFile;
      return;
    }
    if (!m_inFile->Open(tempfile.c_str()))
    {
      CLog::Log(LOGERROR, "7z: Can't open temp out file for read");
      delete m_outFile;
      delete m_inFile;
      return;
    }
    if (!m_TempOut->Open(tempfile.c_str()))
    {
      CLog::Log(LOGERROR, "7z: Can't open temp out file for read");
      printf("Can't open outputfile2\n");
      delete m_outFile;
      delete m_inFile;
      return;
    }


    res = SzExtract(&archiveStream.InStream, &db, i, 
                    &m_blockIndex, &m_Buffer, &m_BufferSize, 
                    &m_offset, &m_outSizeProcessed, 
                    &allocImp, &allocTempImp, &m_NowPos,
                    Write, m_outFile, 1024, Read, m_inFile);
    printf("Extractat klart\n");

    if (res != SZ_OK)
    {
      m_Error = true;
      CLog::Log(LOGERROR, "7z: Couldn't extract #%i", res);
      return;
    }
    allocImp.Free(m_Buffer);
  }
  else
  {
    m_Error = true;
    res = SZE_FAIL;
  }
  SzArDbExFree(&db, allocImp.Free);


  C7zManager::CloseArchiveFile(archiveStream.File);
}


C7zManager g_7zManager;

size_t C7zManager::ReadArchiveFile(XFILE::CFile *file, void *data, size_t size)
{
  if (size == 0)
    return 0;
  return file->Read(data, size);
}

/*size_t C7zManager::Writer(XFILE::CFile *file, void *data, size_t size)
{
  if (size == 0)
    return 0;

  return file->Write(data, size);
}*/

int C7zManager::CloseArchiveFile(XFILE::CFile *file)
{
  file->Close();
  return 0;
}

#ifdef _LZMA_IN_CB

#define kBufferSize (1 << 12)
Byte g_Buffer[kBufferSize];
// Read callback for the archive file 
SZ_RESULT C7zManager::FileRead(void *object, void **buffer, size_t maxRequiredSize, size_t *processedSize)
{
  CFileInStream *s = (CFileInStream *)object;
  size_t processedSizeLoc;
  if (maxRequiredSize > kBufferSize)
    maxRequiredSize = kBufferSize;
  processedSizeLoc = C7zManager::ReadArchiveFile(s->File, g_Buffer, maxRequiredSize);
  *buffer = g_Buffer;
  if (processedSize != 0)
    *processedSize = processedSizeLoc;
  return SZ_OK;
}

#else
// Read callback for the archive file
SZ_RESULT C7zManager::FileRead(void *object, void *buffer, size_t size, size_t *processedSize)
{
  CFileInStream *s = (CFileInStream *)object;
  size_t processedSizeLoc = MyReadFile(s->File, buffer, size);
  if (processedSize != 0)
    *processedSize = processedSizeLoc;
  return SZ_OK;
}

#endif
// Seek callback for the archive file
SZ_RESULT C7zManager::FileSeek(void *object, CFileSize pos)
{
  CFileInStream *s = (CFileInStream *)object;

  int res = s->File->Seek(pos);

  if (res == pos)
    return SZ_OK;
  else
    return SZE_FAIL;
}








/////////////////////////////////////////////////
C7zManager::C7zManager()
{
}

C7zManager::~C7zManager()
{
}

// NB: The rar manager expects paths in rars to be terminated with a "\".
bool C7zManager::GetFilesIn7z(CFileItemList& vecpItems, const CStdString& strRarPath, bool bMask, const CStdString& strPathInRar)
{
  Cache(strRarPath);
  return g_CacheManager.List(vecpItems, strRarPath, strPathInRar);
}

/*bool C7zManager::ListArchive(const CStdString& strRarPath, ArchiveList_struct* &pArchiveList)
{
  return false;
}*/

const CCache* C7zManager::GetFileIn7z(const CStdString& strRarPath, const CStdString& strPathInRar)
{
  return g_CacheManager.GetCached(strRarPath, strPathInRar);
}

bool C7zManager::IsFileIn7z(const CStdString& strRarPath, const CStdString& strPathInRar)
{
  if (!Cache(strRarPath))
    return false;

  return g_CacheManager.IsInCache(strRarPath, strPathInRar);
}

void C7zManager::ClearCache(bool force)
{
}

void C7zManager::ClearCachedFile(const CStdString& strRarPath, const CStdString& strPathInRar)
{
}

void C7zManager::ExtractArchive(const CStdString& strArchive, const CStdString& strPath)
{
}

__int64 C7zManager::CheckFreeSpace(const CStdString& strDrive)
{
  ULARGE_INTEGER lTotalFreeBytes;
  if (GetDiskFreeSpaceEx(strDrive.c_str(), NULL, NULL, &lTotalFreeBytes))
    return lTotalFreeBytes.QuadPart;

  return 0;
}

bool C7zManager::HasMultipleEntries(const CStdString& strPath)
{
  return true;
}

/*************************************************************************************************************/
//     Private
/*************************************************************************************************************/
bool C7zManager::Cache(const CStdString& str7zPath)
{
  if (g_CacheManager.IsCached(str7zPath))
    return true;

  CFileInStream archiveStream;
  CArchiveDatabaseEx db;
  SZ_RESULT res;
  ISzAlloc allocImp;
  ISzAlloc allocTempImp;

  archiveStream.File = new XFILE::CFile();
  if (!archiveStream.File->Open(str7zPath, READ_BUFFERED))
  {
    delete archiveStream.File;
    return false;
  }

  archiveStream.InStream.Read = FileRead;
  archiveStream.InStream.Seek = FileSeek;

  allocImp.Alloc = SzAlloc;
  allocImp.Free = SzFree;

  allocTempImp.Alloc = SzAllocTemp;
  allocTempImp.Free = SzFreeTemp;

  CrcGenerateTable();

  SzArDbExInit(&db);
  res = SzArchiveOpen(&archiveStream.InStream, &db, &allocImp, &allocTempImp);
  if (res == SZ_OK)
  {
    UInt32 i;
    for (i = 0; i < db.Database.NumFiles; i++)
    {
      C7zFileItem *f = db.Database.Files + i;
      CCache *t = new CCache(f->Name, f->IsDirectory, f->Size);

      g_CacheManager.AddToCache(str7zPath, t);
//      g_CacheManager.Print();
    }
  }
  SzArDbExFree(&db, allocImp.Free);

  CloseArchiveFile(archiveStream.File);
  if (res == SZ_OK)
    return true;
  if (res == (SZ_RESULT)SZE_NOTIMPL)
    CLog::Log(LOGERROR, "7z: Decoder doesn't support this archive");
  else if (res == (SZ_RESULT)SZE_OUTOFMEMORY)
    CLog::Log(LOGERROR, "7z: Cannot allocate memory");
  else if (res == (SZ_RESULT)SZE_CRC_ERROR)
    CLog::Log(LOGERROR, "7z: CRC error");
  else     
    CLog::Log(LOGERROR, "7z: Unkown ERROR #%i", res);
  return false;
}
