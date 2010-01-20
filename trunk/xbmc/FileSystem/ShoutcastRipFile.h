#ifndef CShoutcastRipFile_H
#define CShoutcastRipFile_H
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


#include "MusicInfoTag.h"

typedef struct RecStateSt
{
  bool bRecording;
  bool bCanRecord;
  bool bTrackChanged;
  bool bFilenameSet;
  bool bStreamSet;
  bool bHasMetaData;
}
RecState;

/* prototype */
typedef struct RIP_MANAGER_INFOst RIP_MANAGER_INFO;

//if one would like to make other IFiles recordable, it would be
//great, to define a superclass for this one with differend useful
//functions for all, like FilterFileName(), Record(),...
class CShoutcastRipFile
{
public:

  CShoutcastRipFile();
  virtual ~CShoutcastRipFile();


  void SetRipManagerInfo( const RIP_MANAGER_INFO* ripInfo );
  void SetTrackname( const char* trackname );


  bool Record();
  bool CanRecord();
  void StopRecording();
  bool IsRecording();
  void Write( char *buf, unsigned long size );
  void Reset();
  void GetMusicInfoTag(MUSIC_INFO::CMusicInfoTag& tag);
protected:


private:

  void PrepareRecording( );
  void RemoveIllegalChars( char *szRemoveIllegal );
  void GetDirectoryName( char* directoryName );
  void SetFilename( const char* filePath, const char* fileName );
  void CutAfterLastChar( char* szToBeCutted, int where );
  void RemoveLastSpace( char* szToBeRemoved );


  RecState m_recState;
  int m_iTrackCount;
  char m_szFileName[1024]; //FIXME: i think, that length could be optimized here
  char m_szFilteredFileName[1024];
  char m_szStreamName[1024];
  FILE* m_ripFile;
  FILE* m_logFile;
  MUSIC_INFO::CMusicInfoTag m_Tag;
};


#endif
