/*	MikMod sound library
	(c) 1998, 1999, 2000, 2001, 2002 Miodrag Vallat and others - see file
	AUTHORS for complete list.

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

  Sample mixing routines, using a 32 bits mixing buffer.

==============================================================================*/

/*

  Optional features include:
    (a) 4-step reverb (for 16 bit output only)
    (b) Interpolation of sample data during mixing
    (c) Dolby Surround Sound
*/

#include "xbsection_start.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stddef.h>
#ifdef HAVE_MEMORY_H
#include <memory.h>
#endif
#include <string.h>
#include "mikmod.h"
#include "mikmod_internals.h"

/*
   Constant definitions
   ====================

  	BITSHIFT
		Controls the maximum volume of the sound output.  All data is shifted
		right by BITSHIFT after being mixed. Higher values result in quieter
		sound and less chance of distortion.

	REVERBERATION
		Controls the duration of the reverb. Larger values represent a shorter
		reverb loop. Smaller values extend the reverb but can result in more of
		an echo-ish sound.

*/

#define BITSHIFT		9
#define REVERBERATION	110000L

#define FRACBITS 11
#define FRACMASK ((1L<<FRACBITS)-1L)

#define TICKLSIZE 8192
#define TICKWSIZE (TICKLSIZE<<1)
#define TICKBSIZE (TICKWSIZE<<1)

#define CLICK_SHIFT  6
#define CLICK_BUFFER (1L<<CLICK_SHIFT)

#ifndef MIN
#define MIN(a,b) (((a)<(b)) ? (a) : (b))
#endif

typedef struct VINFO {
	UBYTE     kick;              /* =1 -> sample has to be restarted */
	UBYTE     active;            /* =1 -> sample is playing */
	UWORD     flags;             /* 16/8 bits looping/one-shot */
	SWORD     handle;            /* identifies the sample */
	ULONG     start;             /* start index */
	ULONG     size;              /* samplesize */
	ULONG     reppos;            /* loop start */
	ULONG     repend;            /* loop end */
	ULONG     frq;               /* current frequency */
	int       vol;               /* current volume */
	int       pan;               /* current panning position */

	int       rampvol;
	int       lvolsel,rvolsel;   /* Volume factor in range 0-255 */
	int       oldlvol,oldrvol;

	SLONGLONG current;           /* current index in the sample */
	SLONGLONG increment;         /* increment value */
} VINFO;

static	SWORD **Samples;
static	VINFO *vinf=NULL,*vnf;
static	long tickleft,samplesthatfit,vc_memory=0;
static	int vc_softchn;
static	SLONGLONG idxsize,idxlpos,idxlend;
static	SLONG *vc_tickbuf=NULL;
static	UWORD vc_mode;

/* Reverb control variables */

static	int RVc1, RVc2, RVc3, RVc4, RVc5, RVc6, RVc7, RVc8;
static	ULONG RVRindex;

/* For Mono or Left Channel */
static	SLONG *RVbufL1=NULL,*RVbufL2=NULL,*RVbufL3=NULL,*RVbufL4=NULL,
		      *RVbufL5=NULL,*RVbufL6=NULL,*RVbufL7=NULL,*RVbufL8=NULL;

/* For Stereo only (Right Channel) */
static	SLONG *RVbufR1=NULL,*RVbufR2=NULL,*RVbufR3=NULL,*RVbufR4=NULL,
		      *RVbufR5=NULL,*RVbufR6=NULL,*RVbufR7=NULL,*RVbufR8=NULL;

#ifdef NATIVE_64BIT_INT
#define NATIVE SLONGLONG
#else
#define NATIVE SLONG
#endif

/*========== 32 bit sample mixers - only for 32 bit platforms */
#ifndef NATIVE_64BIT_INT

static SLONG Mix32MonoNormal(SWORD* srce,SLONG* dest,SLONG index,SLONG increment,SLONG todo)
{
	SWORD sample;
	SLONG lvolsel = vnf->lvolsel;

	while(todo--) {
		sample = srce[index >> FRACBITS];
		index += increment;

		*dest++ += lvolsel * sample;
	}
	return index;
}

static SLONG Mix32StereoNormal(SWORD* srce,SLONG* dest,SLONG index,SLONG increment,SLONG todo)
{
	SWORD sample;
	SLONG lvolsel = vnf->lvolsel;
	SLONG rvolsel = vnf->rvolsel;

	while(todo--) {
		sample=srce[index >> FRACBITS];
		index += increment;

		*dest++ += lvolsel * sample;
		*dest++ += rvolsel * sample;
	}
	return index;
}
static SLONG Mix32SurroundNormal(SWORD* srce,SLONG* dest,SLONG index,SLONG increment,SLONG todo)
{
	SWORD sample;
	SLONG lvolsel = vnf->lvolsel;
	SLONG rvolsel = vnf->rvolsel;

	if (lvolsel>=rvolsel) {
		while(todo--) {
			sample = srce[index >> FRACBITS];
			index += increment;

			*dest++ += lvolsel*sample;
			*dest++ -= lvolsel*sample;
		}
	} else {
		while(todo--) {
			sample = srce[index >> FRACBITS];
			index += increment;

			*dest++ -= rvolsel*sample;
			*dest++ += rvolsel*sample;
		}
	}
	return index;
}

static SLONG Mix32MonoInterp(SWORD* srce,SLONG* dest,SLONG index,SLONG increment,SLONG todo)
{
	SLONG sample;
	SLONG lvolsel = vnf->lvolsel;
	SLONG rampvol = vnf->rampvol;

	if (rampvol) {
		SLONG oldlvol = vnf->oldlvol - lvolsel;
		while(todo--) {
			sample=(SLONG)srce[index>>FRACBITS]+
			       ((SLONG)(srce[(index>>FRACBITS)+1]-srce[index>>FRACBITS])
			        *(index&FRACMASK)>>FRACBITS);
			index += increment;

			*dest++ += ((lvolsel << CLICK_SHIFT) + oldlvol * rampvol)
			           * sample >> CLICK_SHIFT;
			if (!--rampvol)
				break;
		}
		vnf->rampvol = rampvol;
		if (todo < 0)
			return index;
	}

	while(todo--) {
		sample=(SLONG)srce[index>>FRACBITS]+
		       ((SLONG)(srce[(index>>FRACBITS)+1]-srce[index>>FRACBITS])
		        *(index&FRACMASK)>>FRACBITS);
		index += increment;

		*dest++ += lvolsel * sample;
	}
	return index;
}

static SLONG Mix32StereoInterp(SWORD* srce,SLONG* dest,SLONG index,SLONG increment,SLONG todo)
{
	SLONG sample,saveIndex,lval;
	SLONG lvolsel = vnf->lvolsel;
	SLONG rvolsel = vnf->rvolsel;
	SLONG rampvol = vnf->rampvol;

	if (rampvol) {
		SLONG oldlvol = vnf->oldlvol - lvolsel;
		SLONG oldrvol = vnf->oldrvol - rvolsel;
		while(todo--) {
			sample=(SLONG)srce[index>>FRACBITS]+
			       ((SLONG)(srce[(index>>FRACBITS)+1]-srce[index>>FRACBITS])
			        *(index&FRACMASK)>>FRACBITS);
			index += increment;

			*dest++ += ((lvolsel << CLICK_SHIFT) + oldlvol * rampvol)
			           * sample >> CLICK_SHIFT;
			*dest++ += ((rvolsel << CLICK_SHIFT) + oldrvol * rampvol)
					   * sample >> CLICK_SHIFT;
			if (!--rampvol)
				break;
		}
		vnf->rampvol = rampvol;
		if (todo < 0)
			return index;
	}
	while(todo--) {
		sample=(SLONG)srce[index>>FRACBITS]+
		       ((SLONG)(srce[(index>>FRACBITS)+1]-srce[index>>FRACBITS])
		        *(index&FRACMASK)>>FRACBITS);
		index += increment;

		*dest++ += lvolsel * sample;
		*dest++ += rvolsel * sample;
	}
	return index; 
}

static SLONG Mix32SurroundInterp(SWORD* srce,SLONG* dest,SLONG index,SLONG increment,SLONG todo)
{
	SLONG sample;
	SLONG lvolsel = vnf->lvolsel;
	SLONG rvolsel = vnf->rvolsel;
	SLONG rampvol = vnf->rampvol;
	SLONG oldvol, vol;

	if (lvolsel >= rvolsel) {
		vol = lvolsel;
		oldvol = vnf->oldlvol;
	} else {
		vol = rvolsel;
		oldvol = vnf->oldrvol;
	}

	if (rampvol) {
		oldvol -= vol;
		while(todo--) {
			sample=(SLONG)srce[index>>FRACBITS]+
			       ((SLONG)(srce[(index>>FRACBITS)+1]-srce[index>>FRACBITS])
			        *(index&FRACMASK)>>FRACBITS);
			index += increment;

			sample=((vol << CLICK_SHIFT) + oldvol * rampvol)
				   * sample >> CLICK_SHIFT;
			*dest++ += sample;
			*dest++ -= sample;

			if (!--rampvol)
				break;
		}
		vnf->rampvol = rampvol;
		if (todo < 0)
			return index;
	}

	while(todo--) {
		sample=(SLONG)srce[index>>FRACBITS]+
		       ((SLONG)(srce[(index>>FRACBITS)+1]-srce[index>>FRACBITS])
		        *(index&FRACMASK)>>FRACBITS);
		index += increment;

		*dest++ += vol*sample;
		*dest++ -= vol*sample;
	}
	return index;
}
#endif

/*========== 64 bit sample mixers - all platforms */

static SLONGLONG MixMonoNormal(SWORD* srce,SLONG* dest,SLONGLONG index,SLONGLONG increment,SLONG todo)
{
	SWORD sample;
	SLONG lvolsel = vnf->lvolsel;

	while(todo--) {
		sample = srce[index >> FRACBITS];
		index += increment;

		*dest++ += lvolsel * sample;
	}
	return index;
}

static SLONGLONG MixStereoNormal(SWORD* srce,SLONG* dest,SLONGLONG index,SLONGLONG increment,SLONG todo)
{
	SWORD sample;
	SLONG lvolsel = vnf->lvolsel;
	SLONG rvolsel = vnf->rvolsel;

	while(todo--) {
		sample=srce[index >> FRACBITS];
		index += increment;

		*dest++ += lvolsel * sample;
		*dest++ += rvolsel * sample;
	}
	return index;
}

static SLONGLONG MixSurroundNormal(SWORD* srce,SLONG* dest,SLONGLONG index,SLONGLONG increment,SLONG todo)
{
	SWORD sample;
	SLONG lvolsel = vnf->lvolsel;
	SLONG rvolsel = vnf->rvolsel;

	if(vnf->lvolsel>=vnf->rvolsel) {
		while(todo--) {
			sample = srce[index >> FRACBITS];
			index += increment;

			*dest++ += lvolsel*sample;
			*dest++ -= lvolsel*sample;
		}
	} else {
		while(todo--) {
			sample = srce[index >> FRACBITS];
			index += increment;

			*dest++ -= rvolsel*sample;
			*dest++ += rvolsel*sample;
		}
	}
	return index;
}

static SLONGLONG MixMonoInterp(SWORD* srce,SLONG* dest,SLONGLONG index,SLONGLONG increment,SLONG todo)
{
	SLONG sample;
	SLONG lvolsel = vnf->lvolsel;
	SLONG rampvol = vnf->rampvol;

	if (rampvol) {
		SLONG oldlvol = vnf->oldlvol - lvolsel;
		while(todo--) {
			sample=(SLONG)srce[index>>FRACBITS]+
			       ((SLONG)(srce[(index>>FRACBITS)+1]-srce[index>>FRACBITS])
			        *(index&FRACMASK)>>FRACBITS);
			index += increment;

			*dest++ += ((lvolsel << CLICK_SHIFT) + oldlvol * rampvol)
					   * sample >> CLICK_SHIFT;
			if (!--rampvol)
				break;
		}
		vnf->rampvol = rampvol;
		if (todo < 0)
			return index;
	}

	while(todo--) {
		sample=(SLONG)srce[index>>FRACBITS]+
		       ((SLONG)(srce[(index>>FRACBITS)+1]-srce[index>>FRACBITS])
		        *(index&FRACMASK)>>FRACBITS);
		index += increment;

		*dest++ += lvolsel * sample;
	}
	return index;
}

static SLONGLONG MixStereoInterp(SWORD* srce,SLONG* dest,SLONGLONG index,SLONGLONG increment,SLONG todo)
{
	SLONG sample;
	SLONG lvolsel = vnf->lvolsel;
	SLONG rvolsel = vnf->rvolsel;
	SLONG rampvol = vnf->rampvol;

	if (rampvol) {
		SLONG oldlvol = vnf->oldlvol - lvolsel;
		SLONG oldrvol = vnf->oldrvol - rvolsel;
		while(todo--) {
			sample=(SLONG)srce[index>>FRACBITS]+
			       ((SLONG)(srce[(index>>FRACBITS)+1]-srce[index>>FRACBITS])
			        *(index&FRACMASK)>>FRACBITS);
			index += increment;

			*dest++ +=((lvolsel << CLICK_SHIFT) + oldlvol * rampvol)
					   * sample >> CLICK_SHIFT;
			*dest++ +=((rvolsel << CLICK_SHIFT) + oldrvol * rampvol)
					   * sample >> CLICK_SHIFT;
			if (!--rampvol)
				break;
		}
		vnf->rampvol = rampvol;
		if (todo < 0)
			return index;
	}

	while(todo--) {
		sample=(SLONG)srce[index>>FRACBITS]+
		       ((SLONG)(srce[(index>>FRACBITS)+1]-srce[index>>FRACBITS])
		        *(index&FRACMASK)>>FRACBITS);
		index += increment;

		*dest++ += lvolsel * sample;
		*dest++ += rvolsel * sample;
	}
	return index;
}

static SLONGLONG MixSurroundInterp(SWORD* srce,SLONG* dest,SLONGLONG index,SLONGLONG increment,SLONG todo)
{
	SLONG sample;
	SLONG lvolsel = vnf->lvolsel;
	SLONG rvolsel = vnf->rvolsel;
	SLONG rampvol = vnf->rampvol;
	SLONG oldvol, vol;

	if (lvolsel >= rvolsel) {
		vol = lvolsel;
		oldvol = vnf->oldlvol;
	} else {
		vol = rvolsel;
		oldvol = vnf->oldrvol;
	}

	if (rampvol) {
		oldvol -= vol;
		while(todo--) {
			sample=(SLONG)srce[index>>FRACBITS]+
			       ((SLONG)(srce[(index>>FRACBITS)+1]-srce[index>>FRACBITS])
			        *(index&FRACMASK)>>FRACBITS);
			index += increment;

			sample=((vol << CLICK_SHIFT) + oldvol * rampvol)
				   * sample >> CLICK_SHIFT;
			*dest++ += sample;
			*dest++ -= sample;
			if (!--rampvol)
				break;
		}
		vnf->rampvol = rampvol;
		if (todo < 0)
			return index;
	}

	while(todo--) {
		sample=(SLONG)srce[index>>FRACBITS]+
		       ((SLONG)(srce[(index>>FRACBITS)+1]-srce[index>>FRACBITS])
		        *(index&FRACMASK)>>FRACBITS);
		index += increment;

		*dest++ += vol*sample;
		*dest++ -= vol*sample;
	}
	return index;
}

static void (*MixReverb)(SLONG* srce,NATIVE count);

/* Reverb macros */
#define COMPUTE_LOC(n) loc##n = RVRindex % RVc##n
#define COMPUTE_LECHO(n) RVbufL##n [loc##n ]=speedup+((ReverbPct*RVbufL##n [loc##n ])>>7)
#define COMPUTE_RECHO(n) RVbufR##n [loc##n ]=speedup+((ReverbPct*RVbufR##n [loc##n ])>>7)

static void MixReverb_Normal(SLONG* srce,NATIVE count)
{
	unsigned int speedup;
	int ReverbPct;
	unsigned int loc1,loc2,loc3,loc4;
	unsigned int loc5,loc6,loc7,loc8;

	ReverbPct=58+(md_reverb<<2);

	COMPUTE_LOC(1); COMPUTE_LOC(2); COMPUTE_LOC(3); COMPUTE_LOC(4);
	COMPUTE_LOC(5); COMPUTE_LOC(6); COMPUTE_LOC(7); COMPUTE_LOC(8);

	while(count--) {
		/* Compute the left channel echo buffers */
		speedup = *srce >> 3;

		COMPUTE_LECHO(1); COMPUTE_LECHO(2); COMPUTE_LECHO(3); COMPUTE_LECHO(4);
		COMPUTE_LECHO(5); COMPUTE_LECHO(6); COMPUTE_LECHO(7); COMPUTE_LECHO(8);

		/* Prepare to compute actual finalized data */
		RVRindex++;

		COMPUTE_LOC(1); COMPUTE_LOC(2); COMPUTE_LOC(3); COMPUTE_LOC(4);
		COMPUTE_LOC(5); COMPUTE_LOC(6); COMPUTE_LOC(7); COMPUTE_LOC(8);

		/* left channel */
		*srce++ +=RVbufL1[loc1]-RVbufL2[loc2]+RVbufL3[loc3]-RVbufL4[loc4]+
		          RVbufL5[loc5]-RVbufL6[loc6]+RVbufL7[loc7]-RVbufL8[loc8];
	}
}

static void MixReverb_Stereo(SLONG* srce,NATIVE count)
{
	unsigned int speedup;
	int          ReverbPct;
	unsigned int loc1, loc2, loc3, loc4;
	unsigned int loc5, loc6, loc7, loc8;

	ReverbPct = 92+(md_reverb<<1);

	COMPUTE_LOC(1); COMPUTE_LOC(2); COMPUTE_LOC(3); COMPUTE_LOC(4);
	COMPUTE_LOC(5); COMPUTE_LOC(6); COMPUTE_LOC(7); COMPUTE_LOC(8);

	while(count--) {
		/* Compute the left channel echo buffers */
		speedup = *srce >> 3;

		COMPUTE_LECHO(1); COMPUTE_LECHO(2); COMPUTE_LECHO(3); COMPUTE_LECHO(4);
		COMPUTE_LECHO(5); COMPUTE_LECHO(6); COMPUTE_LECHO(7); COMPUTE_LECHO(8);

		/* Compute the right channel echo buffers */
		speedup = srce[1] >> 3;

		COMPUTE_RECHO(1); COMPUTE_RECHO(2); COMPUTE_RECHO(3); COMPUTE_RECHO(4);
		COMPUTE_RECHO(5); COMPUTE_RECHO(6); COMPUTE_RECHO(7); COMPUTE_RECHO(8);

		/* Prepare to compute actual finalized data */
		RVRindex++;

		COMPUTE_LOC(1); COMPUTE_LOC(2); COMPUTE_LOC(3); COMPUTE_LOC(4);
		COMPUTE_LOC(5); COMPUTE_LOC(6); COMPUTE_LOC(7); COMPUTE_LOC(8);

		/* left channel then right channel */
		*srce++ +=RVbufL1[loc1]-RVbufL2[loc2]+RVbufL3[loc3]-RVbufL4[loc4]+
		          RVbufL5[loc5]-RVbufL6[loc6]+RVbufL7[loc7]-RVbufL8[loc8];

		*srce++ +=RVbufR1[loc1]-RVbufR2[loc2]+RVbufR3[loc3]-RVbufR4[loc4]+
		          RVbufR5[loc5]-RVbufR6[loc6]+RVbufR7[loc7]-RVbufR8[loc8];
	}
}

/* Mixing macros */
#define EXTRACT_SAMPLE(var,size) var=*srce++>>(BITSHIFT+16-size)
#define CHECK_SAMPLE(var,bound) var=(var>=bound)?bound-1:(var<-bound)?-bound:var
#define PUT_SAMPLE(var) *dste++=var

static void Mix32To16(SWORD* dste,SLONG* srce,NATIVE count)
{
	SLONG x1,x2,x3,x4;
	int	remain;

	remain=count&3;
	for(count>>=2;count;count--) {
		EXTRACT_SAMPLE(x1,16); EXTRACT_SAMPLE(x2,16);
		EXTRACT_SAMPLE(x3,16); EXTRACT_SAMPLE(x4,16);

		CHECK_SAMPLE(x1,32768); CHECK_SAMPLE(x2,32768);
		CHECK_SAMPLE(x3,32768); CHECK_SAMPLE(x4,32768);

		PUT_SAMPLE(x1); PUT_SAMPLE(x2); PUT_SAMPLE(x3); PUT_SAMPLE(x4);
	}
	while(remain--) {
		EXTRACT_SAMPLE(x1,16);
		CHECK_SAMPLE(x1,32768);
		PUT_SAMPLE(x1);
	}
}

static void Mix32To8(SBYTE* dste,SLONG* srce,NATIVE count)
{
	SWORD x1,x2,x3,x4;
	int	remain;

	remain=count&3;
	for(count>>=2;count;count--) {
		EXTRACT_SAMPLE(x1,8); EXTRACT_SAMPLE(x2,8);
		EXTRACT_SAMPLE(x3,8); EXTRACT_SAMPLE(x4,8);

		CHECK_SAMPLE(x1,128); CHECK_SAMPLE(x2,128);
		CHECK_SAMPLE(x3,128); CHECK_SAMPLE(x4,128);

		PUT_SAMPLE(x1+128); PUT_SAMPLE(x2+128);
		PUT_SAMPLE(x3+128); PUT_SAMPLE(x4+128);
	}
	while(remain--) {
		EXTRACT_SAMPLE(x1,8);
		CHECK_SAMPLE(x1,128);
		PUT_SAMPLE(x1+128);
	}
}

static void AddChannel(SLONG* ptr,NATIVE todo)
{
	SLONGLONG end,done;
	SWORD *s;

	if(!(s=Samples[vnf->handle])) {
		vnf->current = vnf->active  = 0;
		return;
	}

	/* update the 'current' index so the sample loops, or stops playing if it
	   reached the end of the sample */
	while(todo>0) {
		SLONGLONG endpos;

		if(vnf->flags & SF_REVERSE) {
			/* The sample is playing in reverse */
			if((vnf->flags&SF_LOOP)&&(vnf->current<idxlpos)) {
				/* the sample is looping and has reached the loopstart index */
				if(vnf->flags & SF_BIDI) {
					/* sample is doing bidirectional loops, so 'bounce' the
					   current index against the idxlpos */
					vnf->current = idxlpos+(idxlpos-vnf->current);
					vnf->flags &= ~SF_REVERSE;
					vnf->increment = -vnf->increment;
				} else
					/* normal backwards looping, so set the current position to
					   loopend index */
					vnf->current=idxlend-(idxlpos-vnf->current);
			} else {
				/* the sample is not looping, so check if it reached index 0 */
				if(vnf->current < 0) {
					/* playing index reached 0, so stop playing this sample */
					vnf->current = vnf->active  = 0;
					break;
				}
			}
		} else {
			/* The sample is playing forward */
			if((vnf->flags & SF_LOOP) &&
			   (vnf->current >= idxlend)) {
				/* the sample is looping, check the loopend index */
				if(vnf->flags & SF_BIDI) {
					/* sample is doing bidirectional loops, so 'bounce' the
					   current index against the idxlend */
					vnf->flags |= SF_REVERSE;
					vnf->increment = -vnf->increment;
					vnf->current = idxlend-(vnf->current-idxlend);
				} else
					/* normal backwards looping, so set the current position
					   to loopend index */
					vnf->current=idxlpos+(vnf->current-idxlend);
			} else {
				/* sample is not looping, so check if it reached the last
				   position */
				if(vnf->current >= idxsize) {
					/* yes, so stop playing this sample */
					vnf->current = vnf->active  = 0;
					break;
				}
			}
		}

		end=(vnf->flags&SF_REVERSE)?(vnf->flags&SF_LOOP)?idxlpos:0:
		     (vnf->flags&SF_LOOP)?idxlend:idxsize;

		/* if the sample is not blocked... */
		if((end==vnf->current)||(!vnf->increment))
			done=0;
		else {
			done=MIN((end-vnf->current)/vnf->increment+1,todo);
			if(done<0) done=0;
		}

		if(!done) {
			vnf->active = 0;
			break;
		}

		endpos=vnf->current+done*vnf->increment;

		if(vnf->vol) {
#ifndef NATIVE_64BIT_INT
			/* use the 32 bit mixers as often as we can (they're much faster) */
			if((vnf->current<0x7fffffff)&&(endpos<0x7fffffff)) {
				if((md_mode & DMODE_INTERP)) {
					if(vc_mode & DMODE_STEREO) {
						if((vnf->pan==PAN_SURROUND)&&(md_mode&DMODE_SURROUND))
							vnf->current=Mix32SurroundInterp
							           (s,ptr,vnf->current,vnf->increment,done);
						else
							vnf->current=Mix32StereoInterp
							           (s,ptr,vnf->current,vnf->increment,done);
					} else
						vnf->current=Mix32MonoInterp
						               (s,ptr,vnf->current,vnf->increment,done);
				} else if(vc_mode & DMODE_STEREO) {
					if((vnf->pan==PAN_SURROUND)&&(md_mode&DMODE_SURROUND))
						vnf->current=Mix32SurroundNormal
						               (s,ptr,vnf->current,vnf->increment,done);
					else
						vnf->current=Mix32StereoNormal
						               (s,ptr,vnf->current,vnf->increment,done);
				} else
					vnf->current=Mix32MonoNormal
					                   (s,ptr,vnf->current,vnf->increment,done);
			} else
#endif
			       {
				if((md_mode & DMODE_INTERP)) {
					if(vc_mode & DMODE_STEREO) {
						if((vnf->pan==PAN_SURROUND)&&(md_mode&DMODE_SURROUND))
							vnf->current=MixSurroundInterp
							           (s,ptr,vnf->current,vnf->increment,done);
						else
							vnf->current=MixStereoInterp
							           (s,ptr,vnf->current,vnf->increment,done);
					} else
						vnf->current=MixMonoInterp
						               (s,ptr,vnf->current,vnf->increment,done);
				} else if(vc_mode & DMODE_STEREO) {
					if((vnf->pan==PAN_SURROUND)&&(md_mode&DMODE_SURROUND))
						vnf->current=MixSurroundNormal
						               (s,ptr,vnf->current,vnf->increment,done);
					else
						vnf->current=MixStereoNormal
						               (s,ptr,vnf->current,vnf->increment,done);
				} else
					vnf->current=MixMonoNormal
					                   (s,ptr,vnf->current,vnf->increment,done);
			}
		} else
			/* update sample position */
			vnf->current=endpos;

		todo-=done;
		ptr +=(vc_mode & DMODE_STEREO)?(done<<1):done;
	}
}

#define _IN_VIRTCH_
#include "virtch_common.c"
#undef _IN_VIRTCH_

void VC1_WriteSamples(SBYTE* buf,ULONG todo)
{
	int left,portion=0,count;
	SBYTE  *buffer;
	int t, pan, vol;

	while(todo) {
		if(!tickleft) {
			if(vc_mode & DMODE_SOFT_MUSIC) md_player();
			tickleft=(md_mixfreq*125L)/(md_bpm*50L);
		}
		left = MIN(tickleft, todo);
		buffer    = buf;
		tickleft -= left;
		todo     -= left;
		buf += samples2bytes(left);

		while(left) {
			portion = MIN(left, samplesthatfit);
			count   = (vc_mode & DMODE_STEREO)?(portion<<1):portion;
			memset(vc_tickbuf, 0, count<<2);
			for(t=0;t<vc_softchn;t++) {
				vnf = &vinf[t];

				if(vnf->kick) {
					vnf->current=((SLONGLONG)vnf->start)<<FRACBITS;
					vnf->kick   =0;
					vnf->active =1;
				}

				if(!vnf->frq) vnf->active = 0;

				if(vnf->active) {
					vnf->increment=((SLONGLONG)(vnf->frq<<FRACBITS))/md_mixfreq;
					if(vnf->flags&SF_REVERSE) vnf->increment=-vnf->increment;
					vol = vnf->vol;  pan = vnf->pan;

					vnf->oldlvol=vnf->lvolsel;vnf->oldrvol=vnf->rvolsel;
					if(vc_mode & DMODE_STEREO) {
						if(pan != PAN_SURROUND) {
							vnf->lvolsel=(vol*(PAN_RIGHT-pan))>>8;
							vnf->rvolsel=(vol*pan)>>8;
						} else
							vnf->lvolsel=vnf->rvolsel=vol/2;
					} else
						vnf->lvolsel=vol;

					idxsize = (vnf->size)? ((SLONGLONG)vnf->size << FRACBITS)-1 : 0;
					idxlend = (vnf->repend)? ((SLONGLONG)vnf->repend << FRACBITS)-1 : 0;
					idxlpos = (SLONGLONG)vnf->reppos << FRACBITS;
					AddChannel(vc_tickbuf, portion);
				}
			}

			if(md_reverb) {
				if(md_reverb>15) md_reverb=15;
				MixReverb(vc_tickbuf, portion);
			}

			if(vc_mode & DMODE_16BITS)
				Mix32To16((SWORD*) buffer, vc_tickbuf, count);
			else
				Mix32To8((SBYTE*) buffer, vc_tickbuf, count);

			buffer += samples2bytes(portion);
			left   -= portion;
		}
	}
}

BOOL VC1_Init(void)
{
	VC_SetupPointers();
	
	if (md_mode&DMODE_HQMIXER)
		return VC2_Init();

	if(!(Samples=(SWORD**)_mm_calloc(MAXSAMPLEHANDLES,sizeof(SWORD*)))) {
		_mm_errno = MMERR_INITIALIZING_MIXER;
		return 1;
	}
	if(!vc_tickbuf)
		if(!(vc_tickbuf=(SLONG*)_mm_malloc((TICKLSIZE+32)*sizeof(SLONG)))) {
			_mm_errno = MMERR_INITIALIZING_MIXER;
			return 1;
		}

	MixReverb=(md_mode&DMODE_STEREO)?MixReverb_Stereo:MixReverb_Normal;
	vc_mode = md_mode;
	return 0;
}

BOOL VC1_PlayStart(void)
{
	samplesthatfit=TICKLSIZE;
	if(vc_mode & DMODE_STEREO) samplesthatfit >>= 1;
	tickleft = 0;

	RVc1 = (5000L * md_mixfreq) / REVERBERATION;
	RVc2 = (5078L * md_mixfreq) / REVERBERATION;
	RVc3 = (5313L * md_mixfreq) / REVERBERATION;
	RVc4 = (5703L * md_mixfreq) / REVERBERATION;
	RVc5 = (6250L * md_mixfreq) / REVERBERATION;
	RVc6 = (6953L * md_mixfreq) / REVERBERATION;
	RVc7 = (7813L * md_mixfreq) / REVERBERATION;
	RVc8 = (8828L * md_mixfreq) / REVERBERATION;

	if(!(RVbufL1=(SLONG*)_mm_calloc((RVc1+1),sizeof(SLONG)))) return 1;
	if(!(RVbufL2=(SLONG*)_mm_calloc((RVc2+1),sizeof(SLONG)))) return 1;
	if(!(RVbufL3=(SLONG*)_mm_calloc((RVc3+1),sizeof(SLONG)))) return 1;
	if(!(RVbufL4=(SLONG*)_mm_calloc((RVc4+1),sizeof(SLONG)))) return 1;
	if(!(RVbufL5=(SLONG*)_mm_calloc((RVc5+1),sizeof(SLONG)))) return 1;
	if(!(RVbufL6=(SLONG*)_mm_calloc((RVc6+1),sizeof(SLONG)))) return 1;
	if(!(RVbufL7=(SLONG*)_mm_calloc((RVc7+1),sizeof(SLONG)))) return 1;
	if(!(RVbufL8=(SLONG*)_mm_calloc((RVc8+1),sizeof(SLONG)))) return 1;

	if(!(RVbufR1=(SLONG*)_mm_calloc((RVc1+1),sizeof(SLONG)))) return 1;
	if(!(RVbufR2=(SLONG*)_mm_calloc((RVc2+1),sizeof(SLONG)))) return 1;
	if(!(RVbufR3=(SLONG*)_mm_calloc((RVc3+1),sizeof(SLONG)))) return 1;
	if(!(RVbufR4=(SLONG*)_mm_calloc((RVc4+1),sizeof(SLONG)))) return 1;
	if(!(RVbufR5=(SLONG*)_mm_calloc((RVc5+1),sizeof(SLONG)))) return 1;
	if(!(RVbufR6=(SLONG*)_mm_calloc((RVc6+1),sizeof(SLONG)))) return 1;
	if(!(RVbufR7=(SLONG*)_mm_calloc((RVc7+1),sizeof(SLONG)))) return 1;
	if(!(RVbufR8=(SLONG*)_mm_calloc((RVc8+1),sizeof(SLONG)))) return 1;

	RVRindex = 0;
	return 0;
}

void VC1_PlayStop(void)
{
	if(RVbufL1) free(RVbufL1);
	if(RVbufL2) free(RVbufL2);
	if(RVbufL3) free(RVbufL3);
	if(RVbufL4) free(RVbufL4);
	if(RVbufL5) free(RVbufL5);
	if(RVbufL6) free(RVbufL6);
	if(RVbufL7) free(RVbufL7);
	if(RVbufL8) free(RVbufL8);
	RVbufL1=RVbufL2=RVbufL3=RVbufL4=RVbufL5=RVbufL6=RVbufL7=RVbufL8=NULL;
	if(RVbufR1) free(RVbufR1);
	if(RVbufR2) free(RVbufR2);
	if(RVbufR3) free(RVbufR3);
	if(RVbufR4) free(RVbufR4);
	if(RVbufR5) free(RVbufR5);
	if(RVbufR6) free(RVbufR6);
	if(RVbufR7) free(RVbufR7);
	if(RVbufR8) free(RVbufR8);
	RVbufR1=RVbufR2=RVbufR3=RVbufR4=RVbufR5=RVbufR6=RVbufR7=RVbufR8=NULL;
}

BOOL VC1_SetNumVoices(void)
{
	int t;

	if(!(vc_softchn=md_softchn)) return 0;

	if(vinf) free(vinf);
	if(!(vinf= _mm_calloc(sizeof(VINFO),vc_softchn))) return 1;

	for(t=0;t<vc_softchn;t++) {
		vinf[t].frq=10000;
		vinf[t].pan=(t&1)?PAN_LEFT:PAN_RIGHT;
	}

	return 0;
}

/* ex:set ts=4: */
