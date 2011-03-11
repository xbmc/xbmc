/*
 *      Copyright (C) 2005-2008 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "MusicInfoTagLoaderMP4.h"
#include "Util.h"
#include "pictures/Picture.h"
#include "id3v1genre.h"
#include "MusicInfoTag.h"
#include "guilib/LocalizeStrings.h"
#include "utils/AutoPtrHandle.h"
#include "utils/log.h"

using namespace XFILE;
using namespace AUTOPTR;
using namespace MUSIC_INFO;

// -------------------------------------------------------------------------------------------------
// Private data & functions purely to satisfy MP4 tag processing code...

#define MAKE_ATOM_NAME( a, b, c, d ) ( ( (a) << 24 ) | ( (b) << 16 ) | ( (c) << 8 ) | (d) )

static const unsigned int g_MetaAtomName        = MAKE_ATOM_NAME( 'm', 'e', 't', 'a' );   // 'meta'
static const unsigned int g_IlstAtomName        = MAKE_ATOM_NAME(  'i', 'l', 's', 't' );  // 'ilst'
static const unsigned int g_MdhdAtomName        = MAKE_ATOM_NAME(  'm', 'd', 'h', 'd' );  // 'mdhd'

static const unsigned int g_TitleAtomName       = MAKE_ATOM_NAME( 0xa9, 'n', 'a', 'm' );  // '©nam'
static const unsigned int g_ArtistAtomName      = MAKE_ATOM_NAME( 0xa9, 'A', 'R', 'T' );  // '©ART'
static const unsigned int g_AlbumArtistAtomName = MAKE_ATOM_NAME( 'a', 'A', 'R', 'T' );   // 'aART'
static const unsigned int g_AlbumAtomName       = MAKE_ATOM_NAME( 0xa9, 'a', 'l', 'b' );  // '©alb'
static const unsigned int g_DayAtomName         = MAKE_ATOM_NAME( 0xa9, 'd', 'a', 'y' );  // '©day'
static const unsigned int g_CustomGenreAtomName = MAKE_ATOM_NAME( 0xa9, 'g', 'e', 'n' );  // '©gnr'
static const unsigned int g_GenreAtomName       = MAKE_ATOM_NAME(  'g', 'n', 'r', 'e' );  // 'gnre'
static const unsigned int g_TrackNumberAtomName = MAKE_ATOM_NAME(  't', 'r', 'k', 'n' );  // 'trkn'
static const unsigned int g_DiscNumberAtomName  = MAKE_ATOM_NAME(  'd', 'i', 's', 'k' );  // 'disk'
static const unsigned int g_CoverArtAtomName    = MAKE_ATOM_NAME(  'c', 'o', 'v', 'r' );  // 'covr'
static const unsigned int g_CompilationAtomName = MAKE_ATOM_NAME(  'c', 'p', 'i', 'l' );  // 'cpil'
static const unsigned int g_CommentAtomName     = MAKE_ATOM_NAME(  0xa9, 'c', 'm', 't' ); // 'cpil'
static const unsigned int g_LyricsAtomName      = MAKE_ATOM_NAME(  0xa9, 'l', 'y', 'r' ); // '©lyr'

// These atoms contain other atoms.. so when we find them, we have to recurse..

static unsigned int g_ContainerAtoms[] =
{
  MAKE_ATOM_NAME( 'm', 'o', 'o', 'v' ),
    MAKE_ATOM_NAME( 't', 'r', 'a', 'k' ),
    MAKE_ATOM_NAME( 'u', 'd', 't', 'a' ),
    MAKE_ATOM_NAME( 't', 'r', 'e', 'f' ),
    MAKE_ATOM_NAME( 'i', 'm', 'a', 'p' ),
    MAKE_ATOM_NAME( 'm', 'd', 'i', 'a' ),
    MAKE_ATOM_NAME( 'm', 'i', 'n', 'f' ),
    MAKE_ATOM_NAME( 's', 't', 'b', 'l' ),
    MAKE_ATOM_NAME( 'e', 'd', 't', 's' ),
    MAKE_ATOM_NAME( 'm', 'd', 'r', 'a' ),
    MAKE_ATOM_NAME( 'r', 'm', 'r', 'a' ),
    MAKE_ATOM_NAME( 'i', 'm', 'a', 'g' ),
    MAKE_ATOM_NAME( 'v', 'n', 'r', 'p' ),
    MAKE_ATOM_NAME( 'd', 'i', 'n', 'f' ),
};


// Read a 32-bit unsigned integer from an unaligned buffer address.

unsigned int CMusicInfoTagLoaderMP4::ReadUnsignedInt( const char* pData )
{
  unsigned int result;

  result =  ((unsigned int)pData[0] & 0xff) << 24;
  result |= ((unsigned int)pData[1] & 0xff) << 16;
  result |= ((unsigned int)pData[2] & 0xff) << 8;
  result |= ((unsigned int)pData[3] & 0xff);
  return result;
}

// Given a metadata type, and a pointer to the data (and the size), this function attempts to populate
// XBMC's CMusicInfoTag object. Tags that we don't support are simply ignored..

void CMusicInfoTagLoaderMP4::ParseTag( unsigned int metaKey, const char* pMetaData, int metaSize, CMusicInfoTag& tag)
{
  switch ( metaKey )
  {
  case g_TitleAtomName:
    {
      // We need to zero-terminate the string, which needs workspace..
      auto_aptr<char> dataWorkspace( new char[ metaSize + 1 ] );
      memcpy( dataWorkspace.get(), pMetaData, metaSize );
      dataWorkspace[ metaSize ] = '\0';

      // we use utf8 internally
      tag.SetLoaded( true );
      tag.SetTitle( dataWorkspace.get() );

      break;
    }

  case g_ArtistAtomName:
    {
      // We need to zero-terminate the string, which needs workspace..
      auto_aptr<char> dataWorkspace( new char[ metaSize + 1 ] );
      memcpy( dataWorkspace.get(), pMetaData, metaSize );
      dataWorkspace[ metaSize ] = '\0';

      tag.SetArtist( dataWorkspace.get() );

      break;
    }

  case g_AlbumAtomName:
    {
      // We need to zero-terminate the string, which needs workspace..
      auto_aptr<char> dataWorkspace( new char[ metaSize + 1 ] );
      memcpy( dataWorkspace.get(), pMetaData, metaSize );
      dataWorkspace[ metaSize ] = '\0';

      tag.SetAlbum( dataWorkspace.get() );

      break;
    }

  case g_AlbumArtistAtomName:
    {
      // We need to zero-terminate the string, which needs workspace..
      auto_aptr<char> dataWorkspace( new char[ metaSize + 1 ] );
      memcpy( dataWorkspace.get(), pMetaData, metaSize );
      dataWorkspace[ metaSize ] = '\0';

      tag.SetAlbumArtist( dataWorkspace.get() );

      break;
    }
  case g_DayAtomName:
    {
      // We need to zero-terminate the string, which needs workspace..
      auto_aptr<char> dataWorkspace( new char[ metaSize + 1 ] );
      memcpy( dataWorkspace.get(), pMetaData, metaSize );
      dataWorkspace[ metaSize ] = '\0';

      SYSTEMTIME dateTime;
      dateTime.wYear = atoi( dataWorkspace.get() );
      tag.SetReleaseDate( dateTime );

      break;
    }

  case g_GenreAtomName:
    {
      // When a genre number is specified, we need to translate to a string for display..
      // Note that AAC/iTunes genre numbers are the same as ID3 numbers, but are offset by 1.
      const char* pGenre = ID3_V1GENRE2DESCRIPTION( (unsigned char)pMetaData[ 1 ] - 1 );
      if ( pGenre )
      {
        tag.SetGenre( pGenre );
      }

      break;
    }
  case g_CompilationAtomName:
    {
      if (metaSize == 1)
        m_isCompilation = *pMetaData == 1;
      break;
    }
  case g_CommentAtomName:
    {
      // We need to zero-terminate the string, which needs workspace..
      auto_aptr<char> dataWorkspace( new char[ metaSize + 1 ] );
      memcpy( dataWorkspace.get(), pMetaData, metaSize );
      dataWorkspace[ metaSize ] = '\0';

      tag.SetComment( dataWorkspace.get() );
      break;
    }
  case g_LyricsAtomName:
    {
      // We need to zero-terminate the string, which needs workspace..
      auto_aptr<char> dataWorkspace( new char[ metaSize + 1 ] );
      memcpy( dataWorkspace.get(), pMetaData, metaSize );
      dataWorkspace[ metaSize ] = '\0';

      tag.SetLyrics( dataWorkspace.get() );
      break;
    }
  case g_CustomGenreAtomName:
    {
      // We need to zero-terminate the string, which needs workspace..
      auto_aptr<char> dataWorkspace( new char[ metaSize + 1 ] );
      memcpy( dataWorkspace.get(), pMetaData, metaSize );
      dataWorkspace[ metaSize ] = '\0';

      tag.SetGenre( dataWorkspace.get() );

      break;
    }

  case g_TrackNumberAtomName:
    {
      tag.SetTrackNumber( pMetaData[ 3 ] );

      break;
    }

  case g_DiscNumberAtomName:
    {
      tag.SetPartOfSet( pMetaData[ 3 ] );

      break;
    }

  case g_CoverArtAtomName:
    {
      // This cover-art handling is pretty much what was in the old MP4 tag processing code..

      // note that according to http://atomicparsley.sourceforge.net/mpeg-4files.html the type
      // of image (PNG=14 or JPG=13) is contained in pMetadata[-5] but we currently don't use this.
      m_thumbSize = metaSize;
      delete[] m_thumbData;
      m_thumbData = new BYTE[m_thumbSize];
      if (m_thumbData)
        memcpy(m_thumbData, pMetaData, metaSize);
      break;
    }
  default:
    break;
  }
}

// Used to locate 'ilst' area within 'meta' atom in a really quick and dirty way. Ideally should
// parse 'ilst' atom list, but this method seems to be reliable.

int CMusicInfoTagLoaderMP4::GetILSTOffset( const char* pBuffer, int bufferSize )
{
  for ( int loop = 0; loop < bufferSize; loop++)
  {
    if ( strncmp( pBuffer + loop, "ilst", 4 ) == 0 )
      return loop;
  }
  return 0;
}


// Parses an atom. Note that this function can recurse a little if it encounters an container atom.
// Apologies for the relatively wired nature of the code (with offset adjustments and so on). The
// basis of the parser was taken from code obtained via this thread at HydrogenAudio Forums..
// http://www.hydrogenaudio.org/forums/lofiversion/index.php/t12075.html, just to get something
// working quickly. The code in that thread seems to be somewhat derived from work at www.getid3.org,
// although it fails to credit them.
//
// I hope to make this particular function more readable/structured when time permits.

int CMusicInfoTagLoaderMP4::ParseAtom( int64_t startOffset, int64_t stopOffset, CMusicInfoTag& tag )
{
  int64_t       currentOffset;
  int           atomSize;
  unsigned int  atomName;
  char          atomHeader[ 10 ];
  int64_t       ret = 0;

  currentOffset = startOffset;
  while ( currentOffset < stopOffset)
  {
    // Seek to the atom header
    ret = m_file.Seek( currentOffset, SEEK_SET );
    if(ret != currentOffset || ret == -1) 
    {
      CLog::Log(LOGDEBUG, "%s unable to Seek.", __FUNCTION__);
      return 0;
    }

    // Read it in.. we only want the atom name & size.. they're always there..
    ret = m_file.Read( atomHeader, 8 );
    if(ret < 8 || ret == -1) 
    {
      CLog::Log(LOGDEBUG, "%s unable to Read.", __FUNCTION__);
      return 0;
    }
    
    // Now pull out the bits we need..
    atomSize = ReadUnsignedInt( &atomHeader[ 0 ] );
    atomName = ReadUnsignedInt( &atomHeader[ 4 ] );

    // See if it's a container atom.. if it is, then recursively call ParseAtom on it...
    for ( unsigned int containerAtom = 0; containerAtom < ( sizeof( g_ContainerAtoms ) / sizeof( unsigned int ) ); containerAtom++ )
    {
      if ( atomName == g_ContainerAtoms[ containerAtom ] )
      {
        ParseAtom( m_file.GetPosition(), currentOffset + atomSize, tag );
        break;
      }
    }

    // We're primarily interested in the 'meta' and 'mdhd' tags..
    if ( atomName == g_MetaAtomName )
    {
      // Use auto_aptr for our workspace, so that it tidies up on any throw..
      auto_aptr<char> atomBuffer( new char[ atomSize ] );

      // Read the metadata in..
      m_file.Read( atomBuffer.get(), atomSize - 4 ); // We've already read the size/name in...

      // Look for the 'ilst' atom, and turn it into an offset within atomBuffer.
      int nextTagPosition = GetILSTOffset( atomBuffer.get(), atomSize - 4 ) + 8;

      // Now go through all of the tags we find.. processing is pretty much taken from source at http://www.getid3.org..
      while ( ( nextTagPosition < ( atomSize - 4 ) ) && ( nextTagPosition > 8 ) )
      {
        int metaSize          = ReadUnsignedInt( atomBuffer.get() + ( nextTagPosition - 4 ) ) - 4;
        unsigned int metaKey  = ReadUnsignedInt( atomBuffer.get() + nextTagPosition );
        char* metaData        = ( atomBuffer.get() + nextTagPosition ) + 20;

        if (metaSize - 20 <= 0)
          break;

        // This is where the next chunk of data will be, if present..
        nextTagPosition += ( metaSize + 4 );


        // Ok.. we've got some metadata to process. Go to it.
        ParseTag( metaKey, metaData, metaSize - 20, tag );
      }
    }
    else
      if ( atomName == g_MdhdAtomName )
      {
        char mdhdData[ 20 ];
        m_file.Read( mdhdData, sizeof( mdhdData ) );

        unsigned int timeScale = ReadUnsignedInt( mdhdData+12 );
        unsigned int duration  = ReadUnsignedInt( mdhdData+16 );
        tag.SetDuration( duration / timeScale );
      }

      // If we've got a zero sized atom, then it's all over.. force the offset to trigger a stop.
      if ( atomSize == 0 )
        currentOffset = stopOffset;
      else
        currentOffset += atomSize;
  }

  // Everything seems to have gone ok...
  return 1;
}

// -------------------------------------------------------------------------------------------------

CMusicInfoTagLoaderMP4::CMusicInfoTagLoaderMP4(void)
{
}

CMusicInfoTagLoaderMP4::~CMusicInfoTagLoaderMP4()
{
}

bool CMusicInfoTagLoaderMP4::Load(const CStdString& strFileName, CMusicInfoTag& tag)
{
  try
  {
    // Initially we say that we've not loaded any tag information
    tag.SetLoaded(false);

    // Attempt to open the file..
    if ( !m_file.Open( strFileName ) )
    {
      CLog::Log(LOGDEBUG, "Tag loader mp4: failed to open file %s", strFileName.c_str() );
      return false;
    }

    // We've opened it, so associate the tag with the filename.
    tag.SetURL(strFileName);

    // Now go parse our atom data
    m_thumbSize = false;
    m_thumbData = NULL;
    m_isCompilation = false;
    ParseAtom( 0, m_file.GetLength(), tag );

    if (m_thumbData)
    { // cache the thumb
      // if we don't have an album tag, cache with the full file path so that
      // other non-tagged files don't get this album image
      CStdString strCoverArt;
      if (!tag.GetAlbum().IsEmpty() && (!tag.GetAlbumArtist().IsEmpty() || !tag.GetArtist().IsEmpty()))
        strCoverArt = CUtil::GetCachedAlbumThumb(tag.GetAlbum(), tag.GetAlbumArtist().IsEmpty() ? tag.GetArtist() : tag.GetAlbumArtist());
      else
        strCoverArt = CUtil::GetCachedMusicThumb(tag.GetURL());
      if (!CUtil::ThumbExists(strCoverArt))
      {
        if (CPicture::CreateThumbnailFromMemory( m_thumbData, m_thumbSize, "", strCoverArt ) )
        {
          CUtil::ThumbCacheAdd( strCoverArt, true );
        }
        else
        {
          CLog::Log(LOGDEBUG, "%s unable to cache thumb as %s", __FUNCTION__, strCoverArt.c_str());
          CUtil::ThumbCacheAdd( strCoverArt, false );
        }
      }
      delete[] m_thumbData;
    }

    if (m_isCompilation)
    { // iTunes compilation flag is set - this could be a various artists file
      if (tag.GetAlbumArtist().IsEmpty())
        tag.SetAlbumArtist(g_localizeStrings.Get(340)); // Various Artists
    }
    // Close the file..
    m_file.Close();

    // Return to caller
    return true;
  }

  catch (...)
  {
    CLog::Log(LOGERROR, "Tag loader mp4: exception in file %s", strFileName.c_str());
  }

  // Something must have gone wrong for us to get here, so let's report our failure to the boss..
  tag.SetLoaded(false);
  return false;
}

