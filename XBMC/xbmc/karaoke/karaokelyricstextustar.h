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

#ifndef KARAOKELYRICSTEXTUSTAR_H
#define KARAOKELYRICSTEXTUSTAR_H


#include "karaokelyricstext.h"


//! This class loads UltraStar format lyrics
class CKaraokeLyricsTextUStar : public CKaraokeLyricsText
{
  public:
    CKaraokeLyricsTextUStar( const CStdString & lyricsFile );
    ~CKaraokeLyricsTextUStar();

    //! Parses the lyrics or song file, and loads the lyrics into memory. Returns true if the
    //! lyrics are successfully loaded, false otherwise.
    bool Load();

    static bool isValidFile( const CStdString & lyricsFile );

  private:
    static std::vector<CStdString> readFile( const CStdString & lyricsFile, bool report_errors );

  private:
    CStdString     m_lyricsFile;
};

#endif
