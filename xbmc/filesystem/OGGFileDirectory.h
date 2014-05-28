#pragma once
/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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


#ifndef FILESYSTEM_MUSICFILEDIRECTORY_H_INCLUDED
#define FILESYSTEM_MUSICFILEDIRECTORY_H_INCLUDED
#include "MusicFileDirectory.h"
#endif

#ifndef FILESYSTEM_DLLVORBISFILE_H_INCLUDED
#define FILESYSTEM_DLLVORBISFILE_H_INCLUDED
#include "DllVorbisfile.h"
#endif


namespace XFILE
{
  class COGGFileDirectory : public CMusicFileDirectory
  {
    public:
      COGGFileDirectory(void);
      virtual ~COGGFileDirectory(void);
    protected:
      virtual int GetTrackCount(const CStdString& strPath);
  private:
    DllVorbisfile m_dll;
  };
}
