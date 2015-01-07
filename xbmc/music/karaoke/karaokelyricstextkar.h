#ifndef KARAOKELYRICSTEXTKAR_H
#define KARAOKELYRICSTEXTKAR_H

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

// C++ Interface: karaokelyricstextkar

#include "karaokelyricstext.h"
#include "utils/auto_buffer.h"

//! This class loads MIDI/KAR format lyrics
class CKaraokeLyricsTextKAR : public CKaraokeLyricsText
{
  public:
    CKaraokeLyricsTextKAR( const std::string & midiFile );
    ~CKaraokeLyricsTextKAR();

    //! Parses the lyrics or song file, and loads the lyrics into memory.
    //! Returns true if the lyrics are successfully loaded, false otherwise.
    bool  Load();

  private:
    void      parseMIDI();
    std::string    convertText( const char * data );

    unsigned char   readByte();
    unsigned short  readWord();
    unsigned int  readDword();
    int       readVarLen();
    void      readData( void * buf, unsigned int length );

    unsigned int   currentPos() const;
    void      setPos( unsigned int offset );

    // MIDI file name
    std::string     m_midiFile;

    // MIDI in-memory information
    XUTILS::auto_buffer m_midiData;
    size_t        m_midiOffset;
    bool          m_reportedInvalidVarField;
};

#endif
