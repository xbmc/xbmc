/*	MikMod sound library
	(c) 1998, 1999, 2000, 2001 Miodrag Vallat and others - see file AUTHORS
	for complete list.

	This library is free software; you can redistribute it and/or modify
	it under the terms of the GNU Library General Public License as
	published by the Free Software Foundation; either version 2 of
	the License, or (at your option) any later version.
 
	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU Library General Public License for more details.
 
	You should have received a copy of the GNU Library General Public
	License along with this library; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
	02111-1307, USA.
*/

/*==============================================================================

  $Id$

  WAV sample loader

==============================================================================*/
#include "xbsection_start.h"


/*
   FIXME: Stereo .WAV files are not yet supported as samples.
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <string.h>
#include "mikmod.h"
#include "mikmod_internals.h"

#ifdef SUNOS
extern int fprintf(FILE *, const char *, ...);
#endif

extern BOOL		initialized;

typedef struct WAV {
	CHAR  rID[4];
	ULONG rLen;
	CHAR  wID[4];
	CHAR  fID[4];
	ULONG fLen;
	UWORD wFormatTag;
	UWORD nChannels;
	ULONG nSamplesPerSec;
	ULONG nAvgBytesPerSec;
	UWORD nBlockAlign;
	UWORD nFormatSpecific;
} WAV;

SAMPLE* Sample_LoadGeneric_internal(MREADER* reader)
{
	SAMPLE *si=NULL;
	WAV wh;
	BOOL have_fmt=0;

	/* read wav header */
	_mm_read_string(wh.rID,4,reader);
	wh.rLen = _mm_read_I_ULONG(reader);
	_mm_read_string(wh.wID,4,reader);

	/* check for correct header */
	if(_mm_eof(reader)|| memcmp(wh.rID,"RIFF",4) || memcmp(wh.wID,"WAVE",4)) {
		_mm_errno = MMERR_UNKNOWN_WAVE_TYPE;
		return NULL;
	}

	/* scan all RIFF blocks until we find the sample data */
	for(;;) {
		CHAR dID[4];
		ULONG len,start;

		_mm_read_string(dID,4,reader);
		len = _mm_read_I_ULONG(reader);
		/* truncated file ? */
		if (_mm_eof(reader)) {
			_mm_errno=MMERR_UNKNOWN_WAVE_TYPE;
			return NULL;
		}
		start = _mm_ftell(reader);

		/* sample format block
		   should be present only once and before a data block */
		if(!memcmp(dID,"fmt ",4)) {
			wh.wFormatTag      = _mm_read_I_UWORD(reader);
			wh.nChannels       = _mm_read_I_UWORD(reader);
			wh.nSamplesPerSec  = _mm_read_I_ULONG(reader);
			wh.nAvgBytesPerSec = _mm_read_I_ULONG(reader);
			wh.nBlockAlign     = _mm_read_I_UWORD(reader);
			wh.nFormatSpecific = _mm_read_I_UWORD(reader);

#ifdef MIKMOD_DEBUG
			fprintf(stderr,"\rwavloader : wFormatTag=%04x blockalign=%04x nFormatSpc=%04x\n",
			        wh.wFormatTag,wh.nBlockAlign,wh.nFormatSpecific);
#endif

			if((have_fmt)||(wh.nChannels>1)) {
				_mm_errno=MMERR_UNKNOWN_WAVE_TYPE;
				return NULL;
			}
			have_fmt=1;
		} else
		/* sample data block
		   should be present only once and after a format block */
		  if(!memcmp(dID,"data",4)) {
			if(!have_fmt) {
				_mm_errno=MMERR_UNKNOWN_WAVE_TYPE;
				return NULL;
			}
			if(!(si=(SAMPLE*)_mm_malloc(sizeof(SAMPLE)))) return NULL;
			si->speed  = wh.nSamplesPerSec/wh.nChannels;
			si->volume = 64;
			si->length = len;
			if(wh.nBlockAlign == 2) {
				si->flags    = SF_16BITS | SF_SIGNED;
				si->length >>= 1;
			}
			si->inflags = si->flags;
			SL_RegisterSample(si,MD_SNDFX,reader);
			SL_LoadSamples();
			
			/* skip any other remaining blocks - so in case of repeated sample
			   fragments, we'll return the first anyway instead of an error */
			break;
		}
		/* onto next block */
		_mm_fseek(reader,start+len,SEEK_SET);
		if (_mm_eof(reader))
			break;
	}

	return si;
}

MIKMODAPI SAMPLE* Sample_LoadGeneric(MREADER* reader)
{
	SAMPLE* result;

	if(!initialized)
		return NULL;

	MUTEX_LOCK(vars);
	result=Sample_LoadGeneric_internal(reader);
	MUTEX_UNLOCK(vars);

	return result;
}

MIKMODAPI SAMPLE* Sample_LoadFP(FILE *fp)
{
	SAMPLE* result=NULL;
	MREADER* reader;

	if(!initialized)
		return NULL;
	if((reader=_mm_new_file_reader(fp))) {
		result=Sample_LoadGeneric(reader);
		_mm_delete_file_reader(reader);
	}
	return result;
}

MIKMODAPI SAMPLE* Sample_Load(CHAR* filename)
{
	FILE *fp;
	SAMPLE *si=NULL;

	if(!initialized)
		return NULL;
	if(!(md_mode & DMODE_SOFT_SNDFX)) return NULL;

	if((fp=_mm_fopen(filename,"rb"))) {
		si = Sample_LoadFP(fp);
		_mm_fclose(fp);
	}
	return si;
}

MIKMODAPI void Sample_Free(SAMPLE* si)
{
	if(si) {
		MD_SampleUnload(si->handle);
		free(si);
	}
}

void Sample_Free_internal(SAMPLE *si)
{
	MUTEX_LOCK(vars);
	Sample_Free(si);
	MUTEX_UNLOCK(vars);
}

/* ex:set ts=4: */
