/*
 *  Copyright (c) 2002 Frodo
 *      Portions Copyright (c) by the authors of ffmpeg and xvid
 *  Copyright (C) 2002-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "IFile.h"

#include "URL.h"

#include <cstring>
#include <errno.h>

using namespace XFILE;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

IFile::IFile() = default;

IFile::~IFile() = default;

int IFile::Stat(struct __stat64* buffer)
{
  memset(buffer, 0, sizeof(struct __stat64));
  errno = ENOENT;
  return -1;
}
bool IFile::ReadString(char *szLine, int iLineLength)
{
  if(Seek(0, SEEK_CUR) < 0) return false;

  int64_t iFilePos = GetPosition();
  int iBytesRead = Read( (unsigned char*)szLine, iLineLength - 1);
  if (iBytesRead <= 0)
    return false;

  szLine[iBytesRead] = 0;

  for (int i = 0; i < iBytesRead; i++)
  {
    if ('\n' == szLine[i])
    {
      if ('\r' == szLine[i + 1])
      {
        szLine[i + 1] = 0;
        Seek(iFilePos + i + 2, SEEK_SET);
      }
      else
      {
        // end of line
        szLine[i + 1] = 0;
        Seek(iFilePos + i + 1, SEEK_SET);
      }
      break;
    }
    else if ('\r' == szLine[i])
    {
      if ('\n' == szLine[i + 1])
      {
        szLine[i + 1] = 0;
        Seek(iFilePos + i + 2, SEEK_SET);
      }
      else
      {
        // end of line
        szLine[i + 1] = 0;
        Seek(iFilePos + i + 1, SEEK_SET);
      }
      break;
    }
  }
  return true;
}

ssize_t IFile::GetMaxReadSize()
{
  int chunkSize = GetChunkSize();

  switch (chunkSize)
  {
    case 0:
    case 1:
      // Backwards compatibility:
      // CFileCache used to handle a chunk size of 0 or 1 by
      // a) Getting the chunk size from advanced settings
      // b) Reading one chunk at a time (the only way it knew how!)
      // To not change this behavior for IFile implementations,
      // Assume the ONE_CHUNK behavior in those cases and let the IFiles
      // that want something different override this method.
      return ReadSizeRequestCode::ONE_CHUNK;
    default:
      return chunkSize;
  }
}

CRedirectException::CRedirectException() :
  m_pNewFileImp(NULL), m_pNewUrl(NULL)
{
}

CRedirectException::CRedirectException(IFile *pNewFileImp, CURL *pNewUrl) :
  m_pNewFileImp(pNewFileImp), m_pNewUrl(pNewUrl)
{
}
