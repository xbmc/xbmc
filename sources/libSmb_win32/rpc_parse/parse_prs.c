/* 
   Unix SMB/CIFS implementation.
   Samba memory buffer functions
   Copyright (C) Andrew Tridgell              1992-1997
   Copyright (C) Luke Kenneth Casson Leighton 1996-1997
   Copyright (C) Jeremy Allison               1999
   Copyright (C) Andrew Bartlett              2003.
   
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

#undef DBGC_CLASS
#define DBGC_CLASS DBGC_RPC_PARSE

/**
 * Dump a prs to a file: from the current location through to the end.
 **/
void prs_dump(char *name, int v, prs_struct *ps)
{
	prs_dump_region(name, v, ps, ps->data_offset, ps->buffer_size);
}

/**
 * Dump from the start of the prs to the current location.
 **/
void prs_dump_before(char *name, int v, prs_struct *ps)
{
	prs_dump_region(name, v, ps, 0, ps->data_offset);
}

/**
 * Dump everything from the start of the prs up to the current location.
 **/
void prs_dump_region(char *name, int v, prs_struct *ps,
		     int from_off, int to_off)
{
	int fd, i;
	pstring fname;
	ssize_t sz;
	if (DEBUGLEVEL < 50) return;
	for (i=1;i<100;i++) {
		if (v != -1) {
			slprintf(fname,sizeof(fname)-1, "/tmp/%s_%d.%d.prs", name, v, i);
		} else {
			slprintf(fname,sizeof(fname)-1, "/tmp/%s.%d.prs", name, i);
		}
		fd = open(fname, O_WRONLY|O_CREAT|O_EXCL, 0644);
		if (fd != -1 || errno != EEXIST) break;
	}
	if (fd != -1) {
		sz = write(fd, ps->data_p + from_off, to_off - from_off);
		i = close(fd);
		if ( (sz != to_off-from_off) || (i != 0) ) {
			DEBUG(0,("Error writing/closing %s: %ld!=%ld %d\n", fname, (unsigned long)sz, (unsigned long)to_off-from_off, i ));
		} else {
			DEBUG(0,("created %s\n", fname));
		}
	}
}

/*******************************************************************
 Debug output for parsing info

 XXXX side-effect of this function is to increase the debug depth XXXX.

********************************************************************/

void prs_debug(prs_struct *ps, int depth, const char *desc, const char *fn_name)
{
	DEBUG(5+depth, ("%s%06x %s %s\n", tab_depth(depth), ps->data_offset, fn_name, desc));
}

/**
 * Initialise an expandable parse structure.
 *
 * @param size Initial buffer size.  If >0, a new buffer will be
 * created with malloc().
 *
 * @return False if allocation fails, otherwise True.
 **/

BOOL prs_init(prs_struct *ps, uint32 size, TALLOC_CTX *ctx, BOOL io)
{
	ZERO_STRUCTP(ps);
	ps->io = io;
	ps->bigendian_data = RPC_LITTLE_ENDIAN;
	ps->align = RPC_PARSE_ALIGN;
	ps->is_dynamic = False;
	ps->data_offset = 0;
	ps->buffer_size = 0;
	ps->data_p = NULL;
	ps->mem_ctx = ctx;

	if (size != 0) {
		ps->buffer_size = size;
		if((ps->data_p = (char *)SMB_MALLOC((size_t)size)) == NULL) {
			DEBUG(0,("prs_init: malloc fail for %u bytes.\n", (unsigned int)size));
			return False;
		}
		memset(ps->data_p, '\0', (size_t)size);
		ps->is_dynamic = True; /* We own this memory. */
	} else if (MARSHALLING(ps)) {
		/* If size is zero and we're marshalling we should allocate memory on demand. */
		ps->is_dynamic = True;
	}

	return True;
}

/*******************************************************************
 Delete the memory in a parse structure - if we own it.
 ********************************************************************/

void prs_mem_free(prs_struct *ps)
{
	if(ps->is_dynamic)
		SAFE_FREE(ps->data_p);
	ps->is_dynamic = False;
	ps->buffer_size = 0;
	ps->data_offset = 0;
}

/*******************************************************************
 Clear the memory in a parse structure.
 ********************************************************************/

void prs_mem_clear(prs_struct *ps)
{
	if (ps->buffer_size)
		memset(ps->data_p, '\0', (size_t)ps->buffer_size);
}

/*******************************************************************
 Allocate memory when unmarshalling... Always zero clears.
 ********************************************************************/

#if defined(PARANOID_MALLOC_CHECKER)
char *prs_alloc_mem_(prs_struct *ps, size_t size, unsigned int count)
#else
char *prs_alloc_mem(prs_struct *ps, size_t size, unsigned int count)
#endif
{
	char *ret = NULL;

	if (size) {
		/* We can't call the type-safe version here. */
		ret = _talloc_zero_array(ps->mem_ctx, size, count, "parse_prs");
	}
	return ret;
}

/*******************************************************************
 Return the current talloc context we're using.
 ********************************************************************/

TALLOC_CTX *prs_get_mem_context(prs_struct *ps)
{
	return ps->mem_ctx;
}

/*******************************************************************
 Hand some already allocated memory to a prs_struct.
 ********************************************************************/

void prs_give_memory(prs_struct *ps, char *buf, uint32 size, BOOL is_dynamic)
{
	ps->is_dynamic = is_dynamic;
	ps->data_p = buf;
	ps->buffer_size = size;
}

/*******************************************************************
 Take some memory back from a prs_struct.
 ********************************************************************/

char *prs_take_memory(prs_struct *ps, uint32 *psize)
{
	char *ret = ps->data_p;
	if(psize)
		*psize = ps->buffer_size;
	ps->is_dynamic = False;
	prs_mem_free(ps);
	return ret;
}

/*******************************************************************
 Set a prs_struct to exactly a given size. Will grow or tuncate if neccessary.
 ********************************************************************/

BOOL prs_set_buffer_size(prs_struct *ps, uint32 newsize)
{
	if (newsize > ps->buffer_size)
		return prs_force_grow(ps, newsize - ps->buffer_size);

	if (newsize < ps->buffer_size) {
		ps->buffer_size = newsize;

		/* newsize == 0 acts as a free and set pointer to NULL */
		if (newsize == 0) {
			SAFE_FREE(ps->data_p);
		} else {
			ps->data_p = SMB_REALLOC(ps->data_p, newsize);

			if (ps->data_p == NULL) {
				DEBUG(0,("prs_set_buffer_size: Realloc failure for size %u.\n",
					(unsigned int)newsize));
				DEBUG(0,("prs_set_buffer_size: Reason %s\n",strerror(errno)));
				return False;
			}
		}
	}

	return True;
}

/*******************************************************************
 Attempt, if needed, to grow a data buffer.
 Also depends on the data stream mode (io).
 ********************************************************************/

BOOL prs_grow(prs_struct *ps, uint32 extra_space)
{
	uint32 new_size;

	ps->grow_size = MAX(ps->grow_size, ps->data_offset + extra_space);

	if(ps->data_offset + extra_space <= ps->buffer_size)
		return True;

	/*
	 * We cannot grow the buffer if we're not reading
	 * into the prs_struct, or if we don't own the memory.
	 */

	if(UNMARSHALLING(ps) || !ps->is_dynamic) {
		DEBUG(0,("prs_grow: Buffer overflow - unable to expand buffer by %u bytes.\n",
				(unsigned int)extra_space));
		return False;
	}
	
	/*
	 * Decide how much extra space we really need.
	 */

	extra_space -= (ps->buffer_size - ps->data_offset);
	if(ps->buffer_size == 0) {
		/*
		 * Ensure we have at least a PDU's length, or extra_space, whichever
		 * is greater.
		 */

		new_size = MAX(RPC_MAX_PDU_FRAG_LEN,extra_space);

		if((ps->data_p = SMB_MALLOC(new_size)) == NULL) {
			DEBUG(0,("prs_grow: Malloc failure for size %u.\n", (unsigned int)new_size));
			return False;
		}
		memset(ps->data_p, '\0', (size_t)new_size );
	} else {
		/*
		 * If the current buffer size is bigger than the space needed, just 
		 * double it, else add extra_space.
		 */
		new_size = MAX(ps->buffer_size*2, ps->buffer_size + extra_space);		

		if ((ps->data_p = SMB_REALLOC(ps->data_p, new_size)) == NULL) {
			DEBUG(0,("prs_grow: Realloc failure for size %u.\n",
				(unsigned int)new_size));
			return False;
		}

		memset(&ps->data_p[ps->buffer_size], '\0', (size_t)(new_size - ps->buffer_size));
	}
	ps->buffer_size = new_size;

	return True;
}

/*******************************************************************
 Attempt to force a data buffer to grow by len bytes.
 This is only used when appending more data onto a prs_struct
 when reading an rpc reply, before unmarshalling it.
 ********************************************************************/

BOOL prs_force_grow(prs_struct *ps, uint32 extra_space)
{
	uint32 new_size = ps->buffer_size + extra_space;

	if(!UNMARSHALLING(ps) || !ps->is_dynamic) {
		DEBUG(0,("prs_force_grow: Buffer overflow - unable to expand buffer by %u bytes.\n",
				(unsigned int)extra_space));
		return False;
	}

	if((ps->data_p = SMB_REALLOC(ps->data_p, new_size)) == NULL) {
		DEBUG(0,("prs_force_grow: Realloc failure for size %u.\n",
			(unsigned int)new_size));
		return False;
	}

	memset(&ps->data_p[ps->buffer_size], '\0', (size_t)(new_size - ps->buffer_size));

	ps->buffer_size = new_size;

	return True;
}

/*******************************************************************
 Get the data pointer (external interface).
********************************************************************/

char *prs_data_p(prs_struct *ps)
{
	return ps->data_p;
}

/*******************************************************************
 Get the current data size (external interface).
 ********************************************************************/

uint32 prs_data_size(prs_struct *ps)
{
	return ps->buffer_size;
}

/*******************************************************************
 Fetch the current offset (external interface).
 ********************************************************************/

uint32 prs_offset(prs_struct *ps)
{
	return ps->data_offset;
}

/*******************************************************************
 Set the current offset (external interface).
 ********************************************************************/

BOOL prs_set_offset(prs_struct *ps, uint32 offset)
{
	if(offset <= ps->data_offset) {
		ps->data_offset = offset;
		return True;
	}

	if(!prs_grow(ps, offset - ps->data_offset))
		return False;

	ps->data_offset = offset;
	return True;
}

/*******************************************************************
 Append the data from one parse_struct into another.
 ********************************************************************/

BOOL prs_append_prs_data(prs_struct *dst, prs_struct *src)
{
	if (prs_offset(src) == 0)
		return True;

	if(!prs_grow(dst, prs_offset(src)))
		return False;

	memcpy(&dst->data_p[dst->data_offset], src->data_p, (size_t)prs_offset(src));
	dst->data_offset += prs_offset(src);

	return True;
}

/*******************************************************************
 Append some data from one parse_struct into another.
 ********************************************************************/

BOOL prs_append_some_prs_data(prs_struct *dst, prs_struct *src, int32 start, uint32 len)
{	
	if (len == 0)
		return True;

	if(!prs_grow(dst, len))
		return False;
	
	memcpy(&dst->data_p[dst->data_offset], src->data_p + start, (size_t)len);
	dst->data_offset += len;

	return True;
}

/*******************************************************************
 Append the data from a buffer into a parse_struct.
 ********************************************************************/

BOOL prs_copy_data_in(prs_struct *dst, const char *src, uint32 len)
{
	if (len == 0)
		return True;

	if(!prs_grow(dst, len))
		return False;

	memcpy(&dst->data_p[dst->data_offset], src, (size_t)len);
	dst->data_offset += len;

	return True;
}

/*******************************************************************
 Copy some data from a parse_struct into a buffer.
 ********************************************************************/

BOOL prs_copy_data_out(char *dst, prs_struct *src, uint32 len)
{
	if (len == 0)
		return True;

	if(!prs_mem_get(src, len))
		return False;

	memcpy(dst, &src->data_p[src->data_offset], (size_t)len);
	src->data_offset += len;

	return True;
}

/*******************************************************************
 Copy all the data from a parse_struct into a buffer.
 ********************************************************************/

BOOL prs_copy_all_data_out(char *dst, prs_struct *src)
{
	uint32 len = prs_offset(src);

	if (!len)
		return True;

	prs_set_offset(src, 0);
	return prs_copy_data_out(dst, src, len);
}

/*******************************************************************
 Set the data as X-endian (external interface).
 ********************************************************************/

void prs_set_endian_data(prs_struct *ps, BOOL endian)
{
	ps->bigendian_data = endian;
}

/*******************************************************************
 Align a the data_len to a multiple of align bytes - filling with
 zeros.
 ********************************************************************/

BOOL prs_align(prs_struct *ps)
{
	uint32 mod = ps->data_offset & (ps->align-1);

	if (ps->align != 0 && mod != 0) {
		uint32 extra_space = (ps->align - mod);
		if(!prs_grow(ps, extra_space))
			return False;
		memset(&ps->data_p[ps->data_offset], '\0', (size_t)extra_space);
		ps->data_offset += extra_space;
	}

	return True;
}

/******************************************************************
 Align on a 2 byte boundary
 *****************************************************************/
 
BOOL prs_align_uint16(prs_struct *ps)
{
	BOOL ret;
	uint8 old_align = ps->align;

	ps->align = 2;
	ret = prs_align(ps);
	ps->align = old_align;
	
	return ret;
}

/******************************************************************
 Align on a 8 byte boundary
 *****************************************************************/
 
BOOL prs_align_uint64(prs_struct *ps)
{
	BOOL ret;
	uint8 old_align = ps->align;

	ps->align = 8;
	ret = prs_align(ps);
	ps->align = old_align;
	
	return ret;
}

/******************************************************************
 Align on a specific byte boundary
 *****************************************************************/
 
BOOL prs_align_custom(prs_struct *ps, uint8 boundary)
{
	BOOL ret;
	uint8 old_align = ps->align;

	ps->align = boundary;
	ret = prs_align(ps);
	ps->align = old_align;
	
	return ret;
}



/*******************************************************************
 Align only if required (for the unistr2 string mainly)
 ********************************************************************/

BOOL prs_align_needed(prs_struct *ps, uint32 needed)
{
	if (needed==0)
		return True;
	else
		return prs_align(ps);
}

/*******************************************************************
 Ensure we can read/write to a given offset.
 ********************************************************************/

char *prs_mem_get(prs_struct *ps, uint32 extra_size)
{
	if(UNMARSHALLING(ps)) {
		/*
		 * If reading, ensure that we can read the requested size item.
		 */
		if (ps->data_offset + extra_size > ps->buffer_size) {
			DEBUG(0,("prs_mem_get: reading data of size %u would overrun "
				"buffer by %u bytes.\n",
				(unsigned int)extra_size,
				(unsigned int)(ps->data_offset + extra_size - ps->buffer_size) ));
			return NULL;
		}
	} else {
		/*
		 * Writing - grow the buffer if needed.
		 */
		if(!prs_grow(ps, extra_size))
			return NULL;
	}
	return &ps->data_p[ps->data_offset];
}

/*******************************************************************
 Change the struct type.
 ********************************************************************/

void prs_switch_type(prs_struct *ps, BOOL io)
{
	if ((ps->io ^ io) == True)
		ps->io=io;
}

/*******************************************************************
 Force a prs_struct to be dynamic even when it's size is 0.
 ********************************************************************/

void prs_force_dynamic(prs_struct *ps)
{
	ps->is_dynamic=True;
}

/*******************************************************************
 Associate a session key with a parse struct.
 ********************************************************************/

void prs_set_session_key(prs_struct *ps, const char sess_key[16])
{
	ps->sess_key = sess_key;
}

/*******************************************************************
 Stream a uint8.
 ********************************************************************/

BOOL prs_uint8(const char *name, prs_struct *ps, int depth, uint8 *data8)
{
	char *q = prs_mem_get(ps, 1);
	if (q == NULL)
		return False;

	if (UNMARSHALLING(ps))
		*data8 = CVAL(q,0);
	else
		SCVAL(q,0,*data8);

	DEBUG(5,("%s%04x %s: %02x\n", tab_depth(depth), ps->data_offset, name, *data8));

	ps->data_offset += 1;

	return True;
}

/*******************************************************************
 Stream a uint16* (allocate memory if unmarshalling)
 ********************************************************************/

BOOL prs_pointer( const char *name, prs_struct *ps, int depth, 
                 void **data, size_t data_size,
                 BOOL(*prs_fn)(const char*, prs_struct*, int, void*) )
{
	uint32 data_p;

	/* output f000baaa to stream if the pointer is non-zero. */

	data_p = *data ? 0xf000baaa : 0;

	if ( !prs_uint32("ptr", ps, depth, &data_p ))
		return False;

	/* we're done if there is no data */

	if ( !data_p )
		return True;

	if (UNMARSHALLING(ps)) {
		if ( !(*data = PRS_ALLOC_MEM_VOID(ps, data_size)) )
			return False;
	}

	return prs_fn(name, ps, depth, *data);
}


/*******************************************************************
 Stream a uint16.
 ********************************************************************/

BOOL prs_uint16(const char *name, prs_struct *ps, int depth, uint16 *data16)
{
	char *q = prs_mem_get(ps, sizeof(uint16));
	if (q == NULL)
		return False;

	if (UNMARSHALLING(ps)) {
		if (ps->bigendian_data)
			*data16 = RSVAL(q,0);
		else
			*data16 = SVAL(q,0);
	} else {
		if (ps->bigendian_data)
			RSSVAL(q,0,*data16);
		else
			SSVAL(q,0,*data16);
	}

	DEBUG(5,("%s%04x %s: %04x\n", tab_depth(depth), ps->data_offset, name, *data16));

	ps->data_offset += sizeof(uint16);

	return True;
}

/*******************************************************************
 Stream a uint32.
 ********************************************************************/

BOOL prs_uint32(const char *name, prs_struct *ps, int depth, uint32 *data32)
{
	char *q = prs_mem_get(ps, sizeof(uint32));
	if (q == NULL)
		return False;

	if (UNMARSHALLING(ps)) {
		if (ps->bigendian_data)
			*data32 = RIVAL(q,0);
		else
			*data32 = IVAL(q,0);
	} else {
		if (ps->bigendian_data)
			RSIVAL(q,0,*data32);
		else
			SIVAL(q,0,*data32);
	}

	DEBUG(5,("%s%04x %s: %08x\n", tab_depth(depth), ps->data_offset, name, *data32));

	ps->data_offset += sizeof(uint32);

	return True;
}

/*******************************************************************
 Stream an int32.
 ********************************************************************/

BOOL prs_int32(const char *name, prs_struct *ps, int depth, int32 *data32)
{
	char *q = prs_mem_get(ps, sizeof(int32));
	if (q == NULL)
		return False;

	if (UNMARSHALLING(ps)) {
		if (ps->bigendian_data)
			*data32 = RIVALS(q,0);
		else
			*data32 = IVALS(q,0);
	} else {
		if (ps->bigendian_data)
			RSIVALS(q,0,*data32);
		else
			SIVALS(q,0,*data32);
	}

	DEBUG(5,("%s%04x %s: %08x\n", tab_depth(depth), ps->data_offset, name, *data32));

	ps->data_offset += sizeof(int32);

	return True;
}

/*******************************************************************
 Stream a NTSTATUS
 ********************************************************************/

BOOL prs_ntstatus(const char *name, prs_struct *ps, int depth, NTSTATUS *status)
{
	char *q = prs_mem_get(ps, sizeof(uint32));
	if (q == NULL)
		return False;

	if (UNMARSHALLING(ps)) {
		if (ps->bigendian_data)
			*status = NT_STATUS(RIVAL(q,0));
		else
			*status = NT_STATUS(IVAL(q,0));
	} else {
		if (ps->bigendian_data)
			RSIVAL(q,0,NT_STATUS_V(*status));
		else
			SIVAL(q,0,NT_STATUS_V(*status));
	}

	DEBUG(5,("%s%04x %s: %s\n", tab_depth(depth), ps->data_offset, name, 
		 nt_errstr(*status)));

	ps->data_offset += sizeof(uint32);

	return True;
}

/*******************************************************************
 Stream a DCE error code
 ********************************************************************/

BOOL prs_dcerpc_status(const char *name, prs_struct *ps, int depth, NTSTATUS *status)
{
	char *q = prs_mem_get(ps, sizeof(uint32));
	if (q == NULL)
		return False;

	if (UNMARSHALLING(ps)) {
		if (ps->bigendian_data)
			*status = NT_STATUS(RIVAL(q,0));
		else
			*status = NT_STATUS(IVAL(q,0));
	} else {
		if (ps->bigendian_data)
			RSIVAL(q,0,NT_STATUS_V(*status));
		else
			SIVAL(q,0,NT_STATUS_V(*status));
	}

	DEBUG(5,("%s%04x %s: %s\n", tab_depth(depth), ps->data_offset, name, 
		 dcerpc_errstr(NT_STATUS_V(*status))));

	ps->data_offset += sizeof(uint32);

	return True;
}


/*******************************************************************
 Stream a WERROR
 ********************************************************************/

BOOL prs_werror(const char *name, prs_struct *ps, int depth, WERROR *status)
{
	char *q = prs_mem_get(ps, sizeof(uint32));
	if (q == NULL)
		return False;

	if (UNMARSHALLING(ps)) {
		if (ps->bigendian_data)
			*status = W_ERROR(RIVAL(q,0));
		else
			*status = W_ERROR(IVAL(q,0));
	} else {
		if (ps->bigendian_data)
			RSIVAL(q,0,W_ERROR_V(*status));
		else
			SIVAL(q,0,W_ERROR_V(*status));
	}

	DEBUG(5,("%s%04x %s: %s\n", tab_depth(depth), ps->data_offset, name, 
		 dos_errstr(*status)));

	ps->data_offset += sizeof(uint32);

	return True;
}


/******************************************************************
 Stream an array of uint8s. Length is number of uint8s.
 ********************************************************************/

BOOL prs_uint8s(BOOL charmode, const char *name, prs_struct *ps, int depth, uint8 *data8s, int len)
{
	int i;
	char *q = prs_mem_get(ps, len);
	if (q == NULL)
		return False;

	if (UNMARSHALLING(ps)) {
		for (i = 0; i < len; i++)
			data8s[i] = CVAL(q,i);
	} else {
		for (i = 0; i < len; i++)
			SCVAL(q, i, data8s[i]);
	}

	DEBUG(5,("%s%04x %s: ", tab_depth(depth), ps->data_offset ,name));
	if (charmode)
		print_asc(5, (unsigned char*)data8s, len);
	else {
		for (i = 0; i < len; i++)
			DEBUG(5,("%02x ", data8s[i]));
	}
	DEBUG(5,("\n"));

	ps->data_offset += len;

	return True;
}

/******************************************************************
 Stream an array of uint16s. Length is number of uint16s.
 ********************************************************************/

BOOL prs_uint16s(BOOL charmode, const char *name, prs_struct *ps, int depth, uint16 *data16s, int len)
{
	int i;
	char *q = prs_mem_get(ps, len * sizeof(uint16));
	if (q == NULL)
		return False;

	if (UNMARSHALLING(ps)) {
		if (ps->bigendian_data) {
			for (i = 0; i < len; i++)
				data16s[i] = RSVAL(q, 2*i);
		} else {
			for (i = 0; i < len; i++)
				data16s[i] = SVAL(q, 2*i);
		}
	} else {
		if (ps->bigendian_data) {
			for (i = 0; i < len; i++)
				RSSVAL(q, 2*i, data16s[i]);
		} else {
			for (i = 0; i < len; i++)
				SSVAL(q, 2*i, data16s[i]);
		}
	}

	DEBUG(5,("%s%04x %s: ", tab_depth(depth), ps->data_offset, name));
	if (charmode)
		print_asc(5, (unsigned char*)data16s, 2*len);
	else {
		for (i = 0; i < len; i++)
			DEBUG(5,("%04x ", data16s[i]));
	}
	DEBUG(5,("\n"));

	ps->data_offset += (len * sizeof(uint16));

	return True;
}

/******************************************************************
 Start using a function for streaming unicode chars. If unmarshalling,
 output must be little-endian, if marshalling, input must be little-endian.
 ********************************************************************/

static void dbg_rw_punival(BOOL charmode, const char *name, int depth, prs_struct *ps,
							char *in_buf, char *out_buf, int len)
{
	int i;

	if (UNMARSHALLING(ps)) {
		if (ps->bigendian_data) {
			for (i = 0; i < len; i++)
				SSVAL(out_buf,2*i,RSVAL(in_buf, 2*i));
		} else {
			for (i = 0; i < len; i++)
				SSVAL(out_buf, 2*i, SVAL(in_buf, 2*i));
		}
	} else {
		if (ps->bigendian_data) {
			for (i = 0; i < len; i++)
				RSSVAL(in_buf, 2*i, SVAL(out_buf,2*i));
		} else {
			for (i = 0; i < len; i++)
				SSVAL(in_buf, 2*i, SVAL(out_buf,2*i));
		}
	}

	DEBUG(5,("%s%04x %s: ", tab_depth(depth), ps->data_offset, name));
	if (charmode)
		print_asc(5, (unsigned char*)out_buf, 2*len);
	else {
		for (i = 0; i < len; i++)
			DEBUG(5,("%04x ", out_buf[i]));
	}
	DEBUG(5,("\n"));
}

/******************************************************************
 Stream a unistr. Always little endian.
 ********************************************************************/

BOOL prs_uint16uni(BOOL charmode, const char *name, prs_struct *ps, int depth, uint16 *data16s, int len)
{
	char *q = prs_mem_get(ps, len * sizeof(uint16));
	if (q == NULL)
		return False;

	dbg_rw_punival(charmode, name, depth, ps, q, (char *)data16s, len);
	ps->data_offset += (len * sizeof(uint16));

	return True;
}

/******************************************************************
 Stream an array of uint32s. Length is number of uint32s.
 ********************************************************************/

BOOL prs_uint32s(BOOL charmode, const char *name, prs_struct *ps, int depth, uint32 *data32s, int len)
{
	int i;
	char *q = prs_mem_get(ps, len * sizeof(uint32));
	if (q == NULL)
		return False;

	if (UNMARSHALLING(ps)) {
		if (ps->bigendian_data) {
			for (i = 0; i < len; i++)
				data32s[i] = RIVAL(q, 4*i);
		} else {
			for (i = 0; i < len; i++)
				data32s[i] = IVAL(q, 4*i);
		}
	} else {
		if (ps->bigendian_data) {
			for (i = 0; i < len; i++)
				RSIVAL(q, 4*i, data32s[i]);
		} else {
			for (i = 0; i < len; i++)
				SIVAL(q, 4*i, data32s[i]);
		}
	}

	DEBUG(5,("%s%04x %s: ", tab_depth(depth), ps->data_offset, name));
	if (charmode)
		print_asc(5, (unsigned char*)data32s, 4*len);
	else {
		for (i = 0; i < len; i++)
			DEBUG(5,("%08x ", data32s[i]));
	}
	DEBUG(5,("\n"));

	ps->data_offset += (len * sizeof(uint32));

	return True;
}

/******************************************************************
 Stream an array of unicode string, length/buffer specified separately,
 in uint16 chars. The unicode string is already in little-endian format.
 ********************************************************************/

BOOL prs_buffer5(BOOL charmode, const char *name, prs_struct *ps, int depth, BUFFER5 *str)
{
	char *p;
	char *q = prs_mem_get(ps, str->buf_len * sizeof(uint16));
	if (q == NULL)
		return False;

	if (UNMARSHALLING(ps)) {
		str->buffer = PRS_ALLOC_MEM(ps,uint16,str->buf_len);
		if (str->buffer == NULL)
			return False;
	}

	/* If the string is empty, we don't have anything to stream */
	if (str->buf_len==0)
		return True;

	p = (char *)str->buffer;

	dbg_rw_punival(charmode, name, depth, ps, q, p, str->buf_len);
	
	ps->data_offset += (str->buf_len * sizeof(uint16));

	return True;
}

/******************************************************************
 Stream a "not" unicode string, length/buffer specified separately,
 in byte chars. String is in little-endian format.
 ********************************************************************/

BOOL prs_regval_buffer(BOOL charmode, const char *name, prs_struct *ps, int depth, REGVAL_BUFFER *buf)
{
	char *p;
	char *q = prs_mem_get(ps, buf->buf_len);
	if (q == NULL)
		return False;

	if (UNMARSHALLING(ps)) {
		if (buf->buf_len > buf->buf_max_len) {
			return False;
		}
		if ( buf->buf_max_len ) {
			buf->buffer = PRS_ALLOC_MEM(ps, uint16, buf->buf_max_len);
			if ( buf->buffer == NULL )
				return False;
		}
	}

	p = (char *)buf->buffer;

	dbg_rw_punival(charmode, name, depth, ps, q, p, buf->buf_len/2);
	ps->data_offset += buf->buf_len;

	return True;
}

/******************************************************************
 Stream a string, length/buffer specified separately,
 in uint8 chars.
 ********************************************************************/

BOOL prs_string2(BOOL charmode, const char *name, prs_struct *ps, int depth, STRING2 *str)
{
	unsigned int i;
	char *q = prs_mem_get(ps, str->str_str_len);
	if (q == NULL)
		return False;

	if (UNMARSHALLING(ps)) {
		if (str->str_str_len > str->str_max_len) {
			return False;
		}
		str->buffer = PRS_ALLOC_MEM(ps,unsigned char, str->str_max_len);
		if (str->buffer == NULL)
			return False;
	}

	if (UNMARSHALLING(ps)) {
		for (i = 0; i < str->str_str_len; i++)
			str->buffer[i] = CVAL(q,i);
	} else {
		for (i = 0; i < str->str_str_len; i++)
			SCVAL(q, i, str->buffer[i]);
	}

	DEBUG(5,("%s%04x %s: ", tab_depth(depth), ps->data_offset, name));
	if (charmode)
		print_asc(5, (unsigned char*)str->buffer, str->str_str_len);
	else {
		for (i = 0; i < str->str_str_len; i++)
			DEBUG(5,("%02x ", str->buffer[i]));
	}
	DEBUG(5,("\n"));

	ps->data_offset += str->str_str_len;

	return True;
}

/******************************************************************
 Stream a unicode string, length/buffer specified separately,
 in uint16 chars. The unicode string is already in little-endian format.
 ********************************************************************/

BOOL prs_unistr2(BOOL charmode, const char *name, prs_struct *ps, int depth, UNISTR2 *str)
{
	char *p;
	char *q = prs_mem_get(ps, str->uni_str_len * sizeof(uint16));
	if (q == NULL)
		return False;

	/* If the string is empty, we don't have anything to stream */
	if (str->uni_str_len==0)
		return True;

	if (UNMARSHALLING(ps)) {
		if (str->uni_str_len > str->uni_max_len) {
			return False;
		}
		str->buffer = PRS_ALLOC_MEM(ps,uint16,str->uni_max_len);
		if (str->buffer == NULL)
			return False;
	}

	p = (char *)str->buffer;

	dbg_rw_punival(charmode, name, depth, ps, q, p, str->uni_str_len);
	
	ps->data_offset += (str->uni_str_len * sizeof(uint16));

	return True;
}

/******************************************************************
 Stream a unicode string, length/buffer specified separately,
 in uint16 chars. The unicode string is already in little-endian format.
 ********************************************************************/

BOOL prs_unistr3(BOOL charmode, const char *name, UNISTR3 *str, prs_struct *ps, int depth)
{
	char *p;
	char *q = prs_mem_get(ps, str->uni_str_len * sizeof(uint16));
	if (q == NULL)
		return False;

	if (UNMARSHALLING(ps)) {
		str->str.buffer = PRS_ALLOC_MEM(ps,uint16,str->uni_str_len);
		if (str->str.buffer == NULL)
			return False;
	}

	p = (char *)str->str.buffer;

	dbg_rw_punival(charmode, name, depth, ps, q, p, str->uni_str_len);
	ps->data_offset += (str->uni_str_len * sizeof(uint16));

	return True;
}

/*******************************************************************
 Stream a unicode  null-terminated string. As the string is already
 in little-endian format then do it as a stream of bytes.
 ********************************************************************/

BOOL prs_unistr(const char *name, prs_struct *ps, int depth, UNISTR *str)
{
	unsigned int len = 0;
	unsigned char *p = (unsigned char *)str->buffer;
	uint8 *start;
	char *q;
	uint32 max_len;
	uint16* ptr;

	if (MARSHALLING(ps)) {

		for(len = 0; str->buffer[len] != 0; len++)
			;

		q = prs_mem_get(ps, (len+1)*2);
		if (q == NULL)
			return False;

		start = (uint8*)q;

		for(len = 0; str->buffer[len] != 0; len++) {
			if(ps->bigendian_data) {
				/* swap bytes - p is little endian, q is big endian. */
				q[0] = (char)p[1];
				q[1] = (char)p[0];
				p += 2;
				q += 2;
			} 
			else 
			{
				q[0] = (char)p[0];
				q[1] = (char)p[1];
				p += 2;
				q += 2;
			}
		}

		/*
		 * even if the string is 'empty' (only an \0 char)
		 * at this point the leading \0 hasn't been parsed.
		 * so parse it now
		 */

		q[0] = 0;
		q[1] = 0;
		q += 2;

		len++;

		DEBUG(5,("%s%04x %s: ", tab_depth(depth), ps->data_offset, name));
		print_asc(5, (unsigned char*)start, 2*len);	
		DEBUG(5, ("\n"));
	}
	else { /* unmarshalling */
	
		uint32 alloc_len = 0;
		q = ps->data_p + prs_offset(ps);

		/*
		 * Work out how much space we need and talloc it.
		 */
		max_len = (ps->buffer_size - ps->data_offset)/sizeof(uint16);

		/* the test of the value of *ptr helps to catch the circumstance
		   where we have an emtpty (non-existent) string in the buffer */
		for ( ptr = (uint16 *)q; *ptr++ && (alloc_len <= max_len); alloc_len++)
			/* do nothing */ 
			;

		if (alloc_len < max_len)
			alloc_len += 1;

		/* should we allocate anything at all? */
		str->buffer = PRS_ALLOC_MEM(ps,uint16,alloc_len);
		if ((str->buffer == NULL) && (alloc_len > 0))
			return False;

		p = (unsigned char *)str->buffer;

		len = 0;
		/* the (len < alloc_len) test is to prevent us from overwriting
		   memory that is not ours...if we get that far, we have a non-null
		   terminated string in the buffer and have messed up somewhere */
		while ((len < alloc_len) && (*(uint16 *)q != 0)) {
			if(ps->bigendian_data) 
			{
				/* swap bytes - q is big endian, p is little endian. */
				p[0] = (unsigned char)q[1];
				p[1] = (unsigned char)q[0];
				p += 2;
				q += 2;
			} else {

				p[0] = (unsigned char)q[0];
				p[1] = (unsigned char)q[1];
				p += 2;
				q += 2;
			}

			len++;
		} 
		if (len < alloc_len) {
			/* NULL terminate the UNISTR */
			str->buffer[len++] = '\0';
		}

		DEBUG(5,("%s%04x %s: ", tab_depth(depth), ps->data_offset, name));
		print_asc(5, (unsigned char*)str->buffer, 2*len);	
		DEBUG(5, ("\n"));
	}

	/* set the offset in the prs_struct; 'len' points to the
	   terminiating NULL in the UNISTR so we need to go one more
	   uint16 */
	ps->data_offset += (len)*2;
	
	return True;
}


/*******************************************************************
 Stream a null-terminated string.  len is strlen, and therefore does
 not include the null-termination character.
 ********************************************************************/

BOOL prs_string(const char *name, prs_struct *ps, int depth, char *str, int max_buf_size)
{
	char *q;
	int i;
	int len;

	if (UNMARSHALLING(ps))
		len = strlen(&ps->data_p[ps->data_offset]);
	else
		len = strlen(str);

	len = MIN(len, (max_buf_size-1));

	q = prs_mem_get(ps, len+1);
	if (q == NULL)
		return False;

	for(i = 0; i < len; i++) {
		if (UNMARSHALLING(ps))
			str[i] = q[i];
		else
			q[i] = str[i];
	}

	/* The terminating null. */
	str[i] = '\0';

	if (MARSHALLING(ps)) {
		q[i] = '\0';
	}

	ps->data_offset += len+1;

	dump_data(5+depth, q, len);

	return True;
}

BOOL prs_string_alloc(const char *name, prs_struct *ps, int depth, const char **str)
{
	size_t len;
	char *tmp_str;

	if (UNMARSHALLING(ps)) {
		len = strlen(&ps->data_p[ps->data_offset]);
	} else {
		len = strlen(*str);
	}

	tmp_str = PRS_ALLOC_MEM(ps, char, len+1);

	if (tmp_str == NULL) {
		return False;
	}

	if (MARSHALLING(ps)) {
		strncpy(tmp_str, *str, len);
	}

	if (!prs_string(name, ps, depth, tmp_str, len+1)) {
		return False;
	}

	*str = tmp_str;
	return True;
}

/*******************************************************************
 prs_uint16 wrapper. Call this and it sets up a pointer to where the
 uint16 should be stored, or gets the size if reading.
 ********************************************************************/

BOOL prs_uint16_pre(const char *name, prs_struct *ps, int depth, uint16 *data16, uint32 *offset)
{
	*offset = ps->data_offset;
	if (UNMARSHALLING(ps)) {
		/* reading. */
		return prs_uint16(name, ps, depth, data16);
	} else {
		char *q = prs_mem_get(ps, sizeof(uint16));
		if(q ==NULL)
			return False;
		ps->data_offset += sizeof(uint16);
	}
	return True;
}

/*******************************************************************
 prs_uint16 wrapper.  call this and it retrospectively stores the size.
 does nothing on reading, as that is already handled by ...._pre()
 ********************************************************************/

BOOL prs_uint16_post(const char *name, prs_struct *ps, int depth, uint16 *data16,
				uint32 ptr_uint16, uint32 start_offset)
{
	if (MARSHALLING(ps)) {
		/* 
		 * Writing - temporarily move the offset pointer.
		 */
		uint16 data_size = ps->data_offset - start_offset;
		uint32 old_offset = ps->data_offset;

		ps->data_offset = ptr_uint16;
		if(!prs_uint16(name, ps, depth, &data_size)) {
			ps->data_offset = old_offset;
			return False;
		}
		ps->data_offset = old_offset;
	} else {
		ps->data_offset = start_offset + (uint32)(*data16);
	}
	return True;
}

/*******************************************************************
 prs_uint32 wrapper. Call this and it sets up a pointer to where the
 uint32 should be stored, or gets the size if reading.
 ********************************************************************/

BOOL prs_uint32_pre(const char *name, prs_struct *ps, int depth, uint32 *data32, uint32 *offset)
{
	*offset = ps->data_offset;
	if (UNMARSHALLING(ps) && (data32 != NULL)) {
		/* reading. */
		return prs_uint32(name, ps, depth, data32);
	} else {
		ps->data_offset += sizeof(uint32);
	}
	return True;
}

/*******************************************************************
 prs_uint32 wrapper.  call this and it retrospectively stores the size.
 does nothing on reading, as that is already handled by ...._pre()
 ********************************************************************/

BOOL prs_uint32_post(const char *name, prs_struct *ps, int depth, uint32 *data32,
				uint32 ptr_uint32, uint32 data_size)
{
	if (MARSHALLING(ps)) {
		/* 
		 * Writing - temporarily move the offset pointer.
		 */
		uint32 old_offset = ps->data_offset;
		ps->data_offset = ptr_uint32;
		if(!prs_uint32(name, ps, depth, &data_size)) {
			ps->data_offset = old_offset;
			return False;
		}
		ps->data_offset = old_offset;
	}
	return True;
}

/* useful function to store a structure in rpc wire format */
int tdb_prs_store(TDB_CONTEXT *tdb, char *keystr, prs_struct *ps)
{
    TDB_DATA kbuf, dbuf;
    kbuf.dptr = keystr;
    kbuf.dsize = strlen(keystr)+1;
    dbuf.dptr = ps->data_p;
    dbuf.dsize = prs_offset(ps);
    return tdb_store(tdb, kbuf, dbuf, TDB_REPLACE);
}

/* useful function to fetch a structure into rpc wire format */
int tdb_prs_fetch(TDB_CONTEXT *tdb, char *keystr, prs_struct *ps, TALLOC_CTX *mem_ctx)
{
    TDB_DATA kbuf, dbuf;
    kbuf.dptr = keystr;
    kbuf.dsize = strlen(keystr)+1;

    prs_init(ps, 0, mem_ctx, UNMARSHALL);

    dbuf = tdb_fetch(tdb, kbuf);
    if (!dbuf.dptr)
	    return -1;

    prs_give_memory(ps, dbuf.dptr, dbuf.dsize, True);

    return 0;
} 

/*******************************************************************
 hash a stream.
 ********************************************************************/

BOOL prs_hash1(prs_struct *ps, uint32 offset, int len)
{
	char *q;

	q = ps->data_p;
        q = &q[offset];

#ifdef DEBUG_PASSWORD
	DEBUG(100, ("prs_hash1\n"));
	dump_data(100, ps->sess_key, 16);
	dump_data(100, q, len);
#endif
	SamOEMhash((uchar *) q, (const unsigned char *)ps->sess_key, len);

#ifdef DEBUG_PASSWORD
	dump_data(100, q, len);
#endif

	return True;
}

/*******************************************************************
 Create a digest over the entire packet (including the data), and 
 MD5 it with the session key.
 ********************************************************************/

static void schannel_digest(struct schannel_auth_struct *a,
			  enum pipe_auth_level auth_level,
			  RPC_AUTH_SCHANNEL_CHK * verf,
			  char *data, size_t data_len,
			  uchar digest_final[16]) 
{
	uchar whole_packet_digest[16];
	static uchar zeros[4];
	struct MD5Context ctx3;
	
	/* verfiy the signature on the packet by MD5 over various bits */
	MD5Init(&ctx3);
	/* use our sequence number, which ensures the packet is not
	   out of order */
	MD5Update(&ctx3, zeros, sizeof(zeros));
	MD5Update(&ctx3, verf->sig, sizeof(verf->sig));
	if (auth_level == PIPE_AUTH_LEVEL_PRIVACY) {
		MD5Update(&ctx3, verf->confounder, sizeof(verf->confounder));
	}
	MD5Update(&ctx3, (const unsigned char *)data, data_len);
	MD5Final(whole_packet_digest, &ctx3);
	dump_data_pw("whole_packet_digest:\n", whole_packet_digest, sizeof(whole_packet_digest));
	
	/* MD5 this result and the session key, to prove that
	   only a valid client could had produced this */
	hmac_md5(a->sess_key, whole_packet_digest, sizeof(whole_packet_digest), digest_final);
}

/*******************************************************************
 Calculate the key with which to encode the data payload 
 ********************************************************************/

static void schannel_get_sealing_key(struct schannel_auth_struct *a,
				   RPC_AUTH_SCHANNEL_CHK *verf,
				   uchar sealing_key[16]) 
{
	static uchar zeros[4];
	uchar digest2[16];
	uchar sess_kf0[16];
	int i;

	for (i = 0; i < sizeof(sess_kf0); i++) {
		sess_kf0[i] = a->sess_key[i] ^ 0xf0;
	}
	
	dump_data_pw("sess_kf0:\n", sess_kf0, sizeof(sess_kf0));
	
	/* MD5 of sess_kf0 and 4 zero bytes */
	hmac_md5(sess_kf0, zeros, 0x4, digest2);
	dump_data_pw("digest2:\n", digest2, sizeof(digest2));
	
	/* MD5 of the above result, plus 8 bytes of sequence number */
	hmac_md5(digest2, verf->seq_num, sizeof(verf->seq_num), sealing_key);
	dump_data_pw("sealing_key:\n", sealing_key, 16);
}

/*******************************************************************
 Encode or Decode the sequence number (which is symmetric)
 ********************************************************************/

static void schannel_deal_with_seq_num(struct schannel_auth_struct *a,
				     RPC_AUTH_SCHANNEL_CHK *verf)
{
	static uchar zeros[4];
	uchar sequence_key[16];
	uchar digest1[16];

	hmac_md5(a->sess_key, zeros, sizeof(zeros), digest1);
	dump_data_pw("(sequence key) digest1:\n", digest1, sizeof(digest1));

	hmac_md5(digest1, verf->packet_digest, 8, sequence_key);

	dump_data_pw("sequence_key:\n", sequence_key, sizeof(sequence_key));

	dump_data_pw("seq_num (before):\n", verf->seq_num, sizeof(verf->seq_num));
	SamOEMhash(verf->seq_num, sequence_key, 8);
	dump_data_pw("seq_num (after):\n", verf->seq_num, sizeof(verf->seq_num));
}

/*******************************************************************
creates an RPC_AUTH_SCHANNEL_CHK structure.
********************************************************************/

static BOOL init_rpc_auth_schannel_chk(RPC_AUTH_SCHANNEL_CHK * chk,
			      const uchar sig[8],
			      const uchar packet_digest[8],
			      const uchar seq_num[8], const uchar confounder[8])
{
	if (chk == NULL)
		return False;

	memcpy(chk->sig, sig, sizeof(chk->sig));
	memcpy(chk->packet_digest, packet_digest, sizeof(chk->packet_digest));
	memcpy(chk->seq_num, seq_num, sizeof(chk->seq_num));
	memcpy(chk->confounder, confounder, sizeof(chk->confounder));

	return True;
}

/*******************************************************************
 Encode a blob of data using the schannel alogrithm, also produceing
 a checksum over the original data.  We currently only support
 signing and sealing togeather - the signing-only code is close, but not
 quite compatible with what MS does.
 ********************************************************************/

void schannel_encode(struct schannel_auth_struct *a, enum pipe_auth_level auth_level,
		   enum schannel_direction direction,
		   RPC_AUTH_SCHANNEL_CHK * verf,
		   char *data, size_t data_len)
{
	uchar digest_final[16];
	uchar confounder[8];
	uchar seq_num[8];
	static const uchar nullbytes[8];

	static const uchar schannel_seal_sig[8] = SCHANNEL_SEAL_SIGNATURE;
	static const uchar schannel_sign_sig[8] = SCHANNEL_SIGN_SIGNATURE;
	const uchar *schannel_sig = NULL;

	DEBUG(10,("SCHANNEL: schannel_encode seq_num=%d data_len=%lu\n", a->seq_num, (unsigned long)data_len));
	
	if (auth_level == PIPE_AUTH_LEVEL_PRIVACY) {
		schannel_sig = schannel_seal_sig;
	} else {
		schannel_sig = schannel_sign_sig;
	}

	/* fill the 'confounder' with random data */
	generate_random_buffer(confounder, sizeof(confounder));

	dump_data_pw("a->sess_key:\n", a->sess_key, sizeof(a->sess_key));

	RSIVAL(seq_num, 0, a->seq_num);

	switch (direction) {
	case SENDER_IS_INITIATOR:
		SIVAL(seq_num, 4, 0x80);
		break;
	case SENDER_IS_ACCEPTOR:
		SIVAL(seq_num, 4, 0x0);
		break;
	}

	dump_data_pw("verf->seq_num:\n", seq_num, sizeof(verf->seq_num));

	init_rpc_auth_schannel_chk(verf, schannel_sig, nullbytes,
				 seq_num, confounder);
				
	/* produce a digest of the packet to prove it's legit (before we seal it) */
	schannel_digest(a, auth_level, verf, data, data_len, digest_final);
	memcpy(verf->packet_digest, digest_final, sizeof(verf->packet_digest));

	if (auth_level == PIPE_AUTH_LEVEL_PRIVACY) {
		uchar sealing_key[16];

		/* get the key to encode the data with */
		schannel_get_sealing_key(a, verf, sealing_key);

		/* encode the verification data */
		dump_data_pw("verf->confounder:\n", verf->confounder, sizeof(verf->confounder));
		SamOEMhash(verf->confounder, sealing_key, 8);

		dump_data_pw("verf->confounder_enc:\n", verf->confounder, sizeof(verf->confounder));
		
		/* encode the packet payload */
		dump_data_pw("data:\n", (const unsigned char *)data, data_len);
		SamOEMhash((unsigned char *)data, sealing_key, data_len);
		dump_data_pw("data_enc:\n", (const unsigned char *)data, data_len);
	}

	/* encode the sequence number (key based on packet digest) */
	/* needs to be done after the sealing, as the original version 
	   is used in the sealing stuff... */
	schannel_deal_with_seq_num(a, verf);

	return;
}

/*******************************************************************
 Decode a blob of data using the schannel alogrithm, also verifiying
 a checksum over the original data.  We currently can verify signed messages,
 as well as decode sealed messages
 ********************************************************************/

BOOL schannel_decode(struct schannel_auth_struct *a, enum pipe_auth_level auth_level,
		   enum schannel_direction direction, 
		   RPC_AUTH_SCHANNEL_CHK * verf, char *data, size_t data_len)
{
	uchar digest_final[16];

	static const uchar schannel_seal_sig[8] = SCHANNEL_SEAL_SIGNATURE;
	static const uchar schannel_sign_sig[8] = SCHANNEL_SIGN_SIGNATURE;
	const uchar *schannel_sig = NULL;

	uchar seq_num[8];

	DEBUG(10,("SCHANNEL: schannel_decode seq_num=%d data_len=%lu\n", a->seq_num, (unsigned long)data_len));
	
	if (auth_level == PIPE_AUTH_LEVEL_PRIVACY) {
		schannel_sig = schannel_seal_sig;
	} else {
		schannel_sig = schannel_sign_sig;
	}

	/* Create the expected sequence number for comparison */
	RSIVAL(seq_num, 0, a->seq_num);

	switch (direction) {
	case SENDER_IS_INITIATOR:
		SIVAL(seq_num, 4, 0x80);
		break;
	case SENDER_IS_ACCEPTOR:
		SIVAL(seq_num, 4, 0x0);
		break;
	}

	DEBUG(10,("SCHANNEL: schannel_decode seq_num=%d data_len=%lu\n", a->seq_num, (unsigned long)data_len));
	dump_data_pw("a->sess_key:\n", a->sess_key, sizeof(a->sess_key));

	dump_data_pw("seq_num:\n", seq_num, sizeof(seq_num));

	/* extract the sequence number (key based on supplied packet digest) */
	/* needs to be done before the sealing, as the original version 
	   is used in the sealing stuff... */
	schannel_deal_with_seq_num(a, verf);

	if (memcmp(verf->seq_num, seq_num, sizeof(seq_num))) {
		/* don't even bother with the below if the sequence number is out */
		/* The sequence number is MD5'ed with a key based on the whole-packet
		   digest, as supplied by the client.  We check that it's a valid 
		   checksum after the decode, below
		*/
		DEBUG(2, ("schannel_decode: FAILED: packet sequence number:\n"));
		dump_data(2, (const char*)verf->seq_num, sizeof(verf->seq_num));
		DEBUG(2, ("should be:\n"));
		dump_data(2, (const char*)seq_num, sizeof(seq_num));

		return False;
	}

	if (memcmp(verf->sig, schannel_sig, sizeof(verf->sig))) {
		/* Validate that the other end sent the expected header */
		DEBUG(2, ("schannel_decode: FAILED: packet header:\n"));
		dump_data(2, (const char*)verf->sig, sizeof(verf->sig));
		DEBUG(2, ("should be:\n"));
		dump_data(2, (const char*)schannel_sig, sizeof(schannel_sig));
		return False;
	}

	if (auth_level == PIPE_AUTH_LEVEL_PRIVACY) {
		uchar sealing_key[16];
		
		/* get the key to extract the data with */
		schannel_get_sealing_key(a, verf, sealing_key);

		/* extract the verification data */
		dump_data_pw("verf->confounder:\n", verf->confounder, 
			     sizeof(verf->confounder));
		SamOEMhash(verf->confounder, sealing_key, 8);

		dump_data_pw("verf->confounder_dec:\n", verf->confounder, 
			     sizeof(verf->confounder));
		
		/* extract the packet payload */
		dump_data_pw("data   :\n", (const unsigned char *)data, data_len);
		SamOEMhash((unsigned char *)data, sealing_key, data_len);
		dump_data_pw("datadec:\n", (const unsigned char *)data, data_len);	
	}

	/* digest includes 'data' after unsealing */
	schannel_digest(a, auth_level, verf, data, data_len, digest_final);

	dump_data_pw("Calculated digest:\n", digest_final, 
		     sizeof(digest_final));
	dump_data_pw("verf->packet_digest:\n", verf->packet_digest, 
		     sizeof(verf->packet_digest));
	
	/* compare - if the client got the same result as us, then
	   it must know the session key */
	return (memcmp(digest_final, verf->packet_digest, 
		       sizeof(verf->packet_digest)) == 0);
}
