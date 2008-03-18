/*
* XBoxMediaPlayer
* Copyright (c) 2002 Frodo
* Portions Copyright (c) by the authors of ffmpeg and xvid
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
#include "FileMemUnit.h"
#include <sys/stat.h>
#include "../utils/MemoryUnitManager.h"
#include "MemoryUnits/IFileSystem.h"
#include "MemoryUnits/IDevice.h"

using namespace XFILE;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
//*********************************************************************************************
CFileMemUnit::CFileMemUnit()
{
  m_fileSystem = NULL;
}

//*********************************************************************************************
CFileMemUnit::~CFileMemUnit()
{
  Close();
}
//*********************************************************************************************
bool CFileMemUnit::Open(const CURL& url, bool bBinary)
{
  Close();

  m_fileSystem = GetFileSystem(url);
  if (!m_fileSystem) return false;

  return m_fileSystem->Open(GetPath(url));
}

bool CFileMemUnit::OpenForWrite(const CURL& url, bool bBinary, bool bOverWrite)
{
  Close();

  m_fileSystem = GetFileSystem(url);
  if (!m_fileSystem) return false;

  return m_fileSystem->OpenForWrite(GetPath(url), bOverWrite);
}

//*********************************************************************************************
unsigned int CFileMemUnit::Read(void *lpBuf, __int64 uiBufSize)
{
  if (!m_fileSystem) return 0;
  return m_fileSystem->Read(lpBuf, uiBufSize);
}

int CFileMemUnit::Write(const void* lpBuf, __int64 uiBufSize)
{
  if (!m_fileSystem) return 0;
  return m_fileSystem->Write(lpBuf, uiBufSize);
}

//*********************************************************************************************
void CFileMemUnit::Close()
{
  if (m_fileSystem)
  {
    m_fileSystem->Close();
    delete m_fileSystem;
  }
  m_fileSystem = NULL;
}

//*********************************************************************************************
__int64 CFileMemUnit::Seek(__int64 iFilePosition, int iWhence)
{
  if (!m_fileSystem) return -1;
  __int64 position = iFilePosition;
  if (iWhence == SEEK_CUR)
    position += m_fileSystem->GetPosition();
  else if (iWhence == SEEK_END)
    position += m_fileSystem->GetLength();
  else if (iWhence != SEEK_SET)
    return -1;

  if (position < 0) position = 0;
  if (position > m_fileSystem->GetLength()) position = m_fileSystem->GetLength();
  return m_fileSystem->Seek(position);
}

//*********************************************************************************************
__int64 CFileMemUnit::GetLength()
{
  if (!m_fileSystem) return -1;
  return m_fileSystem->GetLength();
}

//*********************************************************************************************
__int64 CFileMemUnit::GetPosition()
{
  if (!m_fileSystem) return -1;
  return m_fileSystem->GetPosition();
}

bool CFileMemUnit::Exists(const CURL& url)
{
  if (Open(url, true))
  {
    Close();
    return true;
  }
  return false;
}

int CFileMemUnit::Stat(const CURL& url, struct __stat64* buffer)
{
  if (Open(url, true))
  {
    Close();
    return 0;
  }
  return -1;
}

bool CFileMemUnit::Delete(const CURL& url)
{
  IFileSystem *fileSystem = GetFileSystem(url);
  if (fileSystem)
    return fileSystem->Delete(GetPath(url));
  return false;
}

bool CFileMemUnit::Rename(const CURL& url, const CURL& urlnew)
{
  IFileSystem *fileSystem = GetFileSystem(url);
  if (fileSystem)
    return fileSystem->Rename(GetPath(url), GetPath(urlnew));
  return false;
}

IFileSystem *CFileMemUnit::GetFileSystem(const CURL& url)
{
  unsigned char unit = url.GetProtocol()[3] - '0';
  return g_memoryUnitManager.GetFileSystem(unit);
}

CStdString CFileMemUnit::GetPath(const CURL& url)
{
  CStdString path = url.GetFileName();
  path.Replace("\\", "/");
  return path;
}