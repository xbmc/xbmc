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
 Module: LOAD_MID

  MID module loader.
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
#include "itshare.h" // for STMEM_PITCHSLIDE
typedef UBYTE BYTE;
typedef UWORD WORD;
#define MAX_POLYPHONY 12  // max notes in one midi channel
#define MAX_TRACKS    63  // max mod tracks
#define WHEELSHIFT    11  // how many bits the 13bit midi wheel value must shift right 
#else
#include "stdafx.h"
#include "sndfile.h"
#define PAN_LEFT    0x30
#define PAN_RIGHT   0xD0
#define MAX_POLYPHONY 16  // max notes in one midi channel
#define MAX_TRACKS    (MAX_BASECHANNELS-6)  // max mod tracks
#define WHEELSHIFT    10  // how many bits the 13bit midi wheel value must shift right 
#endif

#include "load_pat.h"

#define ROWSPERNOTE 16
#define ENV_MMMID_SPEED	"MMMID_SPEED"
#define ENV_MMMID_DEBUG "MMMID_DEBUG"
#define ENV_MMMID_VERBOSE "MMMID_VERBOSE"

/**************************************************************************
**************************************************************************/
#ifdef NEWMIKMOD
static char  MID_Version[] = "Musical Instrument Digital Interface";
#endif

typedef enum {
	none,
	wheeldown,
	wheelup,
	fxbrk,
	tmpo,
	fxsync,
	modwheel,
	mainvol,
	prog
} MIDEVENT_X_EFFECT;

typedef struct _MIDEVENT
{
	struct _MIDEVENT *next;
	ULONG	tracktick;
	BYTE flg; // 1 = note present
	BYTE note;
	BYTE volume;
	BYTE smpno;
	BYTE fx;
	BYTE fxparam;
} MIDEVENT;

typedef struct _MIDTRACK
{
	struct _MIDTRACK *next;
	MIDEVENT *head;
	MIDEVENT *tail;
	MIDEVENT *workevent; // keeps track of events in track
	int balance; // last balance on this track
	ULONG vtracktick; // tracktick of last note event (on or off)
	BYTE chan;
	BYTE vpos;	// 0xff is track is free for use, otherwise it's the note playing on this track
	BYTE volume; // last note volume on this track
	BYTE instr;	// current instrument for this track
} MIDTRACK;

#ifdef NEWMIKMOD

#define MMFILE						MMSTREAM
#define mmfseek(f,p,w)		_mm_fseek(f,p,w)
#define mmftell(x)				_mm_ftell(x)
#define mmreadUBYTE(f)	_mm_read_UBYTE(f)
#define mmreadSBYTES(buf,sz,f)	_mm_read_SBYTES(buf,sz,f)
#define mmreadUBYTES(buf,sz,f)	_mm_read_UBYTES(buf,sz,f)

#else

#define MMSTREAM										FILE
#define _mm_fseek(f,pos,whence)			fseek(f,pos,whence)
#define _mm_read_UBYTES(buf,sz,f)		fread(buf,sz,1,f)
#define _mm_read_SBYTES(buf,sz,f)		fread(buf,sz,1,f)
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

static long mmftell(MMFILE *mmfile)
{
	return mmfile->pos;
}

static BYTE mmreadUBYTE(MMFILE *mmfile)
{
	BYTE b;
	b = (BYTE)mmfile->mm[mmfile->pos];
	mmfile->pos++;
	return b;
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

/**************************************************************************
**************************************************************************/

typedef struct _MIDHANDLE
{
#ifdef NEWMIKMOD
	MM_ALLOC *allochandle;
	MM_ALLOC *trackhandle;
#endif
	MMFILE *mmf;
	MIDTRACK *track;
	MIDTRACK *tp;
	ULONG tracktime;
	const char *debug;
	const char *verbose;
	int speed;
	int midispeed;
	int midiformat;
	int resolution;
	int miditracks;
	int divider;
	int tempo;
	int percussion;
	long deltatime;
} MIDHANDLE;

static void mid_dump_tracks(MIDHANDLE *h)
{
	MIDTRACK *tr;
	MIDEVENT *e;
	int t;
	printf("tracktime  = %ld\n", (long)(h->tracktime));
	printf("speed      = %d\n", h->speed);
	printf("midispeed  = %d\n", h->midispeed);
	printf("midiformat = %d\n", h->midiformat);
	printf("resolution = %d\n", h->resolution);
	printf("miditracks = %d\n", h->miditracks);
	printf("divider    = %d\n", h->divider);
	printf("tempo      = %d\n", h->tempo);
	printf("percussion = %d\n", h->percussion);
	printf("deltatime  = %ld\n", h->deltatime);
	t = 0;
	for( tr=h->track; tr; tr = tr->next ) {
		t++;
		printf("TRACK %2d chan=%d note=0x%02x vol=%d pan=0x%02x instr=%d\n", t, tr->chan + 1, tr->vpos, tr->balance, tr->volume, tr->instr);
		for( e=tr->head; e; e=e->next ) {
			printf("%2d %6ld %s %3d %3d %3d ",
			       t, (long)(e->tracktick),
			       e->flg? "NOTE": "CTRL", e->note, e->volume, e->smpno);
			switch(	e->fx ) {
				case fxbrk: printf("fxbrk\n");break;
				case fxsync: printf("fxsync\n");break;
				case prog:	printf("prog %d\n", e->fxparam);break;
				case mainvol:	printf("mainvol %d\n", e->fxparam);break;
				case modwheel:	printf("modwheel %d\n", e->fxparam);break;
				case wheeldown:	printf("wheeldown %d\n", e->fxparam);break;
				case wheelup:	printf("wheelup %d\n", e->fxparam);break;
				case tmpo:	printf("tmpo %d\n", e->fxparam);break;
				default: printf("\n");break;
			}
		}
	}
}

static void mid_message(const char *s1, const char *s2)
{
	char txt[256];
	if( strlen(s1) + strlen(s2) > 255 ) return;
	sprintf(txt, s1, s2);
#ifdef NEWMIKMOD
	_mmlog(txt);
#else
	fprintf(stderr, "load_mid > %s\n", txt);
#endif
}

static ULONG miditicks(MIDHANDLE *h, ULONG modtick)
{
	return modtick * h->divider / ROWSPERNOTE / h->speed;
}

static ULONG modticks(MIDHANDLE *h, ULONG miditick)
{
	return miditick * ROWSPERNOTE * h->speed / h->divider;
}

static void mid_adjust_for_optimal_tempo(MIDHANDLE *h, int maxtempo)
{
	// the tempo is adjusted so that the maximum tempo is 255
	// this way we have the biggest change that very short notes get played
	// and we make sure the tempo doesn't become too large or too small
	// if the piece in hand isn't so weird it changes tempo from 20 to 255, that is.
	// tempo is only registered in first track (h->track) because it is a global event
	MIDEVENT *e;
	int d, t;
	if( maxtempo < 1 ) return;
	d = h->divider;
	t = maxtempo;
	h->divider = (t * d) / 255;
	while( (h->midispeed = miditicks(h, h->speed)) < h->speed ) {
		++t;
		h->divider = (t * d) / 255;
	}
	if( h->verbose && t > maxtempo )
		printf("Adjusted maximum tempo from %d to %d to get %d miditicks per patternrow\n",
			maxtempo, 2 * maxtempo - t, h->midispeed);
	if( h->track ) {
		for( e=h->track->head; e; e=e->next ) {
			if( e->fx == tmpo )
				e->fxparam = (255 * e->fxparam ) / t;
		}
	}
}

// =====================================================================================
static MIDEVENT *mid_new_event(MIDHANDLE *h)
// =====================================================================================
{
    MIDEVENT   *retval;

    retval = (MIDEVENT *)_mm_calloc(h->trackhandle, 1,sizeof(MIDEVENT));
		retval->next      = NULL;
    retval->tracktick = h->tracktime;
		retval->flg       = 0;
		retval->note      = 0;
		retval->volume    = 0;
		retval->smpno     = 0;
		retval->fx        = none;
		retval->fxparam   = 0;
    return retval;
}

// =====================================================================================
static MIDTRACK *mid_new_track(MIDHANDLE *h, int mch, int pos)
// =====================================================================================
{
    MIDTRACK *retval;
    retval = (MIDTRACK *)_mm_calloc(h->trackhandle, 1,sizeof(MIDTRACK));
		retval->next       = NULL;
    retval->vpos       = pos;
		retval->instr      = 1;
		retval->chan       = mch;
		retval->head       = NULL;
		retval->tail       = NULL;
		retval->workevent  = NULL;
		retval->vtracktick = 0;
		retval->volume     = h->track? h->track->volume: 120;
		retval->balance    = 64;
    return retval;
}

static int mid_numtracks(MIDHANDLE *h)
{
	int n;
	MIDTRACK *t;
	n=0;
	for( t = h->track; t; t=t->next )
		n++;
	return n;
}

// find out how many midichannel we have
static int mid_numchans(MIDHANDLE *h)
{
	int i,c,n;
	MIDTRACK *t;
	c = 0;
	for( t = h->track; t; t=t->next )
		c |= (1<<t->chan);
	n = 0;
	for( i=0; i<16; i++ )
		if( c & (1<<i) ) n++;
	return n;
}

// find out which ordinal a midichannel has
static int mid_ordchan(MIDHANDLE *h, int mch)
{
	int i,c,n;
	MIDTRACK *t;
	c = 0;
	for( t = h->track; t; t=t->next )
		c |= (1<<t->chan);
	n = 0;
	for( i=0; i<mch; i++ )
		if( c & (1<<i) ) n++;
	return n;
}

static void mid_rewind_tracks(MIDHANDLE *h)
{
	MIDTRACK *tr;
	h->tracktime = 0;
	for( tr = h->track; tr; tr = tr->next ) {
		tr->vpos       = 0xff;
		tr->workevent  = tr->head;
		tr->vtracktick = 0;
	}
}

static void mid_update_track(MIDTRACK *tr)
{
	MIDEVENT *e;
	e = tr->workevent;
	if( e->flg )	{
		if( e->volume ) tr->vpos = e->note;
		else tr->vpos = 0xff;
		tr->volume = e->volume;
		tr->vtracktick = e->tracktick;
	}
	if( e->fx == prog ) tr->instr = e->fxparam;
}

static void mid_sync_track(MIDTRACK *tr, ULONG tracktime)
{
	MIDEVENT *e;
	e = tr->workevent;
	if( e && e->tracktick > tracktime ) e = tr->head; // start again....
	for( ; e && e->tracktick <= tracktime; e=e->next ) {
		tr->workevent = e;
		mid_update_track(tr);
	}
}

// =====================================================================================
static MIDTRACK *mid_find_track(MIDHANDLE *h, int mch, int pos)
// =====================================================================================
{
	MIDTRACK *tr;
	for( tr=h->track; tr; tr=tr->next ) {
		mid_sync_track(tr, h->tracktime);
		if( tr->chan == mch && tr->vpos == pos )
			return tr;
	}
	return NULL;
}

// =====================================================================================
static MIDTRACK *mid_locate_track(MIDHANDLE *h, int mch, int pos)
// =====================================================================================
{
	MIDTRACK *tr, *prev, *trunused;
	MIDEVENT *e;
	int instrno = 1;
	int polyphony;
	int vol = 0, bal = 0;
	int numtracks;
	ULONG tmin;
	prev = NULL;
	trunused = NULL;
	polyphony = 0;
	numtracks = 0;
	tmin = h->midispeed; // minimal distance between note events in track
	// look up track with desired channel and pos (note)
	for( tr=h->track; tr; tr=tr->next ) {
		mid_sync_track(tr, h->tracktime);
		if( tr->chan == mch ) {
			if( tr->vpos == pos )
				return tr;
			if( tr->vpos == 0xff ) {
				// check if track with silence is quiet long enough
				if( h->tracktime > tr->vtracktick + tmin ) trunused = tr;
			}
			else vol = tr->volume;
			instrno = tr->instr;
			bal = tr->balance;
			polyphony++;
		}
		numtracks++;
		prev = tr;
	}
	if( trunused ) {
		trunused->vpos = pos;
		return trunused;
	}
	if( polyphony > MAX_POLYPHONY || (polyphony > 0 && numtracks > MAX_TRACKS) ) { // do not use up too much channels
		for( tr=h->track; tr; tr=tr->next ) {
			if( tr->chan == mch ) {
				e = tr->workevent;
				if( h->tracktime > e->tracktick + tmin ) {
					tmin = h->tracktime - e->tracktick;
					trunused = tr;
				}
			}
		}
		if( trunused ) {
			trunused->vpos = pos;
			return trunused;
		}
	}
	if( numtracks > MAX_TRACKS ) { // we can not allocate new tracks
		tmin = 0;
		for( tr=h->track; tr; tr=tr->next ) {
			if( tr->chan == mch ) {
				e = tr->workevent;
				if( h->tracktime >= e->tracktick + tmin ) {
					tmin = h->tracktime - e->tracktick;
					trunused = tr;
				}
			}
		}
		if( trunused ) {
			trunused->vpos = pos;
			return trunused;
		}
		tmin = 0;
		for( tr=h->track; tr; tr=tr->next ) {
			e = tr->workevent;
			if( h->tracktime >= e->tracktick + tmin ) {
				tmin = h->tracktime - e->tracktick;
				trunused = tr;
			}
		}
		if( trunused ) {
			trunused->vpos = pos;
			trunused->chan = mch;
			return trunused;
		}
	}
	tr = mid_new_track(h, mch, pos);
	tr->instr  = instrno;
	tr->volume = vol;
	tr->balance = bal;
	if( prev ) prev->next = tr;
	else	h->track = tr;
	return tr;
}

static void	mid_add_event(MIDHANDLE *h, MIDTRACK *tp, MIDEVENT *e)
{
	MIDEVENT *ew, *ep;
	ep = NULL;
	ew = tp->workevent;
	if( ew && ew->tracktick > e->tracktick ) ew = tp->head; // start again from the beginning...
	for( ; ew && ew->tracktick <= e->tracktick; ew = ew->next ) {
		ep = ew;
		tp->workevent = ew;
		mid_update_track(tp);
	}
	if( ep ) {
		ep->next = e;
		e->next = ew;
	}
	else {
		e->next = tp->head;
		tp->head = e;
	}
	if( !e->next )
		tp->tail = e;
	tp->workevent = e;
	mid_update_track(tp);
}

static void mid_add_tempo_event(MIDHANDLE *h, int tempo)
{
	MIDEVENT *e;
	e = mid_new_event(h);
	e->flg = 0;
	e->fx = tmpo;
	e->fxparam = tempo;
	mid_add_event(h, h->track, e);
}

static void mid_add_partbreak(MIDHANDLE *h)
{
	MIDEVENT *e;
	e = mid_new_event(h);
	e->flg = 0;
	e->fx = fxbrk;
	mid_add_event(h, h->track, e);
}

static void mid_add_noteoff(MIDHANDLE *h, MIDTRACK *tp)
{
	MIDEVENT *e;
	e = mid_new_event(h);
	e->flg = 1;
	e->note = tp->vpos;
	e->smpno = tp->instr;
	mid_add_event(h, tp, e);
}

static void mid_add_noteon(MIDHANDLE *h, MIDTRACK *tp, int n, int vol)
{
	MIDEVENT *e;
	e = mid_new_event(h);
	e->flg = 1;
	e->note = n;
	e->smpno = tp->instr;
	e->volume = vol;
	mid_add_event(h, tp, e);
}

static BYTE modtremolo(int midimod)
{
	int m;
	if( midimod == 0 ) return 0;
	if( midimod > 63 ) {
		m = (128 - midimod) / 4;
		if( m==0 ) m = 1;
		return m|0xf0; // find slide down
	}
	m = midimod / 4;
	if( m==0 ) m = 1;
	return (m<<4)|0x0f; // find slide up
}

// =====================================================================================
static void mid_mod_wheel(MIDHANDLE *h, int mch, int mod)
// =====================================================================================
{
	MIDTRACK *tr;
	MIDEVENT *e;
	for( tr=h->track; tr; tr=tr->next ) {
		if( tr->chan == mch ) {
			mid_sync_track(tr, h->tracktime);
			if( tr->vpos != 0xff ) { // only on tracks with notes on...
				e = mid_new_event(h);
				e->flg = 0;
				e->fx = modwheel;
				e->fxparam = modtremolo(mod);
				mid_add_event(h, tr, e);
			}
		}
	}
}

// =====================================================================================
static void mid_main_volume(MIDHANDLE *h, int mch, int vol)
// =====================================================================================
{
	MIDTRACK *tr;
	MIDEVENT *e;
	for( tr=h->track; tr; tr=tr->next ) {
		if( tr->chan == mch ) {
			e = mid_new_event(h);
			e->flg = 0;
			e->fx = mainvol;
			e->fxparam = vol;
			mid_add_event(h, tr, e);
		}
	}
}

// transform 0..63..127 to left..center..right in 2n+1 areas
static int modpan(int midipan, int n)
{
	int npan, area, x;
	x    = 2 * n + 1;
	area = (midipan * x * (PAN_RIGHT - PAN_LEFT))>>7;
	npan = (PAN_LEFT * x + area) / x;
	return npan;
}

// =====================================================================================
static void mid_pan(MIDHANDLE *h, int mch, int pan)
// =====================================================================================
{
	MIDTRACK *tr;
	int hits;
	hits = 0;
	for( tr=h->track; tr; tr=tr->next ) {
		if( tr->chan == mch ) {
			hits++;
			tr->balance = pan;
		}
	}
	if( !hits ) {
		tr = mid_locate_track(h, mch, 0xff);
		tr->balance = pan;
	}
}

// =====================================================================================
static void mid_add_program(MIDHANDLE *h, int mch, int pr)
// =====================================================================================
{
	MIDTRACK *tr;
	MIDEVENT *e;
	int hits;
	hits = 0;
	for( tr=h->track; tr; tr=tr->next ) {
		if( tr->chan == mch ) {
			hits++;
			e = mid_new_event(h);
			e->flg = 0;
			e->fx = prog;
			e->fxparam = pat_gmtosmp(pr + 1);
			mid_add_event(h, tr, e);
		}
	}
	if( !hits ) {
		tr = mid_locate_track(h, mch, 0xff);
		e = mid_new_event(h);
		e->flg = 0;
		e->fx = prog;
		e->fxparam = pat_gmtosmp(pr + 1);
		mid_add_event(h, tr, e);
	}
}

// =====================================================================================
static void mid_all_notes_off(MIDHANDLE *h, int mch)
// =====================================================================================
{
	MIDTRACK *tr;
	if( h->debug ) printf("%ld %d all notes off\n",(long)(h->tracktime), mch+1);
	for( tr=h->track; tr; tr=tr->next ) {
		if( tr->chan == mch || mch == -1 ) {
			mid_sync_track(tr, h->tracktime);
			if( tr->vpos != 0xff )
				mid_add_noteoff(h, tr);
		}
	}
}

#ifndef NEWMIKMOD
static void mid_add_sync(MIDHANDLE *h, MIDTRACK *tp)
{
	MIDEVENT *e;
	e = mid_new_event(h);
	e->flg = 0;
	e->fx = fxsync;
	mid_add_event(h, tp, e);
}
#endif

static BYTE mid_to_mod_wheel(unsigned int midwheel)
{
	unsigned int i;
	if( midwheel == 0 ) return 0;
	i = midwheel >> WHEELSHIFT;
	return i+1;
}

static void mid_add_wheel(MIDHANDLE *h, MIDTRACK *tp, int wheel)
{
	MIDEVENT *e;
	e = mid_new_event(h);
	e->flg = 0;
	if( wheel < 0 ) {
		e->fx = wheeldown;
		e->fxparam = mid_to_mod_wheel(-wheel);
	}
	else {
		e->fx = wheelup;
		e->fxparam = mid_to_mod_wheel(wheel);
	}
	mid_add_event(h, tp, e);
}

static void mid_add_pitchwheel(MIDHANDLE *h, int mch, int wheel)
{
	MIDTRACK *tr;
	int hits;
	hits = 0;
	for( tr=h->track; tr; tr=tr->next ) {
		if( tr->chan == mch ) {
			hits++;
			mid_sync_track(tr, h->tracktime);
			if( tr->vpos != 0xff ) // only on tracks with notes on...
				mid_add_wheel(h, tr, wheel);
		}
	}
	if( !hits ) { // special case in midiformat 1 events in first track...
		tr = mid_locate_track(h, mch, 0xff);
		mid_add_wheel(h, tr, wheel);
	}
}

static long int mid_read_long(MIDHANDLE *h)
{
	BYTE buf[4];
	mmreadUBYTES(buf, 4, h->mmf);
	return (buf[0]<<24)|(buf[1]<<16)|(buf[2]<<8)|buf[3];
}

static short int mid_read_short(MIDHANDLE *h)
{
	BYTE buf[2];
	mmreadUBYTES(buf, 2, h->mmf);
	return (buf[0]<<8)|buf[1];
}

static BYTE mid_read_byte(MIDHANDLE *h)
{
	return mmreadUBYTE(h->mmf);
}

static int mid_read_delta(MIDHANDLE *h)
{
	BYTE bits;
	int i, d;
	d = 0;
	for( i=0; i<4; ) {
		bits = mid_read_byte(h);
		i++;
		d = (d<<7)|(bits&0x7f);
		if( !(bits & 0x80) )
			break;
	}
	h->deltatime = d;
	return i;
}

// =====================================================================================
#ifdef NEWMIKMOD
BOOL MID_Test(MMSTREAM *mmfile)
#else
BOOL CSoundFile::TestMID(const BYTE *lpStream, DWORD dwMemLength)
#endif
// =====================================================================================
{
	char id[5];
	MIDHANDLE h;
#ifdef NEWMIKMOD
	h.mmf = mmfile;
#else
	MMFILE mm;
	mm.mm = (char *)lpStream;
	mm.sz = dwMemLength;
	h.mmf = &mm;
#endif
	mmfseek(h.mmf,0,SEEK_SET);
	mmreadSBYTES(id, 4, h.mmf);
	id[4] = '\0';
	return !strcmp(id,"MThd") && mid_read_long(&h) == 6;
}

// =====================================================================================
static MIDHANDLE *MID_Init(void)
{
	MIDHANDLE *retval;
#ifdef NEWMIKMOD
	MM_ALLOC *allochandle;
	
	allochandle = _mmalloc_create("Load_MID", NULL);
	retval = (MIDHANDLE *)_mm_calloc(allochandle, 1,sizeof(MIDHANDLE));
	if( !retval ) return NULL;
	retval->allochandle = allochandle;
	allochandle = _mmalloc_create("Load_ABC_tracks", NULL);
	retval->trackhandle = allochandle;
#else
	retval = (MIDHANDLE *)calloc(1,sizeof(MIDHANDLE));
	if( !retval ) return NULL;
#endif
	retval->track      = NULL;
	retval->percussion = 0;
	retval->debug      = NULL;
	return retval;
}

#ifndef NEWMIKMOD
static void MID_CleanupTrack(MIDTRACK *tp)
{
	MIDEVENT *ep, *en;
	if( tp ) {
		for( ep=tp->head; ep; ep = en ) {
			en=ep->next;
			free(ep);
		}
		tp->head = NULL;
	}
}
#endif

// =====================================================================================
static void MID_CleanupTracks(MIDHANDLE *handle)
// =====================================================================================
{
#ifdef NEWMIKMOD
	if(handle && handle->trackhandle) {
		_mmalloc_close(handle->trackhandle);
		handle->trackhandle = 0;
	}
#else
	MIDTRACK *tp, *tn;
	if(handle) {
		for( tp=handle->track; tp; tp = tn ) {
			tn=tp->next;
			MID_CleanupTrack(tp);
		}
		handle->track = NULL;
	}
#endif
}

// =====================================================================================
static void MID_Cleanup(MIDHANDLE *handle)
// =====================================================================================
{
#ifdef NEWMIKMOD
	if(handle && handle->allochandle) {
		MID_CleanupTracks(handle);
		_mmalloc_close(handle->allochandle);
		handle->allochandle = 0;
	}
#else
	if(handle) {
		MID_CleanupTracks(handle);
		free(handle);
		handle = 0;
	}
#endif
}

static int mid_is_global_event(MIDEVENT *e)
{
	return (e->fx == tmpo || e->fx == fxbrk); 
}

static MIDEVENT *mid_next_global(MIDEVENT *e)
{
	for( ; e && !mid_is_global_event(e); e=e->next ) ;
	return e;
}

static MIDEVENT *mid_next_fx(MIDEVENT *e)
{
	for( ; e && e->fx == none; e=e->next ) ;
	return e;
}

static int mid_is_note_event(MIDEVENT *e)
{
#ifdef LOOPED_NOTES_OFF
	return (e->flg == 0);
#else
	if( e->flg == 0 ) return 0;
	if( e->volume ) return 1;
	return pat_smplooped(e->smpno); // let non looping samples die out...
#endif
}

static MIDEVENT *mid_next_note(MIDEVENT *e)
{
	for( ; e && !mid_is_note_event(e); e=e->next ) ;
	return e;
}

// =====================================================================================
#ifdef NEWMIKMOD
static void MID_ReadPatterns(UNIMOD *of, MIDHANDLE *h, int numpat)
// =====================================================================================
{
	int pat,row,i,ch,trkset;
	BYTE n,ins,vol;
	MIDTRACK *t;
	MIDEVENT *e, *en, *ef, *el;
	ULONG tt1, tt2;
	UNITRK_EFFECT eff;

	// initialize start points of event list in tracks
	for( t = h->track; t; t = t->next ) t->workevent = t->head;
	for( pat = 0; pat < numpat; pat++ ) {
		utrk_reset(of->ut);
		for( row = 0; row < 64; row++ ) {
			tt1 = miditicks(h, (pat * 64 + row ) * h->speed);
			tt2 = tt1 + h->midispeed;
			for( e=mid_next_global(h->track->workevent); e && e->tracktick < tt2; e=mid_next_global(e->next) ) {
				if( e && e->tracktick >= tt1 ) {	// we have a controller event in this row
					switch( e->fx ) {
						case tmpo:
							eff.effect   = UNI_GLOB_TEMPO;
							eff.param.u  = e->fxparam;
							eff.framedly = UFD_RUNONCE;
							utrk_write_global(of->ut, &eff, PTMEM_TEMPO);
							break;
						case fxbrk:
							eff.effect   = UNI_GLOB_PATBREAK;
							eff.param.u  = 0;
							eff.framedly = UFD_RUNONCE;
							utrk_write_global(of->ut, &eff, UNIMEM_NONE);
							break;
					}
				}
			}
			ch = 0;
			for( t = h->track; t; t = t->next ) {
				trkset = 0;
				e = NULL;
				for( el=mid_next_fx(t->workevent); el && el->tracktick < tt2; el=mid_next_fx(el->next) ) {
					if( el && el->tracktick >= tt1 ) {
						switch( el->fx ) {
							case modwheel:
							case wheelup:
							case wheeldown:
								e = el;
							default:
								break;
						}
					}
				}
				if( e )	{	// we have a controller event in this row
					switch( e->fx ) {
						case modwheel:
							if( !trkset ) {
								utrk_settrack(of->ut, ch);
								trkset = 1;
							}
							eff.effect   = UNI_VOLSLIDE;
							eff.framedly = UFD_RUNONCE;
							if( (e->fxparam & 0x0f) == 0x0f )
								eff.param.s = (e->fxparam >> 3)&0x1f;
							else
								eff.param.s = -((e->fxparam & 0x0f)*2);
							utrk_write_local(of->ut, &eff, STMEM_VOLSLIDE);
							break;
						case wheelup:
							if( !trkset ) {
								utrk_settrack(of->ut, ch);
								trkset = 1;
							}
							eff.effect   = UNI_PITCHSLIDE;
							eff.framedly = UFD_RUNONCE;
							eff.param.s = e->fxparam;
							utrk_write_local(of->ut, &eff, STMEM_PITCHSLIDE);
							break;
						case wheeldown:
							if( !trkset ) {
								utrk_settrack(of->ut, ch);
								trkset = 1;
							}
							eff.effect   = UNI_PITCHSLIDE;
							eff.framedly = UFD_RUNONCE;
							eff.param.s = -(int)(e->fxparam);
							utrk_write_local(of->ut, &eff, STMEM_PITCHSLIDE);
							break;
					}
				}
				for( e=mid_next_note(t->workevent); e && e->tracktick < tt1; e=mid_next_note(e->next) )
					t->workevent = e;
				i = 0;
				ef = NULL;
				en = e;
				el = e;
				for( ; e && e->tracktick < tt2; e=mid_next_note(e->next) ) {	// we have a note event in this row
					t->workevent = e;
					i++;
					if( e->volume ) {
						if( !ef ) ef = e;
						el = e;
					}
				}
				if( i ) {
					if( !trkset ) {
						utrk_settrack(of->ut, ch);
						trkset = 1;
					}
					if( i == 1 || ef == el || !ef ) { // only one event in this row
						if( ef ) e = ef;
						else e = en;
						el  = t->workevent;
						n   = pat_modnote(e->note);
						ins = e->smpno;
						eff.framedly = modticks(h, e->tracktick - tt1);
						eff.param.u      = 0;
						eff.param.byte_a = n;
						eff.param.byte_b = ins;
						vol = e->volume;
						if( vol == 0 ) {
							eff.effect  = UNI_NOTEKILL;
							utrk_write_local(of->ut, &eff, UNIMEM_NONE);
						}
						else {
							if( el->volume == 0 ) {
								eff.framedly     = modticks(h, el->tracktick - tt1);
								eff.effect       = UNI_NOTEKILL;
								utrk_write_local(of->ut, &eff, UNIMEM_NONE);
							}
							else {
								if( eff.framedly ) {
									eff.effect       = UNI_NOTEDELAY;
									utrk_write_local(of->ut, &eff, UNIMEM_NONE);
								}
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
						n   = pat_modnote(ef->note);
						i   = pat_modnote(el->note);
						ins = el->smpno;
						vol = el->volume;
						eff.effect    = UNI_PITCHSLIDE;
						eff.framedly  = modticks(h, el->tracktick - tt1)|UFD_RUNONCE;
						eff.param.s   = ((i > n)?i-n:n-i);
						utrk_write_inst(of->ut, ins);
						utrk_write_note(of->ut, n); // <- normal note
						pt_write_effect(of->ut, 0xc, vol);
						utrk_write_local(of->ut, &eff, (i > n)? PTMEM_PITCHSLIDEUP: PTMEM_PITCHSLIDEDN);
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

static int MID_ReadPatterns(MODCOMMAND *pattern[], WORD psize[], MIDHANDLE *h, int numpat, int channels)
// =====================================================================================
{
	int pat,row,i,ch;
	BYTE n,ins,vol;
	MIDTRACK *t;
	MIDEVENT *e, *en, *ef, *el;
	ULONG tt1, tt2;
	MODCOMMAND *m;
	int patbrk, tempo;
	if( numpat > MAX_PATTERNS ) numpat = MAX_PATTERNS;

	// initialize start points of event list in tracks
	for( t = h->track; t; t = t->next ) t->workevent = t->head;
	for( pat = 0; pat < numpat; pat++ ) {
		pattern[pat] = CSoundFile::AllocatePattern(64, channels);
		if( !pattern[pat] ) return 0;
		psize[pat] = 64;
		for( row = 0; row < 64; row++ ) {
			tt1 = miditicks(h, (pat * 64 + row ) * h->speed);
			tt2 = tt1 + h->midispeed;
			ch = 0;
			tempo = 0;
			patbrk = 0;
			for( e=mid_next_global(h->track->workevent); e && e->tracktick < tt2; e=mid_next_global(e->next) ) {
				if( e && e->tracktick >= tt1 ) {	// we have a controller event in this row
					switch( e->fx ) {
						case tmpo:
							tempo = e->fxparam;
							break;
						case fxbrk:
							patbrk = 1;
							break;
					}
				}
			}
			for( t = h->track; t; t = t->next ) {
				m = &pattern[pat][row * channels + ch];
				m->param   = 0;
				m->command = CMD_NONE;
				for( e=mid_next_fx(t->workevent); e && e->tracktick < tt2; e=mid_next_fx(e->next) ) {
					if( e && e->tracktick >= tt1 ) {	// we have a controller event in this row
						switch( e->fx ) {
							case modwheel:
								m->param   = e->fxparam;
								m->command = CMD_VOLUMESLIDE;
								break;
							case wheelup:
								m->param   = e->fxparam|0x10;
								m->command = CMD_XFINEPORTAUPDOWN;
								break;
							case wheeldown:
								m->param   = e->fxparam|0x20;
								m->command = CMD_XFINEPORTAUPDOWN;
								break;
						}
					}
				}
				for( e=mid_next_note(t->workevent); e && e->tracktick < tt1; e=mid_next_note(e->next) )
					t->workevent = e;
				i = 0;
				ef = NULL;
				en = e;
				el = e;
				for( ; e && e->tracktick < tt2; e=mid_next_note(e->next) ) {	// we have a note event in this row
					t->workevent = e;
					i++;
					if( e->volume ) {
						if( !ef ) ef = e;
						el = e;
					}
				}
				if( i ) {
					if( i == 1 || ef == el || !ef ) { // only one event in this row or a note on with some note off
						if( ef ) e = ef;
						else e = en;
						el = t->workevent;
						n   = pat_modnote(e->note);
						ins = e->smpno;
						if( e->volume == 0 ) {
							m->param = (BYTE)modticks(h, e->tracktick - tt1);
							if( m->param ) { // note cut
								m->command = CMD_S3MCMDEX;
								m->param  |= 0xC0;
							}
							else {
								m->param   = 0;
								m->command = CMD_KEYOFF;
							}
							vol = 0;
						}
						else {
							vol = e->volume/2;
							if( el->volume == 0 ) {
								m->param = (BYTE)modticks(h, el->tracktick - tt1);
								if( m->param ) { // note cut
									m->command = CMD_S3MCMDEX;
									m->param  |= 0xC0;
								}
							}
							else {
								m->param = (BYTE)modticks(h, e->tracktick - tt1);
								if( m->param ) { // note delay
									m->command = CMD_S3MCMDEX;
									m->param  |= 0xD0;
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
						n   = pat_modnote(ef->note);
						i   = pat_modnote(el->note);
						ins = el->smpno;
						vol = el->volume/2;
						if( vol > 64 ) vol = 64;
						m->instr  = ins;
						m->note   = n; // <- normal note
						m->volcmd = VOLCMD_VOLUME;
						m->vol    = vol;
						m->param  = ((i > n)?i-n:n-i);
						if( m->param < 16 ) {
							if( m->param ) {
								m->command = CMD_XFINEPORTAUPDOWN;
								m->param |= (i > n)? 0x10: 0x20;
							}
							else {	// retrigger same note...
								m->command = CMD_RETRIG;
								m->param = (BYTE)modticks(h, el->tracktick - tt1);
							}
						}
						else
							m->command = (i > n)? CMD_PORTAMENTOUP: CMD_PORTAMENTODOWN;
					}
				}
				if( m->param == 0 && m->command == CMD_NONE ) {
					if( tempo ) {
						m->command = CMD_TEMPO;
						m->param   = tempo;
						tempo = 0;
					}
					else {
						if( patbrk ) {
							m->command = CMD_PATTERNBREAK;
							patbrk = 0;
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

static ULONG mid_next_tracktick(MIDEVENT *e)
{
	MIDEVENT *en;
	en = e->next;
	if( en ) return en->tracktick;
	return 0x7fffffff; // practically indefinite
}

// cut off alle events that follow the given event
static void mid_stripoff(MIDTRACK *tp, MIDEVENT *e)
{
#ifndef NEWMIKMOD
	MIDEVENT *ep, *en;
	for( ep=e->next; ep; ep = en ) {
		en=ep->next;
		free(ep);
	}
#endif
	e->next  = NULL;
	tp->tail = e;
	tp->workevent = tp->head;
	mid_sync_track(tp, e->tracktick);
}

static void mid_notes_to_percussion(MIDTRACK *tp, ULONG adjust, ULONG tmin)
{
	MIDEVENT *e, *lno = 0;
	int n = 0,v;
	ULONG ton, toff = 0, tnext;
	v = 0x7f; // as loud as it gets
	ton = 0;
	for( e=tp->head; e; e=e->next ) {
		if( e->tracktick < adjust ) e->tracktick = 0;
		else e->tracktick -= adjust;
		if( e->flg == 1 ) {
			if( e->volume > 0 ) {
				n = e->note;
				e->smpno = pat_gmtosmp(pat_gm_drumnr(n));
				e->note = pat_gm_drumnote(n);
				e->volume = (v * e->volume) / 128;
				if( v && !e->volume ) e->volume = 1;
				ton = e->tracktick;
			}
			else {
				toff = ton + tmin;
				if( toff > e->tracktick ) {
					tnext = mid_next_tracktick(e);
					if( toff + tmin < tnext ) e->tracktick = toff;
					else {
						if( toff < tnext ) e->tracktick = toff - 1;
						else e->tracktick = tnext - 1;
					}
				}
				toff = e->tracktick;
				lno = e;
			}
		}
		else {
			if( e->fx == mainvol ) {
				v = e->fxparam;
				if( !v && ton > toff ) {
					e->flg = 1;
					e->volume = 0;
					e->note = pat_gm_drumnote(n);
					toff = e->tracktick;
					lno = e;
				}
			}
		}
	}
	if( ton > toff ) {
		char info[32];
		sprintf(info,"%ld > %ld note %d", (long)ton, (long)toff, n);
		mid_message("drum track ends with note on (%s)", info);
	}
	if( lno && lno->next ) mid_stripoff(tp, lno);
}

static void mid_prog_to_notes(MIDTRACK *tp, ULONG adjust, ULONG tmin)
{
	MIDEVENT *e, *lno = 0;
	int i = 0, n = 0, v = 0x7f;
	ULONG ton, toff = 0, tnext;
	ton = 0;
	for( e=tp->head; e; e=e->next ) {
		if( e->tracktick < adjust ) e->tracktick = 0;
		else e->tracktick -= adjust;
		if( e->flg == 1 ) {
			if( !i ) i = pat_gmtosmp(1); // happens in eternal2.mid
			e->smpno = i;
			n = e->note;
			if( e->volume > 0 ) {
				e->volume = (v * e->volume) / 128;
				if( v && !e->volume ) e->volume = 1;
				ton = e->tracktick;
			}
			else {
				toff = ton + tmin;
				if( toff > e->tracktick ) {
					tnext = mid_next_tracktick(e);
					if( toff + tmin < tnext ) e->tracktick = toff;
					else {
						if( toff < tnext ) e->tracktick = toff - 1;
						else e->tracktick = tnext - 1;
					}
				}
				toff = e->tracktick;
				lno = e;
			}
		}
		else {
			if( e->fx == prog ) i = e->fxparam;
			if( e->fx == mainvol ) {
				v = e->fxparam;
				if( !v && ton > toff ) {
					e->flg = 1;
					e->volume = 0;
					e->note = n;
					toff = e->tracktick;
					lno = e;
				}
			}
		}
	}
	if( ton > toff ) {
		char info[40];
		sprintf(info,"channel %d, %ld > %ld note %d", tp->chan + 1, (long)ton, (long)toff, n);
		mid_message("melody track ends with note on (%s)", info);
	}
	if( lno && lno->next ) mid_stripoff(tp, lno);
}

static int midiword(BYTE *b)
{
	int i;
	i = (b[0]&0x7f)|((b[1]&0x7f)<<7);
	return i;
}

static int midishort(BYTE *b)
{
	return midiword(b) - 0x2000;
}

ULONG mid_first_noteonevent_tick(MIDEVENT *e)
{
	while( e && (e->flg == 0 || e->volume == 0) ) e=e->next;
	if( !e ) return 0x7fffffff;
	return e->tracktick;
}

// =====================================================================================
#ifdef NEWMIKMOD
BOOL MID_Load(MIDHANDLE *h, UNIMOD *of, MMSTREAM *mmfile)
#else
BOOL CSoundFile::ReadMID(const BYTE *lpStream, DWORD dwMemLength)
#endif
{
	static int avoid_reentry = 0;
#ifdef NEWMIKMOD
#define m_nDefaultTempo	of->inittempo
#else
	MIDHANDLE *h;
	MMFILE mm;
#endif
	int ch, dmulti, maxtempo, panlow, panhigh, numchans, numtracks;
	MIDTRACK *ttp;
	uint32_t t, numpats;
	char buf[256];
	long miditracklen;
	BYTE runningstatus;
	BYTE cmd;
	BYTE midibyte[2];
	long metalen, delta;
	BYTE *p;
	while( avoid_reentry ) sleep(1);
	avoid_reentry = 1;
#ifdef NEWMIKMOD
	h->mmf = mmfile;
#else
	if( !TestMID(lpStream, dwMemLength) ) {
		avoid_reentry = 0;
		return FALSE;
	}
	h = MID_Init();
	if( !h ) {
		avoid_reentry = 0;
		return FALSE;
	}
	h->mmf = &mm;
	mm.mm = (char *)lpStream;
	mm.sz = dwMemLength;
	mm.pos = 0;
#endif
	h->debug = getenv(ENV_MMMID_DEBUG);
	h->verbose = getenv(ENV_MMMID_VERBOSE);
	pat_resetsmp();
	pat_init_patnames();
	mmfseek(h->mmf,8,SEEK_SET);
	h->midiformat	= mid_read_short(h);
	h->miditracks = mid_read_short(h);
	h->resolution = mid_read_short(h);
	// at this point the h->mmf is positioned at first miditrack
	if( h->midiformat == 0 ) h->miditracks = 1;
	if( h->resolution & 0x8000 )
		h->divider = ((h->resolution & 0x7f00)>>8)*(h->resolution & 0xff);
	else
		h->divider = h->resolution;
	h->divider <<= 2; // ticks per quartnote ==> ticks per note
	h->tempo = 122;
	m_nDefaultTempo = 0;
	h->tracktime = 0;
	h->speed = 6;
	p = (BYTE *)getenv(ENV_MMMID_SPEED);
	if( p && isdigit(*p) && p[0] != '0' && p[1] == '\0' ) {
		// transform speed
		t = *p - '0';
		h->speed *= t;
		h->divider *= t;
		h->speed /= 6;
		h->divider /= 6;
	}
	// calculate optimal delta multiplier dmulti keeping tempo adjustments
	// from 10 to 255 in mind (hoping there will be no midi's with tempo's
	// lower than 10, that is sooo sick...)
	// this is necessary for the tracks to patterns routine
	dmulti = 1;
	maxtempo = h->divider;
	while( (h->midispeed = miditicks(h, h->speed)) * 10 < 255 * h->speed ) {
		++dmulti;
		h->divider = maxtempo * dmulti;
	}
	h->tp = NULL;
	memset(buf,0,sizeof(buf));
#ifdef NEWMIKMOD
	of->songname = NULL;
#else
	strcpy(m_szNames[0], "");
#endif
	maxtempo = 0;
	panlow   = 64;
	panhigh  = 64;
	if( h->verbose ) {
		printf("Scanning MIDI with format: %d resolution: %d tracks: %d\n",
			h->midiformat,
			h->resolution,
			h->miditracks);
	}
	if( h->verbose && dmulti > 1 ) {
		printf("Multiplying resolution and deltatimes by %d to get %d miditicks per patternrow\n",
			dmulti, h->midispeed);
	}
	for( t=0; t<(uint32_t)h->miditracks; t++ ) {
		if( h->verbose ) printf("Parsing track %d\n", t+1);
		mmreadSBYTES(buf,4,h->mmf);
		buf[4] = '\0';
		if( strcmp(buf,"MTrk") ) {
			mid_message("invalid track-chunk '%s' is not 'MTrk'",buf);
			avoid_reentry = 0;
			return FALSE;
		}
		miditracklen = mid_read_long(h);
		runningstatus = 0;
		if( t && h->midiformat == 1 ) mid_rewind_tracks(h); // tracks sound simultaneously
		while( miditracklen > 0 ) {
			miditracklen -= mid_read_delta(h);
			midibyte[0] = mid_read_byte(h);
			miditracklen--;
			if( midibyte[0] & 0x80 ) {
				runningstatus = midibyte[0];
				switch( runningstatus ) {
					case 0xf1:
					case 0xf4:
					case 0xf5:
					case 0xf6:
					case 0xf7:
					case 0xf8:
					case 0xf9:
					case 0xfa:
					case 0xfb:
					case 0xfc:
					case 0xfd:
					case 0xfe:
						break;
					default:
						midibyte[0] = mid_read_byte(h);
						miditracklen--;
						break;
				}
			}
			h->tracktime += dmulti * h->deltatime;
			ch = runningstatus & 0x0f;
			cmd = runningstatus & 0xf0;
			switch( cmd ) {
				case 0x80: // note off
					midibyte[1] = mid_read_byte(h);
					miditracklen--;
					ttp = mid_find_track(h, ch, midibyte[0]);
					if( ttp ) mid_add_noteoff(h, ttp);
					if( h->debug )
						printf("%2d %08ld       Note off: ch %d 0x%02x 0x%02x\n",
						       t, (long)(h->tracktime),
						       ch + 1, midibyte[0], midibyte[1]);
					break;
				case 0x90: // note on
					midibyte[1] = mid_read_byte(h);
					miditracklen--;
					if( midibyte[1] ) {
						ttp = mid_locate_track(h, ch, midibyte[0]);
						mid_add_noteon(h, ttp, midibyte[0], midibyte[1]);
						if( h->debug )
							printf("%2d %08ld Note  on: ch %d 0x%02x 0x%02x\n",
							       t, (long)(h->tracktime),
							       ch + 1, midibyte[0], midibyte[1]);
					}
					else {
						ttp = mid_find_track(h, ch, midibyte[0]);
						if( ttp ) mid_add_noteoff(h, ttp);
						if( h->debug )
							printf("%2d %08ld note off: ch %d 0x%02x\n",
							t, (long)(h->tracktime),
							ch + 1, midibyte[0]);
					}
					break;
				case 0xa0: // polyphonic key pressure
					midibyte[1] = mid_read_byte(h);
					miditracklen--;
					if( h->debug )
						printf("%2d %08ld polyphonic key pressure: ch %d 0x%02x 0x%02x\n", t, (long)(h->tracktime), ch + 1, midibyte[0], midibyte[1]);
					break;
				case 0xb0: // control change
					midibyte[1] = mid_read_byte(h);
					miditracklen--;
					switch(midibyte[0]) {
						case 0x01: // mod wheel
							mid_mod_wheel(h, ch, midibyte[1]);
							break;
						case 0x07: // main volume
							mid_main_volume(h, ch, midibyte[1]);
							break;
						case 0x0a: // pan
							if( midibyte[1] < panlow )  panlow  = midibyte[1];
							if( midibyte[1] > panhigh ) panhigh = midibyte[1];
							mid_pan(h, ch, midibyte[1]);
							break;
						case 0x0b: // expression
							break;
						case 0x7b: 
							if( midibyte[1] == 0x00 ) // all notes off
								mid_all_notes_off(h, ch);
							break;
						default:
							break;
					}
					if( h->debug )
						printf("%2d %08ld control change: ch %d 0x%02x 0x%02x\n",
						t, (long)(h->tracktime), ch + 1, midibyte[0], midibyte[1]);
					break;
				case 0xc0: // program change
					mid_add_program(h, ch, midibyte[0]);
					if( h->debug )
						printf("%2d %08ld program change: ch %d %d\n",
						t, (long)(h->tracktime), ch + 1, midibyte[0]);
					break;
				case 0xd0: // channel pressure
					if( h->debug )
						printf("%2d %08ld channel pressure: ch %d 0x%02x\n", t, (long)(h->tracktime), ch + 1, midibyte[0]);
					break;
				case 0xe0: // pitch wheel change
					midibyte[1] = mid_read_byte(h);
					miditracklen--;
					if( h->debug )
						printf("%2d %08ld pitch wheel change: ch %d %d\n",
						t, (long)(h->tracktime), ch + 1, midishort(midibyte));
					mid_add_pitchwheel(h, ch, midishort(midibyte));
					break;
				case 0xf0: // system & realtime
					switch( runningstatus ) {
						case 0xf0:	// sysex
							if( h->debug ) printf("%2d %08ld sysex: 0x%02x",
								t, (long)(h->tracktime), midibyte[0]);
							while( midibyte[0] != 0xf7 ) {
								midibyte[0] = mid_read_byte(h);
								miditracklen--;
								if( h->debug ) printf(" %02X", midibyte[0]);
							}
							if( h->debug ) printf("\n");
							break;
						case 0xf2:	// song position pointer
							midibyte[1] = mid_read_byte(h);
							miditracklen--;
							if( h->debug )
								printf("%2d %08ld song position pointer: %d", 
								t, (long)(h->tracktime), midishort(midibyte));
							break;
						case 0xf7:
							delta = h->deltatime;
							miditracklen -= mid_read_delta(h);
							metalen = h->deltatime;
							if( h->debug )
								printf("%2d %08ld sysex continued: %ld",
								t, (long)(h->tracktime), metalen);
							while( metalen > 0 ) {
								midibyte[1] = mid_read_byte(h);
								metalen--;
								miditracklen--;
								if( h->debug ) printf(" %02X", midibyte[1]);
							}
							h->deltatime = delta;
							break;
						case 0xff: // meta event
							delta = h->deltatime;
							miditracklen -= mid_read_delta(h);
							metalen = h->deltatime;
							if( metalen > 31 ) metalen = 31;
							if( metalen ) {
								mmreadSBYTES(buf, metalen, h->mmf);
								miditracklen -= metalen;
							}
							buf[metalen] = '\0';
							metalen = h->deltatime - metalen;
							while( metalen > 0 ) {
								midibyte[1] = mid_read_byte(h);
								metalen--;
								miditracklen--;
							}
							h->deltatime = delta;
							switch( midibyte[0] ) {
								case 0x03: // type: track name
									if( h->debug )
										printf("%2d %08ld META trackname:%s\n", t, (long)(h->tracktime), buf);
#ifdef NEWMIKMOD
									if( !of->songname )
										of->songname = DupStr(of->allochandle, buf, strlen(buf));
#else
									if( m_szNames[0][0] == '\0' )
										strcpy(m_szNames[0], buf);
#endif
									break;
								case 0x51: // type: tempo
									p=(BYTE *)buf;
									delta = (p[0]<<16)|(p[1]<<8)|p[2];
									if( delta )
										h->tempo = 60000000 / delta;
									if( h->debug ) printf("%2d %08ld META tempo:%d\n", t, (long)(h->tracktime), h->tempo);
									if( m_nDefaultTempo == 0 ) m_nDefaultTempo = h->tempo;
									else {
										ttp = h->track;
										if( !ttp ) ttp = mid_locate_track(h, 0, 0xff);
										mid_add_tempo_event(h,h->tempo);
									}
									if( h->tempo > maxtempo ) maxtempo = h->tempo;
									break;
								case 0x2f: // type: end of track
									if( h->debug ) printf("%2d %08ld META end of track\n", t, (long)(h->tracktime));
									if( miditracklen > 0 ) {
										sprintf(buf, "%ld", miditracklen);
										mid_message("Meta event not at end of track, %s bytes left in track", buf);
										miditracklen = 0;
									}
									break;
								default:
									if( h->debug ) printf("%2d %08ld META type 0x%02x\n", t, (long)(h->tracktime), midibyte[0]);
									break;
							}
							break;
						default:
							if( h->debug ) printf("%2d %08ld System type 0x%02x\n", t, (long)(h->tracktime), midibyte[0]);
							break;
					}
					break;
				default:   // no running status, just skip it...
					if( h->debug ) printf("%2d %08ld unknown runningstatus: 0x%02x skipped:0x%02x\n", t, (long)(h->tracktime), runningstatus, midibyte[0]);
					break;
			}
			if( miditracklen < 1 && (runningstatus != 0xff || midibyte[0] != 0x2f) ) {
				delta = mmftell(h->mmf);
				mmreadSBYTES(buf,4,h->mmf);
				buf[4] = '\0';
				if( strcmp(buf,"MTrk") ) {
					miditracklen = 0x7fffffff;
					mid_message("Meta event not at end of track, %s bytes left in track", "superfluous");
				}
				else
					mid_message("Meta event not at end of track, %s bytes left in track", "no");
				mmfseek(h->mmf,delta,SEEK_SET);
			}
		}
	}
	if( h->verbose ) printf("Determining percussion channel\n");
	// get the lowest event time and the used channels
	delta = 0x7fffffff;
	metalen = 0; // use as bit bucket for used channels
	for( ttp=h->track; ttp; ttp=ttp->next ) {
		metalen |= (1<<ttp->chan);
		if( ttp->head ) {
			ULONG tt;
			tt = mid_first_noteonevent_tick(ttp->head);
			if( tt < (ULONG)delta )
				delta = tt;
		}
	}
	if( metalen & 0x03ff ) {
		if( (metalen & 0x0f00) == 0x0400 ) 
			h->percussion = 10; // buggy sng2mid uses channel 10
		else
			h->percussion = 9;
	}
	else h->percussion = 15;
	if( h->verbose )
		printf("Percussion channel is %d\nStripping off silences and other optimalisations\n", h->percussion + 1);
	// last but not least shut off all pending events, transform drumnotes when appropriate
	// strip off silences at begin and end and get the greatest tracktime
	h->tracktime = 0;
	metalen = h->midispeed;
	for( ttp=h->track; ttp; ttp=ttp->next ) {
		if( ttp->chan == h->percussion )
			mid_notes_to_percussion(ttp, delta, metalen);
		else
			mid_prog_to_notes(ttp, delta, metalen);
		if( ttp->tail && ttp->tail->tracktick > h->tracktime )
			h->tracktime = ttp->tail->tracktick;
	}
	h->tracktime += h->divider >> 2; // add one quartnote to the song for silence
	mid_add_partbreak(h);
	if( h->debug )
		mid_dump_tracks(h);
	numchans = mid_numchans(h);
	if( panlow > 48 || panhigh < 80 ) {
		for( ttp=h->track; ttp; ttp=ttp->next ) {
			ttp->balance = ((0x40*numchans+0x80*mid_ordchan(h, ttp->chan))/numchans)&0x7f;
		}
	}
	// set module variables
	numtracks = mid_numtracks(h);
	if( m_nDefaultTempo == 0 ) m_nDefaultTempo = h->tempo;
	if( maxtempo == 0 ) maxtempo = h->tempo;
	if( maxtempo != 255 ) {
		if( h->verbose ) printf("Adjusting tempo %d to 255\n", maxtempo);
		mid_adjust_for_optimal_tempo(h, maxtempo);
	}
	if( maxtempo > 0 ) m_nDefaultTempo = (255 * m_nDefaultTempo) / maxtempo;
	numpats = 1 + (modticks(h, h->tracktime) / h->speed / 64 );
	if( h->verbose ) printf("Generating %d patterns with speed %d\n", numpats, h->speed);
#ifdef NEWMIKMOD
	if( !of->songname ) of->songname = DupStr(of->allochandle, "Untitled", 8);
	of->memsize     = STMEM_LAST;      // Number of memory slots to reserve!
	of->modtype     = _mm_strdup(of->allochandle, MID_Version);
	of->numpat      = numpats;
	of->numpos      = of->numpat;
	of->reppos      = 0;
	of->initspeed   = h->speed;
	of->numchn      = numtracks;
	of->numtrk      = of->numpat * of->numchn;
	of->initvolume  = 64;
	of->pansep      = 128;
	// orderlist
	if(!AllocPositions(of, of->numpos)) {
		avoid_reentry = 0;
		return FALSE;
	}
	for(t=0; t<of->numpos; t++)
		of->positions[t] = t;
	if( !PAT_Load_Instruments(of) ) {
		avoid_reentry = 0;
		return FALSE;
	}
	// ==============================
	// Load the pattern info now!
	if(!AllocTracks(of)) {
		avoid_reentry = 0;
		return FALSE;
	}
	if(!AllocPatterns(of)) {
		avoid_reentry = 0;
		return FALSE;
	}
	of->ut = utrk_init(of->numchn, h->allochandle);
	utrk_memory_reset(of->ut);
	utrk_local_memflag(of->ut, PTMEM_PORTAMENTO, TRUE, FALSE);
	MID_ReadPatterns(of, h, numpats);
	// ============================================================
	// set panning positions
	t = 0;
	for( ttp=h->track; ttp; ttp=ttp->next ) {
		of->panning[t] = modpan(ttp->balance, numchans / 2);
		t++;
	}
#else
	m_nType         = MOD_TYPE_MID;
	m_nDefaultSpeed = h->speed;
	m_nChannels     = numtracks;
	m_dwSongFlags   = SONG_LINEARSLIDES;
	m_nMinPeriod    = 28 << 2;
	m_nMaxPeriod    = 1712 << 3;
	// orderlist
	for(t=0; t < numpats; t++)
		Order[t] = t;
	if( !PAT_Load_Instruments(this) ) {
		avoid_reentry = 0;
		return FALSE;
	}
	// ==============================
	// Load the pattern info now!
	if( MID_ReadPatterns(Patterns, PatternSize, h, numpats, m_nChannels) ) {
		// :^(  need one more channel to handle the global events ;^b
		m_nChannels++;
		h->tp = mid_new_track(h, h->track->chan, 0xff);
		for( ttp=h->track; ttp->next; ttp=ttp->next ) ;
		ttp->next = h->tp;
		mid_add_sync(h, h->tp);
		for( t=0; t<numpats; t++ ) {
			FreePattern(Patterns[t]);
			Patterns[t] = NULL;
		}
		MID_ReadPatterns(Patterns, PatternSize, h, numpats, m_nChannels);
	}
	// ============================================================
	// set panning positions
	t = 0;
	for( ttp=h->track; ttp; ttp=ttp->next ) {
		ChnSettings[t].nPan    = modpan(ttp->balance, numchans / 2);
		ChnSettings[t].nVolume = 64;
		t++;
	}
	MID_Cleanup(h);	// we dont need it anymore
#endif
	if( h->verbose ) printf("Done\n");
	avoid_reentry = 0; // it is safe now, I'm finished
	return TRUE;
}

#ifdef NEWMIKMOD
// =====================================================================================
CHAR *MID_LoadTitle(MMSTREAM *mmfile)
// =====================================================================================
{
	int t;
	char buf[24];
	long miditracklen;
	BYTE runningstatus;
	BYTE cmd;
	BYTE midibyte[2];
	long metalen;
	MIDHANDLE hh, *h;
	h = &hh;
	h->mmf = mmfile;
	mmfseek(h->mmf,8,SEEK_SET);
	h->midiformat	= mid_read_short(h);
	h->miditracks = mid_read_short(h);
	h->resolution = mid_read_short(h);
	// at this point the h->mmf is positioned at first miditrack
	if( h->midiformat == 0 ) h->miditracks = 1;
	h->tracktime = 0;
	for( t=0; t<h->miditracks; t++ ) {
		mmreadSBYTES(buf,4,h->mmf);
		miditracklen = mid_read_long(h);
		runningstatus = 0;
		while( miditracklen > 0 ) {
			miditracklen -= mid_read_delta(h);
			midibyte[0] = mid_read_byte(h);
			miditracklen--;
			if( midibyte[0] & 0x80 ) {
				runningstatus = midibyte[0];
				switch( runningstatus ) {
					case 0xf1:
					case 0xf4:
					case 0xf5:
					case 0xf6:
					case 0xf7:
					case 0xf8:
					case 0xf9:
					case 0xfa:
					case 0xfb:
					case 0xfc:
					case 0xfd:
					case 0xfe:
						break;
					default:
						midibyte[0] = mid_read_byte(h);
						miditracklen--;
						break;
				}
			}
			cmd = runningstatus & 0xf0;
			switch( cmd ) {
				case 0x80: // note off
				case 0x90: // note on
				case 0xa0: // polyphonic key pressure
				case 0xb0: // control change
				case 0xe0: // pitch wheel change
					midibyte[1] = mid_read_byte(h);
					miditracklen--;
				case 0xc0: // program change
				case 0xd0: // channel pressure
					break;
				case 0xf0: // system & realtime
					switch( runningstatus ) {
						case 0xf0:	// sysex
							while( midibyte[0] != 0xf7 ) {
								midibyte[0] = mid_read_byte(h);
								miditracklen--;
							}
							break;
						case 0xf2:	// song position pointer
							midibyte[1] = mid_read_byte(h);
							miditracklen--;
							break;
						case 0xf7:
							miditracklen -= mid_read_delta(h);
							metalen = h->deltatime;
							while( metalen > 0 ) {
								midibyte[1] = mid_read_byte(h);
								metalen--;
								miditracklen--;
							}
							break;
						case 0xff: // meta event
							miditracklen -= mid_read_delta(h);
							metalen = h->deltatime;
							if( metalen > 21 ) metalen = 21;
							if( metalen ) {
								mmreadSBYTES(buf, metalen, h->mmf);
								miditracklen -= metalen;
							}
							buf[metalen] = '\0';
							metalen = h->deltatime - metalen;
							while( metalen > 0 ) {
								midibyte[1] = mid_read_byte(h);
								metalen--;
								miditracklen--;
							}
							switch( midibyte[0] ) {
								case 0x03: // type: track name
									return DupStr(NULL, buf, strlen(buf));
									break;
								case 0x2f: // type: end of track
									miditracklen = 0;
									break;
								default:
									break;
							}
							break;
						default:
							break;
					}
					break;
				default:   // no running status, just skip it...
					break;
			}
			if( miditracklen < 1 && (runningstatus != 0xff || midibyte[0] != 0x2f) ) {
				metalen = mmftell(h->mmf);
				mmreadSBYTES(buf,4,h->mmf);
				buf[4] = '\0';
				if( strcmp(buf,"MTrk") ) miditracklen = 0x7fffffff;
				mmfseek(h->mmf,metalen,SEEK_SET);
			}
		}
	}
	return DupStr(NULL, "Untitled" ,8);
}

MLOADER load_mid =
{
    "MID",
    "Musical Instrument Digital Interface",
    0x30,
    NULL,
    MID_Test,
    (void *(*)(void))MID_Init,
    (void (*)(ML_HANDLE *))MID_Cleanup,
    /* Every single loader seems to need one of these! */
    (BOOL (*)(ML_HANDLE *, UNIMOD *, MMSTREAM *))MID_Load,
    MID_LoadTitle
};
#endif
