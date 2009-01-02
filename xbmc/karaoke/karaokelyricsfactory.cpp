//
// C++ Implementation: karaokelyricsfactory
//
// Description: 
//
//
// Author: Team XBMC <>, (C) 2008
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "stdafx.h"

#include "Util.h"
#include "FileSystem/File.h"

#include "karaokelyricscdg.h"
#include "karaokelyricstextkar.h"
#include "karaokelyricstextlrc.h"
#include "karaokelyricsfactory.h"


CKaraokeLyrics * CKaraokeLyricsFactory::CreateLyrics( const CStdString & songName )
{
	CStdString ext, filename = songName;
	CUtil::RemoveExtension( filename );
	CUtil::GetExtension( songName, ext );

	// CD-G lyrics have .cdg extension
	if ( XFILE::CFile::Exists( filename + ".cdg" ) )
		return new CKaraokeLyricsCDG( filename + ".cdg" );

	// LRC lyrics have .lrc extension
	if ( XFILE::CFile::Exists( filename + ".lrc" ) )
		return new CKaraokeLyricsTextLRC( filename + ".lrc" );

	// MIDI/KAR files keep lyrics inside
	if ( ext.Left(4) == ".mid" || ext == ".kar" )
		return new CKaraokeLyricsTextKAR( songName );

	return 0;
}
