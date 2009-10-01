/*	MikMod sound library
	(c) 1998, 1999 Miodrag Vallat and others - see file AUTHORS for
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

  $Id: mlutil.c,v 1.12 1999/10/25 16:31:41 miod Exp $

  Utility functions for the module loader

==============================================================================*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_MEMORY_H
#include <memory.h>
#endif

#include <string.h>
#include "unimod_priv.h"
extern void *safe_realloc(void *old_ptr, size_t new_size);

SBYTE  remap[64];           /* for removing empty channels */
UBYTE* poslookup=NULL;      /* lookup table for pattern jumps after blank
                               pattern removal */
UBYTE  poslookupcnt;
UWORD* origpositions=NULL;

BOOL   filters;             /* resonant filters in use */
UBYTE  activemacro;         /* active midi macro number for Sxx,xx<80h */
UBYTE  filtermacros[16];    /* midi macros settings */
FILTER filtersettings[256]; /* computed filter settings */

/* tracker identifiers */
CHAR *STM_Signatures[STM_NTRACKERS] =
{
  "!Scream!",
  "BMOD2STM",
  "WUZAMOD!"
};
CHAR *STM_Version[STM_NTRACKERS] =
{
  "Screamtracker 2",
  "Converted by MOD2STM (STM format)",
  "Wuzamod (STM format)"
};


/*========== Linear periods stuff */


/* Triton's linear periods to frequency translation table (for XM modules) */
static ULONG lintab[768] =
{
  535232, 534749, 534266, 533784, 533303, 532822, 532341, 531861,
  531381, 530902, 530423, 529944, 529466, 528988, 528511, 528034,
  527558, 527082, 526607, 526131, 525657, 525183, 524709, 524236,
  523763, 523290, 522818, 522346, 521875, 521404, 520934, 520464,
  519994, 519525, 519057, 518588, 518121, 517653, 517186, 516720,
  516253, 515788, 515322, 514858, 514393, 513929, 513465, 513002,
  512539, 512077, 511615, 511154, 510692, 510232, 509771, 509312,
  508852, 508393, 507934, 507476, 507018, 506561, 506104, 505647,
  505191, 504735, 504280, 503825, 503371, 502917, 502463, 502010,
  501557, 501104, 500652, 500201, 499749, 499298, 498848, 498398,
  497948, 497499, 497050, 496602, 496154, 495706, 495259, 494812,
  494366, 493920, 493474, 493029, 492585, 492140, 491696, 491253,
  490809, 490367, 489924, 489482, 489041, 488600, 488159, 487718,
  487278, 486839, 486400, 485961, 485522, 485084, 484647, 484210,
  483773, 483336, 482900, 482465, 482029, 481595, 481160, 480726,
  480292, 479859, 479426, 478994, 478562, 478130, 477699, 477268,
  476837, 476407, 475977, 475548, 475119, 474690, 474262, 473834,
  473407, 472979, 472553, 472126, 471701, 471275, 470850, 470425,
  470001, 469577, 469153, 468730, 468307, 467884, 467462, 467041,
  466619, 466198, 465778, 465358, 464938, 464518, 464099, 463681,
  463262, 462844, 462427, 462010, 461593, 461177, 460760, 460345,
  459930, 459515, 459100, 458686, 458272, 457859, 457446, 457033,
  456621, 456209, 455797, 455386, 454975, 454565, 454155, 453745,
  453336, 452927, 452518, 452110, 451702, 451294, 450887, 450481,
  450074, 449668, 449262, 448857, 448452, 448048, 447644, 447240,
  446836, 446433, 446030, 445628, 445226, 444824, 444423, 444022,
  443622, 443221, 442821, 442422, 442023, 441624, 441226, 440828,
  440430, 440033, 439636, 439239, 438843, 438447, 438051, 437656,
  437261, 436867, 436473, 436079, 435686, 435293, 434900, 434508,
  434116, 433724, 433333, 432942, 432551, 432161, 431771, 431382,
  430992, 430604, 430215, 429827, 429439, 429052, 428665, 428278,
  427892, 427506, 427120, 426735, 426350, 425965, 425581, 425197,
  424813, 424430, 424047, 423665, 423283, 422901, 422519, 422138,
  421757, 421377, 420997, 420617, 420237, 419858, 419479, 419101,
  418723, 418345, 417968, 417591, 417214, 416838, 416462, 416086,
  415711, 415336, 414961, 414586, 414212, 413839, 413465, 413092,
  412720, 412347, 411975, 411604, 411232, 410862, 410491, 410121,
  409751, 409381, 409012, 408643, 408274, 407906, 407538, 407170,
  406803, 406436, 406069, 405703, 405337, 404971, 404606, 404241,
  403876, 403512, 403148, 402784, 402421, 402058, 401695, 401333,
  400970, 400609, 400247, 399886, 399525, 399165, 398805, 398445,
  398086, 397727, 397368, 397009, 396651, 396293, 395936, 395579,
  395222, 394865, 394509, 394153, 393798, 393442, 393087, 392733,
  392378, 392024, 391671, 391317, 390964, 390612, 390259, 389907,
  389556, 389204, 388853, 388502, 388152, 387802, 387452, 387102,
  386753, 386404, 386056, 385707, 385359, 385012, 384664, 384317,
  383971, 383624, 383278, 382932, 382587, 382242, 381897, 381552,
  381208, 380864, 380521, 380177, 379834, 379492, 379149, 378807,
  378466, 378124, 377783, 377442, 377102, 376762, 376422, 376082,
  375743, 375404, 375065, 374727, 374389, 374051, 373714, 373377,
  373040, 372703, 372367, 372031, 371695, 371360, 371025, 370690,
  370356, 370022, 369688, 369355, 369021, 368688, 368356, 368023,
  367691, 367360, 367028, 366697, 366366, 366036, 365706, 365376,
  365046, 364717, 364388, 364059, 363731, 363403, 363075, 362747,
  362420, 362093, 361766, 361440, 361114, 360788, 360463, 360137,
  359813, 359488, 359164, 358840, 358516, 358193, 357869, 357547,
  357224, 356902, 356580, 356258, 355937, 355616, 355295, 354974,
  354654, 354334, 354014, 353695, 353376, 353057, 352739, 352420,
  352103, 351785, 351468, 351150, 350834, 350517, 350201, 349885,
  349569, 349254, 348939, 348624, 348310, 347995, 347682, 347368,
  347055, 346741, 346429, 346116, 345804, 345492, 345180, 344869,
  344558, 344247, 343936, 343626, 343316, 343006, 342697, 342388,
  342079, 341770, 341462, 341154, 340846, 340539, 340231, 339924,
  339618, 339311, 339005, 338700, 338394, 338089, 337784, 337479,
  337175, 336870, 336566, 336263, 335959, 335656, 335354, 335051,
  334749, 334447, 334145, 333844, 333542, 333242, 332941, 332641,
  332341, 332041, 331741, 331442, 331143, 330844, 330546, 330247,
  329950, 329652, 329355, 329057, 328761, 328464, 328168, 327872,
  327576, 327280, 326985, 326690, 326395, 326101, 325807, 325513,
  325219, 324926, 324633, 324340, 324047, 323755, 323463, 323171,
  322879, 322588, 322297, 322006, 321716, 321426, 321136, 320846,
  320557, 320267, 319978, 319690, 319401, 319113, 318825, 318538,
  318250, 317963, 317676, 317390, 317103, 316817, 316532, 316246,
  315961, 315676, 315391, 315106, 314822, 314538, 314254, 313971,
  313688, 313405, 313122, 312839, 312557, 312275, 311994, 311712,
  311431, 311150, 310869, 310589, 310309, 310029, 309749, 309470,
  309190, 308911, 308633, 308354, 308076, 307798, 307521, 307243,
  306966, 306689, 306412, 306136, 305860, 305584, 305308, 305033,
  304758, 304483, 304208, 303934, 303659, 303385, 303112, 302838,
  302565, 302292, 302019, 301747, 301475, 301203, 300931, 300660,
  300388, 300117, 299847, 299576, 299306, 299036, 298766, 298497,
  298227, 297958, 297689, 297421, 297153, 296884, 296617, 296349,
  296082, 295815, 295548, 295281, 295015, 294749, 294483, 294217,
  293952, 293686, 293421, 293157, 292892, 292628, 292364, 292100,
  291837, 291574, 291311, 291048, 290785, 290523, 290261, 289999,
  289737, 289476, 289215, 288954, 288693, 288433, 288173, 287913,
  287653, 287393, 287134, 286875, 286616, 286358, 286099, 285841,
  285583, 285326, 285068, 284811, 284554, 284298, 284041, 283785,
  283529, 283273, 283017, 282762, 282507, 282252, 281998, 281743,
  281489, 281235, 280981, 280728, 280475, 280222, 279969, 279716,
  279464, 279212, 278960, 278708, 278457, 278206, 277955, 277704,
  277453, 277203, 276953, 276703, 276453, 276204, 275955, 275706,
  275457, 275209, 274960, 274712, 274465, 274217, 273970, 273722,
  273476, 273229, 272982, 272736, 272490, 272244, 271999, 271753,
  271508, 271263, 271018, 270774, 270530, 270286, 270042, 269798,
  269555, 269312, 269069, 268826, 268583, 268341, 268099, 267857
};

static UWORD oldperiods[OCTAVE * 2] =
{
  1712 * 16, 1664 * 16, 1616 * 16, 1570 * 16, 1524 * 16, 1480 * 16,
  1438 * 16, 1396 * 16, 1356 * 16, 1318 * 16, 1280 * 16, 1244 * 16,
  1208 * 16, 1174 * 16, 1140 * 16, 1108 * 16, 1076 * 16, 1046 * 16,
  1016 * 16, 988 * 16, 960 * 16, 932 * 16, 906 * 16, 880 * 16
};

#define LOGFAC 2*16
static UWORD logtab[104] =
{
  LOGFAC * 907, LOGFAC * 900, LOGFAC * 894, LOGFAC * 887,
  LOGFAC * 881, LOGFAC * 875, LOGFAC * 868, LOGFAC * 862,
  LOGFAC * 856, LOGFAC * 850, LOGFAC * 844, LOGFAC * 838,
  LOGFAC * 832, LOGFAC * 826, LOGFAC * 820, LOGFAC * 814,
  LOGFAC * 808, LOGFAC * 802, LOGFAC * 796, LOGFAC * 791,
  LOGFAC * 785, LOGFAC * 779, LOGFAC * 774, LOGFAC * 768,
  LOGFAC * 762, LOGFAC * 757, LOGFAC * 752, LOGFAC * 746,
  LOGFAC * 741, LOGFAC * 736, LOGFAC * 730, LOGFAC * 725,
  LOGFAC * 720, LOGFAC * 715, LOGFAC * 709, LOGFAC * 704,
  LOGFAC * 699, LOGFAC * 694, LOGFAC * 689, LOGFAC * 684,
  LOGFAC * 678, LOGFAC * 675, LOGFAC * 670, LOGFAC * 665,
  LOGFAC * 660, LOGFAC * 655, LOGFAC * 651, LOGFAC * 646,
  LOGFAC * 640, LOGFAC * 636, LOGFAC * 632, LOGFAC * 628,
  LOGFAC * 623, LOGFAC * 619, LOGFAC * 614, LOGFAC * 610,
  LOGFAC * 604, LOGFAC * 601, LOGFAC * 597, LOGFAC * 592,
  LOGFAC * 588, LOGFAC * 584, LOGFAC * 580, LOGFAC * 575,
  LOGFAC * 570, LOGFAC * 567, LOGFAC * 563, LOGFAC * 559,
  LOGFAC * 555, LOGFAC * 551, LOGFAC * 547, LOGFAC * 543,
  LOGFAC * 538, LOGFAC * 535, LOGFAC * 532, LOGFAC * 528,
  LOGFAC * 524, LOGFAC * 520, LOGFAC * 516, LOGFAC * 513,
  LOGFAC * 508, LOGFAC * 505, LOGFAC * 502, LOGFAC * 498,
  LOGFAC * 494, LOGFAC * 491, LOGFAC * 487, LOGFAC * 484,
  LOGFAC * 480, LOGFAC * 477, LOGFAC * 474, LOGFAC * 470,
  LOGFAC * 467, LOGFAC * 463, LOGFAC * 460, LOGFAC * 457,
  LOGFAC * 453, LOGFAC * 450, LOGFAC * 447, LOGFAC * 443,
  LOGFAC * 440, LOGFAC * 437, LOGFAC * 434, LOGFAC * 431
};


int*   noteindex=NULL;      /* remap value for linear period modules */
static int noteindexcount=0;

int *AllocLinear(void)
{
	if(of.numsmp>noteindexcount) {
		noteindexcount=of.numsmp;
		noteindex=safe_realloc(noteindex,noteindexcount*sizeof(int));
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

static SWORD 
Interpolate (SWORD p, SWORD p1, SWORD p2, SWORD v1, SWORD v2)
{
  if ((p1 == p2) || (p == p1))
    return v1;
  return v1 + ((SLONG) ((p - p1) * (v2 - v1)) / (p2 - p1));
}


UWORD 
getlinearperiod (UWORD note, ULONG fine)
{
  UWORD t;

  t = (20L * OCTAVE + 2 - note) * 32L - (fine >> 1);
  return t;
}


/* XM linear period to MOD period conversion */
ULONG getfrequency (UBYTE flags, ULONG period)
{
  if (flags & UF_LINEAR)
    return lintab[period % 768] >> (period / 768);
  else
    return (8363L * 1712L) / (period ? period : 1);
}

UWORD getlogperiod (UWORD note, ULONG fine)
{
  UWORD n, o;
  UWORD p1, p2;
  ULONG i;

  n = note % (2 * OCTAVE);
  o = note / (2 * OCTAVE);
  i = (n << 2) + (fine >> 4);	/* n*8 + fine/16 */

  p1 = logtab[i];
  p2 = logtab[i + 1];

  return (Interpolate (fine >> 4, 0, 15, p1, p2) >> o);
}

UWORD getoldperiod (UWORD note, ULONG speed)
{
  UWORD n, o;

  if (!speed)
    {
#ifdef MIKMOD_DEBUG
      fprintf (stderr, "\rmplayer: getoldperiod() called with note=%d, speed=0 !\n", note);
#endif
      return 4242;		/* <- prevent divide overflow.. (42 hehe) */
    }

  n = note % (2 * OCTAVE);
  o = note / (2 * OCTAVE);
  return ((8363L * (ULONG) oldperiods[n]) >> o) / speed;
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

/* XM linear period to MOD period conversion */
ULONG getAmigaPeriod (UBYTE flags, ULONG period)
{
  if (flags & UF_LINEAR)
    {
      period = lintab[period % 768] >> (period / 768);
      period = (8363L * 1712L) / period;
    }

  return (period);
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
		of.positions[of.numpos]=origpositions[t];
		poslookup[t]=of.numpos; /* bug fix for freaky S3Ms / ITs */
		if(origpositions[t]<254) of.numpos++;
		else
			/* end of song special order */
			if((origpositions[t]==255)&&(!(curious--))) break;
	}
}

/*========== Effect stuff */

/* handles S3M and IT effects */
void S3MIT_ProcessCmd(UBYTE cmd,UBYTE inf,BOOL oldeffect)
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
				if(oldeffect==1)
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
				UniEffect(UNI_ITEFFECTG,inf);
				break;
			case 8: /* Hxy vibrato */
				if(oldeffect&1)
					UniPTEffect(0x4,inf);
				else
					UniEffect(UNI_ITEFFECTH,inf);
				break;
			case 9: /* Ixy tremor, ontime x, offtime y */
				if(oldeffect&1)
					UniEffect(UNI_S3MEFFECTI,inf);
				else                     
					UniEffect(UNI_ITEFFECTI,inf);
				break;
			case 0xa: /* Jxy arpeggio */
				UniPTEffect(0x0,inf);
				break;
			case 0xb: /* Kxy Dual command H00 & Dxy */
				if(oldeffect&1)
					UniPTEffect(0x4,0);    
				else
					UniEffect(UNI_ITEFFECTH,0);
				UniEffect(UNI_S3MEFFECTD,inf);
				break;
			case 0xc: /* Lxy Dual command G00 & Dxy */
				if(oldeffect&1)
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
				if(inf && !lo && !(oldeffect&1))
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
				} else
					UniEffect(UNI_ITEFFECTS0,inf);
				break;
			case 0x14: /* Txx tempo */
				if(inf>=0x20)
					UniEffect(UNI_S3MEFFECTT,inf);
				else {
					if(!(oldeffect&1))
						/* IT Tempo slide */
						UniEffect(UNI_ITEFFECTT,inf);
				}
				break;
			case 0x15: /* Uxy Fine Vibrato speed x, depth y */
				if(oldeffect&1)
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
				if(oldeffect&1) {
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
	if((eff)||(dat)||(of.flags & UF_ARPMEM)) UniEffect(UNI_PTEFFECT0+eff,dat);
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
