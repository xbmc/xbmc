//
// C++ Interface: karaokelyricstextlrc
//
// Description:
//
//
// Author: Team XBMC <>, (C) 2008
//
// Copyright: See COPYING file that comes with this distribution
//
//

#ifndef KARAOKELYRICSTEXTLRC_H
#define KARAOKELYRICSTEXTLRC_H


#include "karaokelyricstext.h"


//! This class loads LRC format lyrics
class CKaraokeLyricsTextLRC : public CKaraokeLyricsText
{
  public:
    CKaraokeLyricsTextLRC( const CStdString & lyricsFile );
    ~CKaraokeLyricsTextLRC();

    //! Parses the lyrics or song file, and loads the lyrics into memory. Returns true if the
    //! lyrics are successfully loaded, false otherwise.
    bool Load();

  private:
    CStdString     m_lyricsFile;
};

#endif
