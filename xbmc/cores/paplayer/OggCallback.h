#pragma once

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

#include "DllVorbisfile.h"
#include "filesystem/File.h"

class COggCallback
{
public:
  COggCallback(XFILE::CFile& file);

  ov_callbacks Get(const CStdString& strFile);

  static size_t ReadCallback(void *ptr, size_t size, size_t nmemb, void *datasource);
  static int    SeekCallback(void *datasource, ogg_int64_t offset, int whence);
  static int    NoSeekCallback(void *datasource, ogg_int64_t offset, int whence);
  static int    CloseCallback(void *datasource);
  static long   TellCallback(void *datasource);
protected:
  XFILE::CFile& m_file;
};

