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

  Routines for loading samples. The sample loader utilizes the routines
  provided by the "registered" sample loader.

==============================================================================*/
#include "xbsection_start.h"


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include "mikmod.h"
#include "mikmod_internals.h"

static	int sl_rlength;
static	SWORD sl_old;
static	SWORD *sl_buffer=NULL;
static	SAMPLOAD *musiclist=NULL,*sndfxlist=NULL;

/* size of the loader buffer in words */
#define SLBUFSIZE 2048

/* IT-Compressed status structure */
typedef struct ITPACK {
	UWORD bits;    /* current number of bits */
	UWORD bufbits; /* bits in buffer */
	SWORD last;    /* last output */
	UBYTE buf;     /* bit buffer */
} ITPACK;

BOOL SL_Init(SAMPLOAD* s)
{
	if(!sl_buffer)
		if(!(sl_buffer=_mm_malloc(SLBUFSIZE*sizeof(SWORD)))) return 0;

	sl_rlength = s->length;
	if(s->infmt & SF_16BITS) sl_rlength>>=1;
	sl_old = 0;

	return 1;
}

void SL_Exit(SAMPLOAD *s)
{
	if(sl_rlength>0) _mm_fseek(s->reader,sl_rlength,SEEK_CUR);
	if(sl_buffer) {
		free(sl_buffer);
		sl_buffer=NULL;
	}
}

/* unpack a 8bit IT packed sample */
static BOOL read_itcompr8(ITPACK* status,MREADER *reader,SWORD *sl_buffer,UWORD count,UWORD* incnt)
{
	SWORD *dest=sl_buffer,*end=sl_buffer+count;
	UWORD x,y,needbits,havebits,new_count=0;
	UWORD bits = status->bits;
	UWORD bufbits = status->bufbits;
	SBYTE last = status->last;
	UBYTE buf = status->buf;

	while (dest<end) {
		needbits=new_count?3:bits;
		x=havebits=0;
		while (needbits) {
			/* feed buffer */
			if (!bufbits) {
				if((*incnt)--)
					buf=_mm_read_UBYTE(reader);
				else
					buf=0;
				bufbits=8;
			}
			/* get as many bits as necessary */
			y = needbits<bufbits?needbits:bufbits;
			x|= (buf & ((1<<y)- 1))<<havebits;
			buf>>=y;
			bufbits-=y;
			needbits-=y;
			havebits+=y;
		}
		if (new_count) {
			new_count = 0;
			if (++x >= bits)
				x++;
			bits = x;
			continue;
		}
		if (bits<7) {
			if (x==(1<<(bits-1))) {
				new_count = 1;
				continue;
			}
		}
		else if (bits<9) {
			y = (0xff >> (9-bits)) - 4;
			if ((x>y)&&(x<=y+8)) {
				if ((x-=y)>=bits)
					x++;
				bits = x;
				continue;
			}
		}
		else if (bits<10) {
			if (x>=0x100) {
				bits=x-0x100+1;
				continue;
			}
		} else {
			/* error in compressed data... */
			_mm_errno=MMERR_ITPACK_INVALID_DATA;
			return 0;
		}

		if (bits<8) /* extend sign */
			x = ((SBYTE)(x <<(8-bits))) >> (8-bits);
		*(dest++)= (last+=x) << 8; /* convert to 16 bit */
	}
	status->bits = bits;
	status->bufbits = bufbits;
	status->last = last;
	status->buf = buf;
	return dest-sl_buffer;
}

/* unpack a 16bit IT packed sample */
static BOOL read_itcompr16(ITPACK *status,MREADER *reader,SWORD *sl_buffer,UWORD count,UWORD* incnt)
{
	SWORD *dest=sl_buffer,*end=sl_buffer+count;
	SLONG x,y,needbits,havebits,new_count=0;
	UWORD bits = status->bits;
	UWORD bufbits = status->bufbits;
	SWORD last = status->last;
	UBYTE buf = status->buf;

	while (dest<end) {
		needbits=new_count?4:bits;
		x=havebits=0;
		while (needbits) {
			/* feed buffer */
			if (!bufbits) {
				if((*incnt)--)
					buf=_mm_read_UBYTE(reader);
				else
					buf=0;
				bufbits=8;
			}
			/* get as many bits as necessary */
			y=needbits<bufbits?needbits:bufbits;
			x|=(buf &((1<<y)-1))<<havebits;
			buf>>=y;
			bufbits-=y;
			needbits-=y;
			havebits+=y;
		}
		if (new_count) {
			new_count = 0;
			if (++x >= bits)
				x++;
			bits = x;
			continue;
		}
		if (bits<7) {
			if (x==(1<<(bits-1))) {
				new_count=1;
				continue;
			}
		}
		else if (bits<17) {
			y=(0xffff>>(17-bits))-8;
			if ((x>y)&&(x<=y+16)) {
				if ((x-=y)>=bits)
					x++;
				bits = x;
				continue;
			}
		}
		else if (bits<18) {
			if (x>=0x10000) {
				bits=x-0x10000+1;
				continue;
			}
		} else {
			 /* error in compressed data... */
			_mm_errno=MMERR_ITPACK_INVALID_DATA;
			return 0;
		}

		if (bits<16) /* extend sign */
			x = ((SWORD)(x<<(16-bits)))>>(16-bits);
		*(dest++)=(last+=x);
	}
	status->bits = bits;
	status->bufbits = bufbits;
	status->last = last;
	status->buf = buf;
	return dest-sl_buffer;
}

static BOOL SL_LoadInternal(void* buffer,UWORD infmt,UWORD outfmt,int scalefactor,ULONG length,MREADER* reader,BOOL dither)
{
	SBYTE *bptr = (SBYTE*)buffer;
	SWORD *wptr = (SWORD*)buffer;
	int stodo,t,u;

	int result,c_block=0;	/* compression bytes until next block */
	ITPACK status;
	UWORD incnt;

	while(length) {
		stodo=(length<SLBUFSIZE)?length:SLBUFSIZE;

		if(infmt&SF_ITPACKED) {
			sl_rlength=0;
			if (!c_block) {
				status.bits = (infmt & SF_16BITS) ? 17 : 9;
				status.last = status.bufbits = 0;
				incnt=_mm_read_I_UWORD(reader);
				c_block = (infmt & SF_16BITS) ? 0x4000 : 0x8000;
				if(infmt&SF_DELTA) sl_old=0;
			}
			if (infmt & SF_16BITS) {
				if(!(result=read_itcompr16(&status,reader,sl_buffer,stodo,&incnt)))
					return 1;
			} else {
				if(!(result=read_itcompr8(&status,reader,sl_buffer,stodo,&incnt)))
					return 1;
			}
			if(result!=stodo) {
				_mm_errno=MMERR_ITPACK_INVALID_DATA;
				return 1;
			}
			c_block -= stodo;
		} else {
			if(infmt&SF_16BITS) {
				if(infmt&SF_BIG_ENDIAN)
					_mm_read_M_SWORDS(sl_buffer,stodo,reader);
				else
					_mm_read_I_SWORDS(sl_buffer,stodo,reader);
			} else {
				SBYTE *src;
				SWORD *dest;

				reader->Read(reader,sl_buffer,sizeof(SBYTE)*stodo);
				src = (SBYTE*)sl_buffer;
				dest  = sl_buffer;
				src += stodo;dest += stodo;

				for(t=0;t<stodo;t++) {
					src--;dest--;
					*dest = (*src)<<8;
				}
			}
			sl_rlength-=stodo;
		}

		if(infmt & SF_DELTA)
			for(t=0;t<stodo;t++) {
				sl_buffer[t] += sl_old;
				sl_old = sl_buffer[t];
			}

		if((infmt^outfmt) & SF_SIGNED) 
			for(t=0;t<stodo;t++)
				sl_buffer[t]^= 0x8000;

		if(scalefactor) {
			int idx = 0;
			SLONG scaleval;

			/* Sample Scaling... average values for better results. */
			t= 0;
			while(t<stodo && length) {
				scaleval = 0;
				for(u=scalefactor;u && t<stodo;u--,t++)
					scaleval+=sl_buffer[t];
				sl_buffer[idx++]=scaleval/(scalefactor-u);
				length--;
			}
			stodo = idx;
		} else
			length -= stodo;

		if (dither) {
			if((infmt & SF_STEREO) && !(outfmt & SF_STEREO)) {
				/* dither stereo to mono, average together every two samples */
				SLONG avgval;
				int idx = 0;

				t=0;
				while(t<stodo && length) {
					avgval=sl_buffer[t++];
					avgval+=sl_buffer[t++];
					sl_buffer[idx++]=avgval>>1;
					length-=2;
				}
				stodo = idx;
			}
		}

		if(outfmt & SF_16BITS) {
			for(t=0;t<stodo;t++)
				*(wptr++)=sl_buffer[t];
		} else {
			for(t=0;t<stodo;t++)
				*(bptr++)=sl_buffer[t]>>8;
		}
	}
	return 0;
}

BOOL SL_Load(void* buffer,SAMPLOAD *smp,ULONG length)
{
	return SL_LoadInternal(buffer,smp->infmt,smp->outfmt,smp->scalefactor,
	                       length,smp->reader,0);
}

/* Registers a sample for loading when SL_LoadSamples() is called. */
SAMPLOAD* SL_RegisterSample(SAMPLE* s,int type,MREADER* reader)
{
	SAMPLOAD *news,**samplist,*cruise;

	if(type==MD_MUSIC) {
		samplist = &musiclist;
		cruise = musiclist;
	} else
	  if (type==MD_SNDFX) {
		samplist = &sndfxlist;
		cruise = sndfxlist;
	} else
		return NULL;
	
	/* Allocate and add structure to the END of the list */
	if(!(news=(SAMPLOAD*)_mm_malloc(sizeof(SAMPLOAD)))) return NULL;

	if(cruise) {
		while(cruise->next) cruise=cruise->next;
		cruise->next = news;
	} else
		*samplist = news;

	news->infmt     = s->flags & SF_FORMATMASK;
	news->outfmt    = news->infmt;
	news->reader    = reader;
	news->sample    = s;
	news->length    = s->length;
	news->loopstart = s->loopstart;
	news->loopend   = s->loopend;

	return news;
}

static void FreeSampleList(SAMPLOAD* s)
{
	SAMPLOAD *old;

	while(s) {
		old = s;
		s = s->next;
		free(old);
	}
}

/* Returns the total amount of memory required by the samplelist queue. */
static ULONG SampleTotal(SAMPLOAD* samplist,int type)
{
	int total = 0;

	while(samplist) {
		samplist->sample->flags=
		  (samplist->sample->flags&~SF_FORMATMASK)|samplist->outfmt;
		total += MD_SampleLength(type,samplist->sample);
		samplist=samplist->next;
	}

	return total;
}

static ULONG RealSpeed(SAMPLOAD *s)
{
	return(s->sample->speed/(s->scalefactor?s->scalefactor:1));
}    

static BOOL DitherSamples(SAMPLOAD* samplist,int type)
{
	SAMPLOAD *c2smp=NULL;
	ULONG maxsize, speed;
	SAMPLOAD *s;

	if(!samplist) return 0;

	if((maxsize=MD_SampleSpace(type)*1024)) 
		while(SampleTotal(samplist,type)>maxsize) {
			/* First Pass - check for any 16 bit samples */
			s = samplist;
			while(s) {
				if(s->outfmt & SF_16BITS) {
					SL_Sample16to8(s);
					break;
				}
				s=s->next;
			}
			/* Second pass (if no 16bits found above) is to take the sample with
			   the highest speed and dither it by half. */
			if(!s) {
				s = samplist;
				speed = 0;
				while(s) {
					if((s->sample->length) && (RealSpeed(s)>speed)) {
						speed=RealSpeed(s);
						c2smp=s;
					}
					s=s->next;
				}
				if (c2smp)
					SL_HalveSample(c2smp,2);
			}
		}

	/* Samples dithered, now load them ! */
	s = samplist;
	while(s) {
		/* sample has to be loaded ? -> increase number of samples, allocate
		   memory and load sample. */
		if(s->sample->length) {
			if(s->sample->seekpos)
				_mm_fseek(s->reader, s->sample->seekpos, SEEK_SET);

			/* Call the sample load routine of the driver module. It has to
			   return a 'handle' (>=0) that identifies the sample. */
			s->sample->handle = MD_SampleLoad(s, type);
			s->sample->flags  = (s->sample->flags & ~SF_FORMATMASK) | s->outfmt;
			if(s->sample->handle<0) {
				FreeSampleList(samplist);
				if(_mm_errorhandler) _mm_errorhandler();
				return 1;
			}
		}
		s = s->next;
	}

	FreeSampleList(samplist);
	return 0;
}

BOOL SL_LoadSamples(void)
{
	BOOL ok;

	_mm_critical = 0;

	if((!musiclist)&&(!sndfxlist)) return 0;
	ok=DitherSamples(musiclist,MD_MUSIC)||DitherSamples(sndfxlist,MD_SNDFX);
	musiclist=sndfxlist=NULL;

	return ok;
}

void SL_Sample16to8(SAMPLOAD* s)
{
	s->outfmt &= ~SF_16BITS;
	s->sample->flags = (s->sample->flags&~SF_FORMATMASK) | s->outfmt;
}

void SL_Sample8to16(SAMPLOAD* s)
{
	s->outfmt |= SF_16BITS;
	s->sample->flags = (s->sample->flags&~SF_FORMATMASK) | s->outfmt;
}

void SL_SampleSigned(SAMPLOAD* s)
{
	s->outfmt |= SF_SIGNED;
	s->sample->flags = (s->sample->flags&~SF_FORMATMASK) | s->outfmt;
}

void SL_SampleUnsigned(SAMPLOAD* s)
{
	s->outfmt &= ~SF_SIGNED;
	s->sample->flags = (s->sample->flags&~SF_FORMATMASK) | s->outfmt;
}

void SL_HalveSample(SAMPLOAD* s,int factor)
{
	s->scalefactor=factor>0?factor:2;

	s->sample->divfactor = s->scalefactor;
	s->sample->length    = s->length / s->scalefactor;
	s->sample->loopstart = s->loopstart / s->scalefactor;
	s->sample->loopend   = s->loopend / s->scalefactor;
}


/* ex:set ts=4: */
