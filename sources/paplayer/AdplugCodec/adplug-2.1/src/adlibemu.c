/*
 * ADLIBEMU.C
 * Copyright (C) 1998-2001 Ken Silverman
 * Ken Silverman's official web site: "http://www.advsys.net/ken"
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*
This file is a digital Adlib emulator for OPL2 and possibly OPL3

Features that could be added in a future version:
- Amplitude and Frequency Vibrato Bits (not hard, but a big speed hit)
- Global Keyboard Split Number Bit (need to research this one some more)
- 2nd Adlib chip for OPL3 (simply need to make my cell array bigger)
- Advanced connection modes of OPL3 (Just need to add more "docell" cases)
- L/R Stereo bits of OPL3 (Need adlibgetsample to return stereo)

Features that aren't worth supporting:
- Anything related to adlib timers&interrupts (Sorry - I always used IRQ0)
- Composite sine wave mode (CSM) (Supported only on ancient cards)

I'm not sure about a few things in my code:
- Attack curve.  What function is this anyway?  I chose to use an order-3
  polynomial to approximate but this doesn't seem right.
- Attack/Decay/Release constants - my constants may not be exact
- What should ADJUSTSPEED be?
- Haven't verified that Global Keyboard Split Number Bit works yet
- Some of the drums don't always sound right.  It's pretty hard to guess
  the exact waveform of drums when you look at random data which is
  slightly randomized due to digital ADC recording.
- Adlib seems to have a lot more treble than my emulator does.  I'm not
  sure if this is simply unfixable due to the sound blaster's different
  filtering on FM and digital playback or if it's a serious bug in my
  code.
*/

#include <math.h>
#include <string.h>

#if !defined(max) && !defined(__cplusplus)
#define max(a,b)  (((a) > (b)) ? (a) : (b))
#endif
#if !defined(min) && !defined(__cplusplus)
#define min(a,b)  (((a) < (b)) ? (a) : (b))
#endif

#define PI 3.141592653589793
#define MAXCELLS 18
#define WAVPREC 2048

static float AMPSCALE=(8192.0);
#define FRQSCALE (49716/512.0)

//Constants for Ken's Awe32, on a PII-266 (Ken says: Use these for KSM's!)
#define MODFACTOR 4.0      //How much of modulator cell goes into carrier
#define MFBFACTOR 1.0      //How much feedback goes back into modulator
#define ADJUSTSPEED 0.75   //0<=x<=1  Simulate finite rate of change of state

//Constants for Ken's Awe64G, on a P-133
//#define MODFACTOR 4.25   //How much of modulator cell goes into carrier
//#define MFBFACTOR 0.5    //How much feedback goes back into modulator
//#define ADJUSTSPEED 0.85 //0<=x<=1  Simulate finite rate of change of state

typedef struct
{
    float val, t, tinc, vol, sustain, amp, mfb;
    float a0, a1, a2, a3, decaymul, releasemul;
    short *waveform;
    long wavemask;
    void (*cellfunc)(void *, float);
    unsigned char flags, dum0, dum1, dum2;
} celltype;

static long numspeakers, bytespersample;
static float recipsamp;
static celltype cell[MAXCELLS];
static signed short wavtable[WAVPREC*3];
static float kslmul[4] = {0.0,0.5,0.25,1.0};
static float frqmul[16] = {.5,1,2,3,4,5,6,7,8,9,10,10,12,12,15,15}, nfrqmul[16];
static unsigned char adlibreg[256], ksl[8][16];
static unsigned char modulatorbase[9] = {0,1,2,8,9,10,16,17,18};
static unsigned char odrumstat = 0;
static unsigned char base2cell[22] = {0,1,2,0,1,2,0,0,3,4,5,3,4,5,0,0,6,7,8,6,7,8};

float lvol[9] = {1,1,1,1,1,1,1,1,1};  //Volume multiplier on left speaker
float rvol[9] = {1,1,1,1,1,1,1,1,1};  //Volume multiplier on right speaker
long lplc[9] = {0,0,0,0,0,0,0,0,0};   //Samples to delay on left speaker
long rplc[9] = {0,0,0,0,0,0,0,0,0};   //Samples to delay on right speaker

long nlvol[9], nrvol[9];
long nlplc[9], nrplc[9];
long rend = 0;
#define FIFOSIZ 256
static float *rptr[9], *nrptr[9];
static float rbuf[9][FIFOSIZ*2];
static float snd[FIFOSIZ*2];

#ifndef USING_ASM
#define _inline
#endif

#ifdef USING_ASM
static _inline void ftol (float f, long *a)
{
    _asm
	{
	    mov eax, a
		fld f
		fistp dword ptr [eax]
		}
}
#else
static void ftol(float f, long *a) {
    *a=f;
}
#endif

#define ctc ((celltype *)c)      //A rare attempt to make code easier to read!
void docell4 (void *c, float modulator) { }
void docell3 (void *c, float modulator)
{
    long i;

    ftol(ctc->t+modulator,&i);
    ctc->t += ctc->tinc;
    ctc->val += (ctc->amp*ctc->vol*((float)ctc->waveform[i&ctc->wavemask])-ctc->val)*ADJUSTSPEED;
}
void docell2 (void *c, float modulator)
{
    long i;

    ftol(ctc->t+modulator,&i);

    if (*(long *)&ctc->amp <= 0x37800000)
    {
	ctc->amp = 0;
	ctc->cellfunc = docell4;
    }
    ctc->amp *= ctc->releasemul;

    ctc->t += ctc->tinc;
    ctc->val += (ctc->amp*ctc->vol*((float)ctc->waveform[i&ctc->wavemask])-ctc->val)*ADJUSTSPEED;
}
void docell1 (void *c, float modulator)
{
    long i;

    ftol(ctc->t+modulator,&i);

    if ((*(long *)&ctc->amp) <= (*(long *)&ctc->sustain))
    {
	if (ctc->flags&32)
	{
	    ctc->amp = ctc->sustain;
	    ctc->cellfunc = docell3;
	}
	else
	    ctc->cellfunc = docell2;
    }
    else
	ctc->amp *= ctc->decaymul;

    ctc->t += ctc->tinc;
    ctc->val += (ctc->amp*ctc->vol*((float)ctc->waveform[i&ctc->wavemask])-ctc->val)*ADJUSTSPEED;
}
void docell0 (void *c, float modulator)
{
    long i;

    ftol(ctc->t+modulator,&i);

    ctc->amp = ((ctc->a3*ctc->amp + ctc->a2)*ctc->amp + ctc->a1)*ctc->amp + ctc->a0;
    if ((*(long *)&ctc->amp) > 0x3f800000)
    {
	ctc->amp = 1;
	ctc->cellfunc = docell1;
    }

    ctc->t += ctc->tinc;
    ctc->val += (ctc->amp*ctc->vol*((float)ctc->waveform[i&ctc->wavemask])-ctc->val)*ADJUSTSPEED;
}


static long waveform[8] = {WAVPREC,WAVPREC>>1,WAVPREC,(WAVPREC*3)>>2,0,0,(WAVPREC*5)>>2,WAVPREC<<1};
static long wavemask[8] = {WAVPREC-1,WAVPREC-1,(WAVPREC>>1)-1,(WAVPREC>>1)-1,WAVPREC-1,((WAVPREC*3)>>2)-1,WAVPREC>>1,WAVPREC-1};
static long wavestart[8] = {0,WAVPREC>>1,0,WAVPREC>>2,0,0,0,WAVPREC>>3};
static float attackconst[4] = {1/2.82624,1/2.25280,1/1.88416,1/1.59744};
static float decrelconst[4] = {1/39.28064,1/31.41608,1/26.17344,1/22.44608};
void cellon (long i, long j, celltype *c, unsigned char iscarrier)
{
    long frn, oct, toff;
    float f;

    frn = ((((long)adlibreg[i+0xb0])&3)<<8) + (long)adlibreg[i+0xa0];
    oct = ((((long)adlibreg[i+0xb0])>>2)&7);
    toff = (oct<<1) + ((frn>>9)&((frn>>8)|(((adlibreg[8]>>6)&1)^1)));
    if (!(adlibreg[j+0x20]&16)) toff >>= 2;

    f = pow(2.0,(adlibreg[j+0x60]>>4)+(toff>>2)-1)*attackconst[toff&3]*recipsamp;
    c->a0 = .0377*f; c->a1 = 10.73*f+1; c->a2 = -17.57*f; c->a3 = 7.42*f;
    f = -7.4493*decrelconst[toff&3]*recipsamp;
    c->decaymul = pow(2.0,f*pow(2.0,(adlibreg[j+0x60]&15)+(toff>>2)));
    c->releasemul = pow(2.0,f*pow(2.0,(adlibreg[j+0x80]&15)+(toff>>2)));
    c->wavemask = wavemask[adlibreg[j+0xe0]&7];
    c->waveform = &wavtable[waveform[adlibreg[j+0xe0]&7]];
    if (!(adlibreg[1]&0x20)) c->waveform = &wavtable[WAVPREC];
    c->t = wavestart[adlibreg[j+0xe0]&7];
    c->flags = adlibreg[j+0x20];
    c->cellfunc = docell0;
    c->tinc = (float)(frn<<oct)*nfrqmul[adlibreg[j+0x20]&15];
    c->vol = pow(2.0,((float)(adlibreg[j+0x40]&63) +
		      (float)kslmul[adlibreg[j+0x40]>>6]*ksl[oct][frn>>6]) * -.125 - 14);
    c->sustain = pow(2.0,(float)(adlibreg[j+0x80]>>4) * -.5);
    if (!iscarrier) c->amp = 0;
    c->mfb = pow(2.0,((adlibreg[i+0xc0]>>1)&7)+5)*(WAVPREC/2048.0)*MFBFACTOR;
    if (!(adlibreg[i+0xc0]&14)) c->mfb = 0;
    c->val = 0;
}

//This function (and bug fix) written by Chris Moeller
void cellfreq (signed long i, signed long j, celltype *c)
{
    long frn, oct;

    frn = ((((long)adlibreg[i+0xb0])&3)<<8) + (long)adlibreg[i+0xa0];
    oct = ((((long)adlibreg[i+0xb0])>>2)&7);

    c->tinc = (float)(frn<<oct)*nfrqmul[adlibreg[j+0x20]&15];
    c->vol = pow(2.0,((float)(adlibreg[j+0x40]&63) +
		      (float)kslmul[adlibreg[j+0x40]>>6]*ksl[oct][frn>>6]) * -.125 - 14);
}

static long initfirstime = 0;
void adlibinit (long dasamplerate, long danumspeakers, long dabytespersample)
{
    long i, j, frn, oct;

    memset((void *)adlibreg,0,sizeof(adlibreg));
    memset((void *)cell,0,sizeof(celltype)*MAXCELLS);
    memset((void *)rbuf,0,sizeof(rbuf));
    rend = 0; odrumstat = 0;

    for(i=0;i<MAXCELLS;i++)
    {
	cell[i].cellfunc = docell4;
	cell[i].amp = 0;
	cell[i].vol = 0;
	cell[i].t = 0;
	cell[i].tinc = 0;
	cell[i].wavemask = 0;
	cell[i].waveform = &wavtable[WAVPREC];
    }

    numspeakers = danumspeakers;
    bytespersample = dabytespersample;

    recipsamp = 1.0 / (float)dasamplerate;
    for(i=15;i>=0;i--) nfrqmul[i] = frqmul[i]*recipsamp*FRQSCALE*(WAVPREC/2048.0);

    if (!initfirstime)
    {
	initfirstime = 1;

	for(i=0;i<(WAVPREC>>1);i++)
	{
	    wavtable[i] =
		wavtable[(i<<1)  +WAVPREC] = (signed short)(16384*sin((float)((i<<1)  )*PI*2/WAVPREC));
	    wavtable[(i<<1)+1+WAVPREC] = (signed short)(16384*sin((float)((i<<1)+1)*PI*2/WAVPREC));
	}
	for(i=0;i<(WAVPREC>>3);i++)
	{
	    wavtable[i+(WAVPREC<<1)] = wavtable[i+(WAVPREC>>3)]-16384;
	    wavtable[i+((WAVPREC*17)>>3)] = wavtable[i+(WAVPREC>>2)]+16384;
	}

	//[table in book]*8/3
	ksl[7][0] = 0; ksl[7][1] = 24; ksl[7][2] = 32; ksl[7][3] = 37;
	ksl[7][4] = 40; ksl[7][5] = 43; ksl[7][6] = 45; ksl[7][7] = 47;
	ksl[7][8] = 48; for(i=9;i<16;i++) ksl[7][i] = i+41;
	for(j=6;j>=0;j--)
	    for(i=0;i<16;i++)
	    {
		oct = (long)ksl[j+1][i]-8; if (oct < 0) oct = 0;
		ksl[j][i] = (unsigned char)oct;
	    }
    }
    else
    {
	for(i=0;i<9;i++)
	{
	    frn = ((((long)adlibreg[i+0xb0])&3)<<8) + (long)adlibreg[i+0xa0];
	    oct = ((((long)adlibreg[i+0xb0])>>2)&7);
	    cell[i].tinc = (float)(frn<<oct)*nfrqmul[adlibreg[modulatorbase[i]+0x20]&15];
	}
    }
}

void adlib0 (long i, long v)
{
    unsigned char tmp = adlibreg[i];
    adlibreg[i] = v;

    if (i == 0xbd)
    {
	if ((v&16) > (odrumstat&16)) //BassDrum
	{
	    cellon(6,16,&cell[6],0);
	    cellon(6,19,&cell[15],1);
	    cell[15].vol *= 2;
	}
	if ((v&8) > (odrumstat&8)) //Snare
	{
	    cellon(16,20,&cell[16],0);
	    cell[16].tinc *= 2*(nfrqmul[adlibreg[17+0x20]&15] / nfrqmul[adlibreg[20+0x20]&15]);
	    if (((adlibreg[20+0xe0]&7) >= 3) && ((adlibreg[20+0xe0]&7) <= 5)) cell[16].vol = 0;
	    cell[16].vol *= 2;
	}
	if ((v&4) > (odrumstat&4)) //TomTom
	{
	    cellon(8,18,&cell[8],0);
	    cell[8].vol *= 2;
	}
	if ((v&2) > (odrumstat&2)) //Cymbal
	{
	    cellon(17,21,&cell[17],0);

	    cell[17].wavemask = wavemask[5];
	    cell[17].waveform = &wavtable[waveform[5]];
	    cell[17].tinc *= 16; cell[17].vol *= 2;

	    //cell[17].waveform = &wavtable[WAVPREC]; cell[17].wavemask = 0;
	    //if (((adlibreg[21+0xe0]&7) == 0) || ((adlibreg[21+0xe0]&7) == 6))
	    //   cell[17].waveform = &wavtable[(WAVPREC*7)>>2];
	    //if (((adlibreg[21+0xe0]&7) == 2) || ((adlibreg[21+0xe0]&7) == 3))
	    //   cell[17].waveform = &wavtable[(WAVPREC*5)>>2];
	}
	if ((v&1) > (odrumstat&1)) //Hihat
	{
	    cellon(7,17,&cell[7],0);
	    if (((adlibreg[17+0xe0]&7) == 1) || ((adlibreg[17+0xe0]&7) == 4) ||
		((adlibreg[17+0xe0]&7) == 5) || ((adlibreg[17+0xe0]&7) == 7)) cell[7].vol = 0;
	    if ((adlibreg[17+0xe0]&7) == 6) { cell[7].wavemask = 0; cell[7].waveform = &wavtable[(WAVPREC*7)>>2]; }
	}

	odrumstat = v;
    }
    else if (((unsigned)(i-0x40) < (unsigned)22) && ((i&7) < 6))
    {
	if ((i&7) < 3) // Modulator
	    cellfreq(base2cell[i-0x40],i-0x40,&cell[base2cell[i-0x40]]);
	else          // Carrier
	    cellfreq(base2cell[i-0x40],i-0x40,&cell[base2cell[i-0x40]+9]);
    }
    else if ((unsigned)(i-0xa0) < (unsigned)9)
    {
	cellfreq(i-0xa0,modulatorbase[i-0xa0],&cell[i-0xa0]);
	cellfreq(i-0xa0,modulatorbase[i-0xa0]+3,&cell[i-0xa0+9]);
    }
    else if ((unsigned)(i-0xb0) < (unsigned)9)
    {
	if ((v&32) > (tmp&32))
	{
	    cellon(i-0xb0,modulatorbase[i-0xb0],&cell[i-0xb0],0);
	    cellon(i-0xb0,modulatorbase[i-0xb0]+3,&cell[i-0xb0+9],1);
	}
	else if ((v&32) < (tmp&32))
	    cell[i-0xb0].cellfunc = cell[i-0xb0+9].cellfunc = docell2;
	cellfreq(i-0xb0,modulatorbase[i-0xb0],&cell[i-0xb0]);
	cellfreq(i-0xb0,modulatorbase[i-0xb0]+3,&cell[i-0xb0+9]);
    }

    //outdata(i,v);
}

#ifdef USING_ASM
static long fpuasm;
static float fakeadd = 8388608.0+128.0;
static _inline void clipit8 (float f, long a)
{
    _asm
	{
	    mov edi, a
		fld dword ptr f
		fadd dword ptr fakeadd
		fstp dword ptr fpuasm
		mov eax, fpuasm
		test eax, 0x007fff00
		jz short skipit
		shr eax, 16
		xor eax, -1
		skipit: mov byte ptr [edi], al
		}
}

static _inline void clipit16 (float f, long a)
{
    _asm
	{
	    mov eax, a
		fld dword ptr f
		fist word ptr [eax]
		cmp word ptr [eax], 0x8000
		jne short skipit2
		fst dword ptr [fpuasm]
		cmp fpuasm, 0x80000000
		sbb word ptr [eax], 0
		skipit2: fstp st
		}
}
#else
static void clipit8(float f,unsigned char *a) {
    f/=256.0;
    f+=128.0;
    if (f>254.5) *a=255;
    else if (f<0.5) *a=0;
    else *a=f;
}

static void clipit16(float f,short *a) {
    if (f>32766.5) *a=32767;
    else if (f<-32767.5) *a=-32768;
    else *a=f;
}
#endif

void adlibsetvolume(int i) {
    AMPSCALE=i;
}

void adlibgetsample (unsigned char *sndptr, long numbytes)
{
    long i, j, k=0, ns, endsamples, rptrs, numsamples;
    celltype *cptr;
    float f;
    short *sndptr2=(short *)sndptr;

    numsamples = (numbytes>>(numspeakers+bytespersample-2));

    if (bytespersample == 1) f = AMPSCALE/256.0; else f = AMPSCALE;
    if (numspeakers == 1)
    {
	nlvol[0] = lvol[0]*f;
	for(i=0;i<9;i++) rptr[i] = &rbuf[0][0];
	rptrs = 1;
    }
    else
    {
	rptrs = 0;
	for(i=0;i<9;i++)
	{
	    if ((!i) || (lvol[i] != lvol[i-1]) || (rvol[i] != rvol[i-1]) ||
		(lplc[i] != lplc[i-1]) || (rplc[i] != rplc[i-1]))
	    {
		nlvol[rptrs] = lvol[i]*f;
		nrvol[rptrs] = rvol[i]*f;
		nlplc[rptrs] = rend-min(max(lplc[i],0),FIFOSIZ);
		nrplc[rptrs] = rend-min(max(rplc[i],0),FIFOSIZ);
		rptrs++;
	    }
	    rptr[i] = &rbuf[rptrs-1][0];
	}
    }


    //CPU time used to be somewhat less when emulator was only mono!
    //   Because of no delay fifos!

    for(ns=0;ns<numsamples;ns+=endsamples)
    {
	endsamples = min(FIFOSIZ*2-rend,FIFOSIZ);
	endsamples = min(endsamples,numsamples-ns);

	for(i=0;i<9;i++)
	    nrptr[i] = &rptr[i][rend];
	for(i=0;i<rptrs;i++)
	    memset((void *)&rbuf[i][rend],0,endsamples*sizeof(float));

	if (adlibreg[0xbd]&0x20)
	{
				//BassDrum (j=6)
	    if (cell[15].cellfunc != docell4)
	    {
		if (adlibreg[0xc6]&1)
		{
		    for(i=0;i<endsamples;i++)
		    {
			(cell[15].cellfunc)((void *)&cell[15],0.0);
			nrptr[6][i] += cell[15].val;
		    }
		}
		else
		{
		    for(i=0;i<endsamples;i++)
		    {
			(cell[6].cellfunc)((void *)&cell[6],cell[6].val*cell[6].mfb);
			(cell[15].cellfunc)((void *)&cell[15],cell[6].val*WAVPREC*MODFACTOR);
			nrptr[6][i] += cell[15].val;
		    }
		}
	    }

				//Snare/Hihat (j=7), Cymbal/TomTom (j=8)
	    if ((cell[7].cellfunc != docell4) || (cell[8].cellfunc != docell4) || (cell[16].cellfunc != docell4) || (cell[17].cellfunc != docell4))
	    {
		for(i=0;i<endsamples;i++)
		{
		    k = k*1664525+1013904223;
		    (cell[16].cellfunc)((void *)&cell[16],k&((WAVPREC>>1)-1)); //Snare
		    (cell[7].cellfunc)((void *)&cell[7],k&(WAVPREC-1));       //Hihat
		    (cell[17].cellfunc)((void *)&cell[17],k&((WAVPREC>>3)-1)); //Cymbal
		    (cell[8].cellfunc)((void *)&cell[8],0.0);                 //TomTom
		    nrptr[7][i] += cell[7].val + cell[16].val;
		    nrptr[8][i] += cell[8].val + cell[17].val;
		}
	    }
	}
	for(j=9-1;j>=0;j--)
	{
	    if ((adlibreg[0xbd]&0x20) && (j >= 6) && (j < 9)) continue;

	    cptr = &cell[j]; k = j;
	    if (adlibreg[0xc0+k]&1)
	    {
		if ((cptr[9].cellfunc == docell4) && (cptr->cellfunc == docell4)) continue;
		for(i=0;i<endsamples;i++)
		{
		    (cptr->cellfunc)((void *)cptr,cptr->val*cptr->mfb);
		    (cptr->cellfunc)((void *)&cptr[9],0);
		    nrptr[j][i] += cptr[9].val + cptr->val;
		}
	    }
	    else
	    {
		if (cptr[9].cellfunc == docell4) continue;
		for(i=0;i<endsamples;i++)
		{
		    (cptr->cellfunc)((void *)cptr,cptr->val*cptr->mfb);
		    (cptr[9].cellfunc)((void *)&cptr[9],cptr->val*WAVPREC*MODFACTOR);
		    nrptr[j][i] += cptr[9].val;
		}
	    }
	}

	if (numspeakers == 1)
	{
	    if (bytespersample == 1)
	    {
		for(i=endsamples-1;i>=0;i--)
		    clipit8(nrptr[0][i]*nlvol[0],sndptr+1);
	    }
	    else
	    {
		for(i=endsamples-1;i>=0;i--)
		    clipit16(nrptr[0][i]*nlvol[0],sndptr2+i);
	    }
	}
	else
	{
	    memset((void *)snd,0,endsamples*sizeof(float)*2);
	    for(j=0;j<rptrs;j++)
	    {
		for(i=0;i<endsamples;i++)
		{
		    snd[(i<<1)  ] += rbuf[j][(nlplc[j]+i)&(FIFOSIZ*2-1)]*nlvol[j];
		    snd[(i<<1)+1] += rbuf[j][(nrplc[j]+i)&(FIFOSIZ*2-1)]*nrvol[j];
		}
		nlplc[j] += endsamples;
		nrplc[j] += endsamples;
	    }

	    if (bytespersample == 1)
	    {
		for(i=(endsamples<<1)-1;i>=0;i--)
		    clipit8(snd[i],sndptr+i);
	    }
	    else
	    {
		for(i=(endsamples<<1)-1;i>=0;i--)
		    clipit16(snd[i],sndptr2+i);
	    }
	}

	sndptr = sndptr+(numspeakers*endsamples);
	sndptr2 = sndptr2+(numspeakers*endsamples);
	rend = ((rend+endsamples)&(FIFOSIZ*2-1));
    }
}
