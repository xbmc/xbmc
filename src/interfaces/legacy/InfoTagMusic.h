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

#include "music/tags/MusicInfoTag.h"
#include "AddonClass.h"

#pragma once

namespace XBMCAddon
{
  namespace xbmc
  {
    /**
     * InfoTagMusic class.\n
     */
    class InfoTagMusic : public AddonClass
    {
    private:
      MUSIC_INFO::CMusicInfoTag* infoTag;

    public:
#ifndef SWIG
      InfoTagMusic(const MUSIC_INFO::CMusicInfoTag& tag);
#endif
      InfoTagMusic();
      virtual ~InfoTagMusic();

      /**
       * getURL() -- returns a string.\n
       */
      String getURL();
      /**
       * getTitle() -- returns a string.\n
       */
      String getTitle();
      /**
       * getArtist() -- returns a string.\n
       */
      String getArtist();
      /**
       * getAlbum() -- returns a string.\n
       */
      String getAlbum();
      /**
       * getAlbumArtist() -- returns a string.\n
       */
      String getAlbumArtist();
      /**
       * getGenre() -- returns a string.\n
       */
      String getGenre();
      /**
       * getDuration() -- returns an integer.\n
       */
      int getDuration();
      /**
       * getTrack() -- returns an integer.\n
       */
      int getTrack();
      /**
       * getDisc() -- returns an integer.\n
       */
      int getDisc();
      /**
       * getReleaseDate() -- returns a string.\n
       */
      String getReleaseDate();
      /**
       * getListeners() -- returns an integer.\n
       */
      int getListeners();
      /**
       * getPlayCount() -- returns an integer.\n
       */
      int getPlayCount();
      /**
       * getLastPlayed() -- returns a string.\n
       */
      String getLastPlayed();
      /**
       * getComment() -- returns a string.\n
       */
      String getComment();
      /**
       * getLyrics() -- returns a string.\n
       */
      String getLyrics();
    };
  }
}
  
