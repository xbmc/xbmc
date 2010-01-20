/*
 * hdhomerun_pkt.c
 *
 * Copyright © 2005-2006 Silicondust Engineering Ltd. <www.silicondust.com>.
 *
 * This library is free software; you can redistribute it and/or 
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see <http://www.gnu.org/licenses/>.
 * 
 * As a special exception to the GNU Lesser General Public License,
 * you may link, statically or dynamically, an application with a
 * publicly distributed version of the Library to produce an
 * executable file containing portions of the Library, and
 * distribute that executable file under terms of your choice,
 * without any of the additional requirements listed in clause 4 of
 * the GNU Lesser General Public License.
 * 
 * By "a publicly distributed version of the Library", we mean
 * either the unmodified Library as distributed by Silicondust, or a
 * modified version of the Library that is distributed under the
 * conditions defined in the GNU Lesser General Public License.
 */

#include "hdhomerun.h"

struct hdhomerun_pkt_t *hdhomerun_pkt_create(void)
{
	struct hdhomerun_pkt_t *pkt = (struct hdhomerun_pkt_t *)calloc(1, sizeof(struct hdhomerun_pkt_t));
	if (!pkt) {
		return NULL;
	}

	hdhomerun_pkt_reset(pkt);

	return pkt;
}

void hdhomerun_pkt_destroy(struct hdhomerun_pkt_t *pkt)
{
	free(pkt);
}

void hdhomerun_pkt_reset(struct hdhomerun_pkt_t *pkt)
{
	pkt->limit = pkt->buffer + sizeof(pkt->buffer) - 4;
	pkt->start = pkt->buffer + 1024;
	pkt->end = pkt->start;
	pkt->pos = pkt->start;
}

static uint32_t hdhomerun_pkt_calc_crc(uint8_t *start, uint8_t *end)
{
	uint8_t *pos = start;
	uint32_t crc = 0xFFFFFFFF;
	while (pos < end) {
		uint8_t x = (uint8_t)(crc) ^ *pos++;
		crc >>= 8;
		if (x & 0x01) crc ^= 0x77073096;
		if (x & 0x02) crc ^= 0xEE0E612C;
		if (x & 0x04) crc ^= 0x076DC419;
		if (x & 0x08) crc ^= 0x0EDB8832;
		if (x & 0x10) crc ^= 0x1DB71064;
		if (x & 0x20) crc ^= 0x3B6E20C8;
		if (x & 0x40) crc ^= 0x76DC4190;
		if (x & 0x80) crc ^= 0xEDB88320;
	}
	return crc ^ 0xFFFFFFFF;
}

uint8_t hdhomerun_pkt_read_u8(struct hdhomerun_pkt_t *pkt)
{
	uint8_t v = *pkt->pos++;
	return v;
}

uint16_t hdhomerun_pkt_read_u16(struct hdhomerun_pkt_t *pkt)
{
	uint16_t v;
	v =  (uint16_t)*pkt->pos++ << 8;
	v |= (uint16_t)*pkt->pos++ << 0;
	return v;
}

uint32_t hdhomerun_pkt_read_u32(struct hdhomerun_pkt_t *pkt)
{
	uint32_t v;
	v =  (uint32_t)*pkt->pos++ << 24;
	v |= (uint32_t)*pkt->pos++ << 16;
	v |= (uint32_t)*pkt->pos++ << 8;
	v |= (uint32_t)*pkt->pos++ << 0;
	return v;
}

size_t hdhomerun_pkt_read_var_length(struct hdhomerun_pkt_t *pkt)
{
	size_t length;
	
	if (pkt->pos + 1 > pkt->end) {
		return (size_t)-1;
	}

	length = (size_t)*pkt->pos++;
	if (length & 0x0080) {
		if (pkt->pos + 1 > pkt->end) {
			return (size_t)-1;
		}

		length &= 0x007F;
		length |= (size_t)*pkt->pos++ << 7;
	}
	
	return length; 
}

uint8_t *hdhomerun_pkt_read_tlv(struct hdhomerun_pkt_t *pkt, uint8_t *ptag, size_t *plength)
{
	if (pkt->pos + 2 > pkt->end) {
		return NULL;
	}
	
	*ptag = hdhomerun_pkt_read_u8(pkt);
	*plength = hdhomerun_pkt_read_var_length(pkt);

	if (pkt->pos + *plength > pkt->end) {
		return NULL;
	}
	
	return pkt->pos + *plength;
}

void hdhomerun_pkt_write_u8(struct hdhomerun_pkt_t *pkt, uint8_t v)
{
	*pkt->pos++ = v;

	if (pkt->pos > pkt->end) {
		pkt->end = pkt->pos;
	}
}

void hdhomerun_pkt_write_u16(struct hdhomerun_pkt_t *pkt, uint16_t v)
{
	*pkt->pos++ = (uint8_t)(v >> 8);
	*pkt->pos++ = (uint8_t)(v >> 0);

	if (pkt->pos > pkt->end) {
		pkt->end = pkt->pos;
	}
}

void hdhomerun_pkt_write_u32(struct hdhomerun_pkt_t *pkt, uint32_t v)
{
	*pkt->pos++ = (uint8_t)(v >> 24);
	*pkt->pos++ = (uint8_t)(v >> 16);
	*pkt->pos++ = (uint8_t)(v >> 8);
	*pkt->pos++ = (uint8_t)(v >> 0);

	if (pkt->pos > pkt->end) {
		pkt->end = pkt->pos;
	}
}

void hdhomerun_pkt_write_var_length(struct hdhomerun_pkt_t *pkt, size_t v)
{
	if (v <= 127) {
		*pkt->pos++ = (uint8_t)v;
	} else {
		*pkt->pos++ = (uint8_t)(v | 0x80);
		*pkt->pos++ = (uint8_t)(v >> 7);
	}

	if (pkt->pos > pkt->end) {
		pkt->end = pkt->pos;
	}
}

void hdhomerun_pkt_write_mem(struct hdhomerun_pkt_t *pkt, const void *mem, size_t length)
{
	memcpy(pkt->pos, mem, length);
	pkt->pos += length;

	if (pkt->pos > pkt->end) {
		pkt->end = pkt->pos;
	}
}

int hdhomerun_pkt_open_frame(struct hdhomerun_pkt_t *pkt, uint16_t *ptype)
{
	pkt->pos = pkt->start;

	if (pkt->pos + 4 > pkt->end) {
		return 0;
	}

	*ptype = hdhomerun_pkt_read_u16(pkt);
	size_t length = hdhomerun_pkt_read_u16(pkt);
	pkt->pos += length;

	if (pkt->pos + 4 > pkt->end) {
		pkt->pos = pkt->start;
		return 0;
	}

	uint32_t calc_crc = hdhomerun_pkt_calc_crc(pkt->start, pkt->pos);

	uint32_t packet_crc;
	packet_crc =  (uint32_t)*pkt->pos++ << 0;
	packet_crc |= (uint32_t)*pkt->pos++ << 8;
	packet_crc |= (uint32_t)*pkt->pos++ << 16;
	packet_crc |= (uint32_t)*pkt->pos++ << 24;
	if (calc_crc != packet_crc) {
		return -1;
	}

	pkt->start += 4;
	pkt->end = pkt->start + length;
	pkt->pos = pkt->start;
	return 1;
}

void hdhomerun_pkt_seal_frame(struct hdhomerun_pkt_t *pkt, uint16_t frame_type)
{
	size_t length = pkt->end - pkt->start;

	pkt->start -= 4;
	pkt->pos = pkt->start;
	hdhomerun_pkt_write_u16(pkt, frame_type);
	hdhomerun_pkt_write_u16(pkt, (uint16_t)length);

	uint32_t crc = hdhomerun_pkt_calc_crc(pkt->start, pkt->end);
	*pkt->end++ = (uint8_t)(crc >> 0);
	*pkt->end++ = (uint8_t)(crc >> 8);
	*pkt->end++ = (uint8_t)(crc >> 16);
	*pkt->end++ = (uint8_t)(crc >> 24);

	pkt->pos = pkt->start;
}
