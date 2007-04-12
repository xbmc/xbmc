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

  Common source parts between the two software mixers.
  This file is probably the ugliest part of libmikmod...

==============================================================================*/

#include "xbsection_start.h"

#ifndef _IN_VIRTCH_
#include "mikmod.h"
#include "mikmod_internals.h"

extern BOOL  VC1_Init(void);
extern BOOL  VC2_Init(void);
static BOOL (*VC_Init_ptr)(void)=VC1_Init;
extern void  VC1_Exit(void);
extern void  VC2_Exit(void);
static void (*VC_Exit_ptr)(void)=VC1_Exit;
extern BOOL  VC1_SetNumVoices(void);
extern BOOL  VC2_SetNumVoices(void);
static BOOL (*VC_SetNumVoices_ptr)(void);
extern ULONG VC1_SampleSpace(int);
extern ULONG VC2_SampleSpace(int);
static ULONG (*VC_SampleSpace_ptr)(int);
extern ULONG VC1_SampleLength(int,SAMPLE*);
extern ULONG VC2_SampleLength(int,SAMPLE*);
static ULONG (*VC_SampleLength_ptr)(int,SAMPLE*);

extern BOOL  VC1_PlayStart(void);
extern BOOL  VC2_PlayStart(void);
static BOOL (*VC_PlayStart_ptr)(void);
extern void  VC1_PlayStop(void);
extern void  VC2_PlayStop(void);
static void (*VC_PlayStop_ptr)(void);

extern SWORD VC1_SampleLoad(struct SAMPLOAD*,int);
extern SWORD VC2_SampleLoad(struct SAMPLOAD*,int);
static SWORD (*VC_SampleLoad_ptr)(struct SAMPLOAD*,int);
extern void  VC1_SampleUnload(SWORD);
extern void  VC2_SampleUnload(SWORD);
static void (*VC_SampleUnload_ptr)(SWORD);

extern ULONG VC1_WriteBytes(SBYTE*,ULONG);
extern ULONG VC2_WriteBytes(SBYTE*,ULONG);
static ULONG (*VC_WriteBytes_ptr)(SBYTE*,ULONG);
extern ULONG VC1_SilenceBytes(SBYTE*,ULONG);
extern ULONG VC2_SilenceBytes(SBYTE*,ULONG);
static ULONG (*VC_SilenceBytes_ptr)(SBYTE*,ULONG);

extern void  VC1_VoiceSetVolume(UBYTE,UWORD);
extern void  VC2_VoiceSetVolume(UBYTE,UWORD);
static void (*VC_VoiceSetVolume_ptr)(UBYTE,UWORD);
extern UWORD VC1_VoiceGetVolume(UBYTE);
extern UWORD VC2_VoiceGetVolume(UBYTE);
static UWORD (*VC_VoiceGetVolume_ptr)(UBYTE);
extern void  VC1_VoiceSetFrequency(UBYTE,ULONG);
extern void  VC2_VoiceSetFrequency(UBYTE,ULONG);
static void (*VC_VoiceSetFrequency_ptr)(UBYTE,ULONG);
extern ULONG VC1_VoiceGetFrequency(UBYTE);
extern ULONG VC2_VoiceGetFrequency(UBYTE);
static ULONG (*VC_VoiceGetFrequency_ptr)(UBYTE);
extern void  VC1_VoiceSetPanning(UBYTE,ULONG);
extern void  VC2_VoiceSetPanning(UBYTE,ULONG);
static void (*VC_VoiceSetPanning_ptr)(UBYTE,ULONG);
extern ULONG VC1_VoiceGetPanning(UBYTE);
extern ULONG VC2_VoiceGetPanning(UBYTE);
static ULONG (*VC_VoiceGetPanning_ptr)(UBYTE);
extern void  VC1_VoicePlay(UBYTE,SWORD,ULONG,ULONG,ULONG,ULONG,UWORD);
extern void  VC2_VoicePlay(UBYTE,SWORD,ULONG,ULONG,ULONG,ULONG,UWORD);
static void (*VC_VoicePlay_ptr)(UBYTE,SWORD,ULONG,ULONG,ULONG,ULONG,UWORD);

extern void  VC1_VoiceStop(UBYTE);
extern void  VC2_VoiceStop(UBYTE);
static void (*VC_VoiceStop_ptr)(UBYTE);
extern BOOL  VC1_VoiceStopped(UBYTE);
extern BOOL  VC2_VoiceStopped(UBYTE);
static BOOL (*VC_VoiceStopped_ptr)(UBYTE);
extern SLONG VC1_VoiceGetPosition(UBYTE);
extern SLONG VC2_VoiceGetPosition(UBYTE);
static SLONG (*VC_VoiceGetPosition_ptr)(UBYTE);
extern ULONG VC1_VoiceRealVolume(UBYTE);
extern ULONG VC2_VoiceRealVolume(UBYTE);
static ULONG (*VC_VoiceRealVolume_ptr)(UBYTE);

#if defined __STDC__ || defined _MSC_VER
#define VC_PROC0(suffix) \
MIKMODAPI void VC_##suffix (void) { VC_##suffix##_ptr(); }

#define VC_FUNC0(suffix,ret) \
MIKMODAPI ret VC_##suffix (void) { return VC_##suffix##_ptr(); }

#define VC_PROC1(suffix,typ1) \
MIKMODAPI void VC_##suffix (typ1 a) { VC_##suffix##_ptr(a); }

#define VC_FUNC1(suffix,ret,typ1) \
MIKMODAPI ret VC_##suffix (typ1 a) { return VC_##suffix##_ptr(a); }

#define VC_PROC2(suffix,typ1,typ2) \
MIKMODAPI void VC_##suffix (typ1 a,typ2 b) { VC_##suffix##_ptr(a,b); }

#define VC_FUNC2(suffix,ret,typ1,typ2) \
MIKMODAPI ret VC_##suffix (typ1 a,typ2 b) { return VC_##suffix##_ptr(a,b); }
#else
#define VC_PROC0(suffix) \
MIKMODAPI void VC_/**/suffix (void) { VC_/**/suffix/**/_ptr(); }

#define VC_FUNC0(suffix,ret) \
MIKMODAPI ret VC_/**/suffix (void) { return VC_/**/suffix/**/_ptr(); }

#define VC_PROC1(suffix,typ1) \
MIKMODAPI void VC_/**/suffix (typ1 a) { VC_/**/suffix/**/_ptr(a); }

#define VC_FUNC1(suffix,ret,typ1) \
MIKMODAPI ret VC_/**/suffix (typ1 a) { return VC_/**/suffix/**/_ptr(a); }

#define VC_PROC2(suffix,typ1,typ2) \
MIKMODAPI void VC_/**/suffix (typ1 a,typ2 b) { VC_/**/suffix/**/_ptr(a,b); }

#define VC_FUNC2(suffix,ret,typ1,typ2) \
MIKMODAPI ret VC_/**/suffix (typ1 a,typ2 b) { return VC_/**/suffix/**/_ptr(a,b); }
#endif

VC_FUNC0(Init,BOOL)
VC_PROC0(Exit)
VC_FUNC0(SetNumVoices,BOOL)
VC_FUNC1(SampleSpace,ULONG,int)
VC_FUNC2(SampleLength,ULONG,int,SAMPLE*)
VC_FUNC0(PlayStart,BOOL)
VC_PROC0(PlayStop)
VC_FUNC2(SampleLoad,SWORD,struct SAMPLOAD*,int)
VC_PROC1(SampleUnload,SWORD)
VC_FUNC2(WriteBytes,ULONG,SBYTE*,ULONG)
VC_FUNC2(SilenceBytes,ULONG,SBYTE*,ULONG)
VC_PROC2(VoiceSetVolume,UBYTE,UWORD)
VC_FUNC1(VoiceGetVolume,UWORD,UBYTE)
VC_PROC2(VoiceSetFrequency,UBYTE,ULONG)
VC_FUNC1(VoiceGetFrequency,ULONG,UBYTE)
VC_PROC2(VoiceSetPanning,UBYTE,ULONG)
VC_FUNC1(VoiceGetPanning,ULONG,UBYTE)
		
void  VC_VoicePlay(UBYTE a,SWORD b,ULONG c,ULONG d,ULONG e,ULONG f,UWORD g)
{ VC_VoicePlay_ptr(a,b,c,d,e,f,g); }

VC_PROC1(VoiceStop,UBYTE)
VC_FUNC1(VoiceStopped,BOOL,UBYTE)
VC_FUNC1(VoiceGetPosition,SLONG,UBYTE)
VC_FUNC1(VoiceRealVolume,ULONG,UBYTE)
		
void VC_SetupPointers(void)
{
	if (md_mode&DMODE_HQMIXER) {
		VC_Init_ptr=VC2_Init;
		VC_Exit_ptr=VC2_Exit;
		VC_SetNumVoices_ptr=VC2_SetNumVoices;
		VC_SampleSpace_ptr=VC2_SampleSpace;
		VC_SampleLength_ptr=VC2_SampleLength;
		VC_PlayStart_ptr=VC2_PlayStart;
		VC_PlayStop_ptr=VC2_PlayStop;
		VC_SampleLoad_ptr=VC2_SampleLoad;
		VC_SampleUnload_ptr=VC2_SampleUnload;
		VC_WriteBytes_ptr=VC2_WriteBytes;
		VC_SilenceBytes_ptr=VC2_SilenceBytes;
		VC_VoiceSetVolume_ptr=VC2_VoiceSetVolume;
		VC_VoiceGetVolume_ptr=VC2_VoiceGetVolume;
		VC_VoiceSetFrequency_ptr=VC2_VoiceSetFrequency;
		VC_VoiceGetFrequency_ptr=VC2_VoiceGetFrequency;
		VC_VoiceSetPanning_ptr=VC2_VoiceSetPanning;
		VC_VoiceGetPanning_ptr=VC2_VoiceGetPanning;
		VC_VoicePlay_ptr=VC2_VoicePlay;
		VC_VoiceStop_ptr=VC2_VoiceStop;
		VC_VoiceStopped_ptr=VC2_VoiceStopped;
		VC_VoiceGetPosition_ptr=VC2_VoiceGetPosition;
		VC_VoiceRealVolume_ptr=VC2_VoiceRealVolume;
	} else {
		VC_Init_ptr=VC1_Init;
		VC_Exit_ptr=VC1_Exit;
		VC_SetNumVoices_ptr=VC1_SetNumVoices;
		VC_SampleSpace_ptr=VC1_SampleSpace;
		VC_SampleLength_ptr=VC1_SampleLength;
		VC_PlayStart_ptr=VC1_PlayStart;
		VC_PlayStop_ptr=VC1_PlayStop;
		VC_SampleLoad_ptr=VC1_SampleLoad;
		VC_SampleUnload_ptr=VC1_SampleUnload;
		VC_WriteBytes_ptr=VC1_WriteBytes;
		VC_SilenceBytes_ptr=VC1_SilenceBytes;
		VC_VoiceSetVolume_ptr=VC1_VoiceSetVolume;
		VC_VoiceGetVolume_ptr=VC1_VoiceGetVolume;
		VC_VoiceSetFrequency_ptr=VC1_VoiceSetFrequency;
		VC_VoiceGetFrequency_ptr=VC1_VoiceGetFrequency;
		VC_VoiceSetPanning_ptr=VC1_VoiceSetPanning;
		VC_VoiceGetPanning_ptr=VC1_VoiceGetPanning;
		VC_VoicePlay_ptr=VC1_VoicePlay;
		VC_VoiceStop_ptr=VC1_VoiceStop;
		VC_VoiceStopped_ptr=VC1_VoiceStopped;
		VC_VoiceGetPosition_ptr=VC1_VoiceGetPosition;
		VC_VoiceRealVolume_ptr=VC1_VoiceRealVolume;
	}
}

#else

#ifndef _VIRTCH_COMMON_
#define _VIRTCH_COMMON_

static ULONG samples2bytes(ULONG samples)
{
	if(vc_mode & DMODE_16BITS) samples <<= 1;
	if(vc_mode & DMODE_STEREO) samples <<= 1;
	return samples;
}

static ULONG bytes2samples(ULONG bytes)
{
	if(vc_mode & DMODE_16BITS) bytes >>= 1;
	if(vc_mode & DMODE_STEREO) bytes >>= 1;
	return bytes;
}

/* Fill the buffer with 'todo' bytes of silence (it depends on the mixing mode
   how the buffer is filled) */
ULONG VC1_SilenceBytes(SBYTE* buf,ULONG todo)
{
	todo=samples2bytes(bytes2samples(todo));

	/* clear the buffer to zero (16 bits signed) or 0x80 (8 bits unsigned) */
	if(vc_mode & DMODE_16BITS)
		memset(buf,0,todo);
	else
		memset(buf,0x80,todo);

	return todo;
}

void VC1_WriteSamples(SBYTE*,ULONG);

/* Writes 'todo' mixed SBYTES (!!) to 'buf'. It returns the number of SBYTES
   actually written to 'buf' (which is rounded to number of samples that fit
   into 'todo' bytes). */
ULONG VC1_WriteBytes(SBYTE* buf,ULONG todo)
{
	if(!vc_softchn)
		return VC1_SilenceBytes(buf,todo);

	todo = bytes2samples(todo);
	VC1_WriteSamples(buf,todo);

	return samples2bytes(todo);
}

void VC1_Exit(void)
{
	if(vc_tickbuf) free(vc_tickbuf);
	if(vinf) free(vinf);
	if(Samples) free(Samples);

	vc_tickbuf = NULL;
	vinf = NULL;
	Samples = NULL;
	
	VC_SetupPointers();
}

UWORD VC1_VoiceGetVolume(UBYTE voice)
{
	return vinf[voice].vol;
}

ULONG VC1_VoiceGetPanning(UBYTE voice)
{
	return vinf[voice].pan;
}

void VC1_VoiceSetFrequency(UBYTE voice,ULONG frq)
{
	if (voice <= 1 && vinf[voice].frq != frq)
		XB_Log("SetFrq:    c=%02d, frq=%03d", voice, frq);
	vinf[voice].frq=frq;
}

ULONG VC1_VoiceGetFrequency(UBYTE voice)
{
	return vinf[voice].frq;
}

void VC1_VoicePlay(UBYTE voice,SWORD handle,ULONG start,ULONG size,ULONG reppos,ULONG repend,UWORD flags)
{
	vinf[voice].flags    = flags;
	vinf[voice].handle   = handle;
	vinf[voice].start    = start;
	vinf[voice].size     = size;
	vinf[voice].reppos   = reppos;
	vinf[voice].repend   = repend;
	vinf[voice].kick     = 1;

	if (voice <= 1)
		XB_Log("VoicePlay: c=%02d, s=%02d, play=%05d-%05d, loop=%05d-%05d, flags=%02x", voice, handle, start, size, reppos, repend, flags >> 8);
}

void VC1_VoiceStop(UBYTE voice)
{
	vinf[voice].active = 0;
}  

BOOL VC1_VoiceStopped(UBYTE voice)
{
	return(vinf[voice].active==0);
}

SLONG VC1_VoiceGetPosition(UBYTE voice)
{
	return(vinf[voice].current>>FRACBITS);
}

void VC1_VoiceSetVolume(UBYTE voice,UWORD vol)
{    
	/* protect against clicks if volume variation is too high */
	if(abs((int)vinf[voice].vol-(int)vol)>32)
		vinf[voice].rampvol=CLICK_BUFFER;
	if (voice <= 1 && vinf[voice].vol != vol)
		XB_Log("SetVolume: c=%02d, vol=%03d", voice, vol);
	vinf[voice].vol=vol;
}

void VC1_VoiceSetPanning(UBYTE voice,ULONG pan)
{
	/* protect against clicks if panning variation is too high */
	if(abs((int)vinf[voice].pan-(int)pan)>48)
		vinf[voice].rampvol=CLICK_BUFFER;
	if (voice <= 1 && vinf[voice].pan != pan)
		XB_Log("SetPan:    c=%02d, pan=%03d", voice, pan);
	vinf[voice].pan=pan;
}

/*========== External mixer interface */

void VC1_SampleUnload(SWORD handle)
{
	if (handle<MAXSAMPLEHANDLES) {
		if (Samples[handle])
			free(Samples[handle]);
		Samples[handle]=NULL;
	}
}

SWORD VC1_SampleLoad(struct SAMPLOAD* sload,int type)
{
	SAMPLE *s = sload->sample;
	int handle;
	ULONG t, length,loopstart,loopend;

	if(type==MD_HARDWARE) return -1;

	/* Find empty slot to put sample address in */
	for(handle=0;handle<MAXSAMPLEHANDLES;handle++)
		if(!Samples[handle]) break;

	if(handle==MAXSAMPLEHANDLES) {
		_mm_errno = MMERR_OUT_OF_HANDLES;
		return -1;
	}
	
	/* Reality check for loop settings */
	if (s->loopend > s->length)
		s->loopend = s->length;
	if (s->loopstart >= s->loopend)
		s->flags &= ~SF_LOOP;

	length    = s->length;
	loopstart = s->loopstart;
	loopend   = s->loopend;

	SL_SampleSigned(sload);
	SL_Sample8to16(sload);

	if(!(Samples[handle]=(SWORD*)_mm_malloc((length+20)<<1))) {
		_mm_errno = MMERR_SAMPLE_TOO_BIG;
		return -1;
	}

	/* read sample into buffer */
	if (SL_Load(Samples[handle],sload,length))
		return -1;

	/* Unclick sample */
	if(s->flags & SF_LOOP) {
		if(s->flags & SF_BIDI)
			for(t=0;t<16;t++)
				Samples[handle][loopend+t]=Samples[handle][(loopend-t)-1];
		else
			for(t=0;t<16;t++)
				Samples[handle][loopend+t]=Samples[handle][t+loopstart];
	} else
		for(t=0;t<16;t++)
			Samples[handle][t+length]=0;

	return handle;
}

ULONG VC1_SampleSpace(int type)
{
	return vc_memory;
}

ULONG VC1_SampleLength(int type,SAMPLE* s)
{
	if (!s) return 0;

	return (s->length*((s->flags&SF_16BITS)?2:1))+16;
}

ULONG VC1_VoiceRealVolume(UBYTE voice)
{
	ULONG i,s,size;
	int k,j;
	SWORD *smp;
	SLONG t;

	t = vinf[voice].current>>FRACBITS;
	if(!vinf[voice].active) return 0;

	s = vinf[voice].handle;
	size = vinf[voice].size;

	i=64; t-=64; k=0; j=0;
	if(i>size) i = size;
	if(t<0) t = 0;
	if(t+i > size) t = size-i;

	i &= ~1;  /* make sure it's EVEN. */

	smp = &Samples[s][t];
	for(;i;i--,smp++) {
		if(k<*smp) k = *smp;
		if(j>*smp) j = *smp;
	}
	return abs(k-j);
}

#endif

#endif

/* ex:set ts=4: */
