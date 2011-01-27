#pragma once
/*
* DAAP Support for XBMC
* Copyright (c) 2004 Forza (Chris Barnett)
* Portions Copyright (c) by the authors of libOpenDAAP
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
#include "IDirectory.h"

extern "C"
{
#include "lib/libXDAAP/client.h"
#include "lib/libXDAAP/private.h"
}

namespace XFILE
{
class CDAAPDirectory :

      public IDirectory
{
public:
  CDAAPDirectory(void);
  virtual ~CDAAPDirectory(void);
  virtual bool IsAllowed(const CStdString &strFile) const { return true; };
  virtual bool GetDirectory(const CStdString& strPath, CFileItemList &items);
  //virtual void CloseDAAP(void);
  int GetCurrLevel(CStdString strPath);

private:
  void free_albums(albumPTR *alb);
  void free_artists();
  void AddToArtistAlbum(char *artist_s, char *album_s);

  DAAP_ClientHost_DatabaseItem *m_currentSongItems;
  int m_currentSongItemCount;

  DAAP_SClientHost *m_thisHost;
  int m_currLevel;

  artistPTR *m_artisthead;
  CStdString m_selectedPlaylist;
  CStdString m_selectedArtist;
  CStdString m_selectedAlbum;
};
}
