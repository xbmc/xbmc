#ifndef KARAOKELYRICS_H
#define KARAOKELYRICS_H

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

// C++ Interface: karaokelyrics

// Abstract interface class for all Karaoke lyrics
class CKaraokeLyrics
{
  public:
      CKaraokeLyrics();
    virtual ~CKaraokeLyrics();

    //! Parses the lyrics or song file, and loads the lyrics into memory. This function
    //! returns true if the lyrics are successfully loaded, or loading is in progress,
    //! and false otherwise.
    virtual bool Load() = 0;

    //! Should return true if the lyrics have background, and therefore should not use
    //! predefined background.
    virtual bool HasBackground() = 0;

    //! Should return true if the lyrics have video file to play
    virtual bool HasVideo() = 0;

    //! Should return video parameters if HasVideo() returned true
    virtual void GetVideoParameters( CStdString& path, int64_t& offset  ) = 0;

    //! This function is called when the karoke visualisation window created. It may
    //! be called after Start(), but is guaranteed to be called before Render()
    //! Default implementation does nothing and returns true.
    virtual bool InitGraphics();

    //! This function is called to render the lyrics (each frame(?))
    virtual void Render() = 0;

    //! This function is called before the object is destroyed. Default implementation does nothing.
    //! You must override it if your lyrics class starts threads which need to be stopped, and stop
    //! all of them before returning back.
    virtual void Shutdown();

    //! This function gets 'real' time since the moment song begins, corrected by using remote control
    //! to increase/decrease lyrics delays. All lyric show functions must use it to properly calculate
    //! the offset.
    double getSongTime() const;

    //! This function gets 'real' time since the moment song begins, corrected by using remote control
    //! to increase/decrease lyrics delays. All lyric show functions must use it to properly calculate
    //! the offset.
    CStdString getSongFile() const;

    //! Sets the path to the lyrics file, and restores musicdb values
    void initData( const CStdString& songPath );

    //! Adjusts lyrics delay
    void lyricsDelayIncrease();
    void lyricsDelayDecrease();

  private:
    //! Number of milliseconds the lyrics are delayed to compensate.
    double        m_avDelay;

    //! Original m_avDelay to see if it was changed
    double        m_avOrigDelay;

    //! Current playing song
    CStdString    m_songPath;
    long          m_idSong;
};

#endif
