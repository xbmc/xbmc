#ifndef KARAOKELYRICSFACTORY_H
#define KARAOKELYRICSFACTORY_H

/**
  @author Team XBMC
*/

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

// C++ Interface: karaokelyricsfactory

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
