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

#include "stdafx.h"
#include "SndtrkDirectory.h"
#include "xbox/IoSupport.h"
#include "FileItem.h"

SOUNDTRACK datastorage; //created a vector of the XSOUNDTRACK_DATA class to keep track of each album

using namespace DIRECTORY;

CSndtrkDirectory::CSndtrkDirectory(void)
{}

CSndtrkDirectory::~CSndtrkDirectory(void)
{}


bool CSndtrkDirectory::GetDirectory(const CStdString& strPath, CFileItemList &items)
{
  CStdString strRoot = strPath;
  if (IsAlone(strRoot))
  {
    // Add each user provided soundtrack to the soundtrack vector
    // Took code from samples in XDK
    XSOUNDTRACK_DATA stData;
    HANDLE hSoundtrack = XFindFirstSoundtrack( &stData );
    if ( INVALID_HANDLE_VALUE != hSoundtrack )
    {
      do
      {
        // Ignore empty soundtracks && parent directories
        if ( stData.uSongCount > 0 && strRoot != "..")
        {
          CSoundtrack stInfo;
          stInfo.uSoundtrackId = stData.uSoundtrackId;
          stInfo.uSongCount = stData.uSongCount;
          wcscpy(stInfo.strName, stData.szName );

          ISOUNDTRACK it=datastorage.find(stData.uSoundtrackId);
          if (it==datastorage.end())
            datastorage.insert(SOUNDTRACK_PAIR(stInfo.uSoundtrackId, stInfo));
          else
            it->second=stInfo;

          CFileItem *pItem = new CFileItem(stData.szName);
          pItem->m_strPath = strRoot;
          pItem->SetLabelPreformated(true);
          char tmpvar[4];
          itoa (stData.uSoundtrackId, tmpvar, 10);
          pItem->m_strPath += tmpvar;
          pItem->m_bIsFolder = true;
          items.Add(pItem);
        }
      }
      while ( XFindNextSoundtrack( hSoundtrack, &stData ) );

      XFindClose(hSoundtrack);
    }
  }
  else
  {
    char *ptr = strstr(strRoot, "//");
    ptr += 2;
    int m_iconvert = atoi(ptr); //convert from char back to int to compare to data

    ISOUNDTRACK it=datastorage.find(m_iconvert);
    if (it == datastorage.end())
      return false;

    CSoundtrack stInfo = it->second;
    for ( UINT i = 0; i < stInfo.uSongCount; ++i )
    {
      DWORD dwSongId;
      DWORD dwSongLength;
      WCHAR strSong[64];
      if ( XGetSoundtrackSongInfo( stInfo.uSoundtrackId, i, &dwSongId,
                                   &dwSongLength, strSong, MAX_SONG_NAME ) )
      {
        // Add it to the list
        CFileItem *pItem = new CFileItem(strSong);
        pItem->m_strPath = "E:\\TDATA\\fffe0000\\music\\";
        char tmpvar[16];
        *tmpvar = NULL;
        sprintf(tmpvar, "%04x", stInfo.uSoundtrackId);
        pItem->m_strPath += tmpvar;
        pItem->m_strPath += "\\";
        *tmpvar = NULL;
        sprintf(tmpvar, "%08x", dwSongId);
        pItem->m_strPath += tmpvar;
        pItem->m_strPath += ".wma";
        pItem->m_bIsFolder = false;
        items.Add(pItem);
      }
    }
  }
  return true;
}

bool CSndtrkDirectory::IsAlone(const CStdString& strPath)
{
  return (strcmp("soundtrack://", strPath) == 0);
}

bool CSndtrkDirectory::FindTrackName(const CStdString& strPath, char *NameOfSong)
{
  char* ptr = strstr(strPath.c_str(), "E:\\TDATA\\fffe0000\\music\\");
  if (ptr == NULL) return false;
  ptr += strlen("E:\\TDATA\\fffe0000\\music\\");
  char album[5];
  int x = 0;
  for (x = 0;x < 4;x++)
  {
    album[x] = *ptr;
    ptr += 1;
  }
  album[4] = '\0';
  ptr += 1;
  char trackno[9];
  for (x = 0;x < 8;x++)
  {
    trackno[x] = *ptr;
    ptr += 1;
  }
  trackno[8] = '\0';
  int AlbumID, SongID;
  sscanf(album, "%x", &AlbumID);
  sscanf(trackno, "%x", &SongID);
  x = 0;
  bool test = true;
  while (test == true)
  {
    DWORD dwSongId;
    DWORD dwSongLength;
    WCHAR Songname[64];
    if ( XGetSoundtrackSongInfo( AlbumID, x, &dwSongId, &dwSongLength, Songname, MAX_SONG_NAME ) == false)
      test = false;
    if (dwSongId == SongID)
    {
      wcstombs(NameOfSong, Songname, 64);
      return true;
    }
    x++;
  }
  return false;
}

