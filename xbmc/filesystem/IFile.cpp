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
  if (buffer)
    *buffer = {};

  errno = ENOENT;
  return -1;
}
IFile::ReadLineResult IFile::ReadLine(char* szLine, std::size_t bufferSize)
{
  if (Seek(0, SEEK_CUR) < 0)
    return {ReadLineResult::FAILURE, 0};

  int64_t iFilePos = GetPosition();
  size_t iBytesRead = Read((unsigned char*)szLine, bufferSize - 1);
  if (iBytesRead <= 0)
    return {ReadLineResult::FAILURE, 0};

  szLine[iBytesRead] = 0;

  for (size_t i = 0; i < iBytesRead; i++)
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
      return {ReadLineResult::OK, i + 1};
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
      return {ReadLineResult::OK, i + 1};
    }
  }
  return {ReadLineResult::TRUNCATED, iBytesRead};
}

CRedirectException::CRedirectException() :
  m_pNewFileImp(NULL), m_pNewUrl(NULL)
{
}

CRedirectException::CRedirectException(IFile *pNewFileImp, CURL *pNewUrl) :
  m_pNewFileImp(pNewFileImp), m_pNewUrl(pNewUrl)
{
}
