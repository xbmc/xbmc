/*
 *      Copyright (C) 2005-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "OGGFileDirectory.h"
#include "cores/paplayer/OggCallback.h"
#include "File.h"

using namespace MUSIC_INFO;
using namespace XFILE;

COGGFileDirectory::COGGFileDirectory(void)
{
  m_strExt = "oggstream";
}

COGGFileDirectory::~COGGFileDirectory(void)
{
}

int COGGFileDirectory::GetTrackCount(const CStdString& strPath)
{
  if (!m_dll.Load())
    return 0;
  
  CFile file;
  if (!file.Open(strPath))
    return 0;
  
  COggCallback callback(file);
  ov_callbacks oggIOCallbacks = callback.Get(strPath);
  OggVorbis_File vf;
  //  open ogg file with decoder
  if (m_dll.ov_open_callbacks(&callback, &vf, NULL, 0, oggIOCallbacks)!=0)
    return 0;
  
  int iStreams=m_dll.ov_streams(&vf);
  
  m_dll.ov_clear(&vf);
  
  return iStreams;
}
