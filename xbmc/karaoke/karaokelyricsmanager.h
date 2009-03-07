//
// C++ Interface: karaokelyricsmanager
//
// Description:
//
//
// Author: Team XBMC <>, (C) 2008
//
// Copyright: See COPYING file that comes with this distribution
//
//

#ifndef KARAOKELYRICSMANAGER_H
#define KARAOKELYRICSMANAGER_H

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
    DWORD       m_lastPlayedTime;
};


#endif
