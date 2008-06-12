/*	MikMod sound library
	(c) 1998, 1999, 2000 Miodrag Vallat and others - see file AUTHORS for
	complete list.

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

  High-quality sample mixing routines, using a 32 bits mixing buffer,
  interpolation, and sample smoothing to improve sound quality and remove
  clicks.

==============================================================================*/

/*

  Future Additions:
	Low-Pass filter to remove annoying staticy buzz.

*/

#include "xbsection_start.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_MEMORY_H
#include <memory.h>
#endif
#include <string.h>
#include "mikmod.h"
#include "mikmod_internals.h"
  
/*
   Constant Definitions
   ====================

	MAXVOL_FACTOR (was BITSHIFT in virtch.c)
		Controls the maximum volume of the output data. All mixed data is
		divided by this number after mixing, so larger numbers result in
		quieter mixing.  Smaller numbers will increase the likeliness of
		distortion on loud modules.

	REVERBERATION
		Larger numbers result in shorter reverb duration. Longer reverb
		durations can cause unwanted static and make the reverb sound more
		like a crappy echo.

	SAMPLING_SHIFT
		Specified the shift multiplier which controls by how much the mixing
		rate is multiplied while mixing.  Higher values can improve quality by
		smoothing the sound and reducing pops and clicks. Note, this is a shift
		value, so a value of 2 becomes a mixing-rate multiplier of 4, and a
		value of 3 = 8, etc.

	FRACBITS
		The number of bits per integer devoted to the fractional part of the
		number. Generally, this number should not be changed for any reason.

	!!! IMPORTANT !!! All values below MUST ALWAYS be greater than 0

*/

#define MAXVOL_FACTOR (1<<9)
#define	REVERBERATION 11000L

#define SAMPLING_SHIFT 2
#define SAMPLING_FACTOR (1UL<<SAMPLING_SHIFT)

#define	FRACBITS 28
#define FRACMASK ((1UL<<FRACBITS)-1UL)

#define TICKLSIZE 8192
#define TICKWSIZE (TICKLSIZE * 2)
#define TICKBSIZE (TICKWSIZE * 2)

#define CLICK_SHIFT_BASE 6
#define CLICK_SHIFT (CLICK_SHIFT_BASE + SAMPLING_SHIFT)
#define CLICK_BUFFER (1L << CLICK_SHIFT)

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

	int       click;
	int       rampvol;
	SLONG     lastvalL,lastvalR;
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
	SWORD sample=0;
	SLONG i,f;

	while(todo--) {
		i=index>>FRACBITS,f=index&FRACMASK;
		sample=(((SLONG)(srce[i]*(FRACMASK+1L-f)) +
		        ((SLONG)srce[i+1]*f)) >> FRACBITS);
		index+=increment;

		if(vnf->rampvol) {
			*dest++ += (long)(
			  ( ( (SLONG)(vnf->oldlvol*vnf->rampvol) +
			      (vnf->lvolsel*(CLICK_BUFFER-vnf->rampvol)) ) *
			    (SLONG)sample ) >> CLICK_SHIFT );
			vnf->rampvol--;
		} else
		  if(vnf->click) {
			*dest++ += (long)(
			  ( ( ((SLONG)vnf->lvolsel*(CLICK_BUFFER-vnf->click)) *
			      (SLONG)sample ) +
			    (vnf->lastvalL*vnf->click) ) >> CLICK_SHIFT );
			vnf->click--;
		} else
			*dest++ +=vnf->lvolsel*sample;
	}
	vnf->lastvalL=vnf->lvolsel * sample;

	return index;
}

static SLONG Mix32StereoNormal(SWORD* srce,SLONG* dest,SLONG index,SLONG increment,ULONG todo)
{
	SWORD sample=0;
	SLONG i,f;
#if 0
	while(todo--) {
		i=index>>FRACBITS,f=index&FRACMASK;
		sample=((((SLONG)srce[i]*(FRACMASK+1L-f)) +
		        ((SLONG)srce[i+1] * f)) >> FRACBITS);
		index += increment;

		/*
		if(vnf->rampvol) {
			*dest++ += (long)(
			  ( ( ((SLONG)vnf->oldlvol*vnf->rampvol) +
			      (vnf->lvolsel*(CLICK_BUFFER-vnf->rampvol))
			    ) * (SLONG)sample ) >> CLICK_SHIFT );
			*dest++ += (long)(
			  ( ( ((SLONG)vnf->oldrvol*vnf->rampvol) +
			      (vnf->rvolsel*(CLICK_BUFFER-vnf->rampvol))
			    ) * (SLONG)sample ) >> CLICK_SHIFT );
			vnf->rampvol--;
		} else
		  if(vnf->click) {
			*dest++ += (long)(
			  ( ( (SLONG)(vnf->lvolsel*(CLICK_BUFFER-vnf->click)) *
			      (SLONG)sample ) + (vnf->lastvalL * vnf->click) )
			    >> CLICK_SHIFT );
			*dest++ += (long)(
			  ( ( ((SLONG)vnf->rvolsel*(CLICK_BUFFER-vnf->click)) *
			      (SLONG)sample ) + (vnf->lastvalR * vnf->click) )
			    >> CLICK_SHIFT );
			vnf->click--;
		} else {
			*dest++ +=vnf->lvolsel*sample;
			*dest++ +=vnf->rvolsel*sample;
		}
		*/
		*dest++ +=vnf->lvolsel*sample;
		*dest++ +=vnf->rvolsel*sample;

	}
	vnf->lastvalL=vnf->lvolsel*sample;
	vnf->lastvalR=vnf->rvolsel*sample;
#else

	return index;

	_asm{
		Mov Ecx,todo
		Mov Ebx,increment
		Mov	Eax,index
		Mov	Edi,dest
looping:
		Mov	Esi,srce
		Push Eax			;save index.
		Push Ebx			;save increment.
		Mov Edx,Eax
		Mov	Ebx,Eax
		And Edx,FRACMASK	;f
		Sar Ebx,FRACBITS	;i
		Movd mm3,Edx
		Mov Eax,FRACMASK
		Movd mm0,[Esi+Ebx*4]
		Inc Eax
		Inc Ebx
		Sub Eax,Edx
		Movd mm1,Eax
		Movd mm2,[Esi+Ebx*4]
		Pmullw mm0,mm1
		Pmullw mm2,mm3
		Paddw mm0,mm2		;sample
		Pop Eax
		Pop Ebx
		Mov Esi,vnf
		Add Eax,Ebx

		Movd mm1,[Esi].lvolsel
		Movd mm2,[Esi].rvolsel
		Pmullw mm1,mm0
		Movd [Edi],mm1
		Pmullw mm2,mm0
		Movd [Edi+4],mm2

		Add	Edi,8
		Dec Ecx
		Jnz looping

		
		Movd [Esi].lastvalL,mm1
		Movd [Esi].lastvalR,mm2
		Mov index,Eax

		emms

	}

#endif

	return index;

}

static SLONG Mix32StereoSurround(SWORD* srce,SLONG* dest,SLONG index,SLONG increment,ULONG todo)
{
	SWORD sample=0;
	long whoop;
	SLONG i, f;

	while(todo--) {
		i=index>>FRACBITS,f=index&FRACMASK;
		sample=((((SLONG)srce[i]*(FRACMASK+1L-f)) +
		        ((SLONG)srce[i+1]*f)) >> FRACBITS);
		index+=increment;

		if(vnf->rampvol) {
			whoop=(long)(
			  ( ( (SLONG)(vnf->oldlvol*vnf->rampvol) +
			      (vnf->lvolsel*(CLICK_BUFFER-vnf->rampvol)) ) *
			    (SLONG)sample) >> CLICK_SHIFT );
			*dest++ +=whoop;
			*dest++ -=whoop;
			vnf->rampvol--;
		} else
		  if(vnf->click) {
			whoop = (long)(
			  ( ( ((SLONG)vnf->lvolsel*(CLICK_BUFFER-vnf->click)) *
			      (SLONG)sample) +
			    (vnf->lastvalL * vnf->click) ) >> CLICK_SHIFT );
			*dest++ +=whoop;
			*dest++ -=whoop;
			vnf->click--;
		} else {
			*dest++ +=vnf->lvolsel*sample;
			*dest++ -=vnf->lvolsel*sample;
		}
	}
	vnf->lastvalL=vnf->lvolsel*sample;
	vnf->lastvalR=vnf->lvolsel*sample;

	return index;
}
#endif

/*========== 64 bit mixers */

static SLONGLONG MixMonoNormal(SWORD* srce,SLONG* dest,SLONGLONG index,SLONGLONG increment,SLONG todo)
{
	SWORD sample=0;
	SLONGLONG i,f;

	while(todo--) {
		i=index>>FRACBITS,f=index&FRACMASK;
		sample=(((SLONGLONG)(srce[i]*(FRACMASK+1L-f)) +
		        ((SLONGLONG)srce[i+1]*f)) >> FRACBITS);
		index+=increment;

		if(vnf->rampvol) {
			*dest++ += (long)(
			  ( ( (SLONGLONG)(vnf->oldlvol*vnf->rampvol) +
			      (vnf->lvolsel*(CLICK_BUFFER-vnf->rampvol)) ) *
			    (SLONGLONG)sample ) >> CLICK_SHIFT );
			vnf->rampvol--;
		} else
		  if(vnf->click) {
			*dest++ += (long)(
			  ( ( ((SLONGLONG)vnf->lvolsel*(CLICK_BUFFER-vnf->click)) *
			      (SLONGLONG)sample ) +
			    (vnf->lastvalL*vnf->click) ) >> CLICK_SHIFT );
			vnf->click--;
		} else
			*dest++ +=vnf->lvolsel*sample;
	}
	vnf->lastvalL=vnf->lvolsel * sample;

	return index;
}

static SLONGLONG MixStereoNormal(SWORD* srce,SLONG* dest,SLONGLONG index,SLONGLONG increment,ULONG todo)
{
	SWORD sample=0;
	SLONGLONG i,f;

	while(todo--) {
		i=index>>FRACBITS,f=index&FRACMASK;
		sample=((((SLONGLONG)srce[i]*(FRACMASK+1L-f)) +
		        ((SLONGLONG)srce[i+1] * f)) >> FRACBITS);
		index += increment;

		if(vnf->rampvol) {
			*dest++ += (long)(
			  ( ( ((SLONGLONG)vnf->oldlvol*vnf->rampvol) +
			      (vnf->lvolsel*(CLICK_BUFFER-vnf->rampvol))
			    ) * (SLONGLONG)sample ) >> CLICK_SHIFT );
			*dest++ += (long)(
			  ( ( ((SLONGLONG)vnf->oldrvol*vnf->rampvol) +
			      (vnf->rvolsel*(CLICK_BUFFER-vnf->rampvol))
			    ) * (SLONGLONG)sample ) >> CLICK_SHIFT );
			vnf->rampvol--;
		} else
		  if(vnf->click) {
			*dest++ += (long)(
			  ( ( (SLONGLONG)(vnf->lvolsel*(CLICK_BUFFER-vnf->click)) *
			      (SLONGLONG)sample ) + (vnf->lastvalL * vnf->click) )
			    >> CLICK_SHIFT );
			*dest++ += (long)(
			  ( ( ((SLONGLONG)vnf->rvolsel*(CLICK_BUFFER-vnf->click)) *
			      (SLONGLONG)sample ) + (vnf->lastvalR * vnf->click) )
			    >> CLICK_SHIFT );
			vnf->click--;
		} else {
			*dest++ +=vnf->lvolsel*sample;
			*dest++ +=vnf->rvolsel*sample;
		}
	}
	vnf->lastvalL=vnf->lvolsel*sample;
	vnf->lastvalR=vnf->rvolsel*sample;

	return index;
}

static SLONGLONG MixStereoSurround(SWORD* srce,SLONG* dest,SLONGLONG index,SLONGLONG increment,ULONG todo)
{
	SWORD sample=0;
	long whoop;
	SLONGLONG i, f;

	while(todo--) {
		i=index>>FRACBITS,f=index&FRACMASK;
		sample=((((SLONGLONG)srce[i]*(FRACMASK+1L-f)) +
		        ((SLONGLONG)srce[i+1]*f)) >> FRACBITS);
		index+=increment;

		if(vnf->rampvol) {
			whoop=(long)(
			  ( ( (SLONGLONG)(vnf->oldlvol*vnf->rampvol) +
			      (vnf->lvolsel*(CLICK_BUFFER-vnf->rampvol)) ) *
			    (SLONGLONG)sample) >> CLICK_SHIFT );
			*dest++ +=whoop;
			*dest++ -=whoop;
			vnf->rampvol--;
		} else
		  if(vnf->click) {
			whoop = (long)(
			  ( ( ((SLONGLONG)vnf->lvolsel*(CLICK_BUFFER-vnf->click)) *
			      (SLONGLONG)sample) +
			    (vnf->lastvalL * vnf->click) ) >> CLICK_SHIFT );
			*dest++ +=whoop;
			*dest++ -=whoop;
			vnf->click--;
		} else {
			*dest++ +=vnf->lvolsel*sample;
			*dest++ -=vnf->lvolsel*sample;
		}
	}
	vnf->lastvalL=vnf->lvolsel*sample;
	vnf->lastvalR=vnf->lvolsel*sample;

	return index;
}

static	void(*Mix32to16)(SWORD* dste,SLONG* srce,NATIVE count);
static	void(*Mix32to8)(SBYTE* dste,SLONG* srce,NATIVE count);
static	void(*MixReverb)(SLONG* srce,NATIVE count);

/* Reverb macros */
#define COMPUTE_LOC(n) loc##n = RVRindex % RVc##n
#define COMPUTE_LECHO(n) RVbufL##n [loc##n ]=speedup+((ReverbPct*RVbufL##n [loc##n ])>>7)
#define COMPUTE_RECHO(n) RVbufR##n [loc##n ]=speedup+((ReverbPct*RVbufR##n [loc##n ])>>7)

static void MixReverb_Normal(SLONG* srce,NATIVE count)
{
	NATIVE speedup;
	int ReverbPct;
	unsigned int loc1,loc2,loc3,loc4,loc5,loc6,loc7,loc8;

	ReverbPct=58+(md_reverb*4);

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

static void MixReverb_Stereo(SLONG *srce,NATIVE count)
{
	NATIVE speedup;
	int ReverbPct;
	unsigned int loc1,loc2,loc3,loc4,loc5,loc6,loc7,loc8;

	ReverbPct=58+(md_reverb*4);

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

		/* left channel */
		*srce++ +=RVbufL1[loc1]-RVbufL2[loc2]+RVbufL3[loc3]-RVbufL4[loc4]+ 
		          RVbufL5[loc5]-RVbufL6[loc6]+RVbufL7[loc7]-RVbufL8[loc8];

		/* right channel */
		*srce++ +=RVbufR1[loc1]-RVbufR2[loc2]+RVbufR3[loc3]-RVbufR4[loc4]+
		          RVbufR5[loc5]-RVbufR6[loc6]+RVbufR7[loc7]-RVbufR8[loc8];
	}
}

/* Mixing macros */
#define EXTRACT_SAMPLE(var,attenuation) var=*srce++/(MAXVOL_FACTOR*attenuation)
#define CHECK_SAMPLE(var,bound) var=(var>=bound)?bound-1:(var<-bound)?-bound:var

static void Mix32To16_Normal(SWORD* dste,SLONG* srce,NATIVE count)
{
	NATIVE x1,x2,tmpx;
	int i;

	for(count/=SAMPLING_FACTOR;count;count--) {
		tmpx=0;

		for(i=SAMPLING_FACTOR/2;i;i--) {
			EXTRACT_SAMPLE(x1,1); EXTRACT_SAMPLE(x2,1);

			CHECK_SAMPLE(x1,32768); CHECK_SAMPLE(x2,32768);

			tmpx+=x1+x2;
		}
		*dste++ =tmpx/SAMPLING_FACTOR;
	}
}

static void Mix32To16_Stereo(SWORD* dste,SLONG* srce,NATIVE count)
{
	NATIVE x1,x2,x3,x4,tmpx,tmpy;
	int i;

	for(count/=SAMPLING_FACTOR;count;count--) {
		tmpx=tmpy=0;

		for(i=SAMPLING_FACTOR/2;i;i--) {
			EXTRACT_SAMPLE(x1,1); EXTRACT_SAMPLE(x2,1);
			EXTRACT_SAMPLE(x3,1); EXTRACT_SAMPLE(x4,1);

			CHECK_SAMPLE(x1,32768); CHECK_SAMPLE(x2,32768);
			CHECK_SAMPLE(x3,32768); CHECK_SAMPLE(x4,32768);

			tmpx+=x1+x3;
			tmpy+=x2+x4;
		}
		*dste++ =tmpx/SAMPLING_FACTOR;
		*dste++ =tmpy/SAMPLING_FACTOR;
	}
}

static void Mix32To8_Normal(SBYTE* dste,SLONG* srce,NATIVE count)
{
	NATIVE x1,x2,tmpx;
	int i;

	for(count/=SAMPLING_FACTOR;count;count--) {
		tmpx = 0;

		for(i=SAMPLING_FACTOR/2;i;i--) {
			EXTRACT_SAMPLE(x1,256); EXTRACT_SAMPLE(x2,256);

			CHECK_SAMPLE(x1,128); CHECK_SAMPLE(x2,128);

			tmpx+=x1+x2;
		}
		*dste++ =(tmpx/SAMPLING_FACTOR)+128;
	}
}

static void Mix32To8_Stereo(SBYTE* dste,SLONG* srce,NATIVE count)
{
	NATIVE x1,x2,x3,x4,tmpx,tmpy;
	int i;

	for(count/=SAMPLING_FACTOR;count;count--) {
		tmpx=tmpy=0;

		for(i=SAMPLING_FACTOR/2;i;i--) {
			EXTRACT_SAMPLE(x1,256); EXTRACT_SAMPLE(x2,256);
			EXTRACT_SAMPLE(x3,256); EXTRACT_SAMPLE(x4,256);

			CHECK_SAMPLE(x1,128); CHECK_SAMPLE(x2,128);
			CHECK_SAMPLE(x3,128); CHECK_SAMPLE(x4,128);

			tmpx+=x1+x3;
			tmpy+=x2+x4;
		}
		*dste++ =(tmpx/SAMPLING_FACTOR)+128;        
		*dste++ =(tmpy/SAMPLING_FACTOR)+128;        
	}
}

static void AddChannel(SLONG* ptr,NATIVE todo)
{
	SLONGLONG end,done;
	SWORD *s;

	if(!(s=Samples[vnf->handle])) {
		vnf->current = vnf->active  = 0;
		vnf->lastvalL = vnf->lastvalR = 0;
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

		if(vnf->vol || vnf->rampvol) {
#ifndef NATIVE_64BIT_INT
			/* use the 32 bit mixers as often as we can (they're much faster) */
			if((vnf->current<0x7fffffff)&&(endpos<0x7fffffff)) {
				if(vc_mode & DMODE_STEREO) {
					if((vnf->pan==PAN_SURROUND)&&(vc_mode&DMODE_SURROUND))
						vnf->current=Mix32StereoSurround
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
				if(vc_mode & DMODE_STEREO) {
					if((vnf->pan==PAN_SURROUND)&&(vc_mode&DMODE_SURROUND))
						vnf->current=MixStereoSurround
						               (s,ptr,vnf->current,vnf->increment,done);
					else
						vnf->current=MixStereoNormal
						               (s,ptr,vnf->current,vnf->increment,done);
				} else
					vnf->current=MixMonoNormal
					                   (s,ptr,vnf->current,vnf->increment,done);
			}
		} else  {
			vnf->lastvalL = vnf->lastvalR = 0;
			/* update sample position */
			vnf->current=endpos;
		}

		todo -= done;
		ptr +=(vc_mode & DMODE_STEREO)?(done<<1):done;
	}
}

#define _IN_VIRTCH_

#define VC1_SilenceBytes      VC2_SilenceBytes
#define VC1_WriteSamples      VC2_WriteSamples
#define VC1_WriteBytes        VC2_WriteBytes
#define VC1_Exit              VC2_Exit
#define VC1_VoiceSetVolume    VC2_VoiceSetVolume
#define VC1_VoiceGetVolume    VC2_VoiceGetVolume
#define VC1_VoiceSetPanning   VC2_VoiceSetPanning
#define VC1_VoiceGetPanning   VC2_VoiceGetPanning
#define VC1_VoiceSetFrequency VC2_VoiceSetFrequency
#define VC1_VoiceGetFrequency VC2_VoiceGetFrequency
#define VC1_VoicePlay         VC2_VoicePlay
#define VC1_VoiceStop         VC2_VoiceStop
#define VC1_VoiceStopped      VC2_VoiceStopped
#define VC1_VoiceGetPosition  VC2_VoiceGetPosition
#define VC1_SampleUnload      VC2_SampleUnload
#define VC1_SampleLoad        VC2_SampleLoad
#define VC1_SampleSpace       VC2_SampleSpace
#define VC1_SampleLength      VC2_SampleLength
#define VC1_VoiceRealVolume   VC2_VoiceRealVolume

#include "virtch_common.c"
#undef _IN_VIRTCH_

void VC2_WriteSamples(SBYTE* buf,ULONG todo)
{
	int left,portion=0;
	SBYTE *buffer;
	int t,pan,vol;

	todo*=SAMPLING_FACTOR;

	while(todo) {
		if(!tickleft) {
			if(vc_mode & DMODE_SOFT_MUSIC) md_player();
			tickleft=(md_mixfreq*125L*SAMPLING_FACTOR)/(md_bpm*50L);
			tickleft&=~(SAMPLING_FACTOR-1);
		}
		left = MIN(tickleft, todo);
		buffer    = buf;
		tickleft -= left;
		todo     -= left;
		buf += samples2bytes(left)/SAMPLING_FACTOR;

		while(left) {
			portion = MIN(left, samplesthatfit);
			memset(vc_tickbuf,0,portion<<((vc_mode&DMODE_STEREO)?3:2));
			for(t=0;t<vc_softchn;t++) {
				vnf = &vinf[t];

				if(vnf->kick) {
					vnf->current=((SLONGLONG)(vnf->start))<<FRACBITS;
					vnf->kick    = 0;
					vnf->active  = 1;
					vnf->click   = CLICK_BUFFER;
					vnf->rampvol = 0;
				}

				if(!vnf->frq) vnf->active = 0;

				if(vnf->active) {
					vnf->increment=((SLONGLONG)(vnf->frq)<<(FRACBITS-SAMPLING_SHIFT))
					               /md_mixfreq;
					if(vnf->flags&SF_REVERSE) vnf->increment=-vnf->increment;
					vol = vnf->vol;  pan = vnf->pan;

					vnf->oldlvol=vnf->lvolsel;vnf->oldrvol=vnf->rvolsel;
					if(vc_mode & DMODE_STEREO) {
						if(pan!=PAN_SURROUND) {
							vnf->lvolsel=(vol*(PAN_RIGHT-pan))>>8;
							vnf->rvolsel=(vol*pan)>>8;
						} else {
							vnf->lvolsel=vnf->rvolsel=(vol * 256L) / 480;
						}
					} else
						vnf->lvolsel=vol;

					idxsize=(vnf->size)?((SLONGLONG)(vnf->size)<<FRACBITS)-1:0;
					idxlend=(vnf->repend)?((SLONGLONG)(vnf->repend)<<FRACBITS)-1:0;
					idxlpos=(SLONGLONG)(vnf->reppos)<<FRACBITS;
					AddChannel(vc_tickbuf,portion);
				}
			}

			if(md_reverb) {
				if(md_reverb>15) md_reverb=15;
				MixReverb(vc_tickbuf,portion);
			}

			if(vc_mode & DMODE_16BITS)
				Mix32to16((SWORD*)buffer,vc_tickbuf,portion);
			else
				Mix32to8((SBYTE*)buffer,vc_tickbuf,portion);

			buffer += samples2bytes(portion) / SAMPLING_FACTOR;
			left   -= portion;
		}
	}
}

BOOL VC2_Init(void)
{
	VC_SetupPointers();
	
	if (!(md_mode&DMODE_HQMIXER))
		return VC1_Init();
	
	if(!(Samples=(SWORD**)_mm_calloc(MAXSAMPLEHANDLES,sizeof(SWORD*)))) {
		_mm_errno = MMERR_INITIALIZING_MIXER;
		return 1;
	}
	if(!vc_tickbuf)
		if(!(vc_tickbuf=(SLONG*)_mm_malloc((TICKLSIZE+32)*sizeof(SLONG)))) {
			_mm_errno = MMERR_INITIALIZING_MIXER;
			return 1;
		}

	if(md_mode & DMODE_STEREO) {
		Mix32to16  = Mix32To16_Stereo;
		Mix32to8   = Mix32To8_Stereo;
		MixReverb  = MixReverb_Stereo;
	} else {
		Mix32to16  = Mix32To16_Normal;
		Mix32to8   = Mix32To8_Normal;
		MixReverb  = MixReverb_Normal;
	}
	md_mode |= DMODE_INTERP;
	vc_mode = md_mode;
	return 0;
}

BOOL VC2_PlayStart(void)
{
	md_mode|=DMODE_INTERP;

	samplesthatfit = TICKLSIZE;
	if(vc_mode & DMODE_STEREO) samplesthatfit >>= 1;
	tickleft = 0;

	RVc1 = (5000L * md_mixfreq) / (REVERBERATION * 10);
	RVc2 = (5078L * md_mixfreq) / (REVERBERATION * 10);
	RVc3 = (5313L * md_mixfreq) / (REVERBERATION * 10);
	RVc4 = (5703L * md_mixfreq) / (REVERBERATION * 10);
	RVc5 = (6250L * md_mixfreq) / (REVERBERATION * 10);
	RVc6 = (6953L * md_mixfreq) / (REVERBERATION * 10);
	RVc7 = (7813L * md_mixfreq) / (REVERBERATION * 10);
	RVc8 = (8828L * md_mixfreq) / (REVERBERATION * 10);

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

void VC2_PlayStop(void)
{
	if(RVbufL1) free(RVbufL1);
	if(RVbufL2) free(RVbufL2);
	if(RVbufL3) free(RVbufL3);
	if(RVbufL4) free(RVbufL4);
	if(RVbufL5) free(RVbufL5);
	if(RVbufL6) free(RVbufL6);
	if(RVbufL7) free(RVbufL7);
	if(RVbufL8) free(RVbufL8);
	if(RVbufR1) free(RVbufR1);
	if(RVbufR2) free(RVbufR2);
	if(RVbufR3) free(RVbufR3);
	if(RVbufR4) free(RVbufR4);
	if(RVbufR5) free(RVbufR5);
	if(RVbufR6) free(RVbufR6);
	if(RVbufR7) free(RVbufR7);
	if(RVbufR8) free(RVbufR8);

	RVbufL1=RVbufL2=RVbufL3=RVbufL4=RVbufL5=RVbufL6=RVbufL7=RVbufL8=NULL;
	RVbufR1=RVbufR2=RVbufR3=RVbufR4=RVbufR5=RVbufR6=RVbufR7=RVbufR8=NULL;
}

BOOL VC2_SetNumVoices(void)
{
	int t;

	md_mode|=DMODE_INTERP;

	if(!(vc_softchn=md_softchn)) return 0;

	if(vinf) free(vinf);
	if(!(vinf=_mm_calloc(sizeof(VINFO),vc_softchn))) return 1;

	for(t=0;t<vc_softchn;t++) {
		vinf[t].frq=10000;
		vinf[t].pan=(t&1)?PAN_LEFT:PAN_RIGHT;
	}

	return 0;
}

/* ex:set ts=4: */
