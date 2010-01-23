/*

 MikMod Sound System

  By Jake Stine of Divine Entertainment (1996-2000)

 Support:
  If you find problems with this code, send mail to:
    air@divent.org

 Distribution / Code rights:
  Use this source code in any fashion you see fit.  Giving me credit where
  credit is due is optional, depending on your own levels of integrity and
  honesty.

 -----------------------------------------
 Module: LOAD_ABC

  ABC module loader.
	by Peter Grootswagers (2006)
	<email:pgrootswagers@planet.nl>

 Portability:
	All systems - all compilers (hopefully)
*/

#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include "stdafx.h"

#ifdef NEWMIKMOD
#include "mikmod.h"
#include "uniform.h"
typedef UBYTE BYTE;
typedef UWORD WORD;
#else
#include "stdafx.h"
#include "sndfile.h"
#endif

#include "load_pat.h"

#define MAXABCINCLUDES	8
#define MAXCHORDNAMES 80
#define ABC_ENV_DUMPTRACKS		"MMABC_DUMPTRACKS"
#define ABC_ENV_NORANDOMPICK	"MMABC_NO_RANDOM_PICK"

// gchords use tracks with vpos 1 thru 7
// drums use track with vpos 8
// voice chords use vpos 0 and vpos from 11 up
#define GCHORDBPOS 1
#define GCHORDFPOS 2
#define GCHORDCPOS 3
#define DRUMPOS	8
#define DRONEPOS1 9
#define DRONEPOS2 10

// in the patterns a whole note at unmodified tempo is 16 rows
#define ROWSPERNOTE 16
// a 1/64-th note played in triool equals a 1/96-th note, to be able
// to play them and also to play the 1/64-th we need a resolution of 192
// because 2/192 = 1/96 and 3/192 = 1/64
#define RESOLUTION	192

#pragma pack(1)

/**************************************************************************
**************************************************************************/
#ifdef NEWMIKMOD
static char  ABC_Version[] = "ABC+2.0 (draft IV)";
#endif

typedef enum {
	note,
	octave,
	smpno,
	volume,
	effect,
	effoper
} ABCEVENT_X_NOTE;

typedef enum {
	none,
	trill,
	bow,
	accent
} ABCEVENT_X_EFFECT;

typedef enum {
	cmdflag,
	command,
	chordnum,
	chordnote,
	chordbase,
	jumptype
} ABCEVENT_X_CMD;

typedef enum {
	cmdsegno   = '$',
	cmdcapo    = 'B',
	cmdchord   = 'C',
	cmdfine    = 'F',
	cmdhide    = 'H',
	cmdjump    = 'J',
	cmdloop    = 'L',
	cmdcoda    = 'O',
	cmdpartbrk = 'P',
	cmdsync    = 'S',
	cmdtempo   = 'T',
	cmdvariant = 'V',
	cmdtocoda  = 'X'
} ABCEVENT_CMD;

typedef enum {
	jumpnormal,
	jumpfade,
	jumpdacapo,
	jumpdcfade,
	jumpdasegno,
	jumpdsfade,
	jumpfine,
	jumptocoda,
	jumpvariant,
	jumpnot
} ABCEVENT_JUMPTYPE;

typedef struct _ABCEVENT
{
	struct _ABCEVENT *next;
	uint32_t tracktick;
	union {
		uint8_t par[6];
		struct {
			uint8_t flg;
			uint8_t cmd;
			uint32_t lpar;	// for variant selections, bit pattern
		};
	};
	uint8_t part;
	uint8_t tiednote;
} ABCEVENT;

typedef struct _ABCTRACK
{
	struct _ABCTRACK *next;
	ABCEVENT *head;
	ABCEVENT *tail;
	ABCEVENT *capostart;
	ABCEVENT *tienote;
	int transpose;
	int octave_shift;
	uint32_t slidevoltime;	// for crescendo and diminuendo
	int slidevol; // -2:fade away, -1:diminuendo, 0:none, +1:crescendo
	uint8_t vno; // 0 is track is free for use, from previous song in multi-songbook
	uint8_t vpos; // 0 is main voice, other is subtrack for gchords, gchords or drumnotes
	uint8_t tiedvpos;
	uint8_t mute;
	uint8_t chan; // 10 is percussion channel, any other is melodic channel
	uint8_t volume;
	uint8_t instr;	// current instrument for this track
	uint8_t legato;
	char v[22];	// first twenty characters are significant
} ABCTRACK;

typedef struct _ABCMACRO
{
	struct _ABCMACRO *next;
	char *name;
	char *subst;
	char *n;
} ABCMACRO;

/**************************************************************************
**************************************************************************/

typedef struct _ABCHANDLE
{
#ifdef NEWMIKMOD
	MM_ALLOC *allochandle;
	MM_ALLOC *macrohandle;
	MM_ALLOC *trackhandle;
	MM_ALLOC *ho;
#endif
	ABCMACRO *macro;
	ABCMACRO *umacro;
	ABCTRACK *track;
	long int pickrandom;
	unsigned int len;
	int speed;
	char *line;
	char *beatstring;
	uint8_t beat[4]; // a:first note, b:strong notes, c:weak notes, n:strong note every n
	char gchord[80];	// last setting for gchord
	char drum[80]; // last setting for drum 
	char drumins[80]; // last setting for drum 
	char drumvol[80]; // last setting for drum 
	uint32_t barticks;
	// parse variables, declared here to avoid parameter pollution
	int abcchordvol, abcchordprog, abcbassvol, abcbassprog;
	int ktrans;
	int drumon, gchordon, droneon;
	int dronegm, dronepitch[2], dronevol[2];
	ABCTRACK *tp, *tpc, *tpr;
	uint32_t tracktime;
} ABCHANDLE;

static int global_voiceno, global_octave_shift, global_tempo_factor, global_tempo_divider;
static char global_part;
static uint32_t global_songstart;
/* Named guitar chords */
static char chordname[MAXCHORDNAMES][8];
static int chordnotes[MAXCHORDNAMES][6];
static int chordlen[MAXCHORDNAMES];
static int chordsnamed = 0;

static const char *sig[] = {
	" C D EF G A Bc d ef g a b",	// 7 sharps C#
	" C D EF G AB c d ef g ab ",	// 6 sharps F#
	" C DE F G AB c de f g ab ",	// 5 sharps B
	" C DE F GA B c de f ga b ",	// 4 sharps E
	" CD E F GA B cd e f ga b ",	// 3 sharps A
	" CD E FG A B cd e fg a b ",	// 2 sharps D
	" C D E FG A Bc d e fg a b",	// 1 sharps G
	" C D EF G A Bc d ef g a b",	// 0 sharps C
	" C D EF G AB c d ef g ab ",	// 1 flats  F
	" C DE F G AB c de f g ab ",	// 2 flats  Bb
	" C DE F GA B c de f ga b ",	// 3 flats  Eb
	" CD E F GA B cd e f ga b ",	// 4 flats  Ab
	" CD E FG A B cd e fg a b ",	// 5 flats  Db
	"C D E FG A Bc d e fg a b ",	// 6 flats  Gb
	"C D EF G A Bc d ef g a b ",	// 7 flats  Cb
// 0123456789012345678901234	
};

static const char *keySigs[] = {
/* 0....:....1....:....2....:....3....:....4....:....5. */
	"7 sharps: C#    A#m   G#Mix D#Dor E#Phr F#Lyd B#Loc ",
	"6 sharps: F#    D#m   C#Mix G#Dor A#Phr BLyd  E#Loc ",
	"5 sharps: B     G#m   F#Mix C#Dor D#Phr ELyd  A#Loc ",
	"4 sharps: E     C#m   BMix  F#Dor G#Phr ALyd  D#Loc ",
	"3 sharps: A     F#m   EMix  BDor  C#Phr DLyd  G#Loc ",
	"2 sharps: D     Bm    AMix  EDor  F#Phr GLyd  C#Loc ",
	"1 sharp : G     Em    DMix  ADor  BPhr  CLyd  F#Loc ",
	"0 sharps: C     Am    GMix  DDor  EPhr  FLyd  BLoc  ",
	"1 flat  : F     Dm    CMix  GDor  APhr  BbLyd ELoc  ",
	"2 flats : Bb    Gm    FMix  CDor  DPhr  EbLyd ALoc  ",
	"3 flats : Eb    Cm    BbMix FDor  GPhr  AbLyd DLoc  ",
	"4 flats : Ab    Fm    EbMix BbDor CPhr  DbLyd GLoc  ",
	"5 flats : Db    Bbm   AbMix EbDor FPhr  GbLyd CLoc  ",
	"6 flats : Gb    Ebm   DbMix AbDor BbPhr CbLyd FLoc  ",
	"7 flats : Cb    Abm   GbMix DbDor EbPhr FbLyd BbLoc ",
	0
};

// local prototypes
static int abc_getnumber(const char *p, int *number);
static ABCTRACK *abc_locate_track(ABCHANDLE *h, const char *voice, int pos);
static void	abc_add_event(ABCHANDLE *h, ABCTRACK *tp, ABCEVENT *e);
static void abc_add_setloop(ABCHANDLE *h, ABCTRACK *tp, uint32_t tracktime);
static void abc_add_setjumploop(ABCHANDLE *h, ABCTRACK *tp, uint32_t tracktime, ABCEVENT_JUMPTYPE j);
static uint32_t abc_pattracktime(ABCHANDLE *h, uint32_t tracktime);
static int abc_patno(ABCHANDLE *h, uint32_t tracktime);

#ifndef HAVE_SETENV
static void setenv(const char *name, const char *value, int overwrite)
{
	int len = strlen(name)+1+strlen(value)+1;
	char *str = (char *)alloca(len);
	sprintf(str, "%s=%s", name, value);
	putenv(str);
}
#endif


static int abc_isvalidchar(char c) {
	return(isalpha(c) || isdigit(c) || isspace(c) || c == '%' || c == ':');
}


static void abc_message(const char *s1, const char *s2)
{
	char txt[256];
	if( strlen(s1) + strlen(s2) > 255 ) return;
	sprintf(txt, s1, s2);
#ifdef NEWMIKMOD
	_mmlog(txt);
#else
	fprintf(stderr, "load_abc > %s\n", txt);
#endif
}

static uint32_t modticks(uint32_t abcticks)
{
	return abcticks / RESOLUTION;
}

static uint32_t abcticks(uint32_t modticks)
{
	return modticks * RESOLUTION;
}

static uint32_t notelen_notediv_to_ticks(int speed, int len, int div)
{
	uint32_t u;
	u = (ROWSPERNOTE * RESOLUTION * speed * len * global_tempo_factor) / (div * global_tempo_divider);
	return u;
}

static void abc_dumptracks(ABCHANDLE *h, const char *p)
{
	ABCTRACK *t;
	ABCEVENT *e;
	int n,pat,row,tck;
	char nn[3];
	if( !h ) return;
	for( t=h->track; t; t=t->next ) {
		printf("track %d.%d chan=%d %s\n", (int)(t->vno), (int)(t->vpos),
		                                   (int)(t->chan), (char *)(t->v));
		if( strcmp(p,"nonotes") )
			n = 1;
		else
			n = 0;
		for( e=t->head; e; e=e->next ) {
			tck = modticks(e->tracktick);
			row = tck / h->speed;
			pat = row / 64;
			tck = tck % h->speed;
			row = row % 64; 
			nn[0] = ( e->tracktick % abcticks(h->speed * 64) ) ? ' ': '-';
			if( e->flg == 1 ) {
				printf("  %6d.%02d.%d%c%c %d.%d %s ",
				       pat, row, tck, nn[0], (int)(e->part), (int)(t->vno),
				       (int)(t->vpos), (char *)(t->v));
				if( e->cmd == cmdchord ) {
					nn[0] = "CCCDDEFFGGAABccddeffggaabb"[e->par[chordnote]];
					nn[1] = "b # #  # # #  # #  # # # #"[e->par[chordnote]];
					nn[2] = '\0';
					if( isspace(nn[1]) ) nn[1] = '\0';
					printf("CMD %c: gchord %s%s",
					       (char)(e->cmd), nn, chordname[e->par[chordnum]]);
					if( e->par[chordbase] != e->par[chordnote] ) {
						nn[0] = "CCCDDEFFGGAABccddeffggaabb"[e->par[chordbase]];
						nn[1] = "b # #  # # #  # #  # # # #"[e->par[chordbase]];
						nn[2] = '\0';
						printf("/%s", nn);
					}
					printf("\n");
				}
				else
					printf("CMD %c @%p 0x%08lX\n",
					       (char)(e->cmd), e,
					       (unsigned long)(e->lpar));
				if( strcmp(p,"nonotes") )
					n = 1;
				else
					n = 0;
			}
			else if( n ) {
				printf("  %6d.%02d.%d%c%c %d.%d %s ", pat, row, tck, nn[0], e->part, t->vno, t->vpos, t->v);
				if( e->par[note] ) {
					nn[0] = "CCCDDEFFGGAABccddeffggaabb"[e->par[note]-23];
					nn[1] = "b # #  # # #  # #  # # # #"[e->par[note]-23];
					nn[2] = '\0';
				}
				else strcpy(nn,"--");
				printf("NOTE %s octave %d inst %s vol %03d\n", 
					nn, e->par[octave], pat_gm_name(pat_smptogm(e->par[smpno])),e->par[volume]);
				if( strcmp(p,"all") )
					n = 0;
			}
		}
	}
}

#ifdef NEWMIKMOD

#define MMFILE				MMSTREAM
#define mmfgetc(x)			_mm_read_SBYTE(x)
#define mmfeof(x)			_mm_feof(x)
#define mmfgets(buf,sz,f) _mm_fgets(f,buf,sz)
#define mmftell(x)			_mm_ftell(x)
#define mmfseek(f,p,w)		_mm_fseek(f,p,w)
#define mmfopen(s,m)			_mm_fopen(s,m)
#define mmfclose(f)			_mm_fclose(f)

#else

#define MMSTREAM										FILE
#define _mm_fopen(name,mode)		fopen(name,mode)
#define _mm_fgets(f,buf,sz)		fgets(buf,sz,f)
#define _mm_fseek(f,pos,whence)		fseek(f,pos,whence)
#define _mm_ftell(f)								ftell(f)
#define _mm_read_UBYTES(buf,sz,f)	fread(buf,sz,1,f)
#define _mm_read_SBYTES(buf,sz,f)	fread(buf,sz,1,f)
#define _mm_feof(f)									feof(f)
#define _mm_fclose(f)								fclose(f)
#define DupStr(h,buf,sz)		strdup(buf)
#define _mm_calloc(h,n,sz)		calloc(n,sz)
#define _mm_recalloc(h,buf,sz,elsz)	realloc(buf,sz)
#undef _mm_free
#define _mm_free(h,p)								free(p)

typedef struct {
	char *mm;
	int sz;
	int pos;
} MMFILE;

static MMFILE *mmfopen(const char *name, const char *mode)
{
	FILE *fp;
	MMFILE *mmfile;
	long len;
	if( *mode != 'r' ) return NULL;
	fp = fopen(name, mode);
	if( !fp ) return NULL;
	fseek(fp, 0, SEEK_END);
	len = ftell(fp);
	mmfile = (MMFILE *)malloc(len+sizeof(MMFILE));
	if( !mmfile ) return NULL;
	fseek(fp, 0, SEEK_SET);
	fread(&mmfile[1],1,len,fp);
	fclose(fp);
	mmfile->mm = (char *)&mmfile[1];
	mmfile->sz = len;
	mmfile->pos = 0;
	return mmfile;
}

static void mmfclose(MMFILE *mmfile)
{
	free(mmfile);
}

static bool mmfeof(MMFILE *mmfile)
{
	if( mmfile->pos < 0 ) return TRUE;
	if( mmfile->pos < mmfile->sz ) return FALSE;
	return TRUE;
}

static int mmfgetc(MMFILE *mmfile)
{
	int b;
	if( mmfeof(mmfile) ) return EOF;
	b = mmfile->mm[mmfile->pos];
	mmfile->pos++;
	if( b=='\r' && mmfile->mm[mmfile->pos] == '\n' ) {
		b = '\n';
		mmfile->pos++;
	}
	return b;
}

static void mmfgets(char buf[], unsigned int bufsz, MMFILE *mmfile)
{
	int i,b;
	for( i=0; i<(int)bufsz-1; i++ ) {
		b = mmfgetc(mmfile);
		if( b==EOF ) break;
		buf[i] = b;
		if( b == '\n' ) break;
	}
	buf[i] = '\0';
}

static long mmftell(MMFILE *mmfile)
{
	return mmfile->pos;
}

static void mmfseek(MMFILE *mmfile, long p, int whence)
{
	switch(whence) {
		case SEEK_SET:
			mmfile->pos = p;
			break;
		case SEEK_CUR:
			mmfile->pos += p;
			break;
		case SEEK_END:
			mmfile->pos = mmfile->sz + p;
			break;
	}
}
#endif

// =====================================================================================
static ABCEVENT *abc_new_event(ABCHANDLE *h, uint32_t abctick, const char data[])
// =====================================================================================
{
    ABCEVENT   *retval;
		int i;

    retval = (ABCEVENT *)_mm_calloc(h->trackhandle, 1,sizeof(ABCEVENT));
		retval->next        = NULL;
    retval->tracktick   = abctick;
		for( i=0; i<6; i++ )
	    retval->par[i]    = data[i];
		retval->part = global_part;
		retval->tiednote = 0;
    return retval;
}

// =============================================================================
static ABCEVENT *abc_copy_event(ABCHANDLE *h, ABCEVENT *se)
// =============================================================================
{
	ABCEVENT *e;
	e = (ABCEVENT *)_mm_calloc(h->trackhandle, 1,sizeof(ABCEVENT));
	e->next        = NULL;
	e->tracktick   = se->tracktick;
	e->flg         = se->flg;
	e->cmd         = se->cmd;
	e->lpar        = se->lpar;
	e->part        = se->part;
	return e;
}

// =============================================================================
static void abc_new_macro(ABCHANDLE *h, const char *m)
// =============================================================================
{
    ABCMACRO *retval;
		const char *p;
		char buf[256],*q;
		for( p=m; *p && isspace(*p); p++ ) ;
		for( q=buf; *p && *p != '='; p++ )
			*q++ = *p;
		if( q != buf )
			while( isspace(q[-1]) ) q--;
		*q = '\0';
    retval = (ABCMACRO *)_mm_calloc(h->macrohandle, 1,sizeof(ABCTRACK));
    retval->name  = DupStr(h->macrohandle, buf,strlen(buf));
		retval->n     = strrchr(retval->name, 'n'); // for transposing macro's
		for( p++; *p && isspace(*p); p++ ) ;
		strncpy(buf,p,200);
		for( q=&buf[strlen(buf)-1]; q!=buf && isspace(*q); q-- ) *q = '\0';
    retval->subst = DupStr(h->macrohandle, buf, strlen(buf));
		retval->next  = h->macro;
		h->macro      = retval;
}

// =============================================================================
static void abc_new_umacro(ABCHANDLE *h, const char *m)
// =============================================================================
{
    ABCMACRO *retval, *mp;
		const char *p;
		char buf[256], let[2], *q;
		for( p=m; *p && isspace(*p); p++ ) ;
		for( q=buf; *p && *p != '='; p++ )
			*q++ = *p;
		if( q != buf )
			while( isspace(q[-1]) ) q--;
		*q = '\0';
		if( strlen(buf) > 1 || strchr("~HIJKLMNOPQRSTUVWXY",toupper(buf[0])) == 0 || strchr("xy",buf[0]) ) return;
		strcpy(let,buf);
		for( p++; *p && isspace(*p); p++ ) ;
		strncpy(buf,p,200);
		for( q=&buf[strlen(buf)-1]; q!=buf && isspace(*q); q-- ) *q = '\0';
		for( q=buf; *q; q++ )	if( *q == '!' ) *q = '+';	// translate oldstyle to newstyle
		if( !strcmp(buf,"+nil+") ) { // delete a macro
			mp = NULL;
			for( retval=h->umacro; retval; retval = retval->next ) {
				if( retval->name[0] == let[0] ) {	// delete this one
					if( mp ) mp->next = retval->next;
					else h->umacro = retval->next;
					_mm_free(h->macrohandle, retval);
					return;
				}
				mp = retval;
			}
			return;
		}
    retval = (ABCMACRO *)_mm_calloc(h->macrohandle, 1,sizeof(ABCTRACK));
    retval->name  = DupStr(h->macrohandle, let,1);
    retval->subst = DupStr(h->macrohandle, buf, strlen(buf));
		retval->n     = 0;
		retval->next  = h->umacro; // by placing it up front we mask out the old macro until we +nil+ it
		h->umacro      = retval;
}

// =============================================================================
static ABCTRACK *abc_new_track(ABCHANDLE *h, const char *voice, int pos)
// =============================================================================
{
	ABCTRACK *retval;
	if( !pos ) global_voiceno++;
	retval = (ABCTRACK *)_mm_calloc(h->trackhandle, 1,sizeof(ABCTRACK));
	retval->next         = NULL;
	retval->vno          = global_voiceno;
	retval->vpos         = pos;
	retval->tiedvpos     = pos;
	retval->instr        = 1;
	strncpy(retval->v, voice, 20);
	retval->v[20]        = '\0';
	retval->head         = NULL;
	retval->tail         = NULL;
	retval->capostart    = NULL;
	retval->tienote      = NULL;
	retval->mute         = 0;
	retval->chan         = 0;
	retval->transpose    = 0;
	retval->volume       = h->track? h->track->volume: 120;
	retval->slidevoltime = 0;
	retval->slidevol     = 0;
	retval->legato       = 0;
	return retval;
}

static int abc_numtracks(ABCHANDLE *h)
{
	int n;
	ABCTRACK *t;
	n=0;
	for( t = h->track; t; t=t->next )
		n++;
	return n;
}

static int abc_interval(const char *s, const char *d)
{
	const char *p;
	int i,j,k;
	int n,oct,m[2];
	for( j=0; j<2; j++ ) {
		if( j ) p = d; 
		else p = s;
		switch(p[0]) {
			case '^':
				n = p[1];
				i = 2;
				break;
			case '_':
				n = p[1];
				i = 2;
				break;
			case '=':
				n = p[1];
				i = 2;
				break;
			default:
				n = p[0];
				i = 1;
				break;
		}
		for( k=0; k<25; k++ )
			if( n == sig[7][k] )
				break;
		oct = 4;	// ABC note pitch C is C4 and pitch c is C5
		if( k > 12 ) {
			oct++;
			k -= 12;
		}
		while( p[i] == ',' || p[i] == '\'' ) {
			if( p[i] == ',' )
				oct--;
			else
				oct++;
			i++;
		}
		m[j] = k + 12 * oct;
	}
	return m[0] - m[1];
}

static int abc_transpose(const char *v)
{
	int i,j,t;
	const char *m = "B", *mv = "";
	t = 0;
	global_octave_shift = 99;
	for( ; *v && *v != ']'; v++ ) {
		if( !strncasecmp(v,"t=",2) ) {
			v+=2;
			if( *v=='-' )	{
				j = -1;
				v++;
			}
			else j = 1;
			v+=abc_getnumber(v,&i);
			t += i * j;
			global_octave_shift = 0;
		}
		if( !strncasecmp(v,"octave=",7) ) {
			v+=7;
			if( *v=='-' )	{
				j = -1;
				v++;
			}
			else j = 1;
			v+=abc_getnumber(v,&i);
			t += i * j * 12;
			global_octave_shift = 0;
		}
		if( !strncasecmp(v,"transpose=",10) ) {
			v+=10;
			if( *v=='-' )	{
				j = -1;
				v++;
			}
			else j = 1;
			v+=abc_getnumber(v,&i);
			t += i * j;
			global_octave_shift = 0;
		}
		if( !strncasecmp(v,"octave=",7) ) { // used in kv304*.abc
			v+=7;
			if( *v=='-' )	{
				j = -1;
				v++;
			}
			else j = 1;
			v+=abc_getnumber(v,&i);
			t += i * j * 12;
			global_octave_shift = 0;
		}
		if( !strncasecmp(v,"m=",2) ) {
			v += 2;
			mv = v; // get the pitch for the middle staff line 
			while( *v && *v != ' ' && *v != ']' ) v++;
			global_octave_shift = 0;
		}
		if( !strncasecmp(v,"middle=",7) ) {
			v += 7;
			mv = v; // get the pitch for the middle staff line 
			while( *v && *v != ' ' && *v != ']' ) v++;
			global_octave_shift = 0;
		}
		if( !strncasecmp(v,"clef=",5) )
			v += 5;
		j = 1;
		if( !strncasecmp(v,"treble",6) ) {
			j = 0;
			v += 6;
			switch( *v ) {
				case '1':	v++; m = "d";	break;
				case '2': v++;
				default: m = "B";	break;
				case '3': v++; m = "G";	break;
				case '4': v++; m = "E";	break;
				case '5': v++; m = "C";	break;
			}
			global_octave_shift = 0;
		}
		if( j && !strncasecmp(v,"bass",4) ) {
			m = "D,";
			j = 0;
			v += 4;
			switch( *v ) {
				case '1':	v++; m = "C";	break;
				case '2': v++; m = "A,";	break;
				case '3': v++; m = "F,";	break;
				case '4': v++;
				default: m = "D,";	break;
				case '5': v++; m = "B,,";	break;
			}
			if( global_octave_shift == 99 )
				global_octave_shift = -2;
		}
		if( j && !strncasecmp(v,"tenor",5) ) {
			j = 0;
			v += 5;
			switch( *v ) {
				case '1':	v++; m = "G";	break;
				case '2': v++; m = "E";	break;
				case '3': v++; m = "C";	break;
				case '4': v++;
				default: m = "A,";	break;
				case '5': v++; m = "F,";	break;
			}
			if( global_octave_shift == 99 )
				global_octave_shift = 1;
		}
		if( j && !strncasecmp(v,"alto",4) ) {
			j = 0;
			v += 4;
			switch( *v ) {
				case '1':	v++; m = "G";	break;
				case '2': v++; m = "E";	break;
				case '3': v++; 
				default: m = "C";	break;
				case '4': v++; m = "A,";	break;
				case '5': v++; m = "F,";	break;
			}
			if( global_octave_shift == 99 )
				global_octave_shift = 1;
		}
		if( j && strchr("+-",*v) && *v && v[1]=='8' ) {
			switch(*v) {
				case '+':
					t += 12;
					break;
				case '-':
					t -= 12;
					break;
			}
			v += 2;
			if( !strncasecmp(v,"va",2) ) v += 2;
			global_octave_shift = 0;
			j = 0;
		}
		if( j ) {
			while( *v && *v != ' ' && *v != ']' ) v++;
		}
	}
	if( strlen(mv) > 0 ) // someone set the middle note
		t += abc_interval(mv, m);
	if( global_octave_shift == 99 )
		global_octave_shift = 0;
	return t;
}

// =============================================================================
static ABCTRACK *abc_locate_track(ABCHANDLE *h, const char *voice, int pos)
// =============================================================================
{
	ABCTRACK *tr, *prev, *trunused;
	char vc[21];
	int i, trans=0, voiceno=0, instrno = 1, channo = 0;
	for( ; *voice == ' '; voice++ ) ;	// skip leading spaces
	for( i=0; *voice && *voice != ']' && *voice != '%' && !isspace(*voice); voice++ )	// can work with inline voice instructions
		vc[i++] = *voice;
	vc[i] = '\0';
	prev = NULL;
	trunused = NULL;
	if( !pos )	trans = abc_transpose(voice);
	for( tr=h->track; tr; tr=tr->next ) {
		if( tr->vno == 0 ) {
			if( !trunused ) trunused = tr; // must reuse mastertrack (h->track) as first
		}
		else {
			if( !strncasecmp(tr->v, vc, 20) ) {
				if( tr->vpos == pos ) 
					return tr;
				trans = tr->transpose;
				global_octave_shift = tr->octave_shift;
				voiceno = tr->vno;
				instrno = tr->instr;
				channo  = tr->chan;
			}
		}
		prev = tr;
	}
	if( trunused ) {
		tr = trunused;
		if( pos ) {
			tr->vno   = voiceno;
			tr->instr = instrno;
			tr->chan  = channo;
		}
		else {
			global_voiceno++;
	    tr->vno   = global_voiceno;
			tr->instr = 1;
			tr->chan  = 0;
		}
    tr->vpos         = pos;
		tr->tiedvpos     = pos;
		strncpy(tr->v, vc, 20);
		tr->v[20]        = '\0';
		tr->mute         = 0;
		tr->transpose    = trans;
		tr->octave_shift = global_octave_shift;
		tr->volume       = h->track->volume;
		tr->tienote      = NULL;
		tr->legato       = 0;
		return tr;
	}
	tr = abc_new_track(h, vc, pos);
	if( pos ) {
		tr->vno   = voiceno;
		tr->instr = instrno;
		tr->chan  = channo;
	}
	tr->transpose    = trans;
	tr->octave_shift = global_octave_shift;
	if( prev ) prev->next = tr;
	else	h->track = tr;
	return tr;
}

// =============================================================================
static ABCTRACK *abc_check_track(ABCHANDLE *h, ABCTRACK *tp)
// =============================================================================
{
	if( !tp ) { 
		tp = abc_locate_track(h, "", 0);	// must work for voiceless abc too...
		tp->transpose = h->ktrans;
	}
	return tp;
}

static void abc_add_capo(ABCHANDLE *h, ABCTRACK *tp, uint32_t tracktime)
{
	ABCEVENT *e;
	char d[6];
	d[0] = d[1] =	d[2] = d[3] = d[4] = d[5] = 0;
	d[cmdflag] = 1;
	d[command] = cmdcapo;
	e = abc_new_event(h, tracktime, d);
	tp->capostart = e;
	abc_add_event(h, tp, e); // do this last (recursion danger)
}

static void abc_add_segno(ABCHANDLE *h, ABCTRACK *tp, uint32_t tracktime)
{
	ABCEVENT *e;
	char d[6];
	d[0] = d[1] =	d[2] = d[3] = d[4] = d[5] = 0;
	d[cmdflag] = 1;
	d[command] = cmdsegno;
	e = abc_new_event(h, tracktime, d);
	abc_add_event(h, tp, e);
}

static void abc_add_coda(ABCHANDLE *h, ABCTRACK *tp, uint32_t tracktime)
{
	ABCEVENT *e;
	char d[6];
	d[0] = d[1] =	d[2] = d[3] = d[4] = d[5] = 0;
	d[cmdflag] = 1;
	d[command] = cmdcoda;
	e = abc_new_event(h, tracktime, d);
	abc_add_event(h, tp, e);
}

static void abc_add_fine(ABCHANDLE *h, ABCTRACK *tp, uint32_t tracktime)
{
	ABCEVENT *e;
	char d[6];
	d[0] = d[1] =	d[2] = d[3] = d[4] = d[5] = 0;
	d[cmdflag] = 1;
	d[command] = cmdfine;
	e = abc_new_event(h, tracktime, d);
	abc_add_event(h, tp, e);
}

static void abc_add_tocoda(ABCHANDLE *h, ABCTRACK *tp, uint32_t tracktime)
{
	ABCEVENT *e;
	char d[6];
	d[0] = d[1] =	d[2] = d[3] = d[4] = d[5] = 0;
	d[cmdflag] = 1;
	d[command] = cmdtocoda;
	e = abc_new_event(h, tracktime, d);
	abc_add_event(h, tp, e);
}

// first track is dirigent, remove all control events from other tracks
// to keep the information where the events should be relative to note events
// in the same tick the ticks are octated and four added for note events
// the control events that come before the note events get a decremented tick,
// those that come after get an incremented tick, for example:
//             ctrl ctrl note ctrl ctrl  note
//   original: t    t    t    t    t+1   t+1
//   recoded:  8t+1 8t+2 8t+4 8t+5 8t+11 8t+12
static void abc_remove_unnecessary_events(ABCHANDLE *h)
{
	ABCTRACK *tp,*ptp;
	ABCEVENT *ep, *el;
	uint32_t ct, et;
	int d;
	ptp = NULL;
	for( tp=h->track; tp; tp=tp->next ) {
		el = NULL;
		ep = tp->head;
		ct = 0;
		d  = -3;
		while( ep ) {
			et = ep->tracktick;
			ep->tracktick <<= 3;
			ep->tracktick += 4;
			if( ep->flg == 1 ) {
				ep->tracktick += d;	
				d++;
				if( d == 0 ) d = -1;
				if( d == 4 ) d =  3;
				if( tp!=h->track ) ep->cmd = cmdhide;
				switch( ep->cmd ) {
					case cmdhide:
					case cmdsync:
						if( el ) {
							el->next = ep->next;
							_mm_free(h->trackhandle,ep);
							ep = el->next;
						}
						else {
							tp->head = ep->next;
							_mm_free(h->trackhandle,ep);
							ep = tp->head;
						}
						break;
					default:
						el = ep;
						ep = ep->next;
						break;
				}
			}
			else {
				el = ep;
				ep = ep->next;
				d  = 1;
			}
			if( et > ct )
				d = -3;
			ct = et;
		}
		if( !tp->head ) { // no need to keep empty tracks...
			if( ptp ) {
				ptp->next = tp->next;
				_mm_free(h->trackhandle,tp);
				tp = ptp;
			}
			else {
				h->track = tp->next;
				_mm_free(h->trackhandle,tp);
				tp = h->track;
			}
		}
		ptp = tp;	// remember previous track
	}
}

// set ticks back, and handle partbreaks
static void abc_retick_events(ABCHANDLE *h)
{
	ABCTRACK *tp;
	ABCEVENT *ep;
	uint32_t et, tt=0, at = abcticks(64 * h->speed);
	for( tp=h->track; tp; tp=tp->next ) {
		// make ticks relative
		tt = 0;
		for( ep=tp->head; ep; ep=ep->next ) {
			et = ep->tracktick >> 3;
			ep->tracktick = et - tt;
			tt = et;
		}
		// make ticks absolute again, skipping no-op partbreaks
		tt = 0;
		for( ep=tp->head; ep; ep=ep->next ) {
			ep->tracktick += tt;
			tt = ep->tracktick;
			if( ep->flg == 1 && ep->cmd == cmdpartbrk ) {
				if( tt % at ) {
					tt += at;
					tt /= at;
					tt *= at;
					ep->tracktick -= abcticks(h->speed); // break plays current row
				}
				else ep->cmd = cmdhide;
			}
		}
	}
}

// make sure every track has the control events it needs, this way it is not
// necessary to have redundant +segno+ +D.C.+ etc in the voices, the first voice
// is the master, it is pointed to by the member 'track' in the ABCHANDLE
static void abc_synchronise_tracks(ABCHANDLE *h)
{
	ABCTRACK *tp;
	uint32_t tm;	// tracktime in master
	ABCEVENT *em, *es, *et, *ec;	// events in master, slave, slave temporary and copied event
	if( !h || !h->track ) return;
	abc_remove_unnecessary_events(h);
	for( tp = h->track->next; tp; tp = tp->next ) {
		for( em=h->track->head; em; em=em->next ) {
			if( em->flg == 1 ) { // some kind of control event
				switch( em->cmd ) {
					case cmdchord:
					case cmdhide:
					case cmdtempo:
					case cmdsync:
						break;
					default:	// check to see if copy is necessary
						ec = abc_copy_event(h, em);
						tm = em->tracktick;
						es = tp->head; // allways search from the begin...
						for( et=es; et && et->tracktick <= tm; et=et->next )
							es = et;
						if( es == NULL || es->tracktick > tm ) {	// special case: head of track
							ec->next = es;
							tp->head = ec;
						}
						else {
							ec->next = es->next;
							es->next = ec;
						}
						break;
				}
			}
		}
	}
	abc_retick_events(h);
}

static void	abc_add_event(ABCHANDLE *h, ABCTRACK *tp, ABCEVENT *e)
{
	if( !tp->capostart ) abc_add_capo(h, tp, global_songstart);
	if( tp->tail ) {
		tp->tail->next = e;
		tp->tail = e;
	}
	else {
		tp->head = e;
		tp->tail = e;
	}
}

static void abc_add_partbreak(ABCHANDLE *h, ABCTRACK *tp, uint32_t tracktime)
{
	ABCEVENT *e;
	char d[6];
	d[0] = d[1] =	d[2] = d[3] = d[4] = d[5] = 0;
	d[cmdflag] = 1;
	d[command] = cmdpartbrk;
	e = abc_new_event(h, tracktime, d);
	abc_add_event(h, tp, e);
}

static void abc_add_tempo_event(ABCHANDLE *h, ABCTRACK *tp, uint32_t tracktime, int tempo)
{
	ABCEVENT *e;
	char d[6];
	d[0] = d[1] =	d[2] = d[3] = d[4] = d[5] = 0;
	d[cmdflag] = 1;
	d[command] = cmdtempo;
	e = abc_new_event(h, tracktime, d);
	e->lpar = tempo;
	abc_add_event(h, tp, e);
}

static void abc_add_noteoff(ABCHANDLE *h, ABCTRACK *tp, uint32_t tracktime)
{
	ABCEVENT *e;
	char d[6];
	d[note]    = 0;
	d[octave]  = 0;
	d[smpno]   = pat_gmtosmp(tp->instr);
	d[volume]  = 0;
	d[effect]  = 0;
	d[effoper] = 0;
	e = abc_new_event(h, tracktime, d);
	abc_add_event(h, tp, e);
}

static int abc_dynamic_volume(ABCTRACK *tp, uint32_t tracktime, int vol)
{
	uint32_t slidetime;
	int voldelta;
	if( tp->mute ) return 0;
	if( tp->slidevol == 0 ) return vol;
	if( tracktime < tp->slidevoltime ) return vol;
	slidetime = modticks(tracktime - tp->slidevoltime);
	voldelta = (slidetime * 15) / 64 / 6;	// slide from say mf up to f in one pattern's time
	if( tp->slidevol > -2 && voldelta > 15 ) voldelta = 15;	// never to much dynamics
	if( tp->slidevol > 0 ) vol += voldelta;
	else vol -= voldelta;
	if( vol < 2 ) vol = 2; // xmms divides this by 2....
	if( vol > 127 ) vol = 127;
	return vol;
}

static void abc_track_untie_short_chordnotes(ABCHANDLE *h)
{
	ABCTRACK *tp;
	int vn;
	tp = h->tp;
	vn = tp->vno;
	for( tp = h->track; tp; tp = tp->next )
		if( tp != h->tp && tp->vno == vn && tp->tienote ) {
			abc_message("short notes in chord can not be tied:\n%s", h->line);
			tp->tienote = 0;
		}
}

static void abc_track_clear_tiednote(ABCHANDLE *h)
{
	ABCTRACK *tp;
	int vn;
	tp = h->tp;
	vn = tp->vno;
	for( tp = h->track; tp; tp = tp->next )
		if( tp->vno == vn ) tp->tienote = 0;
}

static void abc_track_clear_tiedvpos(ABCHANDLE *h)
{
	ABCTRACK *tp;
	int vn;
	tp = h->tp;
	vn = tp->vno;
	for( tp = h->track; tp; tp = tp->next )
		if( tp->vno == vn ) tp->tiedvpos = tp->vpos;
}

static ABCTRACK *abc_track_with_note_tied(ABCHANDLE *h, uint32_t tracktime, int n, int oct)
{
	int vn, vp;
	ABCTRACK *tp;
	ABCEVENT *e;
	tp = h->tp;
	vn = tp->vno;
	vp = tp->vpos;
	for( tp = h->track; tp; tp = tp->next ) {
		if( tp->vno == vn ) {
			e = tp->tienote;
			if( e && e->tracktick < tracktime
			&& e->par[octave] == oct && abs(e->par[note] - n) < 3 ) {
				if( tp->vpos != vp ) tp->tiedvpos = vp;
				h->tp = tp;
				return tp;
			}
		}
	}
	tp = h->tp;
	vp = tp->tiedvpos;
	if( tp->vpos != vp ) {
		// chord note track allready returned in previous call
		for( tp = h->track; tp; tp = tp->next ) {
			if( tp->vno == vn && tp->vpos == vp ) {
				tp->tiedvpos = h->tp->vpos;
				h->tp = tp;
				return tp;
			}
		}
	}
	return h->tp;
}

static int abc_add_noteon(ABCHANDLE *h, int ch, const char *p, uint32_t tracktime, char *barkey, int vol, ABCEVENT_X_EFFECT fx, int fxop)
{
	ABCEVENT *e;
	ABCTRACK *tp;
	int i,j,k;
	int n,oct;
	char d[6];
	tp = h->tp;
	switch(ch) {
		case '^':
			if( p[0] == '^' ) {
				n = p[1];
				i = 2;
				ch = 'x';
			}
			else {
				n = p[0];
				i = 1;
			}
			break;
		case '_':
			if( p[0] == '_' ) {
				n = p[1];
				i = 2;
				ch = 'b';
			}
			else {
				n = p[0];
				i = 1;
			}
			break;
		case '=':
			n = p[0];
			i = 1;
			break;
		default:
			n = ch;
			i = 0;
			break;
	}
	for( k=0; k<51; k++ ) {
		if( n == barkey[k] )
			break;
	}
	j = k;
	if( k > 24 )
		k -= 25; // had something like A# over Bb key F signature....
	if( i ) {
		// propagate accidentals if necessary
		// DON'T do redundant accidentals they're always relative to C-scale
		for( k=0; k<25; k++ ) {
			if( n == sig[7][k] )
				break;
		}
		if( k < 25 ) { // only do real notes...
		switch(ch) {
			case 'x':
				k++;
			case '^':
				k++;
				break;
			case 'b':
				k--;
			case '_':
				k--;
				break;
			case '=':
						break;
				}
			if( j < 25 ) // was it not A# over Bb?
			barkey[j] = ' ';
			barkey[k] = n;
		}
	}
	oct = 3;	// ABC note pitch C is C4 and pitch c is C5
	if( k < 25 ) {
		k += tp->transpose;
		while( k > 12 ) {
			oct++;
			k -= 12;
		}
		while( k < 0 ) {
			oct--;
			k += 12;
		}
		d[note] = 23 + k;	// C0 is midi notenumber 24
	}
	else
		d[note] = 0; // someone has doen ^X3 or something like it...
	while( p[i] && strchr(",'",p[i]) ) {
		if( p[i]==',' ) oct--;
		else oct++;
		i++;
		tp->octave_shift = 0;	// forget we ever had to look at it
	}
	if( tp->octave_shift )
		tp->transpose += 12 * tp->octave_shift;
	oct += tp->octave_shift;
	tp->octave_shift = 0;	// after the first note we never have to look at it again
	if( oct < 0 ) oct = 0;
	if( oct > 9 ) oct = 9;
	d[octave]  = oct;
	d[smpno]   = pat_gmtosmp(tp->instr);
	d[volume]  = abc_dynamic_volume(tp, tracktime, vol);
	d[effect]  = fx; // effect
	d[effoper] = fxop;
	tp = abc_track_with_note_tied(h, tracktime, d[note], oct);
	if( tp->tienote ) {
		if( tp->tienote->par[note] != d[note] ) {
			if( abs(tp->tienote->par[note] - d[note]) < 3 ) {
				// may be tied over bar symbol, recover local accidental to barkey
				k = tp->tienote->par[note] - 23 - tp->transpose;
				while( k < 0 ) k += 12;
				while( k > 12 ) k -= 12;
				if( (isupper(n) && barkey[k+12] == ' ') || (islower(n) && barkey[k] == ' ') ) {
					barkey[j] = ' ';
					if( isupper(n) )
						barkey[k] = n;
					else
						barkey[k+12] = n;
					d[note]   = tp->tienote->par[note];
					d[octave] = tp->tienote->par[octave];
				}
			}
		}
	}
	if( tp->tienote
	&& tp->tienote->par[note]   == d[note]
	&& tp->tienote->par[octave] == d[octave] ) {
		for( e = tp->tienote; e; e = e->next ) {
			if( e->par[note] == 0 && e->par[octave] == 0 ) {	// undo noteoff
				e->flg = 1;
				e->cmd = cmdhide;
				e->lpar = 0;
				break;
			}
		}
		tp->tienote->tiednote = 1; // mark him for the pattern writers
		for( j=i; isdigit(p[j]) || p[j]=='/'; j++ ) ; // look ahead to see if this one is tied too
		if( p[j] != '-' ) // is this note tied too?
			tp->tienote = NULL; // if not the tie ends here...
		return i;
	}
	tp->tienote = NULL;
	if( tp->tail 
	&& tp->tail->tracktick == tracktime
	&& tp->tail->par[note]      == 0
	&& tp->tail->par[octave]    == 0 ) {
		for( j=0; j<6; j++ )
	    tp->tail->par[j]   = d[j];
	}
	else {
		e = abc_new_event(h, tracktime, d);
		abc_add_event(h, tp, e);
	}
	if( i > 0 && p[i-1] == '"' ) {
		i--; // someone coded a weird note like ^"E"
		abc_message("strange note encountered scanning %s", h->line);
	}
	return i;
}

static void abc_add_dronenote(ABCHANDLE *h, ABCTRACK *tp, uint32_t tracktime, int nnum, int vol)
{
	ABCEVENT *e;
	int j,k;
	int oct;
	char d[6];
	oct = -1;	// ABC note pitch C is C4 and pitch c is C5
	k = nnum + 1;
	while( k > 12 ) {
		oct++;
		k -= 12;
	}
	while( k < 0 ) {
		oct--;
		k += 12;
	}
	if( oct < 0 ) oct = 0;
	d[note] = 23 + k;	// C0 is midi notenumber 24
	d[octave]  = oct;
	d[smpno]   = pat_gmtosmp(tp->instr);
	d[volume]  = abc_dynamic_volume(tp, tracktime, vol);
	d[effect]  = 0; // effect
	d[effoper] = 0;
	if( tp->tail 
	&& tp->tail->tracktick == tracktime
	&& tp->tail->par[note]      == 0
	&& tp->tail->par[octave]    == 0 ) {
		for( j=0; j<6; j++ )
	    tp->tail->par[j]   = d[j];
	}
	else {
		e = abc_new_event(h, tracktime, d);
		abc_add_event(h, tp, e);
	}
}

static void abc_add_chordnote(ABCHANDLE *h, ABCTRACK *tp, uint32_t tracktime, int nnum, int vol)
{
	abc_add_dronenote(h, tp, tracktime, nnum + 23, tp->mute? 0: vol);
}

static void abc_add_drumnote(ABCHANDLE *h, ABCTRACK *tp, uint32_t tracktime, int nnum, int vol)
{
	abc_add_dronenote(h, tp, tracktime, nnum, tp->mute? 0: vol);
}

static void abc_add_variant_start(ABCHANDLE *h, ABCTRACK *tp, uint32_t tracktime, int n)
{
	ABCEVENT *e;
	char d[6];
	d[0] = d[1] =	d[2] = d[3] = d[4] = d[5] = 0;
	d[cmdflag] = 1;
	d[command] = cmdvariant;
	e = abc_new_event(h, tracktime, d);
	e->lpar = 1<<n;
	abc_add_event(h, tp, e);
}

static void abc_add_variant_choise(ABCTRACK *tp, int n)
{
  tp->tail->lpar |= 1<<n;
}

static void	abc_add_chord(const char *p, ABCHANDLE *h, ABCTRACK *tp, uint32_t tracktime)
{
	ABCEVENT *e;
	char d[6];
	char s[8];
	int i;
	const char *n = " C D EF G A Bc d ef g a b";
	d[0] = d[1] =	d[2] = d[3] = d[4] = d[5] = 0;
	d[cmdflag] = 1;
	d[command] = cmdchord;
	if( p[0] == '(' ) p++;	// chord between parens like: (C)
	for( i=0; n[i]; i++ )
		if( *p == n[i] ) {
			d[chordnote] = i;
			break;
		}
	p++;
	switch(*p) {
		case 'b':
			d[chordnote]--;
			p++;
			break;
		case '#':
			d[chordnote]++;
			p++;
			break;
	}
	d[chordbase] = d[chordnote];
	for( i=0; p[i] && p[i] != '"' && p[i] != '/' && p[i] != '(' && p[i] != ')' && p[i] != ' '; i++ ) s[i] = p[i];
	s[i] = '\0';
	p = &p[i];
	if( *p=='/' ) {
		p++;
		for( i=0; n[i]; i++ )
			if( *p == n[i] ) {
				d[chordbase] = i;
				break;
			}
		p++;
		switch(*p) {
			case 'b':
				d[chordbase]--;
				p++;
				break;
			case '#':
				d[chordbase]++;
				p++;
				break;
		}
	}
	for( i=0; i<chordsnamed; i++ )
		if( !strcmp(s, chordname[i]) ) {
			d[chordnum] = i;
			break;
		}
	if( i==chordsnamed ) {
		abc_message("Failure: unrecognized chordname %s",s);
		return;
	}
	e = abc_new_event(h, tracktime, d);
	abc_add_event(h, tp, e);
}

static void abc_add_setloop(ABCHANDLE *h, ABCTRACK *tp, uint32_t tracktime)
{
	ABCEVENT *e;
	char d[6];
	d[0] = d[1] =	d[2] = d[3] = d[4] = d[5] = 0;
	d[cmdflag] = 1;
	d[command] = cmdloop;
	e = abc_new_event(h, tracktime, d);
	abc_add_event(h, tp, e);
}

static void abc_fade_track(ABCTRACK *tp, ABCEVENT *e)
{
	while(e) {
		if( e->flg != 1 && e->par[note] != 0 )
			e->par[volume] = abc_dynamic_volume(tp, e->tracktick, e->par[volume]);
		e = e->next;
	}
}

static void abc_add_setjumploop(ABCHANDLE *h, ABCTRACK *tp, uint32_t tracktime, ABCEVENT_JUMPTYPE j)
{
	ABCEVENT *e;
	char d[8];
	d[0] = d[1] =	d[2] = d[3] = d[4] = d[5] = 0;
	d[cmdflag] = 1;
	d[command] = cmdjump;
	d[jumptype] = j;
	e = abc_new_event(h, tracktime, d);
	abc_add_event(h, tp, e);
}

static void abc_add_sync(ABCHANDLE *h, ABCTRACK *tp, uint32_t tracktime)
{
	ABCEVENT *e;
	char d[6];
	e = tp->tail;
	if( e && e->tracktick == tracktime ) return;
	if( e && e->flg == 1 && e->cmd == cmdsync ) {
		e->tracktick = tracktime;
		return;
	}
	d[0] = d[1] =	d[2] = d[3] = d[4] = d[5] = 0;
	d[cmdflag] = 1;
	d[command] = cmdsync;
	e = abc_new_event(h, tracktime, d);
	abc_add_event(h, tp, e);
}

static void abc_add_gchord_syncs(ABCHANDLE *h, ABCTRACK *tpc, uint32_t tracktime)
{
	ABCTRACK *tp;
	int i;
	for( i = GCHORDBPOS; i < DRUMPOS; i++ ) {
		tp = abc_locate_track(h, tpc->v, i);
		abc_add_sync(h,tp,tracktime);
	}
}

static void abc_add_drum_sync(ABCHANDLE *h, ABCTRACK *tpr, uint32_t tracktime)
{
	ABCTRACK *tp;
	tp = abc_locate_track(h, tpr->v, DRUMPOS);
	abc_add_sync(h,tp,tracktime);
}

static int abc_getnumber(const char *p, int *number)
{
	int i,h;
	i = 0;
	h = 0;
	while( isdigit(p[i]) ) {
		h = 10 * h + p[i] - '0';
		i++;
	}
	if( i==0 )
		*number = 1;
	else
		*number = h;
	return i;
}

static int abc_getexpr(const char *p, int *number)
{
	int i, term, total;
	i = 0;
	while( isspace(p[i]) )
		i++;
	if( p[i] == '(' ) {
		i += abc_getexpr(p+i+1, number);
		while( p[i] && (p[i] != ')') )
			i++;
		return i;
		}
	i += abc_getnumber(p+i, &total);
	while( isspace(p[i]) )
		i++;
	while( p[i] == '+' ) {
		i += abc_getexpr(p+i+1, &term);
		total += term;
		while( isspace(p[i]) )
			i++;
	}
	*number = total;
	return i;
}

static int abc_notelen(const char *p, int *len, int *div)
{
	int i,h,k;
	i = abc_getnumber(p,len);
	h = 1;
	while( p[i] == '/' ) {
		h *= 2;
		i++;
	}
	if( isdigit(p[i]) ) {
		h /= 2;
		i += abc_getnumber(p+i,&k);
	}
	else k = 1;
	*div = h * k;
	return i;
}

static int abc_brokenrithm(const char *p, int *nl, int *nd, int *b, int hornpipe)
{
	switch( *b ) {
		case '<':
			*nl *= 3;
			*nd *= 2;
			hornpipe = 0;
			break;
		case '>':
			*nd *= 2;
			hornpipe = 0;
			break;
	}
	*b = *p;
	switch( *b ) {
		case '>':
			*nl *= 3;
			*nd *= 2;
			return 1;
		case '<':
			*nd *= 2;
			return 1;
		default:
			*b = 0;
			break;
	}
	if( hornpipe ) { // still true then make 1/8 notes broken rithme
		if( *nl == 1 && *nd == 1 ) {
			*b = '>';
			*nl = 3;
			*nd = 2; 
		}
	}  
	return 0;
}

// put p notes in the time q for the next r notes
static int abc_tuplet(int *nl, int *nd, int p, int q, int r)
{
	if( !r ) return 0;
	*nl *= q;
	*nd *= p;
	return r - 1;
}

// evaluate [Q:"string" n1/m1 n2/m2 n3/m3 n4/m4=bpm "string"] 
// minimal form [Q:"string"]
// most used form [Q: 1/4=120]
static int abc_extract_tempo(const char *p, int invoice)
{
	int nl, nd, ns, in, tempo;
	int nl1=0, nd1, notes, state;
	const char *q;
	in = 0;
	nl = 0;
	nd = 1;
	ns = 120;
	notes = 0;
	state = 0;
	for( q=p; *q; q++ ) {
		if( in ) {
			if( *q=='"' )
				in = 0;
		}
		else {
			if( *q == ']' ) break;
			switch( *q ) {
				case '"':
					in = 1;
					break;
				case '/':
					notes++;
					state = 1;
					nl1 = ns;
					break;
				case '=':
					break;
				default:
					if( isdigit(*q) ) {
						if( state ) {
							q+=abc_getnumber(q,&nd1)-1;
							state = 0;
							nl = nl * nd1 + nl1 * nd;
							nd = nd * nd1;
						}
						else
							q+=abc_getnumber(q,&ns)-1;
					}
					break;
			}
		}
	}
	if( !notes ) {
		nl = 1;
		nd = 4;
	}
	if( !nd ) tempo = 120;
	else tempo = ns * nl * 4 / nd; // mod tempo is really BPM where one B is equal to a quartnote
	if( invoice ) {
		nl = global_tempo_factor;
		nd = global_tempo_divider;
	}
	global_tempo_factor = 1;
	global_tempo_divider = 1;
	while( tempo/global_tempo_divider > 255 )
		global_tempo_divider++;
	tempo /= global_tempo_divider;
	while( tempo * global_tempo_factor < 256 )
		global_tempo_factor++;
	global_tempo_factor--;
	tempo *= global_tempo_factor;
	if( tempo * 3 < 512 ) {
		global_tempo_factor *= 3;
		global_tempo_divider *= 2;
		tempo = (tempo * 3) / 2;
	}
	if( invoice ) {
		if( nl != global_tempo_factor || nd != global_tempo_divider ) {
			ns = (tempo * nl * global_tempo_divider) / (nd * global_tempo_factor);
			if( ns > 31 && ns < 256 ) {
				tempo = ns;
				global_tempo_factor = nl;
				global_tempo_divider = nd;
			}
			else
				abc_message("Failure: inconvenient tempo change in middle of voice (%s)", p);
		}
	}
	return tempo;
}

static void	abc_set_parts(char **d, char *p)
{
	int i,j,k,m,n;
	char *q;
#ifdef NEWMIKMOD
	static MM_ALLOC *h;
	if( *d )	_mmalloc_close(h);
#else
	if( *d )	free(*d);
#endif
	*d = 0;
	if( !p ) return;
	for( i=0; p[i] && p[i] != '%'; i++ ) {
		if( !strchr("ABCDEFGHIJKLMNOPQRSTUVWXYZ().0123456789 ",p[i]) ) {
			abc_message("invalid characters in part string scanning P:%s", p);
			return;
		}
	}
#ifdef NEWMIKMOD
	h = _mmalloc_create("Load_ABC_parts", NULL);
#endif
	// decode constructs like "((AB)2.(CD)2)3.(AB)E2" to "ABABCDCDABABCDCDABABCDCDABEE"
	// first compute needed storage...
	j=0;
	k=0;
	for( i=0; p[i] && p[i] != '%'; i++ ) {
		if( isupper(p[i]) ) {
			j++;
		}
		if( isdigit(p[i]) ) {
			n=abc_getnumber(p+i,&k);
			if( p[i-1] == ')' )
				j *= k;	// never mind multiple parens, just take the worst case
			else
				j += k-1;
			i += n-1;
		}
	}
	q = (char *)_mm_calloc(h, j+1, sizeof(char));	// enough storage for the worst case
	// now copy bytes from p to *d, taking parens and digits in account
	j = 0;
	for( i=0; p[i] && p[i] != '%'; i++ ) {
		if( isdigit(p[i]) || isupper(p[i]) || p[i] == '(' || p[i] == ')' ) {
			if( p[i] == ')' ) {
				for( n=j; n > 0 && q[n-1] != '('; n-- )	;	// find open paren in q
				// q[n+1] to q[j] contains the substring that must be repeated
				if( n > 0 ) {
					for( k = n; k<j; k++ ) q[k-1] = q[k];	// shift to the left...
					j--;
				}
				else
					abc_message("Warning: Unbalanced right parens in P: definition %s",p);
				n = j - n + 1;	// number of repeatable characters
				i += abc_getnumber(p+i+1,&k);
				while( k-- > 1 ) {
					for( m=0; m<n; m++ ) {
						q[j] = q[j-n];
						j++;
					}
				}
				continue;
			}
			if( isdigit(p[i]) ) {
				n = abc_getnumber(p+i,&k);
				i += n - 1;
				while( k-- > 1 ) {
					q[j] = q[j-1];
					j++;
				}
				continue;
			}
			q[j] = p[i];
			j++;
		}
	}
	q[j] = '\0';
	// remove any left over parens
	for( i=0; i<j; i++ ) {
		if( q[i] == '(' ) {
			abc_message("Warning: Unbalanced left parens in P: definition %s",p);
			for( k=i; k<j; k++ ) q[k] = q[k+1];
			j--;
		}
	}
	*d = q;
}

static void abc_appendpart(ABCHANDLE *h, ABCTRACK *tp, uint32_t pt1, uint32_t pt2)
{
	ABCEVENT *e, *ec;
	uint32_t dt;
	dt = tp->tail->tracktick - pt1;
	for( e=tp->head; e && e->tracktick <= pt2; e=e->next ) {
		if( e->tracktick >= pt1 ) {
			if( e->flg != 1 || e->cmd == cmdsync || e->cmd == cmdchord ) {
				if( e != tp->tail ) {
					// copy this event at tail
					ec = abc_copy_event(h,e);
					ec->tracktick += dt;
					ec->part = '*';
					tp->tail->next = ec;
					tp->tail = ec;
				}
			}
		}
	}
	abc_add_sync(h, tp, pt2 + dt); // make sure there is progression...
}

static uint32_t abc_pattracktime(ABCHANDLE *h, uint32_t tracktime)
{
	ABCEVENT *e;
	uint32_t dt,et,pt=abcticks(64 * h->speed);
	if(!h || !h->track || !h->track->head ) return 0;
	dt = 0;
	for( e=h->track->head; e && e->tracktick <= tracktime; e=e->next ) {
		if( e->flg == 1 && e->cmd == cmdpartbrk ) {
			et = e->tracktick + dt;
			if( et % pt ) {
				et += pt;
				et /= pt;
				et *= pt;
				dt = et - e->tracktick;
			}
		}
	}
	return (tracktime + dt);
}

static int abc_patno(ABCHANDLE *h, uint32_t tracktime)
{
	return modticks(abc_pattracktime(h, tracktime)) / 64 / h->speed;
}

static void abc_stripoff(ABCHANDLE *h, ABCTRACK *tp, uint32_t tt)
{
	ABCEVENT *e1, *e2;
	e2 = NULL;
	for( e1 = tp->head; e1 && e1->tracktick <= tt; e1=e1->next )
		e2 = e1;
	if( e2 ) {
		e1 = e2->next;
		tp->tail = e2;
		e2->next = NULL;
	}
	else {
		e1 = tp->tail;
		tp->head = NULL;
		tp->tail = NULL;
	}
	while( e1 ) {
		e2 = e1->next;
		_mm_free(h->trackhandle,e1);
		e1 = e2;
	}
}

static void abc_keeptiednotes(ABCHANDLE *h, uint32_t fromtime, uint32_t totime) {
	ABCTRACK *tp;
	ABCEVENT *e,*n,*f;
	if( totime <= fromtime ) return;
	for( tp=h->track; tp; tp=tp->next ) {
		if( tp->vno ) { // if track is in use...
			n = NULL;
			for( e=tp->head; e && e->tracktick < fromtime; e = e->next )
				if( e->flg != 1 ) n = e; // remember it when it is a note event
			if( n && n->tiednote ) { // we've a candidate to tie over the break
				while( e && e->tracktick < totime ) e=e->next; // skip to other part
				if( e && e->tracktick == totime ) { // if this is on begin row of this part
					f = NULL;
					while( !f && e && e->tracktick == totime ) {
						if( e->flg != 1 ) f = e;
						e = e->next;
					}
					if( f && f->par[note] ) { // pfoeie, we've found a candidate
						if( abs(n->par[note] - f->par[note]) < 3 ) { // undo the note on
							f->flg = 1;
							f->cmd = cmdhide;
							f->lpar = 0;
						}
					}
				}
			}
		}
	}
}

static uint32_t abc_fade_tracks(ABCHANDLE *h, char *abcparts, uint32_t ptt[27])
{
	ABCTRACK *tp;
	ABCEVENT *e0;
	char *p;
	int vol;
	uint32_t pt1, pt2;
	uint32_t tt;
	tt = h->track->tail->tracktick;
	for( tp=h->track->next; tp; tp=tp->next ) {
		if( !tp->tail ) abc_add_sync(h, tp, tt); // no empty tracks please...
		if( tp->tail->tracktick > tt ) abc_stripoff(h, tp, tt); // should not happen....
		if( tp->tail->tracktick < tt ) abc_add_sync(h, tp, tt);
	}
	for( tp=h->track; tp; tp=tp->next ) {
		vol = 127;
		e0 = tp->tail;
		if( tp->slidevol != -2 ) {
			tp->slidevol = -2;
			tp->slidevoltime = e0->tracktick;
		}
		tp->mute = 0; // unmute track for safety, notes in a muted track already have zero volume...
		while( vol > 5 ) {
			for( p=abcparts; *p && vol > 5; p++ ) {
				pt1 = ptt[*p-'A'];
				pt2 = ptt[*p-'A'+1];
				abc_appendpart(h, tp, pt1, pt2);
				vol = abc_dynamic_volume(tp, tp->tail->tracktick, 127);
			}
		}
		abc_fade_track(tp,e0);
	}
	return h->track->tail->tracktick;
}

static void abc_song_to_parts(ABCHANDLE *h, char **abcparts, BYTE partp[27][2])
{
	uint32_t starttick;
	ABCEVENT *e;
	int i, fading, loop, normal, partno, partsegno, partloop, partcoda, parttocoda, partfine, skip, x, y;
	int vmask[27],nextp[27];
	uint32_t ptt[27];
	char buf[256];	// must be enough, mod's cannot handle more than 240 patterns
	char *pfade;
	if( !h || !h->track || !h->track->capostart ) return;
	strcpy(buf,"A");	// initialize our temporary array
	i = 1;
	loop = 1;
	partno = 0;
	partsegno = 0;
	partloop = 0;
	partcoda = -1;
	parttocoda = -1;
	partfine = -1;
	starttick = h->track->capostart->tracktick;
	ptt[0] = starttick;
	vmask[0] = -1;
	nextp[0] = 1;
	for( e=h->track->capostart; e; e=e->next ) {
		if( e->flg == 1 ) {
			switch( e->cmd ) {
				case cmdpartbrk:
					if( e->tracktick > starttick) {
						starttick = e->tracktick;	// do not make empty parts
						if( partno < 26 ) {
							partno++;
							ptt[partno] = starttick;
						}
						if( i < 255 )	buf[i++] = partno+'A';
						vmask[partno] = -1;
						nextp[partno] = partno+1;
					}
					break;
				case cmdloop:
					partloop = partno;
					loop = 1; // start counting anew...
					break;
				case cmdvariant:
					vmask[partno] = e->lpar;
					break;
				case cmdjump:
					x = 0;
					fading = 0;
					normal = 0;
					skip   = 0;
					pfade  = &buf[i];
					switch( e->par[jumptype] ) {
						case jumpfade:
							fading = 1;
						case jumpnormal:
							normal = 1;
							x = partloop;
							loop++;
							break;
						case jumpdsfade:
							fading = 1;
						case jumpdasegno:
							x = partsegno;
							break;
						case jumpdcfade:
							fading = 1;
						case jumpdacapo:
							x = 0;
							break;
						default:
							x = 0;
							break;
					}
					if( vmask[partno] != -1 ) nextp[partno] = x;
					if( partno < 26 ) ptt[partno+1] = e->tracktick; // for handling ties over breaks
					while( x <= partno ) {
						if( skip == 1 && x == partcoda ) skip = 0;
						y = !skip;
						if( y ) {
							if( !normal ) {
								if( x == partfine ) skip = 2;
								if( x == parttocoda ) skip = 1;
								y = !skip;
							}  
							if( !(vmask[x] & (1<<loop)) ) y = 0;
						}
						if( y ) {
							if( i < 255 )	buf[i++] = x+'A';
							if( nextp[x] != x + 1 ) loop++;
							x = nextp[x];
						}
						else
							x++;
					}
					if( fading && partno < 26 && i < 255 ) {	// add single part with fading tracks
						partno++;
						ptt[partno] = e->tracktick;
						buf[i] = '\0'; // close up pfade with zero byte
						starttick = abc_fade_tracks(h, pfade, ptt);
						buf[i++] = partno+'A';
						partno++;
						ptt[partno] = starttick;
						buf[i++] = partno+'A'; // one extra to throw away...
						e = h->track->tail; // this is the edge of the world captain...
					}
					break;
				case cmdtocoda:
					parttocoda = partno;
					break;
				case cmdcoda:
					partcoda = partno;
					break;
				case cmdfine:
					partfine = partno;
					break;
				case cmdsegno:
					partsegno = partno;
					break;
			}
		}
		e->part = partno+'a';	// small caps for generated parts...
	}
	i--; // strip off last partno
	if( partno > 0 ) partno--;
	buf[i] = '\0';
	if( i > 1 ) {
		for( i=1; buf[i]; i++ ) {
			if( buf[i] != buf[i-1] + 1 ) {
				x = buf[i-1] - 'A';
				y = buf[i] - 'A';
				abc_keeptiednotes(h, ptt[x+1], ptt[y]);
			}
		}
	}
	starttick = h->track->tail->tracktick;
	ptt[partno+1] = starttick;
	for( i=0; i<=partno; i++ ) {
		partp[i][0] = abc_patno(h, ptt[i]);
		partp[i][1] = abc_patno(h, ptt[i+1]);
	}
	// calculate end point of last part
	starttick = abc_pattracktime(h, starttick);
	if( starttick % abcticks(64 * h->speed) )
		partp[partno][1]++;
	abc_set_parts(abcparts, buf);
}

// =====================================================================================
static char *abc_fgets(MMFILE *mmfile, char buf[], unsigned int bufsz)
// =====================================================================================
{
	if( mmfeof(mmfile) ) return NULL;
	mmfgets(buf,bufsz,mmfile);
	return buf;
}

// =====================================================================================
static char *abc_fgetbytes(MMFILE *mmfile, char buf[], unsigned int bufsz)
// =====================================================================================
{
	unsigned int i;
	long pos;
	if( mmfeof(mmfile) ) return NULL;
	for( i=0; i<bufsz-2; i++ ) {
		buf[i] = (char)mmfgetc(mmfile);
		if( buf[i] == '\n' ) break;
		if( buf[i] == '\r' ) {
			pos = mmftell(mmfile);
			if( mmfgetc(mmfile) != '\n' )	mmfseek(mmfile, pos, SEEK_SET);
			buf[i] = '\n';
			break;
		}
	}
	if( buf[i] == '\n' ) i++;
	buf[i] = '\0';
	return buf;
}

static void abc_substitute(ABCHANDLE *h, char *target, char *s)
{
	char *p, *q;
	int i;
	int l = strlen(target);
	int n = strlen(s);
	while( (p=strstr(h->line, target)) ) {
		if( (i=strlen(h->line)) + n - l >= (int)h->len ) {
			h->line = (char *)_mm_recalloc(h->allochandle, h->line, h->len<<1, sizeof(char));
			h->len <<= 1;
			p=strstr(h->line, target);
		}
		if( n > l ) {
			for( q=&h->line[i]; q>p; q-- ) q[n-l] = q[0];
			for( q=s; *q; q++ ) *p++ = *q;
		}
		else {
			strcpy(p,s);
			strcat(p,p+l);
		}
	}
}

static void abc_preprocess(ABCHANDLE *h, ABCMACRO *m)
{
	int i, j, k, l, a, b;
	char t[32];
	char s[200],*p;
	if( m->n ) {
		k = m->n - m->name;
		for( i=0; i<14; i++ ) {
			strncpy(t, m->name, 32);
			t[k] = "CDEFGABcdefgab"[i];
			l = strlen(m->subst);
			p = s;
			for( j=0; j<l; j++ ) {
				a = m->subst[j];
				if( a > 'g' && islower(a) ) {
					b = a - 'n';
					a = "CDEFGABCDEFGABcdefgabcdefgab"[i+b+7];
					*p++ = a;
					if( i+b < 0 )
						*p++ = ',';
					if( i+b > 13 )
						*p++ = '\'';
				}
				else *p++ = a;
			}
			*p = '\0';
			abc_substitute(h, t, s);
		}
	}
	else
		abc_substitute(h, m->name, m->subst);
}

static char *abc_gets(ABCHANDLE *h, MMFILE *mmfile)
{
	int i;
	ABCMACRO *mp;
	if( !h->len ) {
		h->len = 64;	// initial line size, adequate for most abc's
		h->line = (char *)_mm_calloc(h->allochandle, h->len, sizeof(char));
	}
	if( abc_fgetbytes(mmfile, h->line, h->len) ) {
		while( (i=strlen(h->line)) > (int)(h->len - 3) ) {
			// line too short, double it
			h->line = (char *)_mm_recalloc(h->allochandle, h->line, h->len<<1, sizeof(char));
			if( h->line[i-1] != '\n' )
				abc_fgetbytes(mmfile, &h->line[i], h->len);
			h->len <<= 1;
		}
		h->line[i-1] = '\0'; // strip off newline
		for( mp=h->macro; mp; mp=mp->next )
			abc_preprocess(h,mp);
		return h->line;
	}
	return NULL;
}

static int abc_parse_decorations(ABCHANDLE *h, ABCTRACK *tp, const char *p)
{
	int vol=0;
	if( !strncmp(p,"mp",2) ) vol = 75;
	if( !strncmp(p,"mf",2) ) vol = 90;
	if( !strncmp(p,"sfz",3) ) vol = 100;
	if( *p == 'p' ) {
		vol = 60;
		while( *p++ == 'p' ) vol -= 15;
		if( vol < 1 ) vol = 1;
	}	
	if( *p == 'f' ) {
		vol = 105;
		while( *p++ == 'f' ) vol += 15;
		if( vol > 135 ) vol = 127;	// ffff
		if( vol > 127 ) vol = 125;	// fff
	}
	if( vol ) {
		tp->volume = vol;
		if( tp == h->track ) {	// copy volume over to all voice tracks
			for( ; tp; tp=tp->next ) {
				if( tp->vpos == 0 || tp->vpos > DRONEPOS2 ) tp->volume = vol;
			}
			tp = h->track;
		}
	}
	return tp->volume;
}

// =====================================================================================
#ifdef NEWMIKMOD
BOOL ABC_Test(MMSTREAM *mmfile)
#else
BOOL CSoundFile::TestABC(const BYTE *lpStream, DWORD dwMemLength)
#endif
// =====================================================================================
{
    char id[128];
    bool hasK = false, hasX = false;
    // scan file for first K: line (last in header)
#ifdef NEWMIKMOD
    _mm_fseek(mmfile,0,SEEK_SET);
		while(abc_fgets(mmfile,id,128)) {
#else
		MMFILE mmfile;
		mmfile.mm = (char *)lpStream;
		mmfile.sz = dwMemLength;
    mmfseek(&mmfile,0,SEEK_SET);
		while(abc_fgets(&mmfile,id,128)) {
#endif
		if (id[0] == 0) continue; // blank line.
		if (id[0] == '%' && id[1] != '%') continue; // comment line.

		if (!abc_isvalidchar(id[0])  || !abc_isvalidchar(id[1])) {
			return(0); // probably not an ABC.
		}
	    	if(id[0]=='K' 
			&& id[1]==':' 
			&& (isalpha(id[2]) || isspace(id[2])) ) hasK = true;
		if (id[0]=='X'
			&& id[1]== ':'
			&& (abc_isvalidchar(id[2])) ) hasX = true;
		if (hasK && hasX) { return 1; printf("valid\n"); }
		}
    return 0;
}

// =====================================================================================
static ABCHANDLE *ABC_Init(void)
{
    ABCHANDLE   *retval;
		char *p;
		char buf[10];
#ifdef NEWMIKMOD
    MM_ALLOC     *allochandle;
    
		allochandle = _mmalloc_create("Load_ABC", NULL);
    retval = (ABCHANDLE *)_mm_calloc(allochandle, 1,sizeof(ABCHANDLE));
		if( !retval ) return NULL;
    retval->allochandle = allochandle;
    allochandle = _mmalloc_create("Load_ABC_macros", NULL);
    retval->macrohandle = allochandle;
    allochandle = _mmalloc_create("Load_ABC_tracks", NULL);
    retval->trackhandle = allochandle;
#else
    retval = (ABCHANDLE *)calloc(1,sizeof(ABCHANDLE));
		if( !retval ) return NULL;
#endif
		retval->track       = NULL;
		retval->macro       = NULL;
		retval->umacro      = NULL;
		retval->beatstring  = NULL;
		retval->pickrandom  = 0;
		retval->len         = 0;
		retval->line        = NULL;
		strcpy(retval->gchord, "");
		retval->barticks    = 0;
		p = getenv(ABC_ENV_NORANDOMPICK);
		if( p ) {
			if( isdigit(*p) )
				retval->pickrandom = atoi(p);
			if( *p == '-' ) {
#ifdef NEWMIKMOD
				retval->pickrandom = atoi(p+1);
				sprintf(buf,"-%ld",retval->pickrandom+1);
#else
				retval->pickrandom = atoi(p+1)-1; // xmms preloads the file
				sprintf(buf,"-%ld",retval->pickrandom+2);
#endif
				setenv(ABC_ENV_NORANDOMPICK, buf, 1);
			}
		}
		else {
			srandom((unsigned int)time(0));	// initialize random generator with seed
			retval->pickrandom = 1+(int)(10000.0*random()/(RAND_MAX+1.0));
			// can handle pickin' from songbooks with 10.000 songs
#ifdef NEWMIKMOD
			sprintf(buf,"-%ld",retval->pickrandom+1); // next in sequence
#else
			sprintf(buf,"-%ld",retval->pickrandom); // xmms preloads the file
#endif
			setenv(ABC_ENV_NORANDOMPICK, buf, 1);
		}
    return retval;
}

#ifndef NEWMIKMOD
static void ABC_CleanupTrack(ABCTRACK *tp)
{
	ABCEVENT *ep, *en;
	if( tp ) {
		for( ep=tp->head; ep; ep = en ) {
			en=ep->next;
			free(ep);
		}
		tp->head = NULL;
	}
}

static void ABC_CleanupMacro(ABCMACRO *m)
{
	if( m->name )
		free(m->name);
	if( m->subst )
		free(m->subst);
	free(m);
}
#endif

// =====================================================================================
static void ABC_CleanupTracks(ABCHANDLE *handle)
// =====================================================================================
{
#ifdef NEWMIKMOD
	if(handle && handle->trackhandle) {
		_mmalloc_close(handle->trackhandle);
		handle->trackhandle = 0;
	}
#else
	ABCTRACK *tp, *tn;
	if(handle) {
		for( tp=handle->track; tp; tp = tn ) {
			tn=tp->next;
			ABC_CleanupTrack(tp);
		}
		handle->track = NULL;
	}
#endif
}

// =====================================================================================
static void ABC_CleanupMacros(ABCHANDLE *handle)
// =====================================================================================
{
#ifdef NEWMIKMOD
	if(handle && handle->macrohandle) {
		_mmalloc_close(handle->macrohandle);
		handle->macrohandle = 0;
	}
#else
	ABCMACRO *mp, *mn;
	if(handle) {
		for( mp=handle->macro; mp; mp = mn ) {
			mn=mp->next;
			ABC_CleanupMacro(mp);
		}
		for( mp=handle->umacro; mp; mp = mn ) {
			mn=mp->next;
			ABC_CleanupMacro(mp);
		}
		handle->macro = NULL;
		handle->umacro = NULL;
	}
#endif
}

// =====================================================================================
static void ABC_Cleanup(ABCHANDLE *handle)
// =====================================================================================
{
#ifdef NEWMIKMOD
	if(handle && handle->allochandle) {
#else
	if(handle) {
#endif
		ABC_CleanupMacros(handle);
		ABC_CleanupTracks(handle);
#ifdef NEWMIKMOD
		_mmalloc_close(handle->allochandle);
		handle->allochandle = 0;
		handle->len = 0;
#else
		if( handle->line )
			free(handle->line);
		if( handle->beatstring )
			free(handle->beatstring);
		free(handle);
#endif
	}
}

static int abc_is_global_event(ABCEVENT *e)
{
	return e->flg == 1 && (e->cmd == cmdtempo || e->cmd == cmdpartbrk); 
}

static ABCEVENT *abc_next_global(ABCEVENT *e)
{
	for( ; e && !abc_is_global_event(e); e=e->next ) ;
	return e;
}

static ABCEVENT *abc_next_note(ABCEVENT *e)
{
	for( ; e && e->flg == 1; e=e->next ) ;
	return e;
}

// =============================================================================
#ifdef NEWMIKMOD
static void ABC_ReadPatterns(UNIMOD *of, ABCHANDLE *h, int numpat)
// =============================================================================
{
	int pat,row,i,ch,trillbits;
	BYTE n,ins,vol;
	ABCTRACK *t;
	ABCEVENT *e, *en, *ef, *el;
	uint32_t tt1, tt2;
	UNITRK_EFFECT eff;

	// initialize start points of event list in tracks
	for( t = h->track; t; t = t->next ) t->capostart = t->head;
	trillbits = 0; // trill effect admininstration: one bit per channel, max 32 channnels
	for( pat = 0; pat < numpat; pat++ ) {
		utrk_reset(of->ut);
		for( row = 0; row < 64; row++ ) {
			tt1 = abcticks((pat * 64 + row ) * h->speed);
			tt2 = tt1 + abcticks(h->speed);
			ch = 0;
			for( e=abc_next_global(h->track->capostart); e && e->tracktick < tt2; e=abc_next_global(e->next) ) {
				if( e && e->tracktick >= tt1 ) {	// we have a tempo event in this row
					switch( e->cmd ) {
						case cmdtempo:
							eff.effect   = UNI_GLOB_TEMPO;
							eff.param.u  = e->lpar;
							eff.framedly = UFD_RUNONCE;
							utrk_write_global(of->ut, &eff, PTMEM_TEMPO);
							break;
						case cmdpartbrk:
							eff.effect   = UNI_GLOB_PATBREAK;
							eff.param.u  = 0;
							eff.framedly = UFD_RUNONCE;
							utrk_write_global(of->ut, &eff, UNIMEM_NONE);
							break;
					}
				}
			}
			for( t = h->track; t; t = t->next ) {
				for( e=abc_next_note(t->capostart); e && e->tracktick < tt1; e=abc_next_note(e->next) )
					t->capostart = e;
				i = 0;
				ef = NULL;
				en = e;
				el = e;
				for( ; e && e->tracktick < tt2; e=abc_next_note(e->next) ) {	// we have a note event in this row
					t->capostart = e;
					i++;
					if( e->par[volume] ) {
						if( !ef ) ef = e;
						el = e;
					}
				}
				if( i ) {
					trillbits &= ~(1<<ch);
					utrk_settrack(of->ut, ch);
					if( i == 1 || ef == el || !ef ) { // only one event in this row
						if( ef ) e = ef;
						else e = en;
						el = t->capostart;
						i  = e->par[note] + ((e->par[octave])*12);
						if( t->chan == 10 ) {
							n   = pat_gm_drumnote(i) + 23;
							ins = pat_gmtosmp(pat_gm_drumnr(i));
						}
						else {
							n   = pat_modnote(i);
							ins = e->par[smpno];
						}
						eff.framedly = modticks(e->tracktick - tt1);
						vol = e->par[volume];
						if( e->par[effect] == accent ) {
							vol += vol / 10;
							if( vol > 127 ) vol = 127;
						}
						if (vol <= 0) {}
						else if( el->par[volume] == 0 ) {
							eff.framedly     = modticks(el->tracktick - tt1);
							eff.param.u      = 0;
							eff.param.byte_a = n;
							eff.param.byte_b = ins;
							eff.effect       = UNI_NOTEKILL;
							utrk_write_local(of->ut, &eff, UNIMEM_NONE);
						}
						else {
							switch( e->par[effect] ) {
								case trill:
									eff.effect  = UNI_VIBRATO_DEPTH;
									eff.param.u = 12;	// depth 1.5
									utrk_write_local(of->ut, &eff, PTMEM_VIBRATO_DEPTH);
									eff.effect  = UNI_VIBRATO_SPEED;
									eff.param.u = 48; // speed 12
									utrk_write_local(of->ut, &eff, PTMEM_VIBRATO_SPEED);
									trillbits |= (1<<ch);
									break;
								case bow:
									eff.effect   = UNI_PITCHSLIDE;
									eff.framedly = (h->speed/2)|UFD_RUNONCE;
									eff.param.s  = 2;
									utrk_write_local(of->ut, &eff, (e->par[effoper])? PTMEM_PITCHSLIDEUP: PTMEM_PITCHSLIDEDN);
									break;
								default:
									break;
							}
							if( eff.framedly ) {
								eff.param.u      = 0;
								eff.param.byte_a = n;
								eff.param.byte_b = ins;
								eff.effect       = UNI_NOTEDELAY;
								utrk_write_local(of->ut, &eff, UNIMEM_NONE);
							}
						}
						utrk_write_inst(of->ut, ins);
						utrk_write_note(of->ut, n); // <- normal note
						pt_write_effect(of->ut, 0xc, vol);
					}
					else { 
						// two notes in one row, use FINEPITCHSLIDE runonce effect
						// start first note on first tick and framedly runonce on seconds note tick
						// use volume and instrument of last note
						if( t->chan == 10 ) {
							i   = el->par[note] + ((el->par[octave])*12);
							n   = pat_gm_drumnote(i) + 23;
							ins = pat_gmtosmp(pat_gm_drumnr(i));
							i   = n; // cannot change instrument here..
						}
						else {
							i   = ef->par[note] + ((ef->par[octave])*12);
							n   = pat_modnote(i);
							ins = el->par[smpno];
							i   = pat_modnote(el->par[note] + ((el->par[octave])*12));
						}
						vol = el->par[volume];
						eff.effect    = UNI_PITCHSLIDE;
						eff.framedly  = modticks(el->tracktick - tt1)|UFD_RUNONCE;
						eff.param.s   = ((i > n)?i-n:n-i);
						utrk_write_inst(of->ut, ins);
						utrk_write_note(of->ut, n); // <- normal note
						pt_write_effect(of->ut, 0xc, vol);
						utrk_write_local(of->ut, &eff, (i > n)? PTMEM_PITCHSLIDEUP: PTMEM_PITCHSLIDEDN);
					}
				}
				else { // no new notes, keep on trilling...
					if( trillbits & (1<<ch) ) {
						utrk_settrack(of->ut, ch);
						eff.effect  = UNI_VIBRATO_DEPTH;
						eff.param.u = 12;	// depth 1.5
						utrk_write_local(of->ut, &eff, PTMEM_VIBRATO_DEPTH);
						eff.effect  = UNI_VIBRATO_SPEED;
						eff.param.u = 60; // speed 15
						utrk_write_local(of->ut, &eff, PTMEM_VIBRATO_SPEED);
					}
				}
				ch++;
			}
			utrk_newline(of->ut);
		}
		if(!utrk_dup_pattern(of->ut,of)) return;
	}
}

#else

static int ABC_ReadPatterns(MODCOMMAND *pattern[], WORD psize[], ABCHANDLE *h, int numpat, int channels)
// =====================================================================================
{
	int pat,row,i,ch,trillbits;
	BYTE n,ins,vol;
	ABCTRACK *t;
	ABCEVENT *e, *en, *ef, *el;
	uint32_t tt1, tt2;
	MODCOMMAND *m;
	int patbrk, tempo;
	if( numpat > MAX_PATTERNS ) numpat = MAX_PATTERNS;

	// initialize start points of event list in tracks
	for( t = h->track; t; t = t->next ) t->capostart = t->head;
	trillbits = 0; // trill effect admininstration: one bit per channel, max 32 channnels
	for( pat = 0; pat < numpat; pat++ ) {
		pattern[pat] = CSoundFile::AllocatePattern(64, channels);
		if( !pattern[pat] ) return 0;
		psize[pat] = 64;
		for( row = 0; row < 64; row++ ) {
			tt1 = abcticks((pat * 64 + row ) * h->speed);
			tt2 = tt1 + abcticks(h->speed);
			ch = 0;
			tempo = 0;
			patbrk = 0;
			for( e=abc_next_global(h->track->capostart); e && e->tracktick < tt2; e=abc_next_global(e->next) ) {
				if( e && e->tracktick >= tt1 ) {	// we have a tempo event in this row
					switch( e->cmd ) {
						case cmdtempo:
							tempo = e->lpar;
							break;
						case cmdpartbrk:
							patbrk = 1;
							break;
					}
				}
			}
			for( t = h->track; t; t = t->next ) {
				for( e=abc_next_note(t->capostart); e && e->tracktick < tt1; e=abc_next_note(e->next) ) ;
				i = 0;
				ef = NULL;
				en = e;
				el = e;
				for( ; e && e->tracktick < tt2; e=abc_next_note(e->next) ) {	// we have a note event in this row
					t->capostart = e;
					i++;
					if( e->par[volume] ) {
						if( !ef ) ef = e;
						el = e;
					}
				}
				m = &pattern[pat][row * channels + ch];
				m->param   = 0;
				m->command = CMD_NONE;
				if( i ) {
					trillbits &= ~(1<<ch);
					if( i == 1 || ef == el || !ef ) { // only one event in this row
						if( ef ) e = ef;
						else e = en;
						el = t->capostart;
						i  = e->par[note] + ((e->par[octave])*12);
						if( t->chan == 10 ) {
							n   = pat_gm_drumnote(i) + 23;
							ins = pat_gmtosmp(pat_gm_drumnr(i));
						}
						else {
							n   = pat_modnote(i);
							ins = e->par[smpno];
						}
							vol = e->par[volume]/2;
						if( e->par[volume] > 0 ) {
							if( e->par[effect] == accent ) vol += vol / 20;
							if( vol > 64 ) vol = 64;
							if( el->par[volume] == 0 ) { // note cut
								m->param   = el->tracktick - tt1;
								m->command = CMD_S3MCMDEX;
								m->param  |= 0xC0;
							}
							else {
								switch( e->par[effect] ) {
									case trill:
										m->command = CMD_VIBRATO;
										m->param   = 0xC2;	// speed 12 depth 2
										trillbits |= (1<<ch);
										break;
									case bow:
										m->command = CMD_XFINEPORTAUPDOWN;
										m->param  |= (e->par[effoper])? 0x12: 0x22;
										break;
									default:
										m->param = modticks(e->tracktick - tt1);
										if( m->param ) { // note delay
											m->command = CMD_S3MCMDEX;
											m->param  |= 0xD0;
										}
										break;
								}
							}
						}
						m->instr  = ins;
						m->note   = n; // <- normal note
						m->volcmd = VOLCMD_VOLUME;
						m->vol    = vol;
					}
					else { 
						// two notes in one row, use FINEPITCHSLIDE runonce effect
						// start first note on first tick and framedly runonce on seconds note tick
						// use volume and instrument of last note
						if( t->chan == 10 ) {
							i   = el->par[note] + ((el->par[octave])*12);
							n   = pat_gm_drumnote(i) + 23;
							ins = pat_gmtosmp(pat_gm_drumnr(i));
							i   = n; // cannot change instrument here..
						}
						else {
							i   = ef->par[note] + ((ef->par[octave])*12);
							n   = pat_modnote(i);
							ins = el->par[smpno];
							i   = pat_modnote(el->par[note] + ((el->par[octave])*12));
						}
						vol = el->par[volume]/2;
						if( vol > 64 ) vol = 64;
						m->instr  = ins;
						m->note   = n; // <- normal note
						m->volcmd = VOLCMD_VOLUME;
						m->vol    = vol;
						m->param  = ((i > n)?i-n:n-i);
						if( m->param < 16 ) {
							if( m->param ) {
								m->command = CMD_XFINEPORTAUPDOWN;
								m->param  |= (i > n)? 0x10: 0x20;
							}
							else {	// retrigger same note...
								m->command = CMD_RETRIG;
								m->param   = modticks(el->tracktick - tt1);
							}
						}
						else
							m->command = (i > n)? CMD_PORTAMENTOUP: CMD_PORTAMENTODOWN;
					}
				}
				else { // no new notes, keep on trilling...
					if( trillbits & (1<<ch) ) {
						m = &pattern[pat][row * channels + ch];
						m->command = CMD_VIBRATO;
						m->param   = 0;	// inherited from first effect
						m->instr   = 0;
						m->note    = 0;
						m->volcmd  = 0;
						m->vol     = 0;
					}
				}
				if( m->param == 0 && m->command == CMD_NONE ) {
					if( tempo ) {
						m->command = CMD_TEMPO;
						m->param   = tempo;
						tempo      = 0;
					}
					else {
						if( patbrk ) {
							m->command = CMD_PATTERNBREAK;
							patbrk     = 0;
						}
					}
				}
				ch++;
			}
			if( tempo || patbrk ) return 1;
		}
	}
	return 0;
}

#endif

static int ABC_Key(const char *p)
{
	int i,j;
	char c[8];
	const char *q;
	while( isspace(*p) ) p++;
	i = 0;
	q = p;
	for( i=0; i<8 && *p && *p != ']'; p++ ) {
		if( isspace(*p) ) {
			while( isspace(*p) ) p++;
			if( strncasecmp(p, "min", 3) && strncasecmp(p, "maj", 3) )
				break;
		}
		c[i] = *p;
		i++;
	}
	c[i] = '\0';
	if( !strcmp(c,"Hp") || !strcmp(c,"HP") ) // highland pipes
		strcpy(c,"Bm");	// two sharps at c and f
	if( !strcasecmp(c+1, "minor") ) i=2;
	if( !strcasecmp(c+2, "minor") ) i=3;
	if( !strcasecmp(c+1, "major") ) i=1;
	if( !strcasecmp(c+2, "major") ) i=2;
	if( !strcasecmp(c+1, "min") ) i=2;
	if( !strcasecmp(c+2, "min") ) i=3;
	if( !strcasecmp(c+1, "maj") ) i=1;
	if( !strcasecmp(c+2, "maj") ) i=2;
	for( ; i<6; i++ )
		c[i] = ' ';
	c[i] = '\0';
	for( i=0; keySigs[i]; i++ ) {
		for( j=10; j<46; j+=6 )
			if( !strncasecmp(keySigs[i]+j, c, 6) )
				return i;
	}
	abc_message("Failure: Unrecognised K: field %s", q);
	return 7;
}

static char *abc_skip_word(char *p)
{
	while( isspace(*p) ) p++;
	while( *p && !isspace(*p) && *p != ']') p++;
	while( isspace(*p) ) p++;
	return p;
}

static uint32_t abc_tracktime(ABCTRACK *tp)
{
	uint32_t tracktime;
	if( tp->tail ) tracktime = tp->tail->tracktick;
	else tracktime = 0;
	if( tracktime < global_songstart ) 
		tracktime = global_songstart;
	return tracktime;
}

static void abc_addchordname(const char *s, int len, const int *notes)
// adds chord name and note set to list of known chords
{
	int i, j;
	if(strlen(s) > 7) {
		abc_message("Failure: Chord name cannot exceed 7 characters, %s", s);
		return;
	}
	if(len > 6) {
		abc_message("Failure: Named chord cannot have more than 6 notes, %s", s);
		return;
	}
	for( i=0; i < chordsnamed; i++ ) {
		if(strcmp(s, chordname[i]) == 0) {
			/* change chord */
			chordlen[i] = len;
			for(j = 0; j < len; j++) chordnotes[i][j] = notes[j];
			return;
		}
	}
	if(chordsnamed > MAXCHORDNAMES - 1)
		abc_message("Failure: Too many Guitar Chord Names used, %s", s);
	else {
		strcpy(chordname[chordsnamed], s);
		chordlen[chordsnamed] = len;
		for(j = 0; j < len; j++) chordnotes[chordsnamed][j] = notes[j];
		chordsnamed++;
	}
}

static void abc_setup_chordnames()
// set up named guitar chords
{
	static const int list_Maj[3] = { 0, 4, 7 };
	static const int list_m[3] = { 0, 3, 7 };
	static const int list_7[4] = { 0, 4, 7, 10 };
	static const int list_m7[4] = { 0, 3, 7, 10 };
	static const int list_maj7[4] = { 0, 4, 7, 11 };
	static const int list_M7[4] = { 0, 4, 7, 11 };
	static const int list_6[4] = { 0, 4, 7, 9 };
	static const int list_m6[4] = { 0, 3, 7, 9 };
	static const int list_aug[3] = { 0, 4, 8 };
	static const int list_plus[3] = { 0, 4, 8 };
	static const int list_aug7[4] = { 0, 4, 8, 10 };
	static const int list_dim[3] = { 0, 3, 6 };
	static const int list_dim7[4] = { 0, 3, 6, 9 };
	static const int list_9[5] = { 0, 4, 7, 10, 2 };
	static const int list_m9[5] = { 0, 3, 7, 10, 2 };
	static const int list_maj9[5] = { 0, 4, 7, 11, 2 };
	static const int list_M9[5] = { 0, 4, 7, 11, 2 };
	static const int list_11[6] = { 0, 4, 7, 10, 2, 5 };
	static const int list_dim9[5] = { 0, 4, 7, 10, 13 };
	static const int list_sus[3] = { 0, 5, 7 };
	static const int list_sus9[3] = { 0, 2, 7 };
	static const int list_7sus[4] = { 0, 5, 7, 10 };
	static const int list_7sus4[4] = { 0, 5, 7, 10 };
	static const int list_7sus9[4] = { 0, 2, 7, 10 };
	static const int list_9sus4[5] = { 0, 5, 10, 14, 19 };
	static const int list_5[2] = { 0, 7 };
	static const int list_13[6] = { 0, 4, 7, 10, 16, 21 };
	
	chordsnamed = 0;
	abc_addchordname("", 3, list_Maj);
	abc_addchordname("m", 3, list_m);
	abc_addchordname("7", 4, list_7);
	abc_addchordname("m7", 4, list_m7);
	abc_addchordname("maj7", 4, list_maj7);
	abc_addchordname("M7", 4, list_M7);
	abc_addchordname("6", 4, list_6);
	abc_addchordname("m6", 4, list_m6);
	abc_addchordname("aug", 3, list_aug);
	abc_addchordname("+", 3, list_plus);
	abc_addchordname("aug7", 4, list_aug7);
	abc_addchordname("7+", 4, list_aug7);
	abc_addchordname("dim", 3, list_dim);
	abc_addchordname("dim7", 4, list_dim7);
	abc_addchordname("9", 5, list_9);
	abc_addchordname("m9", 5, list_m9);
	abc_addchordname("maj9", 5, list_maj9);
	abc_addchordname("M9", 5, list_M9);
	abc_addchordname("11", 6, list_11);
	abc_addchordname("dim9", 5, list_dim9);
	abc_addchordname("sus", 3, list_sus);
	abc_addchordname("sus9", 3, list_sus9);
	abc_addchordname("7sus", 4, list_7sus);
	abc_addchordname("7sus4", 4, list_7sus4);
	abc_addchordname("7sus9", 4, list_7sus9);
	abc_addchordname("9sus4", 5, list_9sus4);
	abc_addchordname("5", 2, list_5);
	abc_addchordname("13", 6, list_13);
}

static int abc_MIDI_getnumber(const char *p)
{
	int n;
	while( isspace(*p) ) p++;
	abc_getnumber(p, &n);
	if( n < 0 )   n = 0;
	if( n > 127 ) n = 127;
	return n;
}

static int abc_MIDI_getprog(const char *p)
{
	int n;
	while( isspace(*p) ) p++;
	abc_getnumber(p, &n);
	if( n < 1 )   n = 1;
	if( n > 128 ) n = 128;
	return n;
}

// MIDI drone <instr0> <pitch1> <pitch2> <vel1> <vel2>
static void abc_MIDI_drone(const char *p, int *gm, int *ptch, int *vol)
{
	int i;
	while( isspace(*p) ) p++;
	p += abc_getnumber(p, &i);
	i++;	// adjust for 1..128
	if( i>0 && i < 129 )
		*gm = i;
	else
		*gm = 71;	// bassoon
	while( isspace(*p) ) p++;
	p += abc_getnumber(p, &i);
	if( i>0 && i < 127 )
		ptch[0] = i;
	else
		ptch[0] = 45;
	while( isspace(*p) ) p++;
	p += abc_getnumber(p, &i);
	if( i>0 && i < 127 )
		ptch[1] = i;
	else
		ptch[1] = 33;
	while( isspace(*p) ) p++;
	p += abc_getnumber(p, &i);
	if( i>0 && i < 127 )
		vol[0] = i;
	else
		vol[0] = 80;
	while( isspace(*p) ) p++;
	p += abc_getnumber(p, &i);
	if( i>0 && i < 127 )
		vol[1] = i;
	else
		vol[1] = 80;
}

static void abc_chan_to_tracks(ABCHANDLE *h, int tno, int ch)
{
	ABCTRACK *tp;
	if( tno>0 && tno<33 ) {
		for( tp=h->track; tp; tp=tp->next ) {
			if( tp->vno == tno && (tp->vpos < GCHORDBPOS || tp->vpos > DRONEPOS2) )
				tp->chan = ch;
		}
	}
}

// %%MIDI channel int1
// channel numbers are 1-16
static void abc_MIDI_channel(const char *p, ABCTRACK *tp, ABCHANDLE *h)
{
	int i1, i2;
	i1 = tp? tp->vno: 1;
	for( ; *p && isspace(*p); p++ ) ;
	if( isdigit(*p) ) {
		p += abc_getnumber(p, &i2);
		if( i2 >= 1 && i2 <= 16 )
			abc_chan_to_tracks(h, i1, i2); // we start at 1
	}
}

static void abc_instr_to_tracks(ABCHANDLE *h, int tno, int gm)
{
	ABCTRACK *tp;
	if( tno>0 && tno<33 && gm>0 && gm<129 ) {
		for( tp=h->track; tp; tp=tp->next ) {
			if( tp->vno == tno && (tp->vpos < GCHORDBPOS || tp->vpos > DRONEPOS2) )
				tp->instr = gm;
		}
	}
}

// %%MIDI program [int1] <int2>
// instrument numbers are 0-127
static void abc_MIDI_program(const char *p, ABCTRACK *tp, ABCHANDLE *h)
{
	int i1, i2;
	i1 = tp? tp->vno: 1;
	for( ; *p && isspace(*p); p++ ) ;
	if( isdigit(*p) ) {
		p += abc_getnumber(p, &i2);
		for( ; *p && isspace(*p); p++ ) ;
		if( isdigit(*p) ) {
			i1 = i2;
			p += abc_getnumber(p, &i2);
		}
		abc_instr_to_tracks(h, i1, i2 + 1); // we start at 1
	}
}

static void abc_mute_voice(ABCHANDLE *h, ABCTRACK *tp, int m)
{
	ABCTRACK *t;
	for( t=h->track; t; t=t->next ) {
		if( t->vno == tp->vno ) t->mute = m;
	}
}

// %%MIDI voice [<ID>] [instrument=<integer> [bank=<integer>]] [mute]
// instrument numbers are 1-128
static void abc_MIDI_voice(const char *p, ABCTRACK *tp, ABCHANDLE *h)
{
	int i1, i2;
	for( ; *p && isspace(*p); p++ ) ;
	if( strncmp(p,"instrument=",11) && strncmp(p,"mute",4) ) {
		tp = abc_locate_track(h, p, 0);
		for( ; *p && !isspace(*p); p++ ) ;
		for( ; *p && isspace(*p); p++ ) ;
	}
	i1 = tp? tp->vno: 1;
	i2 = 0;
	if( !strncmp(p,"instrument=",11) && isdigit(p[11]) ) {
		p += 11;
		p += abc_getnumber(p, &i2);
		for( ; *p && isspace(*p); p++ ) ;
		if( !strncmp(p,"bank=",5) && isdigit(p[5]) ) {
			for( ; *p && !isspace(*p); p++ ) ;
			for( ; *p && isspace(*p); p++ ) ;
		}
	}
	if( tp ) abc_mute_voice(h,tp,0);
	if( !strncmp(p,"mute",4) && (p[4]=='\0' || p[4]=='%' || isspace(p[4])) ) {
		if( tp ) abc_mute_voice(h,tp,1);
	}
	abc_instr_to_tracks(h, i1, i2); // starts already at 1 (draft 4.0)
}

// %%MIDI chordname <string> <int1> <int2> ... <int6>
static void abc_MIDI_chordname(const char *p)
{
	char name[20];
	int i, notes[6];

	for( ; *p && isspace(*p); p++ ) ;
	i = 0;
	while ((i < 19) && (*p != ' ') && (*p != '\0')) {
		name[i] = *p;
		p = p + 1;
		i = i + 1;
	}
	name[i] = '\0';
	if(*p != ' ') {
		abc_message("Failure: Bad format for chordname command, %s", p);
	}
	else {
		i = 0;
		while ((i <= 6) && isspace(*p)) {
			for( ; *p && isspace(*p); p++ ) ;
			p += abc_getnumber(p, &notes[i]);
			i = i + 1;
		}
		abc_addchordname(name, i, notes);
	}
}

// %%MIDI drum <string> <inst 1> ... <inst n> <vol 1> ... <vol n>
// instrument numbers are 0-127
static int abc_MIDI_drum(const char *p, ABCHANDLE *h)
{
	char *q;
	int i,n,m;
	while( isspace(*p) ) p++;
	if( !strncmp(p,"on",2) && (isspace(p[2]) || p[2] == '\0') ) return 2;
	if( !strncmp(p,"off",3) && (isspace(p[3]) || p[3] == '\0') ) return 1;
	n = 0;
	for( q = h->drum; *p && !isspace(*p); p++ ) {
		if( !strchr("dz0123456789",*p) ) break;
		*q++ = *p;
		if( !isdigit(*p) ) {
			if( !isdigit(p[1]) ) *q++ = '1';
			n++; // count the silences too....
		}
	}
	*q = '\0';
	q = h->drumins;
	for( i = 0; i<n; i++ ) {
		if( h->drum[i*2] == 'd' ) {
			while( isspace(*p) ) p++;
			if( !isdigit(*p) ) {
				m = 0;
				while( !isspace(*p) ) p++;
			}
			else
				p += abc_getnumber(p,&m);
			q[i] = m + 1; // we start at 1
		}
		else q[i] = 0;
	}
	q = h->drumvol;
	for( i = 0; i<n; i++ ) {
		if( h->drum[i*2] == 'd' ) {
			while( isspace(*p) ) p++;
			if( !isdigit(*p) ) {
				m = 0;
				while( !isspace(*p) ) p++;
			}
			else
				p += abc_getnumber(p,&m);
			q[i] = m;
		}
		else q[i] = 0;
	}
	return 0;
}

// %%MIDI gchord <string>
static int abc_MIDI_gchord(const char *p, ABCHANDLE *h)
{
	char *q;
	while( isspace(*p) ) p++;
	if( !strncmp(p,"on",2) && (isspace(p[2]) || p[2] == '\0') ) return 2;
	if( !strncmp(p,"off",3) && (isspace(p[3]) || p[3] == '\0') ) return 1;
	for( q = h->gchord; *p && !isspace(*p); p++ ) {
		if( !strchr("fbcz0123456789ghijGHIJ",*p) ) break;
		*q++ = *p;
		if( !isdigit(*p) && !isdigit(p[1]) ) *q++ = '1';
	}
	*q = '\0';
	return 0;
}

static void abc_metric_gchord(ABCHANDLE *h, int mlen, int mdiv)
{
	switch( 16 * mlen + mdiv ) {
		case 0x24:
		case 0x44:
		case 0x22:
			abc_MIDI_gchord("fzczfzcz", h);
			break;
		case 0x64:
		case 0x32:
			abc_MIDI_gchord("fzczczfzczcz", h);
			break;
		case 0x34:
		case 0x38:
			abc_MIDI_gchord("fzczcz", h);
			break;
		case 0x68:
			abc_MIDI_gchord("fzcfzc", h);
			break;
		case 0x98:
			abc_MIDI_gchord("fzcfzcfzc", h);
			break;
		case 0xc8:
			abc_MIDI_gchord("fzcfzcfzcfzc", h);
			break;
		default:
			if( mlen % 3 == 0 )
				abc_MIDI_gchord("fzcfzcfzcfzcfzcfzcfzcfzcfzc", h);
			else
				abc_MIDI_gchord("fzczfzczfzczfzczfzczfzczfzcz", h);
			if( mdiv == 8 )	h->gchord[mlen*2] = '\0';
			else h->gchord[mlen*4] = '\0';
			break;
	}
}

static void abc_MIDI_legato(const char *p, ABCTRACK *tp)
{
	for( ; *p && isspace(*p); p++ ) ;
	if( !strncmp(p,"off",3) ) tp->legato = 0;
	else tp->legato = 1;
}

static void abc_M_field(const char *p, int *mlen, int *mdiv)
{
	if( !strncmp(p,"none",4) ) {
		*mlen = 1;
		*mdiv = 1;
		return;
	}
	if( !strncmp(p,"C|",2) ) {
		*mlen = 2;
		*mdiv = 2;
		return;
	}
	if( *p == 'C' ) {
		*mlen = 4;
		*mdiv = 4;
		return;
	}
	p += abc_getexpr(p,mlen);
	sscanf(p," / %d", mdiv);
}

static int abc_drum_steps(const char *dch)
{
	const char *p;
	int i=0;
	for( p=dch; *p; p++ ) {
		if( isdigit(*p) ) i += *p - '0';;
	}
	return i;
}

static void abc_add_drum(ABCHANDLE *h, uint32_t tracktime, uint32_t bartime)
{
	ABCEVENT *e;
	ABCTRACK *tp;
	uint32_t etime, ctime , rtime, stime;
	int i, g, steps, gnote, gsteps, nnum;
	steps = abc_drum_steps(h->drum);
	ctime = h->barticks;
	// look up the last event in tpr drumtrack
	tp = abc_locate_track(h, h->tpr->v, DRUMPOS);
	e = tp->tail;
	etime = e? e->tracktick: bartime;
	if( etime > tracktime ) return;
	if( etime < bartime ) rtime = h->barticks - ((bartime - etime) % h->barticks);
	else rtime = (etime - bartime) % h->barticks;
	stime = ctime*steps;
	rtime *= steps;
	rtime += stime;
	gsteps = strlen(h->drum)/2;
	g = 0;
	while( rtime > stime ) {
		rtime -= ctime*(h->drum[g*2+1] - '0');
		if( ++g == gsteps ) g = 0;
	}
	stime = (tracktime - etime) * steps;
	rtime = 0;
	while( rtime < stime ) {
		gnote = h->drum[g*2];
		i = h->drum[g*2+1] - '0';
		if(gnote=='d') {
			tp->instr = pat_gm_drumnr(h->drumins[g]-1);
			nnum      = pat_gm_drumnote(h->drumins[g]);
			abc_add_drumnote(h, tp, etime + rtime/steps, nnum, h->drumvol[g]);
			abc_add_noteoff(h,tp,etime + ( rtime + ctime * i )/steps);
		}
		if( ++g == gsteps ) g = 0;
		rtime += ctime * i;
	}
}

static int abc_gchord_steps(const char *gch)
{
	const char *p;
	int i=0;
	for( p=gch; *p; p++ )
		if( isdigit(*p) ) i += *p - '0';
	return i;
}

static void abc_add_gchord(ABCHANDLE *h, uint32_t tracktime, uint32_t bartime)
{
	ABCEVENT *e, *c;
	ABCTRACK *tp;
	uint32_t etime, ctime , rtime, stime;
	int i, g, steps, gnote, gcnum, gsteps, nnum, glen;
	// look up the last chord event in tpc
	c = 0;
	for( e = h->tpc->head; e; e = e->next )
		if( e->flg == 1 && e->cmd == cmdchord )
			c = e;
	if( !c ) return;
	gcnum = c->par[chordnum];
	steps = abc_gchord_steps(h->gchord);
	ctime = h->barticks;
	etime = 0;
	for( i = GCHORDBPOS; i < DRUMPOS; i++ ) {
		tp = abc_locate_track(h, h->tpc->v, i);
		e = tp->tail;
		if( !e ) e = c;
		stime = e->tracktick;
		if( stime > etime )	etime = stime;
	}
	if( etime > tracktime ) return;
	if( etime < bartime ) rtime = h->barticks - ((bartime - etime) % h->barticks);
	else rtime = (etime - bartime) % h->barticks;
	stime = ctime * steps;
	rtime *= steps;
	rtime += stime;
	gsteps = strlen(h->gchord);
	g = 0;
	while( rtime > stime ) {
		glen = h->gchord[2*g+1] - '0';
		rtime -= ctime * glen;
		if( ++g == gsteps ) g = 0;
	}
	stime = (tracktime - etime) * steps;
	rtime = 0;
	while( rtime < stime ) {
		gnote = h->gchord[2*g];
		glen  = h->gchord[2*g+1] - '0';
		if( ++g == gsteps ) g = 0;
		nnum = 0;
		switch(gnote) {
			case 'b':
				tp = abc_locate_track(h, h->tpc->v, GCHORDFPOS);
				tp->instr = h->abcbassprog;
				nnum = c->par[chordnote]+chordnotes[gcnum][0]+24;
				abc_add_chordnote(h, tp, etime + rtime/steps, nnum, h->abcbassvol);
				abc_add_noteoff(h,tp,etime + ( rtime + ctime * glen )/steps);
			case 'c':
				for( i = 1; i < chordlen[gcnum]; i++ ) {
					tp = abc_locate_track(h, h->tpc->v, i+GCHORDFPOS);
					tp->instr = h->abcchordprog;
					nnum = c->par[chordnote]+chordnotes[gcnum][i]+24;
					abc_add_chordnote(h, tp, etime + rtime/steps, nnum, h->abcchordvol);
					abc_add_noteoff(h,tp,etime + ( rtime + ctime * glen )/steps);
				}
				rtime += ctime * glen;
				break;
			case 'f':
				tp = abc_locate_track(h, h->tpc->v, GCHORDFPOS);
				tp->instr = h->abcbassprog;
				nnum = c->par[chordbase]+12;
				abc_add_chordnote(h, tp, etime + rtime/steps, nnum, h->abcbassvol);
				rtime += ctime * glen;
				abc_add_noteoff(h,tp,etime + rtime/steps);
				break;
			case 'g':
			case 'h':
			case 'i':
			case 'j':
			case 'G':
			case 'H':
			case 'I':
			case 'J':
				i = toupper(gnote) - 'G';
				nnum = 0;
				if( i < chordlen[gcnum] ) {
					tp = abc_locate_track(h, h->tpc->v, GCHORDFPOS+i+1);
					tp->instr = h->abcchordprog;
					nnum = c->par[chordnote]+chordnotes[gcnum][i]+24;
					if( isupper(gnote) ) nnum -= 12;
					abc_add_chordnote(h, tp, etime + rtime/steps, nnum, h->abcchordvol);
				}
				rtime += ctime * glen;
				if( nnum ) abc_add_noteoff(h,tp,etime + rtime/steps);
				break;
			case 'z':
				rtime += ctime * glen;
				break;
		}
	}
}

// %%MIDI beat a b c n
//
// controls the way note velocities are selected. The first note in a bar has
// velocity a. Other "strong" notes have velocity b and all the rest have velocity
// c. a, b and c must be in the range 0-128. The parameter n determines which
// notes are "strong". If the time signature is x/y, then each note is given
// a position number k = 0, 1, 2 .. x-1 within each bar. Note that the units for
// n are not the unit note length. If k is a multiple of n, then the note is 
// "strong". The volume specifiers !ppp! to !fff! are equivalent to the 
// following :
//
// !ppp! = %%MIDI beat 30 20 10 1
// !pp!  = %%MIDI beat 45 35 20 1
// !p!   = %%MIDI beat 60 50 35 1
// !mp!  = %%MIDI beat 75 65 50 1
// !mf!  = %%MIDI beat 90 80 65 1
// !f!   = %%MIDI beat 105 95 80 1
// !ff!  = %%MIDI beat 120 110 95 1
// !fff! = %%MIDI beat 127 125 110 1
static void abc_MIDI_beat(ABCHANDLE *h, const char *p)
{
	int i,j;
	h->beat[0] = 127;
	h->beat[1] = 125;
	h->beat[2] = 110;
	h->beat[3] = 1;
	for( j=0; j<4; j++ ) {
		while( isspace(*p) ) p++;
		if( *p ) {
			p += abc_getnumber(p, &i);
			if( i < 0   ) i = 0;
			if( i > 127 ) i = 127;
			h->beat[j] = i;
		}
	}
	if( h->beat[3] == 0 ) h->beat[3] = 1; // BB Ruud says: do not let you make mad
}

// 
// %%MIDI beatstring <string of f, m and p>
// 
// This provides an alternative way of specifying where the strong and weak
// stresses fall within a bar. 'f' means velocity a (normally strong), 'm' 
// means velocity b (medium velocity) and 'p' means velocity c (soft velocity).
// For example, if the time signature is 7/8 with stresses on the first, fourth 
// and sixth notes in the bar, we could use the following 
//
// %%MIDI beatstring fppmpmp
static void abc_MIDI_beatstring(ABCHANDLE *h, const char *p)
{
	while( isspace(*p) ) p++;
	if( h->beatstring ) _mm_free(h->allochandle, h->beatstring);
	if( strlen(p) )
		h->beatstring = DupStr(h->allochandle,p,strlen(p)+1);
	else
		h->beatstring = NULL;
}

static int abc_beat_vol(ABCHANDLE *h, int abcvol, int barpos)
{
	int vol;
	if( h->beatstring ) {
		vol = (h->beat[2] * 9) / 10;
		if( barpos < (int)strlen(h->beatstring) ) {
			switch(h->beatstring[barpos]) {
				case 'f':
					vol = h->beat[0];
					break;
				case 'm':
					vol = h->beat[1];
					break;
				case 'p':
					vol = h->beat[2];
					break;
				default:
					break;
			}
		}
	}
	else {
		if( (barpos % h->beat[3]) == 0 ) {
			if( barpos )
				vol = h->beat[1];
			else
				vol = h->beat[0];
		}
		else
			vol = h->beat[2];
	}
	vol *= abcvol;
	vol /= 128;
	return vol;
}

static void abc_init_partpat(BYTE partp[27][2])
{
	int i;
	for( i=0; i<27; i++ ) {
		partp[i][0] = 0xff;
		partp[i][1] = 0;
	}
}

static int abc_partpat_to_orderlist(BYTE partp[27][2], const char *abcparts, ABCHANDLE *h, BYTE **list, int orderlen)
{
	int t, partsused;
	const char *p;
	BYTE *orderlist = *list;
	static int ordersize = 0;
	if( *list == NULL ) {
		ordersize = 128;
		orderlist = (BYTE *)_mm_calloc(h->ho, ordersize, sizeof(BYTE));
		*list = orderlist;
	}
	if( abcparts ) {
		partsused = 0;
		for( p = abcparts; *p; p++ ) {
			for( t = partp[*p - 'A'][0]; t < partp[*p - 'A'][1]; t++ ) {
				if( orderlen == ordersize ) {
					ordersize <<= 1;
					orderlist = (BYTE *)_mm_recalloc(h->ho, orderlist, ordersize, sizeof(BYTE));
					*list = orderlist;
				}
				orderlist[orderlen] = t;
				orderlen++;
				partsused++;
			}
		}
		if( partsused ) return orderlen;
	}
	// some fool wrote a P: string in the header but didn't use P: in the body
	for( t = partp[26][0]; t < partp[26][1]; t++ ) {
		if( orderlen == ordersize ) {
			ordersize <<= 1;
			orderlist = (BYTE *)_mm_recalloc(h->ho, orderlist, ordersize, sizeof(BYTE));
			*list = orderlist;
		}
		orderlist[orderlen] = t;
		orderlen++;
	}
	return orderlen;
}

static void abc_globalslide(ABCHANDLE *h, uint32_t tracktime, int slide)
{
	ABCTRACK *tp;
	ABCEVENT *e;
	int hslide;
	hslide = h->track? h->track->slidevol: slide;
	for( tp=h->track; tp; tp = tp->next ) {
		if( slide ) {
			tp->slidevoltime = tracktime;
			if( slide == 2 )
				tp->slidevol = 0;
		}
		if( tp->slidevol > -2 && slide < 2 )
			tp->slidevol = slide;
	}
	if( h->track && h->track->tail 
	&& hslide != slide && slide == -2
	&& h->track->tail->tracktick >= tracktime ) {
		// need to update jumptypes in mastertrack from tracktime on...
		for( e=h->track->head; e; e=e->next )	{
			if( e->flg == 1 && e->cmd == cmdjump && e->tracktick >= tracktime ) {
				switch( e->par[jumptype] ) {
					case jumpnormal:
					case jumpfade:
						e->par[jumptype] = jumpfade;
						break;
					case jumpdacapo:
					case jumpdcfade:
						e->par[jumptype] = jumpdcfade;
						break;
					case jumpdasegno:
					case jumpdsfade:
						e->par[jumptype] = jumpdsfade;
						break;
				}
			}
		}
	}
}

static void abc_recalculate_tracktime(ABCHANDLE *h) {
	ABCTRACK *ttp;
	h->tracktime = 0;
	for( ttp=h->track; ttp; ttp=ttp->next )
		if( ttp->tail && ttp->tail->tracktick > h->tracktime )
			h->tracktime = ttp->tail->tracktick;
}

static void abc_MIDI_command(ABCHANDLE *h, char *p, char delim) {
	int t;
	// interpret some of the possibilitys
	if( !strncmp(p,"bassprog",8)    && isspace(p[8]) ) h->abcbassprog = abc_MIDI_getprog(p+8)+1;
	if( !strncmp(p,"bassvol",7)     && isspace(p[7]) ) h->abcbassvol = abc_MIDI_getnumber(p+7);
	if( !strncmp(p,"beat",4)        && isspace(p[4]) ) abc_MIDI_beat(h, p+4);
	if( !strncmp(p,"beatstring",10) && isspace(p[10]) ) abc_MIDI_beatstring(h, p+4);
	if( !strncmp(p,"chordname",9)   && isspace(p[9]) ) abc_MIDI_chordname(p+9);
	if( !strncmp(p,"chordprog",9)   && isspace(p[9]) ) h->abcchordprog = abc_MIDI_getprog(p+9)+1;
	if( !strncmp(p,"chordvol",8)    && isspace(p[8]) ) h->abcchordvol = abc_MIDI_getnumber(p+8);
	if( !strncmp(p,"drone",5)       && isspace(p[5]) ) abc_MIDI_drone(p+5, &h->dronegm, h->dronepitch, h->dronevol);
	if( !strncmp(p,"droneoff",8)    && (p[8]=='\0' || p[8]==delim || isspace(p[8])) ) h->droneon = 0;
	if( !strncmp(p,"droneon",7)     && (p[7]=='\0' || p[7]==delim || isspace(p[7])) ) h->droneon = 1;
	t = h->drumon;
	if( !strncmp(p,"drum",4)        && isspace(p[4]) ) {
		h->drumon = abc_MIDI_drum(p+4, h);
		if( h->drumon ) --h->drumon;
		else h->drumon = t;
	}
	if( !strncmp(p,"drumoff",7)     && (p[7]=='\0' || p[7]==delim || isspace(p[7])) ) h->drumon = 0;
	if( !strncmp(p,"drumon",6)      && (p[6]=='\0' || p[6]==delim || isspace(p[6])) ) h->drumon = 1;
	if( t != h->drumon ) {
		if( h->drumon && !h->tpr ) h->tpr = h->track;
		if( h->tpr ) abc_add_drum_sync(h, h->tpr, h->tracktime); // don't start drumming from the beginning of time!
		if( h->tpr && !h->drumon ) h->tpr = NULL;
	}
	t = h->gchordon;
	if( !strncmp(p,"gchord",6)      && (p[6]=='\0' || p[6]==delim || isspace(p[6])) ) {
		h->gchordon = abc_MIDI_gchord(p+6, h);
		if( h->gchordon ) --h->gchordon;
		else h->gchordon = t;
	} 
	if( !strncmp(p,"gchordoff",9)   && (p[9]=='\0' || p[9]==delim || isspace(p[9])) ) h->gchordon = 0;
	if( !strncmp(p,"gchordon",8)    && (p[8]=='\0' || p[8]==delim || isspace(p[8])) ) h->gchordon = 1;
	if( t != h->gchordon ) {
		if( h->tpc ) abc_add_gchord_syncs(h, h->tpc, h->tracktime);
	}
	if( !strncmp(p,"channel",7)     && isspace(p[7]) )
		abc_MIDI_channel(p+8, h->tp = abc_check_track(h, h->tp), h);
	if( !strncmp(p,"program",7)     && isspace(p[7]) )
		abc_MIDI_program(p+8, h->tp = abc_check_track(h, h->tp), h);
	if( !strncmp(p,"voice",5)       && isspace(p[5]) )
		abc_MIDI_voice(p+6, h->tp = abc_check_track(h, h->tp), h);
	if( !strncmp(p,"legato",6)      && (p[6]=='\0' || p[6]==delim || isspace(p[6])) )
		abc_MIDI_legato(p+6, h->tp = abc_check_track(h, h->tp));
}

// continuate line that ends with a backslash, can't do this in abc_gets because voice lines
// can have comment lines in between that must be parsed properly, for example:
//  [V:1] cdef gabc' |\ << continuation backslash
//  %%MIDI program 25
//        c'bag fedc |
// informational lines can have this too, so it is rather convoluted code...
static char *abc_continuated(ABCHANDLE *h, MMFILE *mmf, char *p) {
	char *pm, *p1, *p2 = 0;
	int continued;
	pm = p;
	while( pm[strlen(pm)-1]=='\\' ) {
		p1 = strdup(pm);
		if( p2 ) free(p2);
		continued = 1;
		while( continued ) {
			continued = 0;
			pm = abc_gets(h, mmf);
			if( !pm ) {
				abc_message("line not properly continued\n%s", p1);
				return p1;
			}
			while( *pm && isspace(*pm) ) ++pm;
			if( !strncmp(pm,"%%",2) ) {
				for( p2 = pm+2; *p2 && isspace(*p2); p2++ ) ;
				if( !strncmp(p2,"MIDI",4) && (p2[4]=='=' || isspace(p2[4])) ) {
					for( p2+=5; *p2 && isspace(*p2); p2++ ) ;
					if( *p2 == '=' )
						for( p2+=1; *p2 && isspace(*p2); p2++ ) ;
					abc_MIDI_command(h,p2,'%');
				}
				continued = 1;
			}
		}
		p2 = (char *)malloc(strlen(p1)+strlen(pm));
		if( !p2 ) {
			abc_message("macro line too long\n%s", p1);
			return p1;
		}
		p1[strlen(p1)-1] = '\0'; // strip off the backslash
		strcpy(p2,p1);
		strcat(p2,pm);
		pm = p2;
		free(p1);
	}
	return pm;
}

// =====================================================================================
#ifdef NEWMIKMOD
BOOL ABC_Load(ABCHANDLE *h, UNIMOD *of, MMSTREAM *mmfile)
#else
BOOL CSoundFile::ReadABC(const uint8_t *lpStream, DWORD dwMemLength)
#endif
{
	static int avoid_reentry = 0;
#ifdef NEWMIKMOD
#define m_nDefaultTempo	of->inittempo
#else
	ABCHANDLE *h;
	uint32_t numpat;
	MMFILE mm, *mmfile;
#endif
	uint32_t t;
	char	*line, *p, *pp, ch, ch0=0;
	char barsig[52];	// for propagated accidental key signature within bar
	char *abcparts;
	uint8_t partpat[27][2], *orderlist;
	int orderlen;
	enum { NOWHERE, INBETWEEN, INHEAD, INBODY, INSKIPFORX, INSKIPFORQUOTE } abcstate;
	ABCEVENT_JUMPTYPE j;
	ABCEVENT_X_EFFECT abceffect;
	int abceffoper;
	int abcxcount=0, abcxwanted=0, abcxnumber=1;
	int abckey, abcrate, abcchord, abcvol, abcbeatvol, abcnoslurs, abcnolegato, abcfermata, abcarpeggio, abcto;
	int abctempo;
	int cnotelen=0, cnotediv=0, snotelen, snotediv, mnotelen, mnotediv, notelen, notediv; 
	// c for chords, s for standard L: setting, m for M: barlength
	int abchornpipe, brokenrithm, tupletp, tupletq, tupletr;
	int ktempo;
	uint32_t abcgrace=0, bartime, thistime=0;
	ABCTRACK *tpd, *ttp;
	ABCMACRO *mp;
	int mmsp;
#ifdef NEWMIKMOD
	MMSTREAM *mmstack[MAXABCINCLUDES];
	h->ho = _mmalloc_create("Load_ABC_ORDERLIST", NULL);
#else
	MMFILE *mmstack[MAXABCINCLUDES];
	if( !TestABC(lpStream, dwMemLength) ) return FALSE;
	h = ABC_Init();
	if( !h ) return FALSE;
	mmfile = &mm;
	mm.mm = (char *)lpStream;
	mm.sz = dwMemLength;
	mm.pos = 0;
#endif
	while( avoid_reentry ) sleep(1);
	avoid_reentry = 1;
	pat_resetsmp();
	pat_init_patnames();
	m_nDefaultTempo = 0;
	global_voiceno = 0;
	abckey = 0;
	h->tracktime = 0;
	global_songstart = 0;
	h->speed = 6;
	abcrate = 240;
	global_tempo_factor = 2;
	global_tempo_divider = 1;
	abctempo = 0;
	ktempo = 0;
	abceffect = none;
	abceffoper = 0;
	abcvol = 120;
	h->abcchordvol  = abcvol;
	h->abcbassvol   = abcvol;
	h->abcchordprog = 25; // acoustic guitar
	h->abcbassprog  = 33; // acoustic bass
	abcparts = 0;
	abcnoslurs = 1;
	abcnolegato = 1;
	abcfermata = 0;
	abcarpeggio = 0;
	abcto = 0;
	snotelen = 0;
	snotediv = 0;
	mnotelen = 1;
	mnotediv = 1;
	abchornpipe = 0;
	brokenrithm = 0;
	tupletp = 0;
	tupletq = 0;
	tupletr = 0;
	h->ktrans = 0;
	h->drumon = 0;
	h->gchordon = 1;
	h->droneon = 0;
	h->tracktime = 0;
	bartime = 0;
	h->tp = NULL;
	h->tpc = NULL;
	h->tpr = NULL;
	tpd = NULL;
	h->dronegm       = 71;
	h->dronepitch[0] = 45;
	h->dronepitch[1] = 33;
	h->dronevol[0]   = 80;
	h->dronevol[1]   = 80;
	abc_new_umacro(h, "v = +downbow+");
	abc_new_umacro(h, "u = +upbow+");
	abc_new_umacro(h, "O = +coda+");
	abc_new_umacro(h, "S = +segno+");
	abc_new_umacro(h, "P = +uppermordent+");
	abc_new_umacro(h, "M = +lowermordent+");
	abc_new_umacro(h, "L = +emphasis+");
	abc_new_umacro(h, "H = +fermata+");
	abc_new_umacro(h, "T = +trill+");
	abc_new_umacro(h, "~ = +roll+");
	abc_setup_chordnames();
	abc_init_partpat(partpat);
	abc_MIDI_beat(h, ""); // reset beat array
	abc_MIDI_beatstring(h, ""); // reset beatstring
	orderlist  = NULL;
	orderlen   = 0;
	mmsp       = 1;
	mmstack[0] = mmfile;
	mmfseek(mmfile,0,SEEK_SET);
	abcstate = NOWHERE;
	if( h->pickrandom ) {
		abcstate = INSKIPFORX;
		abcxcount = 0;
		mmfseek(mmfile,0,SEEK_SET);
		while( (line=abc_gets(h, mmfile)) ) {
			for( p=line; isspace(*p); p++ ) ;
			if( !strncmp(p,"X:",2) ) abcxcount++;
		}
		if( abcxcount == 0 )
			abcstate = NOWHERE;
		else
			abcxwanted = (h->pickrandom - 1) % abcxcount;
		abcxcount = 0;
		mmfseek(mmfile,0,SEEK_SET);
	}
	while( mmsp > 0 ) {
		mmsp--;
		while((line=abc_gets(h, mmstack[mmsp]))) {
			for( p=line; isspace(*p); p++ ) ;
			switch(abcstate) {
				case INSKIPFORX:
					if( !strncmp(p,"X:",2) ) {
						if( abcxcount++ != abcxwanted )
							break;
					}
					// fall through
				case INBETWEEN:
					if( !strncmp(p,"X:",2) ) {
						abcstate = INHEAD;
#ifdef NEWMIKMOD
						of->songname = NULL;
#else
						memset(m_szNames[0], 0, 32);
#endif
						for( p+=2; isspace(*p); p++ ) ;
						abcxnumber = atoi(p);
						abchornpipe = 0;
						h->droneon = 0;
						h->dronegm       = 71;
						h->dronepitch[0] = 45;
						h->dronepitch[1] = 33;
						h->dronevol[0]   = 80;
						h->dronevol[1]   = 80;
						for( ttp = h->track; ttp; ttp=ttp->next ) {
							ttp->vno = 0;	// mark track unused
							ttp->capostart = NULL;
						}
						h->tp  = NULL; // forget old voices
						h->tpc = NULL;
						h->tpr = NULL;
						global_voiceno = 0;
						abc_set_parts(&abcparts, 0);
						abcgrace = 0;
						h->ktrans = 0;
						ktempo = 0;
						h->gchordon = 1;
						h->drumon = 0;
						global_songstart = h->tracktime;
						abc_MIDI_beat(h, ""); // reset beat array
						abc_MIDI_beatstring(h, ""); // reset beatstring
						strcpy(h->gchord, ""); // reset gchord string
						abcnolegato = 1; // reset legato switch
					}
					break;
				case NOWHERE:
					if( p[0] != '\0' && p[1] == ':' ) {
						abcstate = INHEAD;
						abc_set_parts(&abcparts, 0);
						strcpy(h->gchord, "");
						if( h->drumon && h->tpr ) abc_add_drum_sync(h, h->tpr, h->tracktime);
						if( h->tpc && !h->gchordon ) abc_add_gchord_syncs(h, h->tpc, h->tracktime);
						h->gchordon = 1;
						h->drumon = 0;
					}
					else
						break;
				case INHEAD:
					if( !strncmp(p,"L:",2) ) {
						sscanf(p+2," %d / %d", &snotelen, &snotediv);
						break;
					}
					if( !strncmp(p,"M:",2) ) {
						abc_M_field(p+2, &mnotelen, &mnotediv);
						break;
					}
					if( !strncmp(p,"P:",2) ) {
						abc_set_parts(&abcparts, p+2);
						break;
					}
					if( !strncmp(p,"Q:",2) ) {
						abctempo = abc_extract_tempo(p+2,0);
						ktempo = 1;
						if( h->track ) {
							// make h->tracktime start of a new age...
							abc_add_partbreak(h, h->track, h->tracktime);
							abc_add_tempo_event(h, h->track, h->tracktime, abctempo);
						}
						if( m_nDefaultTempo == 0 ) m_nDefaultTempo = abctempo;
						break;
					}
					if( !strncmp(p,"T:",2) ) {
						char buf[200];
						if( strchr(p,'%') ) *strchr(p,'%') = '\0';
						for( t=strlen(p)-1; isspace(p[t]); t-- )
							p[t]='\0';
						for( t=2; isspace(p[t]); t++ ) ;
#ifdef NEWMIKMOD
						if( of->songname )
							strcpy(buf,of->songname);
						else
							strcpy(buf,"");
#else
						strcpy(buf,m_szNames[0]);
#endif
						if( strlen(buf) + strlen(p+t) > 199 ) p[t+199-strlen(buf)] = '\0'; // chop it of
						if( strlen(buf) ) strcat(buf," "); // add a space
						strcat(buf, p+t);
#ifdef NEWMIKMOD
						of->songname = DupStr(of->allochandle, buf, strlen(buf));
#else
						if( strlen(buf) > 31 ) buf[31] = '\0'; // chop it of
						strcpy(m_szNames[0], buf);
#endif
						break;
					}
					if( !strncmp(p,"R:",2) ) {
						for( p+=2; isspace(*p); p++ ) ;
						if( !strncmp(p,"hornpipe",8) && (isspace(p[8]) || p[8]=='\0') ) abchornpipe = 1;
						else abchornpipe = 0;
						break;
					}
					if( !strncmp(p,"V:",2) ) {
						for( t=2; p[t]==' '; t++ ) ;
						h->tp = abc_locate_track(h, p+t, 0);
						abcvol = h->tp->volume;
						abcnolegato = !h->tp->legato;
						if( !abcnolegato ) abcnoslurs = 0;
						break;
					}
					if( !strncmp(p,"K:",2) ) {
						abcstate = INBODY;
						abckey = ABC_Key(p+2);
						sprintf(barsig, "%s%s", sig[abckey], sig[abckey]);	// reset the key signature
						p = abc_skip_word(p+2);
						h->ktrans = abc_transpose(p);
						*p = '%'; // force skip rest of line
						if( snotelen == 0 ) {	// calculate default notelen from meter M:
							if( mnotediv == 0 ) mnotediv = mnotelen = 1;	// do'nt get nuked
							snotelen = 100 * mnotelen / mnotediv;
							if( snotelen > 74 ) 
								snotediv = 8;
							else
								snotediv = 16;
							snotelen = 1;
						}
						abceffect = none;
						abceffoper = 0;
						if( !(snotelen == 1 && snotediv == 8) ) abchornpipe = 0; // no matter what they said at R:
						brokenrithm = 0;
						global_part = ' ';
						abcgrace = 0;
						abcnoslurs = abcnolegato;
						abcto = 0;
						h->tpc = NULL; // reset chord track
						tpd = NULL; // reset drone track
						h->tpr = NULL; // reset drum  track
						if( !strlen(h->gchord) ) abc_metric_gchord(h, mnotelen, mnotediv);
						h->barticks = notelen_notediv_to_ticks(h->speed, mnotelen, mnotediv);
						if( abctempo && !ktempo ) {	// did not set tempo in this songpiece so reset to abcrate
							abctempo = 0;
							global_tempo_factor = 2;
							global_tempo_divider = 1;
							if( h->track ) {
								// make h->tracktime start of a new age...
								abc_add_partbreak(h, h->track, h->tracktime);
								abc_add_tempo_event(h, h->track, h->tracktime, abcrate);
							}
							if( m_nDefaultTempo == 0 ) m_nDefaultTempo = abcrate;
						}
						abc_init_partpat(partpat);
						partpat[26][0] = abc_patno(h, h->tracktime);
						partpat[26][1] = 0;
						abc_globalslide(h, h->tracktime, 2); // reset all volumeslides
						break;
					}
					if( !strlen(p) )
						abcstate = INBETWEEN;
					break;
				case INSKIPFORQUOTE:
						while( (ch=*p++) && (ch != '"') )
							;
						if( !ch ) break;
						abcstate = INBODY;
						// fall through
				case INBODY:
					if( !strlen(p) && h->track ) { // end of this song
						abcstate = h->pickrandom? INSKIPFORX: INBETWEEN;
						// last but not least shut off all pending events
						abc_recalculate_tracktime(h);
						for( ttp=h->track; ttp; ttp=ttp->next )
							abc_add_noteoff(h,ttp,h->tracktime);
						abc_add_partbreak(h, h->track, h->tracktime);
						t = abc_patno(h, h->tracktime);
						if( abc_pattracktime(h, h->tracktime) % abcticks(64 * h->speed) ) t++;
						if(	global_part == ' ' ) {
							partpat[26][1] = t;
							if( abcparts ) {
								for( t=0; t<26; t++ )
									if( partpat[t][0] < partpat[t][1] ) break;
								if( t == 26 ) {
									abc_message("parts (%s) set but not used", abcparts);
									abc_set_parts(&abcparts, 0);	// forget the parts array
								}
							}
						}
						else
							partpat[global_part - 'A'][1] = t;
						if( !abcparts ) abc_song_to_parts(h, &abcparts, partpat);
						orderlen = abc_partpat_to_orderlist(partpat, abcparts, h, &orderlist, orderlen);
					}
					if( !strncmp(p,"V:",2) ) {
						for( t=2; p[t]==' '; t++ ) ;
						h->tp = abc_locate_track(h, p+t, 0);
						sprintf(barsig, "%s%s", sig[abckey], sig[abckey]);	// reset the key signature
						abcgrace = 0;
						brokenrithm = 0;
						h->tracktime = abc_tracktime(h->tp);
						bartime = h->tracktime; // it is not friendly to break voices in the middle of a track...
						abcnolegato = !h->tp->legato;
						if( !abcnolegato ) abcnoslurs = 0;
						*p = '%';	// make me skip the rest of the line....
					}
					if( !strncmp(p,"K:",2) ) {
						abckey = ABC_Key(p+2);
						sprintf(barsig, "%s%s", sig[abckey], sig[abckey]);	// reset the key signature
						p = abc_skip_word(p+2);
						h->ktrans = abc_transpose(p);
						*p = '%';	// make me skip the rest of the line....
					}
					if( !strncmp(p,"L:",2) ) {
						sscanf(p+2," %d / %d", &snotelen, &snotediv);
						*p = '%';	// make me skip the rest of the line....
					}
					if( !strncmp(p,"M:",2) ) {
						abc_M_field(p+2, &mnotelen, &mnotediv);
						h->barticks = notelen_notediv_to_ticks(h->speed, mnotelen, mnotediv);
						*p = '%';	// make me skip the rest of the line....
					}
					if( !strncmp(p,"Q:",2) ) {
						abctempo = abc_extract_tempo(p+2,ch0=='\\');
						if( !h->track ) {
							h->tp = abc_check_track(h, h->track);
							h->tp->vno = 0; // mark reuseable (temporarely, until first notes come up)
						}
						abc_add_tempo_event(h, h->track, h->tracktime, abctempo);
						*p = '%';	// make me skip the rest of the line....
					}
					if( !strncmp(p,"T:",2) ) {
						char buf[200];
						if( strchr(p,'%') ) *strchr(p,'%') = '\0';
						for( t=strlen(p)-1; isspace(p[t]); t-- )
							p[t]='\0';
						for( t=2; isspace(p[t]); t++ ) ;
#ifdef NEWMIKMOD
						if( of->songname )
							strcpy(buf,of->songname);
						else
							strcpy(buf,"");
#else
						strcpy(buf,m_szNames[0]);
#endif
						if( strlen(buf) + strlen(p+t) > 198 ) p[t+198-strlen(buf)] = '\0'; // chop it of
						if( strlen(buf) ) strcat(buf," "); // add a space
						strcat(buf, p+t);
#ifdef NEWMIKMOD
						of->songname = DupStr(of->allochandle, buf, strlen(buf));
#else
						if( strlen(buf) > 31 ) buf[31] = '\0'; // chop it of
						strcpy(m_szNames[0], buf);
#endif
						*p = '%';	// make me skip the rest of the line....
					}
					break;
			}
			if( !strncmp(p,"m:",2) ) {
				if( abcstate != INSKIPFORX ) {
					char *pm;
					pm = abc_continuated(h, mmstack[mmsp], p);
					abc_new_macro(h, pm+2);
					if( pm != p ) {
						free(pm);
						if( h->tp ) abcnolegato = !h->tp->legato;
						if( !abcnolegato ) abcnoslurs = 0;
					}
				}
				*p = '%'; // skip rest of line
			}
			if( !strncmp(p,"U:",2) ) {
				abc_new_umacro(h, p+2);
				*p = '%'; // skip rest of line
			}
			if( !strncmp(p,"w:",2) ) {	// inline lyrics
				*p = '%'; // skip rest of line
			}
			if( !strncmp(p,"W:",2) ) {	// lyrics at end of song body
				*p = '%'; // skip rest of line
			}
			if( !strncmp(p,"d:",2) ) {	// oldstyle decorations
				abc_message("warning: old style decorations not handled\n%s", p);
				*p = '%'; // skip rest of line
			}
			if( !strncmp(p,"s:",2) ) {	// newstyle decorations (symbols)
				abc_message("warning: new style decorations not handled\n%s", p);
				*p = '%'; // skip rest of line
			}
			if( !strncmp(p,"I:",2) && abcstate != INSKIPFORX ) { // handle like oldstyle '%%command' lines
				p[0]= '%';
				p[1]= '%';
			}
			if( !strncmp(p,"%%",2) ) {
				for( p+=2; *p && isspace(*p); p++ ) ;
				if( !strncmp(p,"abc-include",11) && isspace(p[11]) ) {
					for( t=12; isspace(p[t]); t++ ) ;
					if( p[t] ) {
						mmsp++;
						if( mmsp == MAXABCINCLUDES ) {
							mmsp--;
							abc_message("failure: too many abc-include's, %s", &p[t]);
						} else {
							mmstack[mmsp] = mmfopen(&p[t], "r");
							if( !mmstack[mmsp] ) {
								mmsp--;
								abc_message("failure: abc-include file %s not found", &p[t]);
							}
						}
					}
					else abc_message("failure: abc-include missing file name, %s", p);
				}
				if( !strncmp(p,"MIDI",4) && (p[4]=='=' || isspace(p[4])) && abcstate != INSKIPFORX ) {
					for( p+=5; *p && isspace(*p); p++ ) ;
					if( *p == '=' )
						for( p+=1; *p && isspace(*p); p++ ) ;
					abc_MIDI_command(h,p,'%');
					if( h->tp ) abcnolegato = !h->tp->legato;
					if( !abcnolegato ) abcnoslurs = 0;
				}
				if(*p) *p = '%'; // skip rest of line
			}
			if( abcstate == INBODY ) {
				if( *p == 'P' && p[1] == ':' ) {	// a line with a part indication
					if( abcparts != NULL ) {
						// make h->tracktime start of a new age...
						if( !h->track ) {
							h->tp = abc_check_track(h, h->track);
							h->tp->vno = 0; // mark reuseable (temporarely, until first notes come up)
						}
						h->tracktime = h->track? abc_tracktime(h->track): 0; // global parts are voice independent
						abc_add_partbreak(h, h->track, h->tracktime);
						t = abc_patno(h, h->tracktime);
						if(	global_part == ' ' ) {
							partpat[26][1] = t;
							if( abcparts ) {
								for( t=0; t<26; t++ )
									if( partpat[t][0] < partpat[t][1] ) break;
								if( t == 26 ) {
									abc_message("parts (%s) set but not used", abcparts);
									abc_set_parts(&abcparts, 0);	// forget the parts array
								}
							}
						}
						else
							partpat[global_part - 'A'][1] = t;
						// give every new coming abcevent the desired part indication
						while( p[2]==' ' || p[2]=='.' ) p++;	// skip blancs and dots
						if( isupper(p[2]) )
							global_part = p[2];
						else 
							global_part = ' ';
						if(	global_part == ' ' )
							partpat[26][0] = t;
						else
							partpat[global_part - 'A'][0] = t;
					}
					*p = '%';	// make me skip the rest of the line....
				}
				if( h->droneon && !tpd ) {
					tpd = h->track;
					if( tpd ) {
						tpd = abc_locate_track(h, tpd->v, DRONEPOS1);
						tpd->instr = h->dronegm;
						abc_add_dronenote(h, tpd, h->tracktime, h->dronepitch[0], h->dronevol[0]);
						tpd = abc_locate_track(h, tpd->v, DRONEPOS2);
						tpd->instr = h->dronegm;
						abc_add_dronenote(h, tpd, h->tracktime, h->dronepitch[1], h->dronevol[1]);
					}
				}
				if( tpd && !h->droneon ) {
					tpd = abc_locate_track(h, tpd->v, DRONEPOS1);
					abc_add_noteoff(h, tpd, h->tracktime);
					tpd = abc_locate_track(h, tpd->v, DRONEPOS2);
					abc_add_noteoff(h, tpd, h->tracktime);
					tpd = NULL;
				}
				if( h->drumon && !h->tpr ) {
					h->tpr = h->track;
					if( h->tpr ) abc_add_drum_sync(h, h->tpr, h->tracktime); // don't start drumming from the beginning of time!
				}
				if( h->tpr && !h->drumon ) h->tpr = NULL;
				if( *p != '%' ) {	// skip uninteresting lines
					// plough thru the songline gathering mos....
					ch0 = ' ';
					pp = 0;
					while( (ch = *p++) ) {
						if( isalpha(ch) && *p != ':' ) { // maybe a macro
							for( mp=h->umacro; mp; mp=mp->next ) {
								if( ch == mp->name[0] ) {
									pp = p;
									p = mp->subst;
									ch = *p++;
									break;
								}
							}
						}
						switch(ch) {
							case '%':
								abcto = 0;
								while( *p ) p++;
								break;
							case '[':	// chord follows or some inline field
								abcto = 0;
								if( *p=='|' ) break; // [| a thick-thin bar line, loop around and let case '|' handle it
								if( !strncmp(p,"V:",2) ) {	// inline voice change
									for( t=2; isspace(p[t]); t++ ) ;
									h->tp = abc_locate_track(h, p+t, 0);
									for( ; *p && *p != ']'; p++ ) ;
									abcgrace = 0;
									brokenrithm = 0;
									sprintf(barsig, "%s%s", sig[abckey], sig[abckey]);	// reset the key signature
									h->tracktime = abc_tracktime(h->tp);
									bartime = h->tracktime; // it is not wise to break voices in the middle of a track...
									abcvol = h->tp->volume;
									abcnolegato = !h->tp->legato;
									if( !abcnolegato ) abcnoslurs = 0;
									break;
								}
								if( !strncmp(p,"K:",2) ) {
									abckey = ABC_Key(p+2);
									sprintf(barsig, "%s%s", sig[abckey], sig[abckey]);	// reset the key signature
									p = abc_skip_word(p+2);
									h->ktrans = abc_transpose(p);
									for( ; *p && *p != ']'; p++ ) ;
									break;
								}
								if( !strncmp(p,"M:",2) ) {
									abc_M_field(p+2, &mnotelen, &mnotediv);
									for( ; *p && *p != ']'; p++ ) ;
									h->barticks = notelen_notediv_to_ticks(h->speed, mnotelen, mnotediv);
									break;
								}
								if( !strncmp(p,"P:",2) ) {	// a [P:X] field inline
									if( abcparts != NULL ) {
										// make h->tracktime start of a new age...
										abc_add_partbreak(h, h->track, h->tracktime);
										t = abc_patno(h, h->tracktime);
										if(	global_part == ' ' )
											partpat[26][1] = t;
										else
											partpat[global_part - 'A'][1] = t;
										// give every new coming abcevent the desired part indication
										while( isspace(p[2]) || p[2]=='.' ) p++;	// skip blancs and dots
										if( isupper(p[2]) )
											global_part = p[2];
										else 
											global_part = ' ';
										if(	global_part == ' ' )
											partpat[26][0] = t;
										else
											partpat[global_part - 'A'][0] = t;
									}
									for( ; *p && *p != ']'; p++ ) ;
									break;
								}
								if( !strncmp(p,"Q:",2) ) {
									abctempo = abc_extract_tempo(p+2,1);
									for( ; *p && *p != ']'; p++ ) ;
									abc_add_tempo_event(h, h->track, h->tracktime, abctempo);
									break;
								}
								if( !strncmp(p,"I:",2) ) { // interpret some of the possibilitys
									for( p += 2; isspace(*p); p++ ) ;
									if( !strncmp(p,"MIDI",4) && (p[4]=='=' || isspace(p[4])) ) { // interpret some of the possibilitys
										for( p += 4; isspace(*p); p++ ) ;
										if( *p == '=' )
											for( p += 1; isspace(*p); p++ ) ;											
										abc_MIDI_command(h, p, ']');
										if( h->tp ) abcnolegato = !h->tp->legato;
										if( !abcnolegato ) abcnoslurs = 0;
									}
									for( ; *p && *p != ']'; p++ ) ; // skip rest of inline field
								}
								if( *p && p[1] == ':' ) { // some other kind of inline field
									for( ; *p && *p != ']'; p++ ) ;
									break;
								}
								if( *p && strchr("abcdefgABCDEFG^_=",*p) ) {
									int cnl[8],cnd[8],vnl,nl0=0,nd0=0;	// for chords with notes of varying length
									abcchord = 0;
									vnl = 0;
									h->tp = abc_check_track(h, h->tp);
									abc_track_clear_tiedvpos(h);
									abcbeatvol = abc_beat_vol(h, abcvol, (h->tracktime - bartime)/notelen_notediv_to_ticks(h->speed,1,mnotediv));
									while( (ch=*p++) && (ch != ']') ) {
										h->tp = abc_locate_track(h, h->tp->v, abcchord? abcchord+DRONEPOS2: 0);
										p += abc_add_noteon(h, ch, p, h->tracktime, barsig, abcbeatvol, abceffect, abceffoper);
										p += abc_notelen(p, &notelen, &notediv);
										if( *p == '-' ) {
											p++;
											if( h->tp->tail->flg != 1 )
											h->tp->tienote = h->tp->tail;
										}
										if( abcchord<8 ) {
											cnl[abcchord] = notelen;
											cnd[abcchord] = notediv;
										}
										if( abcchord==0 ) {
											cnotelen = notelen;
											cnotediv = notediv;
											nl0 = notelen;
											nd0 = notediv;
										}
										else {
											if( cnotelen != notelen || cnotediv != notediv ) {
												vnl = 1;
												// update to longest duration
												if( cnotelen * notediv < notelen * cnotediv ) {
													cnotelen = notelen;
													cnotediv = notediv;
													abc_track_untie_short_chordnotes(h);
												}
												if( cnotelen * notediv > notelen * cnotediv ) {
													if( h->tp->tienote ) {
														abc_message("short notes in chord can not be tied:\n%s", h->line);
														h->tp->tienote = 0; // short chord notes cannot be tied...
													}
												}
												// update to shortest duration
												if( nl0 * notediv > notelen * nd0 ) {
													nl0 = notelen;
													nd0 = notediv;
												}
											}
										}
										abcchord++;
									}
									p += abc_notelen(p, &notelen, &notediv);
									if( (ch = *p) == '-' ) p++;	// tied chord...
									if( abcarpeggio ) {	// update starttime in the noteon events...
										thistime = notelen_notediv_to_ticks(h->speed, nl0*notelen*snotelen, nd0*notediv*snotediv)/abcchord;
										if( thistime > abcticks(h->speed) ) thistime = abcticks(h->speed);
										for( nl0=1; nl0<abcchord; nl0++ ) {
											h->tp = abc_locate_track(h, h->tp->v, nl0+DRONEPOS2);
											h->tp->tail->tracktick = h->tracktime + thistime * nl0;
										}
									}	
									notelen *= cnotelen;
									notediv *= cnotediv;
									tupletr = abc_tuplet(&notelen, &notediv, tupletp, tupletq, tupletr);
									while( isspace(*p) ) p++;	// allow spacing in broken rithm notation
									p += abc_brokenrithm(p, &notelen, &notediv, &brokenrithm, abchornpipe);
									thistime = notelen_notediv_to_ticks(h->speed, notelen*snotelen, notediv*snotediv);
									if( abcfermata ) {
										thistime <<= 1;
										abcfermata = 0;
									}
									if( thistime > abcgrace ) {
										thistime -= abcgrace;
										abcgrace = 0;
									}
									else {
										abcgrace -= thistime;
										thistime = abcticks(h->speed);
										abcgrace += abcticks(h->speed);
									}
									h->tracktime += thistime; 
									while( abcchord>0 ) {
										abcchord--;
										h->tp = abc_locate_track(h, h->tp->v, abcchord? abcchord+DRONEPOS2: 0);
										if( vnl && (abcchord < 8) && (cnl[abcchord] != cnotelen || cnd[abcchord] != cnotediv) ) {
											abc_add_noteoff(h, h->tp, 
												h->tracktime - thistime
											 	+ (thistime * cnl[abcchord] * cnotediv)/(cnd[abcchord] * cnotelen) );
										}
										else {
											if( ch=='-' && h->tp->tail->flg != 1 )
												h->tp->tienote = h->tp->tail;	// copy noteon event to tienote in track
											if( thistime > abcticks(h->speed) )
												abc_add_noteoff(h, h->tp, h->tracktime - abcnoslurs);
											else
												abc_add_noteoff(h, h->tp, h->tracktime);
										}
									}
									if( h->gchordon && (h->tp == h->tpc) )	
										abc_add_gchord(h, h->tracktime, bartime);
									if( h->drumon && (h->tp == h->tpr) )	
										abc_add_drum(h, h->tracktime, bartime);
									abcarpeggio = 0;
									if( abceffoper != 255 ) abceffect = none;
									break;
								}
								if( isdigit(*p) ) {	// different endings in repeats [i,j,n-r,s,...
									h->tp = abc_check_track(h, h->tp);
									abc_add_partbreak(h, h->tp, h->tracktime);
									p += abc_getnumber(p, &notelen);
									abc_add_variant_start(h, h->tp, h->tracktime, notelen);
									while( *p==',' || *p=='-' ) {
										if( *p==',' ) {
											p++;
											p += abc_getnumber(p, &notelen);
											abc_add_variant_choise(h->tp, notelen);
										}
										else {
											p++;
											p += abc_getnumber(p, &notediv);
											while( notelen < notediv ) {
												notelen++;
												abc_add_variant_choise(h->tp, notelen);
											}
										}
									}
									break;
								}
								// collect the notes in the chord
								break;
							case '(':	// slurs follow or some tuplet (duplet, triplet etc.)
								abcto = 0;
								if( isdigit(*p) ) {
									p += abc_getnumber(p,&tupletp);
									tupletr = tupletp;	// ABC draft 2.0 (4.13): if r is not given it defaults to p
									switch( tupletp ) {	// ABC draft 2.0 (4.13): q defaults depending on p and time signature
										case 2: case 4: case 8:
											tupletq = 3;
											break;
										case 3: case 6:
											tupletq = 2;
											break;
										default:
											if( snotediv == 8 )
												tupletq = 3;
											else
												tupletq = 2;
											break;
									}
									if( *p==':' ) {
										p++;
										if( isdigit(*p) ) p += abc_getnumber(p,&tupletq);
										if( *p==':' ) {
											p++;
											if( isdigit(*p) ) p += abc_getnumber(p,&tupletr);
										}
									}
								}
								else 
									abcnoslurs=0;
								break;
							case ')':	// end of slurs
								abcto = 0;
								abcnoslurs = abcnolegato;
								break;
							case '{':	// grace notes follow
								abcto = 0;
								h->tp = abc_check_track(h, h->tp);
								abc_track_clear_tiedvpos(h);
								abcgrace = 0;
								abcbeatvol = abc_beat_vol(h, abcvol, (h->tracktime - bartime)/notelen_notediv_to_ticks(h->speed,1,mnotediv));
								while( (ch=*p++) && (ch != '}') ) {
									p += abc_add_noteon(h, ch, p, h->tracktime+abcgrace, barsig, abcbeatvol, none, 0);
									p += abc_notelen(p, &notelen, &notediv);
									if( *p=='-' ) {
										p++;
										if( h->tp->tail->flg != 1 )
										h->tp->tienote = h->tp->tail;
									}
									notediv *= 4;	// grace notes factor 4 shorter (1/8 => 1/32)
									abcgrace += notelen_notediv_to_ticks(h->speed, notelen*snotelen, notediv*snotediv);
									abc_add_noteoff(h, h->tp, h->tracktime + abcgrace);
								}
								h->tracktime += abcgrace;
								abc_add_sync(h, h->tp, h->tracktime);
								if( h->gchordon && (h->tp == h->tpc) )	
									abc_add_gchord(h, h->tracktime, bartime);
								if( h->drumon && (h->tp == h->tpr) )	
									abc_add_drum(h, h->tracktime, bartime);
								break;
							case '|':	// bar symbols
								abcto = 0;
								if( h->gchordon && h->tp && (h->tp == h->tpc) )
									abc_add_gchord(h, h->tracktime, bartime);
								if( h->drumon && (h->tp == h->tpr) )	
									abc_add_drum(h, h->tracktime, bartime);
								sprintf(barsig, "%s%s", sig[abckey], sig[abckey]);	// reset the key signature
								bartime = h->tracktime;
								if( h->tp && h->tp->vpos ) h->tp = abc_locate_track(h, h->tp->v, 0); // reset from voice overlay
								if( isdigit(*p) ) {	// different endings in repeats |i,j,n-r,s,...
									h->tp = abc_check_track(h, h->tp);
									abc_add_partbreak(h, h->tp, h->tracktime);
									p += abc_getnumber(p, &notelen);
									abc_add_variant_start(h, h->tp, h->tracktime, notelen);
									while( *p==',' || *p=='-' ) {
										if( *p==',' ) {
											p++;
											p += abc_getnumber(p, &notelen);
											abc_add_variant_choise(h->tp, notelen);
										}
										else {
											p++;
											p += abc_getnumber(p, &notediv);
											while( notelen < notediv ) {
												notelen++;
												abc_add_variant_choise(h->tp, notelen);
											}
										}
									}
									break;
								}
								if( *p==':' ) {	// repeat start
									p++;
									h->tp = abc_check_track(h, h->tp);
									abc_add_partbreak(h, h->tp, h->tracktime);
									abc_add_setloop(h, h->tp, h->tracktime);
								}
								break;
							case '&': // voice overlay
								abcto = 0;
								h->tracktime = bartime;
								h->tp = abc_check_track(h, h->tp);
								t = h->tp->vpos;
								h->tp = abc_locate_track(h, h->tp->v, t? t+1: DRONEPOS2+1);
								break;
							case ']':	// staff break, end of song
								abcto = 0;
								break;
							case ':': // repeat jump
								abcto = 0;
								h->tp = abc_check_track(h, h->tp);
								j = (h->tp->slidevol == -2)? jumpfade: jumpnormal;
								abc_add_setjumploop(h, h->tp, h->tracktime, j);
								abc_add_partbreak(h, h->tp, h->tracktime);
								if( *p==':' ) {	// repeat start without intermediate bar symbol
									p++;
									abc_add_setloop(h, h->tp, h->tracktime);
								}
								break;
							case '"':	// chord notation
								if( !strchr("_^<>@", *p) && !isdigit(*p) ) { // if it's not a annotation string
									h->tp = abc_check_track(h, h->tp);
									if( !h->tpc ) h->tpc = abc_locate_track(h, h->tp->v, 0);
 									if( h->tp == h->tpc ) abc_add_chord(p, h, h->tpc, h->tracktime); // only do chords for one voice
								}
								abcto = 0;
								while( (ch=*p++) && (ch != '"') ) {
									if( !strncasecmp(p,"fade",4) && h->track && h->track->slidevol > -2 )
										abc_globalslide(h, h->tracktime, -2); // set volumeslide to fade away...
									if( !strncasecmp(p,"to coda",7) ) {
										h->tp = abc_check_track(h, h->tp);
										abc_add_partbreak(h, h->tp, h->tracktime);
										abc_add_tocoda(h, h->tp, h->tracktime);
										p+=7;
										abcto = -1;
									}
									else
										if( !isspace(*p) ) abcto = 0;
									if( !strncasecmp(p,"to",2) && (isspace(p[2]) || p[2] == '"') ) abcto = 1;
								}
								if( !ch ) abcstate = INSKIPFORQUOTE;
								break;
							case '\\':	// skip the rest of this line, should be the end of the line anyway
								while( (ch=*p++) )
									;
								ch = '\\'; // remember for invoice tempo changes....
								break;
							case '!':	// line break, or deprecated old style decoration
							case '+': // decorations new style
								if( !strncmp(p,"coda",4) && p[4] == ch ) {
									h->tp = abc_check_track(h, h->tp);
									if( abcto ) {
										if( abcto > 0 ) {
											abc_add_partbreak(h, h->tp, h->tracktime);
											abc_add_tocoda(h, h->tp, h->tracktime);
										}
									}
									else {
										abc_add_partbreak(h, h->tp, h->tracktime);
										abc_add_coda(h, h->tp, h->tracktime);
									}
									p += 5;
									abcto = 0;
									break;
								}
								abcto = 0;
								if( !strncmp(p,"arpeggio",8) && p[8] == ch ) {
									abcarpeggio = 1;
									p += 9;
									break;
								}
								if( !strncmp(p,"crescendo(",10) && p[10] == ch ) {
									h->tp = abc_check_track(h, h->tp);
									abc_globalslide(h, h->tracktime, 1);
									p += 11;
									break;
								}
								if( !strncmp(p,"crescendo)",10) && p[10] == ch ) {
									h->tp = abc_check_track(h, h->tp);
									abc_globalslide(h, h->tracktime, 0);
									p += 11;
									break;
								}
								if( !strncmp(p,"<(",2) && p[2] == ch ) {
									h->tp = abc_check_track(h, h->tp);
									abc_globalslide(h, h->tracktime, 1);
									p += 3;
									break;
								}
								if( !strncmp(p,"<)",2) && p[2] == ch ) {
									h->tp = abc_check_track(h, h->tp);
									abc_globalslide(h, h->tracktime, 0);
									p += 3;
									break;
								}
								if( !strncmp(p,"dimimuendo(",11) && p[11] == ch ) {
									h->tp = abc_check_track(h, h->tp);
									abc_globalslide(h, h->tracktime, -1);
									p += 12;
									break;
								}
								if( !strncmp(p,"diminuendo)",11) && p[11] == ch ) {
									h->tp = abc_check_track(h, h->tp);
									abc_globalslide(h, h->tracktime, 0);
									p += 12;
									break;
								}
								if( !strncmp(p,">(",2) && p[2] == ch ) {
									h->tp = abc_check_track(h, h->tp);
									abc_globalslide(h, h->tracktime, -1);
									p += 3;
									break;
								}
								if( !strncmp(p,">)",2) && p[2] == ch ) {
									h->tp = abc_check_track(h, h->tp);
									abc_globalslide(h, h->tracktime, 0);
									p += 3;
									break;
								}
								if( !strncmp(p,"upbow",5) && p[5] == ch ) {
									abceffect = bow;
									abceffoper = 1;
									p += 6;
									break;
								}
								if( !strncmp(p,"downbow",7) && p[7] == ch ) {
									abceffect = bow;
									abceffoper = 0;
									p += 8;
									break;
								}
								if( !strncmp(p,"trill",5) && p[5] == ch ) {
									abceffect = trill;
									abceffoper = 0;
									p += 6;
									break;
								}
								if( !strncmp(p,"trill(",6) && p[6] == ch ) {
									abceffect = trill;
									abceffoper = 255;
									p += 7;
									break;
								}
								if( !strncmp(p,"trill)",6) && p[6] == ch ) {
									abceffect = none;
									abceffoper = 0;
									p += 7;
									break;
								}
								if( !strncmp(p,"accent",6) && p[6] == ch ) {
									abceffect = accent;
									abceffoper = 0;
									p += 7;
									break;
								}
								if( !strncmp(p,"emphasis",8) && p[8] == ch ) {
									abceffect = accent;
									abceffoper = 0;
									p += 9;
									break;
								}
								if( !strncmp(p,">",1) && p[1] == ch ) {
									abceffect = accent;
									abceffoper = 0;
									p += 2;
									break;
								}
								if( !strncmp(p,"fermata",7) && p[7] == ch ) {
									abcfermata = 1;
									p += 8;
									break;
								}
								if( !strncmp(p,"fine",4) && p[4] == ch ) {
									h->tp = abc_check_track(h, h->tp);
									abc_add_partbreak(h, h->tp, h->tracktime);
									abc_add_fine(h, h->tp, h->tracktime);
									p += 5;
									break;
								}
								if( !strncmp(p,"segno",5) && p[5] == ch ) {
									h->tp = abc_check_track(h, h->tp);
									abc_add_partbreak(h, h->tp, h->tracktime);
									abc_add_segno(h, h->tp, h->tracktime);
									p += 6;
									break;
								}
								if( !strncmp(p,"tocoda",6) && p[6] == ch ) {
									h->tp = abc_check_track(h, h->tp);
									abc_add_partbreak(h, h->tp, h->tracktime);
									abc_add_tocoda(h, h->tp, h->tracktime);
									p += 7;
									break;
								}
								if( !strncmp(p,"D.C.",4) && p[4] == ch ) {
									h->tp = abc_check_track(h, h->tp);
									j = (h->tp->slidevol == -2)? jumpdcfade: jumpdacapo;
									abc_add_setjumploop(h, h->tp, h->tracktime, j);
									abc_add_partbreak(h, h->tp, h->tracktime);
									p += 5;
									break;
								}
								if( !strncmp(p,"D.S.",4) && p[4] == ch ) {
									h->tp = abc_check_track(h, h->tp);
									j = (h->tp->slidevol == -2)? jumpdsfade: jumpdasegno;
									abc_add_setjumploop(h, h->tp, h->tracktime, j);
									abc_add_partbreak(h, h->tp, h->tracktime);
									p += 5;
									break;
								}
								if( !strncmp(p,"dacapo",6) && p[6] == ch ) {
									h->tp = abc_check_track(h, h->tp);
									j = (h->tp->slidevol == -2)? jumpdcfade: jumpdacapo;
									abc_add_setjumploop(h, h->tp, h->tracktime, j);
									abc_add_partbreak(h, h->tp, h->tracktime);
									p += 7;
									break;
								}
								if( !strncmp(p,"dacoda",6) && p[6] == ch ) {
									h->tp = abc_check_track(h, h->tp);
									j = (h->tp->slidevol == -2)? jumpdcfade: jumpdacapo;
									abc_add_setjumploop(h, h->tp, h->tracktime, j);
									abc_add_partbreak(h, h->tp, h->tracktime);
									p += 7;
									break;
								}
								if( ch == '!' ) {
									for( t=0; p[t] && strchr("|[:]!",p[t])==0 && !isspace(p[t]); t++ ) ;
									if( p[t] == '!' ) {	// volume and other decorations, deprecated
										h->tp = abc_check_track(h, h->tp);
										abcvol = abc_parse_decorations(h, h->tp, p);
										p = &p[t+1];
									}
								}
								else {
									h->tp = abc_check_track(h, h->tp);
									abcvol = abc_parse_decorations(h, h->tp, p);
									while( (ch=*p++) && (ch != '+') )
										;
								}
								break;
							case '`': // back quotes are for readability
								break;
							case '.':	// staccato marks
								break;
							default:	// some kinda note must follow
								if( strchr("abcdefgABCDEFG^_=X",ch) ) {
									h->tp = abc_check_track(h, h->tp);
									abc_track_clear_tiedvpos(h);
									abcbeatvol = abc_beat_vol(h, abcvol, (h->tracktime - bartime)/notelen_notediv_to_ticks(h->speed,1,mnotediv));
									p += abc_add_noteon(h, ch, p, h->tracktime, barsig, abcbeatvol, abceffect, abceffoper);
									if( abceffoper != 255 ) abceffect = none;
									p += abc_notelen(p, &notelen, &notediv);
									if( *p=='-' ) {
										p++;
										if( h->tp->tail->flg != 1 )
										h->tp->tienote = h->tp->tail;
									}
									tupletr = abc_tuplet(&notelen, &notediv, tupletp, tupletq, tupletr);
									while( isspace(*p) ) p++;	// allow spacing in broken rithm notation
									p += abc_brokenrithm(p, &notelen, &notediv, &brokenrithm, abchornpipe);
									thistime = notelen_notediv_to_ticks(h->speed, notelen*snotelen, notediv*snotediv);
									if( abcfermata ) {
										thistime <<= 1;
										abcfermata = 0;
									}
									if( thistime > abcgrace ) {
										thistime -= abcgrace;
										abcgrace = 0;
									}
									else {
										abcgrace -= thistime;
										thistime = abcticks(h->speed);
										abcgrace += abcticks(h->speed);
									}
									h->tracktime += thistime;
									if( thistime > abcticks(h->speed) )
										abc_add_noteoff(h, h->tp, h->tracktime - abcnoslurs - (( ch0 == '.')? thistime / 2: 0));
									else
										abc_add_noteoff(h, h->tp, h->tracktime);
									abc_add_sync(h, h->tp, h->tracktime);
									if( h->gchordon && (h->tp == h->tpc) )
										abc_add_gchord(h, h->tracktime, bartime);
									if( h->drumon && (h->tp == h->tpr) )	
										abc_add_drum(h, h->tracktime, bartime);
									abcarpeggio = 0;
									break;
								}
								if( strchr("zx",ch) ) {
									h->tp = abc_check_track(h, h->tp);
									abc_track_clear_tiednote(h);
									p += abc_notelen(p, &notelen, &notediv);
									tupletr = abc_tuplet(&notelen, &notediv, tupletp, tupletq, tupletr);
									while( isspace(*p) ) p++;	// allow spacing in broken rithm notation
									p += abc_brokenrithm(p, &notelen, &notediv, &brokenrithm, abchornpipe);
									thistime = notelen_notediv_to_ticks(h->speed, notelen*snotelen, notediv*snotediv);
									if( abcfermata ) {
										thistime <<= 1;
										abcfermata = 0;
									}
									if( thistime > abcgrace ) {
										thistime -= abcgrace;
										abcgrace = 0;
									}
									else {
										abcgrace -= thistime;
										thistime = abcticks(h->speed);
										abcgrace += abcticks(h->speed);
									}
									h->tracktime += thistime; 
									abc_add_sync(h, h->tp, h->tracktime);
									if( h->gchordon && (h->tp == h->tpc) )
										abc_add_gchord(h, h->tracktime, bartime);
									if( h->drumon && (h->tp == h->tpr) )	
										abc_add_drum(h, h->tracktime, bartime);
									abcarpeggio = 0;
									break;
								}
								if( strchr("Z",ch) ) {
									h->tp = abc_check_track(h, h->tp);
									abc_track_clear_tiednote(h);
									p += abc_notelen(p, &notelen, &notediv);
									thistime = notelen_notediv_to_ticks(h->speed, notelen*mnotelen, notediv*mnotediv);
									if( abcfermata ) {
										thistime <<= 1;
										abcfermata = 0;
									}
									if( thistime > abcgrace ) {
										thistime -= abcgrace;
										abcgrace = 0;
									}
									else {
										abcgrace -= thistime;
										thistime = abcticks(h->speed);
										abcgrace += abcticks(h->speed);
									}
									h->tracktime += thistime; 
									sprintf(barsig, "%s%s", sig[abckey], sig[abckey]);	// reset the key signature
									abc_add_sync(h, h->tp, h->tracktime);
									if( h->gchordon && (h->tp == h->tpc) )
										abc_add_gchord(h, h->tracktime, bartime);
									if( h->drumon && (h->tp == h->tpr) )	
										abc_add_drum(h, h->tracktime, bartime);
									abcarpeggio = 0;
									break;
								}
								if( isalpha(ch) && *p==':' ) {
									// some unprocessed field line?
									while( *p ) p++;	// skip it
									break; 
								}
								break;
						}
						ch0 = ch;	// remember previous char, can be staccato dot...
						if( pp ) {	// did we have a U: macro substitution?
							if( !*p ) {
								p = pp;
								pp = 0;
							}
						}
					}
				}
			}
		}
		if( mmsp ) mmfclose(mmstack[mmsp]);
	}
	ABC_CleanupMacros(h);	// we dont need them anymore
	if( !h->track ) {
		char buf[10];
		sprintf(buf,"%d",abcxnumber);
		abc_message("abc X:%s has no body", buf);
		h->track = abc_check_track(h, h->track); // for sanity...
	}
	if( abcstate == INBODY ) {
		// last but not least shut off all pending events
		abc_recalculate_tracktime(h);
		for( ttp=h->track; ttp; ttp=ttp->next )
			abc_add_noteoff(h,ttp,h->tracktime);
		abc_add_partbreak(h, h->track, h->tracktime);
		t = abc_patno(h, h->tracktime);
		if( abc_pattracktime(h, h->tracktime) % abcticks(64 * h->speed) ) t++;
		if(	global_part == ' ' ) {
			partpat[26][1] = t;
			if( abcparts ) {
				for( t=0; t<26; t++ )
					if( partpat[t][0] < partpat[t][1] ) break;
				if( t == 26 ) {
					abc_message("parts (%s) set but not used", abcparts);
					abc_set_parts(&abcparts, 0);	// forget the parts array
				}
			}
		}
		else
			partpat[global_part - 'A'][1] = t;
		if( !abcparts ) abc_song_to_parts(h, &abcparts, partpat);
		orderlen = abc_partpat_to_orderlist(partpat, abcparts, h, &orderlist, orderlen);
	}
	abc_synchronise_tracks(h);	// distribute all control events
	abc_recalculate_tracktime(h);
/*
	
	abctrack:
		tracktick		long
		note				byte
		octave			byte
		instrument	byte
		effects			byte
	
	tick = tracktick modulo speed
	row  = (tracktick div speed) modulo 64
	pat  = (tracktick div speed) div 64
	ord  = calculated

*/
	if( (p=getenv(ABC_ENV_DUMPTRACKS)) ) {
		printf("P:%s\n",abcparts);
		for( t=0; t<26; t++ )
			if( partpat[t][1] >= partpat[t][0] )
				printf("  %c ",t+'A');
		if( partpat[26][1] >= partpat[26][0] )
			printf("All");
		printf("\n");
		for( t=0; t<27; t++ )
			if( partpat[t][1] >= partpat[t][0] )
				printf("%3d ",partpat[t][0]);
		printf("\n");
		for( t=0; t<27; t++ )
			if( partpat[t][1] >= partpat[t][0] )
				printf("%3d ",partpat[t][1]);
		printf("\n");
		for( t=0; (int)t<orderlen; t++ )
			printf("%3d ",t);
		printf("\n");
		for( t=0; (int)t<orderlen; t++ )
			printf("%3d ",orderlist[t]);
		printf("\n");
		abc_dumptracks(h,p);
	}
	// set module variables
	if( abctempo == 0 ) abctempo = abcrate;
	if( m_nDefaultTempo == 0 ) m_nDefaultTempo = abctempo;
#ifdef NEWMIKMOD
	of->memsize     = PTMEM_LAST;      // Number of memory slots to reserve!
	of->modtype     = _mm_strdup(of->allochandle, ABC_Version);
	of->numpat      = 1+(modticks(h->tracktime) / h->speed / 64);
	of->numpos      = orderlen;
	of->reppos      = 0;
	of->initspeed   = h->speed;
	of->numchn      = abc_numtracks(h);
	of->numtrk      = of->numpat * of->numchn;
	of->initvolume  = 64;
	of->pansep      = 128;
	// orderlist
	if(!AllocPositions(of, orderlen)) {
		avoid_reentry = 0;
		return FALSE;
	}
	for(t=0; t<orderlen; t++)
		of->positions[t] = orderlist[t];
	_mmalloc_close(h->ho);	// get rid of orderlist memory
#else
	m_nType         = MOD_TYPE_ABC;
	numpat          = 1+(modticks(h->tracktime) / h->speed / 64);
	m_nDefaultSpeed = h->speed;
	m_nChannels     = abc_numtracks(h);
	m_dwSongFlags   = SONG_LINEARSLIDES;
	m_nMinPeriod    = 28 << 2;
	m_nMaxPeriod    = 1712 << 3;
	// orderlist
	for(t=0; t < (uint32_t)orderlen; t++)
		Order[t] = orderlist[t];
	free(orderlist);	// get rid of orderlist memory
#endif
#ifdef NEWMIKMOD
	// ==============================
	// Load the pattern info now!
	if(!AllocTracks(of)) return 0;
	if(!AllocPatterns(of)) return 0;
	of->ut = utrk_init(of->numchn, h->allochandle);
	utrk_memory_reset(of->ut);
	utrk_local_memflag(of->ut, PTMEM_PORTAMENTO, TRUE, FALSE);
	ABC_ReadPatterns(of, h, of->numpat);
	// load instruments after building the patterns (chan == 10 track handling)
	if( !PAT_Load_Instruments(of) ) {
		avoid_reentry = 0;
		return FALSE;
	}
	// ============================================================
	// set panning positions
	for(t=0; t<of->numchn; t++) {
		of->panning[t] = PAN_LEFT+((t+2)%5)*((PAN_RIGHT - PAN_LEFT)/5);     // 0x30 = std s3m val
	}
#else
	// ==============================
	// Load the pattern info now!
	if( ABC_ReadPatterns(Patterns, PatternSize, h, numpat, m_nChannels) ) {
		// :^(  need one more channel to handle the global events ;^b
		m_nChannels++;
		h->tp = abc_locate_track(h, "", 99);
		abc_add_sync(h, h->tp, h->tracktime);
		for( t=0; t<numpat; t++ ) {
			FreePattern(Patterns[t]);
			Patterns[t] = NULL;
		}
		ABC_ReadPatterns(Patterns, PatternSize, h, numpat, m_nChannels);
	}
	// load instruments after building the patterns (chan == 10 track handling)
	if( !PAT_Load_Instruments(this) ) {
		avoid_reentry = 0;
		return FALSE;
	}
	// ============================================================
	// set panning positions
	for(t=0; t<m_nChannels; t++) {
		ChnSettings[t].nPan = 0x30+((t+2)%5)*((0xD0 - 0x30)/5);     // 0x30 = std s3m val
		ChnSettings[t].nVolume = 64;
	}
#endif
	avoid_reentry = 0; // it is safe now, I'm finished
	abc_set_parts(&abcparts, 0);	// free the parts array
#ifndef NEWMIKMOD
	ABC_Cleanup(h);	// we dont need it anymore
#endif
	return 1;
}

#ifdef NEWMIKMOD
// =====================================================================================
CHAR *ABC_LoadTitle(MMSTREAM *mmfile)
// =====================================================================================
{
	char s[128];
	int i;
	// get the first line with T:songtitle
	_mm_fseek(mmfile,0,SEEK_SET);
	while(abc_fgets(mmfile,s,128)) {
		if( s[0]=='T' && s[1]==':' ) {
			for( i=2; s[i]==' '; i++ ) ;
			return(DupStr(NULL, s+i,strlen(s+i)));
		}
	}
	return NULL;
}

MLOADER load_abc =
{
    "ABC",
    "ABC draft 2.0",
    0x30,
    NULL,
    ABC_Test,
    (void *(*)(void))ABC_Init,
    (void (*)(ML_HANDLE *))ABC_Cleanup,
    /* Every single loader seems to need one of these! */
    (BOOL (*)(ML_HANDLE *, UNIMOD *, MMSTREAM *))ABC_Load,
    ABC_LoadTitle
};
#endif
