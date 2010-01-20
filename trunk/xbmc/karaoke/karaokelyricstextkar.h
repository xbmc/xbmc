//
// C++ Interface: karaokelyricstextkar
//
// Description:
//
//
// Author: Team XBMC <>, (C) 2008
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef KARAOKELYRICSTEXTKAR_H
#define KARAOKELYRICSTEXTKAR_H


#include "karaokelyricstext.h"


//! This class loads MIDI/KAR format lyrics
class CKaraokeLyricsTextKAR : public CKaraokeLyricsText
{
  public:
    CKaraokeLyricsTextKAR( const CStdString & midiFile );
    ~CKaraokeLyricsTextKAR();

    //! Parses the lyrics or song file, and loads the lyrics into memory.
    //! Returns true if the lyrics are successfully loaded, false otherwise.
    bool  Load();

  private:
    void      parseMIDI();
    CStdString    convertText( const char * data );

    unsigned char   readByte();
    unsigned short  readWord();
    unsigned int  readDword();
    int       readVarLen();
    void      readData( void * buf, unsigned int length );

    unsigned int   currentPos() const;
    void      setPos( unsigned int offset );

    // MIDI file name
    CStdString     m_midiFile;

    // MIDI in-memory information
    unsigned char *  m_midiData;
    unsigned int  m_midiOffset;
    unsigned int  m_midiSize;
    bool          m_reportedInvalidVarField;
};

#endif
