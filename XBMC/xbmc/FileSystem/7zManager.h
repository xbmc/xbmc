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

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "File.h"
#include "../FileItem.h"
#include "../utils/CriticalSection.h"
#include <map>
#include "../utils/Stopwatch.h"
#include "../utils/Thread.h"

#include "CacheManager.h"

#include "../lib/lib7z/7zIn.h"
#include "../lib/lib7z/7zExtract.h"
#include "../lib/lib7z/7zCrc.h"

#define EXFILE_OVERWRITE 1
#define EXFILE_AUTODELETE 2
#define EXFILE_UNIXPATH 4
#define RAR_DEFAULT_CACHE "Z:\\"
#define RAR_DEFAULT_PASSWORD ""

typedef struct _CFileInStream
{
  ISzInStream InStream;
  XFILE::CFile *File;
} CFileInStream;

class CFile7zExtractThread : public IRunnable
{
public:
  CFile7zExtractThread(CStdString path, CStdString pathInArchive, Byte **Buff, size_t *BuffSize);
  ~CFile7zExtractThread();
  void Run();

  Byte    *m_Buffer;
  size_t  m_BufferSize;
  UInt32  m_blockIndex;
  size_t  m_offset;
  size_t  m_outSizeProcessed;
  size_t  m_NowPos;
  bool    m_Ready;
  bool    m_Error;
  XFILE::CFile *m_TempOut;
private:
  XFILE::CFile *m_outFile; // Internal
  XFILE::CFile *m_inFile;  // Internal
  CStdString str7zPath;
  CStdString strIn7z;
};

class C7zManager
{
public:
  C7zManager();
  ~C7zManager();
  bool Cache7zFile(CStdString& strPathInCache, const CStdString& strRarPath, const CStdString& strPathInRar, BYTE bOptions = EXFILE_AUTODELETE, const CStdString& strDir =RAR_DEFAULT_CACHE, const __int64 iSize=-1);
  bool GetPathInCache(CStdString& strPathInCache, const CStdString& strRarPath, const CStdString& strPathInRar = "");
  bool HasMultipleEntries(const CStdString& strPath);
  bool GetFilesIn7z(CFileItemList& vecpItems, const CStdString& strRarPath, bool bMask=true, const CStdString& strPathInRar = "");
  const CCache *GetFileIn7z(const CStdString& strRarPath, const CStdString& strPathInRar);
  bool IsFileIn7z(const CStdString& strRarPath, const CStdString& strPathInRar);
  void ClearCache(bool force=false);
  void ClearCachedFile(const CStdString& strRarPath, const CStdString& strPathInRar);
  void ExtractArchive(const CStdString& strArchive, const CStdString& strPath);

  bool Cache(const CStdString& str7zPath);
//  void SetWipeAtWill(bool bWipe) { m_bWipe = bWipe; }
//  CThread *CreateExtractThread(const CStdString& str7zPath, const CStdString& strPathIn7z, Byte *Buffer, size_t *BufferSize);
//   7zCallbacks
  static size_t    ReadArchiveFile(XFILE::CFile *file, void *data, size_t size);
  static size_t    Writer(XFILE::CFile *file, void *data, size_t size);
  static SZ_RESULT FileRead(void *, void **, size_t, size_t *);
  static SZ_RESULT FileSeek(void *object, CFileSize pos);
  static int       CloseArchiveFile(XFILE::CFile *file);
protected:
  CCriticalSection m_CritSection;

  __int64 CheckFreeSpace(const CStdString& strDrive);
};

extern C7zManager g_7zManager;

