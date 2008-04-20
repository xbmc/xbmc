/* 
 *  Unix SMB/CIFS implementation.
 *  UUID server routines
 *  Copyright (C) Theodore Ts'o               1996, 1997,
 *  Copyright (C) Jim McDonough <jmcd@us.ibm.com> 2002, 2003
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *  
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "includes.h"

/*
 * Offset between 15-Oct-1582 and 1-Jan-70
 */
#define TIME_OFFSET_HIGH 0x01B21DD2
#define TIME_OFFSET_LOW  0x13814000

void smb_uuid_pack(const struct uuid uu, UUID_FLAT *ptr)
{
	SIVAL(ptr->info, 0, uu.time_low);
	SSVAL(ptr->info, 4, uu.time_mid);
	SSVAL(ptr->info, 6, uu.time_hi_and_version);
	memcpy(ptr->info+8, uu.clock_seq, 2);
	memcpy(ptr->info+10, uu.node, 6);
}

void smb_uuid_unpack(const UUID_FLAT in, struct uuid *uu)
{
	uu->time_low = IVAL(in.info, 0);
	uu->time_mid = SVAL(in.info, 4);
	uu->time_hi_and_version = SVAL(in.info, 6);
	memcpy(uu->clock_seq, in.info+8, 2);
	memcpy(uu->node, in.info+10, 6);
}

struct uuid smb_uuid_unpack_static(const UUID_FLAT in)
{
	static struct uuid uu;

	smb_uuid_unpack(in, &uu);
	return uu;
}

void smb_uuid_generate_random(struct uuid *uu)
{
	UUID_FLAT tmp;

	generate_random_buffer(tmp.info, sizeof(tmp.info));
	smb_uuid_unpack(tmp, uu);

	uu->clock_seq[0] = (uu->clock_seq[0] & 0x3F) | 0x80;
	uu->time_hi_and_version = (uu->time_hi_and_version & 0x0FFF) | 0x4000;
}

char *smb_uuid_to_string(const struct uuid uu)
{
	char *out;

	asprintf(&out, "%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
		 uu.time_low, uu.time_mid, uu.time_hi_and_version,
		 uu.clock_seq[0], uu.clock_seq[1],
		 uu.node[0], uu.node[1], uu.node[2], 
		 uu.node[3], uu.node[4], uu.node[5]);

	return out;
}

const char *smb_uuid_string_static(const struct uuid uu)
{
	static char out[37];

	slprintf(out, sizeof(out), 
		 "%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
		 uu.time_low, uu.time_mid, uu.time_hi_and_version,
		 uu.clock_seq[0], uu.clock_seq[1],
		 uu.node[0], uu.node[1], uu.node[2], 
		 uu.node[3], uu.node[4], uu.node[5]);
	return out;
}

BOOL smb_string_to_uuid(const char *in, struct uuid* uu)
{
	BOOL ret = False;
	const char *ptr = in;
	char *end = (char *)in;
	int i;
	unsigned v1, v2;

	if (!in || !uu) goto out;

	uu->time_low = strtoul(ptr, &end, 16);
	if ((end - ptr) != 8 || *end != '-') goto out;
	ptr = (end + 1);

	uu->time_mid = strtoul(ptr, &end, 16);
	if ((end - ptr) != 4 || *end != '-') goto out;
	ptr = (end + 1);

	uu->time_hi_and_version = strtoul(ptr, &end, 16);
	if ((end - ptr) != 4 || *end != '-') goto out;
	ptr = (end + 1);

	if (sscanf(ptr, "%02x%02x", &v1, &v2) != 2) {
		goto out;
	}
	uu->clock_seq[0] = v1;
	uu->clock_seq[1] = v2;
	ptr += 4;

	if (*ptr != '-') goto out;
	ptr++;

	for (i = 0; i < 6; i++) {
		if (sscanf(ptr, "%02x", &v1) != 1) {
			goto out;
		}
		uu->node[i] = v1;
		ptr += 2;
	}

	ret = True;
out:
        return ret;
}
