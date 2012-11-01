/*
* SAP-Announcement Support for XBMC
* Copyright (c) 2008 elupus (Joakim Plate)
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

#include "SAPFile.h"
#include "SAPDirectory.h"
#include "threads/SingleLock.h"
#include "URL.h"

#include <sys/stat.h>
#include <vector>

using namespace std;
using namespace XFILE;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CSAPFile::CSAPFile()
{}

CSAPFile::~CSAPFile()
{
}

bool CSAPFile::Open(const CURL& url)
{
  CStdString path = url.Get();

  CSingleLock lock(g_sapsessions.m_section);
  for(vector<CSAPSessions::CSession>::iterator it = g_sapsessions.m_sessions.begin(); it != g_sapsessions.m_sessions.end(); it++)
  {
    if(it->path == path)
    {
      m_len = it->payload.length();
      m_stream.str(it->payload);
      m_stream.seekg(0);
      break;
    }
  }
  if(m_len == 0)
    return false;

  return true;
}

bool CSAPFile::Exists(const CURL& url)
{
  CStdString path = url.Get();

  CSingleLock lock(g_sapsessions.m_section);
  for(vector<CSAPSessions::CSession>::iterator it = g_sapsessions.m_sessions.begin(); it != g_sapsessions.m_sessions.end(); it++)
  {
    if(it->path == path)
      return true;
  }
  return false;
}

int CSAPFile::Stat(const CURL& url, struct __stat64* buffer)
{
  CStdString path = url.Get();

  if(path == "smb://")
  {
    if(buffer)
    {
      memset(buffer, 0, sizeof(struct __stat64));
      buffer->st_mode = _S_IFDIR;
    }
    return true;
  }


  CSingleLock lock(g_sapsessions.m_section);
  for(vector<CSAPSessions::CSession>::iterator it = g_sapsessions.m_sessions.begin(); it != g_sapsessions.m_sessions.end(); it++)
  {
    if(it->path == path)
    {
      if(buffer)
      {
        memset(buffer, 0, sizeof(*buffer));

        buffer->st_size = it->payload.size();
        buffer->st_mode = _S_IFREG;
      }
      return true;
    }
  }
  return false;

}


unsigned int CSAPFile::Read(void *lpBuf, int64_t uiBufSize)
{
  return (unsigned int)m_stream.readsome((char*)lpBuf, (streamsize)uiBufSize);
}

void CSAPFile::Close()
{
}

//*********************************************************************************************
int64_t CSAPFile::Seek(int64_t iFilePosition, int iWhence)
{
  switch (iWhence)
  {
    case SEEK_SET:
      m_stream.seekg((int)iFilePosition, ios_base::beg);
      break;
    case SEEK_CUR:
      m_stream.seekg((int)iFilePosition, ios_base::cur);
      break;
    case SEEK_END:
      m_stream.seekg((int)iFilePosition, ios_base::end);
      break;
    default:
      return -1;
  }
  return m_stream.tellg();
}

//*********************************************************************************************
int64_t CSAPFile::GetLength()
{
  return m_len;
}

//*********************************************************************************************
int64_t CSAPFile::GetPosition()
{
  return m_stream.tellg();
}

bool CSAPFile::Delete(const CURL& url)
{
  CStdString path = url.Get();

  CSingleLock lock(g_sapsessions.m_section);
  for(vector<CSAPSessions::CSession>::iterator it = g_sapsessions.m_sessions.begin(); it != g_sapsessions.m_sessions.end(); it++)
  {
    if(it->path == path)
    {
      g_sapsessions.m_sessions.erase(it);
      return true;
    }
  }
  return false;
}

bool CSAPFile::Rename(const CURL& url, const CURL& urlnew)
{
  return false;
}

int CSAPFile::IoControl(EIoControl request, void* param)
{
  if(request == IOCTRL_SEEK_POSSIBLE)
    return 1;

  return -1;
}
