// timidity.cpp : Defines the entry point for the console application.
//

#include "timidity_codec.h"

#include <stdio.h>
#include <string.h>

#ifdef WIN32
	#define EXPORT __declspec(dllexport)
#else
	#define EXPORT
#endif

static int init_done = 0;

int EXPORT DLL_Init( const char * soundfont )
{
    if ( init_done )
      return 1;

    if ( Timidity_Init( 48000, 16, 2, soundfont ) != 0 )
      return 0;

    init_done = 1;
    return 1;
}

void EXPORT DLL_Cleanup()
{
    Timidity_Cleanup();
    init_done = 0;
}

const EXPORT char * DLL_ErrorMsg()
{
    return Timidity_ErrorMsg();
}

void EXPORT * DLL_LoadMID( const char* szFileName )
{
    return Timidity_LoadSong( (char*)szFileName );
}

void EXPORT DLL_FreeMID( void* mid )
{
    Timidity_FreeSong( (MidiSong *) mid );
}

int EXPORT DLL_FillBuffer( void* mid, char* szBuffer, int iSize)
{
    return Timidity_FillBuffer( (MidiSong *) mid, szBuffer, iSize );
}

unsigned long EXPORT DLL_Seek( void* mid, unsigned long iTimePos)
{
    return Timidity_Seek( (MidiSong *) mid, iTimePos );
}

const char EXPORT *DLL_GetTitle(void* mid)
{
    return "";
}

const char EXPORT *DLL_GetArtist(void* mid)
{
     return "";
}

unsigned long EXPORT DLL_GetLength(void* mid)
{
    return Timidity_GetLength( (MidiSong *) mid );
}

