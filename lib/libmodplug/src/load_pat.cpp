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
 Module: LOAD_PAT

  PAT sample loader.
	by Peter Grootswagers (2006)
	<email:pgrootswagers@planet.nl>

 It's primary purpose is loading samples for the .abc and .mid modules
 Can also be used stand alone, in that case a tune (frere Jacques)
 is generated using al samples available in the .pat file

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

#ifdef MSC_VER
#define DIRDELIM		'\\'
#define TIMIDITYCFG	"C:\\TIMIDITY\\TIMIDITY.CFG"
#define PATHFORPAT	"C:\\TIMIDITY\\INSTRUMENTS"
#else
#define DIRDELIM		'/'
#define TIMIDITYCFG	"/usr/local/share/timidity/timidity.cfg"
#define PATHFORPAT	"/usr/local/share/timidity/instruments"
#endif

#define PAT_ENV_PATH2CFG			"MMPAT_PATH_TO_CFG"

// 128 gm and 63 drum
#define MAXSMP				191
static char midipat[MAXSMP][40];
static char pathforpat[128];
static char timiditycfg[128];

#pragma pack(1)

typedef struct {
	char header[12];	// ascizz GF1PATCH110
	char gravis_id[10];	// allways ID#000002
	char description[60];
	BYTE instruments;
	BYTE voices;
	BYTE channels;
	WORD waveforms;
	WORD master_volume;
	DWORD data_size;
	char reserved[36];
} PatchHeader;

typedef struct {
	WORD instrument_id;
	char instrument_name[16];
	DWORD instrument_size;
	BYTE layers;
	char reserved[40];
} InstrumentHeader;

typedef struct {
	BYTE layer_dup;
	BYTE layer_id;
	DWORD layer_size;
	BYTE samples;
	char reserved[40];
} LayerHeader;

typedef struct {
	char wave_name[7];
	BYTE fractions;
	DWORD wave_size;
	DWORD start_loop;
	DWORD end_loop;
	WORD sample_rate;
	DWORD low_frequency ;
	DWORD high_frequency;
	DWORD root_frequency;
	short int tune;
	BYTE balance;
	BYTE envelope_rate[6];
	BYTE envelope_offset[6];
	BYTE tremolo_sweep;
	BYTE tremolo_rate;
	BYTE tremolo_depth;
	BYTE vibrato_sweep;
	BYTE vibrato_rate;
	BYTE vibrato_depth;
	BYTE modes;
	DWORD scale_frequency;
	DWORD scale_factor;
	char reserved[32];
} WaveHeader;

// WaveHeader.modes bits
#define PAT_16BIT     1
#define PAT_UNSIGNED  2
#define PAT_LOOP      4
#define PAT_PINGPONG  8
#define PAT_BACKWARD 16
#define PAT_SUSTAIN  32
#define PAT_ENVELOPE 64
#define PAT_CLAMPED 128

#define C4SPD	8363
#define C4mHz	523251
#define C4	  523.251f
#define PI 	  3.141592653589793f
#define OMEGA	((2.0f * PI * C4)/(float)C4SPD)

/**************************************************************************
**************************************************************************/
#ifdef NEWMIKMOD
static char  PAT_Version[] = "Timidity GUS Patch v1.0";
#endif
static BYTE pat_gm_used[MAXSMP];
static BYTE pat_loops[MAXSMP];

/**************************************************************************
**************************************************************************/

typedef struct _PATHANDLE
{
#ifdef NEWMIKMOD
	MM_ALLOC *allochandle;
#endif
	char patname[16];
	int samples;
} PATHANDLE;

// local prototypes
static int pat_getopt(const char *s, const char *o, int dflt);

static void pat_message(const char *s1, const char *s2)
{
	char txt[256];
	if( strlen(s1) + strlen(s2) > 255 ) return;
	sprintf(txt, s1, s2);
#ifdef NEWMIKMOD
	_mmlog(txt);
#else
	fprintf(stderr, "load_pat > %s\n", txt);
#endif
}

void pat_resetsmp(void)
{
	int i;
	for( i=0; i<MAXSMP; i++ ) {
		pat_loops[i] = 0;
		pat_gm_used[i] = 0;
	}
}

int pat_numsmp()
{
	return strlen((const char *)pat_gm_used);
}

int pat_numinstr(void)
{
	return strlen((const char *)pat_gm_used);
}

int pat_smptogm(int smp)
{
	if( smp < MAXSMP )
		return pat_gm_used[smp - 1];
	return 1;
}

int pat_gmtosmp(int gm)
{
	int smp;
	for( smp=0; pat_gm_used[smp]; smp++ )
		if( pat_gm_used[smp] == gm )
			return smp+1;
	if( smp < MAXSMP ) {
		pat_gm_used[smp] = gm;
		return smp+1;
	}
	return 1;
}

int pat_smplooped(int smp)
{
	if( smp < MAXSMP ) return pat_loops[smp - 1];
	return 1;
}

const char *pat_gm_name(int gm)
{
	static char buf[40];
	if( gm < 1 || gm > MAXSMP ) {
		sprintf(buf, "invalid gm %d", gm);
		return buf;
	}
	return midipat[gm - 1];
}

int pat_gm_drumnr(int n)
{
	if( n < 25 ) return 129;
	if( n+129-25 < MAXSMP )
		return 129+n-25; // timidity.cfg drum patches start at 25
	return MAXSMP;
}

int pat_gm_drumnote(int n)
{
	char *p;
	p = strchr(midipat[pat_gm_drumnr(n)-1], ':');
	if( p ) return pat_getopt(p+1, "note", n);
	return n;
}

static float pat_sinus(int i)
{
	float res = sinf(OMEGA * (float)i);
	return res;
}

static float pat_square(int i)
{
	float res = 30.0f * sinf(OMEGA * (float)i);
	if( res > 0.99f ) return 0.99f;
	if( res < -0.99f ) return -0.99f;
	return res;
}

static float pat_sawtooth(int i)
{
	float res = OMEGA * (float)i;
	while( res > 2 * PI )
		res -= 2 * PI;
	i = 2;
	if( res > PI ) {
		res = PI - res;
		i = -2;
	}
	res = (float)i * res / PI;
	if( res > 0.9f ) return 1.0f - res;
	if( res < -0.9f ) return 1.0f + res;
	return res;
}

typedef float (*PAT_SAMPLE_FUN)(int);

static PAT_SAMPLE_FUN pat_fun[] = { pat_sinus, pat_square, pat_sawtooth };

#ifdef NEWMIKMOD

#define MMFILE						MMSTREAM
#define mmftell(x)				_mm_ftell(x)
#define mmfseek(f,p,w)		_mm_fseek(f,p,w)
#define mmreadUBYTES(buf,sz,f)	_mm_read_UBYTES(buf,sz,f)

#else

#define MMSTREAM										FILE
#define _mm_fopen(name,mode)				fopen(name,mode)
#define _mm_fgets(f,buf,sz)					fgets(buf,sz,f)
#define _mm_fseek(f,pos,whence)			fseek(f,pos,whence)
#define _mm_ftell(f)								ftell(f)
#define _mm_read_UBYTES(buf,sz,f)		fread(buf,sz,1,f)
#define _mm_read_SBYTES(buf,sz,f)		fread(buf,sz,1,f)
#define _mm_feof(f)									feof(f)
#define _mm_fclose(f)								fclose(f)
#define DupStr(h,buf,sz)						strdup(buf)
#define _mm_calloc(h,n,sz)					calloc(n,sz)
#define _mm_recalloc(h,buf,sz,elsz)	realloc(buf,sz)
#undef  _mm_free
#define _mm_free(h,p)								free(p)

typedef struct {
	char *mm;
	int sz;
	int pos;
} MMFILE;

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

static void mmreadUBYTES(BYTE *buf, long sz, MMFILE *mmfile)
{
	memcpy(buf, &mmfile->mm[mmfile->pos], sz);
	mmfile->pos += sz;
}

static void mmreadSBYTES(char *buf, long sz, MMFILE *mmfile)
{
	memcpy(buf, &mmfile->mm[mmfile->pos], sz);
	mmfile->pos += sz;
}

#endif

void pat_init_patnames(void)
{
	int i,j;
	char *p, *q;
	char line[80];
	MMSTREAM *mmcfg;
	strcpy(pathforpat, PATHFORPAT);
	strcpy(timiditycfg, TIMIDITYCFG);
	p = getenv(PAT_ENV_PATH2CFG);
	if( p ) {
		strcpy(timiditycfg,p);
		strcpy(pathforpat,p);
		strcat(timiditycfg,"/timidity.cfg");
		strcat(pathforpat,"/instruments");
	}
	mmcfg = _mm_fopen(timiditycfg,"r");
	for( i=0; i<MAXSMP; i++ )	midipat[i][0] = '\0';
	if( !mmcfg ) {
		pat_message("can not open %s, use environment variable " PAT_ENV_PATH2CFG " for the directory", timiditycfg);
	}
	else {
		// read in bank 0 and drum patches
		j = 0;
		_mm_fgets(mmcfg, line, 80);
		while( !_mm_feof(mmcfg) ) {
			if( isdigit(line[0]) ) {
				i = atoi(line);
				if( i < MAXSMP && i >= 0 ) {
					p = strchr(line,'/')+1;
					if(j) 
						q = midipat[pat_gm_drumnr(i)-1];
					else
						q = midipat[i];
					while( *p && !isspace(*p) )	*q++ = *p++;
					if( isspace(*p) ) {
						*q++ = ':';
						while( isspace(*p) ) {
							while( isspace(*p) ) p++;
							while( *p && !isspace(*p) ) *q++ = *p++;
							if( isspace(*p) ) *q++ = ' ';
						}
					}
					*q++ = '\0';
				}
			}
			if( !strncmp(line,"drumset",7) ) j = 1;
			_mm_fgets(mmcfg, line, 80);
		}
		_mm_fclose(mmcfg);
	}
	q = midipat[0];
	j = 0;
	for( i=0; i<MAXSMP; i++ )	{
		if( midipat[i][0] ) q = midipat[i];
		else {
			strcpy(midipat[i],q);
			if( midipat[i][0] == '\0' ) j++;
		}
	}
	if( j ) {
		for( i=MAXSMP; i-- > 0; )	{
			if( midipat[i][0] ) q = midipat[i];
			else strcpy(midipat[i],q);
		}
	}
}

static char *pat_build_path(char *fname, int pat)
{
	char *ps;
	ps = strrchr(midipat[pat], ':');
	if( ps ) {
		sprintf(fname, "%s%c%s", pathforpat, DIRDELIM, midipat[pat]);
		strcpy(strrchr(fname, ':'), ".pat");
		return ps;
	}
	sprintf(fname, "%s%c%s.pat", pathforpat, DIRDELIM, midipat[pat]);
	return 0;
}

static void pat_read_patname(PATHANDLE *h, MMFILE *mmpat) {
	InstrumentHeader ih;
	mmfseek(mmpat,sizeof(PatchHeader), SEEK_SET);
	mmreadUBYTES((BYTE *)&ih, sizeof(InstrumentHeader), mmpat);
	strncpy(h->patname, ih.instrument_name, 16);
	h->patname[15] = '\0';
}

static void pat_read_layerheader(MMSTREAM *mmpat, LayerHeader *hl)
{
	_mm_fseek(mmpat,sizeof(PatchHeader)+sizeof(InstrumentHeader), SEEK_SET);
	_mm_read_UBYTES((BYTE *)hl, sizeof(LayerHeader), mmpat);
}

static void pat_get_layerheader(MMFILE *mmpat, LayerHeader *hl)
{
	InstrumentHeader ih;
	mmfseek(mmpat,sizeof(PatchHeader), SEEK_SET);
	mmreadUBYTES((BYTE *)&ih, sizeof(InstrumentHeader), mmpat);
	mmreadUBYTES((BYTE *)hl, sizeof(LayerHeader), mmpat);
	strncpy(hl->reserved, ih.instrument_name, 40);
}

static int pat_read_numsmp(MMFILE *mmpat) {
	LayerHeader hl;
	pat_get_layerheader(mmpat, &hl);
	return hl.samples;
}

static void pat_read_waveheader(MMSTREAM *mmpat, WaveHeader *hw, int layer)
{
	long int pos, bestpos=0;
	LayerHeader hl;
	ULONG bestfreq, freqdist;
	int i;
	// read the very first and maybe only sample
	pat_read_layerheader(mmpat, &hl);
	if( hl.samples > 1 ) {
		if( layer ) {
			if( layer > hl.samples ) layer = hl.samples; // you don't fool me....
			for( i=1; i<layer; i++ ) {
				_mm_read_UBYTES((BYTE *)hw, sizeof(WaveHeader), mmpat);
				_mm_fseek(mmpat, hw->wave_size, SEEK_CUR);
			}
		}
		else {
			bestfreq = C4mHz * 1000;	// big enough
			for( i=0; i<hl.samples; i++ ) {
				pos = _mm_ftell(mmpat);
				_mm_read_UBYTES((BYTE *)hw, sizeof(WaveHeader), mmpat);
				if( hw->root_frequency > C4mHz )
					freqdist = hw->root_frequency - C4mHz;
				else
					freqdist = 2 * (C4mHz - hw->root_frequency);
				if( freqdist < bestfreq ) {
					bestfreq = freqdist;
					bestpos  = pos;
				}
				_mm_fseek(mmpat, hw->wave_size, SEEK_CUR);
			}
			_mm_fseek(mmpat, bestpos, SEEK_SET);
		}
	}
	_mm_read_UBYTES((BYTE *)hw, sizeof(WaveHeader), mmpat);
	strncpy(hw->reserved, hl.reserved, 36);
	if( hw->start_loop >= hw->wave_size ) {
		hw->start_loop = 0;
		hw->end_loop = 0;
		hw->modes &= ~PAT_LOOP; // mask off loop indicator
	}
	if( hw->end_loop > hw->wave_size )
		hw->end_loop = hw->wave_size;
}

#ifndef NEWMIKMOD
static void pat_get_waveheader(MMFILE *mmpat, WaveHeader *hw, int layer)
{
	long int pos, bestpos=0;
	LayerHeader hl;
	ULONG bestfreq, freqdist;
	int i;
	// read the very first and maybe only sample
	pat_get_layerheader(mmpat, &hl);
	if( hl.samples > 1 ) {
		if( layer ) {
			if( layer > hl.samples ) layer = hl.samples; // you don't fool me....
			for( i=1; i<layer; i++ ) {
				mmreadUBYTES((BYTE *)hw, sizeof(WaveHeader), mmpat);
				mmfseek(mmpat, hw->wave_size, SEEK_CUR);
			}
		}
		else {
			bestfreq = C4mHz * 1000;	// big enough
			for( i=0; i<hl.samples; i++ ) {
				pos = mmftell(mmpat);
				mmreadUBYTES((BYTE *)hw, sizeof(WaveHeader), mmpat);
				if( hw->root_frequency > C4mHz )
					freqdist = hw->root_frequency - C4mHz;
				else
					freqdist = 2 * (C4mHz - hw->root_frequency);
				if( freqdist < bestfreq ) {
					bestfreq = freqdist;
					bestpos  = pos;
				}
				mmfseek(mmpat, hw->wave_size, SEEK_CUR);
			}
			mmfseek(mmpat, bestpos, SEEK_SET);
		}
	}
	mmreadUBYTES((BYTE *)hw, sizeof(WaveHeader), mmpat);
	if( hw->start_loop >= hw->wave_size ) {
		hw->start_loop = 0;
		hw->end_loop = 0;
		hw->modes &= ~PAT_LOOP; // mask off loop indicator
	}
	if( hw->end_loop > hw->wave_size )
		hw->end_loop = hw->wave_size;
}
#endif

static int pat_readpat_attr(int pat, WaveHeader *hw, int layer)
{
	char fname[128];
	MMSTREAM *mmpat;
	pat_build_path(fname, pat);
	mmpat = _mm_fopen(fname, "r");
	if( !mmpat )
		return 0;
	pat_read_waveheader(mmpat, hw, layer);
	_mm_fclose(mmpat);
	return 1;
}

static void pat_amplify(char *b, int num, int amp, int m)
{
	char *pb;
	BYTE *pu;
	short int *pi;
	WORD *pw;
	int i,n,v;
	n = num;
	if( m & PAT_16BIT ) { // 16 bit
		n >>= 1;
		if( m & 2 ) {	// unsigned
			pw = (WORD *)b;
			for( i=0; i<n; i++ ) {
				v = (((int)(*pw) - 0x8000) * amp) / 100;
				if( v < -0x8000 ) v = -0x8000;
				if( v >  0x7fff ) v =  0x7fff; 
				*pw++ = v + 0x8000;
			}
		}
		else {
			pi = (short int *)b;
			for( i=0; i<n; i++ ) {
				v = ((*pi) * amp) / 100;
				if( v < -0x8000 ) v = -0x8000;
				if( v >  0x7fff ) v =  0x7fff; 
				*pi++ = v;
			}
		}
	}
	else {
		if( m & 2 ) {	// unsigned
			pu = (BYTE *)b;
			for( i=0; i<n; i++ ) {
				v = (((int)(*pu) - 0x80) * amp) / 100;
				if( v < -0x80 ) v = -0x80;
				if( v >  0x7f ) v =  0x7f;
				*pu++ = v + 0x80;
			}
		}
		else {
			pb = (char *)b;
			for( i=0; i<n; i++ ) {
				v = ((*pb) * amp) / 100;
				if( v < -0x80 ) v = -0x80;
				if( v >  0x7f ) v =  0x7f;
				*pb++ = v;
			}
		}
	}
}

static int pat_getopt(const char *s, const char *o, int dflt)
{
	const char *p;
	if( !s ) return dflt;
	p = strstr(s,o);
	if( !p ) return dflt;
	return atoi(strchr(p,'=')+1);
}

static void pat_readpat(int pat, char *dest, int num)
{
	static int readlasttime = 0, wavesize = 0;
	static MMSTREAM *mmpat = 0;
	static char *opt = 0;
	int amp;
	char fname[128];
	WaveHeader hw;
#ifdef NEWMIKMOD
	static int patlast = MAXSMP;
	if( !dest ) { // reset
		if( mmpat )	_mm_fclose(mmpat);
		readlasttime = 0;
		wavesize = 0;
		mmpat = 0;
		patlast = MAXSMP;
		return;
	}
	if( pat != patlast ) { // reset for other instrument
		if( mmpat )	_mm_fclose(mmpat);
		readlasttime = 0;
		patlast = pat;
	}
#endif
	if( !readlasttime ) {
		opt=pat_build_path(fname, pat);
		mmpat = _mm_fopen(fname, "r");
		if( !mmpat )
			return;
		pat_read_waveheader(mmpat, &hw, 0);
		wavesize = hw.wave_size;
	}
	_mm_read_SBYTES(dest, num, mmpat);
	amp = pat_getopt(opt,"amp",100);
	if( amp != 100 ) pat_amplify(dest, num, amp, hw.modes);
	readlasttime += num;
	if( readlasttime < wavesize ) return;
	readlasttime = 0;
	_mm_fclose(mmpat);
	mmpat = 0;
}

#ifdef NEWMIKMOD
// next code pinched from dec_raw.c and rebuild to load bytes from different places
// =====================================================================================
static void *dec_pat_Init(MMSTREAM *mmfp)
{
	pat_readpat(0,0,0); // initialize pat loader
	return (void *)mmfp;
}

static void dec_pat_Cleanup(void *raw)
{
}

static BOOL dec_pat_Decompress16Bit(void *raw, short int *dest, int cbcount, MMSTREAM *mmfp)
{
	long samplenum = _mm_ftell(mmfp) - 1;
#else
static BOOL dec_pat_Decompress16Bit(short int *dest, int cbcount, int samplenum)
{
#endif
	int i;
	PAT_SAMPLE_FUN f;
	if( samplenum < MAXSMP ) pat_readpat(samplenum, (char *)dest, cbcount*2);
	else {
		f = pat_fun[(samplenum - MAXSMP) % 3];
		for( i=0; i<cbcount; i++ )
			dest[i] = (short int)(32000.0*f(i));
	}
  return cbcount;
}

// convert 8 bit data to 16 bit!
// We do the conversion in reverse so that the data we're converting isn't overwritten
// by the result.
static void	pat_blowup_to16bit(short int *dest, int cbcount) {
	char *s;
	short int *d;
	int t;
	s  = (char *)dest;
	d  = dest;
	s += cbcount;
	d += cbcount;
	for(t=0; t<cbcount; t++)
	{   s--;
			d--;
			*d = (*s) << 8;
	}
}

#ifdef NEWMIKMOD
static BOOL dec_pat_Decompress8Bit(void *raw, short int *dest, int cbcount, MMSTREAM *mmfp)
{
	long samplenum = _mm_ftell(mmfp) - 1;
#else
static BOOL dec_pat_Decompress8Bit(short int *dest, int cbcount, int samplenum)
{
#endif
	int i;
	PAT_SAMPLE_FUN f;
	if( samplenum < MAXSMP ) pat_readpat(samplenum, (char *)dest, cbcount);
	else {
		f = pat_fun[(samplenum - MAXSMP) % 3];
		for( i=0; i<cbcount; i++ )
			dest[i] = (char)(120.0*f(i));
	}
	pat_blowup_to16bit(dest, cbcount);
	return cbcount;
}

#ifdef NEWMIKMOD
SL_DECOMPRESS_API dec_pat =
{
    NULL,
    SL_COMPRESS_RAW,
    dec_pat_Init,
    dec_pat_Cleanup,
    dec_pat_Decompress16Bit,
    dec_pat_Decompress8Bit,
};
#endif

// =====================================================================================
#ifdef NEWMIKMOD
BOOL PAT_Test(MMSTREAM *mmfile)
#else
BOOL CSoundFile::TestPAT(const BYTE *lpStream, DWORD dwMemLength)
#endif
// =====================================================================================
{
	PatchHeader ph;
#ifdef NEWMIKMOD
  _mm_fseek(mmfile,0,SEEK_SET);
	_mm_read_UBYTES((BYTE *)&ph, sizeof(PatchHeader), mmfile);
#else
	if( dwMemLength < sizeof(PatchHeader) ) return 0;
	memcpy((BYTE *)&ph, lpStream, sizeof(PatchHeader));
#endif
	if( !strcmp(ph.header,"GF1PATCH110") && !strcmp(ph.gravis_id,"ID#000002") ) return 1;
	return 0;
}

// =====================================================================================
static PATHANDLE *PAT_Init(void)
{
    PATHANDLE   *retval;
#ifdef NEWMIKMOD
    MM_ALLOC     *allochandle;
    
		allochandle = _mmalloc_create("Load_PAT", NULL);
    retval = (PATHANDLE *)_mm_calloc(allochandle, 1,sizeof(PATHANDLE));
		if( !retval ) return NULL;
    SL_RegisterDecompressor(&dec_raw);	// we can not get the samples out of our own routines...!
#else
    retval = (PATHANDLE *)calloc(1,sizeof(PATHANDLE));
		if( !retval ) return NULL;
#endif
    return retval;
}

// =====================================================================================
static void PAT_Cleanup(PATHANDLE *handle)
// =====================================================================================
{
#ifdef NEWMIKMOD
	if(handle && handle->allochandle) {
		_mmalloc_close(handle->allochandle);
		handle->allochandle = 0;
	}
#else
	if(handle) {
		free(handle);
	}
#endif
}

static char tune[] = "c d e c|c d e c|e f g..|e f g..|gagfe c|gagfe c|c G c..|c G c..|";
static int pat_note(int abc)
{
	switch( abc ) {
		case 'C': return 48;
		case 'D': return 50;
		case 'E': return 52;
		case 'F': return 53;
		case 'G': return 55;
		case 'A': return 57;
		case 'B': return 59;
		case 'c': return 60;
		case 'd': return 62;
		case 'e': return 64;
		case 'f': return 65;
		case 'g': return 67;
		case 'a': return 69;
		case 'b': return 71;
		default:
			break;
	}
	return 0;
}

int pat_modnote(int midinote)
{
	int n;
	n = midinote;
#ifdef NEWMIKMOD
	if( n < 12 ) n++;
	else n-=11;
#else
	n += 13;
#endif
	return n;
}

// =====================================================================================
#ifdef NEWMIKMOD
static void PAT_ReadPatterns(UNIMOD *of, PATHANDLE *h, int numpat)
// =====================================================================================
{
	int pat,row,i,ch;
	BYTE n,ins,vol;
	int t;
	int tt1, tt2;
	UNITRK_EFFECT eff;

	tt2 = (h->samples - 1) * 16 + 128;
	for( pat = 0; pat < numpat; pat++ ) {
		utrk_reset(of->ut);
		for( row = 0; row < 64; row++ ) {
			tt1 = (pat * 64 + row);
			for( ch = 0; ch < h->samples; ch++ ) {
				t = tt1 - ch * 16;
				if( t >= 0 ) {
					i = tt2 - 16 * ((h->samples - 1 - ch) & 3);
					if( tt1 < i ) {
						t = t % 64;
						if( isalpha(tune[t]) ) {
							utrk_settrack(of->ut, ch);
							n   = pat_modnote(pat_note(tune[t]));
							ins = ch;
							vol = 100;
							if( (t % 16) == 0 ) {
								vol += vol / 10;
								if( vol > 127 ) vol = 127;
							}
							utrk_write_inst(of->ut, ins);
							utrk_write_note(of->ut, n); // <- normal note
							pt_write_effect(of->ut, 0xc, vol);
						}
						if( tt1 ==  i - 1 && ch == 0 && row < 63 ) {
							eff.effect   = UNI_GLOB_PATBREAK;
							eff.param.u  = 0;
							eff.framedly = UFD_RUNONCE;
							utrk_write_global(of->ut, &eff, UNIMEM_NONE);
						}
					}
					else {
						if( tt1 == i ) {
							eff.param.u = 0;
							eff.effect  = UNI_NOTEKILL;
							utrk_write_local(of->ut, &eff, UNIMEM_NONE);
						}
					}
				}
			}
			utrk_newline(of->ut);
		}
		if(!utrk_dup_pattern(of->ut,of)) return;
	}
}

#else

static void PAT_ReadPatterns(MODCOMMAND *pattern[], WORD psize[], PATHANDLE *h, int numpat)
// =====================================================================================
{
	int pat,row,i,ch;
	BYTE n,ins,vol;
	int t;
	int tt1, tt2;
	MODCOMMAND *m;
	if( numpat > MAX_PATTERNS ) numpat = MAX_PATTERNS;

	tt2 = (h->samples - 1) * 16 + 128;
	for( pat = 0; pat < numpat; pat++ ) {
		pattern[pat] = CSoundFile::AllocatePattern(64, h->samples);
		if( !pattern[pat] ) return;
		psize[pat] = 64;
		for( row = 0; row < 64; row++ ) {
			tt1 = (pat * 64 + row);
			for( ch = 0; ch < h->samples; ch++ ) {
				t = tt1 - ch * 16;
				m = &pattern[pat][row * h->samples + ch];
				m->param   = 0;
				m->command = CMD_NONE;
				if( t >= 0 ) {
					i = tt2 - 16 * ((h->samples - 1 - ch) & 3);
					if( tt1 < i ) {
						t = t % 64;
						if( isalpha(tune[t]) ) {
							n   = pat_modnote(pat_note(tune[t]));
							ins = ch + 1;
							vol = 40;
							if( (t % 16) == 0 ) {
								vol += vol / 10;
								if( vol > 64 ) vol = 64;
							}
							m->instr  = ins;
							m->note   = n; // <- normal note
							m->volcmd = VOLCMD_VOLUME;
							m->vol    = vol;
						}
						if( tt1 ==  i - 1 && ch == 0 && row < 63 ) {
							m->command = CMD_PATTERNBREAK;
						}
					}
					else {
						if( tt1 == i ) {
							m->param   = 0;
							m->command = CMD_KEYOFF;
							m->volcmd = VOLCMD_VOLUME;
							m->vol    = 0;
						}
					}
				}
			}
		}
	}
}

#endif

// calculate the best speed that approximates the pat root frequency as a C note
static ULONG pat_patrate_to_C4SPD(ULONG patRate , ULONG patMilliHz)
{
	ULONG u;
	double x, y;
	u = patMilliHz;
	x = 0.1 * patRate;
	x = x * C4mHz;
	y = u * 0.4;
	x = x / y;
	u = (ULONG)(x+0.5);
	return u;
}

// return relative position in samples for the rate starting with offset start ending with offset end
static int pat_envelope_rpos(int rate, int start, int end)
{
	int r, p, t, s;
	// rate byte is 3 bits exponent and 6 bits increment size
	//   eeiiiiii
	// every 8 to the power ee the volume is incremented/decremented by iiiiii
	// Thank you Gravis for this weirdness...
	r = 3 - ((rate >> 6) & 3) * 3;
	p = rate & 0x3f;
	if( !p ) return 0;
	t = end - start;
	if( !t ) return 0;
	if (t < 0) t = -t;
	s = (t << r)/ p;
	return s;
}

static void	pat_modenv(WaveHeader *hw, int mpos[6], int mvol[6])
{
	int i, sum, s;
	BYTE *prate = hw->envelope_rate, *poffset = hw->envelope_offset;
	for( i=0; i<6; i++ ) {
		mpos[i] = 0;
		mvol[i] = 64;
	}
	if( !memcmp(prate, "??????", 6) || poffset[5] >= 100 ) return; // weird rates or high env end volume
	if( !(hw->modes & PAT_SUSTAIN) ) return; // no sustain thus no need for envelope
	s = hw->wave_size;
	if (s == 0) return;
	if( hw->modes & PAT_16BIT )
		s >>= 1;
	// offsets 0 1 2 3 4 5 are distributed over 0 2 4 6 8 10, the odd numbers are set in between
	sum = 0;
	for( i=0; i<6; i++ ) {
		mvol[i] = poffset[i];
		mpos[i] = pat_envelope_rpos(prate[i], i? poffset[i-1]: 0, poffset[i]);
		sum += mpos[i];
	}
	if( sum == 0 ) return;
	if( sum > s ) {
		for( i=0; i<6; i++ )
			mpos[i] = (s * mpos[i]) / sum;
	}
	for( i=1; i<6; i++ )
		mpos[i] += mpos[i-1];
	for( i=0; i<6 ; i++ ) {
		mpos[i] = (256 * mpos[i]) / s;
		mpos[i]++;
		if( i > 0 && mpos[i] <= mpos[i-1] ) {
			if( mvol[i] == mvol[i-1] ) mpos[i] = mpos[i-1];
			else mpos[i] = mpos[i-1] + 1;
		}
		if( mpos[i] > 256 ) mpos[i] = 256;
	}
	mvol[5] = 0; // kill Bill....
}

#ifdef NEWMIKMOD
static void pat_setpat_inst(WaveHeader *hw, INSTRUMENT *d, int smp)
{
	int u, inuse;
	int envpoint[6], envvolume[6];
	for(u=0; u<120; u++) {        
		d->samplenumber[u] = smp;
		d->samplenote[u] = smp;
	}
	d->globvol = 64;
	d->volfade = 0;
	d->volflg  = EF_CARRY;
	d->panflg  = EF_CARRY;
	if( hw->modes & PAT_ENVELOPE ) d->volflg |= EF_ON;
	if( hw->modes & PAT_SUSTAIN ) d->volflg |= EF_SUSTAIN;
	if( (hw->modes & PAT_LOOP) && (hw->start_loop != hw->end_loop) ) d->volflg |= EF_LOOP;
	d->volsusbeg = 1;
	d->volsusend = 1;
	d->volbeg    = 1;
	d->volend    = 2;
	d->volpts    = 6;
	// scale volume envelope:
	inuse = 0;
	pat_modenv(hw, envpoint, envvolume);
	for(u=0; u<6; u++)
	{
		if( envvolume[u] != 64 ) inuse = 1;
		d->volenv[u].val = envvolume[u]<<2;
		d->volenv[u].pos = envpoint[u];
	}
	if(!inuse) d->volpts = 0;
	d->pansusbeg = 0;
	d->pansusend = 0;
	d->panbeg    = 0;
	d->panend    = 0;
	d->panpts    = 0;
	// scale panning envelope:
	for(u=0; u<12; u++)
	{
		d->panenv[u].val = 0;
		d->panenv[u].pos = 0;
	}
	d->panpts = 0;
}
#else
static void pat_setpat_inst(WaveHeader *hw, INSTRUMENTHEADER *d, int smp)
{
	int u, inuse;
	int envpoint[6], envvolume[6];
	d->nMidiProgram = 0;
	d->nFadeOut = 0;
	d->nPan = 128;
	d->nPPC = 5*12;
	d->dwFlags = 0;
	if( hw->modes & PAT_ENVELOPE ) d->dwFlags |= ENV_VOLUME;
	if( hw->modes & PAT_SUSTAIN ) d->dwFlags |= ENV_VOLSUSTAIN;
	if( (hw->modes & PAT_LOOP) && (hw->start_loop != hw->end_loop) ) d->dwFlags |= ENV_VOLLOOP;
	d->nVolEnv = 6;
	//if (!d->nVolEnv) d->dwFlags &= ~ENV_VOLUME;
	d->nPanEnv = 0;
	d->nVolSustainBegin = 1;
	d->nVolSustainEnd   = 1;
	d->nVolLoopStart    = 1;
	d->nVolLoopEnd      = 2;
	d->nPanSustainBegin = 0;
	d->nPanSustainEnd   = 0;
	d->nPanLoopStart    = 0;
	d->nPanLoopEnd      = 0;
	d->nGlobalVol = 64;
	pat_modenv(hw, envpoint, envvolume);
	inuse = 0;
	for( u=0; u<6; u++)
	{
		if( envvolume[u] != 64 ) inuse = 1;
		d->VolPoints[u] = envpoint[u];
		d->VolEnv[u]    = envvolume[u];
		d->PanPoints[u] = 0;
		d->PanEnv[u]    = 0;
		if (u)
		{
			if (d->VolPoints[u] < d->VolPoints[u-1])
			{
				d->VolPoints[u] &= 0xFF;
				d->VolPoints[u] += d->VolPoints[u-1] & 0xFF00;
				if (d->VolPoints[u] < d->VolPoints[u-1]) d->VolPoints[u] += 0x100;
			}
		}
	}
	if( !inuse ) d->nVolEnv = 0;
	for( u=0; u<128; u++)
	{
		d->NoteMap[u] = u+1;
		d->Keyboard[u] = smp;
	}
}
#endif
#ifdef NEWMIKMOD
static void PATinst(UNIMOD *of, INSTRUMENT *d, int smp, int gm)
#else
static void PATinst(INSTRUMENTHEADER *d, int smp, int gm)
#endif
{
	WaveHeader hw;
	char s[32];
	memset(s,0,32);
	if( pat_readpat_attr(gm-1, &hw, 0) ) {
		pat_setpat_inst(&hw, d, smp);
	}
	else {
		hw.modes = PAT_16BIT|PAT_ENVELOPE|PAT_SUSTAIN|PAT_LOOP;
		hw.start_loop = 0;
		hw.end_loop = 30000;
		hw.wave_size  = 30000;
//  envelope rates and offsets pinched from timidity's acpiano.pat sample no 1
		hw.envelope_rate[0] = 0x3f;
		hw.envelope_rate[1] = 0x3f;
		hw.envelope_rate[2] = 0x3f;
		hw.envelope_rate[3] = 0x08|(3<<6);
		hw.envelope_rate[4] = 0x3f;
		hw.envelope_rate[5] = 0x3f;
		hw.envelope_offset[0] = 246;
		hw.envelope_offset[1] = 246;
		hw.envelope_offset[2] = 246;
		hw.envelope_offset[3] = 0;
		hw.envelope_offset[4] = 0;
		hw.envelope_offset[5] = 0;
		strncpy(hw.reserved, midipat[gm-1], sizeof(hw.reserved));
		pat_setpat_inst(&hw, d, smp);
	}
	if( hw.reserved[0] )
		strncpy(s, hw.reserved, 32);
	else
		strncpy(s, midipat[gm-1], 32);
#ifdef NEWMIKMOD
	d->insname = DupStr(of->allochandle, s,28);
#else
	s[31] = '\0';
	memset(d->name, 0, 32);
	strcpy((char *)d->name, s);
	strncpy(s, midipat[gm-1], 12);
	s[11] = '\0';
	memset(d->filename, 0, 12);
	strcpy((char *)d->filename, s);
#endif
}

#ifdef NEWMIKMOD
static void pat_setpat_attr(WaveHeader *hw, UNISAMPLE *q, int gm)
{
	q->seekpos   = gm;	// dec_pat expects the midi samplenumber in this
	q->speed     = pat_patrate_to_C4SPD(hw->sample_rate , hw->root_frequency);
	q->length    = hw->wave_size;
	q->loopstart = hw->start_loop;
	q->loopend   = hw->end_loop;
	q->volume    = 0x40;
	if( hw->modes & PAT_16BIT ) {
		q->format     |= SF_16BITS;
		q->length    >>= 1;
		q->loopstart >>= 1;
		q->loopend   >>= 1;
		q->speed     <<= 1;
	}
	if( (hw->modes & PAT_UNSIGNED)==0 ) q->format |= SF_SIGNED;
	if( hw->modes & PAT_LOOP ) {
		q->flags |= SL_LOOP;
		if( hw->modes & PAT_PINGPONG ) q->flags |= SL_SUSTAIN_BIDI;
		if( hw->modes & PAT_SUSTAIN ) q->flags |= SL_SUSTAIN_LOOP;
	}
}
#else
static void pat_setpat_attr(WaveHeader *hw, MODINSTRUMENT *q)
{
	q->nC4Speed   = pat_patrate_to_C4SPD(hw->sample_rate , hw->root_frequency);
	q->nLength    = hw->wave_size;
	q->nLoopStart = hw->start_loop;
	q->nLoopEnd   = hw->end_loop;
	q->nVolume    = 256;
	if( hw->modes & PAT_16BIT ) {
		q->nLength    >>= 1;
		q->nLoopStart >>= 1;
		q->nLoopEnd   >>= 1;
	}
	if( hw->modes & PAT_LOOP ) {
		q->uFlags |= CHN_LOOP;
		if( hw->modes & PAT_PINGPONG ) q->uFlags |= CHN_PINGPONGSUSTAIN;
		if( hw->modes & PAT_SUSTAIN ) q->uFlags |= CHN_SUSTAINLOOP;
	}
}
#endif

// ==========================
// Load those darned Samples!
#ifdef NEWMIKMOD
static void PATsample(UNIMOD *of, UNISAMPLE  *q, int smp, int gm)
#else
static void PATsample(CSoundFile *cs, MODINSTRUMENT *q, int smp, int gm)
#endif
{
	WaveHeader hw;
	char s[32];
	sprintf(s, "%d:%s", smp-1, midipat[gm-1]);
#ifdef NEWMIKMOD
	q->samplename = DupStr(of->allochandle, s,28);
	if( pat_readpat_attr(gm-1, &hw, 0) ) {
		pat_setpat_attr(&hw, q, gm);
		pat_loops[smp-1] = (q->flags & (SL_LOOP | SL_SUSTAIN_LOOP))? 1: 0;
	}
	else {
		q->seekpos    = smp + MAXSMP + 1;	// dec_pat expects the samplenumber in this
		q->speed      = C4SPD;
		q->length     = 30000;
		q->loopstart  = 0;
		q->loopend    = 30000;
		q->volume     = 0x40;

		// Enable aggressive declicking for songs that do not loop and that
		// are long enough that they won't be adversely affected.
		
		q->flags  |= SL_LOOP;
		q->format |= SF_16BITS;
		q->format |= SF_SIGNED;
	}
	if(!(q->flags & (SL_LOOP | SL_SUSTAIN_LOOP)) && (q->length > 5000))
		q->flags |= SL_DECLICK;
#else
	s[31] = '\0';
	memset(cs->m_szNames[smp], 0, 32);
	strcpy(cs->m_szNames[smp], s);
	q->nGlobalVol = 64;
	q->nPan       = 128;
	q->uFlags     = CHN_16BIT;
	if( pat_readpat_attr(gm-1, &hw, 0) ) {
		char *p;
		pat_setpat_attr(&hw, q);
		pat_loops[smp-1] = (q->uFlags & CHN_LOOP)? 1: 0;
		if( hw.modes & PAT_16BIT ) p = (char *)malloc(hw.wave_size);
		else p = (char *)malloc(hw.wave_size * sizeof(short int));
		if( p ) {
			if( hw.modes & PAT_16BIT ) {
				dec_pat_Decompress16Bit((short int *)p, hw.wave_size>>1, gm - 1);
				cs->ReadSample(q, (hw.modes&PAT_UNSIGNED)?RS_PCM16U:RS_PCM16S, (LPSTR)p, hw.wave_size);
			}
			else {
				dec_pat_Decompress8Bit((short int *)p, hw.wave_size, gm - 1);
				cs->ReadSample(q, (hw.modes&PAT_UNSIGNED)?RS_PCM16U:RS_PCM16S, (LPSTR)p, hw.wave_size * sizeof(short int));
			}
			free(p);
		}
	}
	else {
		char *p;
		q->nC4Speed   = C4SPD;
		q->nLength    = 30000;
		q->nLoopStart = 0;
		q->nLoopEnd   = 30000;
		q->nVolume    = 256;
		q->uFlags    |= CHN_LOOP;
		q->uFlags    |= CHN_16BIT;
		p = (char *)malloc(q->nLength*sizeof(short int));
		if( p ) {
			dec_pat_Decompress8Bit((short int *)p, q->nLength, smp + MAXSMP - 1);
			cs->ReadSample(q, RS_PCM16S, (LPSTR)p, q->nLength*2);
			free(p);
		}
	}
#endif
}

// =====================================================================================
BOOL PAT_Load_Instruments(void *c)
{
	uint32_t t;
#ifdef NEWMIKMOD
	UNIMOD *of = (UNIMOD *)c;
	INSTRUMENT *d;
	UNISAMPLE  *q;
	if( !pat_numsmp() ) pat_gmtosmp(1); // make sure there is a sample
	of->numsmp = pat_numsmp();
	of->numins = pat_numinstr();
	if(!AllocInstruments(of)) return FALSE;
	if(!AllocSamples(of, 0)) return FALSE;
	d = of->instruments;
	for(t=1; t<=of->numins; t++) {
		PATinst(of, d, t, pat_smptogm(t));
		d++;
	}
	q = of->samples;
	for(t=1; t<=of->numsmp; t++) {
		PATsample(of, q, t, pat_smptogm(t));
		q++;
	}
	SL_RegisterDecompressor(&dec_pat);	// fool him to generate samples
#else
	CSoundFile *of=(CSoundFile *)c;
	if( !pat_numsmp() ) pat_gmtosmp(1); // make sure there is a sample
	of->m_nSamples     = pat_numsmp() + 1; // xmms modplug does not use slot zero
	of->m_nInstruments = pat_numinstr() + 1;
	for(t=1; t<of->m_nInstruments; t++) { // xmms modplug doesn't use slot zero
		if( (of->Headers[t] = new INSTRUMENTHEADER) == NULL ) return FALSE;
		memset(of->Headers[t], 0, sizeof(INSTRUMENTHEADER));
		PATinst(of->Headers[t], t, pat_smptogm(t));
	}
	for(t=1; t<of->m_nSamples; t++) { // xmms modplug doesn't use slot zero
		PATsample(of, &of->Ins[t], t, pat_smptogm(t));
	}
	// copy last of the mohicans to entry 0 for XMMS modinfo to work....
	t = of->m_nInstruments - 1;
	if( (of->Headers[0] = new INSTRUMENTHEADER) == NULL ) return FALSE;
	memcpy(of->Headers[0], of->Headers[t], sizeof(INSTRUMENTHEADER));
	memset(of->Headers[0]->name, 0, 32);
	strncpy((char *)of->Headers[0]->name, "Timidity GM patches", 32);
	t = of->m_nSamples - 1;
	memcpy(&of->Ins[0], &of->Ins[t], sizeof(MODINSTRUMENT));
#endif
	return TRUE;
}
// =====================================================================================
#ifdef NEWMIKMOD
BOOL PAT_Load(PATHANDLE *h, UNIMOD *of, MMSTREAM *mmfile)
#else
BOOL CSoundFile::ReadPAT(const BYTE *lpStream, DWORD dwMemLength)
#endif
{
	static int avoid_reentry = 0;
	char buf[60];
	int t;
#ifdef NEWMIKMOD
	UNISAMPLE  *q;
	INSTRUMENT *d;
#define m_nDefaultTempo	of->inittempo
#else
	PATHANDLE *h;
	int numpat;
	MMFILE mm, *mmfile;
	MODINSTRUMENT *q;
	INSTRUMENTHEADER *d;
	if( !TestPAT(lpStream, dwMemLength) ) return FALSE;
	h = PAT_Init();
	if( !h ) return FALSE;
	mmfile = &mm;
	mm.mm = (char *)lpStream;
	mm.sz = dwMemLength;
	mm.pos = 0;
#endif
	while( avoid_reentry ) sleep(1);
	avoid_reentry = 1;
	pat_read_patname(h, mmfile);
	h->samples = pat_read_numsmp(mmfile);
	if( strlen(h->patname) )
		sprintf(buf,"%s canon %d-v (Fr. Jacques)", h->patname, h->samples);
	else
		sprintf(buf,"%d-voice canon (Fr. Jacques)", h->samples);
#ifdef NEWMIKMOD
	of->songname = DupStr(of->allochandle, buf, strlen(buf));
#else
	if( strlen(buf) > 31 ) buf[31] = '\0'; // chop it of
	strcpy(m_szNames[0], buf);
#endif
	m_nDefaultTempo = 60; // 120 / 2
	t = (h->samples - 1) * 16 + 128;
	if( t % 64 ) t += 64;
	t = t / 64;
#ifdef NEWMIKMOD
	of->memsize     = PTMEM_LAST;      // Number of memory slots to reserve!
	of->modtype     = _mm_strdup(of->allochandle, PAT_Version);
	of->reppos      = 0;
	of->numins      = h->samples;
	of->numsmp      = h->samples;
	of->initspeed   = 6;
	of->numchn      = h->samples;
	of->numpat      = t;
	of->numpos      = of->numpat;	// one repeating pattern
	of->numtrk      = of->numpat * of->numchn;
	of->initvolume  = 64;
	of->pansep=128;
	// allocate resources
	if(!AllocPositions(of, of->numpos)) {
		avoid_reentry = 0;
		return FALSE;
	}
	if(!AllocInstruments(of)) {
		avoid_reentry = 0;
		return FALSE;
	}
	if(!AllocSamples(of, 0)) {
		avoid_reentry = 0;
		return FALSE;
	}
	// orderlist
	for(t=0; t<of->numpos; t++)
		of->positions[t] = t;
	d = of->instruments;
	for(t=1; t<=of->numins; t++) {
		WaveHeader hw;
		char s[32];
		sprintf(s, "%s", h->patname);
		d->insname = DupStr(of->allochandle, s,28);
		pat_read_waveheader(mmfile, &hw, t);
		pat_setpat_inst(&hw, d, t);
	}
	q = of->samples;
	for(t=1; t<=of->numsmp; t++) {
		WaveHeader hw;
		char s[28];
		pat_read_waveheader(mmfile, &hw, t);
		pat_setpat_attr(&hw, q, _mm_ftell(mmfile));
		memset(s,0,28);
		if( hw.wave_name[0] )
			sprintf(s, "%d:%s", t, hw.wave_name);
		else
			sprintf(s, "%d:%s", t, h->patname);
		q->samplename = DupStr(of->allochandle, s,28);
		if(!(q->flags & (SL_LOOP | SL_SUSTAIN_LOOP)) && (q->length > 5000))
			q->flags |= SL_DECLICK;
		q++;
	}
#else
	m_nType         = MOD_TYPE_PAT;
	m_nInstruments  = h->samples + 1; // we know better but use each sample in the pat...
	m_nSamples      = h->samples + 1; // xmms modplug does not use slot zero
	m_nDefaultSpeed = 6;
	m_nChannels     = h->samples;
	numpat          = t;
	
	m_dwSongFlags   = SONG_LINEARSLIDES;
	m_nMinPeriod    = 28 << 2;
	m_nMaxPeriod    = 1712 << 3;
	// orderlist
	for(t=0; t < numpat; t++)
		Order[t] = t;
	for(t=1; t<(int)m_nInstruments; t++) { // xmms modplug doesn't use slot zero
		WaveHeader hw;
		char s[32];
		if( (d = new INSTRUMENTHEADER) == NULL ) {
			avoid_reentry = 0;
			return FALSE;
		}
		memset(d, 0, sizeof(INSTRUMENTHEADER));
		Headers[t] = d;
		sprintf(s, "%s", h->patname);
		s[31] = '\0';
		memset(d->name, 0, 32);
		strcpy((char *)d->name, s);
		s[11] = '\0';
		memset(d->filename, 0, 12);
		strcpy((char *)d->filename, s);
		pat_get_waveheader(mmfile, &hw, t);
		pat_setpat_inst(&hw, d, t);
	}
	for(t=1; t<(int)m_nSamples; t++) { // xmms modplug doesn't use slot zero
		WaveHeader hw;
		char s[32];
		char *p;
		q = &Ins[t];	// we do not use slot zero
		q->nGlobalVol = 64;
		q->nPan       = 128;
		q->uFlags     = CHN_16BIT;
		pat_get_waveheader(mmfile, &hw, t);
		pat_setpat_attr(&hw, q);
		memset(s,0,32);
		if( hw.wave_name[0] )
			sprintf(s, "%d:%s", t, hw.wave_name);
		else {
			if( h->patname[0] )
				sprintf(s, "%d:%s", t, h->patname);
			else
				sprintf(s, "%d:Untitled GM patch", t);
		}
		s[31] = '\0';
		memset(m_szNames[t], 0, 32);
		strcpy(m_szNames[t], s);
		if( hw.modes & PAT_16BIT ) p = (char *)malloc(hw.wave_size);
		else p = (char *)malloc(hw.wave_size * sizeof(short int));
		if( p ) {
			mmreadSBYTES(p, hw.wave_size, mmfile);
			if( hw.modes & PAT_16BIT ) {
				ReadSample(q, (hw.modes&PAT_UNSIGNED)?RS_PCM16U:RS_PCM16S, (LPSTR)p, hw.wave_size);
			}
			else {
				pat_blowup_to16bit((short int *)p, hw.wave_size);
				ReadSample(q, (hw.modes&PAT_UNSIGNED)?RS_PCM16U:RS_PCM16S, (LPSTR)p, hw.wave_size * sizeof(short int));
			}
			free(p);
		}
	}
	// copy last of the mohicans to entry 0 for XMMS modinfo to work....
	t = m_nInstruments - 1;
	if( (Headers[0] = new INSTRUMENTHEADER) == NULL ) {
		avoid_reentry = 0;
		return FALSE;
	}
	memcpy(Headers[0], Headers[t], sizeof(INSTRUMENTHEADER));
	memset(Headers[0]->name, 0, 32);
	if( h->patname[0] )
		strncpy((char *)Headers[0]->name, h->patname, 32);
	else
		strncpy((char *)Headers[0]->name, "Timidity GM patch", 32);
	t = m_nSamples - 1;
	memcpy(&Ins[0], &Ins[t], sizeof(MODINSTRUMENT));
#endif
#ifdef NEWMIKMOD
	// ==============================
	// Load the pattern info now!
	if(!AllocTracks(of)) return 0;
	if(!AllocPatterns(of)) return 0;
	of->ut = utrk_init(of->numchn, h->allochandle);
	utrk_memory_reset(of->ut);
	utrk_local_memflag(of->ut, PTMEM_PORTAMENTO, TRUE, FALSE);
	PAT_ReadPatterns(of, h, of->numpat);
	// ============================================================
	// set panning positions
	for(t=0; t<of->numchn; t++) {
		of->panning[t] = PAN_LEFT+((t+2)%5)*((PAN_RIGHT - PAN_LEFT)/5);     // 0x30 = std s3m val
	}
#else
	// ==============================
	// Load the pattern info now!
	PAT_ReadPatterns(Patterns, PatternSize, h, numpat);
	// ============================================================
	// set panning positions
	for(t=0; t<(int)m_nChannels; t++) {
		ChnSettings[t].nPan = 0x30+((t+2)%5)*((0xD0 - 0x30)/5);     // 0x30 = std s3m val
		ChnSettings[t].nVolume = 64;
	}
#endif
	avoid_reentry = 0; // it is safe now, I'm finished
#ifndef NEWMIKMOD
	PAT_Cleanup(h);	// we dont need it anymore
#endif
	return 1;
}

#ifdef NEWMIKMOD
// =====================================================================================
CHAR *PAT_LoadTitle(MMSTREAM *mmfile)
// =====================================================================================
{
	PATHANDLE dummy;
	pat_read_patname(&dummy, mmfile);
	return(DupStr(NULL, dummy.patname,strlen(dummy.patname)));
}

MLOADER load_pat =
{
    "PAT",
    "PAT loader 1.0",
    0x30,
    NULL,
    PAT_Test,
    (void *(*)(void))PAT_Init,
    (void (*)(ML_HANDLE *))PAT_Cleanup,
    /* Every single loader seems to need one of these! */
    (BOOL (*)(ML_HANDLE *, UNIMOD *, MMSTREAM *))PAT_Load,
    PAT_LoadTitle
};
#endif
