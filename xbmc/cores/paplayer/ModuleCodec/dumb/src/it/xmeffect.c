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
 * xmeffect.c - Code for converting MOD/XM            / / \  \
 *              effects to IT effects.               | <  /   \_
 *                                                   |  \/ /\   /
 * By Julien Cugniere. Ripped out of readxm.c         \_  /  > /
 * by entheh.                                           | \ / /
 *                                                      |  ' /
 *                                                       \__/
 */



#include <stdlib.h>
#include <string.h>

#include "dumb.h"
#include "internal/it.h"



#if 0
unsigned char **_dumb_malloc2(int w, int h)
{
	unsigned char **line =  malloc(h * sizeof(*line));
	int i;
	if (!line) return NULL;

	line[0] = malloc(w * h * sizeof(*line[0]));
	if (!line[0]) {
		free(line);
		return NULL;
	}

	for (i = 1; i < h; i++)
		line[i] = line[i-1] + w;

	memset(line[0], 0, w*h);

	return line;
}



void _dumb_free2(unsigned char **line)
{
	if (line) {
		if (line[0])
			free(line[0]);
		free(line);
	}
}



/* Effects having a memory. 2 means that the two parts of the effectvalue
 * should be handled separately.
 */
static const char xm_has_memory[] = {
/*	0  1  2  3  4  5  6  7  8  9  A  B  C  D (E) F  G  H        K  L           P     R     T          (X) */
	0, 1, 1, 1, 2, 1, 1, 2, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0,

/*  E0 E1 E2 E3 E4 E5 E6 E7    E9 EA EB EC ED EE         X1 X2 */
	0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0,   0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};
#endif



/* Effects marked with 'special' are handled specifically in itrender.c */
void _dumb_it_xm_convert_effect(int effect, int value, IT_ENTRY *entry)
{
const int log = 0;

	if ((!effect && !value) || (effect >= XM_N_EFFECTS))
		return;

if (log) printf("%c%02X", (effect<10)?('0'+effect):('A'+effect-10), value);

	/* Linearisation of the effect number... */
	if (effect == XM_E) {
		effect = EBASE + HIGH(value);
		value = LOW(value);
	} else if (effect == XM_X) {
		effect = XBASE + HIGH(value);
		value = LOW(value);
	}

if (log) printf(" - %2d %02X", effect, value);

#if 0 // This should be handled in itrender.c!
	/* update effect memory */
	switch (xm_has_memory[effect]) {
		case 1:
			if (!value)
				value = memory[entry->channel][effect];
			else
				memory[entry->channel][effect] = value;
			break;

		case 2:
			if (!HIGH(value))
				SET_HIGH(value, HIGH(memory[entry->channel][effect]));
			else
				SET_HIGH(memory[entry->channel][effect], HIGH(value));

			if (!LOW(value))
				SET_LOW(value, LOW(memory[entry->channel][effect]));
			else
				SET_LOW(memory[entry->channel][effect], LOW(value));
			break;
	}
#endif

	/* convert effect */
	entry->mask |= IT_ENTRY_EFFECT;
	switch (effect) {

		case XM_APPREGIO:           effect = IT_ARPEGGIO;           break;
		case XM_VIBRATO:            effect = IT_VIBRATO;            break;
		case XM_TONE_PORTAMENTO:    effect = IT_TONE_PORTAMENTO;    break; /** TODO: glissando control */
		case XM_TREMOLO:            effect = IT_TREMOLO;            break;
		case XM_SET_PANNING:        effect = IT_SET_PANNING;        break;
		case XM_SAMPLE_OFFSET:      effect = IT_SET_SAMPLE_OFFSET;  break;
		case XM_POSITION_JUMP:      effect = IT_JUMP_TO_ORDER;      break;
		case XM_MULTI_RETRIG:       effect = IT_RETRIGGER_NOTE;     break;
		case XM_TREMOR:             effect = IT_TREMOR;             break;
		case XM_PORTAMENTO_UP:      effect = IT_XM_PORTAMENTO_UP;   break;
		case XM_PORTAMENTO_DOWN:    effect = IT_XM_PORTAMENTO_DOWN; break;
		case XM_SET_CHANNEL_VOLUME: effect = IT_SET_CHANNEL_VOLUME; break; /* special */
		case XM_VOLSLIDE_TONEPORTA: effect = IT_VOLSLIDE_TONEPORTA; break; /* special */
		case XM_VOLSLIDE_VIBRATO:   effect = IT_VOLSLIDE_VIBRATO;   break; /* special */

		case XM_PATTERN_BREAK:
			effect = IT_BREAK_TO_ROW;
			value = BCD_TO_NORMAL(value);
			break;

		case XM_VOLUME_SLIDE: /* special */
			effect = IT_VOLUME_SLIDE;
			value = HIGH(value) ? EFFECT_VALUE(HIGH(value), 0) : EFFECT_VALUE(0, LOW(value));
			break;

		case XM_PANNING_SLIDE:
			effect = IT_PANNING_SLIDE;
			value = HIGH(value) ? EFFECT_VALUE(HIGH(value), 0) : EFFECT_VALUE(0, LOW(value));
			//value = HIGH(value) ? EFFECT_VALUE(0, HIGH(value)) : EFFECT_VALUE(LOW(value), 0);
			break;

		case XM_GLOBAL_VOLUME_SLIDE: /* special */
			effect = IT_GLOBAL_VOLUME_SLIDE;
			value = HIGH(value) ? EFFECT_VALUE(HIGH(value), 0) : EFFECT_VALUE(0, LOW(value));
			break;

		case XM_SET_TEMPO_BPM:
			effect = (value < 0x20) ? (IT_SET_SPEED) : (IT_SET_SONG_TEMPO);
			break;

		case XM_SET_GLOBAL_VOLUME:
			effect = IT_SET_GLOBAL_VOLUME;
			value *= 2;
			break;

		case XM_KEY_OFF:
			effect = IT_XM_KEY_OFF;
			break;

		case XM_SET_ENVELOPE_POSITION:
			effect = IT_XM_SET_ENVELOPE_POSITION;
			break;

		case EBASE+XM_E_SET_FILTER:            effect = SBASE+IT_S_SET_FILTER;            break;
		case EBASE+XM_E_SET_GLISSANDO_CONTROL: effect = SBASE+IT_S_SET_GLISSANDO_CONTROL; break; /** TODO */
		case EBASE+XM_E_SET_FINETUNE:          effect = SBASE+IT_S_FINETUNE;              break; /** TODO */
		case EBASE+XM_E_SET_LOOP:              effect = SBASE+IT_S_PATTERN_LOOP;          break;
		case EBASE+XM_E_NOTE_CUT:              effect = SBASE+IT_S_DELAYED_NOTE_CUT;      break;
		case EBASE+XM_E_NOTE_DELAY:            effect = SBASE+IT_S_NOTE_DELAY;            break;
		case EBASE+XM_E_PATTERN_DELAY:         effect = SBASE+IT_S_PATTERN_DELAY;         break;
		case EBASE+XM_E_FINE_VOLSLIDE_UP:      effect = IT_XM_FINE_VOLSLIDE_UP;           break;
		case EBASE+XM_E_FINE_VOLSLIDE_DOWN:    effect = IT_XM_FINE_VOLSLIDE_DOWN;         break;

		case EBASE + XM_E_FINE_PORTA_UP:
			effect = IT_PORTAMENTO_UP;
			value = EFFECT_VALUE(0xF, value);
			break;

		case EBASE + XM_E_FINE_PORTA_DOWN:
			effect = IT_PORTAMENTO_DOWN;
			value = EFFECT_VALUE(0xF, value);
			break;

		case EBASE + XM_E_RETRIG_NOTE:
			effect = IT_XM_RETRIGGER_NOTE;
			value = EFFECT_VALUE(0, value);
			break;

		case EBASE + XM_E_SET_VIBRATO_CONTROL:
			effect = SBASE+IT_S_SET_VIBRATO_WAVEFORM;
			value &= ~4; /** TODO: value&4 -> don't retrig wave */
			break;

		case EBASE + XM_E_SET_TREMOLO_CONTROL:
			effect = SBASE+IT_S_SET_TREMOLO_WAVEFORM;
			value &= ~4; /** TODO: value&4 -> don't retrig wave */
			break;

		case XBASE + XM_X_EXTRAFINE_PORTA_UP:
			effect = IT_PORTAMENTO_UP;
			value = EFFECT_VALUE(0xE, value);
			break;

		case XBASE + XM_X_EXTRAFINE_PORTA_DOWN:
			effect = IT_PORTAMENTO_DOWN;
			value = EFFECT_VALUE(0xE, value);
			break;

		default:
			/* user effect (often used in demos for synchronisation) */
			entry->mask &= ~IT_ENTRY_EFFECT;
	}

if (log) printf(" - %2d %02X", effect, value);

	/* Inverse linearisation... */
	if (effect >= SBASE && effect < SBASE+16) {
		value = EFFECT_VALUE(effect-SBASE, value);
		effect = IT_S;
	}

if (log) printf(" - %c%02X\n", 'A'+effect-1, value);

	entry->effect = effect;
	entry->effectvalue = value;
}
