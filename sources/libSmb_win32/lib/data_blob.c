/* 
   Unix SMB/CIFS implementation.
   Easy management of byte-length data
   Copyright (C) Andrew Tridgell 2001
   Copyright (C) Andrew Bartlett 2001
   
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
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "includes.h"

/*******************************************************************
 Free() a data blob.
*******************************************************************/

static void free_data_blob(DATA_BLOB *d)
{
	if ((d) && (d->free)) {
		SAFE_FREE(d->data);
	}
}

/*******************************************************************
 Construct a data blob, must be freed with data_blob_free().
 You can pass NULL for p and get a blank data blob
*******************************************************************/

DATA_BLOB data_blob(const void *p, size_t length)
{
	DATA_BLOB ret;

	if (!length) {
		ZERO_STRUCT(ret);
		return ret;
	}

	if (p) {
		ret.data = smb_xmemdup(p, length);
	} else {
		ret.data = SMB_XMALLOC_ARRAY(unsigned char, length);
	}
	ret.length = length;
	ret.free = free_data_blob;
	return ret;
}

/*******************************************************************
 Construct a data blob, using supplied TALLOC_CTX.
*******************************************************************/

DATA_BLOB data_blob_talloc(TALLOC_CTX *mem_ctx, const void *p, size_t length)
{
	DATA_BLOB ret;

	if (!length) {
		ZERO_STRUCT(ret);
		return ret;
	}

	if (p) {
		ret.data = TALLOC_MEMDUP(mem_ctx, p, length);
		if (ret.data == NULL)
			smb_panic("data_blob_talloc: talloc_memdup failed.\n");
	} else {
		ret.data = TALLOC(mem_ctx, length);
		if (ret.data == NULL)
			smb_panic("data_blob_talloc: talloc failed.\n");
	}

	ret.length = length;
	ret.free = NULL;
	return ret;
}

/*******************************************************************
 Free a data blob.
*******************************************************************/

void data_blob_free(DATA_BLOB *d)
{
	if (d) {
		if (d->free) {
			(d->free)(d);
		}
		d->length = 0;
	}
}

/*******************************************************************
 Clear a DATA_BLOB's contents
*******************************************************************/

static void data_blob_clear(DATA_BLOB *d)
{
	if (d->data) {
		memset(d->data, 0, d->length);
	}
}

/*******************************************************************
 Free a data blob and clear its contents
*******************************************************************/

void data_blob_clear_free(DATA_BLOB *d)
{
	data_blob_clear(d);
	data_blob_free(d);
}
