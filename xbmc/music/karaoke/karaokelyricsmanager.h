#ifndef KARAOKELYRICSMANAGER_H
#define KARAOKELYRICSMANAGER_H

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

// C++ Interface: karaokelyricsmanager

class CKaraokeLyrics;
class CGUIDialogKaraokeSongSelectorSmall;


//! This is the main lyrics manager class, which is called from XBMC code.
class CKaraokeLyricsManager
{
  public:
    //! The class instance created only once during the application life,
    //! and is destroyed when the app shuts down.
     CKaraokeLyricsManager();
    ~CKaraokeLyricsManager();

    //! A new song is started playing
    bool Start( const CStdString& strSongPath );

    //! Called when the current song is being paused or unpaused
    void SetPaused( bool now_paused );

    //! Called when the current song is being stopped. Changing to a new song
    //! in the queue generates Stop() with followed Start() calls. May be called even if
    //! Start() was not called before, so please check.
    void Stop();

    //! Might pop up a selection dialog if playback is ended
    void ProcessSlow();

  private:
    //! Critical section protects this class from requests from different threads
    CCriticalSection   m_CritSection;

    //! A class which handles loading and rendering for this specific karaoke song.
    //! Obtained from KaraokeLyricsFactory
    CKaraokeLyrics  *  m_Lyrics;

    //! True if we're playing a karaoke song
    bool        m_karaokeSongPlaying;

    //! True if we played a karaoke song
    bool        m_karaokeSongPlayed;

    //! Stores the last time the song was still played
    unsigned int m_lastPlayedTime;
};


#endif
