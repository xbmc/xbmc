/*
 * function: Header file for wave_out.c
 *
 * This program is distributed under the GNU General Public License, version 2.
 * A copy of this license is included with this source.
 *
 * Copyright (C) 2002 John Edwards
 */

#ifndef __WAVE_OUT_H__
#define __WAVE_OUT_H__


#include <stdio.h>
#include <windows.h>

#define Cdecl               __cdecl
#define __attribute__(x)
#define sleep(__sec)        Sleep ((__sec) * 1000)
#define inline              __inline
#define restrict

/*
 * constants
 */

#define CD_SAMPLE_FREQ         44.1e3
#define SAMPLE_SIZE            16
#define SAMPLE_SIZE_STRING     ""
#define WINAUDIO_FD            ((FILE_T)-128)
#define FILE_T                 FILE*
#define INVALID_FILEDESC       NULL

/*
 * Simple types
 */

typedef signed   int        Int;        // at least -32767...+32767, fast type
typedef unsigned int        Uint;       // at least 0...65535, fast type
typedef long double         Ldouble;    // most exact floating point format

/*
 * functions for wave_out.c
 */

Int   Set_WIN_Params        ( FILE_T dummyFile , Ldouble SampleFreq, Uint BitsPerSample, Uint Channels );
int   WIN_Play_Samples      ( const void* buff, size_t len );
int   WIN_Audio_close       ( void );

#endif   /* __WAVE_OUT_H__ */
