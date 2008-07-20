/*
 * hdhomerun_pkt.c
 *
 * Copyright © 2005-2006 Silicondust Engineering Ltd. <www.silicondust.com>.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "hdhomerun_os.h"
#include "hdhomerun_pkt.h"

uint8_t hdhomerun_read_u8(uint8_t **pptr)
{
	uint8_t *ptr = *pptr;
	uint8_t v = *ptr++;
	*pptr = ptr;
	return v;
}

uint16_t hdhomerun_read_u16(uint8_t **pptr)
{
	uint8_t *ptr = *pptr;
	uint16_t v;
	v =  (uint16_t)*ptr++ << 8;
	v |= (uint16_t)*ptr++ << 0;
	*pptr = ptr;
	return v;
}

uint32_t hdhomerun_read_u32(uint8_t **pptr)
{
	uint8_t *ptr = *pptr;
	uint32_t v;
	v =  (uint32_t)*ptr++ << 24;
	v |= (uint32_t)*ptr++ << 16;
	v |= (uint32_t)*ptr++ << 8;
	v |= (uint32_t)*ptr++ << 0;
	*pptr = ptr;
	return v;
}

size_t hdhomerun_read_var_length(uint8_t **pptr, uint8_t *end)
{
	uint8_t *ptr = *pptr;
	size_t length;
	
	if (ptr + 1 > end) {
		return -1;
	}

	length = (size_t)*ptr++;
	if (length & 0x0080) {
		if (ptr + 1 > end) {
			return -1;
		}

		length &= 0x007F;
		length |= (size_t)*ptr++ << 7;
	}
	
	*pptr = ptr;
	return length; 
}

int hdhomerun_read_tlv(uint8_t **pptr, uint8_t *end, uint8_t *ptag, size_t *plength, uint8_t **pvalue)
{
	if (end - *pptr < 2) {
		return -1;
	}
	
	*ptag = hdhomerun_read_u8(pptr);
	*plength = hdhomerun_read_var_length(pptr, end);
	*pvalue = *pptr;
	
	if ((size_t)(end - *pptr) < *plength) {
		return -1;
	}
	
	*pptr += *plength;
	return 0;
}

void hdhomerun_write_u8(uint8_t **pptr, uint8_t v)
{
	uint8_t *ptr = *pptr;
	*ptr++ = v;
	*pptr = ptr;
}

void hdhomerun_write_u16(uint8_t **pptr, uint16_t v)
{
	uint8_t *ptr = *pptr;
	*ptr++ = (uint8_t)(v >> 8);
	*ptr++ = (uint8_t)(v >> 0);
	*pptr = ptr;
}

void hdhomerun_write_u32(uint8_t **pptr, uint32_t v)
{
	uint8_t *ptr = *pptr;
	*ptr++ = (uint8_t)(v >> 24);
	*ptr++ = (uint8_t)(v >> 16);
	*ptr++ = (uint8_t)(v >> 8);
	*ptr++ = (uint8_t)(v >> 0);
	*pptr = ptr;
}

void hdhomerun_write_var_length(uint8_t **pptr, size_t v)
{
	uint8_t *ptr = *pptr;
	if (v <= 127) {
		*ptr++ = (uint8_t)v;
	} else {
		*ptr++ = (uint8_t)(v | 0x80);
		*ptr++ = (uint8_t)(v >> 7);
	}
	*pptr = ptr;
}

void hdhomerun_write_mem(uint8_t **pptr, const void *mem, size_t length)
{
	uint8_t *ptr = *pptr;
	memcpy(ptr, mem, length);
	ptr += length;
	*pptr = ptr;
}

static uint32_t hdhomerun_calc_crc(uint8_t *start, uint8_t *end)
{
	uint8_t *ptr = start;
	uint32_t crc = 0xFFFFFFFF;
	while (ptr < end) {
		uint8_t x = (uint8_t)(crc) ^ *ptr++;
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

static int hdhomerun_check_crc(uint8_t *start, uint8_t *end)
{
	if (end - start < 8) {
		return -1;
	}
	uint8_t *ptr = end -= 4;
	uint32_t actual_crc = hdhomerun_calc_crc(start, ptr);
	uint32_t packet_crc;
	packet_crc =  (uint32_t)*ptr++ << 0;
	packet_crc |= (uint32_t)*ptr++ << 8;
	packet_crc |= (uint32_t)*ptr++ << 16;
	packet_crc |= (uint32_t)*ptr++ << 24;
	if (actual_crc != packet_crc) {
		return -1;
	}
	return 0;
}

void hdhomerun_write_header_length(uint8_t *buffer, uint8_t *end)
{
	uint8_t *ptr = buffer + 2;
	size_t length = end - buffer - 4;
	hdhomerun_write_u16(&ptr, (uint16_t)length);
}

void hdhomerun_write_crc(uint8_t **pptr, uint8_t *start)
{
	uint8_t *ptr = *pptr;
	uint32_t crc = hdhomerun_calc_crc(start, ptr);
	*ptr++ = (uint8_t)(crc >> 0);
	*ptr++ = (uint8_t)(crc >> 8);
	*ptr++ = (uint8_t)(crc >> 16);
	*ptr++ = (uint8_t)(crc >> 24);
	*pptr = ptr;
}

void hdhomerun_write_discover_request(uint8_t **pptr, uint32_t device_type, uint32_t device_id)
{
	uint8_t *start = *pptr;
	hdhomerun_write_u16(pptr, HDHOMERUN_TYPE_DISCOVER_REQ);
	hdhomerun_write_u16(pptr, 0);

	hdhomerun_write_u8(pptr, HDHOMERUN_TAG_DEVICE_TYPE);
	hdhomerun_write_var_length(pptr, 4);
	hdhomerun_write_u32(pptr, device_type);
	hdhomerun_write_u8(pptr, HDHOMERUN_TAG_DEVICE_ID);
	hdhomerun_write_var_length(pptr, 4);
	hdhomerun_write_u32(pptr, device_id);

	hdhomerun_write_header_length(start, *pptr);
	hdhomerun_write_crc(pptr, start);
}

void hdhomerun_write_get_set_request(uint8_t **pptr, const char *name, const char *value)
{
	uint8_t *start = *pptr;
	hdhomerun_write_u16(pptr, HDHOMERUN_TYPE_GETSET_REQ);
	hdhomerun_write_u16(pptr, 0);

	int name_len = (int)strlen(name) + 1;
	hdhomerun_write_u8(pptr, HDHOMERUN_TAG_GETSET_NAME);
	hdhomerun_write_var_length(pptr, name_len);
	hdhomerun_write_mem(pptr, (void *)name, name_len);

	if (value) {
		int value_len = (int)strlen(value) + 1;
		hdhomerun_write_u8(pptr, HDHOMERUN_TAG_GETSET_VALUE);
		hdhomerun_write_var_length(pptr, value_len);
		hdhomerun_write_mem(pptr, (void *)value, value_len);
	}

	hdhomerun_write_header_length(start, *pptr);
	hdhomerun_write_crc(pptr, start);
}

void hdhomerun_write_upgrade_request(uint8_t **pptr, uint32_t sequence, void *data, size_t length)
{
	uint8_t *start = *pptr;
	hdhomerun_write_u16(pptr, HDHOMERUN_TYPE_UPGRADE_REQ);
	hdhomerun_write_u16(pptr, 0);

	hdhomerun_write_u32(pptr, sequence);
	if (length > 0) {
		hdhomerun_write_mem(pptr, data, length);
	}

	hdhomerun_write_header_length(start, *pptr);
	hdhomerun_write_crc(pptr, start);
}

size_t hdhomerun_peek_packet_length(uint8_t *ptr)
{
	ptr += 2;
	return (size_t)hdhomerun_read_u16(&ptr) + 8;
}

int hdhomerun_process_packet(uint8_t **pptr, uint8_t **pend)
{
	if (hdhomerun_check_crc(*pptr, *pend) < 0) {
		return -1;
	}
	*pend -= 4;
	
	uint16_t type = hdhomerun_read_u16(pptr);
	uint16_t length = hdhomerun_read_u16(pptr);
	if ((*pend - *pptr) < length) {
		return -1;
	}
	*pend = *pptr + length;
	return (int)type;
}

