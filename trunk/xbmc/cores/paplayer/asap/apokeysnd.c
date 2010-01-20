/*
 * apokeysnd.c - another POKEY sound emulator
 *
 * Copyright (C) 2007-2009  Piotr Fusik
 *
 * This file is part of ASAP (Another Slight Atari Player),
 * see http://asap.sourceforge.net
 *
 * ASAP is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published
 * by the Free Software Foundation; either version 2 of the License,
 * or (at your option) any later version.
 *
 * ASAP is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ASAP; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "asap_internal.h"

#define ULTRASOUND_CYCLES  112

#define MUTE_FREQUENCY     1
#define MUTE_INIT          2
#define MUTE_USER          4

CONST_ARRAY(byte, poly4_lookup)
	0, 0, 0, 0, 1, 1, 1, 0, 1, 1, 0, 0, 1, 0, 1
END_CONST_ARRAY;
CONST_ARRAY(byte, poly5_lookup)
	0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 1, 0, 0, 0, 1,
	0, 1, 0, 1, 1, 1, 1, 0, 1, 1, 0, 1, 0, 0, 1, 1
END_CONST_ARRAY;

PRIVATE FUNC(void, PokeySound_InitializeChip, (P(PokeyState PTR, pst)))
{
	pst _ audctl = 0;
	pst _ init = FALSE;
	pst _ poly_index = 15 * 31 * 131071;
	pst _ div_cycles = 28;
	pst _ mute1 = MUTE_FREQUENCY | MUTE_USER;
	pst _ mute2 = MUTE_FREQUENCY | MUTE_USER;
	pst _ mute3 = MUTE_FREQUENCY | MUTE_USER;
	pst _ mute4 = MUTE_FREQUENCY | MUTE_USER;
	pst _ audf1 = 0;
	pst _ audf2 = 0;
	pst _ audf3 = 0;
	pst _ audf4 = 0;
	pst _ audc1 = 0;
	pst _ audc2 = 0;
	pst _ audc3 = 0;
	pst _ audc4 = 0;
	pst _ tick_cycle1 = NEVER;
	pst _ tick_cycle2 = NEVER;
	pst _ tick_cycle3 = NEVER;
	pst _ tick_cycle4 = NEVER;
	pst _ period_cycles1 = 28;
	pst _ period_cycles2 = 28;
	pst _ period_cycles3 = 28;
	pst _ period_cycles4 = 28;
	pst _ reload_cycles1 = 28;
	pst _ reload_cycles3 = 28;
	pst _ out1 = 0;
	pst _ out2 = 0;
	pst _ out3 = 0;
	pst _ out4 = 0;
	pst _ delta1 = 0;
	pst _ delta2 = 0;
	pst _ delta3 = 0;
	pst _ delta4 = 0;
	pst _ skctl = 3;
	ZERO_ARRAY(pst _ delta_buffer);
}

FUNC(void, PokeySound_Initialize, (P(ASAP_State PTR, ast)))
{
	V(int, i);
	V(int, reg);
	reg = 0x1ff;
	for (i = 0; i < 511; i++) {
		reg = ((((reg >> 5) ^ reg) & 1) << 8) + (reg >> 1);
		ast _ poly9_lookup[i] = TO_BYTE(reg);
	}
	reg = 0x1ffff;
	for (i = 0; i < 16385; i++) {
		reg = ((((reg >> 5) ^ reg) & 0xff) << 9) + (reg >> 8);
		ast _ poly17_lookup[i] = TO_BYTE(reg >> 1);
	}
	ast _ sample_offset = 0;
	ast _ sample_index = 0;
	ast _ samples = 0;
	ast _ iir_acc_left = 0;
	ast _ iir_acc_right = 0;
	PokeySound_InitializeChip(ADDRESSOF ast _ base_pokey);
	PokeySound_InitializeChip(ADDRESSOF ast _ extra_pokey);
}

#define DO_TICK(ch) \
	if (pst _ init) { \
		switch (pst _ audc##ch >> 4) { \
		case 10: \
		case 14: \
			pst _ out##ch ^= 1; \
			pst _ delta_buffer[CYCLE_TO_SAMPLE(cycle)] += pst _ delta##ch = -pst _ delta##ch; \
			break; \
		default: \
			break; \
		} \
	} \
	else { \
		V(int, poly) = cycle + pst _ poly_index - (ch - 1); \
		V(int, newout) = pst _ out##ch; \
		switch (pst _ audc##ch >> 4) { \
		case 0: \
			if (poly5_lookup[poly % 31] != 0) { \
				if ((pst _ audctl & 0x80) != 0) \
					newout = ast _ poly9_lookup[poly % 511] & 1; \
				else { \
					poly %= 131071; \
					newout = (ast _ poly17_lookup[poly >> 3] >> (poly & 7)) & 1; \
				} \
			} \
			break; \
		case 2: \
		case 6: \
			newout ^= poly5_lookup[poly % 31]; \
			break; \
		case 4: \
			if (poly5_lookup[poly % 31] != 0) \
				newout = poly4_lookup[poly % 15]; \
			break; \
		case 8: \
			if ((pst _ audctl & 0x80) != 0) \
				newout = ast _ poly9_lookup[poly % 511] & 1; \
			else { \
				poly %= 131071; \
				newout = (ast _ poly17_lookup[poly >> 3] >> (poly & 7)) & 1; \
			} \
			break; \
		case 10: \
		case 14: \
			newout ^= 1; \
			break; \
		case 12: \
			newout = poly4_lookup[poly % 15]; \
			break; \
		default: \
			break; \
		} \
		if (newout != pst _ out##ch) { \
			pst _ out##ch = newout; \
			pst _ delta_buffer[CYCLE_TO_SAMPLE(cycle)] += pst _ delta##ch = -pst _ delta##ch; \
		} \
	}

/* Fills delta_buffer up to current_cycle basing on current AUDF/AUDC/AUDCTL values. */
PRIVATE FUNC(void, PokeySound_GenerateUntilCycle, (P(ASAP_State PTR, ast), P(PokeyState PTR, pst), P(int, current_cycle)))
{
	for (;;) {
		V(int, cycle) = current_cycle;
		if (cycle > pst _ tick_cycle1)
			cycle = pst _ tick_cycle1;
		if (cycle > pst _ tick_cycle2)
			cycle = pst _ tick_cycle2;
		if (cycle > pst _ tick_cycle3)
			cycle = pst _ tick_cycle3;
		if (cycle > pst _ tick_cycle4)
			cycle = pst _ tick_cycle4;
		if (cycle == current_cycle)
			break;
		if (cycle == pst _ tick_cycle3) {
			pst _ tick_cycle3 += pst _ period_cycles3;
			if ((pst _ audctl & 4) != 0 && pst _ delta1 > 0 && pst _ mute1 == 0)
				pst _ delta_buffer[CYCLE_TO_SAMPLE(cycle)] += pst _ delta1 = -pst _ delta1;
			DO_TICK(3);
		}
		if (cycle == pst _ tick_cycle4) {
			pst _ tick_cycle4 += pst _ period_cycles4;
			if ((pst _ audctl & 8) != 0)
				pst _ tick_cycle3 = cycle + pst _ reload_cycles3;
			if ((pst _ audctl & 2) != 0 && pst _ delta2 > 0 && pst _ mute2 == 0)
				pst _ delta_buffer[CYCLE_TO_SAMPLE(cycle)] += pst _ delta2 = -pst _ delta2;
			DO_TICK(4);
		}
		if (cycle == pst _ tick_cycle1) {
			pst _ tick_cycle1 += pst _ period_cycles1;
			if ((pst _ skctl & 0x88) == 8) /* two-tone, sending 1 (i.e. timer1) */
				pst _ tick_cycle2 = cycle + pst _ period_cycles2;
			DO_TICK(1);
		}
		if (cycle == pst _ tick_cycle2) {
			pst _ tick_cycle2 += pst _ period_cycles2;
			if ((pst _ audctl & 0x10) != 0)
				pst _ tick_cycle1 = cycle + pst _ reload_cycles1;
			else if ((pst _ skctl & 8) != 0) /* two-tone */
				pst _ tick_cycle1 = cycle + pst _ period_cycles1;
			DO_TICK(2);
		}
	}
}

#ifdef APOKEYSND

#define CURRENT_CYCLE  0
#define CURRENT_SAMPLE 0
#define DO_STORE(reg) \
	if (data == pst _ reg) \
		break; \
	pst _ reg = data;

#else

#define CURRENT_CYCLE  ast _ cycle
#define CURRENT_SAMPLE CYCLE_TO_SAMPLE(ast _ cycle)
#define DO_STORE(reg) \
	if (data == pst _ reg) \
		break; \
	PokeySound_GenerateUntilCycle(ast, pst, ast _ cycle); \
	pst _ reg = data;

#endif /* APOKEYSND */

#define MUTE_CHANNEL(ch, cond, mask) \
	if (cond) { \
		pst _ mute##ch |= mask; \
		pst _ tick_cycle##ch = NEVER; \
	} \
	else { \
		pst _ mute##ch &= ~mask; \
		if (pst _ tick_cycle##ch == NEVER && pst _ mute##ch == 0) \
			pst _ tick_cycle##ch = CURRENT_CYCLE; \
	}

#define DO_ULTRASOUND(ch) \
	MUTE_CHANNEL(ch, pst _ period_cycles##ch <= ULTRASOUND_CYCLES && (pst _ audc##ch >> 4 == 10 || pst _ audc##ch >> 4 == 14), MUTE_FREQUENCY)

#define DO_AUDC(ch) \
	DO_STORE(audc##ch); \
	if ((data & 0x10) != 0) { \
		data = (data & 0xf) << DELTA_SHIFT_POKEY; \
		if ((pst _ mute##ch & MUTE_USER) == 0) \
			pst _ delta_buffer[CURRENT_SAMPLE] \
				+= pst _ delta##ch > 0 ? data - pst _ delta##ch : data; \
		pst _ delta##ch = data; \
	} \
	else { \
		data = (data & 0xf) << DELTA_SHIFT_POKEY; \
		DO_ULTRASOUND(ch); \
		if (pst _ delta##ch > 0) { \
			if ((pst _ mute##ch & MUTE_USER) == 0) \
				pst _ delta_buffer[CURRENT_SAMPLE] \
					+= data - pst _ delta##ch; \
			pst _ delta##ch = data; \
		} \
		else \
			pst _ delta##ch = -data; \
	} \
	break;

#define DO_INIT(ch, cond) \
	MUTE_CHANNEL(ch, pst _ init && cond, MUTE_INIT)

FUNC(void, PokeySound_PutByte, (P(ASAP_State PTR, ast), P(int, addr), P(int, data)))
{
	V(PokeyState PTR, pst) = (addr & ast _ extra_pokey_mask) != 0
		? ADDRESSOF ast _ extra_pokey : ADDRESSOF ast _ base_pokey;
	switch (addr & 0xf) {
	case 0x00:
		DO_STORE(audf1);
		switch (pst _ audctl & 0x50) {
		case 0x00:
			pst _ period_cycles1 = pst _ div_cycles * (data + 1);
			break;
		case 0x10:
			pst _ period_cycles2 = pst _ div_cycles * (data + 256 * pst _ audf2 + 1);
			pst _ reload_cycles1 = pst _ div_cycles * (data + 1);
			DO_ULTRASOUND(2);
			break;
		case 0x40:
			pst _ period_cycles1 = data + 4;
			break;
		case 0x50:
			pst _ period_cycles2 = data + 256 * pst _ audf2 + 7;
			pst _ reload_cycles1 = data + 4;
			DO_ULTRASOUND(2);
			break;
		}
		DO_ULTRASOUND(1);
		break;
	case 0x01:
		DO_AUDC(1)
	case 0x02:
		DO_STORE(audf2);
		switch (pst _ audctl & 0x50) {
		case 0x00:
		case 0x40:
			pst _ period_cycles2 = pst _ div_cycles * (data + 1);
			break;
		case 0x10:
			pst _ period_cycles2 = pst _ div_cycles * (pst _ audf1 + 256 * data + 1);
			break;
		case 0x50:
			pst _ period_cycles2 = pst _ audf1 + 256 * data + 7;
			break;
		}
		DO_ULTRASOUND(2);
		break;
	case 0x03:
		DO_AUDC(2)
	case 0x04:
		DO_STORE(audf3);
		switch (pst _ audctl & 0x28) {
		case 0x00:
			pst _ period_cycles3 = pst _ div_cycles * (data + 1);
			break;
		case 0x08:
			pst _ period_cycles4 = pst _ div_cycles * (data + 256 * pst _ audf4 + 1);
			pst _ reload_cycles3 = pst _ div_cycles * (data + 1);
			DO_ULTRASOUND(4);
			break;
		case 0x20:
			pst _ period_cycles3 = data + 4;
			break;
		case 0x28:
			pst _ period_cycles4 = data + 256 * pst _ audf4 + 7;
			pst _ reload_cycles3 = data + 4;
			DO_ULTRASOUND(4);
			break;
		}
		DO_ULTRASOUND(3);
		break;
	case 0x05:
		DO_AUDC(3)
	case 0x06:
		DO_STORE(audf4);
		switch (pst _ audctl & 0x28) {
		case 0x00:
		case 0x20:
			pst _ period_cycles4 = pst _ div_cycles * (data + 1);
			break;
		case 0x08:
			pst _ period_cycles4 = pst _ div_cycles * (pst _ audf3 + 256 * data + 1);
			break;
		case 0x28:
			pst _ period_cycles4 = pst _ audf3 + 256 * data + 7;
			break;
		}
		DO_ULTRASOUND(4);
		break;
	case 0x07:
		DO_AUDC(4)
	case 0x08:
		DO_STORE(audctl);
		pst _ div_cycles = ((data & 1) != 0) ? 114 : 28;
		/* TODO: tick_cycles */
		switch (data & 0x50) {
		case 0x00:
			pst _ period_cycles1 = pst _ div_cycles * (pst _ audf1 + 1);
			pst _ period_cycles2 = pst _ div_cycles * (pst _ audf2 + 1);
			break;
		case 0x10:
			pst _ period_cycles1 = pst _ div_cycles * 256;
			pst _ period_cycles2 = pst _ div_cycles * (pst _ audf1 + 256 * pst _ audf2 + 1);
			pst _ reload_cycles1 = pst _ div_cycles * (pst _ audf1 + 1);
			break;
		case 0x40:
			pst _ period_cycles1 = pst _ audf1 + 4;
			pst _ period_cycles2 = pst _ div_cycles * (pst _ audf2 + 1);
			break;
		case 0x50:
			pst _ period_cycles1 = 256;
			pst _ period_cycles2 = pst _ audf1 + 256 * pst _ audf2 + 7;
			pst _ reload_cycles1 = pst _ audf1 + 4;
			break;
		}
		DO_ULTRASOUND(1);
		DO_ULTRASOUND(2);
		switch (data & 0x28) {
		case 0x00:
			pst _ period_cycles3 = pst _ div_cycles * (pst _ audf3 + 1);
			pst _ period_cycles4 = pst _ div_cycles * (pst _ audf4 + 1);
			break;
		case 0x08:
			pst _ period_cycles3 = pst _ div_cycles * 256;
			pst _ period_cycles4 = pst _ div_cycles * (pst _ audf3 + 256 * pst _ audf4 + 1);
			pst _ reload_cycles3 = pst _ div_cycles * (pst _ audf3 + 1);
			break;
		case 0x20:
			pst _ period_cycles3 = pst _ audf3 + 4;
			pst _ period_cycles4 = pst _ div_cycles * (pst _ audf4 + 1);
			break;
		case 0x28:
			pst _ period_cycles3 = 256;
			pst _ period_cycles4 = pst _ audf3 + 256 * pst _ audf4 + 7;
			pst _ reload_cycles3 = pst _ audf3 + 4;
			break;
		}
		DO_ULTRASOUND(3);
		DO_ULTRASOUND(4);
		DO_INIT(1, (data & 0x40) == 0);
		DO_INIT(2, (data & 0x50) != 0x50);
		DO_INIT(3, (data & 0x20) == 0);
		DO_INIT(4, (data & 0x28) != 0x28);
		break;
	case 0x09:
		/* TODO: STIMER */
		break;
	case 0x0f:
		DO_STORE(skctl);
		pst _ init = ((data & 3) == 0);
		DO_INIT(1, (pst _ audctl & 0x40) == 0);
		DO_INIT(2, (pst _ audctl & 0x50) != 0x50);
		DO_INIT(3, (pst _ audctl & 0x20) == 0);
		DO_INIT(4, (pst _ audctl & 0x28) != 0x28);
		break;
	default:
		break;
	}
}

FUNC(int, PokeySound_GetRandom, (P(ASAP_State PTR, ast), P(int, addr), P(int, cycle)))
{
	V(PokeyState PTR, pst) = (addr & ast _ extra_pokey_mask) != 0
		? ADDRESSOF ast _ extra_pokey : ADDRESSOF ast _ base_pokey;
	V(int, i);
	if (pst _ init)
		return 0xff;
	i = cycle + pst _ poly_index;
	if ((pst _ audctl & 0x80) != 0)
		return ast _ poly9_lookup[i % 511];
	else {
		V(int, j);
		i %= 131071;
		j = i >> 3;
		i &= 7;
		return ((ast _ poly17_lookup[j] >> i) + (ast _ poly17_lookup[j + 1] << (8 - i))) & 0xff;
	}
}

PRIVATE FUNC(void, end_frame, (P(ASAP_State PTR, ast), P(PokeyState PTR, pst), P(int, cycle_limit)))
{
	V(int, m);
	PokeySound_GenerateUntilCycle(ast, pst, cycle_limit);
	pst _ poly_index += cycle_limit;
	m = ((pst _ audctl & 0x80) != 0) ? 15 * 31 * 511 : 15 * 31 * 131071;
	if (pst _ poly_index >= 2 * m)
		pst _ poly_index -= m;
	if (pst _ tick_cycle1 != NEVER)
		pst _ tick_cycle1 -= cycle_limit;
	if (pst _ tick_cycle2 != NEVER)
		pst _ tick_cycle2 -= cycle_limit;
	if (pst _ tick_cycle3 != NEVER)
		pst _ tick_cycle3 -= cycle_limit;
	if (pst _ tick_cycle4 != NEVER)
		pst _ tick_cycle4 -= cycle_limit;
}

FUNC(void, PokeySound_StartFrame, (P(ASAP_State PTR, ast)))
{
	ZERO_ARRAY(ast _ base_pokey.delta_buffer);
	if (ast _ extra_pokey_mask != 0)
		ZERO_ARRAY(ast _ extra_pokey.delta_buffer);
}

FUNC(void, PokeySound_EndFrame, (P(ASAP_State PTR, ast), P(int, current_cycle)))
{
	end_frame(ast, ADDRESSOF ast _ base_pokey, current_cycle);
	if (ast _ extra_pokey_mask != 0)
		end_frame(ast, ADDRESSOF ast _ extra_pokey, current_cycle);
	ast _ sample_offset += current_cycle * ASAP_SAMPLE_RATE;
	ast _ sample_index = 0;
	ast _ samples = TO_INT(ast _ sample_offset / ASAP_MAIN_CLOCK);
	ast _ sample_offset %= ASAP_MAIN_CLOCK;
}

/* Fills buffer with samples from delta_buffer. */
FUNC(int, PokeySound_Generate, (P(ASAP_State PTR, ast), P(BYTEARRAY, buffer), P(int, buffer_offset), P(int, blocks), P(ASAP_SampleFormat, format)))
{
	V(int, i) = ast _ sample_index;
	V(int, samples) = ast _ samples;
	V(int, acc_left) = ast _ iir_acc_left;
	V(int, acc_right) = ast _ iir_acc_right;
	if (blocks < samples - i)
		samples = i + blocks;
	else
		blocks = samples - i;
	for (; i < samples; i++) {
#ifdef ACTIONSCRIPT
		acc_left += ast _ base_pokey.delta_buffer[i] - (acc_left * 3 >> 10);
		var sample : Number = acc_left / 33553408;
		buffer.writeFloat(sample);
		if (ast.extra_pokey_mask != 0) {
			acc_right += ast _ extra_pokey.delta_buffer[i] - (acc_right * 3 >> 10);
			sample = acc_right / 33553408;
		}
		buffer.writeFloat(sample);
#else
		V(int, sample);
		acc_left += ast _ base_pokey.delta_buffer[i] - (acc_left * 3 >> 10);
		sample = acc_left >> 10;
#define STORE_SAMPLE \
		if (sample < -32767) \
			sample = -32767; \
		else if (sample > 32767) \
			sample = 32767; \
		switch (format) { \
		case ASAP_FORMAT_U8: \
			buffer[buffer_offset++] = CAST(byte) ((sample >> 8) + 128); \
			break; \
		case ASAP_FORMAT_S16_LE: \
			buffer[buffer_offset++] = TO_BYTE(sample); \
			buffer[buffer_offset++] = TO_BYTE(sample >> 8); \
			break; \
		case ASAP_FORMAT_S16_BE: \
			buffer[buffer_offset++] = TO_BYTE(sample >> 8); \
			buffer[buffer_offset++] = TO_BYTE(sample); \
			break; \
		}
		STORE_SAMPLE;
		if (ast _ extra_pokey_mask != 0) {
			acc_right += ast _ extra_pokey.delta_buffer[i] - (acc_right * 3 >> 10);
			sample = acc_right >> 10;
			STORE_SAMPLE;
		}
#endif /* ACTIONSCRIPT */
	}
	if (i == ast _ samples) {
		acc_left += ast _ base_pokey.delta_buffer[i];
		acc_right += ast _ extra_pokey.delta_buffer[i];
	}
	ast _ sample_index = i;
	ast _ iir_acc_left = acc_left;
	ast _ iir_acc_right = acc_right;
#ifdef APOKEYSND
	return buffer_offset;
#else
	return blocks;
#endif
}

FUNC(abool, PokeySound_IsSilent, (P(CONST PokeyState PTR, pst)))
{
	return ((pst _ audc1 | pst _ audc2 | pst _ audc3 | pst _ audc4) & 0xf) == 0;
}

FUNC(void, PokeySound_Mute, (P(CONST ASAP_State PTR, ast), P(PokeyState PTR, pst), P(int, mask)))
{
	MUTE_CHANNEL(1, (mask & 1) != 0, MUTE_USER);
	MUTE_CHANNEL(2, (mask & 2) != 0, MUTE_USER);
	MUTE_CHANNEL(3, (mask & 4) != 0, MUTE_USER);
	MUTE_CHANNEL(4, (mask & 8) != 0, MUTE_USER);
}

#ifdef APOKEYSND

static ASAP_State asap;

__declspec(dllexport) void APokeySound_Initialize(abool stereo)
{
	asap.extra_pokey_mask = stereo ? 0x10 : 0;
	PokeySound_Initialize(&asap);
	PokeySound_Mute(&asap, &asap.base_pokey, 0);
	PokeySound_Mute(&asap, &asap.extra_pokey, 0);
	PokeySound_StartFrame(&asap);
}

__declspec(dllexport) void APokeySound_PutByte(int addr, int data)
{
	PokeySound_PutByte(&asap, addr, data);
}

__declspec(dllexport) int APokeySound_GetRandom(int addr, int cycle)
{
	return PokeySound_GetRandom(&asap, addr, cycle);
}

__declspec(dllexport) int APokeySound_Generate(int cycles, byte buffer[], ASAP_SampleFormat format)
{
	int len;
	PokeySound_EndFrame(&asap, cycles);
	len = PokeySound_Generate(&asap, buffer, 0, asap.samples, format);
	PokeySound_StartFrame(&asap);
	return len;
}

__declspec(dllexport) void APokeySound_About(const char **name, const char **author, const char **description)
{
	*name = "Another POKEY sound emulator, v" ASAP_VERSION;
	*author = "Piotr Fusik, (C) " ASAP_YEARS;
	*description = "Part of ASAP, http://asap.sourceforge.net";
}

#endif /* APOKEYSND */
