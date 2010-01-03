/*
 * asap.c - ASAP engine
 *
 * Copyright (C) 2005-2009  Piotr Fusik
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

FUNC(int, ASAP_GetByte, (P(ASAP_State PTR, ast), P(int, addr)))
{
	switch (addr & 0xff0f) {
	case 0xd20a:
		return PokeySound_GetRandom(ast, addr, ast _ cycle);
	case 0xd20e:
		if ((addr & ast _ extra_pokey_mask) != 0) {
			/* interrupts in the extra POKEY not emulated at the moment */
			return 0xff;
		}
		return ast _ irqst;
	case 0xd20f:
		/* just because some SAP files rely on this */
		return 0xff;
	case 0xd40b:
		return ast _ scanline_number >> 1;
	default:
		return dGetByte(addr);
	}
}

FUNC(void, ASAP_PutByte, (P(ASAP_State PTR, ast), P(int, addr), P(int, data)))
{
	if ((addr >> 8) == 0xd2) {
		if ((addr & (ast _ extra_pokey_mask + 0xf)) == 0xe) {
			ast _ irqst |= data ^ 0xff;
#define SET_TIMER_IRQ(ch) \
			if ((data & ast _ irqst & ch) != 0) { \
				if (ast _ timer##ch##_cycle == NEVER) { \
					V(int, t) = ast _ base_pokey.tick_cycle##ch; \
					while (t < ast _ cycle) \
						t += ast _ base_pokey.period_cycles##ch; \
					ast _ timer##ch##_cycle = t; \
					if (ast _ nearest_event_cycle > t) \
						ast _ nearest_event_cycle = t; \
				} \
			} \
			else \
				ast _ timer##ch##_cycle = NEVER;
			SET_TIMER_IRQ(1);
			SET_TIMER_IRQ(2);
			SET_TIMER_IRQ(4);
		}
		else
			PokeySound_PutByte(ast, addr, data);
	}
	else if ((addr & 0xff0f) == 0xd40a) {
		if (ast _ cycle <= ast _ next_scanline_cycle - 8)
			ast _ cycle = ast _ next_scanline_cycle - 8;
		else
			ast _ cycle = ast _ next_scanline_cycle + 106;
	}
	else if ((addr & 0xff00) == ast _ module_info.covox_addr) {
		V(PokeyState PTR, pst);
		addr &= 3;
		if (addr == 0 || addr == 3)
			pst = ADDRESSOF ast _ base_pokey;
		else
			pst = ADDRESSOF ast _ extra_pokey;
		pst _ delta_buffer[CYCLE_TO_SAMPLE(ast _ cycle)] += (data - UBYTE(ast _ covox[addr])) << DELTA_SHIFT_COVOX;
		ast _ covox[addr] = CAST(byte) (data);
	}
	else if ((addr & 0xff1f) == 0xd01f) {
		V(int, sample) = CYCLE_TO_SAMPLE(ast _ cycle);
		V(int, delta);
		data &= 8;
		/* NOT data - ast _ consol; reverse to the POKEY sound */
		delta = (ast _ consol - data) << DELTA_SHIFT_GTIA;
		ast _ consol = data;
		ast _ base_pokey.delta_buffer[sample] += delta;
		ast _ extra_pokey.delta_buffer[sample] += delta;
	}
	else
		dPutByte(addr, data);
}

#define UWORD(array, index)  (UBYTE(array[index]) + (UBYTE(array[(index) + 1]) << 8))

#ifndef ASAP_ONLY_SAP

#if !defined(JAVA) && !defined(CSHARP)
#include "players.h"
#endif

#define CMR_BASS_TABLE_OFFSET  0x70f

CONST_ARRAY(byte, cmr_bass_table)
	0x5C, 0x56, 0x50, 0x4D, 0x47, 0x44, 0x41, 0x3E,
	0x38, 0x35, CAST(byte) (0x88), 0x7F, 0x79, 0x73, 0x6C, 0x67,
	0x60, 0x5A, 0x55, 0x51, 0x4C, 0x48, 0x43, 0x3F,
	0x3D, 0x39, 0x34, 0x33, 0x30, 0x2D, 0x2A, 0x28,
	0x25, 0x24, 0x21, 0x1F, 0x1E
END_CONST_ARRAY;

CONST_ARRAY(int, perframe2fastplay)
	312, 312 / 2, 312 / 3, 312 / 4
END_CONST_ARRAY;

/* Loads native module (anything except SAP) and 6502 player routine. */
PRIVATE FUNC(abool, load_native, (
	P(ASAP_State PTR, ast), P(ASAP_ModuleInfo PTR, module_info),
	P(CONST BYTEARRAY, module), P(int, module_len), P(RESOURCE, player)))
{
#ifdef JAVA
	InputStream playerStream = ASAP.class.getResourceAsStream(player);
#define READ_BYTE   read
#define READ_ARRAY  read
	try
#elif defined(CSHARP)
	Stream playerStream = System.Reflection.Assembly.GetExecutingAssembly().GetManifestResourceStream(player);
#define READ_BYTE   ReadByte
#define READ_ARRAY  Read
	try
#endif
	{
		V(int, player_last_byte);
		V(int, music_last_byte);
		V(int, block_len);
		if ((UBYTE(module[0]) != 0xff || UBYTE(module[1]) != 0xff)
		 && (module[0] != 0 || module[1] != 0)) /* some CMC and clones start with zeros */
			return FALSE;
#if defined(JAVA) || defined(CSHARP)
		playerStream.READ_BYTE();
		playerStream.READ_BYTE();
		module_info _ player = playerStream.READ_BYTE();
		module_info _ player += playerStream.READ_BYTE() << 8;
		player_last_byte = playerStream.READ_BYTE();
		player_last_byte += playerStream.READ_BYTE() << 8;
#else
		module_info _ player = UWORD(player, 2);
		player_last_byte = UWORD(player, 4);
#endif
		module_info _ music = UWORD(module, 2);
		if (module_info _ music <= player_last_byte)
			return FALSE;
		music_last_byte = UWORD(module, 4);
		if (module_info _ music <= 0xd7ff && music_last_byte >= 0xd000)
			return FALSE;
		block_len = music_last_byte + 1 - module_info _ music;
		if (6 + block_len != module_len) {
			V(int, info_addr);
			V(int, info_len);
			if (module_info _ type != ASAP_TYPE_RMT || 11 + block_len > module_len)
				return FALSE;
			/* allow optional info for Raster Music Tracker */
			info_addr = UWORD(module, 6 + block_len);
			if (info_addr != module_info _ music + block_len)
				return FALSE;
			info_len = UWORD(module, 8 + block_len) + 1 - info_addr;
			if (10 + block_len + info_len != module_len)
				return FALSE;
		}
		if (ast != NULL) {
			COPY_ARRAY(ast _ memory, module_info _ music, module, 6, block_len);
#if defined(JAVA) || defined(CSHARP)
			int addr = module_info _ player;
			do {
				int i = playerStream.READ_ARRAY(ast _ memory, addr, player_last_byte + 1 - addr);
				if (i <= 0)
					throw new IOException();
				addr += i;
			} while (addr <= player_last_byte);
#else
			COPY_ARRAY(ast _ memory, module_info _ player, player, 6, player_last_byte + 1 - module_info _ player);
#endif
		}
		return TRUE;
	}
#ifdef JAVA
	catch (IOException e) {
		throw new RuntimeException();
	}
	finally {
		try {
			playerStream.close();
		} catch (IOException e) {
			throw new RuntimeException();
		}
	}
#elif defined(CSHARP)
	finally {
		playerStream.Close();
	}
#endif
}

PRIVATE FUNC(void, set_song_duration, (P(ASAP_ModuleInfo PTR, module_info), P(int, player_calls)))
{
	module_info _ durations[module_info _ songs] = TO_INT(player_calls * module_info _ fastplay * 114000.0 / 1773447);
	module_info _ songs++;
}

#define SEEN_THIS_CALL  1
#define SEEN_BEFORE     2
#define SEEN_REPEAT     3

PRIVATE FUNC(void, parse_cmc_song, (P(ASAP_ModuleInfo PTR, module_info), P(CONST BYTEARRAY, module), P(int, pos)))
{
	V(int, tempo) = UBYTE(module[0x19]);
	V(int, player_calls) = 0;
	V(int, rep_start_pos) = 0;
	V(int, rep_end_pos) = 0;
	V(int, rep_times) = 0;
	NEW_ARRAY(byte, seen, 0x55);
	INIT_ARRAY(seen);
	while (pos >= 0 && pos < 0x55) {
		V(int, p1);
		V(int, p2);
		V(int, p3);
		if (pos == rep_end_pos && rep_times > 0) {
			for (p1 = 0; p1 < 0x55; p1++)
				if (seen[p1] == SEEN_THIS_CALL || seen[p1] == SEEN_REPEAT)
					seen[p1] = 0;
			rep_times--;
			pos = rep_start_pos;
		}
		if (seen[pos] != 0) {
			if (seen[pos] != SEEN_THIS_CALL)
				module_info _ loops[module_info _ songs] = TRUE;
			break;
		}
		seen[pos] = SEEN_THIS_CALL;
		p1 = UBYTE(module[0x206 + pos]);
		p2 = UBYTE(module[0x25b + pos]);
		p3 = UBYTE(module[0x2b0 + pos]);
		if (p1 == 0xfe || p2 == 0xfe || p3 == 0xfe) {
			pos++;
			continue;
		}
		p1 >>= 4;
		if (p1 == 8)
			break;
		if (p1 == 9) {
			pos = p2;
			continue;
		}
		if (p1 == 0xa) {
			pos -= p2;
			continue;
		}
		if (p1 == 0xb) {
			pos += p2;
			continue;
		}
		if (p1 == 0xc) {
			tempo = p2;
			pos++;
			continue;
		}
		if (p1 == 0xd) {
			pos++;
			rep_start_pos = pos;
			rep_end_pos = pos + p2;
			rep_times = p3 - 1;
			continue;
		}
		if (p1 == 0xe) {
			module_info _ loops[module_info _ songs] = TRUE;
			break;
		}
		p2 = rep_times > 0 ? SEEN_REPEAT : SEEN_BEFORE;
		for (p1 = 0; p1 < 0x55; p1++)
			if (seen[p1] == SEEN_THIS_CALL)
				seen[p1] = CAST(byte) p2;
		player_calls += tempo * (module_info _ type == ASAP_TYPE_CM3 ? 48 : 64);
		pos++;
	}
	set_song_duration(module_info, player_calls);
}

PRIVATE FUNC(void, parse_cmc_songs, (P(ASAP_ModuleInfo PTR, module_info), P(CONST BYTEARRAY, module)))
{
	V(int, last_pos);
	V(int, pos);
	last_pos = 0x54;
	while (--last_pos >= 0) {
		if (UBYTE(module[0x206 + last_pos]) < 0xb0
		 || UBYTE(module[0x25b + last_pos]) < 0x40
		 || UBYTE(module[0x2b0 + last_pos]) < 0x40)
			break;
		if (module_info _ channels == 2) {
			if (UBYTE(module[0x306 + last_pos]) < 0xb0
			 || UBYTE(module[0x35b + last_pos]) < 0x40
			 || UBYTE(module[0x3b0 + last_pos]) < 0x40)
				break;
		}
	}
	module_info _ songs = 0;
	parse_cmc_song(module_info, module, 0);
	for (pos = 0; pos < last_pos && module_info _ songs < ASAP_SONGS_MAX; pos++)
		if (UBYTE(module[0x206 + pos]) == 0x8f || UBYTE(module[0x206 + pos]) == 0xef)
			parse_cmc_song(module_info, module, pos + 1);
}

PRIVATE FUNC(abool, parse_cmc, (
	P(ASAP_State PTR, ast), P(ASAP_ModuleInfo PTR, module_info),
	P(CONST BYTEARRAY, module), P(int, module_len), P(int, type)))
{
	if (module_len < 0x306)
		return FALSE;
	module_info _ type = type;
	if (!load_native(ast, module_info, module, module_len, GET_RESOURCE(cmc, obx)))
		return FALSE;
	if (ast != NULL && type == ASAP_TYPE_CMR)
		COPY_ARRAY(ast _ memory, 0x500 + CMR_BASS_TABLE_OFFSET, cmr_bass_table, 0, sizeof(cmr_bass_table));
	parse_cmc_songs(module_info, module);
	return TRUE;
}

PRIVATE FUNC(abool, parse_cm3, (
	P(ASAP_State PTR, ast), P(ASAP_ModuleInfo PTR, module_info),
	P(CONST BYTEARRAY, module), P(int, module_len)))
{
	if (module_len < 0x306)
		return FALSE;
	module_info _ type = ASAP_TYPE_CM3;
	if (!load_native(ast, module_info, module, module_len, GET_RESOURCE(cm3, obx)))
		return FALSE;
	parse_cmc_songs(module_info, module);
	return TRUE;
}

PRIVATE FUNC(abool, parse_cms, (
	P(ASAP_State PTR, ast), P(ASAP_ModuleInfo PTR, module_info),
	P(CONST BYTEARRAY, module), P(int, module_len)))
{
	if (module_len < 0x406)
		return FALSE;
	module_info _ type = ASAP_TYPE_CMS;
	module_info _ channels = 2;
	if (!load_native(ast, module_info, module, module_len, GET_RESOURCE(cms, obx)))
		return FALSE;
	parse_cmc_songs(module_info, module);
	return TRUE;
}

PRIVATE FUNC(abool, is_dlt_track_empty, (P(CONST BYTEARRAY, module), P(int, pos)))
{
	return UBYTE(module[0x2006 + pos]) >= 0x43
		&& UBYTE(module[0x2106 + pos]) >= 0x40
		&& UBYTE(module[0x2206 + pos]) >= 0x40
		&& UBYTE(module[0x2306 + pos]) >= 0x40;
}

PRIVATE FUNC(abool, is_dlt_pattern_end, (P(CONST BYTEARRAY, module), P(int, pos), P(int, i)))
{
	V(int, ch);
	for (ch = 0; ch < 4; ch++) {
		V(int, pattern) = UBYTE(module[0x2006 + (ch << 8) + pos]);
		if (pattern < 64) {
			V(int, offset) = 6 + (pattern << 7) + (i << 1);
			if ((module[offset] & 0x80) == 0 && (module[offset + 1] & 0x80) != 0)
				return TRUE;
		}
	}
	return FALSE;
}

PRIVATE FUNC(void, parse_dlt_song, (
	P(ASAP_ModuleInfo PTR, module_info), P(CONST BYTEARRAY, module),
	P(BOOLARRAY, seen), P(int, pos)))
{
	V(int, player_calls) = 0;
	V(abool, loop) = FALSE;
	V(int, tempo) = 6;
	while (pos < 128 && !seen[pos] && is_dlt_track_empty(module, pos))
		seen[pos++] = TRUE;
	module_info _ song_pos[module_info _ songs] = CAST(byte) pos;
	while (pos < 128) {
		V(int, p1);
		if (seen[pos]) {
			loop = TRUE;
			break;
		}
		seen[pos] = TRUE;
		p1 = module[0x2006 + pos];
		if (p1 == 0x40 || is_dlt_track_empty(module, pos))
			break;
		if (p1 == 0x41)
			pos = UBYTE(module[0x2086 + pos]);
		else if (p1 == 0x42)
			tempo = UBYTE(module[0x2086 + pos++]);
		else {
			V(int, i);
			for (i = 0; i < 64 && !is_dlt_pattern_end(module, pos, i); i++)
				player_calls += tempo;
			pos++;
		}
	}
	if (player_calls > 0) {
		module_info _ loops[module_info _ songs] = loop;
		set_song_duration(module_info, player_calls);
	}
}

PRIVATE FUNC(abool, parse_dlt, (
	P(ASAP_State PTR, ast), P(ASAP_ModuleInfo PTR, module_info),
	P(CONST BYTEARRAY, module), P(int, module_len)))
{
	V(int, pos);
	NEW_ARRAY(abool, seen, 128);
	if (module_len == 0x2c06) {
		if (ast != NULL)
			ast _ memory[0x4c00] = 0;
	}
	else if (module_len != 0x2c07)
		return FALSE;
	module_info _ type = ASAP_TYPE_DLT;
	if (!load_native(ast, module_info, module, module_len, GET_RESOURCE(dlt, obx))
	 || module_info _ music != 0x2000) {
		return FALSE;
	}
	INIT_ARRAY(seen);
	module_info _ songs = 0;
	for (pos = 0; pos < 128 && module_info _ songs < ASAP_SONGS_MAX; pos++) {
		if (!seen[pos])
			parse_dlt_song(module_info, module, seen, pos);
	}
	return module_info _ songs > 0;
}

PRIVATE FUNC(void, parse_mpt_song, (
	P(ASAP_ModuleInfo PTR, module_info), P(CONST BYTEARRAY, module),
	P(BOOLARRAY, global_seen), P(int, song_len), P(int, pos)))
{
	V(int, addr_to_offset) = UWORD(module, 2) - 6;
	V(int, tempo) = UBYTE(module[0x1cf]);
	V(int, player_calls) = 0;
	NEW_ARRAY(byte, seen, 256);
	NEW_ARRAY(int, pattern_offset, 4);
	NEW_ARRAY(int, blank_rows, 4);
	NEW_ARRAY(int, blank_rows_counter, 4);
	INIT_ARRAY(seen);
	INIT_ARRAY(blank_rows);
	while (pos < song_len) {
		V(int, i);
		V(int, ch);
		V(int, pattern_rows);
		if (seen[pos] != 0) {
			if (seen[pos] != SEEN_THIS_CALL)
				module_info _ loops[module_info _ songs] = TRUE;
			break;
		}
		seen[pos] = SEEN_THIS_CALL;
		global_seen[pos] = TRUE;
		i = UBYTE(module[0x1d0 + pos * 2]);
		if (i == 0xff) {
			pos = UBYTE(module[0x1d1 + pos * 2]);
			continue;
		}
		for (ch = 3; ch >= 0; ch--) {
			i = UBYTE(module[0x1c6 + ch]) + (UBYTE(module[0x1ca + ch]) << 8) - addr_to_offset;
			i = UBYTE(module[i + pos * 2]);
			if (i >= 0x40)
				break;
			i <<= 1;
			i = UWORD(module, 0x46 + i);
			pattern_offset[ch] = i == 0 ? 0 : i - addr_to_offset;
			blank_rows_counter[ch] = 0;
		}
		if (ch >= 0)
			break;
		for (i = 0; i < song_len; i++)
			if (seen[i] == SEEN_THIS_CALL)
				seen[i] = SEEN_BEFORE;
		for (pattern_rows = UBYTE(module[0x1ce]); --pattern_rows >= 0; ) {
			for (ch = 3; ch >= 0; ch--) {
				if (pattern_offset[ch] == 0 || --blank_rows_counter[ch] >= 0)
					continue;
				for (;;) {
					i = UBYTE(module[pattern_offset[ch]++]);
					if (i < 0x40 || i == 0xfe)
						break;
					if (i < 0x80)
						continue;
					if (i < 0xc0) {
						blank_rows[ch] = i - 0x80;
						continue;
					}
					if (i < 0xd0)
						continue;
					if (i < 0xe0) {
						tempo = i - 0xcf;
						continue;
					}
					pattern_rows = 0;
				}
				blank_rows_counter[ch] = blank_rows[ch];
			}
			player_calls += tempo;
		}
		pos++;
	}
	if (player_calls > 0)
		set_song_duration(module_info, player_calls);
}

PRIVATE FUNC(abool, parse_mpt, (
	P(ASAP_State PTR, ast), P(ASAP_ModuleInfo PTR, module_info),
	P(CONST BYTEARRAY, module), P(int, module_len)))
{
	V(int, track0_addr);
	V(int, pos);
	V(int, song_len);
	/* seen[i] == TRUE if the track position i has been processed */
	NEW_ARRAY(abool, global_seen, 256);
	if (module_len < 0x1d0)
		return FALSE;
	module_info _ type = ASAP_TYPE_MPT;
	if (!load_native(ast, module_info, module, module_len, GET_RESOURCE(mpt, obx)))
		return FALSE;
	track0_addr = UWORD(module, 2) + 0x1ca;
	if (UBYTE(module[0x1c6]) + (UBYTE(module[0x1ca]) << 8) != track0_addr)
		return FALSE;
	/* Calculate the length of the first track. Address of the second track minus
	   address of the first track equals the length of the first track in bytes.
	   Divide by two to get number of track positions. */
	song_len = (UBYTE(module[0x1c7]) + (UBYTE(module[0x1cb]) << 8) - track0_addr) >> 1;
	if (song_len > 0xfe)
		return FALSE;
	INIT_ARRAY(global_seen);
	module_info _ songs = 0;
	for (pos = 0; pos < song_len && module_info _ songs < ASAP_SONGS_MAX; pos++) {
		if (!global_seen[pos]) {
			module_info _ song_pos[module_info _ songs] = CAST(byte) pos;
			parse_mpt_song(module_info, module, global_seen, song_len, pos);
		}
	}
	return module_info _ songs > 0;
}

CONST_ARRAY(byte, rmt_volume_silent)
	16, 8, 4, 3, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1
END_CONST_ARRAY;

PRIVATE FUNC(int, rmt_instrument_frames, (
	P(CONST BYTEARRAY, module), P(int, instrument),
	P(int, volume), P(int, volume_frame), P(abool, extra_pokey)))
{
	V(int, addr_to_offset) = UWORD(module, 2) - 6;
	V(int, per_frame) = module[0xc];
	V(int, player_call);
	V(int, player_calls);
	V(int, index);
	V(int, index_end);
	V(int, index_loop);
	V(int, volume_slide_depth);
	V(int, volume_min);
	V(int, volume_slide);
	V(abool, silent_loop);
	instrument = UWORD(module, 0xe) - addr_to_offset + (instrument << 1);
	if (module[instrument + 1] == 0)
		return 0;
	instrument = UWORD(module, instrument) - addr_to_offset;
	player_calls = player_call = volume_frame * per_frame;
	index = UBYTE(module[instrument]) + 1 + player_call * 3;
	index_end = UBYTE(module[instrument + 2]) + 3;
	index_loop = UBYTE(module[instrument + 3]);
	if (index_loop >= index_end)
		return 0; /* error */
	volume_slide_depth = UBYTE(module[instrument + 6]);
	volume_min = UBYTE(module[instrument + 7]);
	if (index >= index_end)
		index = (index - index_end) % (index_end - index_loop) + index_loop;
	else {
		do {
			V(int, vol) = module[instrument + index];
			if (extra_pokey)
				vol >>= 4;
			if ((vol & 0xf) >= rmt_volume_silent[volume])
				player_calls = player_call + 1;
			player_call++;
			index += 3;
		} while (index < index_end);
	}
	if (volume_slide_depth == 0)
		return player_calls / per_frame;
	volume_slide = 128;
	silent_loop = FALSE;
	for (;;) {
		V(int, vol);
		if (index >= index_end) {
			if (silent_loop)
				break;
			silent_loop = TRUE;
			index = index_loop;
		}
		vol = module[instrument + index];
		if (extra_pokey)
			vol >>= 4;
		if ((vol & 0xf) >= rmt_volume_silent[volume]) {
			player_calls = player_call + 1;
			silent_loop = FALSE;
		}
		player_call++;
		index += 3;
		volume_slide -= volume_slide_depth;
		if (volume_slide < 0) {
			volume_slide += 256;
			if (--volume <= volume_min)
				break;
		}
	}
	return player_calls / per_frame;
}

PRIVATE FUNC(void, parse_rmt_song, (
	P(ASAP_ModuleInfo PTR, module_info), P(CONST BYTEARRAY, module),
	P(BOOLARRAY, global_seen), P(int, song_len), P(int, pos_shift), P(int, pos)))
{
	V(int, ch);
	V(int, addr_to_offset) = UWORD(module, 2) - 6;
	V(int, tempo) = UBYTE(module[0xb]);
	V(int, frames) = 0;
	V(int, song_offset) = UWORD(module, 0x14) - addr_to_offset;
	V(int, pattern_lo_offset) = UWORD(module, 0x10) - addr_to_offset;
	V(int, pattern_hi_offset) = UWORD(module, 0x12) - addr_to_offset;
	V(int, instrument_frames);
	NEW_ARRAY(byte, seen, 256);
	NEW_ARRAY(int, pattern_begin, 8);
	NEW_ARRAY(int, pattern_offset, 8);
	NEW_ARRAY(int, blank_rows, 8);
	NEW_ARRAY(int, instrument_no, 8);
	NEW_ARRAY(int, instrument_frame, 8);
	NEW_ARRAY(int, volume_value, 8);
	NEW_ARRAY(int, volume_frame, 8);
	INIT_ARRAY(seen);
	INIT_ARRAY(instrument_no);
	INIT_ARRAY(instrument_frame);
	INIT_ARRAY(volume_value);
	INIT_ARRAY(volume_frame);
	while (pos < song_len) {
		V(int, i);
		V(int, pattern_rows);
		if (seen[pos] != 0) {
			if (seen[pos] != SEEN_THIS_CALL)
				module_info _ loops[module_info _ songs] = TRUE;
			break;
		}
		seen[pos] = SEEN_THIS_CALL;
		global_seen[pos] = TRUE;
		if (UBYTE(module[song_offset + (pos << pos_shift)]) == 0xfe) {
			pos = UBYTE(module[song_offset + (pos << pos_shift) + 1]);
			continue;
		}
		for (ch = 0; ch < 1 << pos_shift; ch++) {
			i = UBYTE(module[song_offset + (pos << pos_shift) + ch]);
			if (i == 0xff)
				blank_rows[ch] = 256;
			else {
				pattern_offset[ch] = pattern_begin[ch] = UBYTE(module[pattern_lo_offset + i])
					+ (UBYTE(module[pattern_hi_offset + i]) << 8) - addr_to_offset;
				blank_rows[ch] = 0;
			}
		}
		for (i = 0; i < song_len; i++)
			if (seen[i] == SEEN_THIS_CALL)
				seen[i] = SEEN_BEFORE;
		for (pattern_rows = UBYTE(module[0xa]); --pattern_rows >= 0; ) {
			for (ch = 0; ch < 1 << pos_shift; ch++) {
				if (--blank_rows[ch] > 0)
					continue;
				for (;;) {
					i = UBYTE(module[pattern_offset[ch]++]);
					if ((i & 0x3f) < 62) {
						i += UBYTE(module[pattern_offset[ch]++]) << 8;
						if ((i & 0x3f) != 61) {
							instrument_no[ch] = i >> 10;
							instrument_frame[ch] = frames;
						}
						volume_value[ch] = (i >> 6) & 0xf;
						volume_frame[ch] = frames;
						break;
					}
					if (i == 62) {
						blank_rows[ch] = UBYTE(module[pattern_offset[ch]++]);
						break;
					}
					if ((i & 0x3f) == 62) {
						blank_rows[ch] = i >> 6;
						break;
					}
					if ((i & 0xbf) == 63) {
						tempo = UBYTE(module[pattern_offset[ch]++]);
						continue;
					}
					if (i == 0xbf) {
						pattern_offset[ch] = pattern_begin[ch] + UBYTE(module[pattern_offset[ch]]);
						continue;
					}
					/* assert(i == 0xff); */
					pattern_rows = -1;
					break;
				}
				if (pattern_rows < 0)
					break;
			}
			if (pattern_rows >= 0)
				frames += tempo;
		}
		pos++;
	}
	instrument_frames = 0;
	for (ch = 0; ch < 1 << pos_shift; ch++) {
		V(int, frame) = instrument_frame[ch];
		frame += rmt_instrument_frames(module, instrument_no[ch], volume_value[ch], volume_frame[ch] - frame, ch >= 4);
		if (instrument_frames < frame)
			instrument_frames = frame;
	}
	if (frames > instrument_frames) {
		if (frames - instrument_frames > 100)
			module_info _ loops[module_info _ songs] = FALSE;
		frames = instrument_frames;
	}
	if (frames > 0)
		set_song_duration(module_info, frames);
}

PRIVATE FUNC(abool, parse_rmt, (
	P(ASAP_State PTR, ast), P(ASAP_ModuleInfo PTR, module_info),
	P(CONST BYTEARRAY, module), P(int, module_len)))
{
	V(int, per_frame);
	V(int, pos_shift);
	V(int, song_len);
	V(int, pos);
	NEW_ARRAY(abool, global_seen, 256);
	if (module_len < 0x30 || module[6] != CHARCODE('R') || module[7] != CHARCODE('M')
	 || module[8] != CHARCODE('T') || module[0xd] != 1)
		return FALSE;
	switch (CAST(char) module[9]) {
	case CHARCODE('4'):
		pos_shift = 2;
		break;
	case CHARCODE('8'):
		module_info _ channels = 2;
		pos_shift = 3;
		break;
	default:
		return FALSE;
	}
	per_frame = module[0xc];
	if (per_frame < 1 || per_frame > 4)
		return FALSE;
	module_info _ type = ASAP_TYPE_RMT;
	if (!load_native(ast, module_info, module, module_len,
		module_info _ channels == 2 ? GET_RESOURCE(rmt8, obx) : GET_RESOURCE(rmt4, obx)))
		return FALSE;
	song_len = UWORD(module, 4) + 1 - UWORD(module, 0x14);
	if (pos_shift == 3 && (song_len & 4) != 0
	 && UBYTE(module[6 + UWORD(module, 4) - UWORD(module, 2) - 3]) == 0xfe)
		song_len += 4;
	song_len >>= pos_shift;
	if (song_len >= 0x100)
		return FALSE;
	INIT_ARRAY(global_seen);
	module_info _ songs = 0;
	for (pos = 0; pos < song_len && module_info _ songs < ASAP_SONGS_MAX; pos++) {
		if (!global_seen[pos]) {
			module_info _ song_pos[module_info _ songs] = CAST(byte) pos;
			parse_rmt_song(module_info, module, global_seen, song_len, pos_shift, pos);
		}
	}
	/* must set fastplay after song durations calculations, so they assume 312 */
	module_info _ fastplay = perframe2fastplay[per_frame - 1];
	module_info _ player = 0x600;
	return module_info _ songs > 0;
}

PRIVATE FUNC(void, parse_tmc_song, (
	P(ASAP_ModuleInfo PTR, module_info), P(CONST BYTEARRAY, module), P(int, pos)))
{
	V(int, addr_to_offset) = UWORD(module, 2) - 6;
	V(int, tempo) = UBYTE(module[0x24]) + 1;
	V(int, frames) = 0;
	NEW_ARRAY(int, pattern_offset, 8);
	NEW_ARRAY(int, blank_rows, 8);
	while (UBYTE(module[0x1a6 + 15 + pos]) < 0x80) {
		V(int, ch);
		V(int, pattern_rows);
		for (ch = 7; ch >= 0; ch--) {
			V(int, pat) = UBYTE(module[0x1a6 + 15 + pos - 2 * ch]);
			pattern_offset[ch] = UBYTE(module[0xa6 + pat]) + (UBYTE(module[0x126 + pat]) << 8) - addr_to_offset;
			blank_rows[ch] = 0;
		}
		for (pattern_rows = 64; --pattern_rows >= 0; ) {
			for (ch = 7; ch >= 0; ch--) {
				if (--blank_rows[ch] >= 0)
					continue;
				for (;;) {
					V(int, i) = UBYTE(module[pattern_offset[ch]++]);
					if (i < 0x40) {
						pattern_offset[ch]++;
						break;
					}
					if (i == 0x40) {
						i = UBYTE(module[pattern_offset[ch]++]);
						if ((i & 0x7f) == 0)
							pattern_rows = 0;
						else
							tempo = (i & 0x7f) + 1;
						if (i >= 0x80)
							pattern_offset[ch]++;
						break;
					}
					if (i < 0x80) {
						i = module[pattern_offset[ch]++] & 0x7f;
						if (i == 0)
							pattern_rows = 0;
						else
							tempo = i + 1;
						pattern_offset[ch]++;
						break;
					}
					if (i < 0xc0)
						continue;
					blank_rows[ch] = i - 0xbf;
					break;
				}
			}
			frames += tempo;
		}
		pos += 16;
	}
	if (UBYTE(module[0x1a6 + 14 + pos]) < 0x80)
		module_info _ loops[module_info _ songs] = TRUE;
	set_song_duration(module_info, frames);
}

PRIVATE FUNC(abool, parse_tmc, (
	P(ASAP_State PTR, ast), P(ASAP_ModuleInfo PTR, module_info),
	P(CONST BYTEARRAY, module), P(int, module_len)))
{
	V(int, i);
	V(int, last_pos);
	if (module_len < 0x1d0)
		return FALSE;
	module_info _ type = ASAP_TYPE_TMC;
	if (!load_native(ast, module_info, module, module_len, GET_RESOURCE(tmc, obx)))
		return FALSE;
	module_info _ channels = 2;
	i = 0;
	/* find first instrument */
	while (module[0x66 + i] == 0) {
		if (++i >= 64)
			return FALSE; /* no instrument */
	}
	last_pos = (UBYTE(module[0x66 + i]) << 8) + UBYTE(module[0x26 + i])
		- UWORD(module, 2) - 0x1b0;
	if (0x1b5 + last_pos >= module_len)
		return FALSE;
	/* skip trailing jumps */
	do {
		if (last_pos <= 0)
			return FALSE; /* no pattern to play */
		last_pos -= 16;
	} while (UBYTE(module[0x1b5 + last_pos]) >= 0x80);
	module_info _ songs = 0;
	parse_tmc_song(module_info, module, 0);
	for (i = 0; i < last_pos && module_info _ songs < ASAP_SONGS_MAX; i += 16)
		if (UBYTE(module[0x1b5 + i]) >= 0x80)
			parse_tmc_song(module_info, module, i + 16);
	/* must set fastplay after song durations calculations, so they assume 312 */
	i = module[0x25];
	if (i < 1 || i > 4)
		return FALSE;
	if (ast != NULL)
		ast _ tmc_per_frame = module[0x25];
	module_info _ fastplay = perframe2fastplay[i - 1];
	return TRUE;
}

PRIVATE FUNC(void, parse_tm2_song, (
	P(ASAP_ModuleInfo PTR, module_info), P(CONST BYTEARRAY, module), P(int, pos)))
{
	V(int, addr_to_offset) = UWORD(module, 2) - 6;
	V(int, tempo) = UBYTE(module[0x24]) + 1;
	V(int, player_calls) = 0;
	NEW_ARRAY(int, pattern_offset, 8);
	NEW_ARRAY(int, blank_rows, 8);
	for (;;) {
		V(int, ch);
		V(int, pattern_rows) = UBYTE(module[0x386 + 16 + pos]);
		if (pattern_rows == 0)
			break;
		if (pattern_rows >= 0x80) {
			module_info _ loops[module_info _ songs] = TRUE;
			break;
		}
		for (ch = 7; ch >= 0; ch--) {
			V(int, pat) = UBYTE(module[0x386 + 15 + pos - 2 * ch]);
			pattern_offset[ch] = UBYTE(module[0x106 + pat]) + (UBYTE(module[0x206 + pat]) << 8) - addr_to_offset;
			blank_rows[ch] = 0;
		}
		while (--pattern_rows >= 0) {
			for (ch = 7; ch >= 0; ch--) {
				if (--blank_rows[ch] >= 0)
					continue;
				for (;;) {
					V(int, i) = UBYTE(module[pattern_offset[ch]++]);
					if (i == 0) {
						pattern_offset[ch]++;
						break;
					}
					if (i < 0x40) {
						if (UBYTE(module[pattern_offset[ch]++]) >= 0x80)
							pattern_offset[ch]++;
						break;
					}
					if (i < 0x80) {
						pattern_offset[ch]++;
						break;
					}
					if (i == 0x80) {
						blank_rows[ch] = UBYTE(module[pattern_offset[ch]++]);
						break;
					}
					if (i < 0xc0)
						break;
					if (i < 0xd0) {
						tempo = i - 0xbf;
						continue;
					}
					if (i < 0xe0) {
						pattern_offset[ch]++;
						break;
					}
					if (i < 0xf0) {
						pattern_offset[ch] += 2;
						break;
					}
					if (i < 0xff) {
						blank_rows[ch] = i - 0xf0;
						break;
					}
					blank_rows[ch] = 64;
					break;
				}
			}
			player_calls += tempo;
		}
		pos += 17;
	}
	set_song_duration(module_info, player_calls);
}

PRIVATE FUNC(abool, parse_tm2, (
	P(ASAP_State PTR, ast), P(ASAP_ModuleInfo PTR, module_info),
	P(CONST BYTEARRAY, module), P(int, module_len)))
{
	V(int, i);
	V(int, last_pos);
	V(int, c);
	if (module_len < 0x3a4)
		return FALSE;
	module_info _ type = ASAP_TYPE_TM2;
	if (!load_native(ast, module_info, module, module_len, GET_RESOURCE(tm2, obx)))
		return FALSE;
	i = module[0x25];
	if (i < 1 || i > 4)
		return FALSE;
	module_info _ fastplay = perframe2fastplay[i - 1];
	module_info _ player = 0x500;
	if (module[0x1f] != 0)
		module_info _ channels = 2;
	last_pos = 0xffff;
	for (i = 0; i < 0x80; i++) {
		V(int, instr_addr) = UBYTE(module[0x86 + i]) + (UBYTE(module[0x306 + i]) << 8);
		if (instr_addr != 0 && instr_addr < last_pos)
			last_pos = instr_addr;
	}
	for (i = 0; i < 0x100; i++) {
		V(int, pattern_addr) = UBYTE(module[0x106 + i]) + (UBYTE(module[0x206 + i]) << 8);
		if (pattern_addr != 0 && pattern_addr < last_pos)
			last_pos = pattern_addr;
	}
	last_pos -= UWORD(module, 2) + 0x380;
	if (0x386 + last_pos >= module_len)
		return FALSE;
	/* skip trailing stop/jump commands */
	do {
		if (last_pos <= 0)
			return FALSE;
		last_pos -= 17;
		c = UBYTE(module[0x386 + 16 + last_pos]);
	} while (c == 0 || c >= 0x80);
	module_info _ songs = 0;
	parse_tm2_song(module_info, module, 0);
	for (i = 0; i < last_pos && module_info _ songs < ASAP_SONGS_MAX; i += 17) {
		c = UBYTE(module[0x386 + 16 + i]);
		if (c == 0 || c >= 0x80)
			parse_tm2_song(module_info, module, i + 17);
	}
	return TRUE;
}

#endif /* ASAP_ONLY_SAP */

#ifdef C

static abool parse_hex(int *retval, const char *p)
{
	int r = 0;
	do {
		char c = *p;
		if (r > 0xfff)
			return FALSE;
		r <<= 4;
		if (c >= '0' && c <= '9')
			r += c - '0';
		else if (c >= 'A' && c <= 'F')
			r += c - 'A' + 10;
		else if (c >= 'a' && c <= 'f')
			r += c - 'a' + 10;
		else
			return FALSE;
	} while (*++p != '\0');
	*retval = r;
	return TRUE;
}

static abool parse_dec(int *retval, const char *p, int minval, int maxval)
{
	int r = 0;
	do {
		char c = *p;
		if (c >= '0' && c <= '9')
			r = 10 * r + c - '0';
		else
			return FALSE;
		if (r > maxval)
			return FALSE;
	} while (*++p != '\0');
	if (r < minval)
		return FALSE;
	*retval = r;
	return TRUE;
}

static abool parse_text(char *retval, const char *p)
{
	int i;
	if (*p != '"')
		return FALSE;
	p++;
	if (p[0] == '<' && p[1] == '?' && p[2] == '>' && p[3] == '"')
		return TRUE;
	i = 0;
	while (*p != '"') {
		if (i >= 127)
			return FALSE;
		if (*p == '\0')
			return FALSE;
		retval[i++] = *p++;
	}
	retval[i] = '\0';
	return TRUE;
}

int ASAP_ParseDuration(const char *s)
{
	int r;
	if (*s < '0' || *s > '9')
		return -1;
	r = *s++ - '0';
	if (*s >= '0' && *s <= '9')
		r = 10 * r + *s++ - '0';
	if (*s == ':') {
		s++;
		if (*s < '0' || *s > '5')
			return -1;
		r = 60 * r + (*s++ - '0') * 10;
		if (*s < '0' || *s > '9')
			return -1;
		r += *s++ - '0';
	}
	r *= 1000;
	if (*s != '.')
		return r;
	s++;
	if (*s < '0' || *s > '9')
		return r;
	r += 100 * (*s++ - '0');
	if (*s < '0' || *s > '9')
		return r;
	r += 10 * (*s++ - '0');
	if (*s < '0' || *s > '9')
		return r;
	r += *s - '0';
	return r;
}

static char *two_digits(char *s, int x)
{
	s[0] = '0' + x / 10;
	s[1] = '0' + x % 10;
	return s + 2;
}

void ASAP_DurationToString(char *s, int duration)
{
	if (duration >= 0 && duration < 100 * 60 * 1000) {
		int seconds = duration / 1000;
		s = two_digits(s, seconds / 60);
		*s++ = ':';
		s = two_digits(s, seconds % 60);
		duration %= 1000;
		if (duration != 0) {
			*s++ = '.';
			s = two_digits(s, duration / 10);
			duration %= 10;
			if (duration != 0)
				*s++ = '0' + duration;
		}
	}
	*s = '\0';
}

#endif /* C */

PRIVATE FUNC(abool, parse_sap_header, (
	P(ASAP_ModuleInfo PTR, module_info), P(CONST BYTEARRAY, module), P(int, module_len)))
{
	V(int, module_index) = 0;
	V(abool, sap_signature) = FALSE;
	V(char, type) = '?';
	V(int, duration_index) = 0;
	for (;;) {
#if defined(JAVASCRIPT) || defined(ACTIONSCRIPT)
		V(STRING, tag) = "";
		V(STRING, arg) = null;
#else
		NEW_ARRAY(char, line, 256);
		int len = 0;
		int arg_pos = -1;
#if !defined(JAVA) && !defined(CSHARP)
#define tag line
		char *arg;
#endif
#endif
		if (module_index + 8 >= module_len)
			return FALSE;
		if (UBYTE(module[module_index]) == 0xff)
			break;
		while (module[module_index] != 0x0d) {
#if defined(JAVASCRIPT) || defined(ACTIONSCRIPT)
			V(int, c) = module[module_index++];
			if (arg != null)
				arg += String.fromCharCode(c);
			else if (c == 32)
				arg = "";
			else
				tag += String.fromCharCode(c);
			if (module_index >= module_len)
				return FALSE;
#else
			V(char, c) = CAST(char) module[module_index++];
			if (c == ' ' && arg_pos < 0)
				arg_pos = len;
			line[len++] = c;
			if (module_index >= module_len || len >= CAST(int) sizeof(line) - 1)
				return FALSE;
#endif
		}
		if (++module_index >= module_len || module[module_index++] != 0x0a)
			return FALSE;

#if defined(JAVA) || defined(CSHARP)
		STRING tag;
		STRING arg;
		if (arg_pos >= 0) {
			tag = new STRING(line, 0, arg_pos);
			arg = new STRING(line, arg_pos + 1, len - arg_pos - 1);
		}
		else {
			tag = new STRING(line, 0, len);
			arg = null;
		}
#define SET_TEXT(v)             if (!EQUAL_STRINGS(arg, "\"<?>\"")) SUBSTRING(v, arg, 1, strlen(arg) - 2)
#endif
		
#ifdef JAVA
#define SET_HEX(v)              v = Integer.parseInt(arg, 16)
#define SET_DEC(v, min, max)    v = Integer.parseInt(arg); if (v < min || v > max) return FALSE
#elif defined(CSHARP)
#define SET_HEX(v)              v = int.Parse(arg, System.Globalization.NumberStyles.HexNumber)
#define SET_DEC(v, min, max)    v = int.Parse(arg); if (v < min || v > max) return FALSE
#elif defined(JAVASCRIPT) || defined(ACTIONSCRIPT)
#define SET_HEX(v)              v = parseInt(arg, 16)
#define SET_DEC(v, min, max)    v = parseInt(arg, 10); if (v < min || v > max) return FALSE
#define SET_TEXT(v)             v = arg.substring(1, arg.length - 1)
#else /* C */
		line[len] = '\0';
		if (arg_pos >= 0) {
			arg = line + arg_pos;
			*arg++ = '\0';
		}
		else
			arg = line + len;
#define SET_HEX(v)              if (!parse_hex(&v, arg)) return FALSE
#define SET_DEC(v, min, max)    if (!parse_dec(&v, arg, min, max)) return FALSE
#define SET_TEXT(v)             if (!parse_text(v, arg)) return FALSE
#endif

		if (EQUAL_STRINGS(tag, "SAP"))
			sap_signature = TRUE;
		if (!sap_signature)
			return FALSE;
		if (EQUAL_STRINGS(tag, "AUTHOR")) {
			SET_TEXT(module_info _ author);
		}
		else if (EQUAL_STRINGS(tag, "NAME")) {
			SET_TEXT(module_info _ name);
		}
		else if (EQUAL_STRINGS(tag, "DATE")) {
			SET_TEXT(module_info _ date);
		}
		else if (EQUAL_STRINGS(tag, "SONGS")) {
			SET_DEC(module_info _ songs, 1, ASAP_SONGS_MAX);
		}
		else if (EQUAL_STRINGS(tag, "DEFSONG")) {
			SET_DEC(module_info _ default_song, 0, ASAP_SONGS_MAX - 1);
		}
		else if (EQUAL_STRINGS(tag, "STEREO"))
			module_info _ channels = 2;
		else if (EQUAL_STRINGS(tag, "TIME")) {
			V(int, duration) = ASAP_ParseDuration(arg);
			if (duration < 0 || duration_index >= ASAP_SONGS_MAX)
				return FALSE;
			module_info _ durations[duration_index] = duration;
			module_info _ loops[duration_index] = CONTAINS_STRING(arg, "LOOP");
			duration_index++;
		}
		else if (EQUAL_STRINGS(tag, "TYPE"))
			type = CHARAT(arg, 0);
		else if (EQUAL_STRINGS(tag, "FASTPLAY")) {
			SET_DEC(module_info _ fastplay, 1, 312);
		}
		else if (EQUAL_STRINGS(tag, "MUSIC")) {
			SET_HEX(module_info _ music);
		}
		else if (EQUAL_STRINGS(tag, "INIT")) {
			SET_HEX(module_info _ init);
		}
		else if (EQUAL_STRINGS(tag, "PLAYER")) {
			SET_HEX(module_info _ player);
		}
		else if (EQUAL_STRINGS(tag, "COVOX")) {
			SET_HEX(module_info _ covox_addr);
			if (module_info _ covox_addr != 0xd600)
				return FALSE;
			module_info _ channels = 2;
		}
	}
	if (module_info _ default_song >= module_info _ songs)
		return FALSE;
	switch (type) {
	case 'B':
		if (module_info _ player < 0 || module_info _ init < 0)
			return FALSE;
		module_info _ type = ASAP_TYPE_SAP_B;
		break;
	case 'C':
		if (module_info _ player < 0 || module_info _ music < 0)
			return FALSE;
		module_info _ type = ASAP_TYPE_SAP_C;
		break;
	case 'D':
		if (module_info _ player < 0 || module_info _ init < 0)
			return FALSE;
		module_info _ type = ASAP_TYPE_SAP_D;
		break;
	case 'S':
		if (module_info _ init < 0)
			return FALSE;
		module_info _ type = ASAP_TYPE_SAP_S;
		module_info _ fastplay = 78;
		break;
	default:
		return FALSE;
	}
	if (UBYTE(module[module_index + 1]) != 0xff)
		return FALSE;
	module_info _ header_len = module_index;
	return TRUE;
}

PRIVATE FUNC(abool, parse_sap, (
	P(ASAP_State PTR, ast), P(ASAP_ModuleInfo PTR, module_info),
	P(CONST BYTEARRAY, module), P(int, module_len)))
{
	V(int, module_index);
	if (!parse_sap_header(module_info, module, module_len))
		return FALSE;
	if (ast == NULL)
		return TRUE;
	ZERO_ARRAY(ast _ memory);
	module_index = module_info _ header_len + 2;
	while (module_index + 5 <= module_len) {
		V(int, start_addr) = UWORD(module, module_index);
		V(int, block_len) = UWORD(module, module_index + 2) + 1 - start_addr;
		if (block_len <= 0 || module_index + block_len > module_len)
			return FALSE;
		module_index += 4;
		COPY_ARRAY(ast _ memory, start_addr, module, module_index, block_len);
		module_index += block_len;
		if (module_index == module_len)
			return TRUE;
		if (module_index + 7 <= module_len
		 && UBYTE(module[module_index]) == 0xff && UBYTE(module[module_index + 1]) == 0xff)
			module_index += 2;
	}
	return FALSE;
}

#define ASAP_EXT(c1, c2, c3) ((CHARCODE(c1) + (CHARCODE(c2) << 8) + (CHARCODE(c3) << 16)) | 0x202020)

PRIVATE FUNC(int, get_packed_ext, (P(STRING, filename)))
{
	V(int, i) = strlen(filename);
	V(int, ext) = 0;
	while (--i > 0) {
		V(char, c) = CHARAT(filename, i);
		if (c <= ' ' || c > 'z')
			return 0;
		if (c == '.')
			return ext | 0x202020;
		ext = (ext << 8) + CHARCODE(c);
	}
	return 0;
}

PRIVATE FUNC(abool, is_our_ext, (P(int, ext)))
{
	switch (ext) {
	case ASAP_EXT('S', 'A', 'P'):
#ifndef ASAP_ONLY_SAP
	case ASAP_EXT('C', 'M', 'C'):
	case ASAP_EXT('C', 'M', '3'):
	case ASAP_EXT('C', 'M', 'R'):
	case ASAP_EXT('C', 'M', 'S'):
	case ASAP_EXT('D', 'M', 'C'):
	case ASAP_EXT('D', 'L', 'T'):
	case ASAP_EXT('M', 'P', 'T'):
	case ASAP_EXT('M', 'P', 'D'):
	case ASAP_EXT('R', 'M', 'T'):
	case ASAP_EXT('T', 'M', 'C'):
	case ASAP_EXT('T', 'M', '8'):
	case ASAP_EXT('T', 'M', '2'):
#endif
		return TRUE;
	default:
		return FALSE;
	}
}

FUNC(abool, ASAP_IsOurFile, (P(STRING, filename)))
{
	V(int, ext) = get_packed_ext(filename);
	return is_our_ext(ext);
}

FUNC(abool, ASAP_IsOurExt, (P(STRING, ext)))
{
	return strlen(ext) == 3
		&& is_our_ext(ASAP_EXT(CHARAT(ext, 0), CHARAT(ext, 1), CHARAT(ext, 2)));
}

PRIVATE FUNC(abool, parse_file, (
	P(ASAP_State PTR, ast), P(ASAP_ModuleInfo PTR, module_info),
	P(STRING, filename), P(CONST BYTEARRAY, module), P(int, module_len)))
{
	V(int, i);
	V(int, len) = strlen(filename);
	V(int, basename) = 0;
	V(int, ext) = -1;
	for (i = 0; i < len; i++) {
		V(char, c) = CHARAT(filename, i);
		if (c == '/' || c == '\\') {
			basename = i + 1;
			ext = -1;
		}
		else if (c == '.')
			ext = i;
	}
	if (ext < 0)
		return FALSE;
	EMPTY_STRING(module_info _ author);
	SUBSTRING(module_info _ name, filename, basename, ext - basename);
	EMPTY_STRING(module_info _ date);
	module_info _ channels = 1;
	module_info _ songs = 1;
	module_info _ default_song = 0;
	for (i = 0; i < ASAP_SONGS_MAX; i++) {
		module_info _ durations[i] = -1;
		module_info _ loops[i] = FALSE;
	}
	module_info _ fastplay = 312;
	module_info _ music = -1;
	module_info _ init = -1;
	module_info _ player = -1;
	module_info _ covox_addr = -1;
	switch (get_packed_ext(filename)) {
	case ASAP_EXT('S', 'A', 'P'):
		return parse_sap(ast, module_info, module, module_len);
#ifndef ASAP_ONLY_SAP
	case ASAP_EXT('C', 'M', 'C'):
		return parse_cmc(ast, module_info, module, module_len, ASAP_TYPE_CMC);
	case ASAP_EXT('C', 'M', '3'):
		return parse_cm3(ast, module_info, module, module_len);
	case ASAP_EXT('C', 'M', 'R'):
		return parse_cmc(ast, module_info, module, module_len, ASAP_TYPE_CMR);
	case ASAP_EXT('C', 'M', 'S'):
		return parse_cms(ast, module_info, module, module_len);
	case ASAP_EXT('D', 'M', 'C'):
		module_info _ fastplay = 156;
		return parse_cmc(ast, module_info, module, module_len, ASAP_TYPE_CMC);
	case ASAP_EXT('D', 'L', 'T'):
		return parse_dlt(ast, module_info, module, module_len);
	case ASAP_EXT('M', 'P', 'T'):
		return parse_mpt(ast, module_info, module, module_len);
	case ASAP_EXT('M', 'P', 'D'):
		module_info _ fastplay = 156;
		return parse_mpt(ast, module_info, module, module_len);
	case ASAP_EXT('R', 'M', 'T'):
		return parse_rmt(ast, module_info, module, module_len);
	case ASAP_EXT('T', 'M', 'C'):
	case ASAP_EXT('T', 'M', '8'):
		return parse_tmc(ast, module_info, module, module_len);
	case ASAP_EXT('T', 'M', '2'):
		return parse_tm2(ast, module_info, module, module_len);
#endif
	default:
		return FALSE;
	}
}

FUNC(abool, ASAP_GetModuleInfo, (
	P(ASAP_ModuleInfo PTR, module_info), P(STRING, filename),
	P(CONST BYTEARRAY, module), P(int, module_len)))
{
	return parse_file(NULL, module_info, filename, module, module_len);
}

FUNC(abool, ASAP_Load, (
	P(ASAP_State PTR, ast), P(STRING, filename),
	P(CONST BYTEARRAY, module), P(int, module_len)))
{
	ast _ silence_cycles = 0;
	return parse_file(ast, ADDRESSOF ast _ module_info, filename, module, module_len);
}

FUNC(void, ASAP_DetectSilence, (P(ASAP_State PTR, ast), P(int, seconds)))
{
	ast _ silence_cycles = seconds * ASAP_MAIN_CLOCK;
}

PRIVATE FUNC(void, call_6502, (P(ASAP_State PTR, ast), P(int, addr), P(int, max_scanlines)))
{
	ast _ cpu_pc = addr;
	/* put a CIM at 0xd20a and a return address on stack */
	dPutByte(0xd20a, 0xd2);
	dPutByte(0x01fe, 0x09);
	dPutByte(0x01ff, 0xd2);
	ast _ cpu_s = 0xfd;
	Cpu_RunScanlines(ast, max_scanlines);
}

/* 50 Atari frames for the initialization routine - some SAPs are self-extracting. */
#define SCANLINES_FOR_INIT  (50 * 312)

PRIVATE FUNC(void, call_6502_init, (P(ASAP_State PTR, ast), P(int, addr), P(int, a), P(int, x), P(int, y)))
{
	ast _ cpu_a = a & 0xff;
	ast _ cpu_x = x & 0xff;
	ast _ cpu_y = y & 0xff;
	call_6502(ast, addr, SCANLINES_FOR_INIT);
}

FUNC(void, ASAP_PlaySong, (P(ASAP_State PTR, ast), P(int, song), P(int, duration)))
{
	ast _ current_song = song;
	ast _ current_duration = duration;
	ast _ blocks_played = 0;
	ast _ silence_cycles_counter = ast _ silence_cycles;
	ast _ extra_pokey_mask = ast _ module_info.channels > 1 ? 0x10 : 0;
	ast _ consol = 8;
	ast _ covox[0] = CAST(byte) 0x80;
	ast _ covox[1] = CAST(byte) 0x80;
	ast _ covox[2] = CAST(byte) 0x80;
	ast _ covox[3] = CAST(byte) 0x80;
	PokeySound_Initialize(ast);
	ast _ cycle = 0;
	ast _ cpu_nz = 0;
	ast _ cpu_c = 0;
	ast _ cpu_vdi = 0;
	ast _ scanline_number = 0;
	ast _ next_scanline_cycle = 0;
	ast _ timer1_cycle = NEVER;
	ast _ timer2_cycle = NEVER;
	ast _ timer4_cycle = NEVER;
	ast _ irqst = 0xff;
	switch (ast _ module_info.type) {
	case ASAP_TYPE_SAP_B:
		call_6502_init(ast, ast _ module_info.init, song, 0, 0);
		break;
	case ASAP_TYPE_SAP_C:
#ifndef ASAP_ONLY_SAP
	case ASAP_TYPE_CMC:
	case ASAP_TYPE_CM3:
	case ASAP_TYPE_CMR:
	case ASAP_TYPE_CMS:
#endif
		call_6502_init(ast, ast _ module_info.player + 3, 0x70, ast _ module_info.music, ast _ module_info.music >> 8);
		call_6502_init(ast, ast _ module_info.player + 3, 0x00, song, 0);
		break;
	case ASAP_TYPE_SAP_D:
	case ASAP_TYPE_SAP_S:
		ast _ cpu_a = song;
		ast _ cpu_x = 0x00;
		ast _ cpu_y = 0x00;
		ast _ cpu_s = 0xff;
		ast _ cpu_pc = ast _ module_info.init;
		break;
#ifndef ASAP_ONLY_SAP
	case ASAP_TYPE_DLT:
		call_6502_init(ast, ast _ module_info.player + 0x100, 0x00, 0x00, ast _ module_info.song_pos[song]);
		break;
	case ASAP_TYPE_MPT:
		call_6502_init(ast, ast _ module_info.player, 0x00, ast _ module_info.music >> 8, ast _ module_info.music);
		call_6502_init(ast, ast _ module_info.player, 0x02, ast _ module_info.song_pos[song], 0);
		break;
	case ASAP_TYPE_RMT:
		call_6502_init(ast, ast _ module_info.player, ast _ module_info.song_pos[song], ast _ module_info.music, ast _ module_info.music >> 8);
		break;
	case ASAP_TYPE_TMC:
	case ASAP_TYPE_TM2:
		call_6502_init(ast, ast _ module_info.player, 0x70, ast _ module_info.music >> 8, ast _ module_info.music);
		call_6502_init(ast, ast _ module_info.player, 0x00, song, 0);
		ast _ tmc_per_frame_counter = 1;
		break;
#endif
	}
	ASAP_MutePokeyChannels(ast, 0);
}

FUNC(void, ASAP_MutePokeyChannels, (P(ASAP_State PTR, ast), P(int, mask)))
{
	PokeySound_Mute(ast, ADDRESSOF ast _ base_pokey, mask);
	PokeySound_Mute(ast, ADDRESSOF ast _ extra_pokey, mask >> 4);
}

FUNC(abool, call_6502_player, (P(ASAP_State PTR, ast)))
{
	V(int, s);
	PokeySound_StartFrame(ast);
	switch (ast _ module_info.type) {
	case ASAP_TYPE_SAP_B:
		call_6502(ast, ast _ module_info.player, ast _ module_info.fastplay);
		break;
	case ASAP_TYPE_SAP_C:
#ifndef ASAP_ONLY_SAP
	case ASAP_TYPE_CMC:
	case ASAP_TYPE_CM3:
	case ASAP_TYPE_CMR:
	case ASAP_TYPE_CMS:
#endif
		call_6502(ast, ast _ module_info.player + 6, ast _ module_info.fastplay);
		break;
	case ASAP_TYPE_SAP_D:
		s = ast _ cpu_s;
#define PUSH_ON_6502_STACK(x)  dPutByte(0x100 + s, x); s = (s - 1) & 0xff
#define RETURN_FROM_PLAYER_ADDR  0xd200
		/* save 6502 state on 6502 stack */
		PUSH_ON_6502_STACK(ast _ cpu_pc >> 8);
		PUSH_ON_6502_STACK(ast _ cpu_pc & 0xff);
		PUSH_ON_6502_STACK(((ast _ cpu_nz | (ast _ cpu_nz >> 1)) & 0x80) + ast _ cpu_vdi + \
			((ast _ cpu_nz & 0xff) == 0 ? Z_FLAG : 0) + ast _ cpu_c + 0x20);
		PUSH_ON_6502_STACK(ast _ cpu_a);
		PUSH_ON_6502_STACK(ast _ cpu_x);
		PUSH_ON_6502_STACK(ast _ cpu_y);
		/* RTS will jump to 6502 code that restores the state */
		PUSH_ON_6502_STACK((RETURN_FROM_PLAYER_ADDR - 1) >> 8);
		PUSH_ON_6502_STACK((RETURN_FROM_PLAYER_ADDR - 1) & 0xff);
		ast _ cpu_s = s;
		dPutByte(RETURN_FROM_PLAYER_ADDR, 0x68);     /* PLA */
		dPutByte(RETURN_FROM_PLAYER_ADDR + 1, 0xa8); /* TAY */
		dPutByte(RETURN_FROM_PLAYER_ADDR + 2, 0x68); /* PLA */
		dPutByte(RETURN_FROM_PLAYER_ADDR + 3, 0xaa); /* TAX */
		dPutByte(RETURN_FROM_PLAYER_ADDR + 4, 0x68); /* PLA */
		dPutByte(RETURN_FROM_PLAYER_ADDR + 5, 0x40); /* RTI */
		ast _ cpu_pc = ast _ module_info.player;
		Cpu_RunScanlines(ast, ast _ module_info.fastplay);
		break;
	case ASAP_TYPE_SAP_S:
		Cpu_RunScanlines(ast, ast _ module_info.fastplay);
		{
			V(int, i) = dGetByte(0x45) - 1;
			dPutByte(0x45, i);
			if (i == 0)
				dPutByte(0xb07b, dGetByte(0xb07b) + 1);
		}
		break;
#ifndef ASAP_ONLY_SAP
	case ASAP_TYPE_DLT:
		call_6502(ast, ast _ module_info.player + 0x103, ast _ module_info.fastplay);
		break;
	case ASAP_TYPE_MPT:
	case ASAP_TYPE_RMT:
	case ASAP_TYPE_TM2:
		call_6502(ast, ast _ module_info.player + 3, ast _ module_info.fastplay);
		break;
	case ASAP_TYPE_TMC:
		if (--ast _ tmc_per_frame_counter <= 0) {
			ast _ tmc_per_frame_counter = ast _ tmc_per_frame;
			call_6502(ast, ast _ module_info.player + 3, ast _ module_info.fastplay);
		}
		else
			call_6502(ast, ast _ module_info.player + 6, ast _ module_info.fastplay);
		break;
#endif
	}
	PokeySound_EndFrame(ast, ast _ module_info.fastplay * 114);
	if (ast _ silence_cycles > 0) {
		if (PokeySound_IsSilent(ADDRESSOF ast _ base_pokey)
		 && PokeySound_IsSilent(ADDRESSOF ast _ extra_pokey)) {
			ast _ silence_cycles_counter -= ast _ module_info.fastplay * 114;
			if (ast _ silence_cycles_counter <= 0)
				return FALSE;
		}
		else
			ast _ silence_cycles_counter = ast _ silence_cycles;
	}
	return TRUE;
}

FUNC(int, ASAP_GetPosition, (P(CONST ASAP_State PTR, ast)))
{
	return ast _ blocks_played * 10 / (ASAP_SAMPLE_RATE / 100);
}

FUNC(int, milliseconds_to_blocks, (P(int, milliseconds)))
{
	return milliseconds * (ASAP_SAMPLE_RATE / 100) / 10;
}

#ifndef ACTIONSCRIPT

FUNC(void, ASAP_Seek, (P(ASAP_State PTR, ast), P(int, position)))
{
	V(int, block) = milliseconds_to_blocks(position);
	if (block < ast _ blocks_played)
		ASAP_PlaySong(ast, ast _ current_song, ast _ current_duration);
	while (ast _ blocks_played + ast _ samples - ast _ sample_index < block) {
		ast _ blocks_played += ast _ samples - ast _ sample_index;
		call_6502_player(ast);
	}
	ast _ sample_index += block - ast _ blocks_played;
	ast _ blocks_played = block;
}

PRIVATE FUNC(void, serialize_int, (P(BYTEARRAY, buffer), P(int, offset), P(int, value)))
{
	buffer[offset] = TO_BYTE(value);
	buffer[offset + 1] = TO_BYTE(value >> 8);
	buffer[offset + 2] = TO_BYTE(value >> 16);
	buffer[offset + 3] = TO_BYTE(value >> 24);
}

FUNC(void, ASAP_GetWavHeaderForPart, (
	P(CONST ASAP_State PTR, ast), P(BYTEARRAY, buffer),
	P(ASAP_SampleFormat, format), P(int, blocks)))
{
	V(int, use_16bit) = format != ASAP_FORMAT_U8 ? 1 : 0;
	V(int, block_size) = ast _ module_info.channels << use_16bit;
	V(int, bytes_per_second) = ASAP_SAMPLE_RATE * block_size;
	V(int, remaining_blocks) = milliseconds_to_blocks(ast _ current_duration) - ast _ blocks_played;
	V(int, n_bytes);
	if (blocks > remaining_blocks)
		blocks = remaining_blocks;
	n_bytes = blocks * block_size;
	buffer[0] = CAST(byte) CHARCODE('R');
	buffer[1] = CAST(byte) CHARCODE('I');
	buffer[2] = CAST(byte) CHARCODE('F');
	buffer[3] = CAST(byte) CHARCODE('F');
	serialize_int(buffer, 4, n_bytes + 36);
	buffer[8] = CAST(byte) CHARCODE('W');
	buffer[9] = CAST(byte) CHARCODE('A');
	buffer[10] = CAST(byte) CHARCODE('V');
	buffer[11] = CAST(byte) CHARCODE('E');
	buffer[12] = CAST(byte) CHARCODE('f');
	buffer[13] = CAST(byte) CHARCODE('m');
	buffer[14] = CAST(byte) CHARCODE('t');
	buffer[15] = CAST(byte) CHARCODE(' ');
	buffer[16] = 16;
	buffer[17] = 0;
	buffer[18] = 0;
	buffer[19] = 0;
	buffer[20] = 1;
	buffer[21] = 0;
	buffer[22] = CAST(byte) ast _ module_info.channels;
	buffer[23] = 0;
	serialize_int(buffer, 24, ASAP_SAMPLE_RATE);
	serialize_int(buffer, 28, bytes_per_second);
	buffer[32] = CAST(byte) block_size;
	buffer[33] = 0;
	buffer[34] = CAST(byte) (8 << use_16bit);
	buffer[35] = 0;
	buffer[36] = CAST(byte) CHARCODE('d');
	buffer[37] = CAST(byte) CHARCODE('a');
	buffer[38] = CAST(byte) CHARCODE('t');
	buffer[39] = CAST(byte) CHARCODE('a');
	serialize_int(buffer, 40, n_bytes);
}

FUNC(void, ASAP_GetWavHeader, (
	P(CONST ASAP_State PTR, ast), P(BYTEARRAY, buffer), P(ASAP_SampleFormat, format)))
{
	V(int, remaining_blocks) = milliseconds_to_blocks(ast _ current_duration) - ast _ blocks_played;
	ASAP_GetWavHeaderForPart(ast, buffer, format, remaining_blocks);
}

#endif /* ACTIONSCRIPT */

#if defined(JAVA) || defined(JAVASCRIPT)
#define ASAP_GENERATE_PARS  (P(ASAP_State PTR, ast), P(VOIDPTR, buffer), P(int, buffer_offset), P(int, buffer_len), P(ASAP_SampleFormat, format))
#elif defined(ACTIONSCRIPT)
#define ASAP_GENERATE_PARS  (P(ASAP_State PTR, ast), P(VOIDPTR, buffer), P(int, buffer_blocks), P(ASAP_SampleFormat, format))
#define block_shift  0
#else
#define ASAP_GENERATE_PARS  (P(ASAP_State PTR, ast), P(VOIDPTR, buffer), P(int, buffer_len), P(ASAP_SampleFormat, format))
#endif

FUNC(int, ASAP_Generate, ASAP_GENERATE_PARS)
{
#ifndef ACTIONSCRIPT
	V(int, block_shift);
	V(int, buffer_blocks);
#endif
	V(int, block);
	if (ast _ silence_cycles > 0 && ast _ silence_cycles_counter <= 0)
		return 0;
#ifndef ACTIONSCRIPT
	block_shift = (ast _ module_info.channels - 1) + (format != ASAP_FORMAT_U8 ? 1 : 0);
	buffer_blocks = buffer_len >> block_shift;
#endif
	if (ast _ current_duration > 0) {
		V(int, total_blocks) = milliseconds_to_blocks(ast _ current_duration);
		if (buffer_blocks > total_blocks - ast _ blocks_played)
			buffer_blocks = total_blocks - ast _ blocks_played;
	}
	block = 0;
	do {
		V(int, blocks) = PokeySound_Generate(ast, CAST(BYTEARRAY) buffer,
#if defined(JAVA) || defined(JAVASCRIPT)
			buffer_offset +
#endif
			(block << block_shift), buffer_blocks - block, format);
		ast _ blocks_played += blocks;
		block += blocks;
	} while (block < buffer_blocks && call_6502_player(ast));
	return block << block_shift;
}

#if defined(C) && !defined(ASAP_ONLY_SAP)

abool ASAP_ChangeExt(char *filename, const char *ext)
{
	char *dest = NULL;
	while (*filename != '\0') {
		if (*filename == '/' || *filename == '\\')
			dest = NULL;
		else if (*filename == '.')
			dest = filename + 1;
		filename++;
	}
	if (dest == NULL)
		return FALSE;
	strcpy(dest, ext);
	return TRUE;
}

abool ASAP_CanSetModuleInfo(const char *filename)
{
	int ext = get_packed_ext(filename);
	return ext == ASAP_EXT('S', 'A', 'P');
}

static byte *put_string(byte *dest, const char *str)
{
	while (*str != '\0')
		*dest++ = *str++;
	return dest;
}

static byte *put_dec(byte *dest, int value)
{
	if (value >= 10) {
		dest = put_dec(dest, value / 10);
		value %= 10;
	}
	*dest++ = '0' + value;
	return dest;
}

static byte *put_text_tag(byte *dest, const char *tag, const char *value)
{
	dest = put_string(dest, tag);
	*dest++ = ' ';
	*dest++ = '"';
	if (*value == '\0')
		value = "<?>";
	while (*value != '\0') {
		if (*value < ' ' || *value > 'z' || *value == '"' || *value == '`')
			return NULL;
		*dest++ = *value++;
	}
	*dest++ = '"';
	*dest++ = '\r';
	*dest++ = '\n';
	return dest;
}

static byte *put_hex_tag(byte *dest, const char *tag, int value)
{
	int i;
	if (value < 0)
		return dest;
	dest = put_string(dest, tag);
	*dest++ = ' ';
	for (i = 12; i >= 0; i -= 4) {
		int digit = (value >> i) & 0xf;
		*dest++ = (byte) (digit + (digit < 10 ? '0' : 'A' - 10));
	}
	*dest++ = '\r';
	*dest++ = '\n';
	return dest;
}

static byte *put_dec_tag(byte *dest, const char *tag, int value)
{
	dest = put_string(dest, tag);
	*dest++ = ' ';
	dest = put_dec(dest, value);
	*dest++ = '\r';
	*dest++ = '\n';
	return dest;
}

static byte *start_sap_header(byte *dest, const ASAP_ModuleInfo *module_info)
{
	dest = put_string(dest, "SAP\r\n");
	dest = put_text_tag(dest, "AUTHOR", module_info->author);
	if (dest == NULL)
		return NULL;
	dest = put_text_tag(dest, "NAME", module_info->name);
	if (dest == NULL)
		return NULL;
	dest = put_text_tag(dest, "DATE", module_info->date);
	if (dest == NULL)
		return NULL;
	if (module_info->songs > 1) {
		dest = put_dec_tag(dest, "SONGS", module_info->songs);
		if (module_info->default_song > 0)
			dest = put_dec_tag(dest, "DEFSONG", module_info->default_song);
	}
	if (module_info->channels > 1)
		dest = put_string(dest, "STEREO\r\n");
	return dest;
}

static byte *put_durations(byte *dest, const ASAP_ModuleInfo *module_info)
{
	int song;
	for (song = 0; song < module_info->songs; song++) {
		if (module_info->durations[song] < 0)
			break;
		dest = put_string(dest, "TIME ");
		ASAP_DurationToString((char *) dest, module_info->durations[song]);
		while (*dest != '\0')
			dest++;
		if (module_info->loops[song])
			dest = put_string(dest, " LOOP");
		*dest++ = '\r';
		*dest++ = '\n';
	}
	return dest;
}

static byte *put_sap_header(byte *dest, const ASAP_ModuleInfo *module_info, char type, int music, int init, int player)
{
	dest = start_sap_header(dest, module_info);
	if (dest == NULL)
		return NULL;
	dest = put_string(dest, "TYPE ");
	*dest++ = type;
	*dest++ = '\r';
	*dest++ = '\n';
	if (module_info->fastplay != 312)
		dest = put_dec_tag(dest, "FASTPLAY", module_info->fastplay);
	dest = put_hex_tag(dest, "MUSIC", music);
	dest = put_hex_tag(dest, "INIT", init);
	dest = put_hex_tag(dest, "PLAYER", player);
	dest = put_durations(dest, module_info);
	return dest;
}

int ASAP_SetModuleInfo(const ASAP_ModuleInfo *module_info, const BYTEARRAY module, int module_len, BYTEARRAY out_module)
{
	byte *dest;
	int i;
	if (memcmp(module, "SAP\r\n", 5) != 0)
		return -1;
	dest = start_sap_header(out_module, module_info);
	if (dest == NULL)
		return -1;
	i = 5;
	while (i < module_len && module[i] != 0xff) {
		if (memcmp(module + i, "AUTHOR ", 7) == 0
		 || memcmp(module + i, "NAME ", 5) == 0
		 || memcmp(module + i, "DATE ", 5) == 0
		 || memcmp(module + i, "SONGS ", 6) == 0
		 || memcmp(module + i, "DEFSONG ", 8) == 0
		 || memcmp(module + i, "STEREO", 6) == 0
		 || memcmp(module + i, "TIME ", 5) == 0) {
			while (i < module_len && module[i++] != 0x0a);
		}
		else {
			int b;
			do {
				b = module[i++];
				*dest++ = b;
			} while (i < module_len && b != 0x0a);
		}
	}
	dest = put_durations(dest, module_info);
	module_len -= i;
	memcpy(dest, module + i, module_len);
	dest += module_len;
	return dest - out_module;
}

#define RMT_INIT  0x0c80
#define TM2_INIT  0x1080

const char *ASAP_CanConvert(
	const char *filename, const ASAP_ModuleInfo *module_info,
	const BYTEARRAY module, int module_len)
{
	switch (module_info->type) {
	case ASAP_TYPE_SAP_B:
		if ((module_info->init == 0x3fb || module_info->init == 0x3f9) && module_info->player == 0x503)
			return "dlt";
		if (module_info->init == 0x4f3 || module_info->init == 0xf4f3 || module_info->init == 0x4ef)
			return module_info->fastplay == 156 ? "mpd" : "mpt";
		if (module_info->init == RMT_INIT)
			return "rmt";
		if ((module_info->init == 0x4f5 || module_info->init == 0xf4f5 || module_info->init == 0x4f2)
		 || ((module_info->init == 0x4e7 || module_info->init == 0xf4e7 || module_info->init == 0x4e4) && module_info->fastplay == 156)
		 || ((module_info->init == 0x4e5 || module_info->init == 0xf4e5 || module_info->init == 0x4e2) && (module_info->fastplay == 104 || module_info->fastplay == 78)))
			return "tmc";
		if (module_info->init == TM2_INIT)
			return "tm2";
		break;
	case ASAP_TYPE_SAP_C:
		if (module_info->player == 0x500 || module_info->player == 0xf500) {
			if (module_info->fastplay == 156)
				return "dmc";
			if (module_info->channels > 1)
				return "cms";
			if (module[module_len - 170] == 0x1e)
				return "cmr";
			if (module[module_len - 909] == 0x30)
				return "cm3";
			return "cmc";
		}
		break;
	case ASAP_TYPE_CMC:
	case ASAP_TYPE_CM3:
	case ASAP_TYPE_CMR:
	case ASAP_TYPE_CMS:
	case ASAP_TYPE_DLT:
	case ASAP_TYPE_MPT:
	case ASAP_TYPE_RMT:
	case ASAP_TYPE_TMC:
	case ASAP_TYPE_TM2:
		return "sap";
	default:
		break;
	}
	return NULL;
}

int ASAP_Convert(
	const char *filename, const ASAP_ModuleInfo *module_info,
	const BYTEARRAY module, int module_len, BYTEARRAY out_module)
{
	int out_len;
	byte *dest;
	int addr;
	int player;
	static const int tmc_player[4] = { 3, -9, -10, -10 };
	static const int tmc_init[4] = { -14, -16, -17, -17 };
	switch (module_info->type) {
	case ASAP_TYPE_SAP_B:
	case ASAP_TYPE_SAP_C:
		out_len = UWORD(module, module_info->header_len + 4) - UWORD(module, module_info->header_len + 2) + 7;
		if (out_len < 7 || module_info->header_len + out_len >= module_len)
			return -1;
		memcpy(out_module, module + module_info->header_len, out_len);
		return out_len;
	case ASAP_TYPE_CMC:
	case ASAP_TYPE_CM3:
	case ASAP_TYPE_CMR:
	case ASAP_TYPE_CMS:
		dest = put_sap_header(out_module, module_info, 'C', module_info->music, -1, module_info->player);
		if (dest == NULL)
			return -1;
		memcpy(dest, module, module_len);
		dest[0] = 0xff; /* some modules start with zeros */
		dest[1] = 0xff;
		dest += module_len;
		if (module_info->type == ASAP_TYPE_CM3) {
			memcpy(dest, cm3_obx + 2, sizeof(cm3_obx) - 2);
			dest += sizeof(cm3_obx) - 2;
		}
		else if (module_info->type == ASAP_TYPE_CMS) {
			memcpy(dest, cms_obx + 2, sizeof(cms_obx) - 2);
			dest += sizeof(cms_obx) - 2;
		}
		else {
			memcpy(dest, cmc_obx + 2, sizeof(cmc_obx) - 2);
			if (module_info->type == ASAP_TYPE_CMR)
				memcpy(dest + 4 + CMR_BASS_TABLE_OFFSET, cmr_bass_table, sizeof(cmr_bass_table));
			dest += sizeof(cmc_obx) - 2;
		}
		return dest - out_module;
	case ASAP_TYPE_DLT:
		if (module_info->songs != 1) {
			addr = module_info->player - 7 - module_info->songs;
			dest = put_sap_header(out_module, module_info, 'B', -1, module_info->player - 7, module_info->player + 0x103);
		}
		else {
			addr = module_info->player - 5;
			dest = put_sap_header(out_module, module_info, 'B', -1, addr, module_info->player + 0x103);
		}
		if (dest == NULL)
			return -1;
		memcpy(dest, module, module_len);
		if (module_len == 0x2c06) {
			dest[4] = 0;
			dest[5] = 0x4c;
			dest[0x2c06] = 0;
		}
		dest += 0x2c07;
		*dest++ = (byte) addr;
		*dest++ = (byte) (addr >> 8);
		*dest++ = dlt_obx[4];
		*dest++ = dlt_obx[5];
		if (module_info->songs != 1) {
			memcpy(dest, module_info->song_pos, module_info->songs);
			dest += module_info->songs;
			*dest++ = 0xaa; /* tax */
			*dest++ = 0xbc; /* ldy song2pos,x */
			*dest++ = (byte) addr;
			*dest++ = (byte) (addr >> 8);
		}
		else {
			*dest++ = 0xa0; /* ldy #0 */
			*dest++ = 0;
		}
		*dest++ = 0x4c; /* jmp init */
		*dest++ = (byte) module_info->player;
		*dest++ = (byte) ((module_info->player >> 8) + 1);
		memcpy(dest, dlt_obx + 6, sizeof(dlt_obx) - 6);
		dest += sizeof(dlt_obx) - 6;
		return dest - out_module;
	case ASAP_TYPE_MPT:
		if (module_info->songs != 1) {
			addr = module_info->player - 17 - module_info->songs;
			dest = put_sap_header(out_module, module_info, 'B', -1, module_info->player - 17, module_info->player + 3);
		}
		else {
			addr = module_info->player - 13;
			dest = put_sap_header(out_module, module_info, 'B', -1, addr, module_info->player + 3);
		}
		if (dest == NULL)
			return -1;
		memcpy(dest, module, module_len);
		dest += module_len;
		*dest++ = (byte) addr;
		*dest++ = (byte) (addr >> 8);
		*dest++ = mpt_obx[4];
		*dest++ = mpt_obx[5];
		if (module_info->songs != 1) {
			memcpy(dest, module_info->song_pos, module_info->songs);
			dest += module_info->songs;
			*dest++ = 0x48; /* pha */
		}
		*dest++ = 0xa0; /* ldy #<music */
		*dest++ = (byte) module_info->music;
		*dest++ = 0xa2; /* ldx #>music */
		*dest++ = (byte) (module_info->music >> 8);
		*dest++ = 0xa9; /* lda #0 */
		*dest++ = 0;
		*dest++ = 0x20; /* jsr player */
		*dest++ = (byte) module_info->player;
		*dest++ = (byte) (module_info->player >> 8);
		if (module_info->songs != 1) {
			*dest++ = 0x68; /* pla */
			*dest++ = 0xa8; /* tay */
			*dest++ = 0xbe; /* ldx song2pos,y */
			*dest++ = (byte) addr;
			*dest++ = (byte) (addr >> 8);
		}
		else {
			*dest++ = 0xa2; /* ldx #0 */
			*dest++ = 0;
		}
		*dest++ = 0xa9; /* lda #2 */
		*dest++ = 2;
		memcpy(dest, mpt_obx + 6, sizeof(mpt_obx) - 6);
		dest += sizeof(mpt_obx) - 6;
		return dest - out_module;
	case ASAP_TYPE_RMT:
		dest = put_sap_header(out_module, module_info, 'B', -1, RMT_INIT, module_info->player + 3);
		if (dest == NULL)
			return -1;
		memcpy(dest, module, module_len);
		dest += module_len;
		*dest++ = (byte) RMT_INIT;
		*dest++ = (byte) (RMT_INIT >> 8);
		if (module_info->songs != 1) {
			addr = RMT_INIT + 10 + module_info->songs;
			*dest++ = (byte) addr;
			*dest++ = (byte) (addr >> 8);
			*dest++ = 0xa8; /* tay */
			*dest++ = 0xb9; /* lda song2pos,y */
			*dest++ = (byte) (RMT_INIT + 11);
			*dest++ = (byte) ((RMT_INIT + 11) >> 8);
		}
		else {
			*dest++ = (byte) (RMT_INIT + 8);
			*dest++ = (byte) ((RMT_INIT + 8) >> 8);
			*dest++ = 0xa9; /* lda #0 */
			*dest++ = 0;
		}
		*dest++ = 0xa2; /* ldx #<music */
		*dest++ = (byte) module_info->music;
		*dest++ = 0xa0; /* ldy #>music */
		*dest++ = (byte) (module_info->music >> 8);
		*dest++ = 0x4c; /* jmp player */
		*dest++ = (byte) module_info->player;
		*dest++ = (byte) (module_info->player >> 8);
		if (module_info->songs != 1) {
			memcpy(dest, module_info->song_pos, module_info->songs);
			dest += module_info->songs;
		}
		if (module_info->channels == 1) {
			memcpy(dest, rmt4_obx + 2, sizeof(rmt4_obx) - 2);
			dest += sizeof(rmt4_obx) - 2;
		}
		else {
			memcpy(dest, rmt8_obx + 2, sizeof(rmt8_obx) - 2);
			dest += sizeof(rmt8_obx) - 2;
		}
		return dest - out_module;
	case ASAP_TYPE_TMC:
		player = module_info->player + tmc_player[module[0x25] - 1];
		addr = player + tmc_init[module[0x25] - 1];
		if (module_info->songs != 1)
			addr -= 3;
		dest = put_sap_header(out_module, module_info, 'B', -1, addr, player);
		if (dest == NULL)
			return -1;
		memcpy(dest, module, module_len);
		dest += module_len;
		*dest++ = (byte) addr;
		*dest++ = (byte) (addr >> 8);
		*dest++ = tmc_obx[4];
		*dest++ = tmc_obx[5];
		if (module_info->songs != 1)
			*dest++ = 0x48; /* pha */
		*dest++ = 0xa0; /* ldy #<music */
		*dest++ = (byte) module_info->music;
		*dest++ = 0xa2; /* ldx #>music */
		*dest++ = (byte) (module_info->music >> 8);
		*dest++ = 0xa9; /* lda #$70 */
		*dest++ = 0x70;
		*dest++ = 0x20; /* jsr player */
		*dest++ = (byte) module_info->player;
		*dest++ = (byte) (module_info->player >> 8);
		if (module_info->songs != 1) {
			*dest++ = 0x68; /* pla */
			*dest++ = 0xaa; /* tax */
			*dest++ = 0xa9; /* lda #0 */
			*dest++ = 0;
		}
		else {
			*dest++ = 0xa9; /* lda #$60 */
			*dest++ = 0x60;
		}
		switch (module[0x25]) {
		case 2:
			*dest++ = 0x06; /* asl 0 */
			*dest++ = 0;
			*dest++ = 0x4c; /* jmp player */
			*dest++ = (byte) module_info->player;
			*dest++ = (byte) (module_info->player >> 8);
			*dest++ = 0xa5; /* lda 0 */
			*dest++ = 0;
			*dest++ = 0xe6; /* inc 0 */
			*dest++ = 0;
			*dest++ = 0x4a; /* lsr @ */
			*dest++ = 0x90; /* bcc player+3 */
			*dest++ = 5;
			*dest++ = 0xb0; /* bcs player+6 */
			*dest++ = 6;
			break;
		case 3:
		case 4:
			*dest++ = 0xa0; /* ldy #1 */
			*dest++ = 1;
			*dest++ = 0x84; /* sty 0 */
			*dest++ = 0;
			*dest++ = 0xd0; /* bne player */
			*dest++ = 10;
			*dest++ = 0xc6; /* dec 0 */
			*dest++ = 0;
			*dest++ = 0xd0; /* bne player+6 */
			*dest++ = 12;
			*dest++ = 0xa0; /* ldy #3 */
			*dest++ = module[0x25];
			*dest++ = 0x84; /* sty 0 */
			*dest++ = 0;
			*dest++ = 0xd0; /* bne player+3 */
			*dest++ = 3;
			break;
		default:
			break;
		}
		memcpy(dest, tmc_obx + 6, sizeof(tmc_obx) - 6);
		dest += sizeof(tmc_obx) - 6;
		return dest - out_module;
	case ASAP_TYPE_TM2:
		dest = put_sap_header(out_module, module_info, 'B', -1, TM2_INIT, module_info->player + 3);
		if (dest == NULL)
			return -1;
		memcpy(dest, module, module_len);
		dest += module_len;
		*dest++ = (byte) TM2_INIT;
		*dest++ = (byte) (TM2_INIT >> 8);
		if (module_info->songs != 1) {
			*dest++ = (byte) (TM2_INIT + 16);
			*dest++ = (byte) ((TM2_INIT + 16) >> 8);
			*dest++ = 0x48; /* pha */
		}
		else {
			*dest++ = (byte) (TM2_INIT + 14);
			*dest++ = (byte) ((TM2_INIT + 14) >> 8);
		}
		*dest++ = 0xa0; /* ldy #<music */
		*dest++ = (byte) module_info->music;
		*dest++ = 0xa2; /* ldx #>music */
		*dest++ = (byte) (module_info->music >> 8);
		*dest++ = 0xa9; /* lda #$70 */
		*dest++ = 0x70;
		*dest++ = 0x20; /* jsr player */
		*dest++ = (byte) module_info->player;
		*dest++ = (byte) (module_info->player >> 8);
		if (module_info->songs != 1) {
			*dest++ = 0x68; /* pla */
			*dest++ = 0xaa; /* tax */
			*dest++ = 0xa9; /* lda #0 */
			*dest++ = 0;
		}
		else {
			*dest++ = 0xa9; /* lda #0 */
			*dest++ = 0;
			*dest++ = 0xaa; /* tax */
		}
		*dest++ = 0x4c; /* jmp player */
		*dest++ = (byte) module_info->player;
		*dest++ = (byte) (module_info->player >> 8);
		memcpy(dest, tm2_obx + 2, sizeof(tm2_obx) - 2);
		dest += sizeof(tm2_obx) - 2;
		return dest - out_module;
	default:
		return -1;
	}
}

#endif /* defined(C) && !defined(ASAP_ONLY_SAP) */
