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

  Utility functions for the module loader

==============================================================================*/
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

#ifdef SUNOS
extern int fprintf(FILE *, const char *, ...);
#endif

/*========== Shared tracker identifiers */

CHAR *STM_Signatures[STM_NTRACKERS] = {
	"!Scream!",
	"BMOD2STM",
	"WUZAMOD!"
};

CHAR *STM_Version[STM_NTRACKERS] = {
	"Screamtracker 2",
	"Converted by MOD2STM (STM format)",
	"Wuzamod (STM format)"
};

/*========== Shared loader variables */

SBYTE  remap[UF_MAXCHAN];   /* for removing empty channels */
UBYTE* poslookup=NULL;      /* lookup table for pattern jumps after blank
                               pattern removal */
UBYTE  poslookupcnt;
UWORD* origpositions=NULL;

BOOL   filters;             /* resonant filters in use */
UBYTE  activemacro;         /* active midi macro number for Sxx,xx<80h */
UBYTE  filtermacros[UF_MAXMACRO];    /* midi macro settings */
FILTER filtersettings[UF_MAXFILTER]; /* computed filter settings */

/*========== Linear periods stuff */

int*   noteindex=NULL;      /* remap value for linear period modules */
static int noteindexcount=0;

int *AllocLinear(void)
{
	if(of.numsmp>noteindexcount) {
		noteindexcount=of.numsmp;
		noteindex=realloc(noteindex,noteindexcount*sizeof(int));
	}
	return noteindex;
}

void FreeLinear(void)
{
	if(noteindex) {
		free(noteindex);
		noteindex=NULL;
	}
	noteindexcount=0;
}

int speed_to_finetune(ULONG speed,int sample)
{
    int ctmp=0,tmp,note=1,finetune=0;

    speed>>=1;
    while((tmp=getfrequency(of.flags,getlinearperiod(note<<1,0)))<speed) {
        ctmp=tmp;
        note++;
    }

    if(tmp!=speed) {
        if((tmp-speed)<(speed-ctmp))
            while(tmp>speed)
                tmp=getfrequency(of.flags,getlinearperiod(note<<1,--finetune));
        else {
            note--;
            while(ctmp<speed)
                ctmp=getfrequency(of.flags,getlinearperiod(note<<1,++finetune));
        }
    }

    noteindex[sample]=note-4*OCTAVE;
    return finetune;
}

/*========== Order stuff */

/* handles S3M and IT orders */
void S3MIT_CreateOrders(BOOL curious)
{
	int t;

	of.numpos = 0;
	memset(of.positions,0,poslookupcnt*sizeof(UWORD));
	memset(poslookup,-1,256);
	for(t=0;t<poslookupcnt;t++) {
		int order=origpositions[t];
		if(order==255) order=LAST_PATTERN;
		of.positions[of.numpos]=order;
		poslookup[t]=of.numpos; /* bug fix for freaky S3Ms / ITs */
		if(origpositions[t]<254) of.numpos++;
		else
			/* end of song special order */
			if((order==LAST_PATTERN)&&(!(curious--))) break;
	}
}

/*========== Effect stuff */

/* handles S3M and IT effects */
void S3MIT_ProcessCmd(UBYTE cmd,UBYTE inf,unsigned int flags)
{
	UBYTE hi,lo;

	lo=inf&0xf;
	hi=inf>>4;

	/* process S3M / IT specific command structure */

	if(cmd!=255) {
		switch(cmd) {
			case 1: /* Axx set speed to xx */
				UniEffect(UNI_S3MEFFECTA,inf);
				break;
			case 2: /* Bxx position jump */
				if (inf<poslookupcnt) {
					/* switch to curious mode if necessary, for example
					   sympex.it, deep joy.it */
					if(((SBYTE)poslookup[inf]<0)&&(origpositions[inf]!=255))
						S3MIT_CreateOrders(1);

					if(!((SBYTE)poslookup[inf]<0))
						UniPTEffect(0xb,poslookup[inf]);
				}
				break;
			case 3: /* Cxx patternbreak to row xx */
				if ((flags & S3MIT_OLDSTYLE) && !(flags & S3MIT_IT))
					UniPTEffect(0xd,(inf>>4)*10+(inf&0xf));
				else
					UniPTEffect(0xd,inf);
				break;
			case 4: /* Dxy volumeslide */
				UniEffect(UNI_S3MEFFECTD,inf);
				break;
			case 5: /* Exy toneslide down */
				UniEffect(UNI_S3MEFFECTE,inf);
				break;
			case 6: /* Fxy toneslide up */
				UniEffect(UNI_S3MEFFECTF,inf);
				break;
			case 7: /* Gxx Tone portamento, speed xx */
				if (flags & S3MIT_OLDSTYLE)
					UniPTEffect(0x3,inf);
				else
					UniEffect(UNI_ITEFFECTG,inf);
				break;
			case 8: /* Hxy vibrato */
				if (flags & S3MIT_OLDSTYLE)
					UniPTEffect(0x4,inf);
				else
					UniEffect(UNI_ITEFFECTH,inf);
				break;
			case 9: /* Ixy tremor, ontime x, offtime y */
				if (flags & S3MIT_OLDSTYLE)
					UniEffect(UNI_S3MEFFECTI,inf);
				else                     
					UniEffect(UNI_ITEFFECTI,inf);
				break;
			case 0xa: /* Jxy arpeggio */
				UniPTEffect(0x0,inf);
				break;
			case 0xb: /* Kxy Dual command H00 & Dxy */
				if (flags & S3MIT_OLDSTYLE)
					UniPTEffect(0x4,0);    
				else
					UniEffect(UNI_ITEFFECTH,0);
				UniEffect(UNI_S3MEFFECTD,inf);
				break;
			case 0xc: /* Lxy Dual command G00 & Dxy */
				if (flags & S3MIT_OLDSTYLE)
					UniPTEffect(0x3,0);
				else
					UniEffect(UNI_ITEFFECTG,0);
				UniEffect(UNI_S3MEFFECTD,inf);
				break;
			case 0xd: /* Mxx Set Channel Volume */
				UniEffect(UNI_ITEFFECTM,inf);
				break;       
			case 0xe: /* Nxy Slide Channel Volume */
				UniEffect(UNI_ITEFFECTN,inf);
				break;
			case 0xf: /* Oxx set sampleoffset xx00h */
				UniPTEffect(0x9,inf);
				break;
			case 0x10: /* Pxy Slide Panning Commands */
				UniEffect(UNI_ITEFFECTP,inf);
				break;
			case 0x11: /* Qxy Retrig (+volumeslide) */
				UniWriteByte(UNI_S3MEFFECTQ);
				if(inf && !lo && !(flags & S3MIT_OLDSTYLE))
					UniWriteByte(1);
				else
					UniWriteByte(inf); 
				break;
			case 0x12: /* Rxy tremolo speed x, depth y */
				UniEffect(UNI_S3MEFFECTR,inf);
				break;
			case 0x13: /* Sxx special commands */
				if (inf>=0xf0) {
					/* change resonant filter settings if necessary */
					if((filters)&&((inf&0xf)!=activemacro)) {
						activemacro=inf&0xf;
						for(inf=0;inf<0x80;inf++)
							filtersettings[inf].filter=filtermacros[activemacro];
					}
				} else {
					/* Scream Tracker does not have samples larger than
					   64 Kb, thus doesn't need the SAx effect */
					if ((flags & S3MIT_SCREAM) && ((inf & 0xf0) == 0xa0))
						break;

					UniEffect(UNI_ITEFFECTS0,inf);
				}
				break;
			case 0x14: /* Txx tempo */
				if(inf>=0x20)
					UniEffect(UNI_S3MEFFECTT,inf);
				else {
					if(!(flags & S3MIT_OLDSTYLE))
						/* IT Tempo slide */
						UniEffect(UNI_ITEFFECTT,inf);
				}
				break;
			case 0x15: /* Uxy Fine Vibrato speed x, depth y */
				if(flags & S3MIT_OLDSTYLE)
					UniEffect(UNI_S3MEFFECTU,inf);
				else
					UniEffect(UNI_ITEFFECTU,inf);
				break;
			case 0x16: /* Vxx Set Global Volume */
				UniEffect(UNI_XMEFFECTG,inf);
				break;
			case 0x17: /* Wxy Global Volume Slide */
				UniEffect(UNI_ITEFFECTW,inf);
				break;
			case 0x18: /* Xxx amiga command 8xx */
				if(flags & S3MIT_OLDSTYLE) {
					if(inf>128)
						UniEffect(UNI_ITEFFECTS0,0x91); /* surround */
					else
						UniPTEffect(0x8,(inf==128)?255:(inf<<1));
				} else
					UniPTEffect(0x8,inf);
				break;
			case 0x19: /* Yxy Panbrello  speed x, depth y */
				UniEffect(UNI_ITEFFECTY,inf);
				break;
			case 0x1a: /* Zxx midi/resonant filters */
				if(filtersettings[inf].filter) {
					UniWriteByte(UNI_ITEFFECTZ);
					UniWriteByte(filtersettings[inf].filter);
					UniWriteByte(filtersettings[inf].inf);
				}
				break;
		}
	}
}

/*========== Unitrk stuff */

/* Generic effect writing routine */
void UniEffect(UWORD eff,UWORD dat)
{
	if((!eff)||(eff>=UNI_LAST)) return;

	UniWriteByte(eff);
	if(unioperands[eff]==2)
		UniWriteWord(dat);
	else
		UniWriteByte(dat);
}

/*  Appends UNI_PTEFFECTX opcode to the unitrk stream. */
void UniPTEffect(UBYTE eff, UBYTE dat)
{
#ifdef MIKMOD_DEBUG
	if (eff>=0x10)
		fprintf(stderr,"UniPTEffect called with incorrect eff value %d\n",eff);
	else
#endif
	if((eff)||(dat)||(of.flags&UF_ARPMEM)) UniEffect(UNI_PTEFFECT0+eff,dat);
}

/* Appends UNI_VOLEFFECT + effect/dat to unistream. */
void UniVolEffect(UWORD eff,UBYTE dat)
{
	if((eff)||(dat)) { /* don't write empty effect */
		UniWriteByte(UNI_VOLEFFECTS);
		UniWriteByte(eff);UniWriteByte(dat);
	}
}

/* ex:set ts=4: */
