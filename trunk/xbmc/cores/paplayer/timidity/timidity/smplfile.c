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
	
	smplfile.c
	
	core and WAVE,AIFF/AIFF-C importer by Kentaro Sato	<kentaro@ranvis.com>
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */
#include <stdlib.h>
#include <math.h>

#ifndef NO_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif

#include "timidity.h"
#include "common.h"
#include "controls.h"
#include "filter.h"
#include "instrum.h"
#include "output.h"
#include "playmidi.h"
#include "resample.h"
#include "tables.h"

typedef int (*SampleImporterDiscriminateProc)(char *sample_file);
	/* returns 0 if file may be loadable */
typedef int (*SampleImporterSampleLoaderProc)(char *sample_file, Instrument *inst);
	/* sets inst->samples, inst->sample and returns 0 if loaded */
	/* inst is pre-allocated, and is freed by caller if loading failed */
	/* -1 to let caller give up testing other importers */

typedef struct {
	char				*extension;		/* file extension excluding '.' */
	SampleImporterDiscriminateProc	discriminant;
	SampleImporterSampleLoaderProc	load;
	/* either extension or discriminant may be NULL */
	int					added;			/* for get_importers()'s internal use */
} SampleImporter, *SampleImporterRef;

Instrument *extract_sample_file(char *sample_file);

static int get_importers(const char *sample_file, int limit, SampleImporter **importers);
static int get_next_importer(char *sample_file, int start, int count, SampleImporter **importers);

static double ConvertFromIeeeExtended(const char *);

static int import_wave_discriminant(char *sample_file);
static int import_wave_load(char *sample_file, Instrument *inst);
static int import_aiff_discriminant(char *sample_file);
static int import_aiff_load(char *sample_file, Instrument *inst);

static SampleImporter	sample_importers[] = {
	{"wav", import_wave_discriminant, import_wave_load},
	{"aiff", import_aiff_discriminant, import_aiff_load},
	{NULL, NULL, NULL},
};

Instrument *extract_sample_file(char *sample_file)
{
	Instrument		*inst;
	SampleImporter	*importers[10], *importer;
	int				i, j, count, result;
	Sample			*sample;
	
	if ((count = get_importers(sample_file, sizeof importers / sizeof importers[0], importers)) == 0)
		return NULL;
	inst = (Instrument *)safe_malloc(sizeof(Instrument));
	inst->type = INST_PCM;
	inst->instname = NULL;
	inst->samples = 0;
	inst->sample = NULL;
	i = 0;
	importer = NULL;
	while ((i = get_next_importer(sample_file, i, count, importers)) < count)
	{
		if ((result = importers[i]->load(sample_file, inst)) == 0)
		{
			importer = importers[i];
			break;
		}
		if (result == -1)	/* importer told to give up test */
			break;
		j = inst->samples;
		while(j > 0)
		{
			if (inst->sample[--j].data_alloced)
				free(inst->sample[j].data);
		}
		inst->samples = 0;
		free(inst->sample);
		inst->sample = NULL;
		i++;	/* try next */
	}
	if (importer == NULL)
	{
		free_instrument(inst);
		return NULL;
	}
	/* post-process */
	if (inst->instname == NULL)
	{
		const char			*name;
		
		name = pathsep_strrchr(sample_file);
		if (name == NULL)
			name = sample_file - 1;
		inst->instname = strdup(name + 1);
	}
	for(i = 0; i < inst->samples; i++)
	{
		sample = &inst->sample[i];
		/* If necessary do some anti-aliasing filtering  */
		if (antialiasing_allowed)
			antialiasing((int16 *)sample->data,
				sample->data_length >> FRACTION_BITS,
				sample->sample_rate, play_mode->rate);
		/* resample it if possible */
		if (sample->note_to_use && !(sample->modes & MODES_LOOPING))
			pre_resample(sample);
#ifdef LOOKUP_HACK
		squash_sample_16to8(sample);
#endif
	}
	return inst;
}

#define ADD_IMPORTER		importer->added = 1;	\
							importers[count++] = importer;

/* returns number of importers which may be suitable for the file */
static int get_importers(const char *sample_file, int limit, SampleImporter **importers)
{
	SampleImporter	*importer;
	int				count;
	const char		*extension;
	
	count = 0;
	importer = sample_importers;
	while(importer->load != NULL && count < limit)
	{
		importer->added = 0;
		importer++;
	}
	/* first, extension matched importers */
	extension = pathsep_strrchr(sample_file);
	if (extension != NULL && (extension = strrchr(extension, '.')) != NULL)
	{
		extension++;
		/* ones which have discriminant first */
		importer = sample_importers;
		while(importer->load != NULL && count < limit)
		{
			if (!importer->added && importer->extension != NULL && importer->discriminant != NULL
					&& strcasecmp(extension, importer->extension) == 0)
				{ADD_IMPORTER}
			importer++;
		}
		/* then ones which don't have discriminant */
		importer = sample_importers;
		while(importer->load != NULL && count < limit)
		{
			if (!importer->added && importer->extension != NULL
					&& importer->discriminant == NULL
					&& strcasecmp(extension, importer->extension) == 0)
				{ADD_IMPORTER}
			importer++;
		}
	}
	/* lastly, ones which has discriminant */
	importer = sample_importers;
	while(importer->load != NULL && count < limit)
	{
		if (!importer->added && importer->discriminant != NULL)
			{ADD_IMPORTER}
		importer++;
	}
	return count;
}

/* returns importer index for the file */
/* returns count if no importer available */
static int get_next_importer(char *sample_file, int start, int count, SampleImporter **importers)
{
	int					i;
	
	for(i = start; i < count; i++)
	{
		if (importers[i]->discriminant != NULL)
		{
			if (importers[i]->discriminant(sample_file) != 0)
				continue;
		}
		return i;
	}
	return i;
}

/*************** Sample Importers ***************/

#define MAX_SAMPLE_CHANNELS		16

/* from instrum.c */
#define READ_CHAR(thing) \
      if (1 != tf_read(&tmpchar, 1, 1, tf)) goto fail; \
      thing = tmpchar;

#define READ_SHORT_LE(thing) \
      if (1 != tf_read(&tmpshort, 2, 1, tf)) goto fail; \
      thing = LE_SHORT(tmpshort);
#define READ_LONG_LE(thing) \
      if (1 != tf_read(&tmplong, 4, 1, tf)) goto fail; \
      thing = LE_LONG(tmplong);
#define READ_SHORT_BE(thing) \
      if (1 != tf_read(&tmpshort, 2, 1, tf)) goto fail; \
      thing = BE_SHORT(tmpshort);
#define READ_LONG_BE(thing) \
      if (1 != tf_read(&tmplong, 4, 1, tf)) goto fail; \
      thing = BE_LONG(tmplong);

const uint8		pan_mono[] = {64};	/* center */
const uint8		pan_stereo[] = {1,127};	/* left,right */
const uint8		pan_3ch[] = {1,127,64};	/* left,right,center*/
/* pannings below are set by guess */
/*const uint8		pan_quad[] = {1,127,16,112};*/	/* front-left?,front-right?,rear-left?,rear-right? */
const uint8		pan_4ch[] = {1,64,127,64};	/* left,center,right,surround?*/
const uint8		pan_6ch[] = {1,32,64,127,95,64};	/* left,left-center?,center,right,right-center?,surround? */
const uint8		*const gen_pan_list[6] = {
		pan_mono, pan_stereo, pan_3ch,
		pan_4ch, NULL, pan_6ch,
};

typedef struct {
	uint8	baseNote;
	int8	detune;
	uint8	lowNote;
	uint8	highNote;
	uint8	lowVelocity;
	uint8	highVelocity;
	int16	gain;
} GeneralInstrumentInfo;

static void initialize_sample(Instrument *inst, int frames, int sample_bits, int sample_rate);
static void apply_GeneralInstrumentInfo(int samples, Sample *sample, const GeneralInstrumentInfo *info);

/* read_sample_data() flags */
#define SAMPLE_BIG_ENDIAN		(1 << 0)
#define SAMPLE_8BIT_UNSIGNED	(1 << 1)

static int read_sample_data(int32 flags, struct timidity_file *tf, int bits, int samples, int frames, sample_t **sdata);

/*************** WAV Importer ***************/

typedef struct {
	int16	wFormatTag;
	uint16	wChannels;
	uint32	dwSamplesPerSec;
	uint32	dwAvgBytesPerSec;
	uint16	wBlockAlign;
	uint16	wBitsPerSample;
} WAVFormatChunk;

typedef struct {
	int32	dwSamplePeriod;
	int32	dwMIDIUnityNote;
	uint32	dwMIDIPitchFraction;
	int		hasLoop, loopType;
	int32	loop_dwStart, loop_dwEnd, loop_dwFraction;
} WAVSamplerChunk;

static int read_WAVFormatChunk(struct timidity_file *tf, WAVFormatChunk *fmt, int psize);
static int read_WAVSamplerChunk(struct timidity_file *tf, WAVSamplerChunk *smpl, int psize);
static int read_WAVInstrumentChunk(struct timidity_file *tf, GeneralInstrumentInfo *inst, int psize);

static int import_wave_discriminant(char *sample_file)
{
	struct timidity_file	*tf;
	char				buf[12];
	
	if ((tf = open_file(sample_file, 1, OF_NORMAL)) == NULL)
		return 1;
	if (tf_read(buf, 12, 1, tf) != 1
			|| memcmp(&buf[0], "RIFF", 4) != 0 || memcmp(&buf[8], "WAVE", 4) != 0)
	{
		close_file(tf);
		return 1;
	}
	close_file(tf);
	return 0;
}

#define WAVE_CHUNKFLAG_SAMPLER		(1 << 0)
#define WAVE_CHUNKFLAG_INSTRUMENT	(1 << 1)

static int import_wave_load(char *sample_file, Instrument *inst)
{
	struct timidity_file	*tf;
	char			buf[12];
	int				state;		/* initial > fmt_read > data_read */
	int				i, chunk_size, type_index, type_size, samples;
	int32			chunk_flags;
	Sample			*sample;
	WAVFormatChunk	format;
	WAVSamplerChunk	samplerc;
	GeneralInstrumentInfo	instc;
	
	if ((tf = open_file(sample_file, 1, OF_NORMAL)) == NULL)
		return 1;
	if (tf_read(buf, 12, 1, tf) != 1
			|| memcmp(&buf[0], "RIFF", 4) != 0 || memcmp(&buf[8], "WAVE", 4) != 0)
	{
		close_file(tf);
		return 1;
	}
	ctl->cmsg(CMSG_INFO, VERB_NOISY, "Loading WAV: %s", sample_file);
	state = chunk_flags = 0;
	type_index = 4, type_size = 8;
	for(;;) {
		if (tf_read(&buf[type_index], type_size, 1, tf) != 1)
			break;
		chunk_size = LE_LONG(*(int32 *)&buf[4 + 4]);
		if (memcmp(&buf[4 + 0], "fmt ", 4) == 0)
		{
			if (state != 0					/* only one format chunk is required */
					|| chunk_size < 0x10)	/* too small */
				break;
			if (!read_WAVFormatChunk(tf, &format, chunk_size))
				break;
			if (format.wChannels < 1				/* invalid range */
					|| format.wChannels > MAX_SAMPLE_CHANNELS
					|| format.wFormatTag != 1		/* compressed */
					|| format.wBitsPerSample & 0x7	/* padding not supported */
					|| format.wBitsPerSample > 16)	/* more than 16-bit is not supported */
				break;
			state++;
		}
		else if (memcmp(&buf[4 + 0], "data", 4) == 0)
		{
			int				frames;
			sample_t		*sdata[MAX_SAMPLE_CHANNELS];
			
			if (state != 1)
				break;
			frames = chunk_size / format.wBlockAlign;
			inst->samples = samples = format.wChannels;
			inst->sample = (Sample *)safe_malloc(sizeof(Sample) * samples);
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "Format: %d-bits %dHz %dch, %d frames",
					format.wBitsPerSample, format.dwSamplesPerSec, samples, frames);
			initialize_sample(inst, frames, format.wBitsPerSample, format.dwSamplesPerSec);
			/* load waveform data */
			for(i = 0; i < samples; i++)
			{
				inst->sample[i].data = sdata[i] = (sample_t *)safe_malloc(sizeof(sample_t) * frames);
				inst->sample[i].data_alloced = 1;
			}
			if (!read_sample_data(SAMPLE_8BIT_UNSIGNED, tf, format.wBitsPerSample, samples, frames, sdata))
				break;
			state++;
		}
		else if (!(chunk_flags & WAVE_CHUNKFLAG_SAMPLER) && memcmp(&buf[4 + 0], "smpl", 4) == 0)
		{
			if (!read_WAVSamplerChunk(tf, &samplerc, chunk_size))
				break;
			chunk_flags |= WAVE_CHUNKFLAG_SAMPLER;
		}
		else if (!(chunk_flags & WAVE_CHUNKFLAG_INSTRUMENT) && memcmp(&buf[4 + 0], "inst", 4) == 0)
		{
			if (!read_WAVInstrumentChunk(tf, &instc, chunk_size))
				break;
			chunk_flags |= WAVE_CHUNKFLAG_INSTRUMENT;
		}
		else if (tf_seek(tf, chunk_size, SEEK_CUR) == -1)
			break;
		type_index = 4 - (chunk_size & 1);
		type_size = 8 + (chunk_size & 1);
	}
	close_file(tf);
	if (chunk_flags & WAVE_CHUNKFLAG_SAMPLER)
	{
		uint8		modes;
		int32		sample_rate, root_freq;
		uint32		loopStart, loopEnd;
		
		sample_rate = 1000000000 / samplerc.dwSamplePeriod;
		root_freq = freq_table[samplerc.dwMIDIUnityNote];
		if (samplerc.dwMIDIPitchFraction != 0
				&& samplerc.dwMIDIUnityNote != 127)	/* no table data */
		{
			int32		diff;
			
			diff = freq_table[samplerc.dwMIDIUnityNote + 1] - root_freq;
			root_freq += (float)samplerc.dwMIDIPitchFraction * diff / 0xFFFFFFFF;
		}
		if (samplerc.hasLoop)
		{
			const uint8		loopModes[] = {MODES_LOOPING, MODES_LOOPING | MODES_PINGPONG, MODES_LOOPING | MODES_REVERSE};
			
			modes = loopModes[samplerc.loopType];
			loopStart = samplerc.loop_dwStart << FRACTION_BITS;
			loopEnd = samplerc.loop_dwEnd << FRACTION_BITS;
		}
		else
			modes = 0;
		for(i = 0; i < samples; i++)
		{
			sample = &inst->sample[i];
			sample->sample_rate = sample_rate;
			sample->root_freq = root_freq;
			if (modes != 0)
			{
				sample->loop_start = loopStart;
				sample->loop_end = loopEnd;
			}
			sample->modes |= modes;
		}
	}
	if (chunk_flags & WAVE_CHUNKFLAG_INSTRUMENT)
		apply_GeneralInstrumentInfo(samples, inst->sample, &instc);
	return (state != 2);
}

static int read_WAVFormatChunk(struct timidity_file *tf, WAVFormatChunk *fmt, int csize)
{
	int32		tmplong;
	int16		tmpshort;
	
	READ_SHORT_LE(fmt->wFormatTag);
	READ_SHORT_LE(fmt->wChannels);
	READ_LONG_LE(fmt->dwSamplesPerSec);
	READ_LONG_LE(fmt->dwAvgBytesPerSec);
	READ_SHORT_LE(fmt->wBlockAlign);
	READ_SHORT_LE(fmt->wBitsPerSample);
	if (tf_seek(tf, csize - 0x10, SEEK_CUR) == -1)
		goto fail;
	return 1;
	fail:
		ctl->cmsg(CMSG_WARNING, VERB_VERBOSE, "Unable to read format chunk");
		return 0;
}

static int read_WAVSamplerChunk(struct timidity_file *tf, WAVSamplerChunk *smpl, int psize)
{
	int32		tmplong;
	int			i, loopCount, cbSamplerData, dwPlayCount;
	unsigned int	loopType;
	
	smpl->hasLoop = 0;
	/* skip dwManufacturer, dwProduct */
	if (tf_seek(tf, 4 + 4, SEEK_CUR) == -1)
		goto fail;
	READ_LONG_LE(smpl->dwSamplePeriod);
	READ_LONG_LE(smpl->dwMIDIUnityNote);
	READ_LONG_LE(smpl->dwMIDIPitchFraction);
	/* skip dwSMPTEFormat, dwSMPTEOffset */
	if (tf_seek(tf, 4 + 4, SEEK_CUR) == -1)
		goto fail;
	READ_LONG_LE(loopCount);
	READ_LONG_LE(cbSamplerData);
	psize -= 4 * 9 + loopCount * 4 * 6;
	for(i = 0; i < loopCount; i++)
	{
		/* skip dwIdentifier */
		if (tf_seek(tf, 4, SEEK_CUR) == -1)
			goto fail;
		READ_LONG_LE(loopType);	/* dwType */
		if (!smpl->hasLoop && loopType <= 2)
		{
			smpl->loopType = loopType;
			READ_LONG_LE(smpl->loop_dwStart);
			READ_LONG_LE(smpl->loop_dwEnd);
			READ_LONG_LE(smpl->loop_dwFraction);
			READ_LONG_LE(dwPlayCount);
			if (dwPlayCount == 0)	/* infinite loop */
				smpl->hasLoop = 1;
		}
		else
		{
			if (tf_seek(tf, 4 * 4, SEEK_CUR) == -1)
				goto fail;
		}
	}
	if (psize != cbSamplerData)
		ctl->cmsg(CMSG_WARNING, VERB_NOISY, "Bad sampler chunk length");
	if (tf_seek(tf, psize, SEEK_CUR) == -1)
		goto fail;
	ctl->cmsg(CMSG_INFO, VERB_NOISY, "Sampler: %dns/frame, note=%d, loops=%d",
				smpl->dwSamplePeriod, smpl->dwMIDIUnityNote, loopCount);
	return 1;
	fail:
		ctl->cmsg(CMSG_WARNING, VERB_VERBOSE, "Unable to read sampler chunk");
		return 0;
}

static int read_WAVInstrumentChunk(struct timidity_file *tf, GeneralInstrumentInfo *inst, int psize)
{
	int8		tmpchar;
	
	if (psize != 7)
		goto fail;
	READ_CHAR(inst->baseNote);
	READ_CHAR(inst->detune);
	READ_CHAR(inst->gain);
	READ_CHAR(inst->lowNote);
	READ_CHAR(inst->highNote);
	READ_CHAR(inst->lowVelocity);
	READ_CHAR(inst->highVelocity);
	ctl->cmsg(CMSG_INFO, VERB_VERBOSE, "Instrument: note=%d (%d-%d), gain=%ddb, velocity=%d-%d",
				inst->baseNote, inst->lowNote, inst->highNote, inst->gain,
				inst->lowVelocity, inst->highVelocity);
	return 1;
	fail:
		ctl->cmsg(CMSG_WARNING, VERB_VERBOSE, "Unable to read instrument chunk");
		return 0;
}

/*************** AIFF importer ***************/

typedef struct {
	uint16			numChannels;
	uint32			numSampleFrames;
	uint16			sampleSize;
	double			sampleRate;
} AIFFCommonChunk;

typedef struct {
	uint32			position;
	Instrument		*inst;
	AIFFCommonChunk	*common;
} AIFFSoundDataChunk;

typedef struct {
	uint16			mode;
	int16			beginID, endID;
} AIFFLoopInfo;

typedef struct {
	int16			id;
	uint32			position;
} AIFFMarkerData;

static int read_AIFFCommonChunk(struct timidity_file *tf, AIFFCommonChunk *comm, int csize, int compressed);
static int read_AIFFSoundDataChunk(struct timidity_file *tf, AIFFSoundDataChunk *sound, int csize, int mode);
static int read_AIFFSoundData(struct timidity_file *tf, Instrument *inst, AIFFCommonChunk *common);
static int read_AIFFInstumentChunk(struct timidity_file *tf, GeneralInstrumentInfo *inst, AIFFLoopInfo *loop, int csize);
static int read_AIFFMarkerChunk(struct timidity_file *tf, AIFFMarkerData **markers, int csize);
static int AIFFGetMarkerPosition(int16 id, const AIFFMarkerData *markers, uint32 *position);

static int import_aiff_discriminant(char *sample_file)
{
	struct timidity_file	*tf;
	char				buf[12];
	
	if ((tf = open_file(sample_file, 1, OF_NORMAL)) == NULL)
		return 1;
	if (tf_read(buf, 12, 1, tf) != 1
			|| memcmp(&buf[0], "FORM", 4) != 0 || memcmp(&buf[8], "AIF", 3) != 0
			|| (buf[8 + 3] != 'F' && buf[8 + 3] != 'C'))
	{
		close_file(tf);
		return 1;
	}
	close_file(tf);
	return 0;
}

#define AIFF_CHUNKFLAG_COMMON		(1 << 0)
#define AIFF_CHUNKFLAG_SOUND		(1 << 1)
#define AIFF_CHUNKFLAG_INSTRUMENT	(1 << 2)
#define AIFF_CHUNKFLAG_MARKER		(1 << 3)
#define AIFF_CHUNKFLAG_SOUNDREAD	(1 << 29)
#define AIFF_CHUNKFLAG_READERR		(1 << 30)
#define AIFF_CHUNKFLAG_DUPCHUNK		(1 << 31)
#define AIFF_CHUNKFLAG_REQUIRED		(AIFF_CHUNKFLAG_COMMON | AIFF_CHUNKFLAG_SOUND)
#define AIFF_CHUNKFLAG_FAILED		(AIFF_CHUNKFLAG_READERR | AIFF_CHUNKFLAG_DUPCHUNK)

static int import_aiff_load(char *sample_file, Instrument *inst)
{
	struct timidity_file	*tf;
	char			buf[12];
	int				chunk_size, type_index, type_size;
	int				compressed;
	int32			chunk_flags;
	AIFFCommonChunk	common;
	AIFFSoundDataChunk	sound;
	GeneralInstrumentInfo	inst_info;
	AIFFLoopInfo	loop_info;
	AIFFMarkerData	*marker_data;
	
	if ((tf = open_file(sample_file, 1, OF_NORMAL)) == NULL)
		return 1;
	if (tf_read(buf, 12, 1, tf) != 1
			|| memcmp(&buf[0], "FORM", 4) != 0 || memcmp(&buf[8], "AIF", 3) != 0
			|| (buf[8 + 3] != 'F' && buf[8 + 3] != 'C'))
	{
		close_file(tf);
		return 1;
	}
	compressed = buf[8 + 3] == 'C';
	ctl->cmsg(CMSG_INFO, VERB_NOISY, "Loading AIFF: %s", sample_file);
	type_index = 4, type_size = 8;
	chunk_flags = 0;
	sound.inst = inst;
	sound.common = &common;
	marker_data = NULL;
	for(;;) {
		if (tf_read(&buf[type_index], type_size, 1, tf) != 1)
			break;
		chunk_size = BE_LONG(*(int32 *)&buf[4 + 4]);
		if (memcmp(&buf[4 + 0], "COMM", 4) == 0)
		{
			if (chunk_flags & AIFF_CHUNKFLAG_COMMON)
			{
				chunk_flags |= AIFF_CHUNKFLAG_DUPCHUNK;
				break;
			}
			if (chunk_size < 18)	/* too small */
				break;
			if (!read_AIFFCommonChunk(tf, &common, chunk_size, compressed))
				break;
			chunk_flags |= AIFF_CHUNKFLAG_COMMON;
		}
		else if (memcmp(&buf[4 + 0], "SSND", 4) == 0)
		{
			if (chunk_flags & AIFF_CHUNKFLAG_SOUND)
			{
				chunk_flags |= AIFF_CHUNKFLAG_DUPCHUNK;
				break;
			}
			if (chunk_flags & AIFF_CHUNKFLAG_COMMON)
			{
				if (!read_AIFFSoundDataChunk(tf, &sound, chunk_size, 0))
					break;
				chunk_flags |= AIFF_CHUNKFLAG_SOUNDREAD;
			}
			else if (!read_AIFFSoundDataChunk(tf, &sound, chunk_size, 1))
				break;
			chunk_flags |= AIFF_CHUNKFLAG_SOUND;
		}
		else if (memcmp(&buf[4 + 0], "INST", 4) == 0)
		{
			if (chunk_flags & AIFF_CHUNKFLAG_INSTRUMENT)
			{
				chunk_flags |= AIFF_CHUNKFLAG_DUPCHUNK;
				break;
			}
			else if (!read_AIFFInstumentChunk(tf, &inst_info, &loop_info, chunk_size))
				break;
			chunk_flags |= AIFF_CHUNKFLAG_INSTRUMENT;
		}
		else if (memcmp(&buf[4 + 0], "MARK", 4) == 0)
		{
			if (chunk_flags & AIFF_CHUNKFLAG_MARKER)
			{
				chunk_flags |= AIFF_CHUNKFLAG_DUPCHUNK;
				break;
			}
			else if (chunk_size < 2 || !read_AIFFMarkerChunk(tf, &marker_data, chunk_size))
				break;
			chunk_flags |= AIFF_CHUNKFLAG_MARKER;
		}
		else if (inst->instname == NULL && memcmp(&buf[4 + 0], "NAME", 4) == 0)
		{
			inst->instname = malloc(chunk_size + 1);
			if (tf_read(inst->instname, chunk_size, 1, tf) != 1)
			{
				chunk_flags |= AIFF_CHUNKFLAG_READERR;
				break;
			}
			inst->instname[chunk_size] = '\0';
		}
		else if (tf_seek(tf, chunk_size, SEEK_CUR) == -1)
			break;
		/* no need to check format version chunk */
		type_index = 4 - (chunk_size & 1);
		type_size = 8 + (chunk_size & 1);
	}
	if (chunk_flags & AIFF_CHUNKFLAG_FAILED
			|| (chunk_flags & AIFF_CHUNKFLAG_REQUIRED) != AIFF_CHUNKFLAG_REQUIRED)
	{
		if (marker_data != NULL)
			free(marker_data);
		close_file(tf);
		return -1;
	}
	if (!(chunk_flags & AIFF_CHUNKFLAG_SOUNDREAD))
	{
		if (!read_AIFFSoundDataChunk(tf, &sound, 0, 2))
		{
			if (marker_data != NULL)
				free(marker_data);
			close_file(tf);
			return 1;
		}
	}
	if (chunk_flags & AIFF_CHUNKFLAG_INSTRUMENT)
	{
		apply_GeneralInstrumentInfo(inst->samples, inst->sample, &inst_info);
		if ((loop_info.mode == 1 || loop_info.mode == 2)
				&& chunk_flags & AIFF_CHUNKFLAG_MARKER && marker_data != NULL)
		{
			Sample			*sample;
			int				i;
			uint32			loopStart, loopEnd;
			uint8			loopMode;
			
			if (AIFFGetMarkerPosition(loop_info.beginID, marker_data, &loopStart)
					&& AIFFGetMarkerPosition(loop_info.endID, marker_data, &loopEnd))
			{
				loopMode = (loop_info.mode == 1) ? MODES_LOOPING : (MODES_LOOPING | MODES_PINGPONG);
				loopStart <<= FRACTION_BITS;
				loopEnd <<= FRACTION_BITS;
				if (loopStart <= loopEnd)
				{
					for(i = 0; i < inst->samples; i++)
					{
						sample = &inst->sample[i];
						sample->loop_start = loopStart;
						sample->loop_end = loopEnd;
						sample->modes |= loopMode;
					}
				}
			}
		}
	}
	if (marker_data != NULL)
		free(marker_data);
	close_file(tf);
	return 0;
}

static int read_AIFFCommonChunk(struct timidity_file *tf, AIFFCommonChunk *comm, int csize, int compressed)
{
	int32		tmplong;
	int16		tmpshort;
	int8		tmpchar;
	char		sampleRate[10];
	uint32		compressionType;
	
	READ_SHORT_BE(comm->numChannels);
	READ_LONG_BE(comm->numSampleFrames);
	READ_SHORT_BE(comm->sampleSize);
	if (tf_read(sampleRate, 10, 1, tf) != 1)
		goto fail;
	comm->sampleRate = ConvertFromIeeeExtended(sampleRate);
	csize -= 8 + 10;
	ctl->cmsg(CMSG_INFO, VERB_NOISY, "Format: %d-bits %dHz %dch, %d frames",
			comm->sampleSize, (int)comm->sampleRate, comm->numChannels, comm->numSampleFrames);
	if (compressed)
	{
		READ_LONG_BE(compressionType);
		if (compressionType != BE_LONG(0x4E4F4E45) /* NONE */)
		{
			char		compressionName[256];
			uint8		compressionNameLength;
			
			READ_CHAR(compressionNameLength);
			if (tf_read(compressionName, compressionNameLength, 1, tf) != 1)
				goto fail;
			compressionName[compressionNameLength] = '\0';
			ctl->cmsg(CMSG_WARNING, VERB_VERBOSE, "AIFF-C unknown compression type: %s", compressionName);
			goto fail;
		}
		csize -= 4;
		/* ignore compressionName and its padding */
	}
	if (tf_seek(tf, csize, SEEK_CUR) == -1)
		goto fail;
	return 1;
	fail:
		ctl->cmsg(CMSG_WARNING, VERB_VERBOSE, "Unable to read common chunk");
		return 0;
}

static int read_AIFFSoundDataChunk(struct timidity_file *tf, AIFFSoundDataChunk *sound, int csize, int mode)
{
	int32		tmplong;
	uint32		offset, blockSize;
	
	if (mode == 0 || mode == 1)
	{
		READ_LONG_BE(offset);
		READ_LONG_BE(blockSize);
		if (blockSize != 0)		/* not implemented */
			goto fail;
		if (mode == 0)		/* read both information and data */
			return read_AIFFSoundData(tf, sound->inst, sound->common);
		/* read information only */
		if ((sound->position = tf_tell(tf)) == -1)
			goto fail;
		sound->position += offset;
		csize -= 8;
		if (tf_seek(tf, csize, SEEK_CUR) == -1)
			goto fail;
		return 1;
	}
	else if (mode == 2)		/* read data using information previously read */
	{
		if (tf_seek(tf, sound->position, SEEK_SET) == -1)
			goto fail;
		return read_AIFFSoundData(tf, sound->inst, sound->common);
	}
	fail:
		ctl->cmsg(CMSG_WARNING, VERB_VERBOSE, "Unable to read sound data chunk");
		return 0;
}

static int read_AIFFSoundData(struct timidity_file *tf, Instrument *inst, AIFFCommonChunk *common)
{
	int				i, samples;
	Sample			*sample;
	sample_t		*sdata[MAX_SAMPLE_CHANNELS];
	
	if ((samples = common->numChannels) > MAX_SAMPLE_CHANNELS)
		goto fail;
	inst->samples = samples;
	inst->sample = sample = (Sample *)safe_malloc(sizeof(Sample) * samples);
	initialize_sample(inst, common->numSampleFrames, common->sampleSize, (int)common->sampleRate);
	/* load samples */
	for(i = 0; i < samples; i++)
	{
		sample[i].data = sdata[i] = (sample_t *)safe_malloc(sizeof(sample_t) * common->numSampleFrames);
		sample[i].data_alloced = 1;
	}
	if (!read_sample_data(SAMPLE_BIG_ENDIAN, tf, common->sampleSize, samples, common->numSampleFrames, sdata))
		goto fail;
	return 1;
	fail:
		ctl->cmsg(CMSG_WARNING, VERB_VERBOSE, "Unable to read sound data");
		return 0;
}

static int read_AIFFInstumentChunk(struct timidity_file *tf, GeneralInstrumentInfo *inst, AIFFLoopInfo *loop, int csize)
{
	int8		tmpchar;
	int16		tmpshort;
	
	if (csize != 20)
	{
		ctl->cmsg(CMSG_WARNING, VERB_VERBOSE, "Bad instrument chunk length");
		if (tf_seek(tf, csize, SEEK_CUR) == -1)
			goto fail;
		return 1;
	}
	READ_CHAR(inst->baseNote);
	READ_CHAR(inst->detune);
	READ_CHAR(inst->lowNote);
	READ_CHAR(inst->highNote);
	READ_CHAR(inst->lowVelocity);
	READ_CHAR(inst->highVelocity);
	READ_SHORT_BE(inst->gain);
	READ_SHORT_BE(loop->mode);	/* sustain loop */
	READ_SHORT_BE(loop->beginID);
	READ_SHORT_BE(loop->endID);
	if (tf_seek(tf, 2 + 2 + 2, SEEK_CUR) == -1)	/* release loop */
		goto fail;
	ctl->cmsg(CMSG_INFO, VERB_VERBOSE, "Instrument: note=%d (%d-%d), gain=%ddb, velocity=%d-%d",
				inst->baseNote, inst->lowNote, inst->highNote, inst->gain,
				inst->lowVelocity, inst->highVelocity);
	return 1;
	fail:
		ctl->cmsg(CMSG_WARNING, VERB_VERBOSE, "Unable to read instrument chunk");
		return 0;
}

static int read_AIFFMarkerChunk(struct timidity_file *tf, AIFFMarkerData **markers, int csize)
{
	int32		tmplong;
	int16		tmpshort;
	int16			markerCount;
	int				i, dest;
	AIFFMarkerData	*m;
	
	m = NULL;
	READ_SHORT_BE(markerCount)
	if (csize != 2 + markerCount * (2 + 4))
	{
		ctl->cmsg(CMSG_WARNING, VERB_VERBOSE, "Bad marker chunk length");
		if (tf_seek(tf, csize, SEEK_CUR) == -1)
			goto fail;
		return 1;
	}
	if ((m = malloc(sizeof(AIFFMarkerData) * (markerCount + 1))) == NULL)
		goto fail;
	for(i = dest = 0; i < markerCount; i++)
	{
		READ_SHORT_BE(m[dest].id);
		READ_LONG_BE(m[dest].position);
		if (m[dest].id > 0)
			dest++; 
	}
	m[dest].id = 0;
	*markers = m;
	return 1;
	fail:
		if (m != NULL)
			free(m);
		ctl->cmsg(CMSG_WARNING, VERB_VERBOSE, "Unable to read marker chunk");
		return 0;
}

static int AIFFGetMarkerPosition(int16 id, const AIFFMarkerData *markers, uint32 *position)
{
	const AIFFMarkerData	*marker;
	
	marker = markers;
	while(marker->id != 0)
	{
		if (marker->id == id)
		{
			*position = marker->position;
			return 1;
		}
		marker++;
	}
	return 0;
}

/******************************/

#define WAVE_BUF_SIZE		(1 << 11)	/* should be power of 2 */
#define READ_WAVE_SAMPLE(dest, b, s)	\
				if (tf_read(dest, (b) * (s), 1, tf) != 1)	\
					goto fail
#define READ_WAVE_FRAME(dest, b, f)	\
				READ_WAVE_SAMPLE(dest, b, (f) * channels)
#define BITS_S8_TO_16(n)	((uint16)((n) << 8) | ((n) ^ 0x80))
#define BITS_U8_TO_16(n)	((uint16)(((n) ^ 0x80) << 8) | (n))

#define BLOCK_READ_BEGIN(stype, sbyte, fch)	{ /* sbyte may be sizeof(stype) */	\
			stype				data[WAVE_BUF_SIZE / sizeof(stype)];	\
			int					j;	\
			for(block_frame_count = (sizeof data / sbyte / fch); block_frame_count != 0; block_frame_count >>= 1) {	\
				while(i <= frames - block_frame_count) {	\
					READ_WAVE_FRAME(data, sbyte, block_frame_count);	\
					for(j = 0; j < (block_frame_count * (fch)); i++)
#define BLOCK_READ_END		} } }

static int read_sample_data(int32 flags, struct timidity_file *tf, int bits, int channels, int frames, sample_t **sdata)
{
	int				i, block_frame_count;
	
	i = 0;
	if (bits == 16)
	{
		if (channels == 1)
		{
			READ_WAVE_SAMPLE(sdata[0], 2, frames);
			if (flags & SAMPLE_BIG_ENDIAN) {
				#ifdef LITTLE_ENDIAN
				for(i = 0; i < frames; i++)
					sdata[0][i] = BE_SHORT(sdata[0][i]);
				#endif
			} else {
				#ifdef BIG_ENDIAN
				for(i = 0; i < frames; i++)
					sdata[0][i] = LE_SHORT(sdata[0][i]);
				#endif
			}
		} else {
			if (flags & SAMPLE_BIG_ENDIAN) {
				BLOCK_READ_BEGIN(uint16, 2, channels)
				{
					int c;
					for(c = 0; c < channels; c++, j++)
						sdata[c][i] = BE_SHORT(data[j]);
				}
				BLOCK_READ_END
			} else {
				BLOCK_READ_BEGIN(uint16, 2, channels)
				{
					int c;
					for(c = 0; c < channels; c++, j++)
						sdata[c][i] = LE_SHORT(data[j]);
				}
				BLOCK_READ_END
			}
		}
	}
	else
	{
		if (channels == 1)
		{
			if (flags & SAMPLE_8BIT_UNSIGNED) {
				BLOCK_READ_BEGIN(uint8, 1, 1)
				{
					sdata[0][i] = BITS_U8_TO_16(data[j]); j++;
				}
				BLOCK_READ_END
			} else {
				BLOCK_READ_BEGIN(uint8, 1, 1)
				{
					sdata[0][i] = BITS_S8_TO_16(data[j]); j++;
				}
				BLOCK_READ_END
			}
		} else {
			if (flags & SAMPLE_8BIT_UNSIGNED) {
				BLOCK_READ_BEGIN(uint8, 1, channels)
				{
					int c;
					for(c = 0; c < channels; c++, j++)
						sdata[c][i] = BITS_U8_TO_16(data[j]);
				}
				BLOCK_READ_END
			} else {
				BLOCK_READ_BEGIN(uint8, 1, channels)
				{
					int c;
					for(c = 0; c < channels; c++, j++)
						sdata[c][i] = BITS_S8_TO_16(data[j]);
				}
				BLOCK_READ_END
			}
		}
	}
	return 1;
	fail:
		ctl->cmsg(CMSG_WARNING, VERB_VERBOSE, "Unable to read sample data");
		return 0;
}

/* from instrum.c */
static int32 convert_envelope_rate(uint8 rate)
{
  int32 r;

  r=3-((rate>>6) & 0x3);
  r*=3;
  r = (int32)(rate & 0x3f) << r; /* 6.9 fixed point */

  /* 15.15 fixed point. */
  return (((r * 44100) / play_mode->rate) * control_ratio)
    << ((fast_decay) ? 10 : 9);
}

/* from instrum.c */
static int32 convert_envelope_offset(uint8 offset)
{
  /* This is not too good... Can anyone tell me what these values mean?
     Are they GUS-style "exponential" volumes? And what does that mean? */

  /* 15.15 fixed point */
  return offset << (7+15);
}

static void initialize_sample(Instrument *inst, int frames, int sample_bits, int sample_rate)
{
	int				i, j, samples;
	Sample			*sample;
	const uint8		*panning;
	
	samples = inst->samples;
	for(i = 0; i < samples; i++)
	{
		sample = &inst->sample[i];
		sample->data_alloced = 0;
		sample->loop_start = 0;
		sample->loop_end = sample->data_length = frames << FRACTION_BITS;
		sample->sample_rate = sample_rate;
		sample->low_freq = freq_table[0];
		sample->high_freq = freq_table[127];
		sample->root_freq = freq_table[60];
		sample->panning = 64;
		sample->note_to_use = 0;
		sample->volume = 1.0;
		sample->modes = MODES_16BIT;
		sample->low_vel = 0;
		sample->high_vel = 127;
		sample->tremolo_sweep_increment =
			sample->tremolo_phase_increment = sample->tremolo_depth =
			sample->vibrato_sweep_increment = sample->vibrato_control_ratio = sample->vibrato_depth = 0;
		sample->cutoff_freq = sample->resonance = sample->tremolo_to_pitch = 
			sample->tremolo_to_fc = sample->modenv_to_pitch = sample->modenv_to_fc =
			sample->vel_to_fc = sample->key_to_fc = sample->vel_to_resonance = 0;
		sample->envelope_velf_bpo = sample->modenv_velf_bpo =
			sample->vel_to_fc_threshold = 64;
		sample->key_to_fc_bpo = 60;
		sample->scale_freq = 60;
		sample->scale_factor = 1024;
		memset(sample->envelope_velf, 0, sizeof(sample->envelope_velf));
		memset(sample->envelope_keyf, 0, sizeof(sample->envelope_keyf));
		memset(sample->modenv_velf, 0, sizeof(sample->modenv_velf));
		memset(sample->modenv_keyf, 0, sizeof(sample->modenv_keyf));
		memset(sample->modenv_rate, 0, sizeof(sample->modenv_rate));
		memset(sample->modenv_offset, 0, sizeof(sample->modenv_offset));
		sample->envelope_delay = sample->modenv_delay =
			sample->tremolo_delay = sample->vibrato_delay = 0;
		sample->inst_type = INST_PCM;
		sample->sample_type = SF_SAMPLETYPE_MONO;
		sample->sf_sample_link = -1;
		sample->sf_sample_index = 0;
	}
	if (samples <= 6 && (panning = gen_pan_list[samples - 1]) != NULL)
	{
		for(i = 0; i < samples; i++)
			inst->sample[i].panning = panning[i];
	}
	for(i = 0; i < 6; i++)
	{
		int32			envelope_rate, envelope_offset;
	
		envelope_rate = convert_envelope_rate(63);	/* wav2pat.c */
		envelope_offset = convert_envelope_offset(240);	/* wav2pat.c */
		for(j = 0; j < samples; j++)
		{
			sample = &inst->sample[j];
			sample->envelope_rate[i] = envelope_rate;
			sample->envelope_offset[i] = envelope_offset;
		}
	}
}

static void apply_GeneralInstrumentInfo(int samples, Sample *sample, const GeneralInstrumentInfo *info)
{
	int32		root_freq;
	FLOAT_T		gain;
	int			i;
	
	root_freq = freq_table[info->baseNote];
	if (info->detune < 0)
	{
		if (info->baseNote != 0)	/* no table data */
			root_freq += (root_freq - freq_table[info->baseNote - 1]) * 50 / info->detune;
	}
	else if (info->detune > 0)
	{
		if (info->baseNote != 127)	/* no table data */
			root_freq += (freq_table[info->baseNote + 1] - root_freq) * 50 / info->detune;
	}
	gain = pow(10, info->gain / 20.0);
	for(i = 0; i < samples; i++)
	{
		sample[i].low_freq = freq_table[info->lowNote];
		sample[i].high_freq = freq_table[info->highNote];
		sample[i].root_freq = root_freq;
		sample[i].volume *= gain;
		sample[i].low_vel = info->lowVelocity;
		sample[i].high_vel = info->highVelocity;
	}
}


/* Copyright (C) 1989-1991 Ken Turkowski. <turk@computer.org>
 *
 * All rights reserved.
 *
 * Warranty Information
 *  Even though I have reviewed this software, I make no warranty
 *  or representation, either express or implied, with respect to this
 *  software, its quality, accuracy, merchantability, or fitness for a
 *  particular purpose.  As a result, this software is provided "as is,"
 *  and you, its user, are assuming the entire risk as to its quality
 *  and accuracy.
 *
 * This code may be used and freely distributed as long as it includes
 * this copyright notice and the above warranty information.
 *
 * Machine-independent I/O routines for IEEE floating-point numbers.
 *
 * NaN's and infinities are converted to HUGE_VAL or HUGE, which
 * happens to be infinity on IEEE machines.  Unfortunately, it is
 * impossible to preserve NaN's in a machine-independent way.
 * Infinities are, however, preserved on IEEE machines.
 *
 * These routines have been tested on the following machines:
 *	Apple Macintosh, MPW 3.1 C compiler
 *	Apple Macintosh, THINK C compiler
 *	Silicon Graphics IRIS, MIPS compiler
 *	Cray X/MP and Y/MP
 *	Digital Equipment VAX
 *	Sequent Balance (Multiprocesor 386)
 *	NeXT
 *
 *
 * Implemented by Malcolm Slaney and Ken Turkowski.
 *
 * Malcolm Slaney contributions during 1988-1990 include big- and little-
 * endian file I/O, conversion to and from Motorola's extended 80-bit
 * floating-point format, and conversions to and from IEEE single-
 * precision floating-point format.
 *
 * In 1991, Ken Turkowski implemented the conversions to and from
 * IEEE double-precision format, added more precision to the extended
 * conversions, and accommodated conversions involving +/- infinity,
 * NaN's, and denormalized numbers.
 */

#ifndef HUGE_VAL
# define HUGE_VAL HUGE
#endif /* HUGE_VAL */

/****************************************************************
 * The following two routines make up for deficiencies in many
 * compilers to convert properly between unsigned integers and
 * floating-point.  Some compilers which have this bug are the
 * THINK_C compiler for the Macintosh and the C compiler for the
 * Silicon Graphics MIPS-based Iris.
 ****************************************************************/

# define UnsignedToFloat(u)	(((double)((long)((u) - 2147483647L - 1))) + 2147483648.0)

/****************************************************************
 * Extended precision IEEE floating-point conversion routines.
 * Extended is an 80-bit number as defined by Motorola,
 * with a sign bit, 15 bits of exponent (offset 16383?),
 * and a 64-bit mantissa, with no hidden bit.
 ****************************************************************/

static double ConvertFromIeeeExtended(const char *bytes)
{
	double	f;
	long	expon;
	unsigned long hiMant, loMant;
	
	expon = ((bytes[0] & 0x7F) << 8) | (bytes[1] & 0xFF);
	hiMant	=	((unsigned long)(bytes[2] & 0xFF) << 24)
			|	((unsigned long)(bytes[3] & 0xFF) << 16)
			|	((unsigned long)(bytes[4] & 0xFF) << 8)
			|	((unsigned long)(bytes[5] & 0xFF));
	loMant	=	((unsigned long)(bytes[6] & 0xFF) << 24)
			|	((unsigned long)(bytes[7] & 0xFF) << 16)
			|	((unsigned long)(bytes[8] & 0xFF) << 8)
			|	((unsigned long)(bytes[9] & 0xFF));

	if (expon == 0 && hiMant == 0 && loMant == 0) {
		f = 0;
	}
	else {
		if (expon == 0x7FFF) {	/* Infinity or NaN */
			f = HUGE_VAL;
		}
		else {
			expon -= 16383;
			f  = ldexp(UnsignedToFloat(hiMant), expon-=31);
			f += ldexp(UnsignedToFloat(loMant), expon-=32);
		}
	}

	if (bytes[0] & 0x80)
		return -f;
	else
		return f;
}
