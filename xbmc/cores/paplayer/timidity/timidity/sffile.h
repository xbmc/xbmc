/*
    TiMidity++ -- MIDI to WAVE converter and player
    Copyright (C) 1999-2002 Masanao Izumo <mo@goice.co.jp>
    Copyright (C) 1995 Tuukka Toivonen <tt@cgs.fi>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

/*================================================================
 * sffile.h
 *	SoundFont file (SBK/SF2) format defintions
 *
 * Copyright (C) 1996,1997 Takashi Iwai
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *================================================================*/

/*
 * Modified by Masanao Izumo <mo@goice.co.jp>
 */

#ifndef SFFILE_H_DEF
#define SFFILE_H_DEF

/* chunk record header */
typedef struct _SFChunk {
	char id[4];
	int32 size;
} SFChunk;

/* generator record */
typedef struct _SFGenRec {
	int16 oper;
	int16 amount;
} SFGenRec;

/* layered generators record */
typedef struct _SFGenLayer {
	int nlists;
	SFGenRec *list;
} SFGenLayer;

/* header record */
typedef struct _SFHeader {
	char name[20];
	uint16 bagNdx;
	/* layered stuff */
	int nlayers;
	SFGenLayer *layer;
} SFHeader;

/* preset header record */
typedef struct _SFPresetHdr {
	SFHeader hdr;
	uint16 preset, bank;
	/*int32 lib, genre, morphology;*/ /* not used */
} SFPresetHdr;

/* instrument header record */
typedef struct _SFInstHdr {
	SFHeader hdr;
} SFInstHdr;

/* sample info record */
typedef struct _SFSampleInfo {
	char name[20];
	int32 startsample, endsample;
	int32 startloop, endloop;
	/* ver.2 additional info */
	int32 samplerate;
	uint8 originalPitch;
	int8 pitchCorrection;
	uint16 samplelink;
	uint16 sampletype;  /*1=mono, 2=right, 4=left, 8=linked, $8000=ROM*/
	/* optional info */
	int32 size; /* sample size */
	int32 loopshot; /* short-shot loop size */
} SFSampleInfo;


/*----------------------------------------------------------------
 * soundfont file info record
 *----------------------------------------------------------------*/

typedef struct _SFInfo {
	/* file name */
	char *sf_name;

	/* version of this file */
	uint16 version, minorversion;
	/* sample position (from origin) & total size (in bytes) */
	long samplepos;
	int32 samplesize;

	/* raw INFO chunk list */
	long infopos, infosize;

	/* preset headers */
	int npresets;
	SFPresetHdr *preset;
	
	/* sample infos */
	int nsamples;
	SFSampleInfo *sample;

	/* instrument headers */
	int ninsts;
	SFInstHdr *inst;

} SFInfo;


/*----------------------------------------------------------------
 * functions
 *----------------------------------------------------------------*/

/* sffile.c */
extern int load_soundfont(SFInfo *sf, struct timidity_file *fp);
extern void free_soundfont(SFInfo *sf);
extern void correct_samples(SFInfo *sf);

#endif
