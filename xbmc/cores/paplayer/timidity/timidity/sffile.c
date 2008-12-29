/*
    TiMidity++ -- MIDI to WAVE converter and player
    Copyright (C) 1999-2002 Masanao Izumo <mo@goice.co.jp>
    Copyright (C) 1995 Tuukka Toivonen <tt@cgs.fi>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

/*================================================================
 * sffile.c
 *	read SoundFont file (SBK/SF2) and store the layer lists
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

/*
 * Modified by Masanao Izumo <mo@goice.co.jp>
 */

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
#include "timidity.h"
#include "common.h"
#include "controls.h"
#include "sffile.h"

extern int progbase;

/*================================================================
 * preset / instrument bag record
 *================================================================*/

typedef struct _SFBags {
	int nbags;
	uint16 *bag;
	int ngens;
	SFGenRec *gen;
} SFBags;

static SFBags prbags, inbags;


/*----------------------------------------------------------------
 * function prototypes
 *----------------------------------------------------------------*/

#define NEW(type,nums)	(type*)safe_malloc(sizeof(type) * (nums))

static int READCHUNK(SFChunk *vp, struct timidity_file *tf)
{
    if(tf_read(vp, 8, 1, tf) != 1)
	return -1;
    vp->size = LE_LONG(vp->size);
    return 1;
}

static int READDW(uint32 *vp, struct timidity_file *tf)
{
    if(tf_read(vp, 4, 1, tf) != 1)
	return -1;
    *vp = LE_LONG(*vp);
    return 1;
}

static int READW(uint16 *vp, struct timidity_file *tf)
{
    if(tf_read(vp, 2, 1, tf) != 1)
	return -1;
    *vp = LE_SHORT(*vp);
    return 1;
}

static int READSTR(char *str, struct timidity_file *tf)
{
    int n;

    if(tf_read(str, 20, 1, tf) != 1)
	return -1;
    str[19] = '\0';
    n = strlen(str);
    while(n > 0 && str[n - 1] == ' ')
	n--;
    str[n] = '\0';
    return n;
}

#define READID(var,tf)	tf_read(var, 4, 1, tf)
#define READB(var,tf)	tf_read(&var, 1, 1, tf)
#define SKIPB(tf)	skip(tf, 1)
#define SKIPW(tf)	skip(tf, 2)
#define SKIPDW(tf)	skip(tf, 4)

#define FSKIP(size,tf)	skip(tf, size)


/*----------------------------------------------------------------*/

static int chunkid(char *id);
static int process_list(int size, SFInfo *sf, struct timidity_file *fd);
static int process_info(int size, SFInfo *sf, struct timidity_file *fd);
static int process_sdta(int size, SFInfo *sf, struct timidity_file *fd);
static int process_pdta(int size, SFInfo *sf, struct timidity_file *fd);
static void load_sample_names(int size, SFInfo *sf, struct timidity_file *fd);
static void load_preset_header(int size, SFInfo *sf, struct timidity_file *fd);
static void load_inst_header(int size, SFInfo *sf, struct timidity_file *fd);
static void load_bag(int size, SFBags *bagp, struct timidity_file *fd);
static void load_gen(int size, SFBags *bagp, struct timidity_file *fd);
static void load_sample_info(int size, SFInfo *sf, struct timidity_file *fd);
static void convert_layers(SFInfo *sf);
static void generate_layers(SFHeader *hdr, SFHeader *next, SFBags *bags);
static void free_layer(SFHeader *hdr);


/*----------------------------------------------------------------
 * id numbers
 *----------------------------------------------------------------*/

enum {
	/* level 0; chunk */
	UNKN_ID, RIFF_ID, LIST_ID, SFBK_ID,
	/* level 1; id only */
	INFO_ID, SDTA_ID, PDTA_ID,
	/* info stuff; chunk */
	IFIL_ID, ISNG_ID, IROM_ID, INAM_ID, IVER_ID, IPRD_ID, ICOP_ID,
	ICRD_ID, IENG_ID, ISFT_ID, ICMT_ID,
	/* sample data stuff; chunk */
	SNAM_ID, SMPL_ID,
	/* preset stuff; chunk */
	PHDR_ID, PBAG_ID, PMOD_ID, PGEN_ID,
	/* inst stuff; chunk */
	INST_ID, IBAG_ID, IMOD_ID, IGEN_ID,
	/* sample header; chunk */
	SHDR_ID
};


/*================================================================
 * load a soundfont file
 *================================================================*/

int load_soundfont(SFInfo *sf, struct timidity_file *fd)
{
	SFChunk chunk;

	sf->preset = NULL;
	sf->sample = NULL;
	sf->inst = NULL;
	sf->sf_name = NULL;

	prbags.bag = inbags.bag = NULL;
	prbags.gen = inbags.gen = NULL;

	/* check RIFF file header */
	READCHUNK(&chunk, fd);
	if (chunkid(chunk.id) != RIFF_ID) {
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			  "%s: *** not a RIFF file", current_filename);
		return -1;
	}
	/* check file id */
	READID(chunk.id, fd);
	if (chunkid(chunk.id) != SFBK_ID) {
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			  "%s: *** not a SoundFont file", current_filename);
		return -1;
	}

	for (;;) {
		if(READCHUNK(&chunk, fd) <= 0)
			break;
		else if (chunkid(chunk.id) == LIST_ID) {
			if (process_list(chunk.size, sf, fd))
				break;
		} else {
			ctl->cmsg(CMSG_WARNING, VERB_NORMAL,
				  "%s: *** illegal id in level 0: %4.4s %4d",
				  current_filename, chunk.id, chunk.size);
			FSKIP(chunk.size, fd);
		}
	}

	/* parse layer structure */
	convert_layers(sf);

	/* free private tables */
	if (prbags.bag) free(prbags.bag);
	if (prbags.gen) free(prbags.gen);
	if (inbags.bag) free(inbags.bag);
	if (inbags.gen) free(inbags.gen);

	return 0;
}


/*================================================================
 * free buffer
 *================================================================*/

void free_soundfont(SFInfo *sf)
{
	int i;
	if (sf->preset) {
		for (i = 0; i < sf->npresets; i++)
			free_layer(&sf->preset[i].hdr);
		free(sf->preset);
	}
	if (sf->inst) {
		for (i = 0; i < sf->ninsts; i++)
			free_layer(&sf->inst[i].hdr);
		free(sf->inst);
	}
	if (sf->sample) free(sf->sample);
	if (sf->sf_name) free(sf->sf_name);
}


/*----------------------------------------------------------------
 * get id value from 4bytes ID string
 *----------------------------------------------------------------*/

static int chunkid(char *id)
{
	static struct idstring {
		char *str;
		int id;
	} idlist[] = {
		{"RIFF", RIFF_ID},
		{"LIST", LIST_ID},
		{"sfbk", SFBK_ID},
		{"INFO", INFO_ID},
		{"sdta", SDTA_ID},
		{"snam", SNAM_ID},
		{"smpl", SMPL_ID},
		{"pdta", PDTA_ID},
		{"phdr", PHDR_ID},
		{"pbag", PBAG_ID},
		{"pmod", PMOD_ID},
		{"pgen", PGEN_ID},
		{"inst", INST_ID},
		{"ibag", IBAG_ID},
		{"imod", IMOD_ID},
		{"igen", IGEN_ID},
		{"shdr", SHDR_ID},
		{"ifil", IFIL_ID},
		{"isng", ISNG_ID},
		{"irom", IROM_ID},
		{"iver", IVER_ID},
		{"INAM", INAM_ID},
		{"IPRD", IPRD_ID},
		{"ICOP", ICOP_ID},
		{"ICRD", ICRD_ID},
		{"IENG", IENG_ID},
		{"ISFT", ISFT_ID},
		{"ICMT", ICMT_ID},
	};

	int i;

	for (i = 0; i < sizeof(idlist)/sizeof(idlist[0]); i++) {
		if (strncmp(id, idlist[i].str, 4) == 0)
			return idlist[i].id;
	}

	return UNKN_ID;
}


/*================================================================
 * process a list chunk
 *================================================================*/

static int process_list(int size, SFInfo *sf, struct timidity_file *fd)
{
	SFChunk chunk;

	/* read the following id string */
	READID(chunk.id, fd); size -= 4;
	ctl->cmsg(CMSG_INFO, VERB_DEBUG, "%c%c%c%c:",
		  chunk.id[0], chunk.id[1], chunk.id[2], chunk.id[3]);
	switch (chunkid(chunk.id)) {
	case INFO_ID:
		return process_info(size, sf, fd);
	case SDTA_ID:
		return process_sdta(size, sf, fd);
	case PDTA_ID:
		return process_pdta(size, sf, fd);
	default:
		ctl->cmsg(CMSG_WARNING, VERB_NORMAL,
			  "%s: *** illegal id in level 1: %4.4s",
			  current_filename, chunk.id);
		FSKIP(size, fd); /* skip it */
		return 0;
	}
}

			
/*================================================================
 * process info list
 *================================================================*/
		
static int process_info(int size, SFInfo *sf, struct timidity_file *fd)
{
	sf->infopos = tf_tell(fd);
	sf->infosize = size;
	
	/* parse the buffer */
	while (size > 0) {
		SFChunk chunk;

		/* read a sub chunk */
		if(READCHUNK(&chunk, fd) <= 0)
		    return -1;
		size -= 8;

		ctl->cmsg(CMSG_INFO, VERB_DEBUG, " %c%c%c%c:",
			  chunk.id[0], chunk.id[1], chunk.id[2], chunk.id[3]);
		switch (chunkid(chunk.id)) {
		case IFIL_ID:
			/* soundfont file version */
			READW(&sf->version, fd);
			READW(&sf->minorversion, fd);
			ctl->cmsg(CMSG_INFO, VERB_DEBUG,
				  "  version %d, minor %d",
				  sf->version, sf->minorversion);
			break;
		case INAM_ID:
			/* name of the font */
			sf->sf_name = (char*)safe_malloc(chunk.size + 1);
			tf_read(sf->sf_name, 1, chunk.size, fd);
			sf->sf_name[chunk.size] = 0;
			ctl->cmsg(CMSG_INFO, VERB_DEBUG,
				  "  name %s", sf->sf_name);
			break;
			
		default:
			if(ctl->verbosity >= VERB_DEBUG)
			{
			    char buff[100];
			    if(chunk.size < sizeof(buff) - 1)
			    {
				tf_read(buff, chunk.size, 1, fd);
				buff[chunk.size] = '\0';
			    }
			    else
			    {
				int i = sizeof(buff) - 4;
				tf_read(buff, i, 1, fd);
				FSKIP(chunk.size - i, fd);
				buff[i++] = '.';
				buff[i++] = '.';
				buff[i++] = '.';
				buff[i] = '\0';
			    }
			    ctl->cmsg(CMSG_INFO, VERB_DEBUG, "  %s", buff);
			}
			else
			    FSKIP(chunk.size, fd);
			break;
		}
		size -= chunk.size;
	}
	return 0;
}


/*================================================================
 * process sample data list
 *================================================================*/

static int process_sdta(int size, SFInfo *sf, struct timidity_file *fd)
{
	while (size > 0) {
		SFChunk chunk;

		/* read a sub chunk */
		if(READCHUNK(&chunk, fd) <= 0)
		    return -1;
		size -= 8;

		ctl->cmsg(CMSG_INFO, VERB_DEBUG, " %c%c%c%c:",
			  chunk.id[0], chunk.id[1], chunk.id[2], chunk.id[3]);
		switch (chunkid(chunk.id)) {
		case SNAM_ID:
			/* sample name list */
			load_sample_names(chunk.size, sf, fd);
			break;
		case SMPL_ID:
			/* sample data starts from here */
			sf->samplepos = tf_tell(fd);
			sf->samplesize = chunk.size;
			FSKIP(chunk.size, fd);
			break;
		default:
			FSKIP(chunk.size, fd);
			break;
		}
		size -= chunk.size;
	}
	return 0;
}


/*================================================================
 * process preset data list
 *================================================================*/

static int process_pdta(int size, SFInfo *sf, struct timidity_file *fd)
{
	while (size > 0) {
		SFChunk chunk;

		/* read a subchunk */
		if(READCHUNK(&chunk, fd) <= 0)
		    return -1;
		size -= 8;

		ctl->cmsg(CMSG_INFO, VERB_DEBUG, " %c%c%c%c:",
			  chunk.id[0], chunk.id[1], chunk.id[2], chunk.id[3]);
		switch (chunkid(chunk.id)) {
		case PHDR_ID:
			load_preset_header(chunk.size, sf, fd);
			break;
		case PBAG_ID:
			load_bag(chunk.size, &prbags, fd);
			break;
		case PGEN_ID:
			load_gen(chunk.size, &prbags, fd);
			break;
		case INST_ID:
			load_inst_header(chunk.size, sf, fd);
			break;
		case IBAG_ID:
			load_bag(chunk.size, &inbags, fd);
			break;
		case IGEN_ID:
			load_gen(chunk.size, &inbags, fd);
			break;
		case SHDR_ID:
			load_sample_info(chunk.size, sf, fd);
			break;
		case PMOD_ID: /* ignored */
		case IMOD_ID: /* ingored */
		default:
			FSKIP(chunk.size, fd);
			break;
		}
		size -= chunk.size;
	}
	return 0;
}


/*----------------------------------------------------------------
 * store sample name list; sf1 only
 *----------------------------------------------------------------*/

static void load_sample_names(int size, SFInfo *sf, struct timidity_file *fd)
{
	int i, nsamples;
	if (sf->version > 1) {
		ctl->cmsg(CMSG_WARNING, VERB_NORMAL,
			  "%s: *** version 2 has obsolete format??",
			  current_filename);
		FSKIP(size, fd);
		return;
	}

	/* each sample name has a fixed lentgh (20 bytes) */
	nsamples = size / 20;
	if (sf->sample == NULL) {
		sf->nsamples = nsamples;
		sf->sample = NEW(SFSampleInfo, sf->nsamples);
	} else if (sf->nsamples != nsamples) {
		ctl->cmsg(CMSG_WARNING, VERB_NORMAL,
			  "%s: *** different # of samples ?? (%d : %d)\n",
			  current_filename, sf->nsamples, nsamples);
		FSKIP(size, fd);
		return;
	}
		
	/* read each name from file */
	for (i = 0; i < sf->nsamples; i++) {
		READSTR(sf->sample[i].name, fd);
	}
}


/*----------------------------------------------------------------
 * preset header list
 *----------------------------------------------------------------*/

static void load_preset_header(int size, SFInfo *sf, struct timidity_file *fd)
{
	int i;

	sf->npresets = size / 38;
	sf->preset = NEW(SFPresetHdr, sf->npresets);
	for (i = 0; i < sf->npresets; i++) {
		READSTR(sf->preset[i].hdr.name, fd);
		READW(&sf->preset[i].preset, fd);
		READW(&sf->preset[i].bank, fd);
		READW(&sf->preset[i].hdr.bagNdx, fd);
		SKIPDW(fd); /* lib; ignored*/
		SKIPDW(fd); /* genre; ignored */
		SKIPDW(fd); /* morph; ignored */
		/* initialize layer table; it'll be parsed later */
		sf->preset[i].hdr.nlayers = 0;
		sf->preset[i].hdr.layer = NULL;

		ctl->cmsg(CMSG_INFO, VERB_DEBUG,
			  "  Preset %d (%s) index=%d bank=%d preset=%d",
			  i, sf->preset[i].hdr.name,
			  sf->preset[i].hdr.bagNdx,
			  sf->preset[i].bank,
			  sf->preset[i].preset + progbase);
	}
}


/*----------------------------------------------------------------
 * instrument header list
 *----------------------------------------------------------------*/

static void load_inst_header(int size, SFInfo *sf, struct timidity_file *fd)
{
	int i;

	sf->ninsts = size / 22;
	sf->inst = NEW(SFInstHdr, sf->ninsts);
	for (i = 0; i < sf->ninsts; i++) {
		READSTR(sf->inst[i].hdr.name, fd);
		READW(&sf->inst[i].hdr.bagNdx, fd);
		/* iniitialize layer table; it'll be parsed later */
		sf->inst[i].hdr.nlayers = 0;
		sf->inst[i].hdr.layer = NULL;

		ctl->cmsg(CMSG_INFO, VERB_DEBUG,
			  "  InstHdr %d (%s) bagNdx=%d",
			  i, sf->inst[i].hdr.name, sf->inst[i].hdr.bagNdx);
	}
}


/*----------------------------------------------------------------
 * load preset/instrument bag list on the private table
 *----------------------------------------------------------------*/

static void load_bag(int size, SFBags *bagp, struct timidity_file *fd)
{
	int i;

	size /= 4;
	bagp->bag = NEW(uint16, size);
	for (i = 0; i < size; i++) {
		READW(&bagp->bag[i], fd);
		SKIPW(fd); /* mod; ignored */
	}
	bagp->nbags = size;
}


/*----------------------------------------------------------------
 * load preset/instrument generator list on the private table
 *----------------------------------------------------------------*/

static void load_gen(int size, SFBags *bagp, struct timidity_file *fd)
{
	int i;

	size /= 4;
	bagp->gen = NEW(SFGenRec, size);
	for (i = 0; i < size; i++) {
		READW((uint16 *)&bagp->gen[i].oper, fd);
		READW((uint16 *)&bagp->gen[i].amount, fd);
	}
	bagp->ngens = size;
}


/*----------------------------------------------------------------
 * load sample info list
 *----------------------------------------------------------------*/

static void load_sample_info(int size, SFInfo *sf, struct timidity_file *fd)
{
	int i;
	int in_rom;

	/* the record size depends on the soundfont version */
	if (sf->version > 1) {
		/* SF2 includes sample name and other infos */
		sf->nsamples = size / 46;
		sf->sample = NEW(SFSampleInfo, sf->nsamples);
	} else  {
		/* SBK; sample name may be read already */
		int nsamples = size / 16;
		if (sf->sample == NULL) {
			sf->nsamples = nsamples;
			sf->sample = NEW(SFSampleInfo, sf->nsamples);
		} else if (sf->nsamples != nsamples) {
#if 0
			fprintf(stderr, "*** different # of infos ?? (%d : %d)\n",
			       sf->nsamples, nsamples);
			FSKIP(size, fd);
			return;
#endif
			/* overwrite it */
			sf->nsamples = nsamples;
		}
	}

	in_rom = 1;  /* data may start from ROM samples */
	for (i = 0; i < sf->nsamples; i++) {
		if (sf->version > 1) /* SF2 only */
			READSTR(sf->sample[i].name, fd);
		READDW((uint32 *)&sf->sample[i].startsample, fd);
		READDW((uint32 *)&sf->sample[i].endsample, fd);
		READDW((uint32 *)&sf->sample[i].startloop, fd);
		READDW((uint32 *)&sf->sample[i].endloop, fd);
		if (sf->version > 1) { /* SF2 only */
			READDW((uint32 *)&sf->sample[i].samplerate, fd);
			READB(sf->sample[i].originalPitch, fd);
			READB(sf->sample[i].pitchCorrection, fd);
			READW(&sf->sample[i].samplelink, fd);
			READW(&sf->sample[i].sampletype, fd);
		} else { /* for SBK; set missing infos */
			sf->sample[i].samplerate = 44100;
			sf->sample[i].originalPitch = 60;
			sf->sample[i].pitchCorrection = 0;
			sf->sample[i].samplelink = 0;
			/* the first RAM data starts from address 0 */
			if (sf->sample[i].startsample == 0)
				in_rom = 0;
			if (in_rom)
				sf->sample[i].sampletype = 0x8001;
			else
				sf->sample[i].sampletype = 1;
		}
	}
}


/*================================================================
 * convert from bags to layers
 *================================================================*/

static void convert_layers(SFInfo *sf)
{
	int i;

	if (prbags.bag == NULL || prbags.gen == NULL ||
	    inbags.bag == NULL || inbags.gen == NULL) {
		ctl->cmsg(CMSG_WARNING, VERB_NORMAL,
			  "%s: *** illegal bags / gens", current_filename);
		return;
	}

	for (i = 0; i < sf->npresets - 1; i++) {
		generate_layers(&sf->preset[i].hdr,
				&sf->preset[i+1].hdr,
				&prbags);
	}
	for (i = 0; i < sf->ninsts - 1; i++) {
		generate_layers(&sf->inst[i].hdr,
				&sf->inst[i+1].hdr,
				&inbags);
	}
}


/*----------------------------------------------------------------
 * generate layer lists from stored bags
 *----------------------------------------------------------------*/

static void generate_layers(SFHeader *hdr, SFHeader *next, SFBags *bags)
{
	int i;
	SFGenLayer *layp;
	
	hdr->nlayers = next->bagNdx - hdr->bagNdx;
	if (hdr->nlayers < 0) {
		ctl->cmsg(CMSG_WARNING, VERB_NORMAL,
			  "%s: illegal layer numbers %d",
			  current_filename, hdr->nlayers);
		return;
	}
	if (hdr->nlayers == 0)
		return;
	hdr->layer = (SFGenLayer*)safe_malloc(sizeof(SFGenLayer) * hdr->nlayers);
	layp = hdr->layer;
	for (layp = hdr->layer, i = hdr->bagNdx; i < next->bagNdx; layp++, i++) {
		int genNdx = bags->bag[i];
		layp->nlists = bags->bag[i+1] - genNdx;
		if (layp->nlists < 0) {
			ctl->cmsg(CMSG_WARNING, VERB_NORMAL,
				  "%s: illegal list numbers %d",
				  current_filename, layp->nlists);
			return;
		}
		layp->list = (SFGenRec*)safe_malloc(sizeof(SFGenRec) * layp->nlists);

		memcpy(layp->list, &bags->gen[genNdx],
		       sizeof(SFGenRec) * layp->nlists);
	}
}

/*----------------------------------------------------------------
 * free a layer
 *----------------------------------------------------------------*/

static void free_layer(SFHeader *hdr)
{
	int i;

	for (i = 0; i < hdr->nlayers; i++) {
		SFGenLayer *layp = &hdr->layer[i];
		if (layp->nlists > 0)
			free(layp->list);
	}
	if (hdr->nlayers > 0)
		free(hdr->layer);
}

/* add blank loop for each data */
int auto_add_blank = 0;
void correct_samples(SFInfo *sf)
{
	int i;
	SFSampleInfo *sp;
	int prev_end;

	prev_end = 0;
	for (sp = sf->sample, i = 0; i < sf->nsamples; i++, sp++) {
		/* correct sample positions for SBK file */
		if (sf->version == 1) {
			sp->startloop++;
			sp->endloop += 2;
		}

		/* calculate sample data size */
		if (sp->sampletype & 0x8000)
			sp->size = 0;
		else if (sp->startsample < prev_end && sp->startsample != 0)
			sp->size = 0;
		else {
			sp->size = -1;
			if (!auto_add_blank && i != sf->nsamples-1)
				sp->size = sp[1].startsample - sp->startsample;
			if (sp->size < 0)
				sp->size = sp->endsample - sp->startsample + 48;		}
		prev_end = sp->endsample;

		/* calculate short-shot loop size */
		if (auto_add_blank || i == sf->nsamples-1)
			sp->loopshot = 48;
		else {
			sp->loopshot = sp[1].startsample - sp->endsample;
			if (sp->loopshot < 0 || sp->loopshot > 48)
				sp->loopshot = 48;
		}
	}
}
