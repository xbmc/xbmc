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

  The Protracker Player Driver

  The protracker driver supports all base Protracker 3.x commands and features.

==============================================================================*/
#include "xbsection_start.h"


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include <stdarg.h>
#ifdef SRANDOM_IN_MATH_H
#include <math.h>
#else
#include <stdlib.h>
#endif
#undef XB_LOG
#include "mikmod.h"
#include "mikmod_internals.h"

#ifdef SUNOS
extern int fprintf(FILE *, const char *, ...);
extern long int random(void);
#endif

/* The currently playing module */
/* This variable should better be static, but it would break the ABI, so this
   will wait */
/*static*/ MODULE *pf = NULL;

#define	HIGH_OCTAVE		2	/* number of above-range octaves */

static	UWORD oldperiods[OCTAVE*2]={
	0x6b00, 0x6800, 0x6500, 0x6220, 0x5f50, 0x5c80,
	0x5a00, 0x5740, 0x54d0, 0x5260, 0x5010, 0x4dc0,
	0x4b90, 0x4960, 0x4750, 0x4540, 0x4350, 0x4160,
	0x3f90, 0x3dc0, 0x3c10, 0x3a40, 0x38b0, 0x3700
};

static	UBYTE VibratoTable[32]={
	  0, 24, 49, 74, 97,120,141,161,180,197,212,224,235,244,250,253,
	255,253,250,244,235,224,212,197,180,161,141,120, 97, 74, 49, 24
};

static	UBYTE avibtab[128]={
	 0, 1, 3, 4, 6, 7, 9,10,12,14,15,17,18,20,21,23,
	24,25,27,28,30,31,32,34,35,36,38,39,40,41,42,44,
	45,46,47,48,49,50,51,52,53,54,54,55,56,57,57,58,
	59,59,60,60,61,61,62,62,62,63,63,63,63,63,63,63,
	64,63,63,63,63,63,63,63,62,62,62,61,61,60,60,59,
	59,58,57,57,56,55,54,54,53,52,51,50,49,48,47,46,
	45,44,42,41,40,39,38,36,35,34,32,31,30,28,27,25,
	24,23,21,20,18,17,15,14,12,10, 9, 7, 6, 4, 3, 1
};

/* Triton's linear periods to frequency translation table (for XM modules) */
static	ULONG lintab[768]={
	535232,534749,534266,533784,533303,532822,532341,531861,
	531381,530902,530423,529944,529466,528988,528511,528034,
	527558,527082,526607,526131,525657,525183,524709,524236,
	523763,523290,522818,522346,521875,521404,520934,520464,
	519994,519525,519057,518588,518121,517653,517186,516720,
	516253,515788,515322,514858,514393,513929,513465,513002,
	512539,512077,511615,511154,510692,510232,509771,509312,
	508852,508393,507934,507476,507018,506561,506104,505647,
	505191,504735,504280,503825,503371,502917,502463,502010,
	501557,501104,500652,500201,499749,499298,498848,498398,
	497948,497499,497050,496602,496154,495706,495259,494812,
	494366,493920,493474,493029,492585,492140,491696,491253,
	490809,490367,489924,489482,489041,488600,488159,487718,
	487278,486839,486400,485961,485522,485084,484647,484210,
	483773,483336,482900,482465,482029,481595,481160,480726,
	480292,479859,479426,478994,478562,478130,477699,477268,
	476837,476407,475977,475548,475119,474690,474262,473834,
	473407,472979,472553,472126,471701,471275,470850,470425,
	470001,469577,469153,468730,468307,467884,467462,467041,
	466619,466198,465778,465358,464938,464518,464099,463681,
	463262,462844,462427,462010,461593,461177,460760,460345,
	459930,459515,459100,458686,458272,457859,457446,457033,
	456621,456209,455797,455386,454975,454565,454155,453745,
	453336,452927,452518,452110,451702,451294,450887,450481,
	450074,449668,449262,448857,448452,448048,447644,447240,
	446836,446433,446030,445628,445226,444824,444423,444022,
	443622,443221,442821,442422,442023,441624,441226,440828,
	440430,440033,439636,439239,438843,438447,438051,437656,
	437261,436867,436473,436079,435686,435293,434900,434508,
	434116,433724,433333,432942,432551,432161,431771,431382,
	430992,430604,430215,429827,429439,429052,428665,428278,
	427892,427506,427120,426735,426350,425965,425581,425197,
	424813,424430,424047,423665,423283,422901,422519,422138,
	421757,421377,420997,420617,420237,419858,419479,419101,
	418723,418345,417968,417591,417214,416838,416462,416086,
	415711,415336,414961,414586,414212,413839,413465,413092,
	412720,412347,411975,411604,411232,410862,410491,410121,
	409751,409381,409012,408643,408274,407906,407538,407170,
	406803,406436,406069,405703,405337,404971,404606,404241,
	403876,403512,403148,402784,402421,402058,401695,401333,
	400970,400609,400247,399886,399525,399165,398805,398445,
	398086,397727,397368,397009,396651,396293,395936,395579,
	395222,394865,394509,394153,393798,393442,393087,392733,
	392378,392024,391671,391317,390964,390612,390259,389907,
	389556,389204,388853,388502,388152,387802,387452,387102,
	386753,386404,386056,385707,385359,385012,384664,384317,
	383971,383624,383278,382932,382587,382242,381897,381552,
	381208,380864,380521,380177,379834,379492,379149,378807,
	378466,378124,377783,377442,377102,376762,376422,376082,
	375743,375404,375065,374727,374389,374051,373714,373377,
	373040,372703,372367,372031,371695,371360,371025,370690,
	370356,370022,369688,369355,369021,368688,368356,368023,
	367691,367360,367028,366697,366366,366036,365706,365376,
	365046,364717,364388,364059,363731,363403,363075,362747,
	362420,362093,361766,361440,361114,360788,360463,360137,
	359813,359488,359164,358840,358516,358193,357869,357547,
	357224,356902,356580,356258,355937,355616,355295,354974,
	354654,354334,354014,353695,353376,353057,352739,352420,
	352103,351785,351468,351150,350834,350517,350201,349885,
	349569,349254,348939,348624,348310,347995,347682,347368,
	347055,346741,346429,346116,345804,345492,345180,344869,
	344558,344247,343936,343626,343316,343006,342697,342388,
	342079,341770,341462,341154,340846,340539,340231,339924,
	339618,339311,339005,338700,338394,338089,337784,337479,
	337175,336870,336566,336263,335959,335656,335354,335051,
	334749,334447,334145,333844,333542,333242,332941,332641,
	332341,332041,331741,331442,331143,330844,330546,330247,
	329950,329652,329355,329057,328761,328464,328168,327872,
	327576,327280,326985,326690,326395,326101,325807,325513,
	325219,324926,324633,324340,324047,323755,323463,323171,
	322879,322588,322297,322006,321716,321426,321136,320846,
	320557,320267,319978,319690,319401,319113,318825,318538,
	318250,317963,317676,317390,317103,316817,316532,316246,
	315961,315676,315391,315106,314822,314538,314254,313971,
	313688,313405,313122,312839,312557,312275,311994,311712,
	311431,311150,310869,310589,310309,310029,309749,309470,
	309190,308911,308633,308354,308076,307798,307521,307243,
	306966,306689,306412,306136,305860,305584,305308,305033,
	304758,304483,304208,303934,303659,303385,303112,302838,
	302565,302292,302019,301747,301475,301203,300931,300660,
	300388,300117,299847,299576,299306,299036,298766,298497,
	298227,297958,297689,297421,297153,296884,296617,296349,
	296082,295815,295548,295281,295015,294749,294483,294217,
	293952,293686,293421,293157,292892,292628,292364,292100,
	291837,291574,291311,291048,290785,290523,290261,289999,
	289737,289476,289215,288954,288693,288433,288173,287913,
	287653,287393,287134,286875,286616,286358,286099,285841,
	285583,285326,285068,284811,284554,284298,284041,283785,
	283529,283273,283017,282762,282507,282252,281998,281743,
	281489,281235,280981,280728,280475,280222,279969,279716,
	279464,279212,278960,278708,278457,278206,277955,277704,
	277453,277203,276953,276703,276453,276204,275955,275706,
	275457,275209,274960,274712,274465,274217,273970,273722,
	273476,273229,272982,272736,272490,272244,271999,271753,
	271508,271263,271018,270774,270530,270286,270042,269798,
	269555,269312,269069,268826,268583,268341,268099,267857
};

#define LOGFAC 2*16
static	UWORD logtab[104]={
	LOGFAC*907,LOGFAC*900,LOGFAC*894,LOGFAC*887,
	LOGFAC*881,LOGFAC*875,LOGFAC*868,LOGFAC*862,
	LOGFAC*856,LOGFAC*850,LOGFAC*844,LOGFAC*838,
	LOGFAC*832,LOGFAC*826,LOGFAC*820,LOGFAC*814,
	LOGFAC*808,LOGFAC*802,LOGFAC*796,LOGFAC*791,
	LOGFAC*785,LOGFAC*779,LOGFAC*774,LOGFAC*768,
	LOGFAC*762,LOGFAC*757,LOGFAC*752,LOGFAC*746,
	LOGFAC*741,LOGFAC*736,LOGFAC*730,LOGFAC*725,
	LOGFAC*720,LOGFAC*715,LOGFAC*709,LOGFAC*704,
	LOGFAC*699,LOGFAC*694,LOGFAC*689,LOGFAC*684,
	LOGFAC*678,LOGFAC*675,LOGFAC*670,LOGFAC*665,
	LOGFAC*660,LOGFAC*655,LOGFAC*651,LOGFAC*646,
	LOGFAC*640,LOGFAC*636,LOGFAC*632,LOGFAC*628,
	LOGFAC*623,LOGFAC*619,LOGFAC*614,LOGFAC*610,
	LOGFAC*604,LOGFAC*601,LOGFAC*597,LOGFAC*592,
	LOGFAC*588,LOGFAC*584,LOGFAC*580,LOGFAC*575,
	LOGFAC*570,LOGFAC*567,LOGFAC*563,LOGFAC*559,
	LOGFAC*555,LOGFAC*551,LOGFAC*547,LOGFAC*543,
	LOGFAC*538,LOGFAC*535,LOGFAC*532,LOGFAC*528,
	LOGFAC*524,LOGFAC*520,LOGFAC*516,LOGFAC*513,
	LOGFAC*508,LOGFAC*505,LOGFAC*502,LOGFAC*498,
	LOGFAC*494,LOGFAC*491,LOGFAC*487,LOGFAC*484,
	LOGFAC*480,LOGFAC*477,LOGFAC*474,LOGFAC*470,
	LOGFAC*467,LOGFAC*463,LOGFAC*460,LOGFAC*457,
	LOGFAC*453,LOGFAC*450,LOGFAC*447,LOGFAC*443,
	LOGFAC*440,LOGFAC*437,LOGFAC*434,LOGFAC*431
};

static	SBYTE PanbrelloTable[256]={
	  0,  2,  3,  5,  6,  8,  9, 11, 12, 14, 16, 17, 19, 20, 22, 23,
	 24, 26, 27, 29, 30, 32, 33, 34, 36, 37, 38, 39, 41, 42, 43, 44,
	 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 56, 57, 58, 59,
	 59, 60, 60, 61, 61, 62, 62, 62, 63, 63, 63, 64, 64, 64, 64, 64,
	 64, 64, 64, 64, 64, 64, 63, 63, 63, 62, 62, 62, 61, 61, 60, 60,
	 59, 59, 58, 57, 56, 56, 55, 54, 53, 52, 51, 50, 49, 48, 47, 46,
	 45, 44, 43, 42, 41, 39, 38, 37, 36, 34, 33, 32, 30, 29, 27, 26,
	 24, 23, 22, 20, 19, 17, 16, 14, 12, 11,  9,  8,  6,  5,  3,  2,
	  0,- 2,- 3,- 5,- 6,- 8,- 9,-11,-12,-14,-16,-17,-19,-20,-22,-23,
	-24,-26,-27,-29,-30,-32,-33,-34,-36,-37,-38,-39,-41,-42,-43,-44,
	-45,-46,-47,-48,-49,-50,-51,-52,-53,-54,-55,-56,-56,-57,-58,-59,
	-59,-60,-60,-61,-61,-62,-62,-62,-63,-63,-63,-64,-64,-64,-64,-64,
	-64,-64,-64,-64,-64,-64,-63,-63,-63,-62,-62,-62,-61,-61,-60,-60,
	-59,-59,-58,-57,-56,-56,-55,-54,-53,-52,-51,-50,-49,-48,-47,-46,
	-45,-44,-43,-42,-41,-39,-38,-37,-36,-34,-33,-32,-30,-29,-27,-26,
	-24,-23,-22,-20,-19,-17,-16,-14,-12,-11,- 9,- 8,- 6,- 5,- 3,- 2
};

static void Mod_Player_Reinit_internal(MODULE* mod);
static void Mod_Player_Init_internal(MODULE* mod);

/* returns a random value between 0 and ceil-1, ceil must be a power of two */
static int getrandom(int ceil)
{
#ifdef HAVE_SRANDOM
	return random()&(ceil-1);
#else
	return (rand()*ceil)/(RAND_MAX+1.0);
#endif
}

/*	New Note Action Scoring System :
	--------------------------------
	1)	total-volume (fadevol, chanvol, volume) is the main scorer.
	2)	a looping sample is a bonus x2
	3)	a foreground channel is a bonus x4
	4)	an active envelope with keyoff is a handicap -x2
*/
static int MP_FindEmptyChannel(MODULE *mod)
{
	MP_VOICE *a;
	ULONG t,k,tvol,pp;

	for (t=0;t<md_sngchn;t++)
		if (((mod->voice[t].main.kick==KICK_ABSENT)||
			 (mod->voice[t].main.kick==KICK_ENV))&&
		   Voice_Stopped_internal(t))
			return t;

	tvol=0xffffffUL;t=0;a=mod->voice;
	for (k=0;k<md_sngchn;k++,a++) {
		/* allow us to take over a nonexisting sample */
		if (!a->main.s)
			return k;

		if ((a->main.kick==KICK_ABSENT)||(a->main.kick==KICK_ENV)) {
			pp=a->totalvol<<((a->main.s->flags&SF_LOOP)?1:0);
			if ((a->master)&&(a==a->master->slave))
				pp<<=2;

			if (pp<tvol) {
				tvol=pp;
				t=k;
			}
		}
	}

	if (tvol>8000*7) return -1;
	return t;
}

static SWORD Interpolate(SWORD p,SWORD p1,SWORD p2,SWORD v1,SWORD v2)
{
	if ((p1==p2)||(p==p1)) return v1;
	return v1+((SLONG)((p-p1)*(v2-v1))/(p2-p1));
}

UWORD getlinearperiod(UWORD note,ULONG fine)
{
	UWORD t;

	t=((20L+2*HIGH_OCTAVE)*OCTAVE+2-note)*32L-(fine>>1);
	return t;
}

static UWORD getlogperiod(UWORD note,ULONG fine)
{
	UWORD n,o;
	UWORD p1,p2;
	ULONG i;

	n=note%(2*OCTAVE);
	o=note/(2*OCTAVE);
	i=(n<<2)+(fine>>4); /* n*8 + fine/16 */

	p1=logtab[i];
	p2=logtab[i+1];

	return (Interpolate(fine>>4,0,15,p1,p2)>>o);
}

static UWORD getoldperiod(UWORD note,ULONG speed)
{
	UWORD n,o;

	/* This happens sometimes on badly converted AMF, and old MOD */
	if (!speed) {
#ifdef MIKMOD_DEBUG
		fprintf(stderr,"\rmplayer: getoldperiod() called with note=%d, speed=0 !\n",note);
#endif
		return 4242; /* <- prevent divide overflow.. (42 hehe) */
	}

	n=note%(2*OCTAVE);
	o=note/(2*OCTAVE);
	return ((8363L*(ULONG)oldperiods[n])>>o)/speed;
}

static UWORD GetPeriod(UWORD flags, UWORD note, ULONG speed)
{
	if (flags & UF_XMPERIODS) {
		if (flags & UF_LINEAR)
				return getlinearperiod(note, speed);
		else
				return getlogperiod(note, speed);
	} else
		return getoldperiod(note, speed);
}

static SWORD InterpolateEnv(SWORD p,ENVPT *a,ENVPT *b)
{
	return (Interpolate(p,a->pos,b->pos,a->val,b->val));
}

static SWORD DoPan(SWORD envpan,SWORD pan)
{
	int newpan;

	newpan=pan+(((envpan-PAN_CENTER)*(128-abs(pan-PAN_CENTER)))/128);

	return (newpan<PAN_LEFT)?PAN_LEFT:(newpan>PAN_RIGHT?PAN_RIGHT:newpan);
}

static SWORD StartEnvelope(ENVPR *t,UBYTE flg,UBYTE pts,UBYTE susbeg,UBYTE susend,UBYTE beg,UBYTE end,ENVPT *p,UBYTE keyoff)
{
	t->flg=flg;
	t->pts=pts;
	t->susbeg=susbeg;
	t->susend=susend;
	t->beg=beg;
	t->end=end;
	t->env=p;
	t->p=0;
	t->a=0;
	t->b=((t->flg&EF_SUSTAIN)&&(!(keyoff&KEY_OFF)))?0:1;

	/* Imago Orpheus sometimes stores an extra initial point in the envelope */
	if ((t->pts>=2)&&(t->env[0].pos==t->env[1].pos)) {
		t->a++;t->b++;
	}

	/* Fit in the envelope, still */
	if (t->a >= t->pts)
		t->a = t->pts - 1;
	if (t->b >= t->pts)
		t->b = t->pts-1;

	return t->env[t->a].val;
}

/* This procedure processes all envelope types, include volume, pitch, and
   panning.  Envelopes are defined by a set of points, each with a magnitude
   [relating either to volume, panning position, or pitch modifier] and a tick
   position.

   Envelopes work in the following manner:

   (a) Each tick the envelope is moved a point further in its progression. For
       an accurate progression, magnitudes between two envelope points are
       interpolated.

   (b) When progression reaches a defined point on the envelope, values are
       shifted to interpolate between this point and the next, and checks for
       loops or envelope end are done.

   Misc:
     Sustain loops are loops that are only active as long as the keyoff flag is
     clear.  When a volume envelope terminates, so does the current fadeout.
*/
static SWORD ProcessEnvelope(MP_VOICE *aout, ENVPR *t, SWORD v)
{
	if (t->flg & EF_ON) {
		UBYTE a, b;		/* actual points in the envelope */
		UWORD p;		/* the 'tick counter' - real point being played */

		a = t->a;
		b = t->b;
		p = t->p;

		/*
		 * Sustain loop on one point (XM type).
		 * Not processed if KEYOFF.
		 * Don't move and don't interpolate when the point is reached
		 */
		if ((t->flg & EF_SUSTAIN) && t->susbeg == t->susend &&
		   (!(aout->main.keyoff & KEY_OFF) && p == t->env[t->susbeg].pos)) {
			v = t->env[t->susbeg].val;
		} else {
			/*
			 * All following situations will require interpolation between
			 * two envelope points.
			 */

			/*
			 * Sustain loop between two points (IT type).
			 * Not processed if KEYOFF.
			 */
			/* if we were on a loop point, loop now */
			if ((t->flg & EF_SUSTAIN) && !(aout->main.keyoff & KEY_OFF) &&
			   a >= t->susend) {
				a = t->susbeg;
				b = (t->susbeg==t->susend)?a:a+1;
				p = t->env[a].pos;
				v = t->env[a].val;
			} else
			/*
			 * Regular loop.
			 * Be sure to correctly handle single point loops.
			 */
			if ((t->flg & EF_LOOP) && a >= t->end) {
				a = t->beg;
				b = t->beg == t->end ? a : a + 1;
				p = t->env[a].pos;
				v = t->env[a].val;
			} else
			/*
			 * Non looping situations.
			 * Start to fade if the volume envelope is finished.
			 */
			if (p > t->env[t->pts - 1].pos) {
				v = t->env[a].val;
				if (t->flg & EF_VOLENV) {
					aout->main.keyoff |= KEY_FADE;
					if (!v)
						aout->main.fadevol = 0;
				}
			} else {
				/*
				 * Regular processing : compute value, progress one step.
				 */
				if (a != b)
					v = InterpolateEnv(p, &t->env[a], &t->env[b]);
				else
					v = t->env[a].val;

				p++;
				/* did pointer reach point b? */
				if (p >= t->env[b].pos)
					a = b++; /* shift points a and b */
			}
			t->a = a;
			t->b = b;
			t->p = p;
		}
	}
	return v;
}

/* XM linear period to frequency conversion */
ULONG getfrequency(UWORD flags,ULONG period)
{
	if (flags & UF_LINEAR) {
		SLONG shift = ((SLONG)period / 768) - HIGH_OCTAVE;

		if (shift >= 0)
			return lintab[period % 768] >> shift;
		else
			return lintab[period % 768] << (-shift);
	} else
		return (8363L*1712L)/(period?period:1);
}

/*========== Protracker effects */

static void DoArpeggio(UWORD tick, UWORD flags, MP_CONTROL *a, UBYTE style)
{
	UBYTE note=a->main.note;

	if (a->arpmem) {
		switch (style) {
		case 0:		/* mod style: N, N+x, N+y */
			switch (tick % 3) {
			/* case 0: unchanged */
			case 1:
				note += (a->arpmem >> 4);
				break;
			case 2:
				note += (a->arpmem & 0xf);
				break;
			}
			break;
		case 3:		/* okt arpeggio 3: N-x, N, N+y */
			switch (tick % 3) {
			case 0:
				note -= (a->arpmem >> 4);
				break;
			/* case 1: unchanged */
			case 2:
				note += (a->arpmem & 0xf);
				break;
			}
			break;
		case 4:		/* okt arpeggio 4: N, N+y, N, N-x */
			switch (tick % 4) {
			/* case 0, case 2: unchanged */
			case 1:
				note += (a->arpmem & 0xf);
				break;
			case 3:
				note -= (a->arpmem >> 4);
				break;
			}
			break;
		case 5:		/* okt arpeggio 5: N-x, N+y, N, and nothing at tick 0 */
			if (!tick)
				break;
			switch (tick % 3) {
			/* case 0: unchanged */
			case 1:
				note -= (a->arpmem >> 4);
				break;
			case 2:
				note += (a->arpmem & 0xf);
				break;
			}
			break;
		}
		a->main.period = GetPeriod(flags, (UWORD)note << 1, a->speed);
		a->ownper = 1;
	}
}

static int DoPTEffect0(UWORD tick, UWORD flags, MP_CONTROL *a, MODULE *mod, SWORD channel)
{
	UBYTE dat;

	dat = UniGetByte();
	if (!tick) {
		if (!dat && (flags & UF_ARPMEM))
			dat=a->arpmem;
		else
			a->arpmem=dat;
	}
	if (a->main.period)
		DoArpeggio(tick, flags, a, 0);

	return 0;
}

static int DoPTEffect1(UWORD tick, UWORD flags, MP_CONTROL *a, MODULE *mod, SWORD channel)
{
	UBYTE dat;

	dat = UniGetByte();
	if (!tick && dat)
		a->slidespeed = (UWORD)dat << 2;
	if (a->main.period)
		if (tick)
			a->tmpperiod -= a->slidespeed;

	return 0;
}

static int DoPTEffect2(UWORD tick, UWORD flags, MP_CONTROL *a, MODULE *mod, SWORD channel)
{
	UBYTE dat;

	dat = UniGetByte();
	if (!tick && dat)
		a->slidespeed = (UWORD)dat << 2;
	if (a->main.period)
		if (tick)
			a->tmpperiod += a->slidespeed;

	return 0;
}

static void DoToneSlide(UWORD tick, MP_CONTROL *a)
{
	if (!a->main.fadevol)
		a->main.kick = (a->main.kick == KICK_NOTE)? KICK_NOTE : KICK_KEYOFF;
	else
		a->main.kick = (a->main.kick == KICK_NOTE)? KICK_ENV : KICK_ABSENT;

	if (tick != 0) {
		int dist;

		/* We have to slide a->main.period towards a->wantedperiod, so compute
		   the difference between those two values */
		dist=a->main.period-a->wantedperiod;

		/* if they are equal or if portamentospeed is too big ...*/
		if (dist == 0 || a->portspeed > abs(dist))
			/* ...make tmpperiod equal tperiod */
			a->tmpperiod=a->main.period=a->wantedperiod;
		else if (dist>0) {
			a->tmpperiod-=a->portspeed;	
			a->main.period-=a->portspeed; /* dist>0, slide up */
		} else {
			a->tmpperiod+=a->portspeed;	
			a->main.period+=a->portspeed; /* dist<0, slide down */
		}
	} else
		a->tmpperiod=a->main.period;
	a->ownper = 1;
}

static int DoPTEffect3(UWORD tick, UWORD flags, MP_CONTROL *a, MODULE *mod, SWORD channel)
{
	UBYTE dat;

	dat=UniGetByte();
	if ((!tick)&&(dat)) a->portspeed=(UWORD)dat<<2;
	if (a->main.period)
		DoToneSlide(tick, a);

	return 0;
}

static void DoVibrato(UWORD tick, MP_CONTROL *a)
{
	UBYTE q;
	UWORD temp = 0;	/* silence warning */

	if (!tick)
		return;

	q=(a->vibpos>>2)&0x1f;

	switch (a->wavecontrol&3) {
	case 0: /* sine */
		temp=VibratoTable[q];
		break;
	case 1: /* ramp down */
		q<<=3;
		if (a->vibpos<0) q=255-q;
		temp=q;
		break;
	case 2: /* square wave */
		temp=255;
		break;
	case 3: /* random wave */
		temp=getrandom(256);
		break;
	}

	temp*=a->vibdepth;
	temp>>=7;temp<<=2;

	if (a->vibpos>=0)
		a->main.period=a->tmpperiod+temp;
	else
		a->main.period=a->tmpperiod-temp;
	a->ownper = 1;

	if (tick != 0)
		a->vibpos+=a->vibspd;
}

static int DoPTEffect4(UWORD tick, UWORD flags, MP_CONTROL *a, MODULE *mod, SWORD channel)
{
	UBYTE dat;

	dat=UniGetByte();
	if (!tick) {
		if (dat&0x0f) a->vibdepth=dat&0xf;
		if (dat&0xf0) a->vibspd=(dat&0xf0)>>2;
	}
	if (a->main.period)
		DoVibrato(tick, a);

	return 0;
}

static void DoVolSlide(MP_CONTROL *a, UBYTE dat)
{
	if (dat&0xf) {
		a->tmpvolume-=(dat&0x0f);
		if (a->tmpvolume<0)
			a->tmpvolume=0;
	} else {
		a->tmpvolume+=(dat>>4);
		if (a->tmpvolume>64)
			a->tmpvolume=64;
	}
}

static int DoPTEffect5(UWORD tick, UWORD flags, MP_CONTROL *a, MODULE *mod, SWORD channel)
{
	UBYTE dat;

	dat=UniGetByte();
	if (a->main.period)
		DoToneSlide(tick, a);

	if (tick)
		DoVolSlide(a, dat);

	return 0;
}

/* DoPTEffect6 after DoPTEffectA */

static int DoPTEffect7(UWORD tick, UWORD flags, MP_CONTROL *a, MODULE *mod, SWORD channel)
{
	UBYTE dat;
	UBYTE q;
	UWORD temp = 0;	/* silence warning */

	dat=UniGetByte();
	if (!tick) {
		if (dat&0x0f) a->trmdepth=dat&0xf;
		if (dat&0xf0) a->trmspd=(dat&0xf0)>>2;
	}
	if (a->main.period) {
		q=(a->trmpos>>2)&0x1f;

		switch ((a->wavecontrol>>4)&3) {
		case 0: /* sine */
			temp=VibratoTable[q];
			break;
		case 1: /* ramp down */
			q<<=3;
			if (a->trmpos<0) q=255-q;
			temp=q;
			break;
		case 2: /* square wave */
			temp=255;
			break;
		case 3: /* random wave */
			temp=getrandom(256);
			break;
		}
		temp*=a->trmdepth;
		temp>>=6;

		if (a->trmpos>=0) {
			a->volume=a->tmpvolume+temp;
			if (a->volume>64) a->volume=64;
		} else {
			a->volume=a->tmpvolume-temp;
			if (a->volume<0) a->volume=0;
		}
		a->ownvol = 1;

		if (tick)
			a->trmpos+=a->trmspd;
	}

	return 0;
}

static int DoPTEffect8(UWORD tick, UWORD flags, MP_CONTROL *a, MODULE *mod, SWORD channel)
{
	UBYTE dat;

	dat = UniGetByte();
	if (mod->panflag)
		a->main.panning = mod->panning[channel] = dat;

	return 0;
}

static int DoPTEffect9(UWORD tick, UWORD flags, MP_CONTROL *a, MODULE *mod, SWORD channel)
{
	UBYTE dat;

	dat=UniGetByte();
	if (!tick) {
		if (dat) a->soffset=(UWORD)dat<<8;
		a->main.start=a->hioffset|a->soffset;

		if ((a->main.s)&&(a->main.start>a->main.s->length))
			a->main.start=a->main.s->flags&(SF_LOOP|SF_BIDI)?
			    a->main.s->loopstart:a->main.s->length;
	}

	return 0;
}

static int DoPTEffectA(UWORD tick, UWORD flags, MP_CONTROL *a, MODULE *mod, SWORD channel)
{
	UBYTE dat;

	dat=UniGetByte();
	if (tick)
		DoVolSlide(a, dat);

	return 0;
}

static int DoPTEffect6(UWORD tick, UWORD flags, MP_CONTROL *a, MODULE *mod, SWORD channel)
{
	if (a->main.period)
		DoVibrato(tick, a);
	DoPTEffectA(tick, flags, a, mod, channel);

	return 0;
}

static int DoPTEffectB(UWORD tick, UWORD flags, MP_CONTROL *a, MODULE *mod, SWORD channel)
{
	UBYTE dat;

	dat=UniGetByte();

	if (tick || mod->patdly2)
		return 0;

	/* Vincent Voois uses a nasty trick in "Universal Bolero" */
	if (dat == mod->sngpos && mod->patbrk == mod->patpos)
		return 0;

	if (!mod->loop && !mod->patbrk &&
	    (dat < mod->sngpos ||
		 (mod->sngpos == (mod->numpos - 1) && !mod->patbrk) ||
	     (dat == mod->sngpos && (flags & UF_NOWRAP))
		)) {
		/* if we don't loop, better not to skip the end of the
		   pattern, after all... so:
		mod->patbrk=0; */
		mod->posjmp=3;
	} else {
		/* if we were fading, adjust... */
		if (mod->sngpos == (mod->numpos-1))
			mod->volume=mod->initvolume>128?128:mod->initvolume;
		mod->sngpos=dat;
		mod->posjmp=2;
		mod->patpos=0;
#ifdef _XBOX
		mod->loop = 0; // special credits loop/wrap semantics
#endif
	}

	return 0;
}

static int DoPTEffectC(UWORD tick, UWORD flags, MP_CONTROL *a, MODULE *mod, SWORD channel)
{
	UBYTE dat;

			dat=UniGetByte();
			if (tick) return 0;
			if (dat==(UBYTE)-1) a->anote=dat=0; /* note cut */
			else if (dat>64) dat=64;
			a->tmpvolume=dat;

	return 0;
}

static int DoPTEffectD(UWORD tick, UWORD flags, MP_CONTROL *a, MODULE *mod, SWORD channel)
{
	UBYTE dat;

			dat=UniGetByte();
			if ((tick)||(mod->patdly2)) return 0;
			if ((mod->positions[mod->sngpos]!=LAST_PATTERN)&&
			   (dat>mod->pattrows[mod->positions[mod->sngpos]]))
				dat=mod->pattrows[mod->positions[mod->sngpos]];
			mod->patbrk=dat;
			if (!mod->posjmp) {
				/* don't ask me to explain this code - it makes
				   backwards.s3m and children.xm (heretic's version) play
				   correctly, among others. Take that for granted, or write
				   the page of comments yourself... you might need some
				   aspirin - Miod */
				if ((mod->sngpos==mod->numpos-1)&&(dat)&&((mod->loop)||
				               (mod->positions[mod->sngpos]==(mod->numpat-1)
								&& !(flags&UF_NOWRAP)))) {
					mod->sngpos=0;
					mod->posjmp=2;
#ifdef _XBOX
					mod->loop = 0; // special credits loop/wrap semantics
#endif
				} else
					mod->posjmp=3;
			}

	return 0;
}

static void DoEEffects(UWORD tick, UWORD flags, MP_CONTROL *a, MODULE *mod,
	SWORD channel, UBYTE dat)
{
	UBYTE nib = dat & 0xf;

	switch (dat>>4) {
	case 0x0: /* hardware filter toggle, not supported */
		break;
	case 0x1: /* fineslide up */
		if (a->main.period)
			if (!tick)
				a->tmpperiod-=(nib<<2);
		break;
	case 0x2: /* fineslide dn */
		if (a->main.period)
			if (!tick)
				a->tmpperiod+=(nib<<2);
		break;
	case 0x3: /* glissando ctrl */
		a->glissando=nib;
		break;
	case 0x4: /* set vibrato waveform */
		a->wavecontrol&=0xf0;
		a->wavecontrol|=nib;
		break;
	case 0x5: /* set finetune */
		if (a->main.period) {
			if (flags&UF_XMPERIODS)
				a->speed=nib+128;
			else
				a->speed=finetune[nib];
			a->tmpperiod=GetPeriod(flags, (UWORD)a->main.note<<1,a->speed);
		}
		break;
	case 0x6: /* set patternloop */
		if (tick)
			break;
		if (nib) { /* set reppos or repcnt ? */
			/* set repcnt, so check if repcnt already is set, which means we
			   are already looping */
			if (a->pat_repcnt)
				a->pat_repcnt--; /* already looping, decrease counter */
			else {
#if 0
				/* this would make walker.xm, shipped with Xsoundtracker,
				   play correctly, but it's better to remain compatible
				   with FT2 */
				if ((!(flags&UF_NOWRAP))||(a->pat_reppos!=POS_NONE))
#endif
					a->pat_repcnt=nib; /* not yet looping, so set repcnt */
			}

			if (a->pat_repcnt) { /* jump to reppos if repcnt>0 */
				if (a->pat_reppos==POS_NONE)
					a->pat_reppos=mod->patpos-1;
				if (a->pat_reppos==-1) {
					mod->pat_repcrazy=1;
					mod->patpos=0;
					//Mod_Player_Init_internal(mod);
				} 
				else
					mod->patpos=a->pat_reppos;
			} else a->pat_reppos=POS_NONE;
		} else
			a->pat_reppos=mod->patpos-1; /* set reppos - can be (-1) */
		break;
	case 0x7: /* set tremolo waveform */
		a->wavecontrol&=0x0f;
		a->wavecontrol|=nib<<4;
		break;
	case 0x8: /* set panning */
		if (mod->panflag) {
			if (nib<=8) nib<<=4;
			else nib*=17;
			a->main.panning=mod->panning[channel]=nib;
		}
		break;
	case 0x9: /* retrig note */
		/* do not retrigger on tick 0, until we are emulating FT2 and effect
		   data is zero */
		if (!tick && !((flags & UF_FT2QUIRKS) && (!nib)))
			break;
		/* only retrigger if data nibble > 0, or if tick 0 (FT2 compat) */
		if (nib || !tick) {
			if (!a->retrig) {
				/* when retrig counter reaches 0, reset counter and restart
				   the sample */
				if (a->main.period) a->main.kick=KICK_NOTE;
				a->retrig=nib;
			}
			a->retrig--; /* countdown */
		}
		break;
	case 0xa: /* fine volume slide up */
		if (tick)
			break;
		a->tmpvolume+=nib;
		if (a->tmpvolume>64) a->tmpvolume=64;
		break;
	case 0xb: /* fine volume slide dn  */
		if (tick)
			break;
		a->tmpvolume-=nib;
		if (a->tmpvolume<0)a->tmpvolume=0;
		break;
	case 0xc: /* cut note */
		/* When tick reaches the cut-note value, turn the volume to
		   zero (just like on the amiga) */
		if (tick>=nib)
			a->tmpvolume=0; /* just turn the volume down */
		break;
	case 0xd: /* note delay */
		/* delay the start of the sample until tick==nib */
		if (!tick)
			a->main.notedelay=nib;
		else if (a->main.notedelay)
			a->main.notedelay--;
		break;
	case 0xe: /* pattern delay */
		if (!tick)
			if (!mod->patdly2)
				mod->patdly=nib+1; /* only once, when tick=0 */
		break;
	case 0xf: /* invert loop, not supported  */
		break;
	}
}

static int DoPTEffectE(UWORD tick, UWORD flags, MP_CONTROL *a, MODULE *mod, SWORD channel)
{
	DoEEffects(tick, flags, a, mod, channel, UniGetByte());

	return 0;
}

static int DoPTEffectF(UWORD tick, UWORD flags, MP_CONTROL *a, MODULE *mod, SWORD channel)
{
	UBYTE dat;

	dat=UniGetByte();
	if (tick||mod->patdly2) return 0;
	if (mod->extspd&&(dat>=mod->bpmlimit))
		mod->bpm=dat;
	else 
	  if (dat) {
		mod->sngspd=(dat>=mod->bpmlimit)?mod->bpmlimit-1:dat;
		mod->vbtick=0;
	}

	return 0;
}

/*========== Scream Tracker effects */

static int DoS3MEffectA(UWORD tick, UWORD flags, MP_CONTROL *a, MODULE *mod, SWORD channel)
{
	UBYTE speed;

	XB_Log("S3m: EffectA: chan %d", channel);

	speed = UniGetByte();

	if (tick || mod->patdly2)
		return 0;

	if (speed > 128)
		speed -= 128;
	if (speed) {
		mod->sngspd = speed;
		mod->vbtick = 0;
	}

	return 0;
}

static void DoS3MVolSlide(UWORD tick, UWORD flags, MP_CONTROL *a, UBYTE inf)
{
	UBYTE lo, hi;

	if (inf)
		a->s3mvolslide=inf;
	else
		inf=a->s3mvolslide;

	lo=inf&0xf;
	hi=inf>>4;

	if (!lo) {
		if ((tick)||(flags&UF_S3MSLIDES)) a->tmpvolume+=hi;
	} else
	  if (!hi) {
		if ((tick)||(flags&UF_S3MSLIDES)) a->tmpvolume-=lo;
	} else
	  if (lo==0xf) {
		if (!tick) a->tmpvolume+=(hi?hi:0xf);
	} else
	  if (hi==0xf) {
		if (!tick) a->tmpvolume-=(lo?lo:0xf);
	} else
	  return;

	if (a->tmpvolume<0)
		a->tmpvolume=0;
	else if (a->tmpvolume>64)
		a->tmpvolume=64;
}

static int DoS3MEffectD(UWORD tick, UWORD flags, MP_CONTROL *a, MODULE *mod, SWORD channel)
{
	XB_Log("S3m: EffectD: chan %d", channel);

	DoS3MVolSlide(tick, flags, a, UniGetByte());

	return 1;
}

static void DoS3MSlideDn(UWORD tick, MP_CONTROL *a, UBYTE inf)
{
	UBYTE hi,lo;

	if (inf)
		a->slidespeed=inf;
	else
		inf=a->slidespeed;

	hi=inf>>4;
	lo=inf&0xf;

	if (hi==0xf) {
		if (!tick) a->tmpperiod+=(UWORD)lo<<2;
	} else
	  if (hi==0xe) {
		if (!tick) a->tmpperiod+=lo;
	} else {
		if (tick) a->tmpperiod+=(UWORD)inf<<2;
	}
}

static int DoS3MEffectE(UWORD tick, UWORD flags, MP_CONTROL *a, MODULE *mod, SWORD channel)
{
	UBYTE dat;

	XB_Log("S3m: EffectE: chan %d", channel);

	dat=UniGetByte();
	if (a->main.period)
		DoS3MSlideDn(tick, a,dat);

	return 0;
}

static void DoS3MSlideUp(UWORD tick, MP_CONTROL *a, UBYTE inf)
{
	UBYTE hi,lo;

	if (inf) a->slidespeed=inf;
	else inf=a->slidespeed;

	hi=inf>>4;
	lo=inf&0xf;

	if (hi==0xf) {
		if (!tick) a->tmpperiod-=(UWORD)lo<<2;
	} else
	  if (hi==0xe) {
		if (!tick) a->tmpperiod-=lo;
	} else {
		if (tick) a->tmpperiod-=(UWORD)inf<<2;
	}
}

static int DoS3MEffectF(UWORD tick, UWORD flags, MP_CONTROL *a, MODULE *mod, SWORD channel)
{
	UBYTE dat;

	XB_Log("S3m: EffectF: chan %d", channel);

	dat=UniGetByte();
	if (a->main.period)
		DoS3MSlideUp(tick, a,dat);

	return 0;
}

static int DoS3MEffectI(UWORD tick, UWORD flags, MP_CONTROL *a, MODULE *mod, SWORD channel)
{
	UBYTE inf, on, off;

	XB_Log("S3m: EffectI: chan %d", channel);

	inf = UniGetByte();
	if (inf)
		a->s3mtronof = inf;
	else {
		inf = a->s3mtronof;
		if (!inf)
			return 0;
	}

	if (!tick)
		return 0;

	on=(inf>>4)+1;
	off=(inf&0xf)+1;
	a->s3mtremor%=(on+off);
	a->volume=(a->s3mtremor<on)?a->tmpvolume:0;
	a->ownvol=1;
	a->s3mtremor++;

	return 0;
}

static int DoS3MEffectQ(UWORD tick, UWORD flags, MP_CONTROL *a, MODULE *mod, SWORD channel)
{
	UBYTE inf;

	XB_Log("S3m: EffectQ: chan %d", channel);

	inf = UniGetByte();
	if (a->main.period) {
		if (inf) {
			a->s3mrtgslide=inf>>4;
			a->s3mrtgspeed=inf&0xf;
		}

		/* only retrigger if low nibble > 0 */
		if (a->s3mrtgspeed>0) {
			if (!a->retrig) {
				/* when retrig counter reaches 0, reset counter and restart the
				   sample */
				if (a->main.kick!=KICK_NOTE) a->main.kick=KICK_KEYOFF;
				a->retrig=a->s3mrtgspeed;

				if ((tick)||(flags&UF_S3MSLIDES)) {
					switch (a->s3mrtgslide) {
					case 1:
					case 2:
					case 3:
					case 4:
					case 5:
						a->tmpvolume-=(1<<(a->s3mrtgslide-1));
						break;
					case 6:
						a->tmpvolume=(2*a->tmpvolume)/3;
						break;
					case 7:
						a->tmpvolume>>=1;
						break;
					case 9:
					case 0xa:
					case 0xb:
					case 0xc:
					case 0xd:
						a->tmpvolume+=(1<<(a->s3mrtgslide-9));
						break;
					case 0xe:
						a->tmpvolume=(3*a->tmpvolume)>>1;
						break;
					case 0xf:
						a->tmpvolume=a->tmpvolume<<1;
						break;
					}
					if (a->tmpvolume<0)
						a->tmpvolume=0;
					else if (a->tmpvolume>64)
						a->tmpvolume=64;
				}
			}
			a->retrig--; /* countdown  */
		}
	}

	return 0;
}

static int DoS3MEffectR(UWORD tick, UWORD flags, MP_CONTROL *a, MODULE *mod, SWORD channel)
{
	UBYTE dat, q;
	UWORD temp=0;	/* silence warning */

	XB_Log("S3m: EffectR: chan %d", channel);

	dat = UniGetByte();
	if (!tick) {
		if (dat&0x0f) a->trmdepth=dat&0xf;
		if (dat&0xf0) a->trmspd=(dat&0xf0)>>2;
	}

	q=(a->trmpos>>2)&0x1f;

	switch ((a->wavecontrol>>4)&3) {
	case 0: /* sine */
		temp=VibratoTable[q];
		break;
	case 1: /* ramp down */
		q<<=3;
		if (a->trmpos<0) q=255-q;
		temp=q;
		break;
	case 2: /* square wave */
		temp=255;
		break;
	case 3: /* random */
		temp=getrandom(256);
		break;
	}

	temp*=a->trmdepth;
	temp>>=7;

	if (a->trmpos>=0) {
		a->volume=a->tmpvolume+temp;
		if (a->volume>64) a->volume=64;
	} else {
		a->volume=a->tmpvolume-temp;
		if (a->volume<0) a->volume=0;
	}
	a->ownvol = 1;

	if (tick)
		a->trmpos+=a->trmspd;

	return 0;
}

static int DoS3MEffectT(UWORD tick, UWORD flags, MP_CONTROL *a, MODULE *mod, SWORD channel)
{
	UBYTE tempo;

	XB_Log("S3m: EffectT: chan %d", channel);

	tempo = UniGetByte();

	if (tick || mod->patdly2)
		return 0;

	mod->bpm = (tempo < 32) ? 32 : tempo;

	return 0;
}

static int DoS3MEffectU(UWORD tick, UWORD flags, MP_CONTROL *a, MODULE *mod, SWORD channel)
{
	UBYTE dat, q;
	UWORD temp = 0;	/* silence warning */

	XB_Log("S3m: EffectU: chan %d", channel);

	dat = UniGetByte();
	if (!tick) {
		if (dat&0x0f) a->vibdepth=dat&0xf;
		if (dat&0xf0) a->vibspd=(dat&0xf0)>>2;
	} else
		if (a->main.period) {
			q=(a->vibpos>>2)&0x1f;

			switch (a->wavecontrol&3) {
			case 0: /* sine */
				temp=VibratoTable[q];
				break;
			case 1: /* ramp down */
				q<<=3;
				if (a->vibpos<0) q=255-q;
				temp=q;
				break;
			case 2: /* square wave */
				temp=255;
				break;
			case 3: /* random */
				temp=getrandom(256);
				break;
			}

			temp*=a->vibdepth;
			temp>>=8;

			if (a->vibpos>=0)
				a->main.period=a->tmpperiod+temp;
			else
				a->main.period=a->tmpperiod-temp;
			a->ownper = 1;

			a->vibpos+=a->vibspd;
	}

	return 0;
}

/*========== Envelope helpers */

static int DoKeyOff(UWORD tick, UWORD flags, MP_CONTROL *a, MODULE *mod, SWORD channel)
{
	a->main.keyoff|=KEY_OFF;
	if ((!(a->main.volflg&EF_ON))||(a->main.volflg&EF_LOOP))
		a->main.keyoff=KEY_KILL;

	return 0;
}

static int DoKeyFade(UWORD tick, UWORD flags, MP_CONTROL *a, MODULE *mod, SWORD channel)
{
	UBYTE dat;

	dat=UniGetByte();
	if ((tick>=dat)||(tick==mod->sngspd-1)) {
		a->main.keyoff=KEY_KILL;
		if (!(a->main.volflg&EF_ON))
			a->main.fadevol=0;
	}

	return 0;
}

/*========== Fast Tracker effects */

/* DoXMEffect6 after DoXMEffectA */

static int DoXMEffectA(UWORD tick, UWORD flags, MP_CONTROL *a, MODULE *mod, SWORD channel)
{
	UBYTE inf, lo, hi;

	inf = UniGetByte();
	if (inf)
		a->s3mvolslide = inf;
	else
		inf = a->s3mvolslide;
	
	if (tick) {
		lo=inf&0xf;
		hi=inf>>4;

		if (!hi) {
			a->tmpvolume-=lo;
			if (a->tmpvolume<0) a->tmpvolume=0;
		} else {
			a->tmpvolume+=hi;
			if (a->tmpvolume>64) a->tmpvolume=64;
		}
	}

	return 0;
}

static int DoXMEffect6(UWORD tick, UWORD flags, MP_CONTROL *a, MODULE *mod, SWORD channel)
{
	if (a->main.period)
		DoVibrato(tick, a);

	return DoXMEffectA(tick, flags, a, mod, channel);
}

static int DoXMEffectE1(UWORD tick, UWORD flags, MP_CONTROL *a, MODULE *mod, SWORD channel)
{
	UBYTE dat;

	dat=UniGetByte();
	if (!tick) {
		if (dat) a->fportupspd=dat;
		if (a->main.period)
			a->tmpperiod-=(a->fportupspd<<2);
	}

	return 0;
}

static int DoXMEffectE2(UWORD tick, UWORD flags, MP_CONTROL *a, MODULE *mod, SWORD channel)
{
	UBYTE dat;

	dat=UniGetByte();
	if (!tick) {
		if (dat) a->fportdnspd=dat;
		if (a->main.period)
			a->tmpperiod+=(a->fportdnspd<<2);
	}

	return 0;
}

static int DoXMEffectEA(UWORD tick, UWORD flags, MP_CONTROL *a, MODULE *mod, SWORD channel)
{
	UBYTE dat;

	dat=UniGetByte();
	if (!tick)
		if (dat) a->fslideupspd=dat;
	a->tmpvolume+=a->fslideupspd;
	if (a->tmpvolume>64) a->tmpvolume=64;

	return 0;
}

static int DoXMEffectEB(UWORD tick, UWORD flags, MP_CONTROL *a, MODULE *mod, SWORD channel)
{
	UBYTE dat;

	dat=UniGetByte();
	if (!tick)
		if (dat) a->fslidednspd=dat;
	a->tmpvolume-=a->fslidednspd;
	if (a->tmpvolume<0) a->tmpvolume=0;

	return 0;
}

static int DoXMEffectG(UWORD tick, UWORD flags, MP_CONTROL *a, MODULE *mod, SWORD channel)
{
	mod->volume=UniGetByte()<<1;
	if (mod->volume>128) mod->volume=128;

	return 0;
}

static int DoXMEffectH(UWORD tick, UWORD flags, MP_CONTROL *a, MODULE *mod, SWORD channel)
{
	UBYTE inf;

	inf = UniGetByte();

	if (tick) {
		if (inf) mod->globalslide=inf;
		else inf=mod->globalslide;
		if (inf & 0xf0) inf&=0xf0;
		mod->volume=mod->volume+((inf>>4)-(inf&0xf))*2;

		if (mod->volume<0)
			mod->volume=0;
		else if (mod->volume>128)
			mod->volume=128;
	}

	return 0;
}

static int DoXMEffectL(UWORD tick, UWORD flags, MP_CONTROL *a, MODULE *mod, SWORD channel)
{
	UBYTE dat;

	dat=UniGetByte();
	if ((!tick)&&(a->main.i)) {
		UWORD points;
		INSTRUMENT *i=a->main.i;
		MP_VOICE *aout;

		if ((aout=a->slave)) {
			if (aout->venv.env) {
				points=i->volenv[i->volpts-1].pos;
				aout->venv.p=aout->venv.env[(dat>points)?points:dat].pos;
			}
			if (aout->penv.env) {
				points=i->panenv[i->panpts-1].pos;
				aout->penv.p=aout->penv.env[(dat>points)?points:dat].pos;
			}
		}
	}

	return 0;
}

static int DoXMEffectP(UWORD tick, UWORD flags, MP_CONTROL *a, MODULE *mod, SWORD channel)
{
	UBYTE inf, lo, hi;
	SWORD pan;

	inf = UniGetByte();
	if (!mod->panflag)
		return 0;

	if (inf)
		a->pansspd = inf;
	else
		inf =a->pansspd;

	if (tick) {
		lo=inf&0xf;
		hi=inf>>4;

		/* slide right has absolute priority */
		if (hi)
			lo = 0;

		pan=((a->main.panning==PAN_SURROUND)?PAN_CENTER:a->main.panning)+hi-lo;
		a->main.panning=(pan<PAN_LEFT)?PAN_LEFT:(pan>PAN_RIGHT?PAN_RIGHT:pan);
	}

	return 0;
}

static int DoXMEffectX1(UWORD tick, UWORD flags, MP_CONTROL *a, MODULE *mod, SWORD channel)
{
	UBYTE dat;

	dat = UniGetByte();
	if (dat)
		a->ffportupspd = dat;
	else
		dat = a->ffportupspd;

	if (a->main.period)
		if (!tick) {
			a->main.period-=dat;
			a->tmpperiod-=dat;
			a->ownper = 1;
		}

	return 0;
}

static int DoXMEffectX2(UWORD tick, UWORD flags, MP_CONTROL *a, MODULE *mod, SWORD channel)
{
	UBYTE dat;

	dat = UniGetByte();
	if (dat)
		a->ffportdnspd=dat;
	else
		dat = a->ffportdnspd;

	if (a->main.period)
		if (!tick) {
			a->main.period+=dat;
			a->tmpperiod+=dat;
			a->ownper = 1;
		}

	return 0;
}

/*========== Impulse Tracker effects */

static void DoITToneSlide(UWORD tick, MP_CONTROL *a, UBYTE dat)
{
	if (dat)
		a->portspeed = dat;

	/* if we don't come from another note, ignore the slide and play the note
	   as is */
	if (!a->oldnote || !a->main.period)
			return;

	if ((!tick)&&(a->newsamp)){
		a->main.kick=KICK_NOTE;
		a->main.start=-1;
	} else
		a->main.kick=(a->main.kick==KICK_NOTE)?KICK_ENV:KICK_ABSENT;

	if (tick) {
		int dist;

		/* We have to slide a->main.period towards a->wantedperiod, compute the
		   difference between those two values */
		dist=a->main.period-a->wantedperiod;

	    /* if they are equal or if portamentospeed is too big... */
		if ((!dist)||((a->portspeed<<2)>abs(dist)))
			/* ... make tmpperiod equal tperiod */
			a->tmpperiod=a->main.period=a->wantedperiod;
		else
		  if (dist>0) {	
			a->tmpperiod-=a->portspeed<<2;
			a->main.period-=a->portspeed<<2; /* dist>0 slide up */
		} else {				
			a->tmpperiod+=a->portspeed<<2;
			a->main.period+=a->portspeed<<2; /* dist<0 slide down */
		}
	} else
		a->tmpperiod=a->main.period;
	a->ownper=1;
}

static int DoITEffectG(UWORD tick, UWORD flags, MP_CONTROL *a, MODULE *mod, SWORD channel)
{
	DoITToneSlide(tick, a, UniGetByte());

	return 0;
}

static void DoITVibrato(UWORD tick, MP_CONTROL *a, UBYTE dat)
{
	UBYTE q;
	UWORD temp=0;

	if (!tick) {
		if (dat&0x0f) a->vibdepth=dat&0xf;
		if (dat&0xf0) a->vibspd=(dat&0xf0)>>2;
	}
	if (!a->main.period)
			return;

	q=(a->vibpos>>2)&0x1f;

	switch (a->wavecontrol&3) {
	case 0: /* sine */
		temp=VibratoTable[q];
		break;
	case 1: /* square wave */
		temp=255;
		break;
	case 2: /* ramp down */
		q<<=3;
		if (a->vibpos<0) q=255-q;
		temp=q;
		break;
	case 3: /* random */
		temp=getrandom(256);
		break;
	}

	temp*=a->vibdepth;
	temp>>=8;
	temp<<=2;

	if (a->vibpos>=0)
		a->main.period=a->tmpperiod+temp;
	else
		a->main.period=a->tmpperiod-temp;
	a->ownper=1;

	a->vibpos+=a->vibspd;
}

static int DoITEffectH(UWORD tick, UWORD flags, MP_CONTROL *a, MODULE *mod, SWORD channel)
{
	DoITVibrato(tick, a, UniGetByte());

	return 0;
}

static int DoITEffectI(UWORD tick, UWORD flags, MP_CONTROL *a, MODULE *mod, SWORD channel)
{
	UBYTE inf, on, off;

	inf = UniGetByte();
	if (inf)
		a->s3mtronof = inf;
	else {
		inf = a->s3mtronof;
		if (!inf)
			return 0;
	}

	on=(inf>>4);
	off=(inf&0xf);

	a->s3mtremor%=(on+off);
	a->volume=(a->s3mtremor<on)?a->tmpvolume:0;
	a->ownvol = 1;
	a->s3mtremor++;

	return 0;
}

static int DoITEffectM(UWORD tick, UWORD flags, MP_CONTROL *a, MODULE *mod, SWORD channel)
{
	a->main.chanvol=UniGetByte();
	if (a->main.chanvol>64)
		a->main.chanvol=64;
	else if (a->main.chanvol<0)
		a->main.chanvol=0;

	return 0;
}

static int DoITEffectN(UWORD tick, UWORD flags, MP_CONTROL *a, MODULE *mod, SWORD channel)
{
	UBYTE inf, lo, hi;

	inf = UniGetByte();

	if (inf)
		a->chanvolslide = inf;
	else
		inf = a->chanvolslide;

	lo=inf&0xf;
	hi=inf>>4;

	if (!hi) 
		a->main.chanvol-=lo;
	else
	  if (!lo) {
		a->main.chanvol+=hi;
	} else
	  if (hi==0xf) {
		if (!tick) a->main.chanvol-=lo;
	} else
	  if (lo==0xf) {
		if (!tick) a->main.chanvol+=hi;
	}

	if (a->main.chanvol<0)
		a->main.chanvol=0;
	else if (a->main.chanvol>64)
		a->main.chanvol=64;

	return 0;
}

static int DoITEffectP(UWORD tick, UWORD flags, MP_CONTROL *a, MODULE *mod, SWORD channel)
{
	UBYTE inf, lo, hi;
	SWORD pan;

	inf = UniGetByte();
	if (inf)
		a->pansspd = inf;
	else
		inf = a->pansspd;

	if (!mod->panflag)
		return 0;

	lo=inf&0xf;
	hi=inf>>4;

	pan=(a->main.panning==PAN_SURROUND)?PAN_CENTER:a->main.panning;

	if (!hi)
		pan+=lo<<2;
	else
	  if (!lo) {
		pan-=hi<<2;
	} else
	  if (hi==0xf) {
		if (!tick) pan+=lo<<2;
	} else
	  if (lo==0xf) {
		if (!tick) pan-=hi<<2;
	}
	a->main.panning=
	  (pan<PAN_LEFT)?PAN_LEFT:(pan>PAN_RIGHT?PAN_RIGHT:pan);

	return 0;
}

static int DoITEffectT(UWORD tick, UWORD flags, MP_CONTROL *a, MODULE *mod, SWORD channel)
{
	UBYTE tempo;
	SWORD temp;

   	tempo = UniGetByte();

	if (mod->patdly2)
		return 0;

	temp = mod->bpm;
	if (tempo & 0x10)
		temp += (tempo & 0x0f);
	else
		temp -= tempo;

	mod->bpm=(temp>255)?255:(temp<1?1:temp);

	return 0;
}

static int DoITEffectU(UWORD tick, UWORD flags, MP_CONTROL *a, MODULE *mod, SWORD channel)
{
	UBYTE dat, q;
	UWORD temp = 0;	/* silence warning */

	dat = UniGetByte();
	if (!tick) {
		if (dat&0x0f) a->vibdepth=dat&0xf;
		if (dat&0xf0) a->vibspd=(dat&0xf0)>>2;
	}
	if (a->main.period) {
		q=(a->vibpos>>2)&0x1f;

		switch (a->wavecontrol&3) {
		case 0: /* sine */
			temp=VibratoTable[q];
			break;
		case 1: /* square wave */
			temp=255;
			break;
		case 2: /* ramp down */
			q<<=3;
			if (a->vibpos<0) q=255-q;
			temp=q;
			break;
		case 3: /* random */
			temp=getrandom(256);
			break;
		}

		temp*=a->vibdepth;
		temp>>=8;

		if (a->vibpos>=0)
			a->main.period=a->tmpperiod+temp;
		else
			a->main.period=a->tmpperiod-temp;
		a->ownper = 1;

		a->vibpos+=a->vibspd;
	}

	return 0;
}

static int DoITEffectW(UWORD tick, UWORD flags, MP_CONTROL *a, MODULE *mod, SWORD channel)
{
	UBYTE inf, lo, hi;

	inf = UniGetByte();

	if (inf)
		mod->globalslide = inf;
	else
		inf = mod->globalslide;

	lo=inf&0xf;
	hi=inf>>4;

	if (!lo) {
		if (tick) mod->volume+=hi;
	} else
	  if (!hi) {
		if (tick) mod->volume-=lo;
	} else
	  if (lo==0xf) {
		if (!tick) mod->volume+=hi;
	} else
	  if (hi==0xf) {
		if (!tick) mod->volume-=lo;
	}

	if (mod->volume<0)
		mod->volume=0;
	else if (mod->volume>128)
		mod->volume=128;

	return 0;
}

static int DoITEffectY(UWORD tick, UWORD flags, MP_CONTROL *a, MODULE *mod, SWORD channel)
{
	UBYTE dat, q;
	SLONG temp = 0;	/* silence warning */


	dat=UniGetByte();
	if (!tick) {
		if (dat&0x0f) a->panbdepth=(dat&0xf);
		if (dat&0xf0) a->panbspd=(dat&0xf0)>>4;
	}
	if (mod->panflag) {
		q=a->panbpos;

		switch (a->panbwave) {
		case 0: /* sine */
			temp=PanbrelloTable[q];
			break;
		case 1: /* square wave */
			temp=(q<0x80)?64:0;
			break;
		case 2: /* ramp down */
			q<<=3;
			temp=q;
			break;
		case 3: /* random */
			temp=getrandom(256);
			break;
		}

		temp*=a->panbdepth;
		temp=(temp/8)+mod->panning[channel];

		a->main.panning=
			(temp<PAN_LEFT)?PAN_LEFT:(temp>PAN_RIGHT?PAN_RIGHT:temp);
		a->panbpos+=a->panbspd;

	}

	return 0;
}

static void DoNNAEffects(MODULE *, MP_CONTROL *, UBYTE);

/* Impulse/Scream Tracker Sxx effects.
   All Sxx effects share the same memory space. */
static int DoITEffectS0(UWORD tick, UWORD flags, MP_CONTROL *a, MODULE *mod, SWORD channel)
{
	UBYTE dat, inf, c;

	dat = UniGetByte();
	inf=dat&0xf;
	c=dat>>4;

	if (!dat) {
		c=a->sseffect;
		inf=a->ssdata;
	} else {
		a->sseffect=c;
		a->ssdata=inf;
	}

	switch (c) {
	case SS_GLISSANDO: /* S1x set glissando voice */
		DoEEffects(tick, flags, a, mod, channel, 0x30|inf);
		break;              
	case SS_FINETUNE: /* S2x set finetune */
		DoEEffects(tick, flags, a, mod, channel, 0x50|inf);
		break;
	case SS_VIBWAVE: /* S3x set vibrato waveform */
		DoEEffects(tick, flags, a, mod, channel, 0x40|inf);
		break;   
	case SS_TREMWAVE: /* S4x set tremolo waveform */
		DoEEffects(tick, flags, a, mod, channel, 0x70|inf);
		break;
	case SS_PANWAVE: /* S5x panbrello */
		a->panbwave=inf;
		break;
	case SS_FRAMEDELAY: /* S6x delay x number of frames (patdly) */
		DoEEffects(tick, flags, a, mod, channel, 0xe0|inf);
		break;
	case SS_S7EFFECTS: /* S7x instrument / NNA commands */
		DoNNAEffects(mod, a, inf);
		break;
	case SS_PANNING: /* S8x set panning position */
		DoEEffects(tick, flags, a, mod, channel, 0x80 | inf);
		break;
	case SS_SURROUND: /* S9x set surround sound */
		if (mod->panflag)
			a->main.panning = mod->panning[channel] = PAN_SURROUND;
		break;    
	case SS_HIOFFSET: /* SAy set high order sample offset yxx00h */
		if (!tick) {
			a->hioffset=inf<<16;
			a->main.start=a->hioffset|a->soffset;

			if ((a->main.s)&&(a->main.start>a->main.s->length))
				a->main.start=a->main.s->flags&(SF_LOOP|SF_BIDI)?
				    a->main.s->loopstart:a->main.s->length;
		}
		break;
	case SS_PATLOOP: /* SBx pattern loop */
		DoEEffects(tick, flags, a, mod, channel, 0x60|inf);
		break;
	case SS_NOTECUT: /* SCx notecut */
		if (!inf) inf = 1;
		DoEEffects(tick, flags, a, mod, channel, 0xC0|inf);
		break;
	case SS_NOTEDELAY: /* SDx notedelay */
		DoEEffects(tick, flags, a, mod, channel, 0xD0|inf);
		break;
	case SS_PATDELAY: /* SEx patterndelay */
		DoEEffects(tick, flags, a, mod, channel, 0xE0|inf);
		break;
	}

	return 0;
}

/*========== Impulse Tracker Volume/Pan Column effects */

/*
 * All volume/pan column effects share the same memory space.
 */

static int DoVolEffects(UWORD tick, UWORD flags, MP_CONTROL *a, MODULE *mod, SWORD channel)
{
	UBYTE c, inf;
	
	c = UniGetByte(); 
	inf = UniGetByte(); 

	if ((!c)&&(!inf)) {
		c=a->voleffect;
		inf=a->voldata;
	} else {
		a->voleffect=c;
		a->voldata=inf;
	}

	if (c)
		switch (c) {
		case VOL_VOLUME:
			if (tick) break;
			if (inf>64) inf=64;
			a->tmpvolume=inf;
			break;
		case VOL_PANNING:
			if (mod->panflag)
				a->main.panning=inf;
			break;
		case VOL_VOLSLIDE:
			DoS3MVolSlide(tick, flags, a, inf);
			return 1;
		case VOL_PITCHSLIDEDN:
			if (a->main.period)
				DoS3MSlideDn(tick, a, inf);
			break;
		case VOL_PITCHSLIDEUP:
			if (a->main.period)
				DoS3MSlideUp(tick, a, inf);
			break;
		case VOL_PORTAMENTO:
			DoITToneSlide(tick, a, inf);
			break;
		case VOL_VIBRATO:
			DoITVibrato(tick, a, inf);
			break;
	}

	return 0;
}

/*========== UltraTracker effects */

static int DoULTEffect9(UWORD tick, UWORD flags, MP_CONTROL *a, MODULE *mod, SWORD channel)
{
	UWORD offset=UniGetWord();

	if (offset)
		a->ultoffset=offset;

	a->main.start=a->ultoffset<<2;
	if ((a->main.s)&&(a->main.start>a->main.s->length))
		a->main.start=a->main.s->flags&(SF_LOOP|SF_BIDI)?
		    a->main.s->loopstart:a->main.s->length;

	return 0;
}

/*========== OctaMED effects */

static int DoMEDSpeed(UWORD tick, UWORD flags, MP_CONTROL *a, MODULE *mod, SWORD channel)
{
	UWORD speed=UniGetWord();

	mod->bpm=speed;

	return 0;
}

static int DoMEDEffectF1(UWORD tick, UWORD flags, MP_CONTROL *a, MODULE *mod, SWORD channel)
{
	DoEEffects(tick, flags, a, mod, channel, 0x90|(mod->sngspd/2));

	return 0;
}

static int DoMEDEffectF2(UWORD tick, UWORD flags, MP_CONTROL *a, MODULE *mod, SWORD channel)
{
	DoEEffects(tick, flags, a, mod, channel, 0xd0|(mod->sngspd/2));

	return 0;
}

static int DoMEDEffectF3(UWORD tick, UWORD flags, MP_CONTROL *a, MODULE *mod, SWORD channel)
{
	DoEEffects(tick, flags, a, mod, channel, 0x90|(mod->sngspd/3));

	return 0;
}

/*========== Oktalyzer effects */

static int DoOktArp(UWORD tick, UWORD flags, MP_CONTROL *a, MODULE *mod, SWORD channel)
{
	UBYTE dat, dat2;

	dat2 = UniGetByte();	/* arpeggio style */
	dat = UniGetByte();
	if (!tick) {
		if (!dat && (flags & UF_ARPMEM))
			dat=a->arpmem;
		else
			a->arpmem=dat;
	}
	if (a->main.period)
		DoArpeggio(tick, flags, a, dat2);

	return 0;
}

/*========== General player functions */

static int DoNothing(UWORD tick, UWORD flags, MP_CONTROL *a, MODULE *mod, SWORD channel)
{
	UniSkipOpcode();

	return 0;
}

typedef int (*effect_func) (UWORD, UWORD, MP_CONTROL *, MODULE *, SWORD);

static effect_func effects[UNI_LAST] = {
		DoNothing,		/* 0 */
		DoNothing,		/* UNI_NOTE */
		DoNothing,		/* UNI_INSTRUMENT */
		DoPTEffect0,	/* UNI_PTEFFECT0 */
		DoPTEffect1,	/* UNI_PTEFFECT1 */
		DoPTEffect2,	/* UNI_PTEFFECT2 */
		DoPTEffect3,	/* UNI_PTEFFECT3 */
		DoPTEffect4,	/* UNI_PTEFFECT4 */
		DoPTEffect5,	/* UNI_PTEFFECT5 */
		DoPTEffect6,	/* UNI_PTEFFECT6 */
		DoPTEffect7,	/* UNI_PTEFFECT7 */
		DoPTEffect8,	/* UNI_PTEFFECT8 */
		DoPTEffect9,	/* UNI_PTEFFECT9 */
		DoPTEffectA,	/* UNI_PTEFFECTA */
		DoPTEffectB,	/* UNI_PTEFFECTB */
		DoPTEffectC,	/* UNI_PTEFFECTC */
		DoPTEffectD,	/* UNI_PTEFFECTD */
		DoPTEffectE,	/* UNI_PTEFFECTE */
		DoPTEffectF,	/* UNI_PTEFFECTF */
		DoS3MEffectA,	/* UNI_S3MEFFECTA */
		DoS3MEffectD,	/* UNI_S3MEFFECTD */
		DoS3MEffectE,	/* UNI_S3MEFFECTE */
		DoS3MEffectF,	/* UNI_S3MEFFECTF */
		DoS3MEffectI,	/* UNI_S3MEFFECTI */
		DoS3MEffectQ,	/* UNI_S3MEFFECTQ */
		DoS3MEffectR,	/* UNI_S3MEFFECTR */
		DoS3MEffectT,	/* UNI_S3MEFFECTT */
		DoS3MEffectU,	/* UNI_S3MEFFECTU */
		DoKeyOff,		/* UNI_KEYOFF */
		DoKeyFade,		/* UNI_KEYFADE */
		DoVolEffects,	/* UNI_VOLEFFECTS */
		DoPTEffect4,	/* UNI_XMEFFECT4 */
		DoXMEffect6,	/* UNI_XMEFFECT6 */
		DoXMEffectA,	/* UNI_XMEFFECTA */
		DoXMEffectE1,	/* UNI_XMEFFECTE1 */
		DoXMEffectE2,	/* UNI_XMEFFECTE2 */
		DoXMEffectEA,	/* UNI_XMEFFECTEA */
		DoXMEffectEB,	/* UNI_XMEFFECTEB */
		DoXMEffectG,	/* UNI_XMEFFECTG */
		DoXMEffectH,	/* UNI_XMEFFECTH */
		DoXMEffectL,	/* UNI_XMEFFECTL */
		DoXMEffectP,	/* UNI_XMEFFECTP */
		DoXMEffectX1,	/* UNI_XMEFFECTX1 */
		DoXMEffectX2,	/* UNI_XMEFFECTX2 */
		DoITEffectG,	/* UNI_ITEFFECTG */
		DoITEffectH,	/* UNI_ITEFFECTH */
		DoITEffectI,	/* UNI_ITEFFECTI */
		DoITEffectM,	/* UNI_ITEFFECTM */
		DoITEffectN,	/* UNI_ITEFFECTN */
		DoITEffectP,	/* UNI_ITEFFECTP */
		DoITEffectT,	/* UNI_ITEFFECTT */
		DoITEffectU,	/* UNI_ITEFFECTU */
		DoITEffectW,	/* UNI_ITEFFECTW */
		DoITEffectY,	/* UNI_ITEFFECTY */
		DoNothing,		/* UNI_ITEFFECTZ */
		DoITEffectS0,	/* UNI_ITEFFECTS0 */
		DoULTEffect9,	/* UNI_ULTEFFECT9 */
		DoMEDSpeed,		/* UNI_MEDSPEED */
		DoMEDEffectF1,	/* UNI_MEDEFFECTF1 */
		DoMEDEffectF2,	/* UNI_MEDEFFECTF2 */
		DoMEDEffectF3,	/* UNI_MEDEFFECTF3 */
		DoOktArp,		/* UNI_OKTARP */
};

static int pt_playeffects(MODULE *mod, SWORD channel, MP_CONTROL *a)
{
	UWORD tick = mod->vbtick;
	UWORD flags = mod->flags;
	UBYTE c;
	int explicitslides = 0;
	effect_func f;

	while((c=UniGetByte())) {
		f = effects[c];
		if (f != DoNothing)
				a->sliding = 0;
		explicitslides |= f(tick, flags, a, mod, channel);
	}
	return explicitslides;
}

static void DoNNAEffects(MODULE *mod, MP_CONTROL *a, UBYTE dat)
{
	int t;
	MP_VOICE *aout;

	dat&=0xf; 
	aout=(a->slave)?a->slave:NULL;

	switch (dat) {
	case 0x0: /* past note cut */
		for (t=0;t<md_sngchn;t++)
			if (mod->voice[t].master==a)
				mod->voice[t].main.fadevol=0;
		break;
	case 0x1: /* past note off */
		for (t=0;t<md_sngchn;t++)
			if (mod->voice[t].master==a) {
				mod->voice[t].main.keyoff|=KEY_OFF;
				if ((!(mod->voice[t].venv.flg & EF_ON))||
				   (mod->voice[t].venv.flg & EF_LOOP))
					mod->voice[t].main.keyoff=KEY_KILL;
			}
		break;
	case 0x2: /* past note fade */
		for (t=0;t<md_sngchn;t++)
			if (mod->voice[t].master==a)
				mod->voice[t].main.keyoff|=KEY_FADE;
		break;
	case 0x3: /* set NNA note cut */
		a->main.nna=(a->main.nna&~NNA_MASK)|NNA_CUT;
		break;
	case 0x4: /* set NNA note continue */
		a->main.nna=(a->main.nna&~NNA_MASK)|NNA_CONTINUE;
		break;
	case 0x5: /* set NNA note off */
		a->main.nna=(a->main.nna&~NNA_MASK)|NNA_OFF;
		break;   
	case 0x6: /* set NNA note fade */
		a->main.nna=(a->main.nna&~NNA_MASK)|NNA_FADE;
		break;
	case 0x7: /* disable volume envelope */
		if (aout)
			aout->main.volflg&=~EF_ON;
		break;
	case 0x8: /* enable volume envelope  */
		if (aout)
			aout->main.volflg|=EF_ON;
		break;
	case 0x9: /* disable panning envelope */
		if (aout)
			aout->main.panflg&=~EF_ON;
		break;    
	case 0xa: /* enable panning envelope */
		if (aout)
			aout->main.panflg|=EF_ON;
		break;
	case 0xb: /* disable pitch envelope */
		if (aout)
			aout->main.pitflg&=~EF_ON;
		break;
	case 0xc: /* enable pitch envelope */
		if (aout)
			aout->main.pitflg|=EF_ON;
		break;
	}
}

void pt_UpdateVoices(MODULE *mod, int max_volume)
{
	SWORD envpan,envvol,envpit,channel;
	UWORD playperiod;
	SLONG vibval,vibdpt;
	ULONG tmpvol;

	MP_VOICE *aout;
	INSTRUMENT *i;
	SAMPLE *s;

	mod->totalchn=mod->realchn=0;
	for (channel=0;channel<md_sngchn;channel++) {
		aout=&mod->voice[channel];
		i=aout->main.i;
		s=aout->main.s;

		if (!s || !s->length) continue;

		if (aout->main.period<40)
			aout->main.period=40;
		else if (aout->main.period>50000)
			aout->main.period=50000;

		if ((aout->main.kick==KICK_NOTE)||(aout->main.kick==KICK_KEYOFF)) {
			Voice_Play_internal(channel,s,(aout->main.start==-1)?
			    ((s->flags&SF_UST_LOOP)?s->loopstart:0):aout->main.start);
			aout->main.fadevol=32768;
			aout->aswppos=0;
		}

		envvol = 256;
		envpan = PAN_CENTER;
		envpit = 32;
		if (i && ((aout->main.kick==KICK_NOTE)||(aout->main.kick==KICK_ENV))) {
			if (aout->main.volflg & EF_ON)
				envvol = StartEnvelope(&aout->venv,aout->main.volflg,
				  i->volpts,i->volsusbeg,i->volsusend,
				  i->volbeg,i->volend,i->volenv,aout->main.keyoff);
			if (aout->main.panflg & EF_ON)
				envpan = StartEnvelope(&aout->penv,aout->main.panflg,
				  i->panpts,i->pansusbeg,i->pansusend,
				  i->panbeg,i->panend,i->panenv,aout->main.keyoff);
			if (aout->main.pitflg & EF_ON)
				envpit = StartEnvelope(&aout->cenv,aout->main.pitflg,
				  i->pitpts,i->pitsusbeg,i->pitsusend,
				  i->pitbeg,i->pitend,i->pitenv,aout->main.keyoff);

			if (aout->cenv.flg & EF_ON)
				aout->masterperiod=GetPeriod(mod->flags,
				  (UWORD)aout->main.note<<1, aout->master->speed);
		} else {
			if (aout->main.volflg & EF_ON)
				envvol = ProcessEnvelope(aout,&aout->venv,256);
			if (aout->main.panflg & EF_ON)
				envpan = ProcessEnvelope(aout,&aout->penv,PAN_CENTER);
			if (aout->main.pitflg & EF_ON)
				envpit = ProcessEnvelope(aout,&aout->cenv,32);
		}
		aout->main.kick=KICK_ABSENT;

		tmpvol = aout->main.fadevol;	/* max 32768 */
		tmpvol *= aout->main.chanvol;	/* * max 64 */
		tmpvol *= aout->main.outvolume;	/* * max 256 */
		tmpvol /= (256 * 64);			/* tmpvol is max 32768 again */
		aout->totalvol = tmpvol >> 2;	/* used to determine samplevolume */
		tmpvol *= envvol;				/* * max 256 */
		tmpvol *= mod->volume;			/* * max 128 */
		tmpvol /= (128 * 256 * 128);

		/* fade out */
		if (mod->sngpos>=mod->numpos)
			tmpvol=0;
		else
			tmpvol=(tmpvol*max_volume)/128;

		if ((aout->masterchn!=-1)&& mod->control[aout->masterchn].muted)
			Voice_SetVolume_internal(channel,0);
		else {
			Voice_SetVolume_internal(channel,tmpvol);
			if ((tmpvol)&&(aout->master)&&(aout->master->slave==aout))
				mod->realchn++;
			mod->totalchn++;
		}

		if (aout->main.panning==PAN_SURROUND)
			Voice_SetPanning_internal(channel,PAN_SURROUND);
		else
			if ((mod->panflag)&&(aout->penv.flg & EF_ON))
				Voice_SetPanning_internal(channel,
				    DoPan(envpan,aout->main.panning));
			else
				Voice_SetPanning_internal(channel,aout->main.panning);

		if (aout->main.period && s->vibdepth)
			switch (s->vibtype) {
			case 0:
				vibval=avibtab[s->avibpos&127];
				if (aout->avibpos & 0x80) vibval=-vibval;
				break;
			case 1:
				vibval=64;
				if (aout->avibpos & 0x80) vibval=-vibval;
				break;
			case 2:
				vibval=63-(((aout->avibpos+128)&255)>>1);
				break;
			default:
				vibval=(((aout->avibpos+128)&255)>>1)-64;
				break;
			}
		else
			vibval=0;

		if (s->vibflags & AV_IT) {
			if ((aout->aswppos>>8)<s->vibdepth) {
				aout->aswppos += s->vibsweep;
				vibdpt=aout->aswppos;
			} else
				vibdpt=s->vibdepth<<8;
			vibval=(vibval*vibdpt)>>16;
			if (aout->mflag) {
				if (!(mod->flags&UF_LINEAR)) vibval>>=1;
				aout->main.period-=vibval;
			}
		} else {
			/* do XM style auto-vibrato */
			if (!(aout->main.keyoff & KEY_OFF)) {
				if (aout->aswppos<s->vibsweep) {
					vibdpt=(aout->aswppos*s->vibdepth)/s->vibsweep;
					aout->aswppos++;
				} else
					vibdpt=s->vibdepth;
			} else {
				/* keyoff -> depth becomes 0 if final depth wasn't reached or
				   stays at final level if depth WAS reached */
				if (aout->aswppos>=s->vibsweep)
					vibdpt=s->vibdepth;
				else
					vibdpt=0;
			}
			vibval=(vibval*vibdpt)>>8;
			aout->main.period-=vibval;
		}

		/* update vibrato position */
		aout->avibpos=(aout->avibpos+s->vibrate)&0xff;

		/* process pitch envelope */
		playperiod=aout->main.period;

		if ((aout->main.pitflg&EF_ON)&&(envpit!=32)) {
			long p1;

			envpit-=32;
			if ((aout->main.note<<1)+envpit<=0) envpit=-(aout->main.note<<1);

			p1=GetPeriod(mod->flags, ((UWORD)aout->main.note<<1)+envpit,
			    aout->master->speed)-aout->masterperiod;
			if (p1>0) {
				if ((UWORD)(playperiod+p1)<=playperiod) {
					p1=0;
					aout->main.keyoff|=KEY_OFF;
				}
			} else if (p1<0) {
				if ((UWORD)(playperiod+p1)>=playperiod) {
					p1=0;
					aout->main.keyoff|=KEY_OFF;
				}
			}
			playperiod+=p1;
		}

		if (!aout->main.fadevol) { /* check for a dead note (fadevol=0) */
			Voice_Stop_internal(channel);
			mod->totalchn--;
			if ((tmpvol)&&(aout->master)&&(aout->master->slave==aout))
				mod->realchn--;
		} else {
			Voice_SetFrequency_internal(channel,
			                            getfrequency(mod->flags,playperiod));

			/* if keyfade, start substracting fadeoutspeed from fadevol: */
			if ((i)&&(aout->main.keyoff&KEY_FADE)) {
				if (aout->main.fadevol>=i->volfade)
					aout->main.fadevol-=i->volfade;
				else
					aout->main.fadevol=0;
			}
		}

		md_bpm=mod->bpm+mod->relspd;
		if (md_bpm<32)
			md_bpm=32;
		else if ((!(mod->flags&UF_HIGHBPM)) && md_bpm>255)
			md_bpm=255;
	}
}

/* Handles new notes or instruments */
void pt_Notes(MODULE *mod)
{
	SWORD channel;
	MP_CONTROL *a;
	UBYTE c,inst;
	int tr,funky; /* funky is set to indicate note or instrument change */

	for (channel=0;channel<mod->numchn;channel++) {
		a=&mod->control[channel];

		if (mod->sngpos>=mod->numpos) {
			tr=mod->numtrk;
			mod->numrow=0;
		} else {
			tr=mod->patterns[(mod->positions[mod->sngpos]*mod->numchn)+channel];
			mod->numrow=mod->pattrows[mod->positions[mod->sngpos]];
		}

		a->row=(tr<mod->numtrk)?UniFindRow(mod->tracks[tr],mod->patpos):NULL;
		a->newsamp=0;
		if (!mod->vbtick) a->main.notedelay=0;

		if (!a->row) continue;
		UniSetRow(a->row);
		funky=0;

		while((c=UniGetByte()))
			switch (c) {
			case UNI_NOTE:
				funky|=1;
				a->oldnote=a->anote,a->anote=UniGetByte();
				a->main.kick =KICK_NOTE;
				a->main.start=-1;
				a->sliding=0;

				/* retrig tremolo and vibrato waves ? */
				if (!(a->wavecontrol & 0x80)) a->trmpos=0;
				if (!(a->wavecontrol & 0x08)) a->vibpos=0;
				if (!a->panbwave) a->panbpos=0;
				break;
			case UNI_INSTRUMENT:
				inst=UniGetByte();
				if (inst>=mod->numins) break; /* safety valve */
				funky|=2;
				a->main.i=(mod->flags & UF_INST)?&mod->instruments[inst]:NULL;
				a->retrig=0;
				a->s3mtremor=0;
				a->ultoffset=0;
				a->main.sample=inst;
				break;
			default:
				UniSkipOpcode();
				break;
			}

		if (funky) {
			INSTRUMENT *i;
			SAMPLE *s;

			if ((i=a->main.i)) {
				if (i->samplenumber[a->anote] >= mod->numsmp) continue;
				s=&mod->samples[i->samplenumber[a->anote]];
				a->main.note=i->samplenote[a->anote];
			} else {
				a->main.note=a->anote;
				s=&mod->samples[a->main.sample];
			}

			if (a->main.s!=s) {
				a->main.s=s;
				a->newsamp=a->main.period;
			}

			/* channel or instrument determined panning ? */
			a->main.panning=mod->panning[channel];
			if (s->flags & SF_OWNPAN)
				a->main.panning=s->panning;
			else if ((i)&&(i->flags & IF_OWNPAN))
				a->main.panning=i->panning;

			a->main.handle=s->handle;
			a->speed=s->speed;

			if (i) {
				if ((mod->panflag)&&(i->flags & IF_PITCHPAN)
				   &&(a->main.panning!=PAN_SURROUND)){
					a->main.panning+=
					    ((a->anote-i->pitpancenter)*i->pitpansep)/8;
					if (a->main.panning<PAN_LEFT)
						a->main.panning=PAN_LEFT;
					else if (a->main.panning>PAN_RIGHT)
						a->main.panning=PAN_RIGHT;
				}
				a->main.pitflg=i->pitflg;
				a->main.volflg=i->volflg;
				a->main.panflg=i->panflg;
				a->main.nna=i->nnatype;
				a->dca=i->dca;
				a->dct=i->dct;
			} else {
				a->main.pitflg=a->main.volflg=a->main.panflg=0;
				a->main.nna=a->dca=0;
				a->dct=DCT_OFF;
			}

			if (funky&2) /* instrument change */ {
				/* IT random volume variations: 0:8 bit fixed, and one bit for
				   sign. */
				a->volume=a->tmpvolume=s->volume;
				if ((s)&&(i)) {
					if (i->rvolvar) {
						a->volume=a->tmpvolume=s->volume+
						  ((s->volume*((SLONG)i->rvolvar*(SLONG)getrandom(512)
						  ))/25600);
						if (a->volume<0)
							a->volume=a->tmpvolume=0;
						else if (a->volume>64)
							a->volume=a->tmpvolume=64;
					}
					if ((mod->panflag)&&(a->main.panning!=PAN_SURROUND)) {
						a->main.panning+=((a->main.panning*((SLONG)i->rpanvar*
						  (SLONG)getrandom(512)))/25600);
						if (a->main.panning<PAN_LEFT)
							a->main.panning=PAN_LEFT;
						else if (a->main.panning>PAN_RIGHT)
							a->main.panning=PAN_RIGHT;
					}
				}
			}

			a->wantedperiod=a->tmpperiod=
			    GetPeriod(mod->flags, (UWORD)a->main.note<<1,a->speed);
			a->main.keyoff=KEY_KICK;
		}
	}
}

/* Handles effects */
void pt_EffectsPass1(MODULE *mod)
{
	SWORD channel;
	MP_CONTROL *a;
	MP_VOICE *aout;
	int explicitslides;

	for (channel=0;channel<mod->numchn;channel++) {
		a=&mod->control[channel];

		if ((aout=a->slave)) {
			a->main.fadevol=aout->main.fadevol;
			a->main.period=aout->main.period;
			if (a->main.kick==KICK_KEYOFF)
				a->main.keyoff=aout->main.keyoff;
		}

		if (!a->row) continue;
		UniSetRow(a->row);

		a->ownper=a->ownvol=0;
		explicitslides = pt_playeffects(mod, channel, a);

		/* continue volume slide if necessary for XM and IT */
		if (mod->flags&UF_BGSLIDES) {
			if (!explicitslides && a->sliding)
				DoS3MVolSlide(mod->vbtick, mod->flags, a, 0);
			else if (a->tmpvolume)
				a->sliding = explicitslides;
		}

		if (!a->ownper)
			a->main.period=a->tmpperiod;
		if (!a->ownvol)
			a->volume=a->tmpvolume;

		if (a->main.s) {
			if (a->main.i)
				a->main.outvolume=
				    (a->volume*a->main.s->globvol*a->main.i->globvol)>>10;
			else
				a->main.outvolume=(a->volume*a->main.s->globvol)>>4;
			if (a->main.outvolume>256)
				a->main.outvolume=256;
			else if (a->main.outvolume<0)
				a->main.outvolume=0;
		}
	}
}

/* NNA management */
void pt_NNA(MODULE *mod)
{
	SWORD channel;
	MP_CONTROL *a;

	for (channel=0;channel<mod->numchn;channel++) {
		a=&mod->control[channel];

		if (a->main.kick==KICK_NOTE) {
			BOOL kill=0;

			if (a->slave) {
				MP_VOICE *aout;

				aout=a->slave;
				if (aout->main.nna & NNA_MASK) {
					/* Make sure the old MP_VOICE channel knows it has no
					   master now ! */
					a->slave=NULL;
					/* assume the channel is taken by NNA */
					aout->mflag=0;

					switch (aout->main.nna) {
					case NNA_CONTINUE: /* continue note, do nothing */
						break;
					case NNA_OFF: /* note off */
						aout->main.keyoff|=KEY_OFF;
						if ((!(aout->main.volflg & EF_ON))||
							  (aout->main.volflg & EF_LOOP))
							aout->main.keyoff=KEY_KILL;
						break;
					case NNA_FADE:
						aout->main.keyoff |= KEY_FADE;
						break;
					}
				}
			}

			if (a->dct!=DCT_OFF) {
				int t;

				for (t=0;t<md_sngchn;t++)
					if ((!Voice_Stopped_internal(t))&&
					   (mod->voice[t].masterchn==channel)&&
					   (a->main.sample==mod->voice[t].main.sample)) {
						kill=0;
						switch (a->dct) {
						case DCT_NOTE:
							if (a->main.note==mod->voice[t].main.note)
								kill=1;
							break;
						case DCT_SAMPLE:
							if (a->main.handle==mod->voice[t].main.handle)
								kill=1;
							break;
						case DCT_INST:
							kill=1;
							break;
						}
						if (kill)
							switch (a->dca) {
							case DCA_CUT:
								mod->voice[t].main.fadevol=0;
								break;
							case DCA_OFF:
								mod->voice[t].main.keyoff|=KEY_OFF;
								if ((!(mod->voice[t].main.volflg&EF_ON))||
								    (mod->voice[t].main.volflg&EF_LOOP))
									mod->voice[t].main.keyoff=KEY_KILL;
								break;
							case DCA_FADE:
								mod->voice[t].main.keyoff|=KEY_FADE;
								break;
							}
					}
			}
		} /* if (a->main.kick==KICK_NOTE) */
	}
}

/* Setup module and NNA voices */
void pt_SetupVoices(MODULE *mod)
{
	SWORD channel;
	MP_CONTROL *a;
	MP_VOICE *aout;

	for (channel=0;channel<mod->numchn;channel++) {
		a=&mod->control[channel];

		if (a->main.notedelay) continue;
		if (a->main.kick==KICK_NOTE) {
			/* if no channel was cut above, find an empty or quiet channel
			   here */
			if (mod->flags&UF_NNA) {
				if (!a->slave) {
					int newchn;

					if ((newchn=MP_FindEmptyChannel(mod))!=-1)
						a->slave=&mod->voice[a->slavechn=newchn];
				}
			} else 
				a->slave=&mod->voice[a->slavechn=channel];

			/* assign parts of MP_VOICE only done for a KICK_NOTE */
			if ((aout=a->slave)) {
				if (aout->mflag && aout->master) aout->master->slave=NULL;
				aout->master=a;
				a->slave=aout;
				aout->masterchn=channel;
				aout->mflag=1;
			}
		} else
			aout=a->slave;

		if (aout)
			aout->main=a->main;
		a->main.kick=KICK_ABSENT;
	}
}

/* second effect pass */
void pt_EffectsPass2(MODULE *mod)
{
	SWORD channel;
	MP_CONTROL *a;
	UBYTE c;

	for (channel=0;channel<mod->numchn;channel++) {
		a=&mod->control[channel];

		if (!a->row) continue;
		UniSetRow(a->row);

		while((c=UniGetByte()))
			if (c==UNI_ITEFFECTS0) {
				c=UniGetByte();
				if ((c>>4)==SS_S7EFFECTS)
					DoNNAEffects(mod, a, c&0xf);
			} else
				UniSkipOpcode();
	}
}

void Mod_Player_HandleTick(void)
{
	SWORD channel;
	int max_volume;

#if 0
	/* don't handle the very first ticks, this allows the other hardware to
	   settle down so we don't loose any starting notes */
	if (isfirst) {
		isfirst--;
		return;
	}
#endif

	if ((!pf)||(pf->forbid)||(pf->sngpos>=pf->numpos)) return;

	/* update time counter (sngtime is in milliseconds (in fact 2^-10)) */
	pf->sngremainder+=(1<<9)*5; /* thus 2.5*(1<<10), since fps=0.4xtempo */
	pf->sngtime+=pf->sngremainder/pf->bpm;
	pf->sngremainder%=pf->bpm;

	if (++pf->vbtick>=pf->sngspd) {
		if (pf->pat_repcrazy) 
			pf->pat_repcrazy=0; /* play 2 times row 0 */
		else
			pf->patpos++;
		pf->vbtick=0;

		/* process pattern-delay. pf->patdly2 is the counter and pf->patdly is
		   the command memory. */
		if (pf->patdly)
			pf->patdly2=pf->patdly,pf->patdly=0;
		if (pf->patdly2) {
			/* patterndelay active */
			if (--pf->patdly2)
				/* so turn back pf->patpos by 1 */
				if (pf->patpos) pf->patpos--;
		}

		/* do we have to get a new patternpointer ? (when pf->patpos reaches the
		   pattern size, or when a patternbreak is active) */
		if (((pf->patpos>=pf->numrow)&&(pf->numrow>0))&&(!pf->posjmp))
			pf->posjmp=3;

		if (pf->posjmp) {
			pf->patpos=pf->numrow?(pf->patbrk%pf->numrow):0;
			pf->pat_repcrazy=0;
			pf->sngpos+=(pf->posjmp-2);
			for (channel=0;channel<pf->numchn;channel++)
				pf->control[channel].pat_reppos=-1;

			pf->patbrk=pf->posjmp=0;
			/* handle the "---" (end of song) pattern since it can occur
			   *inside* the module in some formats */
			if ((pf->sngpos>=pf->numpos)||
				(pf->positions[pf->sngpos]==LAST_PATTERN)) {
				if (!pf->wrap) return;
				if (!(pf->sngpos=pf->reppos)) {
					Mod_Player_Reinit_internal(pf);
				    pf->volume=pf->initvolume>128?128:pf->initvolume;
					if(pf->initspeed!=0)
						pf->sngspd=pf->initspeed<32?pf->initspeed:32;
					else
						pf->sngspd=6;
					pf->bpm=pf->inittempo<32?32:pf->inittempo;
#ifdef _XBOX
					pf->loop = 1; // special credits loop/wrap semantics
#endif
				}
			}
			if (pf->sngpos<0) pf->sngpos=pf->numpos-1;
		}

		if (!pf->patdly2)
			pt_Notes(pf);
	}

	/* Fade global volume if enabled and we're playing the last pattern */
	if (((pf->sngpos==pf->numpos-1)||
		 (pf->positions[pf->sngpos+1]==LAST_PATTERN))&&
	    (pf->fadeout))
		max_volume=pf->numrow?((pf->numrow-pf->patpos)*128)/pf->numrow:0;
	else
		max_volume=128;

	pt_EffectsPass1(pf);
	if (pf->flags&UF_NNA)
		pt_NNA(pf);
	pt_SetupVoices(pf);
	pt_EffectsPass2(pf);

	/* now set up the actual hardware channel playback information */
	pt_UpdateVoices(pf, max_volume);
}
static void Mod_Player_Reinit_internal(MODULE* mod)
{
	int t;
	UBYTE mute[256];

	//save muted channels informations.
 	for (t=0;t<mod->numchn;t++){
		mute[t] = mod->control[t].muted;
	}

	memset(mod->control,0,mod->numchn*sizeof(MP_CONTROL));
	//memset(mod->voice,0,md_sngchn*sizeof(MP_VOICE));

	for (t=0;t<mod->numchn;t++) {
		mod->control[t].muted = mute[t];
		mod->control[t].main.chanvol=mod->chanvol[t];
		mod->control[t].main.panning=mod->panning[t];
	}

	/*
	mod->sngtime=0;
	mod->sngremainder=0;

	mod->pat_repcrazy=0;
	mod->sngpos=0;
	if(mod->initspeed!=0)
		mod->sngspd=mod->initspeed<32?mod->initspeed:32;
	else
		mod->sngspd=6;
	mod->volume=mod->initvolume>128?128:mod->initvolume;

	mod->vbtick=mod->sngspd;
	mod->patdly=0;
	mod->patdly2=0;
	mod->bpm=mod->inittempo<32?32:mod->inittempo;
	mod->realchn=0;

	mod->patpos=0;
	mod->posjmp=2; 
	mod->numrow=-1;
	mod->patbrk=0;
	*/

}
static void Mod_Player_Init_internal(MODULE* mod)
{
	int t;

	memset(mod->control,0,mod->numchn*sizeof(MP_CONTROL));
	memset(mod->voice,0,md_sngchn*sizeof(MP_VOICE));

	for (t=0;t<mod->numchn;t++) {
		mod->control[t].main.chanvol=mod->chanvol[t];
		mod->control[t].main.panning=mod->panning[t];
	}

	OutputDebugString("Start\n");
	
	mod->sngtime=0;
	mod->sngremainder=0;

	mod->pat_repcrazy=0;
	mod->sngpos=0;
	if(mod->initspeed!=0)
		mod->sngspd=mod->initspeed<32?mod->initspeed:32;
	else
		mod->sngspd=6;
	mod->volume=mod->initvolume>128?128:mod->initvolume;

	mod->vbtick=mod->sngspd;
	mod->patdly=0;
	mod->patdly2=0;
	mod->bpm=mod->inittempo<32?32:mod->inittempo;
	mod->realchn=0;

	mod->patpos=0;
	mod->posjmp=2; /* make sure the player fetches the first note */
	mod->numrow=-1;
	mod->patbrk=0;
}

BOOL Mod_Player_Init(MODULE* mod)
{
	mod->extspd=1;
	mod->panflag=1;
	mod->wrap=0; //modified wrapping as a default value.
	mod->loop=0; // no looping
	mod->fadeout=0;

	mod->relspd=0;

	/* make sure the player doesn't start with garbage */
	if (!(mod->control=(MP_CONTROL*)_mm_calloc(mod->numchn,sizeof(MP_CONTROL))))
		return 1;
	if (!(mod->voice=(MP_VOICE*)_mm_calloc(md_sngchn,sizeof(MP_VOICE))))
		return 1;

	Mod_Player_Init_internal(mod);
	return 0;
}

void Mod_Player_Exit_internal(MODULE* mod)
{
	if (!mod)
		return;

	/* Stop playback if necessary */
	if (mod==pf) {
		Mod_Player_Stop_internal();
		pf=NULL;
	}

	if (mod->control)
		free(mod->control);
	if (mod->voice)
		free(mod->voice);
	mod->control=NULL;
	mod->voice=NULL;
}

void Mod_Player_Exit(MODULE* mod)
{
	MUTEX_LOCK(vars);
	Mod_Player_Exit_internal(mod);
	MUTEX_UNLOCK(vars);
}

MIKMODAPI void Mod_Player_SetVolume(SWORD volume)
{
	MUTEX_LOCK(vars);
	if (pf){
		pf->volume=(volume<0)?0:(volume>128)?128:volume;
		pf->initvolume = pf->volume;
	}
	MUTEX_UNLOCK(vars);
}

MIKMODAPI MODULE* Mod_Player_GetModule(void)
{
	MODULE* result;

	MUTEX_LOCK(vars);
	result=pf;
	MUTEX_UNLOCK(vars);

	return result;
}

MIKMODAPI void Mod_Player_Start(MODULE *mod)
{
	int t;

	if (!mod)
		return;

	if (!MikMod_Active())
		MikMod_EnableOutput();

	mod->forbid=0;

	MUTEX_LOCK(vars);
	if (pf!=mod) {
		/* new song is being started, so completely stop out the old one. */
		if (pf) pf->forbid=1;
		for (t=0;t<md_sngchn;t++) Voice_Stop_internal(t);
	}
	pf=mod;
	MUTEX_UNLOCK(vars);
}

void Mod_Player_Stop_internal(void)
{
#ifdef _XBOX
	if (pf) pf->forbid=1;
	MikMod_DisableOutput_internal();
	pf=NULL;
#else
	if (md_sfxchn) 
		MikMod_DisableOutput_internal();
	if (pf) pf->forbid=1;
	pf=NULL;
#endif
}

MIKMODAPI void Mod_Player_Stop(void)
{
	MUTEX_LOCK(vars);
		Mod_Player_Stop_internal();
	MUTEX_UNLOCK(vars);
}

MIKMODAPI BOOL Mod_Player_Active(void)
{
	BOOL result=0;

	MUTEX_LOCK(vars);
	if (pf)
		result=(!(pf->sngpos>=pf->numpos));
	MUTEX_UNLOCK(vars);

	return result;
}

MIKMODAPI void Mod_Player_NextPosition(void)
{
	MUTEX_LOCK(vars);
	if (pf) {
		int t;

		pf->forbid=1;
		pf->posjmp=3;
		pf->patbrk=0;
		pf->vbtick=pf->sngspd;

		for (t=0;t<md_sngchn;t++) {
			Voice_Stop_internal(t);
			pf->voice[t].main.i=NULL;
			pf->voice[t].main.s=NULL;
		}
		for (t=0;t<pf->numchn;t++) {
			pf->control[t].main.i=NULL;
			pf->control[t].main.s=NULL;
		}
		pf->forbid=0;
	}
	MUTEX_UNLOCK(vars);
}

MIKMODAPI void Mod_Player_PrevPosition(void)
{
	MUTEX_LOCK(vars);
	if (pf) {
		int t;

		pf->forbid=1;
		pf->posjmp=1;
		pf->patbrk=0;
		pf->vbtick=pf->sngspd;

		for (t=0;t<md_sngchn;t++) {
			Voice_Stop_internal(t);
			pf->voice[t].main.i=NULL;
			pf->voice[t].main.s=NULL;
		}
		for (t=0;t<pf->numchn;t++) {
			pf->control[t].main.i=NULL;
			pf->control[t].main.s=NULL;
		}
		pf->forbid=0;
	}
	MUTEX_UNLOCK(vars);
}

MIKMODAPI void Mod_Player_SetPosition(UWORD pos)
{
	MUTEX_LOCK(vars);
	if (pf) {
		int t;

		pf->forbid=1;
		if (pos>=pf->numpos) pos=pf->numpos;
		pf->posjmp=2;
		pf->patbrk=0;
		pf->sngpos=pos;
		pf->vbtick=pf->sngspd;

		for (t=0;t<md_sngchn;t++) {
			Voice_Stop_internal(t);
			pf->voice[t].main.i=NULL;
			pf->voice[t].main.s=NULL;
		}
		for (t=0;t<pf->numchn;t++) {
			pf->control[t].main.i=NULL;
			pf->control[t].main.s=NULL;
		}
		pf->forbid=0;
		
		if (!pos)
			Mod_Player_Reinit_internal(pf);
			//Mod_Player_Init_internal(pf);
	}
	MUTEX_UNLOCK(vars);
}    

static void Mod_Player_Unmute_internal(SLONG arg1,va_list ap)
{
	SLONG t,arg2,arg3=0;

	if (pf) {
		switch (arg1) {
		case MUTE_INCLUSIVE:
			if (((!(arg2=va_arg(ap,SLONG)))&&(!(arg3=va_arg(ap,SLONG))))||
			   (arg2>arg3)||(arg3>=pf->numchn))
				return;
			for (;arg2<pf->numchn && arg2<=arg3;arg2++)
				pf->control[arg2].muted=0;
			break;
		case MUTE_EXCLUSIVE:
			if (((!(arg2=va_arg(ap,SLONG)))&&(!(arg3=va_arg(ap,SLONG))))||
			   (arg2>arg3)||(arg3>=pf->numchn))
				return;
			for (t=0;t<pf->numchn;t++) {
				if ((t>=arg2) && (t<=arg3))
					continue;
				pf->control[t].muted=0;
			}
			break;
		default:
			if (arg1<pf->numchn) pf->control[arg1].muted=0;
			break;
		}
	}
}

MIKMODAPI void Mod_Player_Unmute(SLONG arg1, ...)
{
	va_list args;

	va_start(args,arg1);
	MUTEX_LOCK(vars);
	Mod_Player_Unmute_internal(arg1,args);
	MUTEX_UNLOCK(vars);
	va_end(args);
}

static void Mod_Player_Mute_internal(SLONG arg1,va_list ap)
{
	SLONG t,arg2,arg3=0;

	if (pf) {
		switch (arg1) {
		case MUTE_INCLUSIVE:
			if (((!(arg2=va_arg(ap,SLONG)))&&(!(arg3=va_arg(ap,SLONG))))||
			    (arg2>arg3)||(arg3>=pf->numchn))
				return;
			for (;arg2<pf->numchn && arg2<=arg3;arg2++)
				pf->control[arg2].muted=1;
			break;
		case MUTE_EXCLUSIVE:
			if (((!(arg2=va_arg(ap,SLONG)))&&(!(arg3=va_arg(ap,SLONG))))||
			    (arg2>arg3)||(arg3>=pf->numchn))
				return;
			for (t=0;t<pf->numchn;t++) {
				if ((t>=arg2) && (t<=arg3))
					continue;
				pf->control[t].muted=1;
			}
			break;
		default:
			if (arg1<pf->numchn)
				pf->control[arg1].muted=1;
			break;
		}
	}
}

MIKMODAPI void Mod_Player_Mute(SLONG arg1,...)
{
	va_list args;

	va_start(args,arg1);
	MUTEX_LOCK(vars);
	Mod_Player_Mute_internal(arg1,args);
	MUTEX_UNLOCK(vars);
	va_end(args);
}

static void Mod_Player_ToggleMute_internal(SLONG arg1,va_list ap)
{
	SLONG arg2,arg3=0;
	ULONG t;

	if (pf) {
		switch (arg1) {
		case MUTE_INCLUSIVE:
			if (((!(arg2=va_arg(ap,SLONG)))&&(!(arg3=va_arg(ap,SLONG))))||
			    (arg2>arg3)||(arg3>=pf->numchn))
				return;
			for (;arg2<pf->numchn && arg2<=arg3;arg2++)
				pf->control[arg2].muted=1-pf->control[arg2].muted;
			break;
		case MUTE_EXCLUSIVE:
			if (((!(arg2=va_arg(ap,SLONG)))&&(!(arg3=va_arg(ap,SLONG))))||
			    (arg2>arg3)||(arg3>=pf->numchn))
				return;
			for (t=0;t<pf->numchn;t++) {
				if ((t>=arg2) && (t<=arg3))
					continue;
				pf->control[t].muted=1-pf->control[t].muted;
			}
			break;
		default:
			if (arg1<pf->numchn) 
				pf->control[arg1].muted=1-pf->control[arg1].muted;
			break;
		}
	}
}

MIKMODAPI void Mod_Player_ToggleMute(SLONG arg1,...)
{
	va_list args;

	va_start(args,arg1);
	MUTEX_LOCK(vars);
	Mod_Player_ToggleMute_internal(arg1,args);
	MUTEX_UNLOCK(vars);
	va_end(args);
}

MIKMODAPI BOOL Mod_Player_Muted(UBYTE chan)
{
	BOOL result=1;

	MUTEX_LOCK(vars);
	if (pf)
		result=(chan<pf->numchn)?pf->control[chan].muted:1;
	MUTEX_UNLOCK(vars);

	return result;
}

MIKMODAPI int Mod_Player_GetChannelVoice(UBYTE chan)
{
	int result=0;

	MUTEX_LOCK(vars);
	if (pf)
		result=(chan<pf->numchn)?pf->control[chan].slavechn:-1;
	MUTEX_UNLOCK(vars);

	return result;
}

MIKMODAPI UWORD Mod_Player_GetChannelPeriod(UBYTE chan)
{
	UWORD result=0;

	MUTEX_LOCK(vars);
    if (pf)
	    result=(chan<pf->numchn)?pf->control[chan].main.period:0;
	MUTEX_UNLOCK(vars);

	return result;
}

BOOL Mod_Player_Paused_internal(void)
{
	return pf?pf->forbid:1;
}

MIKMODAPI BOOL Mod_Player_Paused(void)
{
	BOOL result;

	MUTEX_LOCK(vars);
	result=Mod_Player_Paused_internal();
	MUTEX_UNLOCK(vars);

	return result;
}

MIKMODAPI void Mod_Player_TogglePause(void)
{
	MUTEX_LOCK(vars);
	if (pf)
		pf->forbid=1-pf->forbid;
#ifdef _XBOX
	if (md_driver == &drv_xbox)
		md_driver->Pause();
#endif
	MUTEX_UNLOCK(vars);
}

MIKMODAPI void Mod_Player_SetSpeed(UWORD speed)
{
	MUTEX_LOCK(vars);
	if (pf) 
		pf->sngspd=speed?(speed<32?speed:32):1;
	MUTEX_UNLOCK(vars);
}

MIKMODAPI void Mod_Player_SetTempo(UWORD tempo)
{
	if (tempo<32) tempo=32;
	MUTEX_LOCK(vars);
	if (pf) {
		if ((!(pf->flags&UF_HIGHBPM))&&(tempo>255)) tempo=255;
		pf->bpm=tempo;
	}
	MUTEX_UNLOCK(vars);
}

/* ex:set ts=4: */
