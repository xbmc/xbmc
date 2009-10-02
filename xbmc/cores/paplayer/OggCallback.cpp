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

#include "OggCallback.h"
  
COggCallback::COggCallback(XFILE::CFile& file) : m_file(file)
{
}

ov_callbacks COggCallback::Get(const CStdString& strFile)
{
  // libvorbis requires that a non-seekable stream would always return -1 from seek actions.
  // so for network streams - tweak the seek method to a static one that always return -1.
  bool bIsStream = false;
  if (strFile.Left(5).ToLower() == "shout" || strFile.Left(4).ToLower() == "http")
    bIsStream = true;

  ov_callbacks oggIOCallbacks;
  oggIOCallbacks.read_func=ReadCallback;
  oggIOCallbacks.seek_func=bIsStream?NoSeekCallback:SeekCallback;
  oggIOCallbacks.tell_func=TellCallback;
  oggIOCallbacks.close_func=CloseCallback;

  return oggIOCallbacks;
}

size_t COggCallback::ReadCallback(void *ptr, size_t size, size_t nmemb, void *datasource)
{
  COggCallback* pCallback=(COggCallback*)datasource;
  if (!pCallback)
    return 0;

  return pCallback->m_file.Read(ptr, size*nmemb);
}

int COggCallback::SeekCallback(void *datasource, ogg_int64_t offset, int whence)
{
  COggCallback* pCallback=(COggCallback*)datasource;
  if (!pCallback)
    return 0;

  return (int)pCallback->m_file.Seek(offset, whence);
}

int COggCallback::NoSeekCallback(void *datasource, ogg_int64_t offset, int whence)
{
  return -1;
}

int COggCallback::CloseCallback(void *datasource)
{
  COggCallback* pCallback=(COggCallback*)datasource;
  if (!pCallback)
    return 0;

  pCallback->m_file.Close();
  return 1;
}

long COggCallback::TellCallback(void *datasource)
{
  COggCallback* pCallback=(COggCallback*)datasource;
  if (!pCallback)
    return 0;

  return (long)pCallback->m_file.GetPosition();
}

