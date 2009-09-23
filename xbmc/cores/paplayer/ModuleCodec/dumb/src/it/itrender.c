/*  _______         ____    __         ___    ___
 * \    _  \       \    /  \  /       \   \  /   /       '   '  '
 *  |  | \  \       |  |    ||         |   \/   |         .      .
 *  |  |  |  |      |  |    ||         ||\  /|  |
 *  |  |  |  |      |  |    ||         || \/ |  |         '  '  '
 *  |  |  |  |      |  |    ||         ||    |  |         .      .
 *  |  |_/  /        \  \__//          ||    |  |
 * /_______/ynamic    \____/niversal  /__\  /____\usic   /|  .  . ibliotheque
 *                                                      /  \
 *                                                     / .  \
 * itrender.c - Code to render an Impulse Tracker     / / \  \
 *              module.                              | <  /   \_
 *                                                   |  \/ /\   /
 * Written - painstakingly - by entheh.               \_  /  > /
 *                                                      | \ / /
 *                                                      |  ' /
 *                                                       \__/
 */

#include <math.h>
#include <stdlib.h>

#include "dumb.h"
#include "internal/it.h"



static IT_PLAYING *dup_playing(IT_PLAYING *src, IT_CHANNEL *dstchannel, IT_CHANNEL *srcchannel)
{
	IT_PLAYING *dst;

	if (!src) return NULL;

	dst = malloc(sizeof(*dst));
	if (!dst) return NULL;

	dst->flags = src->flags;

	ASSERT(src->channel);
	dst->channel = &dstchannel[src->channel - srcchannel];
	dst->sample = src->sample;
	dst->instrument = src->instrument;
	dst->env_instrument = src->env_instrument;

	dst->sampnum = src->sampnum;
	dst->instnum = src->instnum;

	dst->channel_volume = src->channel_volume;

	dst->volume = src->volume;
	dst->pan = src->pan;

	dst->note = src->note;

	dst->filter_cutoff = src->filter_cutoff;
	dst->filter_resonance = src->filter_resonance;

	dst->true_filter_cutoff = src->true_filter_cutoff;
	dst->true_filter_resonance = src->true_filter_resonance;

	dst->vibrato_speed = src->vibrato_speed;
	dst->vibrato_depth = src->vibrato_depth;
	dst->vibrato_n = src->vibrato_n;
	dst->vibrato_time = src->vibrato_time;

	dst->tremolo_speed = src->tremolo_speed;
	dst->tremolo_depth = src->tremolo_depth;
	dst->tremolo_time = src->tremolo_time;

	dst->sample_vibrato_time = src->sample_vibrato_time;
	dst->sample_vibrato_depth = src->sample_vibrato_depth;

	dst->slide = src->slide;
	dst->delta = src->delta;

	dst->volume_envelope = src->volume_envelope;
	dst->pan_envelope = src->pan_envelope;
	dst->pitch_envelope = src->pitch_envelope;

	dst->fadeoutcount = src->fadeoutcount;

	dst->filter_state[0] = src->filter_state[0];
	dst->filter_state[1] = src->filter_state[1];

	dst->resampler = src->resampler;
	dst->resampler.pickup_data = dst;
	dst->time_lost = src->time_lost;

	return dst;
}



static void dup_channel(IT_CHANNEL *dst, IT_CHANNEL *src)
{
	dst->flags = src->flags;

	dst->volume = src->volume;
	dst->volslide = src->volslide;
	dst->xm_volslide = src->xm_volslide;
	dst->panslide = src->panslide;

	dst->pan = src->pan;
	dst->truepan = src->truepan;

	dst->channelvolume = src->channelvolume;
	dst->channelvolslide = src->channelvolslide;

	dst->instrument = src->instrument;
	dst->note = src->note;

	dst->SFmacro = src->SFmacro;

	dst->filter_cutoff = src->filter_cutoff;
	dst->filter_resonance = src->filter_resonance;

	dst->key_off_count = src->key_off_count;
	dst->note_cut_count = src->note_cut_count;
	dst->note_delay_count = src->note_delay_count;
	dst->note_delay_entry = src->note_delay_entry;

	dst->arpeggio = src->arpeggio;
	dst->retrig = src->retrig;
	dst->xm_retrig = src->xm_retrig;
	dst->retrig_tick = src->retrig_tick;

	dst->tremor_time = src->tremor_time;

	dst->portamento = src->portamento;
	dst->toneporta = src->toneporta;
	dst->destnote = src->destnote;

	dst->sample = src->sample;
	dst->truenote = src->truenote;

	dst->midi_state = src->midi_state;

	dst->lastvolslide = src->lastvolslide;
	dst->lastDKL = src->lastDKL;
	dst->lastEF = src->lastEF;
	dst->lastG = src->lastG;
	dst->lastHspeed = src->lastHspeed;
	dst->lastHdepth = src->lastHdepth;
	dst->lastRspeed = src->lastRspeed;
	dst->lastRdepth = src->lastRdepth;
	dst->lastI = src->lastI;
	dst->lastJ = src->lastJ;
	dst->lastN = src->lastN;
	dst->lastO = src->lastO;
	dst->high_offset = src->high_offset;
	dst->lastP = src->lastP;
	dst->lastQ = src->lastQ;
	dst->lastS = src->lastS;
	dst->pat_loop_row = src->pat_loop_row;
	dst->pat_loop_count = src->pat_loop_count;
	dst->pat_loop_end_row = src->pat_loop_end_row;
	dst->lastW = src->lastW;

	dst->xm_lastE1 = src->xm_lastE1;
	dst->xm_lastE2 = src->xm_lastE2;
	dst->xm_lastEA = src->xm_lastEA;
	dst->xm_lastEB = src->xm_lastEB;
	dst->xm_lastX1 = src->xm_lastX1;
	dst->xm_lastX2 = src->xm_lastX2;

	dst->playing = dup_playing(src->playing, dst, src);
}



/* Allocate the new callbacks first, then pass them to this function!
 * It will free them on failure.
 */
static DUMB_IT_SIGRENDERER *dup_sigrenderer(DUMB_IT_SIGRENDERER *src, int n_channels, IT_CALLBACKS *callbacks)
{
	DUMB_IT_SIGRENDERER *dst;
	int i;

	if (!src) {
		if (callbacks) free(callbacks);
		return NULL;
	}

	dst = malloc(sizeof(*dst));
	if (!dst) {
		if (callbacks) free(callbacks);
		return NULL;
	}

	dst->sigdata = src->sigdata;

	dst->n_channels = n_channels;

	dst->globalvolume = src->globalvolume;
	dst->globalvolslide = src->globalvolslide;

	dst->tempo = src->tempo;
	dst->temposlide = src->temposlide;

	for (i = 0; i < DUMB_IT_N_CHANNELS; i++)
		dup_channel(&dst->channel[i], &src->channel[i]);

	for (i = 0; i < DUMB_IT_N_NNA_CHANNELS; i++)
		dst->playing[i] = dup_playing(src->playing[i], dst->channel, src->channel);

	dst->tick = src->tick;
	dst->speed = src->speed;
	dst->rowcount = src->rowcount;

	dst->order = src->order;
	dst->row = src->row;
	dst->processorder = src->processorder;
	dst->processrow = src->processrow;
	dst->breakrow = src->breakrow;
	dst->pat_loop_row = src->pat_loop_row;

	dst->n_rows = src->n_rows;

	dst->entry_start = src->entry_start;
	dst->entry = src->entry;
	dst->entry_end = src->entry_end;

	dst->time_left = src->time_left;
	dst->sub_time_left = src->sub_time_left;

	dst->click_remover = NULL;

	dst->callbacks = callbacks;

	return dst;
}



static IT_MIDI default_midi = {
	/* unsigned char SFmacro[16][16]; */
	{
		{0xF0, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
	},
	/* unsigned char SFmacrolen[16]; */
	{4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	/* unsigned short SFmacroz[16]; */
	/* Bitfield; bit 0 set = z in first position */
	{
		0x0008, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
		0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000
	},
	/* unsigned char Zmacro[128][16]; */
	{
		{0xF0, 0xF0, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0xF0, 0xF0, 0x01, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0xF0, 0xF0, 0x01, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0xF0, 0xF0, 0x01, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0xF0, 0xF0, 0x01, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0xF0, 0xF0, 0x01, 0x28, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0xF0, 0xF0, 0x01, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0xF0, 0xF0, 0x01, 0x38, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0xF0, 0xF0, 0x01, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0xF0, 0xF0, 0x01, 0x48, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0xF0, 0xF0, 0x01, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0xF0, 0xF0, 0x01, 0x58, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0xF0, 0xF0, 0x01, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0xF0, 0xF0, 0x01, 0x68, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0xF0, 0xF0, 0x01, 0x70, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0xF0, 0xF0, 0x01, 0x78, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},

		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},

		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},

		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},

		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},

		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},

		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},

		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
	},
	/* unsigned char Zmacrolen[128]; */
	{
		4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	}
};



static void it_reset_filter_state(IT_FILTER_STATE *state)
{
	state->currsample = 0;
	state->prevsample = 0;
}



#define LOG10 2.30258509299

/* IMPORTANT: This function expects one extra sample in 'src' so it can apply
 * click removal. It reads size samples, starting from src[0], and writes its
 * output starting at dst[pos]. The pos parameter is required for getting
 * click removal right.
 */
static void it_filter(DUMB_CLICK_REMOVER *cr, IT_FILTER_STATE *state, sample_t *dst, long pos, sample_t *src, long size, int step, int sampfreq, int cutoff, int resonance)
{
	sample_t currsample = state->currsample;
	sample_t prevsample = state->prevsample;

	float a, b, c;

	long datasize;

	{
		float inv_angle = (float)(sampfreq * pow(0.5, 0.25 + cutoff*(1.0/(24<<IT_ENVELOPE_SHIFT))) * (1.0/(2*3.14159265358979323846*110.0)));
		float loss = (float)exp(resonance*(-LOG10*1.2/128.0));
		float d, e;
#if 0
		loss *= 2; // This is the mistake most players seem to make!
#endif

#if 1
		d = (1.0f - loss) / inv_angle;
		if (d > 2.0f) d = 2.0f;
		d = (loss - d) * inv_angle;
		e = inv_angle * inv_angle;
		a = 1.0f / (1.0f + d + e);
		c = -e * a;
		b = 1.0f - a - c;
#else
		a = 1.0f / (inv_angle*inv_angle + inv_angle*loss + loss);
		c = -(inv_angle*inv_angle) * a;
		b = 1.0f - a - c;
#endif
	}

	dst += pos * step;
	datasize = size * step;

#define INT_FILTERS
#ifdef INT_FILTERS
#define MULSCA(a, b) ((int)((LONG_LONG)((a) << 4) * (b) >> 32))
#define SCALEB 12
	{
		int ai = (int)(a * (1 << (16+SCALEB)));
		int bi = (int)(b * (1 << (16+SCALEB)));
		int ci = (int)(c * (1 << (16+SCALEB)));
		int i;

		if (cr) {
			sample_t startstep = MULSCA(src[0], ai) + MULSCA(currsample, bi) + MULSCA(prevsample, ci);
			dumb_record_click(cr, pos, startstep);
		}

		for (i = 0; i < datasize; i += step) {
			{
				sample_t newsample = MULSCA(src[i], ai) + MULSCA(currsample, bi) + MULSCA(prevsample, ci);
				prevsample = currsample;
				currsample = newsample;
			}
			dst[i] += currsample;
		}

		if (cr) {
			sample_t endstep = MULSCA(src[datasize], ai) + MULSCA(currsample, bi) + MULSCA(prevsample, ci);
			dumb_record_click(cr, pos + size, -endstep);
		}
	}
#else
#error This version is broken - it does not use step, and state should contain floats for it
	if (cr) {
		float startstep = src[0]*a + currsample*b + prevsample*c;
		dumb_record_click(cr, pos, (sample_t)startstep);
	}

	{
		int i = size % 3;
		while (i > 0) {
			{
				float newsample = *src++*a + currsample*b + prevsample*c;
				prevsample = currsample;
				currsample = newsample;
			}
			*dst++ += (sample_t)currsample;
			i--;
		}
		i = size / 3;
		while (i > 0) {
			float newsample;
			/* Gotta love unrolled loops! */
			*dst++ += (sample_t)(newsample = *src++*a + currsample*b + prevsample*c);
			*dst++ += (sample_t)(prevsample = *src++*a + newsample*b + currsample*c);
			*dst++ += (sample_t)(currsample = *src++*a + prevsample*b + newsample*c);
			i--;
		}
	}

	if (cr) {
		float endstep = src[datasize]*a + currsample*b + prevsample*c;
		dumb_record_click(cr, pos + size, -(sample_t)endstep);
	}
#endif

	state->currsample = currsample;
	state->prevsample = prevsample;
}

#undef LOG10



static signed char it_sine[256] = {
	  0,  2,  3,  5,  6,  8,  9, 11, 12, 14, 16, 17, 19, 20, 22, 23,
	 24, 26, 27, 29, 30, 32, 33, 34, 36, 37, 38, 39, 41, 42, 43, 44,
	 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 56, 57, 58, 59,
	 59, 60, 60, 61, 61, 62, 62, 62, 63, 63, 63, 64, 64, 64, 64, 64,
	 64, 64, 64, 64, 64, 64, 63, 63, 63, 62, 62, 62, 61, 61, 60, 60,
	 59, 59, 58, 57, 56, 56, 55, 54, 53, 52, 51, 50, 49, 48, 47, 46,
	 45, 44, 43, 42, 41, 39, 38, 37, 36, 34, 33, 32, 30, 29, 27, 26,
	 24, 23, 22, 20, 19, 17, 16, 14, 12, 11,  9,  8,  6,  5,  3,  2,
	  0, -2, -3, -5, -6, -8, -9,-11,-12,-14,-16,-17,-19,-20,-22,-23,
	-24,-26,-27,-29,-30,-32,-33,-34,-36,-37,-38,-39,-41,-42,-43,-44,
	-45,-46,-47,-48,-49,-50,-51,-52,-53,-54,-55,-56,-56,-57,-58,-59,
	-59,-60,-60,-61,-61,-62,-62,-62,-63,-63,-63,-64,-64,-64,-64,-64,
	-64,-64,-64,-64,-64,-64,-63,-63,-63,-62,-62,-62,-61,-61,-60,-60,
	-59,-59,-58,-57,-56,-56,-55,-54,-53,-52,-51,-50,-49,-48,-47,-46,
	-45,-44,-43,-42,-41,-39,-38,-37,-36,-34,-33,-32,-30,-29,-27,-26,
	-24,-23,-22,-20,-19,-17,-16,-14,-12,-11, -9, -8, -6, -5, -3, -2
};



#if 0
/** WARNING: use these! */
/** JULIEN: Plus for XM compatibility it could be interesting to rename
 * it_sawtooth[] to it_rampdown[], and add an it_rampup[].
 * Also, still for XM compat', twood be good if it was possible to tell the
 * the player not to retrig' the waveform on a new instrument.
 * Both of these are only for completness though, as I don't think it would
 * be very noticeable ;)
 */
/** ENTHEH: IT also has the 'don't retrig' thingy :) */

static signed char it_sawtooth[256] = {
	 64, 63, 63, 62, 62, 61, 61, 60, 60, 59, 59, 58, 58, 57, 57, 56,
	 56, 55, 55, 54, 54, 53, 53, 52, 52, 51, 51, 50, 50, 49, 49, 48,
	 48, 47, 47, 46, 46, 45, 45, 44, 44, 43, 43, 42, 42, 41, 41, 40,
	 40, 39, 39, 38, 38, 37, 37, 36, 36, 35, 35, 34, 34, 33, 33, 32,
	 32, 31, 31, 30, 30, 29, 29, 28, 28, 27, 27, 26, 26, 25, 25, 24,
	 24, 23, 23, 22, 22, 21, 21, 20, 20, 19, 19, 18, 18, 17, 17, 16,
	 16, 15, 15, 14, 14, 13, 13, 12, 12, 11, 11, 10, 10,  9,  9,  8,
	  8,  7,  7,  6,  6,  5,  5,  4,  4,  3,  3,  2,  2,  1,  1,  0,
	  0, -1, -1, -2, -2, -3, -3, -4, -4, -5, -5, -6, -6, -7, -7, -8,
	 -8, -9, -9,-10,-10,-11,-11,-12,-12,-13,-13,-14,-14,-15,-15,-16,
	-16,-17,-17,-18,-18,-19,-19,-20,-20,-21,-21,-22,-22,-23,-23,-24,
	-24,-25,-25,-26,-26,-27,-27,-28,-28,-29,-29,-30,-30,-31,-31,-32,
	-32,-33,-33,-34,-34,-35,-35,-36,-36,-37,-37,-38,-38,-39,-39,-40,
	-40,-41,-41,-42,-42,-43,-43,-44,-44,-45,-45,-46,-46,-47,-47,-48,
	-48,-49,-49,-50,-50,-51,-51,-52,-52,-53,-53,-54,-54,-55,-55,-56,
	-56,-57,-57,-58,-58,-59,-59,-60,-60,-61,-61,-62,-62,-63,-63,-64
};



static signed char it_squarewave[256] = {
	 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
	 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
	 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
	 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
	 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
	 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
	 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
	 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
	  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0
};

#endif



static void reset_tick_counts(DUMB_IT_SIGRENDERER *sigrenderer)
{
	int i;

	for (i = 0; i < DUMB_IT_N_CHANNELS; i++) {
		IT_CHANNEL *channel = &sigrenderer->channel[i];
		channel->key_off_count = 0;
		channel->note_cut_count = 0;
		channel->note_delay_count = 0;
	}
}



static void reset_effects(DUMB_IT_SIGRENDERER *sigrenderer)
{
	int i;

	sigrenderer->globalvolslide = 0;
	sigrenderer->temposlide = 0;

	for (i = 0; i < DUMB_IT_N_CHANNELS; i++) {
		IT_CHANNEL *channel = &sigrenderer->channel[i];
		channel->volslide = 0;
		channel->xm_volslide = 0;
		channel->panslide = 0;
		channel->channelvolslide = 0;
		channel->arpeggio = 0;
		channel->retrig = 0;
		if (channel->xm_retrig) {
			channel->xm_retrig = 0;
			channel->retrig_tick = 0;
		}
		channel->tremor_time &= 127;
		channel->portamento = 0;
		channel->toneporta = 0;
		if (channel->playing) {
			channel->playing->vibrato_n = 0;
			channel->playing->tremolo_speed = 0;
			channel->playing->tremolo_depth = 0;
		}
	}
}



static void update_tremor(IT_CHANNEL *channel)
{
	if ((channel->tremor_time & 128) && channel->playing) {
		if (channel->tremor_time == 128)
			channel->tremor_time = (channel->lastI >> 4) | 192;
		else if (channel->tremor_time == 192)
			channel->tremor_time = (channel->lastI & 15) | 128;
		else
			channel->tremor_time--;
	}
}



static void it_pickup_loop(DUMB_RESAMPLER *resampler, void *data)
{
	resampler->pos -= resampler->end - resampler->start;
	((IT_PLAYING *)data)->time_lost += resampler->end - resampler->start;
}



static void it_pickup_pingpong_loop(DUMB_RESAMPLER *resampler, void *data)
{
	if (resampler->dir < 0) {
		resampler->pos = (resampler->start << 1) - 1 - resampler->pos;
		resampler->subpos ^= 65535;
		resampler->dir = 1;
		((IT_PLAYING *)data)->time_lost += (resampler->end - resampler->start) << 1;
	} else {
		resampler->pos = (resampler->end << 1) - 1 - resampler->pos;
		resampler->subpos ^= 65535;
		resampler->dir = -1;
	}
}



static void it_pickup_stop_at_end(DUMB_RESAMPLER *resampler, void *data)
{
	(void)data;

	if (resampler->dir < 0) {
		resampler->pos = (resampler->start << 1) - 1 - resampler->pos;
		resampler->subpos ^= 65535;
		/* By rights, time_lost would be updated here. However, there is no
		 * need at this point; it will not be used.
		 *
		 * ((IT_PLAYING *)data)->time_lost += (resampler->src_end - resampler->src_start) << 1;
		 */
		resampler->dir = 1;
	} else
		resampler->dir = 0;
}



static void it_playing_update_resamplers(IT_PLAYING *playing)
{
	if ((playing->sample->flags & IT_SAMPLE_SUS_LOOP) && !(playing->flags & IT_PLAYING_SUSTAINOFF)) {
		playing->resampler.start = playing->sample->sus_loop_start;
		playing->resampler.end = playing->sample->sus_loop_end;
		if (playing->sample->flags & IT_SAMPLE_PINGPONG_SUS_LOOP)
			playing->resampler.pickup = &it_pickup_pingpong_loop;
		else
			playing->resampler.pickup = &it_pickup_loop;
	} else if (playing->sample->flags & IT_SAMPLE_LOOP) {
		playing->resampler.start = playing->sample->loop_start;
		playing->resampler.end = playing->sample->loop_end;
		if (playing->sample->flags & IT_SAMPLE_PINGPONG_LOOP)
			playing->resampler.pickup = &it_pickup_pingpong_loop;
		else
			playing->resampler.pickup = &it_pickup_loop;
	} else {
		if (playing->sample->flags & IT_SAMPLE_SUS_LOOP)
			playing->resampler.start = playing->sample->sus_loop_start;
		else
			playing->resampler.start = 0;
		playing->resampler.end = playing->sample->length;
		playing->resampler.pickup = &it_pickup_stop_at_end;
	}
	ASSERT(playing->resampler.pickup_data == playing);
}



/* This should be called whenever the sample or sample position changes. */
static void it_playing_reset_resamplers(IT_PLAYING *playing, long pos)
{
	int bits = playing->sample->flags & IT_SAMPLE_16BIT ? 16 : 8;
	int channels = playing->sample->flags & IT_SAMPLE_STEREO ? 2 : 1;
	dumb_reset_resampler_n(bits, &playing->resampler, playing->sample->data, channels, pos, 0, 0);
	playing->resampler.pickup_data = playing;
	playing->time_lost = 0;
	playing->flags &= ~IT_PLAYING_DEAD;
	it_playing_update_resamplers(playing);
}



static void update_retrig(IT_CHANNEL *channel)
{
	if (channel->xm_retrig) {
		channel->retrig_tick--;
		if (channel->retrig_tick <= 0) {
			if (channel->playing) it_playing_reset_resamplers(channel->playing, 0);
			channel->retrig_tick = channel->xm_retrig;
		}
	} else if (channel->retrig & 0x0F) {
		channel->retrig_tick--;
		if (channel->retrig_tick <= 0) {
			if (channel->retrig < 0x10) {
			} else if (channel->retrig < 0x20) {
				channel->volume--;
				if (channel->volume > 64) channel->volume = 0;
			} else if (channel->retrig < 0x30) {
				channel->volume -= 2;
				if (channel->volume > 64) channel->volume = 0;
			} else if (channel->retrig < 0x40) {
				channel->volume -= 4;
				if (channel->volume > 64) channel->volume = 0;
			} else if (channel->retrig < 0x50) {
				channel->volume -= 8;
				if (channel->volume > 64) channel->volume = 0;
			} else if (channel->retrig < 0x60) {
				channel->volume -= 16;
				if (channel->volume > 64) channel->volume = 0;
			} else if (channel->retrig < 0x70) {
				channel->volume <<= 1;
				channel->volume /= 3;
			} else if (channel->retrig < 0x80) {
				channel->volume >>= 1;
			} else if (channel->retrig < 0x90) {
			} else if (channel->retrig < 0xA0) {
				channel->volume++;
				if (channel->volume > 64) channel->volume = 64;
			} else if (channel->retrig < 0xB0) {
				channel->volume += 2;
				if (channel->volume > 64) channel->volume = 64;
			} else if (channel->retrig < 0xC0) {
				channel->volume += 4;
				if (channel->volume > 64) channel->volume = 64;
			} else if (channel->retrig < 0xD0) {
				channel->volume += 8;
				if (channel->volume > 64) channel->volume = 64;
			} else if (channel->retrig < 0xE0) {
				channel->volume += 16;
				if (channel->volume > 64) channel->volume = 64;
			} else if (channel->retrig < 0xF0) {
				channel->volume *= 3;
				channel->volume >>= 1;
				if (channel->volume > 64) channel->volume = 64;
			} else {
				channel->volume <<= 1;
				if (channel->volume > 64) channel->volume = 64;
			}
			if (channel->playing) it_playing_reset_resamplers(channel->playing, 0);
			channel->retrig_tick = channel->retrig & 0x0F;
		}
	}
}



static void update_smooth_effects(DUMB_IT_SIGRENDERER *sigrenderer)
{
	int i;

	for (i = 0; i < DUMB_IT_N_CHANNELS; i++) {
		IT_CHANNEL *channel = &sigrenderer->channel[i];
		IT_PLAYING *playing = channel->playing;

		if (playing) {
			playing->vibrato_time += playing->vibrato_n *
			                         (playing->vibrato_speed << 2);
			playing->tremolo_time += playing->tremolo_speed << 2;
		}
	}
}



static void update_effects(DUMB_IT_SIGRENDERER *sigrenderer)
{
	int i;

	if (sigrenderer->globalvolslide) {
		sigrenderer->globalvolume += sigrenderer->globalvolslide;
		if (sigrenderer->globalvolume > 128) {
			if (sigrenderer->globalvolslide >= 0)
				sigrenderer->globalvolume = 128;
			else
				sigrenderer->globalvolume = 0;
		}
	}

	if (sigrenderer->temposlide) {
		sigrenderer->tempo += sigrenderer->temposlide;
		if (sigrenderer->tempo < 32) {
			if (sigrenderer->temposlide >= 0)
				sigrenderer->tempo = 255;
			else
				sigrenderer->tempo = 32;
		}
	}

	for (i = 0; i < DUMB_IT_N_CHANNELS; i++) {
		IT_CHANNEL *channel = &sigrenderer->channel[i];
		IT_PLAYING *playing = channel->playing;

		if (channel->xm_volslide) {
			channel->volume += channel->xm_volslide;
			if (channel->volume > 64) {
				if (channel->xm_volslide >= 0)
					channel->volume = 64;
				else
					channel->volume = 0;
			}
		}

		if (channel->volslide) {
			channel->volume += channel->volslide;
			if (channel->volume > 64) {
				if (channel->volslide >= 0)
					channel->volume = 64;
				else
					channel->volume = 0;
			}
		}

		if (channel->panslide && !IT_IS_SURROUND(channel->pan)) {
			if (sigrenderer->sigdata->flags & IT_WAS_AN_XM) {
				if (channel->panslide == -128)
					channel->truepan = 32;
				else
					channel->truepan = MID(32, channel->truepan + channel->panslide*64, 32+255*64);
			} else {
				channel->pan += channel->panslide;
				if (channel->pan > 64) {
					if (channel->panslide >= 0)
						channel->pan = 64;
					else
						channel->pan = 0;
				}
				channel->truepan = channel->pan << IT_ENVELOPE_SHIFT;
			}
		}

		if (channel->channelvolslide) {
			channel->channelvolume += channel->channelvolslide;
			if (channel->channelvolume > 64) {
				if (channel->channelvolslide >= 0)
					channel->channelvolume = 64;
				else
					channel->channelvolume = 0;
			}
			if (channel->playing)
				channel->playing->channel_volume = channel->channelvolume;
		}

		update_tremor(channel);

		channel->arpeggio = (channel->arpeggio << 4) | (channel->arpeggio >> 8);
		channel->arpeggio &= 0xFFF;

		update_retrig(channel);

		if (playing) {
			playing->slide += channel->portamento;

			if (sigrenderer->sigdata->flags & IT_LINEAR_SLIDES) {
				if (channel->toneporta && channel->destnote < 120) {
					int currpitch = ((playing->note - 60) << 8) + playing->slide;
					int destpitch = (channel->destnote - 60) << 8;
					if (currpitch > destpitch) {
						currpitch -= channel->toneporta;
						if (currpitch < destpitch) {
							currpitch = destpitch;
							channel->destnote = IT_NOTE_OFF;
						}
					} else if (currpitch < destpitch) {
						currpitch += channel->toneporta;
						if (currpitch > destpitch) {
							currpitch = destpitch;
							channel->destnote = IT_NOTE_OFF;
						}
					}
					playing->slide = currpitch - ((playing->note - 60) << 8);
				}
			} else {
				if (channel->toneporta && channel->destnote < 120) {
					float amiga_multiplier = playing->sample->C5_speed * (1.0f / AMIGA_DIVISOR);

					float deltanote = (float)pow(DUMB_SEMITONE_BASE, 60 - playing->note);
					/* deltanote is 1.0 for C-5, 0.5 for C-6, etc. */

					float deltaslid = deltanote - playing->slide * amiga_multiplier;

					float destdelta = (float)pow(DUMB_SEMITONE_BASE, 60 - channel->destnote);
					if (deltaslid < destdelta) {
						playing->slide -= channel->toneporta;
						deltaslid = deltanote - playing->slide * amiga_multiplier;
						if (deltaslid > destdelta) {
							playing->note = channel->destnote;
							playing->slide = 0;
							channel->destnote = IT_NOTE_OFF;
						}
					} else {
						playing->slide += channel->toneporta;
						deltaslid = deltanote - playing->slide * amiga_multiplier;
						if (deltaslid < destdelta) {
							playing->note = channel->destnote;
							playing->slide = 0;
							channel->destnote = IT_NOTE_OFF;
						}
					}
				}
			}
		}
	}

	update_smooth_effects(sigrenderer);
}



// This function should be renamed; it doesn't do the 'Update Pattern Variables' operation ittech.txt describes
/* Returns 1 if a pattern loop is happening. */
static int update_pattern_variables(DUMB_IT_SIGRENDERER *sigrenderer, IT_ENTRY *entry)
{
	IT_CHANNEL *channel = &sigrenderer->channel[(int)entry->channel];

	if (entry->mask & IT_ENTRY_EFFECT) {
		switch (entry->effect) {
			case IT_JUMP_TO_ORDER:
				sigrenderer->breakrow = 0;
				sigrenderer->processorder = entry->effectvalue - 1;
				sigrenderer->processrow = 0xFFFE;
				break;

			case IT_S:
				{
					unsigned char effectvalue = entry->effectvalue;
					if (effectvalue == 0)
						effectvalue = channel->lastS;
					channel->lastS = effectvalue;
					switch (effectvalue >> 4) {
						//case IT_S7:
						case IT_S_PATTERN_LOOP:
							{
								unsigned char v = effectvalue & 15;
								if (v == 0)
									channel->pat_loop_row = sigrenderer->processrow;
								else {
									if (channel->pat_loop_count == 0) {
										channel->pat_loop_count = v;
										sigrenderer->breakrow = channel->pat_loop_row;
										if (sigrenderer->sigdata->flags & IT_WAS_AN_XM) {
											/* For XM files, if a loop occurs by itself, keep breakrow set for when the pattern ends - fun bug in FT2! */
											if (sigrenderer->processrow < 0xFFFE) {
												/* Infinite pattern loops are possible, so we check whether the pattern loop we're hitting now is earlier than the last one we hit. */
												if (sigrenderer->processrow < channel->pat_loop_end_row)
													sigrenderer->processorder = 0xFFFE; /* suspect infinite loop, so trigger loop callback */
												else
													sigrenderer->processorder = 0xFFFF; /* don't trigger loop callback */
												channel->pat_loop_end_row = sigrenderer->processrow;
												sigrenderer->processrow = 0xFFFF; /* special case: don't reset breakrow or pat_loop_end_row */
											}
										} else {
											/* IT files do this regardless of other flow control effects seen here. */
											sigrenderer->processorder = 0xFFFF; /* special case: don't trigger loop callback */
											sigrenderer->processrow = 0xFFFE;
										}
										return 1;
									} else if (--channel->pat_loop_count) {
										sigrenderer->breakrow = channel->pat_loop_row;
										if (sigrenderer->sigdata->flags & IT_WAS_AN_XM) {
											/* For XM files, if a loop occurs by itself, keep breakrow set for when the pattern ends - fun bug in FT2! */
											if (sigrenderer->processrow < 0xFFFE) {
												/* Infinite pattern loops are possible, so we check whether the pattern loop we're hitting now is earlier than the last one we hit. */
												if (sigrenderer->processrow < channel->pat_loop_end_row)
													sigrenderer->processorder = 0xFFFE; /* suspect infinite loop, so trigger loop callback */
												else
													sigrenderer->processorder = 0xFFFF; /* don't trigger loop callback */
												channel->pat_loop_end_row = sigrenderer->processrow;
												sigrenderer->processrow = 0xFFFF; /* special case: don't reset breakrow or pat_loop_end_row */
											}
										} else {
											/* IT files do this regardless of other flow control effects seen here. */
											sigrenderer->processorder = 0xFFFF; /* special case: don't trigger loop callback */
											sigrenderer->processrow = 0xFFFE;
										}
										return 1;
									} else if (sigrenderer->sigdata->flags & IT_WAS_AN_XM) {
										channel->pat_loop_end_row = 0;
										// TODO
										/* Findings:
										- If a pattern loop completes successfully, and then the pattern terminates, then the next pattern will start on the row corresponding to the E60.
										- If a pattern loop doesn't do any loops, and then the pattern terminates, then the next pattern will start on the first row.
										- If a break appears to the left of the pattern loop, it jumps into the relevant position in the next pattern, and that's it.
										- If a break appears to the right of the pattern loop, it jumps to the start of the next pattern, and that's it.
										- If we jump, then effect a loop using an old E60, and then the pattern ends, the next pattern starts on the row corresponding to the E60.
										- Theory: breakrow is not cleared when it's a pattern loop effect!
										*/
										//if (sigrenderer->processrow < 0xFFFE) // I have no idea if this is correct or not - FT2 is so weird :(
										//	sigrenderer->breakrow = channel->pat_loop_row; /* emulate bug in FT2 */
									} else
										channel->pat_loop_row = sigrenderer->processrow + 1;
								}
							}
							break;
						case IT_S_PATTERN_DELAY:
							sigrenderer->rowcount = 1 + (effectvalue & 15);
							break;
					}
				}
		}
	}

	return 0;
}



/* This function guarantees that channel->sample will always be valid if it
 * is nonzero. In other words, to check if it is valid, simply check if it is
 * nonzero.
 */
static void instrument_to_sample(DUMB_IT_SIGDATA *sigdata, IT_CHANNEL *channel)
{
	if (sigdata->flags & IT_USE_INSTRUMENTS) {
		if (channel->instrument >= 1 && channel->instrument <= sigdata->n_instruments) {
			if (channel->note < 120) {
				channel->sample = sigdata->instrument[channel->instrument-1].map_sample[channel->note];
				channel->truenote = sigdata->instrument[channel->instrument-1].map_note[channel->note];
			} else
				channel->sample = 0;
		} else
			channel->sample = 0;
	} else {
		channel->sample = channel->instrument;
		channel->truenote = channel->note;
	}
	if (!(channel->sample >= 1 && channel->sample <= sigdata->n_samples && (sigdata->sample[channel->sample-1].flags & IT_SAMPLE_EXISTS)))
		channel->sample = 0;
}



static void fix_sample_looping(IT_PLAYING *playing)
{
	if ((playing->sample->flags & (IT_SAMPLE_LOOP | IT_SAMPLE_SUS_LOOP)) ==
	                              (IT_SAMPLE_LOOP | IT_SAMPLE_SUS_LOOP)) {
		if (playing->resampler.dir < 0) {
			playing->resampler.pos = (playing->sample->sus_loop_end << 1) - 1 - playing->resampler.pos;
			playing->resampler.subpos ^= 65535;
			playing->resampler.dir = 1;
		}

		playing->resampler.pos += playing->time_lost;
	}
}



static void it_compatible_gxx_retrigger(DUMB_IT_SIGDATA *sigdata, IT_CHANNEL *channel)
{
	channel->playing->volume_envelope.next_node = 0;
	channel->playing->volume_envelope.tick = 0;
	channel->playing->pan_envelope.next_node = 0;
	channel->playing->pan_envelope.tick = 0;
	channel->playing->pitch_envelope.next_node = 0;
	channel->playing->pitch_envelope.tick = 0;
	channel->playing->fadeoutcount = 1024;
	// Should we remove IT_PLAYING_BACKGROUND? Test with sample with sustain loop...
	channel->playing->flags &= ~(IT_PLAYING_BACKGROUND | IT_PLAYING_SUSTAINOFF | IT_PLAYING_FADING | IT_PLAYING_DEAD);
	it_playing_update_resamplers(channel->playing);

	if (channel->sample)
		if (sigdata->flags & IT_USE_INSTRUMENTS)
			channel->playing->env_instrument = &sigdata->instrument[channel->instrument-1];
}



static void it_note_off(IT_PLAYING *playing)
{
	if (playing) {
		playing->flags |= IT_PLAYING_BACKGROUND | IT_PLAYING_SUSTAINOFF;
		fix_sample_looping(playing);
		it_playing_update_resamplers(playing);
		if (playing->instrument)
			if ((playing->instrument->volume_envelope.flags & (IT_ENVELOPE_ON | IT_ENVELOPE_LOOP_ON)) != IT_ENVELOPE_ON)
				playing->flags |= IT_PLAYING_FADING;
	}
}



static void xm_note_off(DUMB_IT_SIGDATA *sigdata, IT_CHANNEL *channel)
{
	if (channel->playing) {
		if (!(sigdata->instrument[channel->instrument-1].volume_envelope.flags & IT_ENVELOPE_ON))
			//if (!(entry->mask & IT_ENTRY_INSTRUMENT))
			// dunno what that was there for ...
				channel->volume = 0;
		channel->playing->flags |= IT_PLAYING_SUSTAINOFF | IT_PLAYING_FADING;
		it_playing_update_resamplers(channel->playing);
	}
}



static void it_retrigger_note(DUMB_IT_SIGRENDERER *sigrenderer, IT_CHANNEL *channel)
{
	DUMB_IT_SIGDATA *sigdata = sigrenderer->sigdata;
	unsigned char nna;
	int i;

	if (channel->playing) {
#ifdef INVALID_NOTES_CAUSE_NOTE_CUT
		if (channel->note == IT_NOTE_OFF)
			nna = NNA_NOTE_OFF;
		else if (channel->note >= 120 || !channel->playing->instrument || (channel->playing->flags & IT_PLAYING_DEAD))
			nna = NNA_NOTE_CUT;
		else
			nna = channel->playing->instrument->new_note_action;
#else
		if (channel->note == IT_NOTE_CUT)
			nna = NNA_NOTE_CUT;
		if (channel->note >= 120)
			nna = NNA_NOTE_OFF;
		else if (!channel->playing->instrument || (channel->playing->flags & IT_PLAYING_DEAD))
			nna = NNA_NOTE_CUT;
		else
			nna = channel->playing->instrument->new_note_action;
#endif

		switch (nna) {
			case NNA_NOTE_CUT:
				free(channel->playing);
				channel->playing = NULL;
				break;
			case NNA_NOTE_OFF:
				it_note_off(channel->playing);
				break;
			case NNA_NOTE_FADE:
				channel->playing->flags |= IT_PLAYING_BACKGROUND | IT_PLAYING_FADING;
				break;
		}
	}

	if (channel->sample == 0 || channel->note >= 120)
		return;

	channel->destnote = IT_NOTE_OFF;

	if (channel->playing) {
		for (i = 0; i < DUMB_IT_N_NNA_CHANNELS; i++) {
			if (!sigrenderer->playing[i]) {
				sigrenderer->playing[i] = channel->playing;
				channel->playing = NULL;
				break;
			}
		}
/** WARNING - come up with some more heuristics for replacing old notes */
#if 0
		if (channel->playing) {
			for (i = 0; i < DUMB_IT_N_NNA_CHANNELS; i++) {
				if (sigrenderer->playing[i]->flags & IT_PLAYING_BACKGROUND) {
					write_seqtime();
					sequence_c(SEQUENCE_STOP_SIGNAL);
					sequence_c(i);
					channel->VChannel = &module->VChannel[i];
					break;
				}
			}
		}
#endif
	}

	if (channel->playing)
		free(channel->playing);

	channel->playing = malloc(sizeof(*channel->playing));

	if (!channel->playing)
		return;

	channel->playing->flags = 0;
	channel->playing->channel = channel;
	channel->playing->sample = &sigdata->sample[channel->sample-1];
	if (sigdata->flags & IT_USE_INSTRUMENTS)
		channel->playing->instrument = &sigdata->instrument[channel->instrument-1];
	else
		channel->playing->instrument = NULL;
	channel->playing->env_instrument = channel->playing->instrument;
	channel->playing->sampnum = channel->sample;
	channel->playing->instnum = channel->instrument;
	channel->playing->channel_volume = channel->channelvolume;
	channel->playing->note = channel->truenote;
	channel->playing->filter_cutoff = 127;
	channel->playing->filter_resonance = 0;
	channel->playing->true_filter_cutoff = 127 << 8;
	channel->playing->true_filter_resonance = 0;
	channel->playing->vibrato_speed = 0;
	channel->playing->vibrato_depth = 0;
	channel->playing->vibrato_n = 0;
	channel->playing->vibrato_time = 0;
	channel->playing->tremolo_speed = 0;
	channel->playing->tremolo_depth = 0;
	channel->playing->tremolo_time = 0;
	channel->playing->sample_vibrato_time = 0;
	channel->playing->sample_vibrato_depth = 0;
	channel->playing->slide = 0;
	channel->playing->volume_envelope.next_node = 0;
	channel->playing->volume_envelope.tick = 0;
	channel->playing->pan_envelope.next_node = 0;
	channel->playing->pan_envelope.tick = 0;
	channel->playing->pitch_envelope.next_node = 0;
	channel->playing->pitch_envelope.tick = 0;
	channel->playing->fadeoutcount = 1024;
	it_reset_filter_state(&channel->playing->filter_state[0]);
	it_reset_filter_state(&channel->playing->filter_state[1]);
	it_playing_reset_resamplers(channel->playing, 0);

	/** WARNING - is everything initialised? */
}



static void get_default_volpan(DUMB_IT_SIGDATA *sigdata, IT_CHANNEL *channel)
{
	if (channel->sample == 0)
		return;

	channel->volume = sigdata->sample[channel->sample-1].default_volume;

	if (sigdata->flags & IT_WAS_AN_XM) {
		if (!(sigdata->flags & IT_WAS_A_MOD))
			channel->truepan = 32 + sigdata->sample[channel->sample-1].default_pan*64;
		return;
	}

	{
		int pan = sigdata->sample[channel->sample-1].default_pan;
		if (pan >= 128 && pan <= 192) {
			channel->pan = pan - 128;
			return;
		}
	}

	if (sigdata->flags & IT_USE_INSTRUMENTS) {
		IT_INSTRUMENT *instrument = &sigdata->instrument[channel->instrument-1];
		if (instrument->default_pan <= 64)
			channel->pan = instrument->default_pan;
		if (instrument->filter_cutoff >= 128)
			channel->filter_cutoff = instrument->filter_cutoff - 128;
		if (instrument->filter_resonance >= 128)
			channel->filter_resonance = instrument->filter_resonance - 128;
	}
}



static void get_true_pan(DUMB_IT_SIGDATA *sigdata, IT_CHANNEL *channel)
{
	channel->truepan = channel->pan << IT_ENVELOPE_SHIFT;

	if (!IT_IS_SURROUND_SHIFTED(channel->truepan) && (sigdata->flags & IT_USE_INSTRUMENTS)) {
		IT_INSTRUMENT *instrument = &sigdata->instrument[channel->instrument-1];
		int truepan = channel->truepan;
		truepan += (channel->note - instrument->pp_centre) * instrument->pp_separation << (IT_ENVELOPE_SHIFT - 3);
		channel->truepan = (unsigned short)MID(0, truepan, 64 << IT_ENVELOPE_SHIFT);
	}
}



static void post_process_it_volpan(DUMB_IT_SIGRENDERER *sigrenderer, IT_ENTRY *entry)
{
	IT_CHANNEL *channel = &sigrenderer->channel[(int)entry->channel];

	if (entry->mask & IT_ENTRY_VOLPAN) {
		if (entry->volpan <= 84) {
			/* Volume */
			/* Fine volume slide up */
			/* Fine volume slide down */
		} else if (entry->volpan <= 94) {
			/* Volume slide up */
			unsigned char v = entry->volpan - 85;
			if (v == 0)
				v = channel->lastvolslide;
			channel->lastvolslide = v;
			/* = effect Dx0 where x == entry->volpan - 85 */
			channel->volslide = v;
		} else if (entry->volpan <= 104) {
			/* Volume slide down */
			unsigned char v = entry->volpan - 95;
			if (v == 0)
				v = channel->lastvolslide;
			channel->lastvolslide = v;
			/* = effect D0x where x == entry->volpan - 95 */
			channel->volslide = -v;
		} else if (entry->volpan <= 114) {
			/* Portamento down */
			unsigned char v = (entry->volpan - 105) << 2;
			if (v == 0)
				v = channel->lastEF;
			channel->lastEF = v;
			channel->portamento -= v << 4;
		} else if (entry->volpan <= 124) {
			/* Portamento up */
			unsigned char v = (entry->volpan - 115) << 2;
			if (v == 0)
				v = channel->lastEF;
			channel->lastEF = v;
			channel->portamento += v << 4;
		} else if (entry->volpan <= 202) {
			/* Pan */
			/* Tone Portamento */
		} else if (entry->volpan <= 212) {
			/* Vibrato */
			/* This is unaffected by IT_OLD_EFFECTS. However, if v == 0, then any doubling of depth that happened before (with Hxy in the effect column) will be preserved. */
			unsigned char v = entry->volpan - 203;
			if (v == 0)
				v = channel->lastHdepth;
			else {
				v <<= 2;
				channel->lastHdepth = v;
			}
			if (channel->playing) {
				channel->playing->vibrato_speed = channel->lastHspeed;
				channel->playing->vibrato_depth = v;
				channel->playing->vibrato_n++;
			}
		}
	}
}



static void it_send_midi(DUMB_IT_SIGRENDERER *sigrenderer, IT_CHANNEL *channel, unsigned char midi_byte)
{
	if (sigrenderer->callbacks->midi)
		if ((*sigrenderer->callbacks->midi)(sigrenderer->callbacks->midi_data, channel - sigrenderer->channel, midi_byte))
			return;

	switch (channel->midi_state) {
		case 4: /* Ready to receive resonance parameter */
			if (midi_byte < 0x80) channel->filter_resonance = midi_byte;
			channel->midi_state = 0;
			break;
		case 3: /* Ready to receive cutoff parameter */
			if (midi_byte < 0x80) channel->filter_cutoff = midi_byte;
			channel->midi_state = 0;
			break;
		case 2: /* Ready for byte specifying which parameter will follow */
			if (midi_byte == 0) /* Cutoff */
				channel->midi_state = 3;
			else if (midi_byte == 1) /* Resonance */
				channel->midi_state = 4;
			else
				channel->midi_state = 0;
			break;
		default: /* Counting initial F0 bytes */
			switch (midi_byte) {
				case 0xF0:
					channel->midi_state++;
					break;
				case 0xFA:
				case 0xFC:
				case 0xFF:
					/* Reset filter parameters for all channels */
					{
						int i;
						for (i = 0; i < DUMB_IT_N_CHANNELS; i++) {
							sigrenderer->channel[i].filter_cutoff = 127;
							sigrenderer->channel[i].filter_resonance = 0;
							//// should we be resetting channel[i].playing->filter_* here?
						}
					}
					/* Fall through */
				default:
					channel->midi_state = 0;
					break;
			}
	}
}



static void xm_envelope_calculate_value(IT_ENVELOPE *envelope, IT_PLAYING_ENVELOPE *pe)
{
	if (pe->next_node <= 0)
		pe->value = envelope->node_y[0] << IT_ENVELOPE_SHIFT;
	else if (pe->next_node >= envelope->n_nodes)
		pe->value = envelope->node_y[envelope->n_nodes-1] << IT_ENVELOPE_SHIFT;
	else {
		int ys = envelope->node_y[pe->next_node-1] << IT_ENVELOPE_SHIFT;
		int ts = envelope->node_t[pe->next_node-1];
		int te = envelope->node_t[pe->next_node];

		if (ts == te)
			pe->value = ys;
		else {
			int ye = envelope->node_y[pe->next_node] << IT_ENVELOPE_SHIFT;
			int t = pe->tick;

			pe->value = ys + (ye - ys) * (t - ts) / (te - ts);
		}
	}
}



/* Returns 1 if a callback caused termination of playback. */
static int process_effects(DUMB_IT_SIGRENDERER *sigrenderer, IT_ENTRY *entry, int ignore_cxx)
{
	DUMB_IT_SIGDATA *sigdata = sigrenderer->sigdata;

	IT_CHANNEL *channel = &sigrenderer->channel[(int)entry->channel];

	if (entry->mask & IT_ENTRY_EFFECT) {
		switch (entry->effect) {
/*
Notes about effects (as compared to other module formats)

C               This is now in *HEX*. (Used to be in decimal in ST3)
E/F/G/H/U       You need to check whether the song uses Amiga/Linear slides.
H/U             Vibrato in Impulse Tracker is two times finer than in
                any other tracker and is updated EVERY tick.
                If "Old Effects" is *ON*, then the vibrato is played in the
                normal manner (every non-row tick and normal depth)
E/F/G           These commands ALL share the same memory.
Oxx             Offsets to samples are to the 'xx00th' SAMPLE. (ie. for
                16 bit samples, the offset is xx00h*2)
                Oxx past the sample end will be ignored, unless "Old Effects"
                is ON, in which case the Oxx will play from the end of the
                sample.
Yxy             This uses a table 4 times larger (hence 4 times slower) than
                vibrato or tremelo. If the waveform is set to random, then
                the 'speed' part of the command is interpreted as a delay.
*/
			case IT_SET_SPEED:
				if (entry->effectvalue)
					sigrenderer->tick = sigrenderer->speed = entry->effectvalue;
				else if (sigdata->flags & IT_WAS_AN_XM) {
					sigrenderer->speed = 0;
					if (sigrenderer->callbacks->xm_speed_zero && (*sigrenderer->callbacks->xm_speed_zero)(sigrenderer->callbacks->xm_speed_zero_data))
						return 1;
				}
				break;

			case IT_BREAK_TO_ROW:
				if (ignore_cxx) break;
				sigrenderer->breakrow = entry->effectvalue;
				sigrenderer->processrow = 0xFFFE;
				break;

			case IT_VOLSLIDE_VIBRATO:
				if (channel->playing) {
					channel->playing->vibrato_speed = channel->lastHspeed;
					channel->playing->vibrato_depth = channel->lastHdepth;
					channel->playing->vibrato_n++;
				}
				/* Fall through and process volume slide. */
			case IT_VOLUME_SLIDE:
			case IT_VOLSLIDE_TONEPORTA:
				/* The tone portamento component is handled elsewhere. */
				{
					unsigned char v = entry->effectvalue;
					if (!(sigdata->flags & IT_WAS_A_MOD)) {
						if (v == 0)
							v = channel->lastDKL;
						channel->lastDKL = v;
					}
					if ((v & 0x0F) == 0) { /* Dx0 */
						channel->volslide = v >> 4;
						if (channel->volslide == 15 && !(sigdata->flags & IT_WAS_AN_XM)) {
							channel->volume += 15;
							if (channel->volume > 64) channel->volume = 64;
						}
					} else if ((v & 0xF0) == 0) { /* D0x */
						channel->volslide = -v;
						if (channel->volslide == -15 && !(sigdata->flags & IT_WAS_AN_XM)) {
							channel->volume -= 15;
							if (channel->volume > 64) channel->volume = 0;
						}
					} else if ((v & 0x0F) == 0x0F) { /* DxF */
						channel->volume += v >> 4;
						if (channel->volume > 64) channel->volume = 64;
					} else if ((v & 0xF0) == 0xF0) { /* DFx */
						channel->volume -= v & 15;
						if (channel->volume > 64) channel->volume = 0;
					}
				}
				break;
			case IT_XM_FINE_VOLSLIDE_DOWN:
				{
					unsigned char v = entry->effectvalue;
					if (v == 0)
						v = channel->xm_lastEB;
					channel->xm_lastEB = v;
					channel->volume -= v;
					if (channel->volume > 64) channel->volume = 0;
				}
				break;
			case IT_XM_FINE_VOLSLIDE_UP:
				{
					unsigned char v = entry->effectvalue;
					if (v == 0)
						v = channel->xm_lastEA;
					channel->xm_lastEA = v;
					channel->volume += v;
					if (channel->volume > 64) channel->volume = 64;
				}
				break;
			case IT_PORTAMENTO_DOWN:
				{
					unsigned char v = entry->effectvalue;
					if (sigdata->flags & IT_WAS_AN_XM) {
						if (!(sigdata->flags & IT_WAS_A_MOD)) {
							if (v == 0xF0)
								v |= channel->xm_lastE2;
							else if (v >= 0xF0)
								channel->xm_lastE2 = v & 15;
							else if (v == 0xE0)
								v |= channel->xm_lastX2;
							else
								channel->xm_lastX2 = v & 15;
						}
					} else {
						if (v == 0)
							v = channel->lastEF;
						channel->lastEF = v;
					}
					if (channel->playing) {
						if ((v & 0xF0) == 0xF0)
							channel->playing->slide -= (v & 15) << 4;
						else if ((v & 0xF0) == 0xE0)
							channel->playing->slide -= (v & 15) << 2;
						else
							channel->portamento -= v << 4;
					}
				}
				break;
			case IT_PORTAMENTO_UP:
				{
					unsigned char v = entry->effectvalue;
					if (sigdata->flags & IT_WAS_AN_XM) {
						if (!(sigdata->flags & IT_WAS_A_MOD)) {
							if (v == 0xF0)
								v |= channel->xm_lastE1;
							else if (v >= 0xF0)
								channel->xm_lastE1 = v & 15;
							else if (v == 0xE0)
								v |= channel->xm_lastX1;
							else
								channel->xm_lastX1 = v & 15;
						}
					} else {
						if (v == 0)
							v = channel->lastEF;
						channel->lastEF = v;
					}
					if (channel->playing) {
						if ((v & 0xF0) == 0xF0)
							channel->playing->slide += (v & 15) << 4;
						else if ((v & 0xF0) == 0xE0)
							channel->playing->slide += (v & 15) << 2;
						else
							channel->portamento += v << 4;
					}
				}
				break;
			case IT_XM_PORTAMENTO_DOWN:
				{
					unsigned char v = entry->effectvalue;
					if (!(sigdata->flags & IT_WAS_A_MOD)) {
						if (v == 0)
							v = channel->lastJ;
						channel->lastJ = v;
					}
					if (channel->playing)
						channel->portamento -= v << 4;
				}
				break;
			case IT_XM_PORTAMENTO_UP:
				{
					unsigned char v = entry->effectvalue;
					if (!(sigdata->flags & IT_WAS_A_MOD)) {
						if (v == 0)
							v = channel->lastEF;
						channel->lastEF = v;
					}
					if (channel->playing)
						channel->portamento += v << 4;
				}
				break;
			case IT_XM_KEY_OFF:
				channel->key_off_count = entry->effectvalue;
				if (!channel->key_off_count) xm_note_off(sigdata, channel);
				break;
			case IT_VIBRATO:
				{
					unsigned char speed = entry->effectvalue >> 4;
					unsigned char depth = entry->effectvalue & 15;
					if (speed == 0)
						speed = channel->lastHspeed;
					channel->lastHspeed = speed;
					if (depth == 0)
						depth = channel->lastHdepth;
					else {
						if (sigdata->flags & IT_OLD_EFFECTS)
							depth <<= 3;
						else
							depth <<= 2;
						channel->lastHdepth = depth;
					}
					if (channel->playing) {
						channel->playing->vibrato_speed = speed;
						channel->playing->vibrato_depth = depth;
						channel->playing->vibrato_n++;
					}
				}
				break;
			case IT_TREMOR:
				{
					unsigned char v = entry->effectvalue;
					if (v == 0)
						v = channel->lastI;
					else if (!(sigdata->flags & IT_OLD_EFFECTS)) {
						if (v & 0xF0) v -= 0x10;
						if (v & 0x0F) v -= 0x01;
					}
					channel->lastI = v;
					channel->tremor_time |= 128;
				}
				update_tremor(channel);
				break;
			case IT_ARPEGGIO:
				{
					unsigned char v = entry->effectvalue;
					/* XM files have no memory for arpeggio (000 = no effect)
					 * and we use lastJ for portamento down instead.
					 */
					if (!(sigdata->flags & IT_WAS_AN_XM)) {
						if (v == 0)
							v = channel->lastJ;
						channel->lastJ = v;
					}
					channel->arpeggio = v;
				}
				break;
			case IT_SET_CHANNEL_VOLUME:
				if (sigdata->flags & IT_WAS_AN_XM)
					channel->volume = MIN(entry->effectvalue, 64);
				else if (entry->effectvalue <= 64)
					channel->channelvolume = entry->effectvalue;
#ifdef VOLUME_OUT_OF_RANGE_SETS_MAXIMUM
				else
					channel->channelvolume = 64;
#endif
				if (channel->playing)
					channel->playing->channel_volume = channel->channelvolume;
				break;
			case IT_CHANNEL_VOLUME_SLIDE:
				{
					unsigned char v = entry->effectvalue;
					if (v == 0)
						v = channel->lastN;
					channel->lastN = v;
					if ((v & 0x0F) == 0) { /* Nx0 */
						channel->channelvolslide = v >> 4;
					} else if ((v & 0xF0) == 0) { /* N0x */
						channel->channelvolslide = -v;
					} else {
						if ((v & 0x0F) == 0x0F) { /* NxF */
							channel->channelvolume += v >> 4;
							if (channel->channelvolume > 64) channel->channelvolume = 64;
						} else if ((v & 0xF0) == 0xF0) { /* NFx */
							channel->channelvolume -= v & 15;
							if (channel->channelvolume > 64) channel->channelvolume = 0;
						} else
							break;
						if (channel->playing)
							channel->playing->channel_volume = channel->channelvolume;
					}
				}
				break;
			case IT_SET_SAMPLE_OFFSET:
				{
					unsigned char v = entry->effectvalue;
					if (sigdata->flags & IT_WAS_A_MOD) {
						if (v == 0) break;
					} else {
						if (v == 0)
							v = channel->lastO;
						channel->lastO = v;
					}
					/* Note: we set the offset even if tone portamento is
					 * specified. Impulse Tracker does the same.
					 */
					if (entry->mask & IT_ENTRY_NOTE) {
						if (channel->playing) {
							int offset = ((int)channel->high_offset << 16) | ((int)v << 8);
							IT_PLAYING *playing = channel->playing;
							IT_SAMPLE *sample = playing->sample;
							int end;
							if ((sample->flags & IT_SAMPLE_SUS_LOOP) && !(playing->flags & IT_PLAYING_SUSTAINOFF))
								end = sample->sus_loop_end;
							else if (sample->flags & IT_SAMPLE_LOOP)
								end = sample->loop_end;
							else
								end = sample->length;
							if (offset < end)
								it_playing_reset_resamplers(playing, offset);
							else if (sigdata->flags & IT_OLD_EFFECTS)
								it_playing_reset_resamplers(playing, end);
						}
					}
				}
				break;
			case IT_PANNING_SLIDE:
				/** JULIEN: guess what? the docs are wrong! (how unusual ;)
				 * Pxy seems to memorize its previous value... and there
				 * might be other mistakes like that... (sigh!)
				 */
				/** ENTHEH: umm... but... the docs say that Pxy memorises its
				 * value... don't they? :o
				 */
				{
					unsigned char v = entry->effectvalue;
					int p = channel->truepan;
					if (sigdata->flags & IT_WAS_AN_XM)
						p >>= 6;
					else {
						p = (p + 128) >> 8;
						channel->pan = p;
					}
					if (v == 0)
						v = channel->lastP;
					channel->lastP = v;
					if ((v & 0x0F) == 0) { /* Px0 */
						channel->panslide = v >> 4;
					} else if ((v & 0xF0) == 0) { /* P0x */
						channel->panslide = -v;
					} else if ((v & 0x0F) == 0x0F) { /* PxF */
						p += v >> 4;
					} else if ((v & 0xF0) == 0xF0) { /* PFx */
						p -= v & 15;
					}
					if (sigdata->flags & IT_WAS_AN_XM)
						channel->truepan = 32 + MID(0, p, 255) * 64;
					else if (!IT_IS_SURROUND(channel->pan)) {
						channel->pan = p;
						channel->truepan = p << 8;
					}
				}
				break;
			case IT_RETRIGGER_NOTE:
				{
					unsigned char v = entry->effectvalue;
					if (sigdata->flags & IT_WAS_AN_XM) {
						if ((v & 0x0F) == 0) v |= channel->lastQ & 0x0F;
						if ((v & 0xF0) == 0) v |= channel->lastQ & 0xF0;
					} else {
						if (v == 0)
							v = channel->lastQ;
					}
					channel->lastQ = v;
					if ((v & 0x0F) == 0) v |= 0x01;
					channel->retrig = v;
					if (entry->mask & IT_ENTRY_NOTE) {
						channel->retrig_tick = v & 0x0F;
						/* Emulate a bug */
						if (sigdata->flags & IT_WAS_AN_XM)
							update_retrig(channel);
					} else
						update_retrig(channel);
				}
				break;
			case IT_XM_RETRIGGER_NOTE:
				channel->retrig_tick = channel->xm_retrig = entry->effectvalue;
				if (entry->effectvalue == 0)
					if (channel->playing) it_playing_reset_resamplers(channel->playing, 0);
				break;
			case IT_TREMOLO:
				{
					unsigned char speed = entry->effectvalue >> 4;
					unsigned char depth = entry->effectvalue & 15;
					if (speed == 0)
						speed = channel->lastRspeed;
					channel->lastRspeed = speed;
					if (depth == 0)
						depth = channel->lastRdepth;
					channel->lastRdepth = depth;
					if (channel->playing) {
						channel->playing->tremolo_speed = speed;
						channel->playing->tremolo_depth = depth;
					}
				}
				break;
			case IT_S:
				{
					/* channel->lastS was set in update_pattern_variables(). */
					unsigned char effectvalue = channel->lastS;
					switch (effectvalue >> 4) {
						//case IT_S_SET_FILTER:
						//case IT_S_SET_GLISSANDO_CONTROL:
						//case IT_S_FINETUNE:
						//case IT_S_SET_VIBRATO_WAVEFORM:
						//case IT_S_SET_TREMOLO_WAVEFORM:
						//case IT_S_SET_PANBRELLO_WAVEFORM:
							/* Waveforms for commands S3x, S4x and S5x:
							 *   0: Sine wave
							 *   1: Ramp down
							 *   2: Square wave
							 *   3: Random wave
							 */
						case IT_S_FINE_PATTERN_DELAY:
							sigrenderer->tick += effectvalue & 15;
							break;
						//case IT_S7:
						case IT_S_SET_PAN:
							ASSERT(!(sigdata->flags & IT_WAS_AN_XM));
							channel->pan =
								((effectvalue & 15) << 2) |
								((effectvalue & 15) >> 2);
							channel->truepan = channel->pan << IT_ENVELOPE_SHIFT;
							break;
						case IT_S_SET_SURROUND_SOUND:
							if ((effectvalue & 15) == 1) {
								channel->pan = IT_SURROUND;
								channel->truepan = channel->pan << IT_ENVELOPE_SHIFT;
							}
							break;
						case IT_S_SET_HIGH_OFFSET:
							channel->high_offset = effectvalue & 15;
							break;
						//case IT_S_PATTERN_LOOP:
						case IT_S_DELAYED_NOTE_CUT:
							channel->note_cut_count = effectvalue & 15;
							if (!channel->note_cut_count) {
								if (sigdata->flags & IT_WAS_AN_XM)
									channel->volume = 0;
								else
									channel->note_cut_count = 1;
							}
							break;
						case IT_S_SET_MIDI_MACRO:
							channel->SFmacro = effectvalue & 15;
							break;
					}
				}
				break;
			case IT_SET_SONG_TEMPO:
				{
					unsigned char v = entry->effectvalue;
					if (v == 0)
						v = channel->lastW;
					channel->lastW = v;
					if (v < 0x10)
						sigrenderer->temposlide = -v;
					else if (v < 0x20)
						sigrenderer->temposlide = v & 15;
					else
						sigrenderer->tempo = v;
				}
				break;
			case IT_FINE_VIBRATO:
				{
					unsigned char speed = entry->effectvalue >> 4;
					unsigned char depth = entry->effectvalue & 15;
					if (speed == 0)
						speed = channel->lastHspeed;
					channel->lastHspeed = speed;
					if (depth == 0)
						depth = channel->lastHdepth;
					else {
						if (sigdata->flags & IT_OLD_EFFECTS)
							depth <<= 1;
						channel->lastHdepth = depth;
					}
					if (channel->playing) {
						channel->playing->vibrato_speed = speed;
						channel->playing->vibrato_depth = depth;
						channel->playing->vibrato_n++;
					}
				}
				break;
			case IT_SET_GLOBAL_VOLUME:
				if (entry->effectvalue <= 128)
					sigrenderer->globalvolume = entry->effectvalue;
#ifdef VOLUME_OUT_OF_RANGE_SETS_MAXIMUM
				else
					sigrenderer->globalvolume = 128;
#endif
				break;
			case IT_GLOBAL_VOLUME_SLIDE:
				{
					unsigned char v = entry->effectvalue;
					if (v == 0)
						v = channel->lastW;
					channel->lastW = v;
					if ((v & 0x0F) == 0) { /* Wx0 */
						sigrenderer->globalvolslide =
							(sigdata->flags & IT_WAS_AN_XM) ? (v >> 4)*2 : (v >> 4);
					} else if ((v & 0xF0) == 0) { /* W0x */
						sigrenderer->globalvolslide =
							(sigdata->flags & IT_WAS_AN_XM) ? (-v)*2 : (-v);
					} else if ((v & 0x0F) == 0x0F) { /* WxF */
						sigrenderer->globalvolume += v >> 4;
						if (sigrenderer->globalvolume > 128) sigrenderer->globalvolume = 128;
					} else if ((v & 0xF0) == 0xF0) { /* WFx */
						sigrenderer->globalvolume -= v & 15;
						if (sigrenderer->globalvolume > 128) sigrenderer->globalvolume = 0;
					}
				}
				break;
			case IT_SET_PANNING:
				if (sigdata->flags & IT_WAS_AN_XM)
					channel->truepan = 32 + entry->effectvalue*64;
				else {
					channel->pan = (entry->effectvalue + 2) >> 2;
					channel->truepan = channel->pan << IT_ENVELOPE_SHIFT;
				}
				break;
			//case IT_PANBRELLO:
			case IT_MIDI_MACRO:
				{
					IT_MIDI *midi = sigdata->midi ? sigdata->midi : &default_midi;
					if (entry->effectvalue >= 0x80) {
						int n = midi->Zmacrolen[entry->effectvalue-0x80];
						int i;
						for (i = 0; i < n; i++)
							it_send_midi(sigrenderer, channel, midi->Zmacro[entry->effectvalue-0x80][i]);
					} else {
						int n = midi->SFmacrolen[channel->SFmacro];
						int i, j;
						for (i = 0, j = 1; i < n; i++, j <<= 1)
							it_send_midi(sigrenderer, channel,
								(unsigned char)(midi->SFmacroz[channel->SFmacro] & j ?
									entry->effectvalue : midi->SFmacro[channel->SFmacro][i]));
					}
				}
				break;
			case IT_XM_SET_ENVELOPE_POSITION:
				if (channel->playing && channel->playing->env_instrument) {
					IT_ENVELOPE *envelope = &channel->playing->env_instrument->volume_envelope;
					if (envelope->flags & IT_ENVELOPE_ON) {
						IT_PLAYING_ENVELOPE *pe = &channel->playing->volume_envelope;
						pe->tick = entry->effectvalue;
						if (pe->tick >= envelope->node_t[envelope->n_nodes-1])
							pe->tick = envelope->node_t[envelope->n_nodes-1];
						pe->next_node = 0;
						while (pe->tick > envelope->node_t[pe->next_node]) pe->next_node++;
						xm_envelope_calculate_value(envelope, pe);
					}
				}
				break;
		}
	}

	if (!(sigdata->flags & IT_WAS_AN_XM))
		post_process_it_volpan(sigrenderer, entry);

	return 0;
}



static int process_it_note_data(DUMB_IT_SIGRENDERER *sigrenderer, IT_ENTRY *entry)
{
	DUMB_IT_SIGDATA *sigdata = sigrenderer->sigdata;
	IT_CHANNEL *channel = &sigrenderer->channel[(int)entry->channel];

	// When tone portamento and instrument are specified:
	// If Gxx is off:
	//   - same sample, do nothing but portamento
	//   - diff sample, retrigger all but keep current note+slide + do porta
	//   - if instrument is invalid, nothing; if sample is invalid, cut
	// If Gxx is on:
	//   - same sample or new sample invalid, retrigger envelopes and initialise note value for portamento to 'seek' to
	//   - diff sample/inst, start using new envelopes
	// When tone portamento is specified alone, sample won't change.
	// TODO: consider what happens with instrument alone after all this...

	if (entry->mask & (IT_ENTRY_NOTE | IT_ENTRY_INSTRUMENT)) {
		if (entry->mask & IT_ENTRY_INSTRUMENT)
			channel->instrument = entry->instrument;
		instrument_to_sample(sigdata, channel);
		if (channel->note < 120) {
			if ((sigdata->flags & IT_USE_INSTRUMENTS) && channel->sample == 0)
				return 1;
			if (entry->mask & IT_ENTRY_INSTRUMENT)
				get_default_volpan(sigdata, channel);
		} else
			it_retrigger_note(sigrenderer, channel); /* Stop the note */
	}

	/** WARNING: This is not ideal, since channel->playing might not get allocated owing to lack of memory... */
	if (((entry->mask & IT_ENTRY_VOLPAN) && entry->volpan >= 193 && entry->volpan <= 202) ||
	    ((entry->mask & IT_ENTRY_EFFECT) && (entry->effect == IT_TONE_PORTAMENTO || entry->effect == IT_VOLSLIDE_TONEPORTA)))
	{
		if (channel->playing && (entry->mask & IT_ENTRY_INSTRUMENT)) {
			if (sigdata->flags & IT_COMPATIBLE_GXX)
				it_compatible_gxx_retrigger(sigdata, channel);
			else if ((!(sigdata->flags & IT_USE_INSTRUMENTS) ||
				(channel->instrument >= 1 && channel->instrument <= sigdata->n_instruments)) &&
				channel->sample != channel->playing->sampnum)
			{
				unsigned char note = channel->playing->note;
				int slide = channel->playing->slide;
				it_retrigger_note(sigrenderer, channel);
				if (channel->playing) {
					channel->playing->note = note;
					channel->playing->slide = slide;
					// Should we be preserving sample_vibrato_time? depth?
				}
			}
		}

		if ((entry->mask & IT_ENTRY_VOLPAN) && entry->volpan >= 193 && entry->volpan <= 202) {
			/* Tone Portamento in the volume column */
			static const unsigned char slidetable[] = {0, 1, 4, 8, 16, 32, 64, 96, 128, 255};
			unsigned char v = slidetable[entry->volpan - 193];
			if (sigdata->flags & IT_COMPATIBLE_GXX) {
				if (v == 0)
					v = channel->lastG;
				channel->lastG = v;
			} else {
				if (v == 0)
					v = channel->lastEF;
				channel->lastEF = v;
			}
			if ((entry->mask & IT_ENTRY_NOTE) || ((sigdata->flags & IT_COMPATIBLE_GXX) && (entry->mask & IT_ENTRY_INSTRUMENT))) {
				if (channel->sample)
					channel->destnote = channel->truenote;
				else
					channel->destnote = channel->note;
			}
			channel->toneporta = v << 4;
		} else {
			/* Tone Portamento in the effect column */
			unsigned char v;
			if (entry->effect == IT_TONE_PORTAMENTO)
				v = entry->effectvalue;
			else
				v = 0;
			if (sigdata->flags & IT_COMPATIBLE_GXX) {
				if (v == 0)
					v = channel->lastG;
				channel->lastG = v;
			} else {
				if (v == 0)
					v = channel->lastEF;
				channel->lastEF = v;
			}
			if ((entry->mask & IT_ENTRY_NOTE) || ((sigdata->flags & IT_COMPATIBLE_GXX) && (entry->mask & IT_ENTRY_INSTRUMENT))) {
				if (channel->sample)
					channel->destnote = channel->truenote;
				else
					channel->destnote = channel->note;
			}
			channel->toneporta = v << 4;
		}
		if (channel->playing) goto skip_start_note;
	}

	if ((entry->mask & IT_ENTRY_NOTE) ||
		((entry->mask & IT_ENTRY_INSTRUMENT) && (!channel->playing || entry->instrument != channel->playing->instnum)))
	{
		if (channel->note < 120) {
			get_true_pan(sigdata, channel);
			it_retrigger_note(sigrenderer, channel);
		}
	}

	skip_start_note:

	if (entry->mask & IT_ENTRY_VOLPAN) {
		if (entry->volpan <= 64) {
			/* Volume */
			channel->volume = entry->volpan;
		} else if (entry->volpan <= 74) {
			/* Fine volume slide up */
			unsigned char v = entry->volpan - 65;
			if (v == 0)
				v = channel->lastvolslide;
			channel->lastvolslide = v;
			/* = effect DxF where x == entry->volpan - 65 */
			channel->volume += v;
			if (channel->volume > 64) channel->volume = 64;
		} else if (entry->volpan <= 84) {
			/* Fine volume slide down */
			unsigned char v = entry->volpan - 75;
			if (v == 0)
				v = channel->lastvolslide;
			channel->lastvolslide = v;
			/* = effect DFx where x == entry->volpan - 75 */
			channel->volume -= v;
			if (channel->volume > 64) channel->volume = 0;
		} else if (entry->volpan < 128) {
			/* Volume slide up */
			/* Volume slide down */
			/* Portamento down */
			/* Portamento up */
		} else if (entry->volpan <= 192) {
			/* Pan */
			channel->pan = entry->volpan - 128;
			channel->truepan = channel->pan << IT_ENVELOPE_SHIFT;
		}
		/* else */
		/* Tone Portamento */
		/* Vibrato */
	}
	return 0;
}



static void retrigger_xm_envelopes(IT_PLAYING *playing)
{
	playing->volume_envelope.next_node = 0;
	playing->volume_envelope.tick = -1;
	playing->pan_envelope.next_node = 0;
	playing->pan_envelope.tick = -1;
	playing->fadeoutcount = 1024;
}



static void process_xm_note_data(DUMB_IT_SIGRENDERER *sigrenderer, IT_ENTRY *entry)
{
	DUMB_IT_SIGDATA *sigdata = sigrenderer->sigdata;
	IT_CHANNEL *channel = &sigrenderer->channel[(int)entry->channel];

	if (entry->mask & IT_ENTRY_INSTRUMENT) {
		channel->instrument = entry->instrument;
		instrument_to_sample(sigdata, channel);
		if (channel->playing) {
			/* Retrigger vol/pan envelopes if enabled, and cancel fadeout.
			 * Also reset vol/pan to that of _original_ instrument.
			 */
			channel->playing->flags &= ~(IT_PLAYING_SUSTAINOFF | IT_PLAYING_FADING);
			it_playing_update_resamplers(channel->playing);

			channel->volume = channel->playing->sample->default_volume;
			if (!(sigdata->flags & IT_WAS_A_MOD))
				channel->truepan = 32 + channel->playing->sample->default_pan*64;

			retrigger_xm_envelopes(channel->playing);
		}
	}

	if (!((entry->mask & IT_ENTRY_EFFECT) && entry->effect == IT_XM_KEY_OFF && entry->effectvalue == 0) &&
		(entry->mask & IT_ENTRY_NOTE))
	{
		if (!(entry->mask & IT_ENTRY_INSTRUMENT))
			instrument_to_sample(sigdata, channel);

		if (channel->note >= 120)
			xm_note_off(sigdata, channel);
		else if (channel->sample == 0) {
			/** If we get here, one of the following is the case:
			 ** 1. The instrument has never been specified on this channel.
			 ** 2. The specified instrument is invalid.
			 ** 3. The instrument has no sample mapped to the selected note.
			 ** What should happen?
			 **
			 ** Experimentation shows that any existing note stops and cannot
			 ** be brought back. A subsequent instrument change fixes that.
			 **/
			if (channel->playing) {
				free(channel->playing);
				channel->playing = NULL;
			}
			return;
		} else if (channel->playing && (entry->mask & IT_ENTRY_VOLPAN) && ((entry->volpan>>4) == 0xF)) {
			/* Don't retrigger note; portamento in the volume column. */
		} else if (channel->playing &&
		           (entry->mask & IT_ENTRY_EFFECT) &&
		           (entry->effect == IT_TONE_PORTAMENTO ||
		            entry->effect == IT_VOLSLIDE_TONEPORTA)) {
			/* Don't retrigger note; portamento in the effects column. */
		} else {
			channel->destnote = IT_NOTE_OFF;

			if (!channel->playing) {
				channel->playing = malloc(sizeof(*channel->playing));
				if (!channel->playing)
					return;
				// Adding the following seems to do the trick for the case where a piece starts with an instrument alone and then some notes alone.
				retrigger_xm_envelopes(channel->playing);
			}

			channel->playing->flags = 0;
			channel->playing->channel = channel;
			channel->playing->sample = &sigdata->sample[channel->sample-1];
			if (sigdata->flags & IT_USE_INSTRUMENTS)
				channel->playing->instrument = &sigdata->instrument[channel->instrument-1];
			else
				channel->playing->instrument = NULL;
			channel->playing->env_instrument = channel->playing->instrument;
			channel->playing->sampnum = channel->sample;
			channel->playing->instnum = channel->instrument;
			channel->playing->channel_volume = channel->channelvolume;
			channel->playing->note = channel->truenote;
			channel->playing->filter_cutoff = 127;
			channel->playing->filter_resonance = 0;
			channel->playing->true_filter_cutoff = 127 << 8;
			channel->playing->true_filter_resonance = 0;
			channel->playing->vibrato_speed = 0;
			channel->playing->vibrato_depth = 0;
			channel->playing->vibrato_n = 0;
			channel->playing->vibrato_time = 0;
			channel->playing->tremolo_speed = 0;
			channel->playing->tremolo_depth = 0;
			channel->playing->tremolo_time = 0;
			channel->playing->sample_vibrato_time = 0;
			channel->playing->sample_vibrato_depth = 0;
			channel->playing->slide = 0;
			it_reset_filter_state(&channel->playing->filter_state[0]); // Are these
			it_reset_filter_state(&channel->playing->filter_state[1]); // necessary?
			it_playing_reset_resamplers(channel->playing, 0);

			/** WARNING - is everything initialised? */
		}
	}

	if (!((entry->mask & IT_ENTRY_EFFECT) && entry->effect == IT_XM_KEY_OFF && entry->effectvalue == 0) &&
		(entry->mask & (IT_ENTRY_NOTE | IT_ENTRY_INSTRUMENT)) == (IT_ENTRY_NOTE | IT_ENTRY_INSTRUMENT))
	{
		if (channel->playing) retrigger_xm_envelopes(channel->playing);
		get_default_volpan(sigdata, channel);
	}

	if ((entry->mask & IT_ENTRY_VOLPAN) && ((entry->volpan>>4) == 0xF)) {
		/* Tone Portamento */
		unsigned char v = (entry->volpan & 15) << 4;
		if (v == 0)
			v = channel->lastG;
		channel->lastG = v;
		if (entry->mask & IT_ENTRY_NOTE)
			if (channel->sample)
				channel->destnote = channel->truenote;
		channel->toneporta = v << 4;
	} else if ((entry->mask & IT_ENTRY_EFFECT) &&
	           (entry->effect == IT_TONE_PORTAMENTO ||
	            entry->effect == IT_VOLSLIDE_TONEPORTA)) {
		unsigned char v;
		if (entry->effect == IT_TONE_PORTAMENTO)
			v = entry->effectvalue;
		else
			v = 0;
		if (v == 0)
			v = channel->lastG;
		channel->lastG = v;
		if (entry->mask & IT_ENTRY_NOTE)
			if (channel->sample)
				channel->destnote = channel->truenote;
		channel->toneporta = v << 4;
	}

	if (entry->mask & IT_ENTRY_VOLPAN) {
		int effect = entry->volpan >> 4;
		int value  = entry->volpan & 15;
		switch (effect) {
			case 0x6: /* Volume slide down */
				channel->xm_volslide = -value;
				break;
			case 0x7: /* Volume slide up */
				channel->xm_volslide = value;
				break;
			case 0x8: /* Fine volume slide down */
				channel->volume -= value;
				if (channel->volume > 64) channel->volume = 0;
				break;
			case 0x9: /* Fine volume slide up */
				channel->volume += value;
				if (channel->volume > 64) channel->volume = 64;
				break;
			case 0xA: /* Set vibrato speed */
				if (value)
					channel->lastHspeed = value;
				if (channel->playing)
					channel->playing->vibrato_speed = channel->lastHspeed;
				break;
			case 0xB: /* Vibrato */
				if (value)
					channel->lastHdepth = value << 2; /** WARNING: correct ? */
				if (channel->playing) {
					channel->playing->vibrato_depth = channel->lastHdepth;
					channel->playing->vibrato_speed = channel->lastHspeed;
					channel->playing->vibrato_n++;
				}
				break;
			case 0xC: /* Set panning */
				channel->truepan = 32 + value*(17*64);
				break;
			case 0xD: /* Pan slide left */
				/* -128 is a special case for emulating a 'feature' in FT2.
				 * As soon as effects are processed, it goes hard left.
				 */
				channel->panslide = value ? -value : -128;
				break;
			case 0xE: /* Pan slide Right */
				channel->panslide = value;
				break;
			case 0xF: /* Tone porta */
				break;
			default:  /* Volume */
				channel->volume = entry->volpan - 0x10;
				break;
		}
	}
}



/* This function assumes !IT_IS_END_ROW(entry). */
static int process_note_data(DUMB_IT_SIGRENDERER *sigrenderer, IT_ENTRY *entry, int ignore_cxx)
{
	DUMB_IT_SIGDATA *sigdata = sigrenderer->sigdata;

	if (sigdata->flags & IT_WAS_AN_XM)
		process_xm_note_data(sigrenderer, entry);
	else
		if (process_it_note_data(sigrenderer, entry)) return 0;

	return process_effects(sigrenderer, entry, ignore_cxx);
}



static int process_entry(DUMB_IT_SIGRENDERER *sigrenderer, IT_ENTRY *entry, int ignore_cxx)
{
	IT_CHANNEL *channel = &sigrenderer->channel[(int)entry->channel];

	if (entry->mask & IT_ENTRY_NOTE)
		channel->note = entry->note;

	if ((entry->mask & IT_ENTRY_EFFECT) && entry->effect == IT_S) {
		/* channel->lastS was set in update_pattern_variables(). */
		unsigned char effectvalue = channel->lastS;
		if (effectvalue >> 4 == IT_S_NOTE_DELAY) {
			channel->note_delay_count = effectvalue & 15;
			if (channel->note_delay_count == 0)
				channel->note_delay_count = 1;
			channel->note_delay_entry = entry;
			return 0;
		}
	}

	return process_note_data(sigrenderer, entry, ignore_cxx);
}



static void update_tick_counts(DUMB_IT_SIGRENDERER *sigrenderer)
{
	int i;

	for (i = 0; i < DUMB_IT_N_CHANNELS; i++) {
		IT_CHANNEL *channel = &sigrenderer->channel[i];

		if (channel->key_off_count) {
			channel->key_off_count--;
			if (channel->key_off_count == 0)
				xm_note_off(sigrenderer->sigdata, channel);
		} else if (channel->note_cut_count) {
			channel->note_cut_count--;
			if (channel->note_cut_count == 0) {
				if (sigrenderer->sigdata->flags & IT_WAS_AN_XM)
					channel->volume = 0;
				else if (channel->playing) {
					free(channel->playing);
					channel->playing = NULL;
				}
			}
		} else if (channel->note_delay_count) {
			channel->note_delay_count--;
			if (channel->note_delay_count == 0)
				process_note_data(sigrenderer, channel->note_delay_entry, 0);
					/* Don't bother checking the return value; if the note
					 * was delayed, there can't have been a speed=0.
					 */
		}
	}
}



static int envelope_get_y(IT_ENVELOPE *envelope, IT_PLAYING_ENVELOPE *pe)
{
#if 1
	(void)envelope; //TODO: remove the parameter
	return pe->value;
#else
	int ys, ye;
	int ts, te;
	int t;

	if (pe->next_node <= 0)
		return envelope->node_y[0] << IT_ENVELOPE_SHIFT;

	if (pe->next_node >= envelope->n_nodes)
		return envelope->node_y[envelope->n_nodes-1] << IT_ENVELOPE_SHIFT;

	ys = envelope->node_y[pe->next_node-1] << IT_ENVELOPE_SHIFT;
	ts = envelope->node_t[pe->next_node-1];
	te = envelope->node_t[pe->next_node];

	if (ts == te)
		return ys;

	ye = envelope->node_y[pe->next_node] << IT_ENVELOPE_SHIFT;

	t = pe->tick;

	return ys + (ye - ys) * (t - ts) / (te - ts);
#endif
}



#if 0
static int it_envelope_end(IT_PLAYING *playing, IT_ENVELOPE *envelope, IT_PLAYING_ENVELOPE *pe)
{
	if (pe->next_node >= envelope->n_nodes)
		return 1;

	if (pe->tick < envelope->node_t[pe->next_node]) return 0;

	if ((envelope->flags & IT_ENVELOPE_LOOP_ON) &&
	    envelope->loop_end >= pe->next_node &&
	    envelope->node_t[envelope->loop_end] <= pe->tick) return 0;

	if ((envelope->flags & IT_ENVELOPE_SUSTAIN_LOOP) &&
	    !(playing->flags & IT_PLAYING_SUSTAINOFF) &&
	    envelope->sus_loop_end >= pe->next_node &&
	    envelope->node_t[envelope->sus_loop_end] <= pe->tick) return 0;

	if (envelope->node_t[envelope->n_nodes-1] <= pe->tick) return 1;

	return 0;
}
#endif



/* Returns 1 when fading should be initiated for a volume envelope. */
static int update_it_envelope(IT_PLAYING *playing, IT_ENVELOPE *envelope, IT_PLAYING_ENVELOPE *pe)
{
	if (!(envelope->flags & IT_ENVELOPE_ON))
		return 0;

	ASSERT(envelope->n_nodes > 0);

	if (pe->next_node <= 0)
		pe->value = envelope->node_y[0] << IT_ENVELOPE_SHIFT;
	else if (pe->next_node >= envelope->n_nodes) {
		pe->value = envelope->node_y[envelope->n_nodes-1] << IT_ENVELOPE_SHIFT;
		return 1;
	} else {
		int ys = envelope->node_y[pe->next_node-1] << IT_ENVELOPE_SHIFT;
		int ts = envelope->node_t[pe->next_node-1];
		int te = envelope->node_t[pe->next_node];

		if (ts == te)
			pe->value = ys;
		else {
			int ye = envelope->node_y[pe->next_node] << IT_ENVELOPE_SHIFT;
			int t = pe->tick;

			pe->value = ys + (ye - ys) * (t - ts) / (te - ts);
		}
	}

	pe->tick++;
	while (pe->tick >= envelope->node_t[pe->next_node]) {
		pe->next_node++;
		if ((envelope->flags & IT_ENVELOPE_SUSTAIN_LOOP) && !(playing->flags & IT_PLAYING_SUSTAINOFF)) {
			if (pe->next_node > envelope->sus_loop_end) {
				pe->next_node = envelope->sus_loop_start;
				ASSERT(pe->next_node < envelope->n_nodes);
				pe->tick = envelope->node_t[envelope->sus_loop_start];
				return 0;
			}
		} else if (envelope->flags & IT_ENVELOPE_LOOP_ON) {
			if (pe->next_node > envelope->loop_end) {
				pe->next_node = envelope->loop_start;
				ASSERT(pe->next_node < envelope->n_nodes);
				pe->tick = envelope->node_t[envelope->loop_start];
				return 0;
			}
		}
		if (pe->next_node >= envelope->n_nodes)
			return 0;
	}
	return 0;
}



static void update_it_envelopes(IT_PLAYING *playing)
{
	IT_ENVELOPE *envelope = &playing->env_instrument->volume_envelope;
	IT_PLAYING_ENVELOPE *pe = &playing->volume_envelope;

	if (update_it_envelope(playing, envelope, pe)) {
		playing->flags |= IT_PLAYING_FADING;
		if (pe->value == 0)
			playing->flags |= IT_PLAYING_DEAD;
	}

	update_it_envelope(playing, &playing->env_instrument->pan_envelope, &playing->pan_envelope);
	update_it_envelope(playing, &playing->env_instrument->pitch_envelope, &playing->pitch_envelope);
}



static int xm_envelope_is_sustaining(IT_PLAYING *playing, IT_ENVELOPE *envelope, IT_PLAYING_ENVELOPE *pe)
{
	if ((envelope->flags & IT_ENVELOPE_SUSTAIN_LOOP) && !(playing->flags & IT_PLAYING_SUSTAINOFF))
		if (envelope->sus_loop_start < envelope->n_nodes)
			if (pe->tick == envelope->node_t[envelope->sus_loop_start])
				return 1;
	return 0;
}



static void update_xm_envelope(IT_PLAYING *playing, IT_ENVELOPE *envelope, IT_PLAYING_ENVELOPE *pe)
{
	if (!(envelope->flags & IT_ENVELOPE_ON))
		return;

	if (xm_envelope_is_sustaining(playing, envelope, pe))
		return;

	if (pe->tick >= envelope->node_t[envelope->n_nodes-1])
		return;

	pe->tick++;

	/* pe->next_node must be kept up to date for envelope_get_y(). */
	while (pe->tick > envelope->node_t[pe->next_node])
		pe->next_node++;

	if ((envelope->flags & IT_ENVELOPE_LOOP_ON) && envelope->loop_end < envelope->n_nodes) {
		if (pe->tick == envelope->node_t[envelope->loop_end]) {
			pe->next_node = MID(0, envelope->loop_start, envelope->n_nodes - 1);
			pe->tick = envelope->node_t[pe->next_node];
		}
	}

	xm_envelope_calculate_value(envelope, pe);
}



static void update_xm_envelopes(IT_PLAYING *playing)
{
	update_xm_envelope(playing, &playing->env_instrument->volume_envelope, &playing->volume_envelope);
	update_xm_envelope(playing, &playing->env_instrument->pan_envelope, &playing->pan_envelope);
}



static void update_fadeout(DUMB_IT_SIGDATA *sigdata, IT_PLAYING *playing)
{
	if (playing->flags & IT_PLAYING_FADING) {
		playing->fadeoutcount -= playing->env_instrument->fadeout;
		if (playing->fadeoutcount <= 0) {
			playing->fadeoutcount = 0;
			if (!(sigdata->flags & IT_WAS_AN_XM))
				playing->flags |= IT_PLAYING_DEAD;
		}
	}
}



static void process_playing(DUMB_IT_SIGDATA *sigdata, IT_PLAYING *playing)
{
	if (playing->instrument) {
		if (sigdata->flags & IT_WAS_AN_XM)
			update_xm_envelopes(playing);
		else
			update_it_envelopes(playing);
		update_fadeout(sigdata, playing);
	}

	//Calculate final volume if required
	//Calculate final pan if required

	if (sigdata->flags & IT_WAS_AN_XM) {
		/* 'depth' is used to store the tick number for XM files. */
		if (playing->sample_vibrato_depth < playing->sample->vibrato_rate)
			playing->sample_vibrato_depth++;
	} else {
		playing->sample_vibrato_depth += playing->sample->vibrato_rate;
		if (playing->sample_vibrato_depth > playing->sample->vibrato_depth << 8)
			playing->sample_vibrato_depth = playing->sample->vibrato_depth << 8;
	}

	playing->sample_vibrato_time += playing->sample->vibrato_speed;
}



static void process_all_playing(DUMB_IT_SIGRENDERER *sigrenderer)
{
	DUMB_IT_SIGDATA *sigdata = sigrenderer->sigdata;
	int i;

	for (i = 0; i < DUMB_IT_N_CHANNELS; i++) {
		IT_CHANNEL *channel = &sigrenderer->channel[i];
		IT_PLAYING *playing = channel->playing;

		if (playing) {
			int vibrato_shift = it_sine[playing->vibrato_time];
			vibrato_shift *= playing->vibrato_n;
			vibrato_shift *= playing->vibrato_depth;
			vibrato_shift >>= 4;

			if (sigdata->flags & IT_OLD_EFFECTS)
				vibrato_shift = -vibrato_shift;

			playing->volume = channel->volume;
			playing->pan = channel->truepan;

			if (sigdata->flags & IT_LINEAR_SLIDES) {
				int currpitch = ((playing->note - 60) << 8) + playing->slide
				                                            + vibrato_shift;

				/* We add a feature here, which is that of keeping the pitch
				 * within range. Otherwise it crashes. Trust me. It happened.
				 * The limit 32768 gives almost 11 octaves either way.
				 */
				if (currpitch < -32768)
					currpitch = -32768;
				else if (currpitch > 32767)
					currpitch = 32767;

				playing->delta = (float)pow(DUMB_PITCH_BASE, currpitch);
				playing->delta *= playing->sample->C5_speed / 65536.0f;
			} else {
				int slide = playing->slide + vibrato_shift;

				playing->delta = (float)pow(DUMB_SEMITONE_BASE, 60 - playing->note);
				/* playing->delta is 1.0 for C-5, 0.5 for C-6, etc. */

				playing->delta *= 1.0f / playing->sample->C5_speed;

				playing->delta -= slide / AMIGA_DIVISOR;

				if (playing->delta < (1.0f / 65536.0f) / 32768.0f) {
					// Should XM notes die if Amiga slides go out of range?
					playing->flags |= IT_PLAYING_DEAD;
					continue;
				}

				playing->delta = (1.0f / 65536.0f) / playing->delta;
			}

			playing->delta *= (float)pow(DUMB_SEMITONE_BASE, channel->arpeggio >> 8);

			playing->filter_cutoff = channel->filter_cutoff;
			playing->filter_resonance = channel->filter_resonance;
		}
	}

	for (i = 0; i < DUMB_IT_N_CHANNELS; i++) {
		if (sigrenderer->channel[i].playing) {
			process_playing(sigdata, sigrenderer->channel[i].playing);
			if (!(sigdata->flags & IT_WAS_AN_XM)) {
				//if ((sigrenderer->channel[i].playing->flags & (IT_PLAYING_BACKGROUND | IT_PLAYING_DEAD)) == (IT_PLAYING_BACKGROUND | IT_PLAYING_DEAD)) {
				// This change was made so Gxx would work correctly when a note faded out or whatever. Let's hope nothing else was broken by it.
				if (sigrenderer->channel[i].playing->flags & IT_PLAYING_DEAD) {
					free(sigrenderer->channel[i].playing);
					sigrenderer->channel[i].playing = NULL;
				}
			}
		}
	}

	for (i = 0; i < DUMB_IT_N_NNA_CHANNELS; i++) {
		if (sigrenderer->playing[i]) {
			process_playing(sigdata, sigrenderer->playing[i]);
			if (sigrenderer->playing[i]->flags & IT_PLAYING_DEAD) {
				free(sigrenderer->playing[i]);
				sigrenderer->playing[i] = NULL;
			}
		}
	}
}



static int process_tick(DUMB_IT_SIGRENDERER *sigrenderer)
{
	DUMB_IT_SIGDATA *sigdata = sigrenderer->sigdata;

	// Set note vol/freq to vol/freq set for each channel

	if (sigrenderer->speed && --sigrenderer->tick == 0) {
		reset_tick_counts(sigrenderer);
		sigrenderer->tick = sigrenderer->speed;
		sigrenderer->rowcount--;
		if (sigrenderer->rowcount == 0) {
			sigrenderer->rowcount = 1;

			sigrenderer->processrow++;

			if (sigrenderer->processrow >= sigrenderer->n_rows) {
				IT_PATTERN *pattern;
				int n;
				int processorder = sigrenderer->processorder;

				if (sigrenderer->processrow == 0xFFFE + 1) { /* It was incremented above! */
					sigrenderer->processrow = sigrenderer->breakrow;
					sigrenderer->breakrow = 0;
					for (n = 0; n < DUMB_IT_N_CHANNELS; n++) sigrenderer->channel[n].pat_loop_end_row = 0;
				} else
					sigrenderer->processrow = sigrenderer->breakrow;

				if (sigrenderer->processorder == 0xFFFF)
					sigrenderer->processorder = sigrenderer->order - 1;

				for (;;) {
					sigrenderer->processorder++;

					if (sigrenderer->processorder >= sigdata->n_orders) {
						sigrenderer->processorder = sigdata->restart_position;
						if (sigrenderer->processorder >= sigdata->n_orders) {
							/* Restarting beyond end. We'll loop for now. */
							sigrenderer->processorder = -1;
							continue;
						}
					}

					n = sigdata->order[sigrenderer->processorder];

					if (n < sigdata->n_patterns)
						break;

#ifdef INVALID_ORDERS_END_SONG
					if (n != IT_ORDER_SKIP)
						sigrenderer->processorder = -1;
#else
					if (n == IT_ORDER_END)
						sigrenderer->processorder = -1;
#endif
				}

				pattern = &sigdata->pattern[n];

				n = sigrenderer->n_rows;
				sigrenderer->n_rows = pattern->n_rows;

				if (sigrenderer->processrow >= sigrenderer->n_rows)
					sigrenderer->processrow = 0;

/** WARNING - everything pertaining to a new pattern initialised? */

				sigrenderer->entry = sigrenderer->entry_start = pattern->entry;
				sigrenderer->entry_end = sigrenderer->entry + pattern->n_entries;

				/* If n_rows was 0, we're only just starting. Don't do anything weird here. */
				if (n && (processorder == 0xFFFF ? sigrenderer->order > sigrenderer->processorder : sigrenderer->order >= sigrenderer->processorder)) {
					if (sigrenderer->callbacks->loop) {
						if ((*sigrenderer->callbacks->loop)(sigrenderer->callbacks->loop_data))
							return 1;
						if (sigrenderer->speed == 0)
							goto speed0; /* I love goto */
					}
				}
				sigrenderer->order = sigrenderer->processorder;

				n = sigrenderer->processrow;
				while (n) {
					while (sigrenderer->entry < sigrenderer->entry_end) {
						if (IT_IS_END_ROW(sigrenderer->entry)) {
							sigrenderer->entry++;
							break;
						}
						sigrenderer->entry++;
					}
					n--;
				}
				sigrenderer->row = sigrenderer->processrow;
			} else {
				if (sigrenderer->entry) {
					while (sigrenderer->entry < sigrenderer->entry_end) {
						if (IT_IS_END_ROW(sigrenderer->entry)) {
							sigrenderer->entry++;
							break;
						}
						sigrenderer->entry++;
					}
					sigrenderer->row++;
				} else {
					sigrenderer->entry = sigrenderer->entry_start;
					sigrenderer->row = 0;
				}
			}

			reset_effects(sigrenderer);

			{
				IT_ENTRY *entry = sigrenderer->entry;
				int ignore_cxx = 0;

				while (entry < sigrenderer->entry_end && !IT_IS_END_ROW(entry))
					ignore_cxx |= update_pattern_variables(sigrenderer, entry++);

				entry = sigrenderer->entry;

				while (entry < sigrenderer->entry_end && !IT_IS_END_ROW(entry))
					if (process_entry(sigrenderer, entry++, sigdata->flags & IT_WAS_AN_XM ? 0 : ignore_cxx))
						return 1;
			}

			if (!(sigdata->flags & IT_OLD_EFFECTS))
				update_smooth_effects(sigrenderer);
		} else {
			{
				IT_ENTRY *entry = sigrenderer->entry;

				while (entry < sigrenderer->entry_end && !IT_IS_END_ROW(entry))
					process_effects(sigrenderer, entry++, 0);
						/* Don't bother checking the return value; if there
						 * was a pattern delay, there can't be a speed=0.
						 */
			}

			update_effects(sigrenderer);
		}
	} else {
		speed0:
		update_effects(sigrenderer);
		update_tick_counts(sigrenderer);
	}

	process_all_playing(sigrenderer);

	{
		LONG_LONG t = sigrenderer->sub_time_left + ((LONG_LONG)TICK_TIME_DIVIDEND << 16) / sigrenderer->tempo;
		sigrenderer->time_left += (int)(t >> 16);
		sigrenderer->sub_time_left = (int)t & 65535;
	}

	return 0;
}



int dumb_it_max_to_mix = 64;



static float calculate_volume(DUMB_IT_SIGRENDERER *sigrenderer, IT_PLAYING *playing, float volume)
{
	if (volume != 0) {
		int vol;

		if (playing->channel->flags & IT_CHANNEL_MUTED)
			return 0;

		if ((playing->channel->tremor_time & 192) == 128)
			return 0;

		vol = it_sine[playing->tremolo_time];
		vol *= playing->tremolo_depth;

		vol = (playing->volume << 5) + vol;

		if (vol <= 0)
			return 0;

		if (vol > 64 << 5)
			vol = 64 << 5;

		volume *= vol; /* 64 << 5 */
		volume *= playing->sample->global_volume; /* 64 */
		volume *= playing->channel_volume; /* 64 */
		volume *= sigrenderer->globalvolume; /* 128 */
		volume *= sigrenderer->sigdata->mixing_volume; /* 128 */
		volume *= 1.0f / ((64 << 5) * 64.0f * 64.0f * 128.0f * 128.0f);

		if (volume && playing->instrument) {
			if (playing->env_instrument->volume_envelope.flags & IT_ENVELOPE_ON) {
				volume *= envelope_get_y(&playing->env_instrument->volume_envelope, &playing->volume_envelope);
				volume *= 1.0f / (64 << IT_ENVELOPE_SHIFT);
			}
			volume *= playing->instrument->global_volume; /* 128 */
			volume *= playing->fadeoutcount; /* 1024 */
			volume *= 1.0f / (128.0f * 1024.0f);
		}
	}

	return volume;
}



static int apply_pan_envelope(IT_PLAYING *playing)
{
	int pan = playing->pan;
	if (pan <= 64 << IT_ENVELOPE_SHIFT && playing->env_instrument && (playing->env_instrument->pan_envelope.flags & IT_ENVELOPE_ON)) {
		int p = envelope_get_y(&playing->env_instrument->pan_envelope, &playing->pan_envelope);
		if (pan > 32 << IT_ENVELOPE_SHIFT)
			p *= (64 << IT_ENVELOPE_SHIFT) - pan;
		else
			p *= pan;
		pan += p >> (5 + IT_ENVELOPE_SHIFT);
	}
	return pan;
}



/* Note: if a click remover is provided, and store_end_sample is set, then
 * the end point will be computed twice. This situation should not arise.
 */
static long render_playing(DUMB_IT_SIGRENDERER *sigrenderer, IT_PLAYING *playing, float volume, float delta, long pos, long size, sample_t **samples, int store_end_sample, int *left_to_mix)
{
	int bits;

	int pan;
	float span; /* separated pan, range -1 to 1; garbage for surround */

	long size_rendered;

	if (playing->flags & IT_PLAYING_DEAD)
		return 0;

	if (*left_to_mix <= 0)
		volume = 0;

	bits = playing->sample->flags & IT_SAMPLE_16BIT ? 16 : 8;

	pan = apply_pan_envelope(playing);
	span = (pan - (32<<8)) * sigrenderer->sigdata->pan_separation * (1.0f / ((32<<8) * 128));

	if (volume == 0) {
		if (playing->sample->flags & IT_SAMPLE_STEREO)
			size_rendered = dumb_resample_n_2_1(bits, &playing->resampler, NULL, size, 0, 0, delta);
		else
			size_rendered = dumb_resample_n_1_1(bits, &playing->resampler, NULL, size, 0, delta);
	} else {
		if (sigrenderer->n_channels == 2) {
			float lvol, rvol;
			lvol = volume;
			if (!IT_IS_SURROUND_SHIFTED(pan)) lvol *= 1.0f - span;
			rvol = -lvol;
			if (!IT_IS_SURROUND_SHIFTED(pan)) rvol += 2.0f * volume;
			if (playing->sample->flags & IT_SAMPLE_STEREO) {
				if (sigrenderer->click_remover) {
					sample_t click[2];
					dumb_resample_get_current_sample_n_2_2(bits, &playing->resampler, lvol, rvol, click);
					dumb_record_click(sigrenderer->click_remover[0], pos, click[0]);
					dumb_record_click(sigrenderer->click_remover[1], pos, click[1]);
				}
				size_rendered = dumb_resample_n_2_2(bits, &playing->resampler, samples[0] + pos*2, size, lvol, rvol, delta);
				if (store_end_sample) {
					sample_t click[2];
					dumb_resample_get_current_sample_n_2_2(bits, &playing->resampler, lvol, rvol, click);
					samples[0][(pos + size_rendered) * 2] = click[0];
					samples[0][(pos + size_rendered) * 2 + 1] = click[1];
				}
				if (sigrenderer->click_remover) {
					sample_t click[2];
					dumb_resample_get_current_sample_n_2_2(bits, &playing->resampler, lvol, rvol, click);
					dumb_record_click(sigrenderer->click_remover[0], pos + size_rendered, -click[0]);
					dumb_record_click(sigrenderer->click_remover[1], pos + size_rendered, -click[1]);
				}
			} else {
				if (sigrenderer->click_remover) {
					sample_t click[2];
					dumb_resample_get_current_sample_n_1_2(bits, &playing->resampler, lvol, rvol, click);
					dumb_record_click(sigrenderer->click_remover[0], pos, click[0]);
					dumb_record_click(sigrenderer->click_remover[1], pos, click[1]);
				}
				size_rendered = dumb_resample_n_1_2(bits, &playing->resampler, samples[0] + pos*2, size, lvol, rvol, delta);
				if (store_end_sample) {
					sample_t click[2];
					dumb_resample_get_current_sample_n_1_2(bits, &playing->resampler, lvol, rvol, click);
					samples[0][(pos + size_rendered) * 2] = click[0];
					samples[0][(pos + size_rendered) * 2 + 1] = click[1];
				}
				if (sigrenderer->click_remover) {
					sample_t click[2];
					dumb_resample_get_current_sample_n_1_2(bits, &playing->resampler, lvol, rvol, click);
					dumb_record_click(sigrenderer->click_remover[0], pos + size_rendered, -click[0]);
					dumb_record_click(sigrenderer->click_remover[1], pos + size_rendered, -click[1]);
				}
			}
		} else {
			if (playing->sample->flags & IT_SAMPLE_STEREO) {
				float lvol, rvol;
				lvol = 0.5f * volume;
				if (!IT_IS_SURROUND_SHIFTED(pan)) lvol *= 1.0f - span;
				rvol = lvol;
				if (!IT_IS_SURROUND_SHIFTED(pan)) rvol = volume - rvol;
				if (sigrenderer->click_remover) {
					sample_t click;
					dumb_resample_get_current_sample_n_2_1(bits, &playing->resampler, lvol, rvol, &click);
					dumb_record_click(sigrenderer->click_remover[0], pos, click);
				}
				size_rendered = dumb_resample_n_2_1(bits, &playing->resampler, samples[0] + pos, size, lvol, rvol, delta);
				if (store_end_sample)
					dumb_resample_get_current_sample_n_2_1(bits, &playing->resampler, lvol, rvol, &samples[0][pos + size_rendered]);
				if (sigrenderer->click_remover) {
					sample_t click;
					dumb_resample_get_current_sample_n_2_1(bits, &playing->resampler, lvol, rvol, &click);
					dumb_record_click(sigrenderer->click_remover[0], pos + size_rendered, -click);
				}
			} else {
				if (sigrenderer->click_remover) {
					sample_t click;
					dumb_resample_get_current_sample_n_1_1(bits, &playing->resampler, volume, &click);
					dumb_record_click(sigrenderer->click_remover[0], pos, click);
				}
				size_rendered = dumb_resample_n_1_1(bits, &playing->resampler, samples[0] + pos, size, volume, delta);
				if (store_end_sample)
					dumb_resample_get_current_sample_n_1_1(bits, &playing->resampler, volume, &samples[0][pos + size_rendered]);
				if (sigrenderer->click_remover) {
					sample_t click;
					dumb_resample_get_current_sample_n_1_1(bits, &playing->resampler, volume, &click);
					dumb_record_click(sigrenderer->click_remover[0], pos + size_rendered, -click);
				}
			}
		}
		(*left_to_mix)--;
	}

	if (playing->resampler.dir == 0)
		playing->flags |= IT_PLAYING_DEAD;

	return size_rendered;
}



typedef struct IT_TO_MIX
{
	IT_PLAYING *playing;
	float volume;
}
IT_TO_MIX;



static int it_to_mix_compare(const void *e1, const void *e2)
{
	if (((const IT_TO_MIX *)e1)->volume > ((const IT_TO_MIX *)e2)->volume)
		return -1;

	if (((const IT_TO_MIX *)e1)->volume < ((const IT_TO_MIX *)e2)->volume)
		return 1;

	return 0;
}



static void apply_pitch_modifications(DUMB_IT_SIGDATA *sigdata, IT_PLAYING *playing, float *delta, int *cutoff)
{
	{
		int sample_vibrato_shift = it_sine[playing->sample_vibrato_time];

		if (sigdata->flags & IT_WAS_AN_XM) {
			int depth = playing->sample->vibrato_depth; /* True depth */
			if (playing->sample->vibrato_rate) {
				depth *= playing->sample_vibrato_depth; /* Tick number */
				depth /= playing->sample->vibrato_rate; /* XM sweep */
			}
			sample_vibrato_shift *= depth;
		} else
			sample_vibrato_shift *= playing->sample_vibrato_depth >> 8;

		sample_vibrato_shift >>= 4;

		*delta *= (float)pow(DUMB_PITCH_BASE, sample_vibrato_shift);
	}

	if (playing->env_instrument &&
		(playing->env_instrument->pitch_envelope.flags & IT_ENVELOPE_ON))
	{
		int p = envelope_get_y(&playing->env_instrument->pitch_envelope, &playing->pitch_envelope);
		if (playing->env_instrument->pitch_envelope.flags & IT_ENVELOPE_PITCH_IS_FILTER)
			*cutoff = (*cutoff * (p+(32<<IT_ENVELOPE_SHIFT))) >> (6 + IT_ENVELOPE_SHIFT);
		else
			*delta *= (float)pow(DUMB_PITCH_BASE, p >> (IT_ENVELOPE_SHIFT - 7));
	}
}



static void render(DUMB_IT_SIGRENDERER *sigrenderer, float volume, float delta, long pos, long size, sample_t **samples)
{
	int i;

	int n_to_mix = 0;
	IT_TO_MIX to_mix[DUMB_IT_TOTAL_CHANNELS];
	int left_to_mix = dumb_it_max_to_mix;

	sample_t **samples_to_filter = NULL;

	for (i = 0; i < DUMB_IT_N_CHANNELS; i++) {
		if (sigrenderer->channel[i].playing && !(sigrenderer->channel[i].playing->flags & IT_PLAYING_DEAD)) {
			to_mix[n_to_mix].playing = sigrenderer->channel[i].playing;
			to_mix[n_to_mix].volume = volume == 0 ? 0 : calculate_volume(sigrenderer, sigrenderer->channel[i].playing, volume);
			n_to_mix++;
		}
	}

	for (i = 0; i < DUMB_IT_N_NNA_CHANNELS; i++) {
		if (sigrenderer->playing[i]) { /* Won't be dead; it would have been freed. */
			to_mix[n_to_mix].playing = sigrenderer->playing[i];
			to_mix[n_to_mix].volume = volume == 0 ? 0 : calculate_volume(sigrenderer, sigrenderer->playing[i], volume);
			n_to_mix++;
		}
	}

	if (volume != 0)
		qsort(to_mix, n_to_mix, sizeof(IT_TO_MIX), &it_to_mix_compare);

	for (i = 0; i < n_to_mix; i++) {
		IT_PLAYING *playing = to_mix[i].playing;
		float note_delta = delta * playing->delta;
		int cutoff = playing->filter_cutoff << IT_ENVELOPE_SHIFT;

		apply_pitch_modifications(sigrenderer->sigdata, playing, &note_delta, &cutoff);

		if (cutoff != 127 << IT_ENVELOPE_SHIFT || playing->filter_resonance != 0) {
			playing->true_filter_cutoff = cutoff;
			playing->true_filter_resonance = playing->filter_resonance;
		}

		if (to_mix[i].volume && (playing->true_filter_cutoff != 127 << IT_ENVELOPE_SHIFT || playing->true_filter_resonance != 0)) {
			if (!samples_to_filter) {
				samples_to_filter = allocate_sample_buffer(sigrenderer->n_channels, size + 1);
				if (!samples_to_filter) {
					render_playing(sigrenderer, playing, 0, note_delta, pos, size, NULL, 0, &left_to_mix);
					continue;
				}
			}
			{
				long size_rendered;
				DUMB_CLICK_REMOVER **cr = sigrenderer->click_remover;
				dumb_silence(samples_to_filter[0], sigrenderer->n_channels * (size + 1));
				sigrenderer->click_remover = NULL;
				size_rendered = render_playing(sigrenderer, playing, to_mix[i].volume, note_delta, 0, size, samples_to_filter, 1, &left_to_mix);
				sigrenderer->click_remover = cr;
				if (sigrenderer->n_channels == 2) {
					it_filter(cr ? cr[0] : NULL, &playing->filter_state[0], samples[0], pos, samples_to_filter[0], size_rendered,
						2, (int)(65536.0f/delta), playing->true_filter_cutoff, playing->true_filter_resonance);
					it_filter(cr ? cr[1] : NULL, &playing->filter_state[1], samples[0]+1, pos, samples_to_filter[0]+1, size_rendered,
						2, (int)(65536.0f/delta), playing->true_filter_cutoff, playing->true_filter_resonance);
				} else {
					it_filter(cr ? cr[0] : NULL, &playing->filter_state[0], samples[0], pos, samples_to_filter[0], size_rendered,
						1, (int)(65536.0f/delta), playing->true_filter_cutoff, playing->true_filter_resonance);
				}
				// FIXME: filtering is not prevented by low left_to_mix!
				// FIXME: change 'warning' to 'FIXME' everywhere
			}
		} else {
			it_reset_filter_state(&playing->filter_state[0]);
			it_reset_filter_state(&playing->filter_state[1]);
			render_playing(sigrenderer, playing, to_mix[i].volume, note_delta, pos, size, samples, 0, &left_to_mix);
		}
	}

	destroy_sample_buffer(samples_to_filter);

	for (i = 0; i < DUMB_IT_N_CHANNELS; i++) {
		if (sigrenderer->channel[i].playing) {
			//if ((sigrenderer->channel[i].playing->flags & (IT_PLAYING_BACKGROUND | IT_PLAYING_DEAD)) == (IT_PLAYING_BACKGROUND | IT_PLAYING_DEAD)) {
			// This change was made so Gxx would work correctly when a note faded out or whatever. Let's hope nothing else was broken by it.
			if (sigrenderer->channel[i].playing->flags & IT_PLAYING_DEAD) {
				free(sigrenderer->channel[i].playing);
				sigrenderer->channel[i].playing = NULL;
			}
		}
	}

	for (i = 0; i < DUMB_IT_N_NNA_CHANNELS; i++) {
		if (sigrenderer->playing[i]) {
			if (sigrenderer->playing[i]->flags & IT_PLAYING_DEAD) {
				free(sigrenderer->playing[i]);
				sigrenderer->playing[i] = NULL;
			}
		}
	}
}



static DUMB_IT_SIGRENDERER *init_sigrenderer(DUMB_IT_SIGDATA *sigdata, int n_channels, int startorder, IT_CALLBACKS *callbacks, DUMB_CLICK_REMOVER **cr)
{
	DUMB_IT_SIGRENDERER *sigrenderer;
	int i;

	if (startorder > sigdata->n_orders) {
		free(callbacks);
		dumb_destroy_click_remover_array(n_channels, cr);
		return NULL;
	}

	sigrenderer = malloc(sizeof(*sigrenderer));
	if (!sigrenderer) {
		free(callbacks);
		dumb_destroy_click_remover_array(n_channels, cr);
		return NULL;
	}

	sigrenderer->callbacks = callbacks;
	sigrenderer->click_remover = cr;

	sigrenderer->sigdata = sigdata;
	sigrenderer->n_channels = n_channels;
	sigrenderer->globalvolume = sigdata->global_volume;
	sigrenderer->tempo = sigdata->tempo;

	for (i = 0; i < DUMB_IT_N_CHANNELS; i++) {
		IT_CHANNEL *channel = &sigrenderer->channel[i];
#if IT_CHANNEL_MUTED != 1
#error this is wrong
#endif
		channel->flags = sigdata->channel_pan[i] >> 7;
		channel->volume = (sigdata->flags & IT_WAS_AN_XM) ? 0 : 64;
		channel->pan = sigdata->channel_pan[i] & 0x7F;
		channel->truepan = channel->pan << IT_ENVELOPE_SHIFT;
		channel->channelvolume = sigdata->channel_volume[i];
		channel->instrument = 0;
		channel->note = 0;
		channel->SFmacro = 0;
		channel->filter_cutoff = 127;
		channel->filter_resonance = 0;
		channel->xm_retrig = 0;
		channel->retrig_tick = 0;
		channel->tremor_time = 0;
		channel->midi_state = 0;
		channel->lastvolslide = 0;
		channel->lastDKL = 0;
		channel->lastEF = 0;
		channel->lastG = 0;
		channel->lastHspeed = 0;
		channel->lastHdepth = 0;
		channel->lastRspeed = 0;
		channel->lastRdepth = 0;
		channel->lastI = 0;
		channel->lastJ = 0;
		channel->lastN = 0;
		channel->lastO = 0;
		channel->high_offset = 0;
		channel->lastP = 0;
		channel->lastQ = 0;
		channel->lastS = 0;
		channel->pat_loop_row = 0;
		channel->pat_loop_count = 0;
		channel->pat_loop_end_row = 0;
		channel->lastW = 0;
		channel->xm_lastE1 = 0;
		channel->xm_lastE2 = 0;
		channel->xm_lastEA = 0;
		channel->xm_lastEB = 0;
		channel->xm_lastX1 = 0;
		channel->xm_lastX2 = 0;
		channel->playing = NULL;
	}

	for (i = 0; i < DUMB_IT_N_NNA_CHANNELS; i++)
		sigrenderer->playing[i] = NULL;

	sigrenderer->speed = sigdata->speed;

	sigrenderer->processrow = 0xFFFE;
	sigrenderer->n_rows = 0;
	sigrenderer->breakrow = 0;
	sigrenderer->pat_loop_row = -1;
	sigrenderer->rowcount = 1;
	sigrenderer->order = startorder;
	sigrenderer->row = 0;
	sigrenderer->processorder = startorder - 1;
	sigrenderer->tick = 1;

	{
		int order;
		for (order = 0; order < sigdata->n_orders; order++) {
			int n = sigdata->order[order];
			if (n < sigdata->n_patterns) goto found_valid_order;
#ifdef INVALID_ORDERS_END_SONG
			if (n != IT_ORDER_SKIP)
#else
			if (n == IT_ORDER_END)
#endif
				break;
		}
		/* If we get here, there were no valid orders in the song. */
		_dumb_it_end_sigrenderer(sigrenderer);
		return NULL;
	}
	found_valid_order:

	sigrenderer->time_left = 0;
	sigrenderer->sub_time_left = 0;

	return sigrenderer;
}



void dumb_it_set_loop_callback(DUMB_IT_SIGRENDERER *sigrenderer, int (*callback)(void *data), void *data)
{
	if (sigrenderer) {
		sigrenderer->callbacks->loop = callback;
		sigrenderer->callbacks->loop_data = data;
	}
}



void dumb_it_set_xm_speed_zero_callback(DUMB_IT_SIGRENDERER *sigrenderer, int (*callback)(void *data), void *data)
{
	if (sigrenderer) {
		sigrenderer->callbacks->xm_speed_zero = callback;
		sigrenderer->callbacks->xm_speed_zero_data = data;
	}
}



void dumb_it_set_midi_callback(DUMB_IT_SIGRENDERER *sigrenderer, int (*callback)(void *data, int channel, unsigned char midi_byte), void *data)
{
	if (sigrenderer) {
		sigrenderer->callbacks->midi = callback;
		sigrenderer->callbacks->midi_data = data;
	}
}



static IT_CALLBACKS *create_callbacks(void)
{
	IT_CALLBACKS *callbacks = malloc(sizeof(*callbacks));
	if (!callbacks) return NULL;
	callbacks->loop = NULL;
	callbacks->xm_speed_zero = NULL;
	callbacks->midi = NULL;
	return callbacks;
}



static DUMB_IT_SIGRENDERER *dumb_it_init_sigrenderer(DUMB_IT_SIGDATA *sigdata, int n_channels, int startorder)
{
	IT_CALLBACKS *callbacks;

	if (!sigdata) return NULL;

	callbacks = create_callbacks();
	if (!callbacks) return NULL;

	return init_sigrenderer(sigdata, n_channels, startorder, callbacks,
		dumb_create_click_remover_array(n_channels));
}



DUH_SIGRENDERER *dumb_it_start_at_order(DUH *duh, int n_channels, int startorder)
{
	DUMB_IT_SIGDATA *itsd = duh_get_it_sigdata(duh);
	DUMB_IT_SIGRENDERER *itsr = dumb_it_init_sigrenderer(itsd, n_channels, startorder);
	return duh_encapsulate_it_sigrenderer(itsr, n_channels, 0);
}



static sigrenderer_t *it_start_sigrenderer(DUH *duh, sigdata_t *vsigdata, int n_channels, long pos)
{
	DUMB_IT_SIGDATA *sigdata = vsigdata;
	DUMB_IT_SIGRENDERER *sigrenderer;

	(void)duh;

	{
		IT_CALLBACKS *callbacks = create_callbacks();
		if (!callbacks) return NULL;

		if (sigdata->checkpoint) {
			IT_CHECKPOINT *checkpoint = sigdata->checkpoint;
			while (checkpoint->next && checkpoint->next->time < pos)
				checkpoint = checkpoint->next;
			sigrenderer = dup_sigrenderer(checkpoint->sigrenderer, n_channels, callbacks);
			if (!sigrenderer) return NULL;
			sigrenderer->click_remover = dumb_create_click_remover_array(n_channels);
			pos -= checkpoint->time;
		} else {
			sigrenderer = init_sigrenderer(sigdata, n_channels, 0, callbacks,
				dumb_create_click_remover_array(n_channels));
			if (!sigrenderer) return NULL;
		}
	}

	while (pos >= sigrenderer->time_left) {
		render(sigrenderer, 0, 1.0f, 0, sigrenderer->time_left, NULL);

		pos -= sigrenderer->time_left;
		sigrenderer->time_left = 0;

		if (process_tick(sigrenderer)) {
			_dumb_it_end_sigrenderer(sigrenderer);
			return NULL;
		}
	}

	render(sigrenderer, 0, 1.0f, 0, pos, NULL);
	sigrenderer->time_left -= pos;

	return sigrenderer;
}



static long it_sigrenderer_get_samples(
	sigrenderer_t *vsigrenderer,
	float volume, float delta,
	long size, sample_t **samples
)
{
	DUMB_IT_SIGRENDERER *sigrenderer = vsigrenderer;
	long pos;
	int dt;
	long todo;
	LONG_LONG t;

	if (sigrenderer->order < 0) return 0; // problematic

	pos = 0;
	dt = (int)(delta * 65536.0f + 0.5f);

	/* When samples is finally used in render_playing(), it won't be used if
	 * volume is 0.
	 */
	if (!samples) volume = 0;

	for (;;) {
		todo = (long)((((LONG_LONG)sigrenderer->time_left << 16) | sigrenderer->sub_time_left) / dt);

		if (todo >= size)
			break;

		render(sigrenderer, volume, delta, pos, todo, samples);

		pos += todo;
		size -= todo;

		t = sigrenderer->sub_time_left - (LONG_LONG)todo * dt;
		sigrenderer->sub_time_left = (long)t & 65535;
		sigrenderer->time_left += (long)(t >> 16);

		if (process_tick(sigrenderer)) {
			sigrenderer->order = -1;
			sigrenderer->row = -1;
			return pos;
		}
	}

	render(sigrenderer, volume, delta, pos, size, samples);

	pos += size;

	t = sigrenderer->sub_time_left - (LONG_LONG)size * dt;
	sigrenderer->sub_time_left = (long)t & 65535;
	sigrenderer->time_left += (long)(t >> 16);

	if (samples)
		dumb_remove_clicks_array(sigrenderer->n_channels, sigrenderer->click_remover, samples, pos, 512.0f / delta);

	return pos;
}



static void it_sigrenderer_get_current_sample(sigrenderer_t *vsigrenderer, float volume, sample_t *samples)
{
	DUMB_IT_SIGRENDERER *sigrenderer = vsigrenderer;
	(void)volume; // for consideration: in any of these such functions, is 'volume' going to be required?
	dumb_click_remover_get_offset_array(sigrenderer->n_channels, sigrenderer->click_remover, samples);
}



void _dumb_it_end_sigrenderer(sigrenderer_t *vsigrenderer)
{
	DUMB_IT_SIGRENDERER *sigrenderer = vsigrenderer;

	int i;

	if (sigrenderer) {
		for (i = 0; i < DUMB_IT_N_CHANNELS; i++)
			if (sigrenderer->channel[i].playing)
				free(sigrenderer->channel[i].playing);

		for (i = 0; i < DUMB_IT_N_NNA_CHANNELS; i++)
			if (sigrenderer->playing[i])
				free(sigrenderer->playing[i]);

		dumb_destroy_click_remover_array(sigrenderer->n_channels, sigrenderer->click_remover);

		if (sigrenderer->callbacks)
			free(sigrenderer->callbacks);

		free(vsigrenderer);
	}
}



DUH_SIGTYPE_DESC _dumb_sigtype_it = {
	SIGTYPE_IT,
	NULL,
	&it_start_sigrenderer,
	NULL,
	&it_sigrenderer_get_samples,
	&it_sigrenderer_get_current_sample,
	&_dumb_it_end_sigrenderer,
	&_dumb_it_unload_sigdata
};



DUH_SIGRENDERER *duh_encapsulate_it_sigrenderer(DUMB_IT_SIGRENDERER *it_sigrenderer, int n_channels, long pos)
{
	return duh_encapsulate_raw_sigrenderer(it_sigrenderer, &_dumb_sigtype_it, n_channels, pos);
}



DUMB_IT_SIGRENDERER *duh_get_it_sigrenderer(DUH_SIGRENDERER *sigrenderer)
{
	return duh_get_raw_sigrenderer(sigrenderer, SIGTYPE_IT);
}



/* Values of 64 or more will access NNA channels here. */
void dumb_it_sr_get_channel_state(DUMB_IT_SIGRENDERER *sr, int channel, DUMB_IT_CHANNEL_STATE *state)
{
	IT_PLAYING *playing;
	int t; /* temporary var for holding accurate pan and filter cutoff */
	float delta;
	ASSERT(channel < DUMB_IT_TOTAL_CHANNELS);
	if (!sr) { state->sample = 0; return; }
	if (channel >= DUMB_IT_N_CHANNELS) {
		playing = sr->playing[channel - DUMB_IT_N_CHANNELS];
		if (!playing) { state->sample = 0; return; }
	} else {
		playing = sr->channel[channel].playing;
		if (!playing) { state->sample = 0; return; }
	}

	if (playing->flags & IT_PLAYING_DEAD) { state->sample = 0; return; }

	state->channel = playing->channel - sr->channel;
	state->sample = playing->sampnum;
	state->volume = calculate_volume(sr, playing, 1.0f);

	t = apply_pan_envelope(playing);
	state->pan = (unsigned char)((t + 128) >> IT_ENVELOPE_SHIFT);
	state->subpan = (signed char)t;

	delta = playing->delta * 65536.0f;
	t = playing->filter_cutoff << IT_ENVELOPE_SHIFT;
	apply_pitch_modifications(sr->sigdata, playing, &delta, &t);
	state->freq = (int)delta;
	if (t == 127 << IT_ENVELOPE_SHIFT && playing->filter_resonance == 0) {
		state->filter_resonance = playing->true_filter_resonance;
		t = playing->true_filter_cutoff;
	} else
		state->filter_resonance = playing->filter_resonance;
	state->filter_cutoff = (unsigned char)(t >> 8);
	state->filter_subcutoff = (unsigned char)t;
}



int dumb_it_callback_terminate(void *data)
{
	(void)data;
	return 1;
}



int dumb_it_callback_midi_block(void *data, int channel, unsigned char midi_byte)
{
	(void)data;
	(void)channel;
	(void)midi_byte;
	return 1;
}



#define IT_CHECKPOINT_INTERVAL (30 * 65536) /* Half a minute */



/* Returns the length of the module, up until it first loops. */
long dumb_it_build_checkpoints(DUMB_IT_SIGDATA *sigdata)
{
	IT_CHECKPOINT *checkpoint;
	if (!sigdata) return 0;
	checkpoint = sigdata->checkpoint;
	while (checkpoint) {
		IT_CHECKPOINT *next = checkpoint->next;
		_dumb_it_end_sigrenderer(checkpoint->sigrenderer);
		free(checkpoint);
		checkpoint = next;
	}
	sigdata->checkpoint = NULL;
	checkpoint = malloc(sizeof(*checkpoint));
	if (!checkpoint) return 0;
	checkpoint->time = 0;
	checkpoint->sigrenderer = dumb_it_init_sigrenderer(sigdata, 0, 0);
	if (!checkpoint->sigrenderer) {
		free(checkpoint);
		return 0;
	}
	checkpoint->sigrenderer->callbacks->loop = &dumb_it_callback_terminate;
	checkpoint->sigrenderer->callbacks->xm_speed_zero = &dumb_it_callback_terminate;
	sigdata->checkpoint = checkpoint;

	for (;;) {
		long l;
		DUMB_IT_SIGRENDERER *sigrenderer = dup_sigrenderer(checkpoint->sigrenderer, 0, checkpoint->sigrenderer->callbacks);
		checkpoint->sigrenderer->callbacks = NULL;
		if (!sigrenderer) {
			checkpoint->next = NULL;
			return checkpoint->time;
		}

		l = it_sigrenderer_get_samples(sigrenderer, 0, 1.0f, IT_CHECKPOINT_INTERVAL, NULL);
		if (l < IT_CHECKPOINT_INTERVAL) {
			_dumb_it_end_sigrenderer(sigrenderer);
			checkpoint->next = NULL;
			return checkpoint->time + l;
		}

		checkpoint->next = malloc(sizeof(*checkpoint->next));
		if (!checkpoint->next) {
			_dumb_it_end_sigrenderer(sigrenderer);
			return checkpoint->time + IT_CHECKPOINT_INTERVAL;
		}

		checkpoint->next->time = checkpoint->time + IT_CHECKPOINT_INTERVAL;
		checkpoint = checkpoint->next;
		checkpoint->sigrenderer = sigrenderer;
	}
}



void dumb_it_do_initial_runthrough(DUH *duh)
{
	if (duh) {
		DUMB_IT_SIGDATA *sigdata = duh_get_it_sigdata(duh);

		if (sigdata)
			duh_set_length(duh, dumb_it_build_checkpoints(sigdata));
	}
}
