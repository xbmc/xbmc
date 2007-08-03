/*
 * ALAC (Apple Lossless Audio Codec) decoder
 * Copyright (c) 2005 David Hammerton
 * All rights reserved.
 *
 * This is the quicktime container demuxer.
 *
 * http://crazney.net/programs/itunes/alac.html
 * 
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 */


#include <string.h>
#include <stdio.h>
//#include <stdint.h>
#include <stdlib.h>

#include "stream.h"
#include "demux.h"

typedef struct
{
    stream_t *stream;
    demux_res_t *res;
} qtmovie_t;

static	int	g_IsM4AFile;
static	int g_IsAppleLossless;
static	int	g_FoundMdatAtom;
static	int g_StopParsing;

// -------------------------------------------------------------------------------------------------
// Private data & functions purely to satisfy MP4 tag processing code...

#define	MAKE_ATOM_NAME( a, b, c, d )	( ( (a) << 24 ) | ( (b) << 16 ) | ( (c) << 8 ) | (d) )

static const unsigned int	g_FtypAtomName			=	MAKE_ATOM_NAME(	 'f', 't', 'y', 'p' );	// 'ftyp'
static const unsigned int	g_StsdAtomName			=	MAKE_ATOM_NAME(	 's', 't', 's', 'd' );	// 'stsd'
static const unsigned int	g_SttsAtomName			=	MAKE_ATOM_NAME(	 's', 't', 't', 's' );	// 'stts'
static const unsigned int	g_StszAtomName			=	MAKE_ATOM_NAME(	 's', 't', 's', 'z' );	// 'stsz'
static const unsigned int	g_MdatAtomName			=	MAKE_ATOM_NAME(	 'm', 'd', 'a', 't' );	// 'mdat'

// These atoms contain other atoms.. so when we find them, we have to recurse..

static unsigned int	g_ContainerAtoms[] =
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

static unsigned int ReadUnsignedInt( const char* pData )
{
  unsigned int result;

  result =  ((unsigned int)pData[0] & 0xff) << 24;
  result |= ((unsigned int)pData[1] & 0xff) << 16;
  result |= ((unsigned int)pData[2] & 0xff) << 8;
  result |= ((unsigned int)pData[3] & 0xff) << 0;
  return result;
}

// Read a 16-bit unsigned short from an unaligned buffer address.

static unsigned short ReadUnsignedShort( const char* pData )
{
  unsigned int result;

  result =  ((unsigned int)pData[0] & 0xff) << 8;
  result |= ((unsigned int)pData[1] & 0xff) << 0;
  return result;
}

static void	ProcessFtypAtom( char* atomData, qtmovie_t* qtmovie )
{
	unsigned int	type = ReadUnsignedInt( atomData );

	if ( type == MAKE_ATOM_NAME( 'M', '4', 'A', ' ' ) )
		g_IsM4AFile = 1;
	else
	{
		// Not an M4A file, so time to stop.. 
		g_StopParsing = 1;
	}
}

static void	ProcessStsdAtom( char* atomData, qtmovie_t* qtmovie )
{	
	unsigned int entrySize;
	unsigned int numEntries = ReadUnsignedInt( &atomData[ 4 ] );

	// According to original source, we only expect one entry in sample description atom
	if ( numEntries != 1 )
		return;
	
	// The size of the entry is...?
	entrySize = ReadUnsignedInt( &atomData[ 8 ] );
	qtmovie->res->format = ReadUnsignedInt( &atomData[ 12 ] );

	if ( qtmovie->res->format == MAKE_ATOM_NAME( 'a', 'l', 'a', 'c' ) )
	{
		g_IsAppleLossless = 1;

		qtmovie->res->num_channels	= ReadUnsignedShort( &atomData[ 32 ] );
		qtmovie->res->sample_size	= ReadUnsignedShort( &atomData[ 34 ] );
		qtmovie->res->sample_rate	= ReadUnsignedShort( &atomData[ 40 ] );

		// 36 is the bytes for prior to codec data for this entry.
		// 12 is the additional size of our atom header (the 3 uint writes)
		// 8 is for padding, as the original code is a bit paranoid..
		qtmovie->res->codecdata_len = ( entrySize - 36 ) + 12 + 8;
		qtmovie->res->codecdata = malloc( qtmovie->res->codecdata_len );
		memset( qtmovie->res->codecdata, 0, qtmovie->res->codecdata_len );
		
		( ( unsigned int* )qtmovie->res->codecdata )[0] = 0x0c000000;
		( ( unsigned int* )qtmovie->res->codecdata )[1] = MAKE_ATOM_NAME( 'a', 'm', 'r', 'f' );
		( ( unsigned int* )qtmovie->res->codecdata )[2] = MAKE_ATOM_NAME( 'c', 'a', 'l', 'a' );
		memcpy( ( ( char* )qtmovie->res->codecdata ) + 12, &atomData[ 44 ], qtmovie->res->codecdata_len - 8 );
	}
	else
	{
		// Not an apple lossless file.. not interested.
		g_StopParsing = 1;
	}
}

static void	ProcessSttsAtom( char* atomData, qtmovie_t* qtmovie )
{
	unsigned int loop;
	unsigned int numEntries = ReadUnsignedInt( &atomData[ 4 ] );
	qtmovie->res->num_time_to_samples = numEntries;
	qtmovie->res->time_to_sample = malloc( numEntries * sizeof( *qtmovie->res->time_to_sample ) );

	for ( loop = 0; loop < numEntries; loop++ )
	{	
		qtmovie->res->time_to_sample[ loop ].sample_count		= ReadUnsignedInt( &atomData[ 8 + ( 8 * loop ) + 0 ] );
		qtmovie->res->time_to_sample[ loop ].sample_duration	= ReadUnsignedInt( &atomData[ 8 + ( 8 * loop ) + 4 ] );
	}
}

static void ProcessStszAtom( char* atomData, qtmovie_t* qtmovie )
{
	unsigned int loop;
	unsigned int numEntries;
	if ( ReadUnsignedInt( &atomData[ 4 ] ) == 0 )
	{
		numEntries = ReadUnsignedInt( &atomData[ 8 ] );
		qtmovie->res->num_sample_byte_sizes = numEntries;	
		qtmovie->res->sample_byte_size = malloc( numEntries * sizeof( *qtmovie->res->sample_byte_size ) );
	
		for ( loop = 0; loop < numEntries; loop++ )
		{
			qtmovie->res->sample_byte_size[ loop ] = ReadUnsignedInt( &atomData[ 12 + ( 4 * loop ) ] );
		}
	}
}

static char*	GetAtomData( qtmovie_t* qtmovie, int atomSize )
{
	char* pMemory = (char*)malloc( atomSize );
	stream_read( qtmovie->stream, atomSize, pMemory );
	return pMemory;
}

static void		FreeAtomData( char* atomData )
{
	free( atomData );
}

static void ParseAtom( int startOffset, int stopOffset, qtmovie_t* qtmovie )
{
  long			currentOffset;

  int			containerAtom;
  int			atomSize;
  unsigned int	atomName;
  char			atomHeader[ 10 ];

  if ( g_StopParsing )
	return;

  currentOffset = startOffset;
  while ( currentOffset < stopOffset)
  {
    // Seek to the atom header
    stream_seek( qtmovie->stream, currentOffset, SEEK_SET );

	memset( atomHeader, 0, 10 );

    // Read it in.. we only want the atom name & size..	they're always there..
    stream_read( qtmovie->stream, 8, atomHeader );

    // Now pull out the bits we need..
    atomSize = ReadUnsignedInt( &atomHeader[ 0 ] );
    atomName = ReadUnsignedInt( &atomHeader[ 4 ] );

    // See if it's a container atom.. if it is, then recursively call ParseAtom on it...
    for ( containerAtom = 0; containerAtom < ( sizeof( g_ContainerAtoms ) / sizeof( unsigned int ) ); containerAtom++ )
    {
      if ( atomName == g_ContainerAtoms[ containerAtom ] )
      {
        ParseAtom( stream_tell( qtmovie->stream ), currentOffset + atomSize, qtmovie );
        break;
      }
    }

	if ( atomName == g_FtypAtomName )
	{
		void* pAtomData = GetAtomData( qtmovie, atomSize );
		ProcessFtypAtom( pAtomData, qtmovie );
		FreeAtomData( pAtomData );
	}
	else
	if ( atomName == g_StsdAtomName )
	{
		void* pAtomData = GetAtomData( qtmovie, atomSize );
		ProcessStsdAtom( pAtomData, qtmovie );
		FreeAtomData( pAtomData );
	}
	else
	if ( atomName == g_SttsAtomName )
	{
		void* pAtomData = GetAtomData( qtmovie, atomSize );
		ProcessSttsAtom( pAtomData, qtmovie );
		FreeAtomData( pAtomData );
	}
	else
	if ( atomName == g_StszAtomName )
	{
		void* pAtomData = GetAtomData( qtmovie, atomSize );
		ProcessStszAtom( pAtomData, qtmovie );
		FreeAtomData( pAtomData );
	}
	else
	if ( atomName == g_MdatAtomName )
	{
		g_FoundMdatAtom = 1;
		g_StopParsing = 1;
	}

    // If we've got a zero sized atom, then it's all over.. force the offset to trigger a stop.
    if ( atomSize == 0 )
      currentOffset = stopOffset;
    else
      currentOffset += atomSize;	
  }

  // Everything seems to have gone ok...
  return;
}

// I kept the same interface, because.. well.. it seemed to make sense at the time.
int qtmovie_read(stream_t *file, demux_res_t *demux_res)
{
	// Our parameter object..
	qtmovie_t	qtmovie;

	// Our filesize
	long		fileSize;

	// We haven't found the mdat chunk yet..
	g_FoundMdatAtom		= 0;
	g_IsM4AFile			= 0;
	g_IsAppleLossless	= 0;
	g_StopParsing		= 0;
	
    // construct the stream;
    qtmovie.stream	= file;
    qtmovie.res		= demux_res;

	// Find the file length..
	stream_seek( file, 0, SEEK_END );
	fileSize = stream_tell( file );
	stream_seek( file, 0, SEEK_SET );
	
	// find the goo inside..
	ParseAtom( 0, fileSize, &qtmovie );
		
	// Return our state to the caller..
	if ( g_FoundMdatAtom && g_IsM4AFile && g_IsAppleLossless )
		return 1;
	else
	{
		// Something didn't go right, so we attempt to clear the allocations that were made during
		// the failed call, before returning that it failed to the caller.
		free( demux_res->codecdata );
		free( demux_res->time_to_sample );
		free( demux_res->sample_byte_size );
		return 0;
	}
}



