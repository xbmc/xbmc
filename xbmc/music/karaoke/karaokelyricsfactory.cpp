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

// C++ Implementation: karaokelyricsfactory

#include "utils/URIUtils.h"
#include "filesystem/File.h"

#include "karaokelyricscdg.h"
#include "karaokelyricstextkar.h"
#include "karaokelyricstextlrc.h"
#include "karaokelyricstextustar.h"
#include "karaokelyricsfactory.h"


// A helper function to have all the checks in a single place
bool CheckAndCreateLyrics( const CStdString & songName, CKaraokeLyrics ** lyricptr )
{
  CStdString ext, filename = songName;
  URIUtils::RemoveExtension( filename );
  URIUtils::GetExtension( songName, ext );

  // LRC lyrics have .lrc extension
  if ( XFILE::CFile::Exists( filename + ".lrc" ) )
  {
    if ( lyricptr )
      *lyricptr = new CKaraokeLyricsTextLRC( filename + ".lrc" );

    return true;
  }

  // MIDI/KAR files keep lyrics inside
  if ( ext.Left(4) == ".mid" || ext == ".kar" )
  {
    if ( lyricptr )
      *lyricptr = new CKaraokeLyricsTextKAR( songName );

    return true;
  }

  // CD-G lyrics have .cdg extension
  if ( XFILE::CFile::Exists( filename + ".cdg" ) )
  {
    if ( lyricptr )
      *lyricptr = new CKaraokeLyricsCDG( filename + ".cdg" );

    return true;
  }

  // UltraStar lyrics have .txt extension
  if ( XFILE::CFile::Exists( filename + ".txt" ) && CKaraokeLyricsTextUStar::isValidFile( filename + ".txt" ) )
  {
    if ( lyricptr )
      *lyricptr = new CKaraokeLyricsTextUStar( filename + ".txt" );

    return true;
  }

  if ( lyricptr )
    *lyricptr = 0;

  return false;
}


CKaraokeLyrics * CKaraokeLyricsFactory::CreateLyrics( const CStdString & songName )
{
  CKaraokeLyrics * lyricptr = 0;

  CheckAndCreateLyrics( songName, &lyricptr );
  return lyricptr;
}


bool CKaraokeLyricsFactory::HasLyrics(const CStdString & songName)
{
  return CheckAndCreateLyrics( songName, 0 );
}
