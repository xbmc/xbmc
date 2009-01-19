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

    //! Called when a button is pressed; used to select songs, adjust lyrics speed and so on.
    bool OnAction( const CAction &action );

    //! A new song is started playing
    bool Start( const CStdString& strSongPath );

    //! Called when the current song is being stopped, or paused. Changing to a new song
    //! in the queue generates Stop() with followed Start() calls. May be called even if
    //! Start() was not called before, so please check.
    void Stop();

    //! This function is called to render the lyrics (each frame(?))
    void Render();

    //! Returns true if lyrics are loaded and we have something to render
    bool isLyricsAvailable() const;

    //! Might pop up a selection dialog if playback is ended
    void ProcessSlow();

  protected:
    bool isSongSelectorAvailable();

  private:
    //! Critical section protects this class from requests from different threads
    CCriticalSection   m_CritSection;

    //! A class which handles loading and rendering for this specific karaoke song.
    //! Obtained from KaraokeLyricsFactory
    CKaraokeLyrics  *  m_Lyrics;

    //! A class which handles remote buttons pressed during karaoke playback,
    //! provides visual feedback, and puts selected songs into a queue.
    CGUIDialogKaraokeSongSelectorSmall * m_songSelector;

    //! True if we're playing a karaoke song
    bool        m_karaokeSongPlaying;

    //! True if we played a karaoke song
    bool        m_karaokeSongPlayed;
    
    DWORD       m_lastPlayedTime;
};


#endif
