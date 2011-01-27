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

#include "system.h"

#ifdef HAS_FILESYSTEM_MMS

#include "FileItem.h"
#include "DVDInputStreamMMS.h"
#include "filesystem/IFile.h"
#include "settings/GUISettings.h"

#ifndef _WIN32
// work around for braindead usage of "this" keyword as parameter name in libmms headers
// some distros have already patched this but upstream @ https://launchpad.net/libmms
// does still has the "this" usage.
#define this instance
#undef byte
#include <libmms/mmsio.h> // FIXME: remove this header once the ubuntu headers is fixed (variable named this)
#include <libmms/mms.h>
#include <libmms/mmsh.h>
#include <libmms/mmsx.h>
#undef this
#else
#include "win32/libmms_win32/src/mmsx.h"
#endif

using namespace XFILE;

CDVDInputStreamMMS::CDVDInputStreamMMS() : CDVDInputStream(DVDSTREAM_TYPE_MMS)
{
  m_mms = NULL;
}

CDVDInputStreamMMS::~CDVDInputStreamMMS()
{
  Close();
}

bool CDVDInputStreamMMS::IsEOF()
{
  return false;
}

#ifndef _WIN32
struct mmsx_s {
  mms_t *connection;
  mmsh_t *connection_h;
};
#endif

bool CDVDInputStreamMMS::Open(const char* strFile, const std::string& content)
{
  int bandwidth = g_guiSettings.GetInt("network.bandwidth") * 1024;
  if(bandwidth == 0)
    bandwidth = 2000*1000;

  // TODO: remove this code if upstream accepts my patch
  // tracked at https://bugs.launchpad.net/libmms/+bug/512089
  // also see the struct definition further up (needed due to opaqueness)
#ifndef _WIN32
  m_mms = (mmsx_t*)calloc(1, sizeof(mmsx_t));
  
  if (!m_mms)
    return false;
  
  m_mms->connection_h = mmsh_connect((mms_io_t*)mms_get_default_io_impl(),
                                     NULL,strFile,bandwidth);
  if (m_mms->connection_h)
    return true;
    
  m_mms->connection = mms_connect((mms_io_t*)mms_get_default_io_impl(),
                                  NULL,strFile,bandwidth);
  if (m_mms->connection)
    return true;
    
  free(m_mms);
  m_mms = NULL;
  return false;
#else
  m_mms = mmsx_connect((mms_io_t*)mms_get_default_io_impl(),NULL,strFile,bandwidth); // TODO: what to do with bandwidth?
  return (m_mms != NULL);
#endif
}

// close file and reset everyting
void CDVDInputStreamMMS::Close()
{
  CDVDInputStream::Close();
  if (m_mms)
    mmsx_close(m_mms);
}

int CDVDInputStreamMMS::Read(BYTE* buf, int buf_size)
{
  return mmsx_read((mms_io_t*)mms_get_default_io_impl(),m_mms,(char*)buf,buf_size);
}

__int64 CDVDInputStreamMMS::Seek(__int64 offset, int whence)
{
  if(whence == SEEK_POSSIBLE)
    return 0;
  else
    return -1; // TODO: implement offset based seeks
}

bool CDVDInputStreamMMS::SeekTime(int iTimeInMsec)
{
  if (mmsx_get_seekable(m_mms))
    return (mmsx_time_seek(NULL,m_mms,double(iTimeInMsec)/1000) != -1);

  return false;
}

__int64 CDVDInputStreamMMS::GetLength()
{
  return (__int64)mmsx_get_time_length(m_mms);
}

bool CDVDInputStreamMMS::NextStream()
{
  return false;
}

#endif
