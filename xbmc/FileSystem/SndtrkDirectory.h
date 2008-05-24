#pragma once
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


#include "IDirectory.h"

class CSoundtrack
{
public:

  //VOID    GetSoundtrackName( WCHAR* strName ) { wcscpy( strName, m_strName ); }
  //UINT    GetSongCount() { return m_uSongCount; }

  WCHAR strName[42];
  UINT uSongCount;
  UINT uSoundtrackId;
};

typedef std::map<UINT, CSoundtrack> SOUNDTRACK;
typedef std::map<UINT, CSoundtrack>::iterator ISOUNDTRACK;
typedef std::pair<UINT, CSoundtrack> SOUNDTRACK_PAIR;

namespace DIRECTORY
{

class CSndtrkDirectory :
      public IDirectory
{
public:
  CSndtrkDirectory(void);
  virtual ~CSndtrkDirectory(void);
  virtual bool GetDirectory(const CStdString& strPath, CFileItemList &items);
  bool IsAlone(const CStdString& strPath);
  bool FindTrackName(const CStdString& strPath, char* NameOfSong);
};
};
