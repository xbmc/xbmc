/*	MikMod sound library
	(c) 1998, 1999, 2000, 2001, 2002 Miodrag Vallat and others - see file
	AUTHORS for complete list.

	This library is free software;you can redistribute it and/or modify
	it under the terms of the GNU Library General Public License as
	published by the Free Software Foundation;either version 2 of
	the License,or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY;without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU Library General Public License for more details.

	You should have received a copy of the GNU Library General Public
	License along with this library;if not,write to the Free Software
	Foundation,Inc.,59 Temple Place - Suite 330,Boston,MA
	02111-1307,USA.
*/

/*==============================================================================

  $Id$

  General DigiMusic (GDM) module loader

==============================================================================*/

/*

	Written by Kev Vance<kvance@zeux.org>
	based on the file format description written by 'MenTaLguY'
	                                                        <mental@kludge.org>

*/
#include "xbsection_start.h"


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <stdio.h>
#ifdef HAVE_MEMORY_H
#include <memory.h>
#endif
#include <string.h>
#include "mikmod.h"
#include "mikmod_internals.h"

#ifdef USE_GDM_FORMAT

#ifdef SUNOS
extern int fprintf(FILE *, const char *, ...);
#endif

typedef struct GDMNOTE {
	UBYTE note;
	UBYTE samp;
	struct {
		UBYTE effect;
		UBYTE param;
	} effect[4];
} GDMNOTE;

typedef GDMNOTE GDMTRACK[64];

typedef struct GDMHEADER {
	CHAR  id1[4];
	CHAR  songname[32];
	CHAR  author[32];
	CHAR  eofmarker[3];
	CHAR  id2[4];

	UBYTE majorver;
	UBYTE minorver;
	UWORD trackerid;
	UBYTE t_majorver;
	UBYTE t_minorver;
	UBYTE pantable[32];
	UBYTE mastervol;
	UBYTE mastertempo;
	UBYTE masterbpm;
	UWORD flags;

	ULONG orderloc;
	UBYTE ordernum;
	ULONG patternloc;
	UBYTE patternnum;
	ULONG samhead;
	ULONG samdata;
	UBYTE samnum;
	ULONG messageloc;
	ULONG messagelen;
	ULONG scrollyloc;
	UWORD scrollylen;
	ULONG graphicloc;
	UWORD graphiclen;
} GDMHEADER;

typedef struct GDMSAMPLE {
	CHAR  sampname[32];
	CHAR  filename[13];
	UBYTE ems;
	ULONG length;
	ULONG loopbeg;
	ULONG loopend;
	UBYTE flags;
	UWORD c4spd;
	UBYTE vol;
	UBYTE pan;
} GDMSAMPLE;

static GDMHEADER *mh=NULL;	/* pointer to GDM header */
static GDMNOTE *gdmbuf=NULL;	/* pointer to a complete GDM pattern */

CHAR GDM_Version[]="General DigiMusic 1.xx";

BOOL GDM_Test(void)
{
	/* test for gdm magic numbers */
	UBYTE id[4];

	_mm_fseek(modreader,0x00,SEEK_SET);
	if (!_mm_read_UBYTES(id,4,modreader))
		return 0;
	if (!memcmp(id,"GDM\xfe",4)) {
		_mm_fseek(modreader,71,SEEK_SET);
		if (!_mm_read_UBYTES(id,4,modreader))
			return 0;
		if (!memcmp(id,"GMFS",4))
			return 1;
	}
	return 0;
}

BOOL GDM_Init(void)
{
	if (!(gdmbuf=(GDMNOTE*)_mm_malloc(32*64*sizeof(GDMNOTE)))) return 0;
	if (!(mh=(GDMHEADER*)_mm_malloc(sizeof(GDMHEADER)))) return 0;

	return 1;
}

void GDM_Cleanup(void)
{
	_mm_free(mh);
	_mm_free(gdmbuf);
}

BOOL GDM_ReadPattern(void)
{
	int pos,flag,ch,i,maxch;
	GDMNOTE n;
	UWORD length,x=0;

	/* get pattern length */
	length=_mm_read_I_UWORD(modreader)-2;

	/* clear pattern data */
	memset(gdmbuf,255,32*64*sizeof(GDMNOTE));
	pos=0;
	maxch=0;

	while (x<length) {
		memset(&n,255,sizeof(GDMNOTE));
		flag=_mm_read_UBYTE(modreader);
		x++;

		if (_mm_eof(modreader)) {
			_mm_errno=MMERR_LOADING_PATTERN;
			return 0;
		}

		ch=flag&31;
		if (ch>maxch) maxch=ch;
		if (!flag) {
			pos++;
			continue;
		}
		if (flag&0x60) {
			if (flag&0x20) {
				/* new note */
				n.note=_mm_read_UBYTE(modreader)&127;
				n.samp=_mm_read_UBYTE(modreader);
				x +=2;
			}
			if (flag&0x40) {
				do {
					/* effect channel set */
					i=_mm_read_UBYTE(modreader);
					n.effect[i>>6].effect=i&31;
					n.effect[i>>6].param=_mm_read_UBYTE(modreader);
					x +=2;
				} while (i&32);
			}
			memcpy(gdmbuf+(64U*ch)+pos,&n,sizeof(GDMNOTE));
		}
	}
	return 1;
}

UBYTE *GDM_ConvertTrack(GDMNOTE*tr)
{
	int t,i=0;
	UBYTE note,ins,inf;

	UniReset();
	for (t=0;t<64;t++) {
		note=tr[t].note;
		ins=tr[t].samp;

		if ((ins)&&(ins!=255))
			UniInstrument(ins-1);
		if (note!=255) {
			UniNote(((note>>4)*OCTAVE)+(note&0xf)-1);
		}
		for (i=0;i<4;i++) {
			inf = tr[t].effect[i].param;
			switch (tr[t].effect[i].effect) {
				case 1:	/* toneslide up */
					UniEffect(UNI_S3MEFFECTF,inf);
					break;
				case 2:	/* toneslide down */
					UniEffect(UNI_S3MEFFECTE,inf);
					break;
				case 3:	/* glissando to note */
					UniEffect(UNI_ITEFFECTG,inf);
					break;
				case 4:	/* vibrato */
					UniEffect(UNI_ITEFFECTH,inf);
					break;
				case 5:	/* portamento+volslide */
					UniEffect(UNI_ITEFFECTG,0);
					UniEffect(UNI_S3MEFFECTD,inf);
					break;
				case 6:	/* vibrato+volslide */
					UniEffect(UNI_ITEFFECTH,0);
					UniEffect(UNI_S3MEFFECTD,inf);
					break;
				case 7:	/* tremolo */
					UniEffect(UNI_S3MEFFECTR,inf);
					break;
				case 8:	/* tremor */
					UniEffect(UNI_S3MEFFECTI,inf);
					break;
				case 9:	/* offset */
					UniPTEffect(0x09,inf);
					break;
				case 0x0a:	/* volslide */
					UniEffect(UNI_S3MEFFECTD,inf);
					break;
				case 0x0b:	/* jump to order */
					UniPTEffect(0x0b,inf);
					break;
				case 0x0c:	/* volume set */
					UniPTEffect(0x0c,inf);
					break;
				case 0x0d:	/* pattern break */
					UniPTEffect(0x0d,inf);
					break;
				case 0x0e:	/* extended */
					switch (inf&0xf0) {
						case 0x10:	/* fine portamento up */
							UniEffect(UNI_S3MEFFECTF, 0x0f|((inf<<4)&0x0f));
							break;
						case 0x20:	/* fine portamento down */
							UniEffect(UNI_S3MEFFECTE, 0xf0|(inf&0x0f));
							break;
						case 0x30:	/* glissando control */
							UniEffect(SS_GLISSANDO, inf&0x0f);
							break;
						case 0x40:	/* vibrato waveform */
							UniEffect(SS_VIBWAVE, inf&0x0f);
							break;
						case 0x50:	/* set c4spd */
							UniEffect(SS_FINETUNE, inf&0x0f);
							break;
						case 0x60:	/* loop fun */
							UniEffect(UNI_ITEFFECTS0, (inf&0x0f)|0xb0);
							break;
						case 0x70:	/* tremolo waveform */
							UniEffect(SS_TREMWAVE, inf&0x0f);
							break;
						case 0x80:	/* extra fine porta up */
							UniEffect(UNI_S3MEFFECTF, 0x0e|((inf<<4)&0x0f));
							break;
						case 0x90:	/* extra fine porta down */
							UniEffect(UNI_S3MEFFECTE, 0xe0|(inf&0x0f));
							break;
						case 0xa0:	/* fine volslide up */
							UniEffect(UNI_S3MEFFECTD, 0x0f|((inf<<4)&0x0f));
							break;
						case 0xb0:	/* fine volslide down */
							UniEffect(UNI_S3MEFFECTE, 0xf0|(inf&0x0f));
							break;
						case 0xc0:	/* note cut */
						case 0xd0:	/* note delay */
						case 0xe0:	/* extend row */
							UniPTEffect(0xe,inf);
							break;
					}
					break;
				case 0x0f:	/* set tempo */
					UniEffect(UNI_S3MEFFECTA,inf);
					break;
				case 0x10:	/* arpeggio */
					UniPTEffect(0x0,inf);
					break;
				case 0x12:	/* retrigger */
					UniEffect(UNI_S3MEFFECTQ,inf);
					break;
				case 0x13:	/* set global volume */
					UniEffect(UNI_XMEFFECTG,inf<<1);
					break;
				case 0x14:	/* fine vibrato */
					UniEffect(UNI_ITEFFECTU,inf);
					break;
				case 0x1e:	/* special */
					switch (inf&0xf0) {
						case 8:	/* set pan position */
							if (inf >=128)
								UniPTEffect(0x08,255);
							else
								UniPTEffect(0x08,inf<<1);
							break;
					}
					break;
				case 0x1f:	/* set bpm */
					if (inf >=0x20)
						UniEffect(UNI_S3MEFFECTT,inf);
					break;
			}
		}
		UniNewline();
	}
	return UniDup();
}

BOOL GDM_Load(BOOL curious)
{
	int i,x,u,track;
	SAMPLE *q;
	GDMSAMPLE s;
	ULONG position;

	/* read header */
	_mm_read_string(mh->id1,4,modreader);
	_mm_read_string(mh->songname,32,modreader);
	_mm_read_string(mh->author,32,modreader);
	_mm_read_string(mh->eofmarker,3,modreader);
	_mm_read_string(mh->id2,4,modreader);

	mh->majorver=_mm_read_UBYTE(modreader);
	mh->minorver=_mm_read_UBYTE(modreader);
	mh->trackerid=_mm_read_I_UWORD(modreader);
	mh->t_majorver=_mm_read_UBYTE(modreader);
	mh->t_minorver=_mm_read_UBYTE(modreader);
	_mm_read_UBYTES(mh->pantable,32,modreader);
	mh->mastervol=_mm_read_UBYTE(modreader);
	mh->mastertempo=_mm_read_UBYTE(modreader);
	mh->masterbpm=_mm_read_UBYTE(modreader);
	mh->flags=_mm_read_I_UWORD(modreader);

	mh->orderloc=_mm_read_I_ULONG(modreader);
	mh->ordernum=_mm_read_UBYTE(modreader);
	mh->patternloc=_mm_read_I_ULONG(modreader);
	mh->patternnum=_mm_read_UBYTE(modreader);
	mh->samhead=_mm_read_I_ULONG(modreader);
	mh->samdata=_mm_read_I_ULONG(modreader);
	mh->samnum=_mm_read_UBYTE(modreader);
	mh->messageloc=_mm_read_I_ULONG(modreader);
	mh->messagelen=_mm_read_I_ULONG(modreader);
	mh->scrollyloc=_mm_read_I_ULONG(modreader);
	mh->scrollylen=_mm_read_I_UWORD(modreader);
	mh->graphicloc=_mm_read_I_ULONG(modreader);
	mh->graphiclen=_mm_read_I_UWORD(modreader);

	/* have we ended abruptly? */
	if (_mm_eof(modreader)) {
		_mm_errno=MMERR_LOADING_HEADER;
		return 0;
	}

	/* any orders? */
	if(mh->ordernum==255) {
		_mm_errno=MMERR_LOADING_PATTERN;
		return 0;
	}

	/* now we fill */
	of.modtype=strdup(GDM_Version);
	of.modtype[18]=mh->majorver+'0';
	of.modtype[20]=mh->minorver/10+'0';
	of.modtype[21]=mh->minorver%10+'0';
	of.songname=DupStr(mh->songname,32,0);
	of.numpat=mh->patternnum+1;
	of.reppos=0;
	of.numins=of.numsmp=mh->samnum+1;
	of.initspeed=mh->mastertempo;
	of.inittempo=mh->masterbpm;
	of.initvolume=mh->mastervol<<1;
	of.flags|=UF_S3MSLIDES | UF_PANNING;
	/* XXX whenever possible, we should try to determine the original format.
	   Here we assume it was S3M-style wrt bpmlimit... */
	of.bpmlimit = 32;

	/* read the order data */
	if (!AllocPositions(mh->ordernum+1)) {
		_mm_errno=MMERR_OUT_OF_MEMORY;
		return 0;
	}

	_mm_fseek(modreader,mh->orderloc,SEEK_SET);
	for (i=0;i<mh->ordernum+1;i++)
		of.positions[i]=_mm_read_UBYTE(modreader);

	of.numpos=0;
	for (i=0;i<mh->ordernum+1;i++) {
		int order=of.positions[i];
		if(order==255) order=LAST_PATTERN;
		of.positions[of.numpos]=order;
		if (of.positions[i]<254) of.numpos++;
	}

	/* have we ended abruptly yet? */
	if (_mm_eof(modreader)) {
		_mm_errno=MMERR_LOADING_HEADER;
		return 0;
	}

	/* time to load the samples */
	if (!AllocSamples()) {
		_mm_errno=MMERR_OUT_OF_MEMORY;
		return 0;
	}

	q=of.samples;
	position=mh->samdata;

	/* seek to instrument position */
	_mm_fseek(modreader,mh->samhead,SEEK_SET);

	for (i=0;i<of.numins;i++) {
		/* load sample info */
		_mm_read_UBYTES(s.sampname,32,modreader);
		_mm_read_UBYTES(s.filename,12,modreader);
		s.ems=_mm_read_UBYTE(modreader);
		s.length=_mm_read_I_ULONG(modreader);
		s.loopbeg=_mm_read_I_ULONG(modreader);
		s.loopend=_mm_read_I_ULONG(modreader);
		s.flags=_mm_read_UBYTE(modreader);
		s.c4spd=_mm_read_I_UWORD(modreader);
		s.vol=_mm_read_UBYTE(modreader);
		s.pan=_mm_read_UBYTE(modreader);

		if (_mm_eof(modreader)) {
			_mm_errno=MMERR_LOADING_SAMPLEINFO;
			return 0;
		}
		q->samplename=DupStr(s.sampname,32,0);
		q->speed=s.c4spd;
		q->length=s.length;
		q->loopstart=s.loopbeg;
		q->loopend=s.loopend;
		q->volume=s.vol;
		q->panning=s.pan;
		q->seekpos=position;

		position +=s.length;

		if (s.flags&1)
			q->flags |=SF_LOOP;
		if (s.flags&2)
			q->flags |=SF_16BITS;
		if (s.flags&16)
			q->flags |=SF_STEREO;
		q++;
	}

	/* set the panning */
	for (i=x=0;i<32;i++) {
		of.panning[i]=mh->pantable[i];
		if (!of.panning[i])
			of.panning[i]=PAN_LEFT;
		else if (of.panning[i]==8)
			of.panning[i]=PAN_CENTER;
		else if (of.panning[i]==15)
			of.panning[i]=PAN_RIGHT;
		else if (of.panning[i]==16)
			of.panning[i]=PAN_SURROUND;
		else if (of.panning[i]==255)
			of.panning[i]=128;
		else
			of.panning[i]<<=3;
		if (mh->pantable[i]!=255)
			x=i;
	}

	of.numchn=x+1;
	if (of.numchn<1)
		of.numchn=1;	/* for broken counts */

	/* load the pattern info */
	of.numtrk=of.numpat*of.numchn;

	/* jump to patterns */
	_mm_fseek(modreader,mh->patternloc,SEEK_SET);

	if (!AllocTracks()) {
		_mm_errno=MMERR_OUT_OF_MEMORY;
		return 0;
	}

	if (!AllocPatterns()) {
		_mm_errno=MMERR_OUT_OF_MEMORY;
		return 0;
	}

	for (i=track=0;i<of.numpat;i++) {
		if (!GDM_ReadPattern()) {
			_mm_errno=MMERR_LOADING_PATTERN;
			return 0;
		}
		for (u=0;u<of.numchn;u++,track++) {
			of.tracks[track]=GDM_ConvertTrack(&gdmbuf[u<<6]);
			if (!of.tracks[track]) {
				_mm_errno=MMERR_LOADING_TRACK;
				return 0;
			}
		}
	}
	return 1;
}

CHAR *GDM_LoadTitle(void)
{
	CHAR s[32];

	_mm_fseek(modreader,4,SEEK_SET);
	if (!_mm_read_UBYTES(s,32,modreader)) return NULL;

	return DupStr(s,28,0);
}

MIKMODAPI MLOADER load_gdm=
{
	NULL,
	"GDM",
	"GDM (General DigiMusic)",
	GDM_Init,
	GDM_Test,
	GDM_Load,
	GDM_Cleanup,
	GDM_LoadTitle
};

#endif //USE_FAR_FORMAT

/* ex:set ts=4: */
