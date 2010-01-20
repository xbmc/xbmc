/*
    TiMidity++ -- MIDI to WAVE converter and player
    Copyright (C) 1999-2002 Masanao Izumo <mo@goice.co.jp>
    Copyright (C) 1995 Tuukka Toivonen <tt@cgs.fi>
*/

/* This code from awesfx
 * Modified by Masanao Izumo <mo@goice.co.jp>
 */

/*================================================================
 * parsesf.c
 *	parse SoundFonr layers and convert it to AWE driver patch
 *
 * Copyright (C) 1996,1997 Takashi Iwai
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *================================================================*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */
#include <stdio.h>
#ifndef NO_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif
#include <stdlib.h>
#include <math.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include "timidity.h"
#include "common.h"
#include "tables.h"
#include "instrum.h"
#include "playmidi.h"
#include "controls.h"
#include "sffile.h"
#include "sflayer.h"
#include "sfitem.h"
#include "output.h"
#include "filter.h"
#include "resample.h"

#define FILENAME_NORMALIZE(fname) url_expand_home_dir(fname)
#define FILENAME_REDUCED(fname)   url_unexpand_home_dir(fname)
#define SFMalloc(rec, count)      new_segment(&(rec)->pool, count)
#define SFStrdup(rec, s)          strdup_mblock(&(rec)->pool, s)

/*----------------------------------------------------------------
 * compile flags
 *----------------------------------------------------------------*/

#ifdef CFG_FOR_SF
#define SF_SUPPRESS_ENVELOPE
#define SF_SUPPRESS_TREMOLO
#define SF_SUPPRESS_VIBRATO
#else
#define SF_CLOSE_EACH_FILE
/*#define SF_SUPPRESS_ENVELOPE*/
/*#define SF_SUPPRESS_TREMOLO*/
/*#define SF_SUPPRESS_VIBRATO*/
#endif /* CFG_FOR_SF */

/* return value */
#define AWE_RET_OK		0	/* successfully loaded */
#define AWE_RET_ERR		1	/* some fatal error occurs */
#define AWE_RET_SKIP		2	/* some fonts are skipped */
#define AWE_RET_NOMEM		3	/* out or memory; not all fonts loaded */
#define AWE_RET_NOT_FOUND	4	/* the file is not found */

/*----------------------------------------------------------------
 * local parameters
 *----------------------------------------------------------------*/

typedef struct _SFPatchRec {
	int preset, bank, keynote; /* -1 = matches all */
} SFPatchRec;

typedef struct _SampleList {
	Sample v;
	struct _SampleList *next;
	int32 start;
	int32 len;
	int32 cutoff_freq;
	int16 resonance;
	int16 root, tune;
	char low, high;		/* key note range */
	int8 reverb_send, chorus_send;

	/* Depend on play_mode->rate */
	int32 vibrato_freq;
	int32 attack;
	int32 hold;
	int32 sustain;
	int32 decay;
	int32 release;

	int32 modattack;
	int32 modhold;
	int32 modsustain;
	int32 moddecay;
	int32 modrelease;

	int bank, keynote;	/* for drum instruments */
} SampleList;

typedef struct _InstList {
	SFPatchRec pat;
	int pr_idx;
	int samples;
	int order;
	SampleList *slist;
	struct _InstList *next;
} InstList;

typedef struct _SFExclude {
	SFPatchRec pat;
	struct _SFExclude *next;
} SFExclude;

typedef struct _SFOrder {
	SFPatchRec pat;
	int order;
	struct _SFOrder *next;
} SFOrder;

#define INSTHASHSIZE 127
#define INSTHASH(bank, preset, keynote) \
	((int)(((unsigned)bank ^ (unsigned)preset ^ (unsigned)keynote) % INSTHASHSIZE))

typedef struct _SFInsts {
	struct timidity_file *tf;
	char *fname;
	int8 def_order, def_cutoff_allowed, def_resonance_allowed;
	uint16 version, minorversion;
	int32 samplepos, samplesize;
	InstList *instlist[INSTHASHSIZE];
	char **inst_namebuf;
	SFExclude *sfexclude;
	SFOrder *sforder;
	struct _SFInsts *next;
	FLOAT_T amptune;
	MBlockList pool;
} SFInsts;

/*----------------------------------------------------------------*/

/* prototypes */

#define P_GLOBAL	1
#define P_LAYER		2
#ifndef FALSE
#define FALSE 0
#endif /* FALSE */
#ifndef TRUE
#define TRUE 1
#endif /* TRUE */


static SFInsts *find_soundfont(char *sf_file);
static SFInsts *new_soundfont(char *sf_file);
static void init_sf(SFInsts *rec);
static void end_soundfont(SFInsts *rec);
static Instrument *try_load_soundfont(SFInsts *rec, int order, int bank,
				      int preset, int keynote);
static Instrument *load_from_file(SFInsts *rec, InstList *ip);
static int is_excluded(SFInsts *rec, int bank, int preset, int keynote);
static int is_ordered(SFInsts *rec, int bank, int preset, int keynote);
static int load_font(SFInfo *sf, int pridx);
static int parse_layer(SFInfo *sf, int pridx, LayerTable *tbl, int level);
static int is_global(SFGenLayer *layer);
static void clear_table(LayerTable *tbl);
static void set_to_table(SFInfo *sf, LayerTable *tbl, SFGenLayer *lay, int level);
static void add_item_to_table(LayerTable *tbl, int oper, int amount, int level);
static void merge_table(SFInfo *sf, LayerTable *dst, LayerTable *src);
static void init_and_merge_table(SFInfo *sf, LayerTable *dst, LayerTable *src);
static int sanity_range(LayerTable *tbl);
static int make_patch(SFInfo *sf, int pridx, LayerTable *tbl);
static void make_info(SFInfo *sf, SampleList *vp, LayerTable *tbl);
static FLOAT_T calc_volume(LayerTable *tbl);
static void set_sample_info(SFInfo *sf, SampleList *vp, LayerTable *tbl);
static void set_init_info(SFInfo *sf, SampleList *vp, LayerTable *tbl);
static int abscent_to_Hz(int abscents);
static void set_rootkey(SFInfo *sf, SampleList *vp, LayerTable *tbl);
static void set_rootfreq(SampleList *vp);
static int32 to_offset(int32 offset);
static int32 to_rate(int32 diff, int timecent);
static int32 calc_rate(int32 diff, double msec);
static double to_msec(int timecent);
static int32 calc_sustain(int sust_cB);
static void convert_volume_envelope(SampleList *vp, LayerTable *tbl);
static void convert_tremolo(SampleList *vp, LayerTable *tbl);
static void convert_vibrato(SampleList *vp, LayerTable *tbl);

/*----------------------------------------------------------------*/

static SFInsts *sfrecs = NULL;
static SFInsts *current_sfrec = NULL;
#define def_drum_inst 0

void free_soundfonts()
{
    SFInsts *sf, *sf_next;

    for ( sf = sfrecs; sf; sf = sf_next )
	{
		sf_next = sf->next;
		end_soundfont( sf );
		free( sf );
	}

	sfrecs = 0;
	current_sfrec = 0;
}


static SFInsts *find_soundfont(char *sf_file)
{
    SFInsts *sf;

    sf_file = FILENAME_NORMALIZE(sf_file);
    for(sf = sfrecs; sf != NULL; sf = sf->next)
	if(sf->fname != NULL && strcmp(sf->fname, sf_file) == 0)
	    return sf;
    return NULL;
}

static SFInsts *new_soundfont(char *sf_file)
{
    SFInsts *sf;

    sf_file = FILENAME_NORMALIZE(sf_file);
    for(sf = sfrecs; sf != NULL; sf = sf->next)
	if(sf->fname == NULL)
	    break;
    if(sf == NULL)
	sf = (SFInsts *)safe_malloc(sizeof(SFInsts));
    memset(sf, 0, sizeof(SFInsts));
    init_mblock(&sf->pool);
    sf->fname = SFStrdup(sf, FILENAME_NORMALIZE(sf_file));
    sf->def_order = DEFAULT_SOUNDFONT_ORDER;
    sf->amptune = 1.0;
    return sf;
}

void add_soundfont(char *sf_file,
		   int sf_order, int sf_cutoff, int sf_resonance,
		   int amp)
{
    SFInsts *sf;

    if((sf = find_soundfont(sf_file)) == NULL)
    {
        sf = new_soundfont(sf_file);
        sf->next = sfrecs;
        sfrecs = sf;
    }

    if(sf_order >= 0)
        sf->def_order = sf_order;
    if(sf_cutoff >= 0)
        sf->def_cutoff_allowed = sf_cutoff;
    if(sf_resonance >= 0)
        sf->def_resonance_allowed = sf_resonance;
    if(amp >= 0)
        sf->amptune = (FLOAT_T)amp * 0.01;
    current_sfrec = sf;
}

void remove_soundfont(char *sf_file)
{
    SFInsts *sf;

    if((sf = find_soundfont(sf_file)) != NULL)
	end_soundfont(sf);
}

char *soundfont_preset_name(int bank, int preset, int keynote,
			    char **sndfile)
{
    SFInsts *rec;
    if(sndfile != NULL)
	*sndfile = NULL;
    for(rec = sfrecs; rec != NULL; rec = rec->next)
	if(rec->fname != NULL)
	{
	    int addr;
	    InstList *ip;

	    addr = INSTHASH(bank, preset, keynote);
	    for(ip = rec->instlist[addr]; ip; ip = ip->next)
		if(ip->pat.bank == bank && ip->pat.preset == preset &&
		   (keynote < 0 || keynote == ip->pat.keynote))
		    break;
	    if(ip != NULL)
	    {
		if(sndfile != NULL)
		    *sndfile = rec->fname;
		return rec->inst_namebuf[ip->pr_idx];
	    }
	}
    return NULL;
}

static void init_sf(SFInsts *rec)
{
	SFInfo sfinfo;
	int i;

	ctl->cmsg(CMSG_INFO, VERB_NOISY, "Init soundfonts `%s'",
		  FILENAME_REDUCED(rec->fname));

	if ((rec->tf = open_file(rec->fname, 1, OF_VERBOSE)) == NULL) {
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			  "Can't open soundfont file %s",
			  FILENAME_REDUCED(rec->fname));
		end_soundfont(rec);
		return;
	}

	if(load_soundfont(&sfinfo, rec->tf))
	{
	    end_soundfont(rec);
	    return;
	}

	correct_samples(&sfinfo);
	current_sfrec = rec;
	for (i = 0; i < sfinfo.npresets; i++) {
		int bank = sfinfo.preset[i].bank;
		int preset = sfinfo.preset[i].preset;

		if (bank == 128)
		    /* FIXME: why not allow exclusion of drumsets? */
		    alloc_instrument_bank(1, preset);
		else {
			if (is_excluded(rec, bank, preset, -1))
				continue;
			alloc_instrument_bank(0, bank);
		}
		load_font(&sfinfo, i);
	}

	/* copy header info */
	rec->version = sfinfo.version;
	rec->minorversion = sfinfo.minorversion;
	rec->samplepos = sfinfo.samplepos;
	rec->samplesize = sfinfo.samplesize;
	rec->inst_namebuf =
	    (char **)SFMalloc(rec, sfinfo.npresets * sizeof(char *));
	for(i = 0; i < sfinfo.npresets; i++)
	    rec->inst_namebuf[i] =
		(char *)SFStrdup(rec, sfinfo.preset[i].hdr.name);
	free_soundfont(&sfinfo);

#ifndef SF_CLOSE_EACH_FILE
	if(!IS_URL_SEEK_SAFE(rec->tf->url))
#endif
	{
	    close_file(rec->tf);
	    rec->tf = NULL;
	}
}

void init_load_soundfont(void)
{
    SFInsts *rec;
    for(rec = sfrecs; rec != NULL; rec = rec->next)
	if(rec->fname != NULL)
	    init_sf(rec);
}

static void end_soundfont(SFInsts *rec)
{
	if (rec->tf) {
		close_file(rec->tf);
		rec->tf = NULL;
	}

	rec->fname = NULL;
	rec->inst_namebuf = NULL;
	rec->sfexclude = NULL;
	rec->sforder = NULL;
	reuse_mblock(&rec->pool);
}

Instrument *extract_soundfont(char *sf_file, int bank, int preset,
			      int keynote)
{
    SFInsts *sf;

    if((sf = find_soundfont(sf_file)) != NULL)
	return try_load_soundfont(sf, -1, bank, preset, keynote);
    sf = new_soundfont(sf_file);
    sf->next = sfrecs;
    sf->def_order = 2;
    sfrecs = sf;
    init_sf(sf);
    return try_load_soundfont(sf, -1, bank, preset, keynote);
}

/*----------------------------------------------------------------
 * get converted instrument info and load the wave data from file
 *----------------------------------------------------------------*/

static Instrument *try_load_soundfont(SFInsts *rec, int order, int bank,
				      int preset, int keynote)
{
	InstList *ip;
	Instrument *inst = NULL;
	int addr;

	if (rec->tf == NULL) {
		if (rec->fname == NULL)
			return NULL;
		if ((rec->tf = open_file(rec->fname, 1, OF_VERBOSE)) == NULL) {
			ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
				  "Can't open soundfont file %s",
				  FILENAME_REDUCED(rec->fname));
			end_soundfont(rec);
			return NULL;
		}
#ifndef SF_CLOSE_EACH_FILE
		if(!IS_URL_SEEK_SAFE(rec->tf->url))
		    rec->tf->url = url_cache_open(rec->tf->url, 1);
#endif /* SF_CLOSE_EACH_FILE */
	}

	addr = INSTHASH(bank, preset, keynote);
	for (ip = rec->instlist[addr]; ip; ip = ip->next) {
		if (ip->pat.bank == bank && ip->pat.preset == preset &&
		    (keynote < 0 || ip->pat.keynote == keynote) &&
		    (order < 0 || ip->order == order))
			break;
	}

	if (ip && ip->samples)
		inst = load_from_file(rec, ip);

#ifdef SF_CLOSE_EACH_FILE
	close_file(rec->tf);
	rec->tf = NULL;
#endif

	return inst;
}

Instrument *load_soundfont_inst(int order,
				int bank, int preset, int keynote)
{
    SFInsts *rec;
    Instrument *ip;
    /*
     * Search through all ordered soundfonts
     */
    int o = order;

    for(rec = sfrecs; rec != NULL; rec = rec->next)
    {
	if(rec->fname != NULL)
	{
	    ip = try_load_soundfont(rec, o, bank, preset, keynote);
	    if(ip != NULL)
		return ip;
	    if (o > 0) o++;
	}
    }
    return NULL;
}

/*----------------------------------------------------------------*/
#define TO_MHZ(abscents) (int32)(8176.0 * pow(2.0,(double)(abscents)/1200.0))
#if 0
#ifndef M_LN2
#define M_LN2		0.69314718055994530942
#endif /* M_LN2 */
#ifndef M_LN10
#define M_LN10		2.30258509299404568402
#endif /* M_LN10 */
#define TO_VOLUME(centibel) (uint8)(255 * (1.0 - \
				(centibel) * (M_LN10 / 1200.0 / M_LN2)))
#else
#define TO_VOLUME(level)  (uint8)(255.0 - (level) * (255.0/1000.0))
#endif


static FLOAT_T calc_volume(LayerTable *tbl)
{
    int v;

    if(!tbl->set[SF_initAtten] || (int)tbl->val[SF_initAtten] == 0)
	return (FLOAT_T)1.0;

	v = (int)tbl->val[SF_initAtten];
    if(v < 0) {v = 0;}
    else if(v > 960) {v = 960;}
	return cb_to_amp_table[v];
}

/* convert from 16bit value to fractional offset (15.15) */
static int32 to_offset(int32 offset)
{
	return offset << 14;
}

#define SF_ENVRATE_MAX (0x3FFFFFFFL)
#define SF_ENVRATE_MIN (1L)

/* calculate ramp rate in fractional unit;
 * diff = 16bit, time = msec
 */
static int32 calc_rate(int32 diff, double msec)
{
    double rate;

    if(msec == 0) {return (int32)SF_ENVRATE_MAX + 1;}
    if(diff <= 0) {diff = 1;}
    diff <<= 14;
    rate = ((double)diff / play_mode->rate) * control_ratio * 1000.0 / msec;
    if(fast_decay) {rate *= 2;}
	if(rate > SF_ENVRATE_MAX) {rate = SF_ENVRATE_MAX;}
	else if(rate < SF_ENVRATE_MIN) {rate = SF_ENVRATE_MIN;}
    return (int32)rate;
}

/* calculate ramp rate in fractional unit;
 * diff = 16bit, timecent
 */
static int32 to_rate(int32 diff, int timecent)
{
    double rate;

    if(timecent == -12000)	/* instantaneous attack */
	{return (int32)SF_ENVRATE_MAX + 1;}
    if(diff <= 0) {diff = 1;}
    diff <<= 14;
    rate = (double)diff * control_ratio / play_mode->rate / pow(2.0, (double)timecent / 1200.0);
    if(fast_decay) {rate *= 2;}
	if(rate > SF_ENVRATE_MAX) {rate = SF_ENVRATE_MAX;}
	else if(rate < SF_ENVRATE_MIN) {rate = SF_ENVRATE_MIN;}
    return (int32)rate;
}

/*
 * convert timecents to sec
 */
static double to_msec(int timecent)
{
    return timecent == -12000 ? 0 : 1000.0 * pow(2.0, (double)timecent / 1200.0);
}

/*
 * Sustain level
 * sf: centibels
 * parm: 0x7f - sustain_level(dB) * 0.75
 */
static int32 calc_sustain(int sust_cB)
{
	if(sust_cB <= 0) {return 65533;}
	else if(sust_cB >= 1000) {return 0;}
	else {return (1000 - sust_cB) * 65533 / 1000;}
}

static Instrument *load_from_file(SFInsts *rec, InstList *ip)
{
	SampleList *sp;
	Instrument *inst;
	int i;
	int32 len;

	if(ip->pat.bank == 128)
	    ctl->cmsg(CMSG_INFO, VERB_NOISY,
		      "Loading SF Drumset %d %d: %s",
		      ip->pat.preset + progbase, ip->pat.keynote,
		      rec->inst_namebuf[ip->pr_idx]);
	else
	    ctl->cmsg(CMSG_INFO, VERB_NOISY,
		      "Loading SF Tonebank %d %d: %s",
		      ip->pat.bank, ip->pat.preset + progbase,
		      rec->inst_namebuf[ip->pr_idx]);
	inst = (Instrument *)safe_malloc(sizeof(Instrument));
	inst->instname = rec->inst_namebuf[ip->pr_idx];
	inst->type = INST_SF2;
	inst->samples = ip->samples;
	inst->sample = (Sample *)safe_malloc(sizeof(Sample) * ip->samples);
	memset(inst->sample, 0, sizeof(Sample) * ip->samples);
	for (i = 0, sp = ip->slist; i < ip->samples && sp; i++, sp = sp->next) {
		Sample *sample = inst->sample + i;
		int32 j;
#ifndef LITTLE_ENDIAN
		int32 k;
		int16 *tmp, s;
#endif
		ctl->cmsg(CMSG_INFO, VERB_DEBUG,
			  "[%d] Rate=%d LV=%d HV=%d "
			  "Low=%d Hi=%d Root=%d Pan=%d",
			  sp->start, sp->v.sample_rate, 
			  sp->v.low_vel, sp->v.high_vel, 
			  sp->v.low_freq, sp->v.high_freq, sp->v.root_freq,
			  sp->v.panning);
		memcpy(sample, &sp->v, sizeof(Sample));
		sample->data = NULL;
		sample->data_alloced = 0;

		if(i > 0 && (!sample->note_to_use ||
			     (sample->modes & MODES_LOOPING)))
		{
		    SampleList *sps;
		    Sample *found, *s;

		    found = NULL;
		    for(j = 0, sps = ip->slist, s = inst->sample; j < i && sps;
			j++, sps = sps->next, s++)
		    {
			if(s->data == NULL)
			    break;
			if(sp->start == sps->start)
			{
			    if(antialiasing_allowed)
			    {
				 if(sample->data_length != s->data_length ||
				    sample->sample_rate != s->sample_rate)
				     continue;
			    }
			    if(s->note_to_use && !(s->modes & MODES_LOOPING))
				continue;
			    found = s;
			    break;
			}
		    }
		    if(found)
		    {
			sample->data = found->data;
			sample->data_alloced = 0;
			ctl->cmsg(CMSG_INFO, VERB_DEBUG, " * Cached");
			continue;
		    }
		}

		sample->data = (sample_t *)safe_large_malloc(sp->len + 2 * 3);
		sample->data_alloced = 1;

		tf_seek(rec->tf, sp->start, SEEK_SET);
		tf_read(sample->data, sp->len, 1, rec->tf);

#ifndef LITTLE_ENDIAN
		tmp = (int16*)sample->data;
		k = sp->len / 2;
		for (j = 0; j < k; j++) {
			s = LE_SHORT(*tmp);
			*tmp++ = s;
		}
#endif
		/* set a small blank loop at the tail for avoiding abnormal loop. */
		len = sp->len / 2;
		sample->data[len] = sample->data[len + 1] = sample->data[len + 2] = 0;

		if (antialiasing_allowed)
		    antialiasing((int16 *)sample->data,
				 sample->data_length >> FRACTION_BITS,
				 sample->sample_rate,
				 play_mode->rate);

		/* resample it if possible */
		if (sample->note_to_use && !(sample->modes & MODES_LOOPING))
			pre_resample(sample);

		/* do pitch detection on drums if surround chorus is used */
		if (ip->pat.bank == 128 && opt_surround_chorus)
		{
		    sample->chord = -1;
		    sample->root_freq_detected =
		    	freq_fourier(sample, &(sample->chord));
		    sample->transpose_detected =
			assign_pitch_to_freq(sample->root_freq_detected) -
			assign_pitch_to_freq(sample->root_freq / 1024.0);
		}

#ifdef LOOKUP_HACK
		squash_sample_16to8(sample);
#endif
	}

	return inst;
}


/*----------------------------------------------------------------
 * excluded samples
 *----------------------------------------------------------------*/

int exclude_soundfont(int bank, int preset, int keynote)
{
	SFExclude *exc;
	if(current_sfrec == NULL)
	    return 1;
	exc = (SFExclude*)SFMalloc(current_sfrec , sizeof(SFExclude));
	exc->pat.bank = bank;
	exc->pat.preset = preset;
	exc->pat.keynote = keynote;
	exc->next = current_sfrec->sfexclude;
	current_sfrec->sfexclude = exc;
	return 0;
}

/* check the instrument is specified to be excluded */
static int is_excluded(SFInsts *rec, int bank, int preset, int keynote)
{
	SFExclude *p;
	for (p = rec->sfexclude; p; p = p->next) {
		if (p->pat.bank == bank &&
		    (p->pat.preset < 0 || p->pat.preset == preset) &&
		    (p->pat.keynote < 0 || p->pat.keynote == keynote))
			return 1;
	}
	return 0;
}


/*----------------------------------------------------------------
 * ordered samples
 *----------------------------------------------------------------*/

int order_soundfont(int bank, int preset, int keynote, int order)
{
	SFOrder *p;
	if(current_sfrec == NULL)
	    return 1;
	p = (SFOrder*)SFMalloc(current_sfrec, sizeof(SFOrder));
	p->pat.bank = bank;
	p->pat.preset = preset;
	p->pat.keynote = keynote;
	p->order = order;
	p->next = current_sfrec->sforder;
	current_sfrec->sforder = p;
	return 0;
}

/* check the instrument is specified to be ordered */
static int is_ordered(SFInsts *rec, int bank, int preset, int keynote)
{
	SFOrder *p;
	for (p = rec->sforder; p; p = p->next) {
		if (p->pat.bank == bank &&
		    (p->pat.preset < 0 || p->pat.preset == preset) &&
		    (p->pat.keynote < 0 || p->pat.keynote == keynote))
			return p->order;
	}
	return -1;
}


/*----------------------------------------------------------------*/

static int load_font(SFInfo *sf, int pridx)
{
	SFPresetHdr *preset = &sf->preset[pridx];
	int rc, j, nlayers;
	SFGenLayer *layp, *globalp;

	/* if layer is empty, skip it */
	if ((nlayers = preset->hdr.nlayers) <= 0 ||
	    (layp = preset->hdr.layer) == NULL)
		return AWE_RET_SKIP;
	/* check global layer */
	globalp = NULL;
	if (is_global(layp)) {
		globalp = layp;
		layp++;
		nlayers--;
	}
	/* parse for each preset layer */
	for (j = 0; j < nlayers; j++, layp++) {
		LayerTable tbl;

		/* set up table */
		clear_table(&tbl);
		if (globalp)
			set_to_table(sf, &tbl, globalp, P_GLOBAL);
		set_to_table(sf, &tbl, layp, P_LAYER);
		
		/* parse the instrument */
		rc = parse_layer(sf, pridx, &tbl, 0);
		if(rc == AWE_RET_ERR || rc == AWE_RET_NOMEM)
			return rc;
	}

	return AWE_RET_OK;
}


/*----------------------------------------------------------------*/

/* parse a preset layer and convert it to the patch structure */
static int parse_layer(SFInfo *sf, int pridx, LayerTable *tbl, int level)
{
	SFInstHdr *inst;
	int rc, i, nlayers;
	SFGenLayer *lay, *globalp;
#if 0
	SFPresetHdr *preset = &sf->preset[pridx];
#endif

	if (level >= 2) {
		fprintf(stderr, "parse_layer: too deep instrument level\n");
		return AWE_RET_ERR;
	}

	/* instrument must be defined */
	if (!tbl->set[SF_instrument])
		return AWE_RET_SKIP;

	inst = &sf->inst[tbl->val[SF_instrument]];

	/* Here, TiMidity makes the reference of the data.  The real data
	 * is loaded after.  So, duplicated data is allowed */
#if 0
	/* if non-standard drumset includes standard drum instruments,
	   skip it to avoid duplicate the data */
	if (def_drum_inst >= 0 && preset->bank == 128 && preset->preset != 0 &&
	    tbl->val[SF_instrument] == def_drum_inst)
			return AWE_RET_SKIP;
#endif

	/* if layer is empty, skip it */
	if ((nlayers = inst->hdr.nlayers) <= 0 ||
	    (lay = inst->hdr.layer) == NULL)
		return AWE_RET_SKIP;

	/* check global layer */
	globalp = NULL;
	if (is_global(lay)) {
		globalp = lay;
		lay++;
		nlayers--;
	}

	/* parse for each layer */
	for (i = 0; i < nlayers; i++, lay++) {
		LayerTable ctbl;
		clear_table(&ctbl);
		if (globalp)
			set_to_table(sf, &ctbl, globalp, P_GLOBAL);
		set_to_table(sf, &ctbl, lay, P_LAYER);

		if (!ctbl.set[SF_sampleId]) {
			/* recursive loading */
			merge_table(sf, &ctbl, tbl);
			if (! sanity_range(&ctbl))
				continue;
			rc = parse_layer(sf, pridx, &ctbl, level+1);
			if (rc != AWE_RET_OK && rc != AWE_RET_SKIP)
				return rc;
		} else {
			init_and_merge_table(sf, &ctbl, tbl);
			if (! sanity_range(&ctbl))
				continue;

			/* load the info data */
			if ((rc = make_patch(sf, pridx, &ctbl)) == AWE_RET_ERR)
				return rc;
		}
	}
	return AWE_RET_OK;
}


static int is_global(SFGenLayer *layer)
{
	int i;
	for (i = 0; i < layer->nlists; i++) {
		if (layer->list[i].oper == SF_instrument ||
		    layer->list[i].oper == SF_sampleId)
			return 0;
	}
	return 1;
}


/*----------------------------------------------------------------
 * layer table handlers
 *----------------------------------------------------------------*/

/* initialize layer table */
static void clear_table(LayerTable *tbl)
{
	memset(tbl->val, 0, sizeof(tbl->val));
	memset(tbl->set, 0, sizeof(tbl->set));
}

/* set items in a layer to the table */
static void set_to_table(SFInfo *sf, LayerTable *tbl, SFGenLayer *lay, int level)
{
	int i;
	for (i = 0; i < lay->nlists; i++) {
		SFGenRec *gen = &lay->list[i];
		/* copy the value regardless of its copy policy */
		tbl->val[gen->oper] = gen->amount;
		tbl->set[gen->oper] = level;
	}
}

/* add an item to the table */
static void add_item_to_table(LayerTable *tbl, int oper, int amount, int level)
{
	LayerItem *item = &layer_items[oper];
	int o_lo, o_hi, lo, hi;

	switch (item->copy) {
	case L_INHRT:
		tbl->val[oper] += amount;
		break;
	case L_OVWRT:
		tbl->val[oper] = amount;
		break;
	case L_PRSET:
	case L_INSTR:
		/* do not overwrite */
		if (!tbl->set[oper])
			tbl->val[oper] = amount;
		break;
	case L_RANGE:
		if (!tbl->set[oper]) {
			tbl->val[oper] = amount;
		} else {
			o_lo = LOWNUM(tbl->val[oper]);
			o_hi = HIGHNUM(tbl->val[oper]);
			lo = LOWNUM(amount);
			hi = HIGHNUM(amount);
			if (lo < o_lo) lo = o_lo;
			if (hi > o_hi) hi = o_hi;
			tbl->val[oper] = RANGE(lo, hi);
		}
		break;
	}
}

/* merge two tables */
static void merge_table(SFInfo *sf, LayerTable *dst, LayerTable *src)
{
	int i;
	for (i = 0; i < SF_EOF; i++) {
		if (src->set[i]) {
			if (sf->version == 1) {
				if (!dst->set[i] ||
				    i == SF_keyRange || i == SF_velRange)
					/* just copy it */
					dst->val[i] = src->val[i];
			}
			else
				add_item_to_table(dst, i, src->val[i], P_GLOBAL);
			dst->set[i] = P_GLOBAL;
		}
	}
}

/* merge and set default values */
static void init_and_merge_table(SFInfo *sf, LayerTable *dst, LayerTable *src)
{
	int i;

	/* default value is not zero */
	if (sf->version == 1) {
		layer_items[SF_sustainEnv1].defv = 1000;
		layer_items[SF_sustainEnv2].defv = 1000;
		layer_items[SF_freqLfo1].defv = -725;
		layer_items[SF_freqLfo2].defv = -15600;
	} else {
		layer_items[SF_sustainEnv1].defv = 0;
		layer_items[SF_sustainEnv2].defv = 0;
		layer_items[SF_freqLfo1].defv = 0;
		layer_items[SF_freqLfo2].defv = 0;
	}

	/* set default */
	for (i = 0; i < SF_EOF; i++) {
		if (!dst->set[i])
			dst->val[i] = layer_items[i].defv;
	}
	merge_table(sf, dst, src);
	/* convert from SBK to SF2 */
	if (sf->version == 1) {
		for (i = 0; i < SF_EOF; i++) {
			if (dst->set[i])
				dst->val[i] = sbk_to_sf2(i, dst->val[i]);
		}
	}
}


/*----------------------------------------------------------------
 * check key and velocity range
 *----------------------------------------------------------------*/

static int sanity_range(LayerTable *tbl)
{
	int lo, hi;

	lo = LOWNUM(tbl->val[SF_keyRange]);
	hi = HIGHNUM(tbl->val[SF_keyRange]);
	if (lo < 0 || lo > 127 || hi < 0 || hi > 127 || hi < lo)
		return 0;

	lo = LOWNUM(tbl->val[SF_velRange]);
	hi = HIGHNUM(tbl->val[SF_velRange]);
	if (lo < 0 || lo > 127 || hi < 0 || hi > 127 || hi < lo)
		return 0;

	return 1;
}


/*----------------------------------------------------------------
 * create patch record from the stored data table
 *----------------------------------------------------------------*/

#ifdef CFG_FOR_SF
int opt_reverb_control;	/* to avoid warning. */
static int cfg_for_sf_scan(char *name, int x_bank, int x_preset, int x_keynote_from, int x_keynote_to, int romflag);
#endif

static int make_patch(SFInfo *sf, int pridx, LayerTable *tbl)
{
    int bank, preset, keynote;
    int keynote_from, keynote_to, done;
    int addr, order;
    InstList *ip;
    SFSampleInfo *sample;
    SampleList *sp;

    sample = &sf->sample[tbl->val[SF_sampleId]];
#ifdef CFG_FOR_SF
	cfg_for_sf_scan(sample->name,sf->preset[pridx].bank,sf->preset[pridx].preset,LOWNUM(tbl->val[SF_keyRange]),
		HIGHNUM(tbl->val[SF_keyRange]),sample->sampletype & SF_SAMPLETYPE_ROM);
#endif
    if(sample->sampletype & SF_SAMPLETYPE_ROM) /* is ROM sample? */
    {
	ctl->cmsg(CMSG_INFO, VERB_DEBUG, "preset %d is ROM sample: 0x%x",
		  pridx, sample->sampletype);
	return AWE_RET_SKIP;
    }

    bank = sf->preset[pridx].bank;
    preset = sf->preset[pridx].preset;
    if(bank == 128){
		keynote_from = LOWNUM(tbl->val[SF_keyRange]);
		keynote_to = HIGHNUM(tbl->val[SF_keyRange]);
    } else
	keynote_from = keynote_to = -1;

	done = 0;
	for(keynote=keynote_from;keynote<=keynote_to;keynote++){

    ctl->cmsg(CMSG_INFO, VERB_DEBUG_SILLY,
	      "SF make inst pridx=%d bank=%d preset=%d keynote=%d",
	      pridx, bank, preset, keynote);

    if(is_excluded(current_sfrec, bank, preset, keynote))
    {
	ctl->cmsg(CMSG_INFO, VERB_DEBUG_SILLY, " * Excluded");
	continue;
    } else
	done++;

    order = is_ordered(current_sfrec, bank, preset, keynote);
    if(order < 0)
	order = current_sfrec->def_order;

    addr = INSTHASH(bank, preset, keynote);

    for(ip = current_sfrec->instlist[addr]; ip; ip = ip->next)
    {
	if(ip->pat.bank == bank && ip->pat.preset == preset &&
	   (keynote < 0 || keynote == ip->pat.keynote))
	    break;
    }

    if(ip == NULL)
    {
	ip = (InstList*)SFMalloc(current_sfrec, sizeof(InstList));
	memset(ip, 0, sizeof(InstList));
	ip->pr_idx = pridx;
	ip->pat.bank = bank;
	ip->pat.preset = preset;
	ip->pat.keynote = keynote;
	ip->order = order;
	ip->samples = 0;
	ip->slist = NULL;
	ip->next = current_sfrec->instlist[addr];
	current_sfrec->instlist[addr] = ip;
    }

    /* new sample */
    sp = (SampleList *)SFMalloc(current_sfrec, sizeof(SampleList));
    memset(sp, 0, sizeof(SampleList));

	sp->bank = bank;
	sp->keynote = keynote;

	if(tbl->set[SF_keynum]) {
		sp->v.note_to_use = (int)tbl->val[SF_keynum];
	} else if(bank == 128) {
		sp->v.note_to_use = keynote;
	}
    make_info(sf, sp, tbl);

    /* add a sample */
    if(ip->slist == NULL)
	ip->slist = sp;
    else
    {
	SampleList *cur, *prev;
	int32 start;

	/* Insert sample */
	start = sp->start;
	cur = ip->slist;
	prev = NULL;
	while(cur && cur->start <= start)
	{
	    prev = cur;
	    cur = cur->next;
	}
	if(prev == NULL)
	{
	    sp->next = ip->slist;
	    ip->slist = sp;
	}
	else
	{
	    prev->next = sp;
	    sp->next = cur;
	}
    }
    ip->samples++;

	} /* for(;;) */


	if(done==0)
	return AWE_RET_SKIP;
	else
	return AWE_RET_OK;
}

/*----------------------------------------------------------------
 *
 * Modified for TiMidity
 */

/* conver to Sample parameter */
static void make_info(SFInfo *sf, SampleList *vp, LayerTable *tbl)
{
	set_sample_info(sf, vp, tbl);
	set_init_info(sf, vp, tbl);
	set_rootkey(sf, vp, tbl);
	set_rootfreq(vp);

	/* tremolo & vibrato */
#ifndef SF_SUPPRESS_TREMOLO
	convert_tremolo(vp, tbl);
#endif /* SF_SUPPRESS_TREMOLO */

#ifndef SF_SUPPRESS_VIBRATO
	convert_vibrato(vp, tbl);
#endif /* SF_SUPPRESS_VIBRATO */
}

static void set_envelope_parameters(SampleList *vp)
{
		/* convert envelope parameters */
		vp->v.envelope_offset[0] = to_offset(65535);
		vp->v.envelope_rate[0] = vp->attack;

		vp->v.envelope_offset[1] = to_offset(65534);
		vp->v.envelope_rate[1] = vp->hold;

		vp->v.envelope_offset[2] = to_offset(vp->sustain);
		vp->v.envelope_rate[2] = vp->decay;

		vp->v.envelope_offset[3] = 0;
		vp->v.envelope_rate[3] = vp->release;

		vp->v.envelope_offset[4] = 0;
		vp->v.envelope_rate[4] = vp->release;

		vp->v.envelope_offset[5] = 0;
		vp->v.envelope_rate[5] = vp->release;

		/* convert modulation envelope parameters */
		vp->v.modenv_offset[0] = to_offset(65535);
		vp->v.modenv_rate[0] = vp->modattack;

		vp->v.modenv_offset[1] = to_offset(65534);
		vp->v.modenv_rate[1] = vp->modhold;

		vp->v.modenv_offset[2] = to_offset(vp->modsustain);
		vp->v.modenv_rate[2] = vp->moddecay;

		vp->v.modenv_offset[3] = 0;
		vp->v.modenv_rate[3] = vp->modrelease;

		vp->v.modenv_offset[4] = 0;
		vp->v.modenv_rate[4] = vp->modrelease;

		vp->v.modenv_offset[5] = 0;
		vp->v.modenv_rate[5] = vp->modrelease;
}

/* set sample address */
static void set_sample_info(SFInfo *sf, SampleList *vp, LayerTable *tbl)
{
    SFSampleInfo *sp = &sf->sample[tbl->val[SF_sampleId]];

    /* set sample position */
    vp->start = (tbl->val[SF_startAddrsHi] << 15)
	+ tbl->val[SF_startAddrs]
	+ sp->startsample;
    vp->len = (tbl->val[SF_endAddrsHi] << 15)
	+ tbl->val[SF_endAddrs]
	+ sp->endsample - vp->start;

	vp->start = abs(vp->start);
	vp->len = abs(vp->len);

    /* set loop position */
    vp->v.loop_start = (tbl->val[SF_startloopAddrsHi] << 15)
	+ tbl->val[SF_startloopAddrs]
	+ sp->startloop - vp->start;
    vp->v.loop_end = (tbl->val[SF_endloopAddrsHi] << 15)
	+ tbl->val[SF_endloopAddrs]
	+ sp->endloop - vp->start;

    /* set data length */
    vp->v.data_length = vp->len + 1;
    if (vp->v.loop_end > vp->len + 1)
	vp->v.loop_end = vp->len + 1;
	if (vp->v.loop_start > vp->len)
	vp->v.loop_start = vp->len;

    /* Sample rate */
	if(sp->samplerate > 50000) {sp->samplerate = 50000;}
	else if(sp->samplerate < 400) {sp->samplerate = 400;}
    vp->v.sample_rate = sp->samplerate;

    /* sample mode */
    vp->v.modes = MODES_16BIT;

    /* volume envelope & total volume */
    vp->v.volume = calc_volume(tbl) * current_sfrec->amptune;

#ifndef SF_SUPPRESS_ENVELOPE
	convert_volume_envelope(vp, tbl);
#endif /* SF_SUPPRESS_ENVELOPE */
	set_envelope_parameters(vp);

    if(tbl->val[SF_sampleFlags] == 1 || tbl->val[SF_sampleFlags] == 3)
    {
		/* looping */
		vp->v.modes |= MODES_LOOPING | MODES_SUSTAIN;
		if(tbl->val[SF_sampleFlags] == 3)
			vp->v.data_length = vp->v.loop_end; /* strip the tail */
	}
    else
    {
		/* set a small blank loop at the tail for avoiding abnormal loop. */
		vp->v.loop_start = vp->len;
		vp->v.loop_end = vp->len + 1;
    }

    /* convert to fractional samples */
    vp->v.data_length <<= FRACTION_BITS;
    vp->v.loop_start <<= FRACTION_BITS;
    vp->v.loop_end <<= FRACTION_BITS;

    /* point to the file position */
    vp->start = vp->start * 2 + sf->samplepos;
    vp->len *= 2;

	vp->v.vel_to_fc = -2400;	/* SF2 default value */
	vp->v.key_to_fc = vp->v.vel_to_resonance = 0;
	vp->v.envelope_velf_bpo = vp->v.modenv_velf_bpo =
		vp->v.vel_to_fc_threshold = 64;
	vp->v.key_to_fc_bpo = 60;
	memset(vp->v.envelope_velf, 0, sizeof(vp->v.envelope_velf));
	memset(vp->v.modenv_velf, 0, sizeof(vp->v.modenv_velf));

	vp->v.inst_type = INST_SF2;
}

/*----------------------------------------------------------------*/

/* set global information */
static void set_init_info(SFInfo *sf, SampleList *vp, LayerTable *tbl)
{
    int val;
    SFSampleInfo *sample;
    sample = &sf->sample[tbl->val[SF_sampleId]];

    /* key range */
    if(tbl->set[SF_keyRange])
    {
	vp->low = LOWNUM(tbl->val[SF_keyRange]);
	vp->high = HIGHNUM(tbl->val[SF_keyRange]);
    }
    else
    {
	vp->low = 0;
	vp->high = 127;
    }
    vp->v.low_freq = freq_table[(int)vp->low];
    vp->v.high_freq = freq_table[(int)vp->high];

    /* velocity range */
    if(tbl->set[SF_velRange]) {
		vp->v.low_vel = LOWNUM(tbl->val[SF_velRange]);
		vp->v.high_vel = HIGHNUM(tbl->val[SF_velRange]);
    } else {
		vp->v.low_vel = 0;
		vp->v.high_vel = 127;
	}

    /* fixed key & velocity */
    if(tbl->set[SF_keynum])
		vp->v.note_to_use = (int)tbl->val[SF_keynum];
	if(tbl->set[SF_velocity] && (int)tbl->val[SF_velocity] != 0) {
		ctl->cmsg(CMSG_INFO,VERB_DEBUG,"error: fixed-velocity is not supported.");
	}

	vp->v.sample_type = sample->sampletype;
	vp->v.sf_sample_index = tbl->val[SF_sampleId];
	if (sample->sampletype == SF_SAMPLETYPE_MONO) {
		vp->v.sf_sample_link = -1;
	} else {
		vp->v.sf_sample_link = sample->samplelink;
	}

	/* panning position: 0 to 127 */
	val = (int)tbl->val[SF_panEffectsSend];
    if(sample->sampletype == SF_SAMPLETYPE_MONO || val != 0) {	/* monoSample = 1 */
		if(val < -500)
		vp->v.panning = 0;
		else if(val > 500)
		vp->v.panning = 127;
		else
		vp->v.panning = (int8)((val + 500) * 127 / 1000);
	} else if(sample->sampletype == SF_SAMPLETYPE_RIGHT) {	/* rightSample = 2 */
		vp->v.panning = 127;
	} else if(sample->sampletype == SF_SAMPLETYPE_LEFT) {	/* leftSample = 4 */
		vp->v.panning = 0;
	} else if(sample->sampletype == SF_SAMPLETYPE_LINKED) {	/* linkedSample = 8 */
		ctl->cmsg(CMSG_ERROR,VERB_NOISY,"error: linkedSample is not supported.");
	}

	memset(vp->v.envelope_keyf, 0, sizeof(vp->v.envelope_keyf));
	memset(vp->v.modenv_keyf, 0, sizeof(vp->v.modenv_keyf));
	if(tbl->set[SF_autoHoldEnv2]) {
		vp->v.envelope_keyf[1] = (int16)tbl->val[SF_autoHoldEnv2];
	}
	if(tbl->set[SF_autoDecayEnv2]) {
		vp->v.envelope_keyf[2] = (int16)tbl->val[SF_autoDecayEnv2];
	}
	if(tbl->set[SF_autoHoldEnv1]) {
		vp->v.modenv_keyf[1] = (int16)tbl->val[SF_autoHoldEnv1];
	}
	if(tbl->set[SF_autoDecayEnv1]) {
		vp->v.modenv_keyf[2] = (int16)tbl->val[SF_autoDecayEnv1];
	}

#ifndef CFG_FOR_SF
	current_sfrec->def_cutoff_allowed = 1;
	current_sfrec->def_resonance_allowed = 1;
#endif

    /* initial cutoff & resonance */
    vp->cutoff_freq = 0;
    if((int)tbl->val[SF_initialFilterFc] < 0)
	tbl->set[SF_initialFilterFc] = tbl->val[SF_initialFilterFc] = 0;
    if(current_sfrec->def_cutoff_allowed && tbl->set[SF_initialFilterFc]
		&& (int)tbl->val[SF_initialFilterFc] >= 1500 && (int)tbl->val[SF_initialFilterFc] <= 13500)
    {
    val = (int)tbl->val[SF_initialFilterFc];
	val = abscent_to_Hz(val);

#ifndef CFG_FOR_SF
	if(!opt_modulation_envelope) {
		if(tbl->set[SF_env1ToFilterFc] && (int)tbl->val[SF_env1ToFilterFc] > 0)
		{
			val *= pow(2.0,(double)tbl->val[SF_env1ToFilterFc] / 1200.0f);
			if(val > 20000) {val = 20000;}
		}
	}
#endif

	vp->cutoff_freq = val;
    }
	vp->v.cutoff_freq = vp->cutoff_freq;

	vp->resonance = 0;
    if(current_sfrec->def_resonance_allowed && tbl->set[SF_initialFilterQ])
    {
	val = (int)tbl->val[SF_initialFilterQ];
	vp->resonance = val;
	}
	vp->v.resonance = vp->resonance;

#if 0 /* Not supported */
    /* exclusive class key */
    vp->exclusiveClass = tbl->val[SF_keyExclusiveClass];
#endif
}

static int abscent_to_Hz(int abscents)
{
	return (int)(8.176 * pow(2.0, (double)abscents / 1200.0));
}

/*----------------------------------------------------------------*/

#define SF_MODENV_CENT_MAX 1200	/* Live! allows only +-1200cents. */

/* calculate root key & fine tune */
static void set_rootkey(SFInfo *sf, SampleList *vp, LayerTable *tbl)
{
	SFSampleInfo *sp = &sf->sample[tbl->val[SF_sampleId]];
	int temp;
	
	/* scale factor */
	vp->v.scale_factor = 1024 * (double) tbl->val[SF_scaleTuning] / 100 + 0.5;
	/* set initial root key & fine tune */
	if (sf->version == 1 && tbl->set[SF_samplePitch]) {
		/* set from sample pitch */
		vp->root = tbl->val[SF_samplePitch] / 100;
		vp->tune = -tbl->val[SF_samplePitch] % 100;
		if (vp->tune <= -50)
			vp->root++, vp->tune += 100;
	} else {
		/* from sample info */
		vp->root = sp->originalPitch;
		vp->tune = (int8) sp->pitchCorrection;
	}
	/* orverride root key */
	if (tbl->set[SF_rootKey])
		vp->root = tbl->val[SF_rootKey];
	else if (vp->bank == 128 && vp->v.scale_factor != 0)
		vp->tune += (vp->keynote - sp->originalPitch)
				* 100 * (double) vp->v.scale_factor / 1024;
	vp->tune += tbl->val[SF_coarseTune] * 100 + tbl->val[SF_fineTune];
	/* correct too high pitch */
	if (vp->root >= vp->high + 60)
		vp->root -= 60;
	vp->v.tremolo_to_pitch =
			(tbl->set[SF_lfo1ToPitch]) ? tbl->val[SF_lfo1ToPitch] : 0;
	vp->v.tremolo_to_fc =
			(tbl->set[SF_lfo1ToFilterFc]) ? tbl->val[SF_lfo1ToFilterFc] : 0;
	vp->v.modenv_to_pitch =
			(tbl->set[SF_env1ToPitch]) ? tbl->val[SF_env1ToPitch] : 0;
	/* correct tune with the sustain level of modulation envelope */
	temp = vp->v.modenv_to_pitch
			* (double) (1000 - tbl->val[SF_sustainEnv1]) / 1000 + 0.5;
	vp->tune += temp, vp->v.modenv_to_pitch -= temp;
	vp->v.modenv_to_fc =
			(tbl->set[SF_env1ToFilterFc]) ? tbl->val[SF_env1ToFilterFc] : 0;
}

static void set_rootfreq(SampleList *vp)
{
	int root = vp->root;
	int tune = 0.5 - 256 * (double) vp->tune / 100;
	
	/* 0 <= tune < 255 */
	while (tune < 0)
		root--, tune += 256;
	while (tune > 255)
		root++, tune -= 256;
	if (root < 0) {
		vp->v.root_freq = freq_table[0] * (double) bend_fine[tune]
				/ bend_coarse[-root] + 0.5;
		vp->v.scale_freq = 0;		/* scale freq */
	} else if (root > 127) {
		vp->v.root_freq = freq_table[127] * (double) bend_fine[tune]
				* bend_coarse[root - 127] + 0.5;
		vp->v.scale_freq = 127;		/* scale freq */
	} else {
		vp->v.root_freq = freq_table[root] * (double) bend_fine[tune] + 0.5;
		vp->v.scale_freq = root;	/* scale freq */
	}
}

/*----------------------------------------------------------------*/


/*Pseudo Reverb*/
extern int32 modify_release;


/* volume envelope parameters */
static void convert_volume_envelope(SampleList *vp, LayerTable *tbl)
{
    vp->attack  = to_rate(65535, tbl->val[SF_attackEnv2]);
    vp->hold    = to_rate(1, tbl->val[SF_holdEnv2]);
    vp->sustain = calc_sustain(tbl->val[SF_sustainEnv2]);
    vp->decay   = to_rate(65533 - vp->sustain, tbl->val[SF_decayEnv2]);
    if(modify_release) /* Pseudo Reverb */
	vp->release = calc_rate(65535, modify_release);
    else
	vp->release = to_rate(65535, tbl->val[SF_releaseEnv2]);
	vp->v.envelope_delay = play_mode->rate * 
		to_msec(tbl->val[SF_delayEnv2]) * 0.001;

	/* convert modulation envelope */
    vp->modattack  = to_rate(65535, tbl->val[SF_attackEnv1]);
    vp->modhold    = to_rate(1, tbl->val[SF_holdEnv1]);
    vp->modsustain = calc_sustain(tbl->val[SF_sustainEnv1]);
    vp->moddecay   = to_rate(65533 - vp->modsustain, tbl->val[SF_decayEnv1]);
    if(modify_release) /* Pseudo Reverb */
	vp->modrelease = calc_rate(65535, modify_release);
    else
	vp->modrelease = to_rate(65535, tbl->val[SF_releaseEnv1]);
	vp->v.modenv_delay = play_mode->rate * 
		to_msec(tbl->val[SF_delayEnv1]) * 0.001;

    vp->v.modes |= MODES_ENVELOPE;
}


#ifndef SF_SUPPRESS_TREMOLO
/*----------------------------------------------------------------
 * tremolo (LFO1) conversion
 *----------------------------------------------------------------*/

static void convert_tremolo(SampleList *vp, LayerTable *tbl)
{
    int32 freq;
	double level;

    if(!tbl->set[SF_lfo1ToVolume])
	return;

    level = pow(10.0, (double)abs(tbl->val[SF_lfo1ToVolume]) / -200.0);
    vp->v.tremolo_depth = 256 * (1.0 - level);
	if((int)tbl->val[SF_lfo1ToVolume] < 0) {vp->v.tremolo_depth = -vp->v.tremolo_depth;}

    /* frequency in mHz */
    if(!tbl->set[SF_freqLfo1])
	freq = 0;
    else
    {
	freq = (int)tbl->val[SF_freqLfo1];
	freq = TO_MHZ(freq);
    }

    /* convert mHz to sine table increment; 1024<<rate_shift=1wave */
    vp->v.tremolo_phase_increment = ((play_mode->rate / 1000 * freq) >> RATE_SHIFT) / control_ratio;
    vp->v.tremolo_delay = play_mode->rate * 
		to_msec(tbl->val[SF_delayLfo1]) * 0.001;
}
#endif

#ifndef SF_SUPPRESS_VIBRATO
/*----------------------------------------------------------------
 * vibrato (LFO2) conversion
 *----------------------------------------------------------------*/

static void convert_vibrato(SampleList *vp, LayerTable *tbl)
{
    int32 shift, freq;

    if(!tbl->set[SF_lfo2ToPitch]) {
		vp->v.vibrato_control_ratio = 0;
		return;
	}

    shift = (int)tbl->val[SF_lfo2ToPitch];

    /* cents to linear; 400cents = 256 */
    shift = shift * 256 / 400;
	if(shift > 255) {shift = 255;}
    else if(shift < -255) {shift = -255;}
    vp->v.vibrato_depth = (int16)shift;

    /* frequency in mHz */
    if(!tbl->set[SF_freqLfo2])
	freq = 0;
    else
    {
	freq = (int)tbl->val[SF_freqLfo2];
	freq = TO_MHZ(freq);
	if(freq == 0) {freq = 1;}
	/* convert mHz to control ratio */
	vp->v.vibrato_control_ratio = (1000 * play_mode->rate) /
			(freq * 2 * VIBRATO_SAMPLE_INCREMENTS);
    }

    vp->v.vibrato_delay = play_mode->rate * 
		to_msec(tbl->val[SF_delayLfo2]) * 0.001;
}
#endif

#ifdef CFG_FOR_SF

/*********************************************************************

    cfg for soundfont utility.

  demanded sources.
     common.c  controls.c  dumb_c.c  instrum.c  sbkconv.c  sffile.c
     sfitem.c  sndfont.c  tables.c  version.c
		 mt19937ar.c quantity.c smplfile.c
     utils/  libarc/
  additional sources (for -f option)
     filter.c  freq.c  resample.c

  MACRO
      CFG_FOR_SF

 *********************************************************************/

#include "freq.h"

#define CFG_FOR_SF_SUPPORT_FFT	1

static FILE *x_out;
static char x_sf_file_name[1024];
static int x_pre_bank = -1;
static int x_pre_preset = -1;
static int x_sort = 1;
static int x_comment = 1;
#if CFG_FOR_SF_SUPPORT_FFT
static int x_fft = 0;
#endif

typedef struct x_cfg_info_t_ {
	char m_bank[128][128];
	char m_preset[128][128];
	char m_rom[128][128];
	char *m_str[128][128];
	char d_preset[128][128];
	char d_keynote[128][128];
	char d_rom[128][128];
	char *d_str[128][128];
} x_cfg_info_t;

static x_cfg_info_t x_cfg_info;
static int x_cfg_info_init_flag = 0;

static void x_cfg_info_init(void)
{
	if(!x_cfg_info_init_flag){
		int i,j;
		for(i=0;i<128;i++){
			for(j=0;j<128;j++){
				x_cfg_info.m_bank[i][j] = -1;
				x_cfg_info.m_preset[i][j] = -1;
				x_cfg_info.m_rom[i][j] = -1;
				x_cfg_info.m_str[i][j] = NULL;
				x_cfg_info.d_preset[i][j] = -1;
				x_cfg_info.d_keynote[i][j] = -1;
				x_cfg_info.d_rom[i][j] = -1;
				x_cfg_info.d_str[i][j] = NULL;
			}
		}
	}
	x_cfg_info_init_flag = 1;
}

static int cfg_for_sf_scan(char *x_name, int x_bank, int x_preset, int x_keynote_from, int x_keynote_to, int romflag)
{
	int x_keynote;
	x_cfg_info_init();
	if(x_sort){
		/*if(x_bank!=x_pre_bank || x_preset!=x_pre_preset){*/
		{
			char *str;
			char buff[256];
			if(x_bank==128){
				for(x_keynote=x_keynote_from;x_keynote<=x_keynote_from;x_keynote++){
					x_cfg_info.d_preset[x_preset][x_keynote] = x_preset;
					x_cfg_info.d_keynote[x_preset][x_keynote] = x_keynote;
					if(romflag && x_cfg_info.d_rom[x_preset][x_keynote])
						x_cfg_info.d_rom[x_preset][x_keynote] = 1;
					else
						x_cfg_info.d_rom[x_preset][x_keynote] = 0;
					str = x_cfg_info.d_str[x_preset][x_keynote];
					str = (char *)safe_realloc(str,(str==NULL?0:strlen(str))+strlen(x_name)+30);
					if(x_cfg_info.d_str[x_preset][x_keynote]==NULL){
						str[0] = '\0';
					}
					sprintf(buff," %s",x_name);
					strcat(str,buff);
					x_cfg_info.d_str[x_preset][x_keynote] = str;
				}
			} else {
				char *strROM;
				str = x_cfg_info.m_str[x_bank][x_preset];
				x_cfg_info.m_bank[x_bank][x_preset] = x_bank;
				x_cfg_info.m_preset[x_bank][x_preset] = x_preset;
				if(romflag)
					strROM = " (ROM)";
				else
					strROM = "";
				if(romflag && x_cfg_info.m_rom[x_bank][x_preset])
					x_cfg_info.m_rom[x_bank][x_preset] = 1;
				else
					x_cfg_info.m_rom[x_bank][x_preset] = 0;
				str = (char *)safe_realloc(str,(str==NULL?0:strlen(str))+strlen(x_name)+30);
				if(x_cfg_info.m_str[x_bank][x_preset]==NULL){
					str[0] = '\0';
				}
				if(x_comment){
					if(x_keynote_from!=x_keynote_to)
						sprintf(buff,"        # %d-%d:%s%s\n",x_keynote_from,x_keynote_to,x_name,strROM);
					else
						sprintf(buff,"        # %d:%s%s\n",x_keynote_from,x_name,strROM);
					strcat(str,buff);
				}
				x_cfg_info.m_str[x_bank][x_preset] = str;
			}
		}
	} else {
		if(x_bank==128){
			if(x_preset!=x_pre_preset)
				fprintf(x_out,"drumset %d\n",x_preset);
		} else {
			if(x_bank!=x_pre_bank)
				fprintf(x_out,"bank %d\n",x_bank);
		}
		if(romflag){
			if(x_bank==128){
				for(x_keynote=x_keynote_from;x_keynote<=x_keynote_from;x_keynote++){
					if(x_comment)
						fprintf(x_out,"#  %d %%font %s %d %d %d # %s (ROM)\n",x_keynote,x_sf_file_name,x_bank,x_preset,x_keynote,x_name);
					else
						fprintf(x_out,"#  %d %%font %s %d %d %d # (ROM)\n",x_keynote,x_sf_file_name,x_bank,x_preset,x_keynote);
				}
			} else {
				if(x_keynote_from==x_keynote_to) {
					if(x_comment)
						fprintf(x_out,"#   %d %%font %s %d %d # %d:%s (ROM)\n",x_preset,x_sf_file_name,x_bank,x_preset,x_keynote_from,x_name);
					else
						fprintf(x_out,"#   %d %%font %s %d %d # (ROM)\n",x_preset,x_sf_file_name,x_bank,x_preset);
				} else {
					if(x_comment)
						fprintf(x_out,"#   %d %%font %s %d %d # %d-%d:%s (ROM)\n",x_preset,x_sf_file_name,x_bank,x_preset,x_keynote_from,x_keynote_to,x_name);
					else
						fprintf(x_out,"#   %d %%font %s %d %d # (ROM)\n",x_preset,x_sf_file_name,x_bank,x_preset);
				}
			}
		} else {
			if(x_bank==128){
				for(x_keynote=x_keynote_from;x_keynote<=x_keynote_from;x_keynote++){
					if(x_comment)
						fprintf(x_out,"    %d %%font %s %d %d %d # %s\n",x_keynote,x_sf_file_name,x_bank,x_preset,x_keynote,x_name);
					else
						fprintf(x_out,"    %d %%font %s %d %d %d\n",x_keynote,x_sf_file_name,x_bank,x_preset,x_keynote);
				}
			} else {
				if(x_keynote_from==x_keynote_to){
					if(x_comment)
						fprintf(x_out,"    %d %%font %s %d %d # %d:%s\n",x_preset,x_sf_file_name,x_bank,x_preset,x_keynote_from,x_name);
					else
						fprintf(x_out,"    %d %%font %s %d %d\n",x_preset,x_sf_file_name,x_bank,x_preset);
				} else {
					if(x_comment)
						fprintf(x_out,"    %d %%font %s %d %d # %d-%d:%s\n",x_preset,x_sf_file_name,x_bank,x_preset,x_keynote_from,x_keynote_to,x_name);
					else
						fprintf(x_out,"    %d %%font %s %d %d\n",x_preset,x_sf_file_name,x_bank,x_preset);
				}
			}
		}
	}
	x_pre_bank = x_bank;
	x_pre_preset = x_preset;
	return 0;
}

PlayMode dpm = {
    DEFAULT_RATE, PE_16BIT|PE_SIGNED, PF_PCM_STREAM,
    -1,
    {0,0,0,0,0},
    "null", 'n',
    NULL,
		NULL,
		NULL,
		NULL,
		NULL
};
PlayMode *play_mode = &dpm;
#if !CFG_FOR_SF_SUPPORT_FFT
int32 freq_table[1];
FLOAT_T bend_fine[1];
FLOAT_T bend_coarse[1];
void pre_resample(Sample *sp) {}
void antialiasing(int16 *data, int32 data_length,int32 sample_rate, int32 output_rate) {}
#endif

char *wrdt = NULL; /* :-P */

#ifdef WIN32
static int ctl_open(int using_stdin, int using_stdout) { return 0;}
static void ctl_close(void) {}
static int ctl_read(int32 *valp) { return 0; } 
#include <stdarg.h>
static int cmsg(int type, int verbosity_level, char *fmt, ...)
{
  va_list ap;
  if ((type==CMSG_TEXT || type==CMSG_INFO || type==CMSG_WARNING) &&
      ctl->verbosity<verbosity_level)
    return 0;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  fputs(NLS, stderr);
  va_end(ap);
  return 0;
}
static void ctl_event(CtlEvent *e) {}
ControlMode w32gui_control_mode =
{
	"w32gui interface", 'd',
    1,0,0,
    0,
    ctl_open,
    ctl_close,
    dumb_pass_playing_list,
    ctl_read,
    cmsg,
    ctl_event
};
#endif
extern struct URL_module URL_module_file;
#ifndef __MACOS__
extern struct URL_module URL_module_dir;
#endif /* __MACOS__ */
#ifdef SUPPORT_SOCKET
extern struct URL_module URL_module_http;
extern struct URL_module URL_module_ftp;
extern struct URL_module URL_module_news;
extern struct URL_module URL_module_newsgroup;
#endif /* SUPPORT_SOCKET */
#ifdef HAVE_POPEN
extern struct URL_module URL_module_pipe;
#endif /* HAVE_POPEN */
static struct URL_module *url_module_list[] =
{
    &URL_module_file,
#ifndef __MACOS__
    &URL_module_dir,
#endif /* __MACOS__ */
#ifdef SUPPORT_SOCKET
    &URL_module_http,
    &URL_module_ftp,
    &URL_module_news,
    &URL_module_newsgroup,
#endif /* SUPPORT_SOCKET */
#if !defined(__MACOS__) && defined(HAVE_POPEN)
    &URL_module_pipe,
#endif
#if defined(main) || defined(ANOTHER_MAIN)
    /* You can put some other modules */
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
#endif /* main */
    NULL
};

static void cfgforsf_usage(const char *program_name, int status)
{
	printf(
"USAGE: %s [options] soundfont [cfg_output]\n"
"  Options:\n"
"    -s: sort by bank/program. (default)\n"
"    -S or -s-: do not sort by bank/program.\n"
"    -c: output comment. (default)\n"
"    -C or -c-: do not output comment.\n"
#if CFG_FOR_SF_SUPPORT_FFT
"    -f drumset note: calculate sample frequency.\n"
"    -F drumset note: do not calculate sample frequency. (default = -F - -)\n"
#endif
		, program_name );
	exit(status);
}

#if CFG_FOR_SF_SUPPORT_FFT
#define SET_PROG_MAP(map, bank, prog)		((map)[(bank << (7 - 3)) | (prog >> 3)] |= (1 << (prog & 7)))
#define UNSET_PROG_MAP(map, bank, prog)		((map)[(bank << (7 - 3)) | (prog >> 3)] &= ~(1 << (prog & 7)))
#define IS_SET_PROG_MAP(map, bank, prog)	((map)[(bank << (7 - 3)) | (prog >> 3)] & (1 << (prog & 7)))
#endif

#if CFG_FOR_SF_SUPPORT_FFT
int check_apply_control(void) { return 0; } // not pass
void dumb_pass_playing_list(int number_of_files, char *list_of_files[]) {}
void recompute_freq(int v) {} // not pass
int32 control_ratio = 0;
int reduce_quality_flag = 0;
Voice *voice = NULL;
Channel channel[MAX_CHANNELS];
// from playmidi.c
int32 get_note_freq(Sample *sp, int note)
{
	int32 f;
	int16 sf, sn;
	double ratio;
	
	f = freq_table[note];
	/* GUS/SF2 - Scale Tuning */
	if ((sf = sp->scale_factor) != 1024) {
		sn = sp->scale_freq;
		ratio = pow(2.0, (note - sn) * (sf - 1024) / 12288.0);
		f = f * ratio + 0.5;
	}
	return f;
}
#endif

int main(int argc, char **argv)
{
    SFInsts *sf;
	int i, x_bank, x_preset, x_keynote;
	int initial = 0;
	#if CFG_FOR_SF_SUPPORT_FFT
	uint8 fft_range[128 * 128 / 8];
	int8 x_playnote[128 + 1];
	char playnote_str[272], *p;	/* ,0,1,3,4,6,7,9,10,...,126,127 */
	#endif
	const char *program_name;

	argc--, program_name = *argv++;
	#if CFG_FOR_SF_SUPPORT_FFT
	memset(fft_range, 0, sizeof fft_range);
	#endif
	while(argc > 0 && argv[0][0] == '-' && isalpha(argv[0][1])) {
		int opt_switch = 1;
		if(argv[0][2] == '\0')
			opt_switch = 1;
		else if(argv[0][2] == '-' && argv[0][3] == '\0')
			opt_switch = 0;
		else
			break;
		switch(argv[0][1]) {
			case 's':
				x_sort = 1; if(!opt_switch) x_sort = 0; break;
			case 'S':
				x_sort = 0; break;
			case 'c':
				x_comment = 1; if(!opt_switch) x_comment = 0; break;
			case 'C':
				x_comment = 0; break;
			#if CFG_FOR_SF_SUPPORT_FFT
			case 'f': case 'F':
				if(argc < 3)
					cfgforsf_usage(program_name, EXIT_FAILURE);
				else {
					const char *mask_string;
					int8 mask[128];
					int flag, start, end;
					
					flag = argv[0][1] == 'f';
					memset(mask, 0, sizeof mask);
					mask_string = argv[2];
					do {
						if(string_to_7bit_range(mask_string, &start, &end)) {
							for(i = start; i <= end; i++)
								mask[i] = 1;
						}
						mask_string = strchr(mask_string, ',');
					} while(mask_string++ != NULL);
					mask_string = argv[1];
					do {
						if(string_to_7bit_range(mask_string, &start, &end)) {
							x_fft = 1;
							while(start <= end) {
								for(i = 0; i < 128; i++) {
									if (!mask[i])
										continue;
									if(flag)
										SET_PROG_MAP(fft_range, start, i);
									else
										UNSET_PROG_MAP(fft_range, start, i);
								}
								start++;
							}
						}
						mask_string = strchr(mask_string, ',');
					} while(mask_string++ != NULL);
					argc -= 2, argv += 2;
				}
				break;
			#endif
			default:
				fprintf(stderr, "Error: Invalid option %s.\n", argv[0]);
				cfgforsf_usage(program_name, EXIT_FAILURE);
		}
		argc--, argv++;
	}
	if(argc <= 0 || argc > 2)
		cfgforsf_usage(program_name, EXIT_SUCCESS);
	x_out = (argc < 2) ? stdout : fopen(argv[1], "w");
	if (x_out == NULL) {
		fprintf(stderr, "Error: Unable to open %s.\n", argv[1]);
		exit(EXIT_FAILURE);
	}
	#if CFG_FOR_SF_SUPPORT_FFT
	if (!x_sort && x_fft) {
		fprintf(stderr, "Error: -f option should be used with -s option.\n");
		exit(EXIT_FAILURE);
	}
	memset(x_playnote, -1, sizeof x_playnote);
	#endif
	ctl->verbosity = -1;
#ifdef SUPPORT_SOCKET
	/*init_mail_addr();*/
	if(url_user_agent == NULL){
	    url_user_agent = (char *)safe_malloc(10 + strlen(timidity_version));
	    strcpy(url_user_agent, "TiMidity-");
	    strcat(url_user_agent, timidity_version);
	}
#endif /* SUPPORT_SOCKET */
	for(i = 0; url_module_list[i]; i++)
	    url_add_module(url_module_list[i]);
	init_freq_table();
	init_bend_fine();
	init_bend_coarse();
	initialize_resampler_coeffs();
	control_ratio = play_mode->rate / CONTROLS_PER_SECOND;
	strncpy(x_sf_file_name, argv[0], 1024);
    sf = new_soundfont(x_sf_file_name);
    sf->next = NULL;
    sf->def_order = 2;
    sfrecs = sf;
	x_cfg_info_init();
	init_sf(sf);
	if(strchr(x_sf_file_name, ' ') != NULL) {
		char quote = strchr(x_sf_file_name, '"') == NULL ? '"' : '\'';
		sprintf(x_sf_file_name, "%c%s%c", quote, argv[0], quote);
	}
	if(x_sort){
	for(x_bank=0;x_bank<=127;x_bank++){
		int flag = 0;
		for(x_preset=0;x_preset<=127;x_preset++){
			if(x_cfg_info.m_bank[x_bank][x_preset] >= 0 && x_cfg_info.m_preset[x_bank][x_preset] >= 0){
				flag = 1;
			}
		}
		if(!flag)
			continue;
		if(!initial){
			initial = 1;
			fprintf(x_out,"bank %d\n",x_bank);
		} else
			fprintf(x_out,"\nbank %d\n",x_bank);
		for(x_preset=0;x_preset<=127;x_preset++){
			if(x_cfg_info.m_bank[x_bank][x_preset] >= 0 && x_cfg_info.m_preset[x_bank][x_preset] >= 0){
				if(x_cfg_info.m_rom[x_bank][x_preset])
					fprintf(x_out,"#   %d %%font %s %d %d # (ROM)\n%s",x_preset,x_sf_file_name,x_cfg_info.m_bank[x_bank][x_preset],x_cfg_info.m_preset[x_bank][x_preset],x_cfg_info.m_str[x_bank][x_preset]);
				else
					fprintf(x_out,"    %d %%font %s %d %d\n%s",x_preset,x_sf_file_name,x_cfg_info.m_bank[x_bank][x_preset],x_cfg_info.m_preset[x_bank][x_preset],x_cfg_info.m_str[x_bank][x_preset]);
			}
		}
	}
	for(x_preset=0;x_preset<=127;x_preset++){
		int flag = 0, start;
		for(x_keynote=0;x_keynote<=127;x_keynote++){
			if(x_cfg_info.d_preset[x_preset][x_keynote] >= 0 && x_cfg_info.d_keynote[x_preset][x_keynote] >= 0){
				flag = 1;
			}
		}
		if(!flag)
			continue;
		if(!initial){
			initial = 1;
			fprintf(x_out,"drumset %d\n",x_preset);
		} else
			fprintf(x_out,"\ndrumset %d\n",x_preset);
		for(x_keynote=0;x_keynote<=127;x_keynote++){
			if(x_cfg_info.d_preset[x_preset][x_keynote] >= 0 && x_cfg_info.d_keynote[x_preset][x_keynote] >= 0){
				if(x_cfg_info.d_rom[x_preset][x_keynote]){
					if(x_comment)
						fprintf(x_out,"#   %d %%font %s 128 %d %d #%s (ROM)\n",x_keynote,x_sf_file_name,x_cfg_info.d_preset[x_preset][x_keynote],x_cfg_info.d_keynote[x_preset][x_keynote],x_cfg_info.d_str[x_preset][x_keynote]);
					else
						fprintf(x_out,"#   %d %%font %s 128 %d %d # (ROM)\n",x_keynote,x_sf_file_name,x_cfg_info.d_preset[x_preset][x_keynote],x_cfg_info.d_keynote[x_preset][x_keynote]);
				} else {
					if(x_comment)
						fprintf(x_out,"    %d %%font %s 128 %d %d #%s\n",x_keynote,x_sf_file_name,x_cfg_info.d_preset[x_preset][x_keynote],x_cfg_info.d_keynote[x_preset][x_keynote],x_cfg_info.d_str[x_preset][x_keynote]);
					else
						fprintf(x_out,"    %d %%font %s 128 %d %d\n",x_keynote,x_sf_file_name,x_cfg_info.d_preset[x_preset][x_keynote],x_cfg_info.d_keynote[x_preset][x_keynote]);
					#if CFG_FOR_SF_SUPPORT_FFT
					if(IS_SET_PROG_MAP(fft_range, x_preset, x_keynote)) {
						Instrument *inst;
						float freq;
						int chord, note;
						
						inst = try_load_soundfont(sf, -1, 128, x_preset, x_keynote);
						if(inst != NULL) {
							freq = freq_fourier(inst->sample, &chord);
							if (freq != 260) { /* 260 Hz is only returned when pitch is uncertain */
								x_playnote[x_keynote] = assign_pitch_to_freq(freq);
							}
							free_instrument(inst);
						}
					}
					#endif
				}
			}
		}
		#if CFG_FOR_SF_SUPPORT_FFT
		for(x_keynote = 0; x_keynote <= 127;) {
			if (x_playnote[x_keynote] == -1) {
				x_keynote++;
				continue;
			}
			p = playnote_str;
			flag = x_playnote[x_keynote];
			do {
				start = x_keynote;
				while (x_playnote[x_keynote] = -1, ++x_keynote <= 127) {
					if (x_playnote[x_keynote] != flag)
						break;
				}
				if (x_keynote - start == 1)
					p += sprintf(p, ",%d", start);
				else
					p += sprintf(p, ",%d%c%d", start, (x_keynote - start > 2) ? '-' : ',', x_keynote - 1);
				while (x_keynote <= 127 && x_playnote[x_keynote] == -1)
					x_keynote++;
			} while(x_keynote <= 127 && x_playnote[x_keynote] == flag);
			fprintf(x_out, "#extension playnote %s %d\n", &playnote_str[1], flag);
		}
		#endif
	}
	}
	if(x_out!=stdout)
		fclose(x_out);
	return 0;
}

#endif /* CFG_FOR_SF */
