//
// C++ Interface: karaokelyricsfactory
//
// Description:
//
//
// Author: Team XBMC <>, (C) 2008
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef KARAOKELYRICSFACTORY_H
#define KARAOKELYRICSFACTORY_H

/**
  @author Team XBMC
*/

#include "karaokelyrics.h"

class CKaraokeLyricsFactory
{
  public:
      CKaraokeLyricsFactory() {};
     ~CKaraokeLyricsFactory() {};

    //! This function will be called to check if there are any classes which could load lyrics
    //! for the song played. The action will be executed in a single thread, and therefore
    //! should be limited to simple checks like whether the specific filename exists.
    //! If the loader needs more than that to make sure lyrics are there, it must create this
    //! loader, which should handle the processing in load().
    static CKaraokeLyrics * CreateLyrics( const CStdString& songName );

    //! This function returns true if the lyrics are (or might be) available for this song.
    static bool HasLyrics( const CStdString& songName );
};

#endif
