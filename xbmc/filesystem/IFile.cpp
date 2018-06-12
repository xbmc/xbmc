/*
 *      Copyright (c) 2002 Frodo
 *      Portions Copyright (c) by the authors of ffmpeg and xvid
 *      Copyright (C) 2002-2013 Team XBMC
 *      http://kodi.tv
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

CRedirectException::CRedirectException() :
  m_pNewFileImp(NULL), m_pNewUrl(NULL)
{
}

CRedirectException::CRedirectException(IFile *pNewFileImp, CURL *pNewUrl) :
  m_pNewFileImp(pNewFileImp), m_pNewUrl(pNewUrl)
{
}
