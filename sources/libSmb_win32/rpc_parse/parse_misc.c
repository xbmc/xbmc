/* 
 *  Unix SMB/CIFS implementation.
 *  RPC Pipe client / server routines
 *  Copyright (C) Andrew Tridgell              1992-1997,
 *  Copyright (C) Luke Kenneth Casson Leighton 1996-1997,
 *  Copyright (C) Paul Ashton                       1997.
 *  Copyright (C) Gerald (Jerry) Carter             2005
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

#undef DBGC_CLASS
#define DBGC_CLASS DBGC_RPC_PARSE

/****************************************************************************
 A temporary TALLOC context for things like unistrs, that is valid for
 the life of a complete RPC call.
****************************************************************************/

static TALLOC_CTX *current_rpc_talloc = NULL;

static TALLOC_CTX *get_current_rpc_talloc(void)
{
    return current_rpc_talloc;
}

void set_current_rpc_talloc( TALLOC_CTX *ctx)
{
	current_rpc_talloc = ctx;
}

static TALLOC_CTX *main_loop_talloc = NULL;

/*******************************************************************
free up temporary memory - called from the main loop
********************************************************************/

void main_loop_TALLOC_FREE(void)
{
    if (!main_loop_talloc)
        return;
    talloc_destroy(main_loop_talloc);
    main_loop_talloc = NULL;
}

/*******************************************************************
 Get a talloc context that is freed in the main loop...
********************************************************************/

TALLOC_CTX *main_loop_talloc_get(void)
{
    if (!main_loop_talloc) {
        main_loop_talloc = talloc_init("main loop talloc (mainly parse_misc)");
        if (!main_loop_talloc)
            smb_panic("main_loop_talloc: malloc fail\n");
    }

    return main_loop_talloc;
}

/*******************************************************************
 Try and get a talloc context. Get the rpc one if possible, else
 get the main loop one. The main loop one is more dangerous as it
 goes away between packets, the rpc one will stay around for as long
 as a current RPC lasts.
********************************************************************/ 

TALLOC_CTX *get_talloc_ctx(void)
{
	TALLOC_CTX *tc = get_current_rpc_talloc();

	if (tc)
		return tc;
	return main_loop_talloc_get();
}

/*******************************************************************
 Reads or writes a UTIME type.
********************************************************************/

static BOOL smb_io_utime(const char *desc, UTIME *t, prs_struct *ps, int depth)
{
	if (t == NULL)
		return False;

	prs_debug(ps, depth, desc, "smb_io_utime");
	depth++;

	if(!prs_align(ps))
		return False;
	
	if(!prs_uint32 ("time", ps, depth, &t->time))
		return False;

	return True;
}

/*******************************************************************
 Reads or writes an NTTIME structure.
********************************************************************/

BOOL smb_io_time(const char *desc, NTTIME *nttime, prs_struct *ps, int depth)
{
	if (nttime == NULL)
		return False;

	prs_debug(ps, depth, desc, "smb_io_time");
	depth++;

	if(!prs_align(ps))
		return False;
	
	if(!prs_uint32("low ", ps, depth, &nttime->low)) /* low part */
		return False;
	if(!prs_uint32("high", ps, depth, &nttime->high)) /* high part */
		return False;

	return True;
}

/*******************************************************************
 Reads or writes an NTTIME structure.
********************************************************************/

BOOL smb_io_nttime(const char *desc, prs_struct *ps, int depth, NTTIME *nttime)
{
	return smb_io_time( desc, nttime, ps, depth );
}

/*******************************************************************
 Gets an enumeration handle from an ENUM_HND structure.
********************************************************************/

uint32 get_enum_hnd(ENUM_HND *enh)
{
	return (enh && enh->ptr_hnd != 0) ? enh->handle : 0;
}

/*******************************************************************
 Inits an ENUM_HND structure.
********************************************************************/

void init_enum_hnd(ENUM_HND *enh, uint32 hnd)
{
	DEBUG(5,("smb_io_enum_hnd\n"));

	enh->ptr_hnd = (hnd != 0) ? 1 : 0;
	enh->handle = hnd;
}

/*******************************************************************
 Reads or writes an ENUM_HND structure.
********************************************************************/

BOOL smb_io_enum_hnd(const char *desc, ENUM_HND *hnd, prs_struct *ps, int depth)
{
	if (hnd == NULL)
		return False;

	prs_debug(ps, depth, desc, "smb_io_enum_hnd");
	depth++;

	if(!prs_align(ps))
		return False;
	
	if(!prs_uint32("ptr_hnd", ps, depth, &hnd->ptr_hnd)) /* pointer */
		return False;

	if (hnd->ptr_hnd != 0) {
		if(!prs_uint32("handle ", ps, depth, &hnd->handle )) /* enum handle */
			return False;
	}

	return True;
}

/*******************************************************************
 Reads or writes a DOM_SID structure.
********************************************************************/

BOOL smb_io_dom_sid(const char *desc, DOM_SID *sid, prs_struct *ps, int depth)
{
	int i;

	if (sid == NULL)
		return False;

	prs_debug(ps, depth, desc, "smb_io_dom_sid");
	depth++;

	if(!prs_uint8 ("sid_rev_num", ps, depth, &sid->sid_rev_num))
		return False;

	if(!prs_uint8 ("num_auths  ", ps, depth, &sid->num_auths))
		return False;

	for (i = 0; i < 6; i++)
	{
		fstring tmp;
		slprintf(tmp, sizeof(tmp) - 1, "id_auth[%d] ", i);
		if(!prs_uint8 (tmp, ps, depth, &sid->id_auth[i]))
			return False;
	}

	/* oops! XXXX should really issue a warning here... */
	if (sid->num_auths > MAXSUBAUTHS)
		sid->num_auths = MAXSUBAUTHS;

	if(!prs_uint32s(False, "sub_auths ", ps, depth, sid->sub_auths, sid->num_auths))
		return False;

	return True;
}

/*******************************************************************
 Inits a DOM_SID2 structure.
********************************************************************/

void init_dom_sid2(DOM_SID2 *sid2, const DOM_SID *sid)
{
	sid2->sid = *sid;
	sid2->num_auths = sid2->sid.num_auths;
}

/*******************************************************************
 Reads or writes a DOM_SID2 structure.
********************************************************************/

BOOL smb_io_dom_sid2_p(const char *desc, prs_struct *ps, int depth, DOM_SID2 **sid2)
{
	uint32 data_p;

	/* caputure the pointer value to stream */

	data_p = *sid2 ? 0xf000baaa : 0;

	if ( !prs_uint32("dom_sid2_p", ps, depth, &data_p ))
		return False;

	/* we're done if there is no data */

	if ( !data_p )
		return True;

	if (UNMARSHALLING(ps)) {
		if ( !(*sid2 = PRS_ALLOC_MEM(ps, DOM_SID2, 1)) )
		return False;
	}

	return True;
}
/*******************************************************************
 Reads or writes a DOM_SID2 structure.
********************************************************************/

BOOL smb_io_dom_sid2(const char *desc, DOM_SID2 *sid, prs_struct *ps, int depth)
{
	if (sid == NULL)
		return False;

	prs_debug(ps, depth, desc, "smb_io_dom_sid2");
	depth++;

	if(!prs_align(ps))
		return False;
	
	if(!prs_uint32("num_auths", ps, depth, &sid->num_auths))
		return False;

	if(!smb_io_dom_sid("sid", &sid->sid, ps, depth))
		return False;

	return True;
}

/*******************************************************************
 Reads or writes a struct uuid
********************************************************************/

BOOL smb_io_uuid(const char *desc, struct uuid *uuid, 
		 prs_struct *ps, int depth)
{
	if (uuid == NULL)
		return False;

	prs_debug(ps, depth, desc, "smb_io_uuid");
	depth++;

	if(!prs_uint32 ("data   ", ps, depth, &uuid->time_low))
		return False;
	if(!prs_uint16 ("data   ", ps, depth, &uuid->time_mid))
		return False;
	if(!prs_uint16 ("data   ", ps, depth, &uuid->time_hi_and_version))
		return False;

	if(!prs_uint8s (False, "data   ", ps, depth, uuid->clock_seq, sizeof(uuid->clock_seq)))
		return False;
	if(!prs_uint8s (False, "data   ", ps, depth, uuid->node, sizeof(uuid->node)))
		return False;

	return True;
}

/*******************************************************************
creates a STRHDR structure.
********************************************************************/

void init_str_hdr(STRHDR *hdr, int max_len, int len, uint32 buffer)
{
	hdr->str_max_len = max_len;
	hdr->str_str_len = len;
	hdr->buffer      = buffer;
}

/*******************************************************************
 Reads or writes a STRHDR structure.
********************************************************************/

BOOL smb_io_strhdr(const char *desc,  STRHDR *hdr, prs_struct *ps, int depth)
{
	if (hdr == NULL)
		return False;

	prs_debug(ps, depth, desc, "smb_io_strhdr");
	depth++;

	prs_align(ps);
	
	if(!prs_uint16("str_str_len", ps, depth, &hdr->str_str_len))
		return False;
	if(!prs_uint16("str_max_len", ps, depth, &hdr->str_max_len))
		return False;
	if(!prs_uint32("buffer     ", ps, depth, &hdr->buffer))
		return False;

	return True;
}

/*******************************************************************
 Inits a UNIHDR structure.
********************************************************************/

void init_uni_hdr(UNIHDR *hdr, UNISTR2 *str2)
{
	hdr->uni_str_len = 2 * (str2->uni_str_len);
	hdr->uni_max_len = 2 * (str2->uni_max_len);
	hdr->buffer = (str2->uni_str_len != 0) ? 1 : 0;
}

/*******************************************************************
 Reads or writes a UNIHDR structure.
********************************************************************/

BOOL smb_io_unihdr(const char *desc, UNIHDR *hdr, prs_struct *ps, int depth)
{
	if (hdr == NULL)
		return False;

	prs_debug(ps, depth, desc, "smb_io_unihdr");
	depth++;

	if(!prs_align(ps))
		return False;
	
	if(!prs_uint16("uni_str_len", ps, depth, &hdr->uni_str_len))
		return False;
	if(!prs_uint16("uni_max_len", ps, depth, &hdr->uni_max_len))
		return False;
	if(!prs_uint32("buffer     ", ps, depth, &hdr->buffer))
		return False;

	return True;
}

/*******************************************************************
 Inits a BUFHDR structure.
********************************************************************/

void init_buf_hdr(BUFHDR *hdr, int max_len, int len)
{
	hdr->buf_max_len = max_len;
	hdr->buf_len     = len;
}

/*******************************************************************
 prs_uint16 wrapper. Call this and it sets up a pointer to where the
 uint16 should be stored, or gets the size if reading.
 ********************************************************************/

BOOL smb_io_hdrbuf_pre(const char *desc, BUFHDR *hdr, prs_struct *ps, int depth, uint32 *offset)
{
	(*offset) = prs_offset(ps);
	if (ps->io) {

		/* reading. */

		if(!smb_io_hdrbuf(desc, hdr, ps, depth))
			return False;

	} else {

		/* writing. */

		if(!prs_set_offset(ps, prs_offset(ps) + (sizeof(uint32) * 2)))
			return False;
	}

	return True;
}

/*******************************************************************
 smb_io_hdrbuf wrapper. Call this and it retrospectively stores the size.
 Does nothing on reading, as that is already handled by ...._pre()
 ********************************************************************/

BOOL smb_io_hdrbuf_post(const char *desc, BUFHDR *hdr, prs_struct *ps, int depth, 
				uint32 ptr_hdrbuf, uint32 max_len, uint32 len)
{
	if (!ps->io) {
		/* writing: go back and do a retrospective job.  i hate this */

		uint32 old_offset = prs_offset(ps);

		init_buf_hdr(hdr, max_len, len);
		if(!prs_set_offset(ps, ptr_hdrbuf))
			return False;
		if(!smb_io_hdrbuf(desc, hdr, ps, depth))
			return False;

		if(!prs_set_offset(ps, old_offset))
			return False;
	}

	return True;
}

/*******************************************************************
 Reads or writes a BUFHDR structure.
********************************************************************/

BOOL smb_io_hdrbuf(const char *desc, BUFHDR *hdr, prs_struct *ps, int depth)
{
	if (hdr == NULL)
		return False;

	prs_debug(ps, depth, desc, "smb_io_hdrbuf");
	depth++;

	if(!prs_align(ps))
		return False;
	
	if(!prs_uint32("buf_max_len", ps, depth, &hdr->buf_max_len))
		return False;
	if(!prs_uint32("buf_len    ", ps, depth, &hdr->buf_len))
		return False;

	return True;
}

/*******************************************************************
 Inits a UNISTR structure.
********************************************************************/

void init_unistr(UNISTR *str, const char *buf)
{
	size_t len;

	if (buf == NULL) {
		str->buffer = NULL;
		return;
	}
		
	len = strlen(buf) + 1;

	str->buffer = TALLOC_ZERO_ARRAY(get_talloc_ctx(), uint16, len);
	if (str->buffer == NULL)
		smb_panic("init_unistr: malloc fail\n");

	rpcstr_push(str->buffer, buf, len*sizeof(uint16), STR_TERMINATE);
}

/*******************************************************************
reads or writes a UNISTR structure.
XXXX NOTE: UNISTR structures NEED to be null-terminated.
********************************************************************/

BOOL smb_io_unistr(const char *desc, UNISTR *uni, prs_struct *ps, int depth)
{
	if (uni == NULL)
		return False;

	prs_debug(ps, depth, desc, "smb_io_unistr");
	depth++;

	if(!prs_unistr("unistr", ps, depth, uni))
		return False;

	return True;
}

/*******************************************************************
 Allocate the RPC_DATA_BLOB memory.
********************************************************************/

size_t create_rpc_blob(RPC_DATA_BLOB *str, size_t len)
{
	str->buffer = TALLOC_ZERO(get_talloc_ctx(), len);
	if (str->buffer == NULL)
		smb_panic("create_rpc_blob: talloc fail\n");
	return len;
}

/*******************************************************************
 Inits a RPC_DATA_BLOB structure from a uint32
********************************************************************/

void init_rpc_blob_uint32(RPC_DATA_BLOB *str, uint32 val)
{
	ZERO_STRUCTP(str);

	/* set up string lengths. */
	str->buf_len = create_rpc_blob(str, sizeof(uint32));
	SIVAL(str->buffer, 0, val);
}

/*******************************************************************
 Inits a RPC_DATA_BLOB structure.
********************************************************************/

void init_rpc_blob_str(RPC_DATA_BLOB *str, const char *buf, int len)
{
	ZERO_STRUCTP(str);

	/* set up string lengths. */
	str->buf_len = create_rpc_blob(str, len*2);
	rpcstr_push(str->buffer, buf, (size_t)str->buf_len, STR_TERMINATE);
	
}

/*******************************************************************
 Inits a RPC_DATA_BLOB structure from a hex string.
********************************************************************/

void init_rpc_blob_hex(RPC_DATA_BLOB *str, const char *buf)
{
	ZERO_STRUCTP(str);
	str->buf_len = create_rpc_blob(str, strlen(buf));
	str->buf_len = strhex_to_str((char *)str->buffer, str->buf_len, buf);
}

/*******************************************************************
 Inits a RPC_DATA_BLOB structure.
********************************************************************/

void init_rpc_blob_bytes(RPC_DATA_BLOB *str, uint8 *buf, size_t len)
{
	ZERO_STRUCTP(str);

	/* max buffer size (allocated size) */
	if (buf != NULL) {
		len = create_rpc_blob(str, len);
		memcpy(str->buffer, buf, len);
	}
	str->buf_len = len;
}

/*******************************************************************
reads or writes a BUFFER5 structure.
the buf_len member tells you how large the buffer is.
********************************************************************/
BOOL smb_io_buffer5(const char *desc, BUFFER5 *buf5, prs_struct *ps, int depth)
{
	prs_debug(ps, depth, desc, "smb_io_buffer5");
	depth++;

	if (buf5 == NULL) return False;

	if(!prs_align(ps))
		return False;
	if(!prs_uint32("buf_len", ps, depth, &buf5->buf_len))
		return False;

	if(buf5->buf_len) {
		if(!prs_buffer5(True, "buffer" , ps, depth, buf5))
			return False;
	}

	return True;
}

/*******************************************************************
 Inits a REGVAL_BUFFER structure.
********************************************************************/

void init_regval_buffer(REGVAL_BUFFER *str, const uint8 *buf, size_t len)
{
	ZERO_STRUCTP(str);

	/* max buffer size (allocated size) */
	str->buf_max_len = len;
	str->offset = 0;
	str->buf_len = buf != NULL ? len : 0;

	if (buf != NULL) {
		SMB_ASSERT(str->buf_max_len >= str->buf_len);
		str->buffer = TALLOC_ZERO(get_talloc_ctx(), str->buf_max_len);
		if (str->buffer == NULL)
			smb_panic("init_regval_buffer: talloc fail\n");
		memcpy(str->buffer, buf, str->buf_len);
	}
}

/*******************************************************************
 Reads or writes a REGVAL_BUFFER structure.
   the uni_max_len member tells you how large the buffer is.
   the uni_str_len member tells you how much of the buffer is really used.
********************************************************************/

BOOL smb_io_regval_buffer(const char *desc, prs_struct *ps, int depth, REGVAL_BUFFER *buf2)
{

	prs_debug(ps, depth, desc, "smb_io_regval_buffer");
	depth++;

	if(!prs_align(ps))
		return False;
		
	if(!prs_uint32("buf_max_len", ps, depth, &buf2->buf_max_len))
		return False;
	if(!prs_uint32("offset     ", ps, depth, &buf2->offset))
		return False;
	if(!prs_uint32("buf_len    ", ps, depth, &buf2->buf_len))
		return False;

	/* buffer advanced by indicated length of string
	   NOT by searching for null-termination */

	if(!prs_regval_buffer(True, "buffer     ", ps, depth, buf2))
		return False;

	return True;
}

/*******************************************************************
creates a UNISTR2 structure: sets up the buffer, too
********************************************************************/

void init_buf_unistr2(UNISTR2 *str, uint32 *ptr, const char *buf)
{
	if (buf != NULL) {
		*ptr = 1;
		init_unistr2(str, buf, UNI_STR_TERMINATE);
	} else {
		*ptr = 0;
		init_unistr2(str, NULL, UNI_FLAGS_NONE);

	}
}

/*******************************************************************
 Copies a UNISTR2 structure.
********************************************************************/

void copy_unistr2(UNISTR2 *str, const UNISTR2 *from)
{
	if (from->buffer == NULL) {
		ZERO_STRUCTP(str);
		return;
	}

	SMB_ASSERT(from->uni_max_len >= from->uni_str_len);

	str->uni_max_len = from->uni_max_len;
	str->offset      = from->offset;
	str->uni_str_len = from->uni_str_len;

	/* the string buffer is allocated to the maximum size
	   (the the length of the source string) to prevent
	   reallocation of memory. */
	if (str->buffer == NULL) {
   		str->buffer = (uint16 *)TALLOC_ZERO_ARRAY(get_talloc_ctx(), uint16, str->uni_max_len);
		if ((str->buffer == NULL)) {
			smb_panic("copy_unistr2: talloc fail\n");
			return;
		}
	}

	/* copy the string */
	memcpy(str->buffer, from->buffer, str->uni_max_len*sizeof(uint16));
}

/*******************************************************************
 Creates a STRING2 structure.
********************************************************************/

void init_string2(STRING2 *str, const char *buf, size_t max_len, size_t str_len)
{
	/* set up string lengths. */
	SMB_ASSERT(max_len >= str_len);

	/* Ensure buf is valid if str_len was set. Coverity check. */
	if (str_len && !buf) {
		return;
	}

	str->str_max_len = max_len;
	str->offset = 0;
	str->str_str_len = str_len;

	/* store the string */
	if(str_len != 0) {
		str->buffer = TALLOC_ZERO(get_talloc_ctx(), str->str_max_len);
		if (str->buffer == NULL)
			smb_panic("init_string2: malloc fail\n");
		memcpy(str->buffer, buf, str_len);
	}
}

/*******************************************************************
 Reads or writes a STRING2 structure.
 XXXX NOTE: STRING2 structures need NOT be null-terminated.
   the str_str_len member tells you how long the string is;
   the str_max_len member tells you how large the buffer is.
********************************************************************/

BOOL smb_io_string2(const char *desc, STRING2 *str2, uint32 buffer, prs_struct *ps, int depth)
{
	if (str2 == NULL)
		return False;

	if (buffer) {

		prs_debug(ps, depth, desc, "smb_io_string2");
		depth++;

		if(!prs_align(ps))
			return False;
		
		if(!prs_uint32("str_max_len", ps, depth, &str2->str_max_len))
			return False;
		if(!prs_uint32("offset     ", ps, depth, &str2->offset))
			return False;
		if(!prs_uint32("str_str_len", ps, depth, &str2->str_str_len))
			return False;

		/* buffer advanced by indicated length of string
		   NOT by searching for null-termination */
		if(!prs_string2(True, "buffer     ", ps, depth, str2))
			return False;

	} else {

		prs_debug(ps, depth, desc, "smb_io_string2 - NULL");
		depth++;
		memset((char *)str2, '\0', sizeof(*str2));

	}

	return True;
}

/*******************************************************************
 Inits a UNISTR2 structure.
********************************************************************/

void init_unistr2(UNISTR2 *str, const char *buf, enum unistr2_term_codes flags)
{
	size_t len = 0;
	uint32 num_chars = 0;

	if (buf) {
		/* We always null terminate the copy. */
		len = strlen(buf) + 1;
		if ( flags == UNI_STR_DBLTERMINATE )
			len++;
	} else {
		/* no buffer -- nothing to do */
		str->uni_max_len = 0;
		str->offset = 0;
		str->uni_str_len = 0;

		return;
	}
	

	str->buffer = TALLOC_ZERO_ARRAY(get_talloc_ctx(), uint16, len);
	if (str->buffer == NULL) {
		smb_panic("init_unistr2: malloc fail\n");
		return;
	}

	/* Ensure len is the length in *bytes* */
	len *= sizeof(uint16);

	/*
	 * The UNISTR2 must be initialized !!!
	 * jfm, 7/7/2001.
	 */
	if (buf) {
		rpcstr_push((char *)str->buffer, buf, len, STR_TERMINATE);
		num_chars = strlen_w(str->buffer);
		if (flags == UNI_STR_TERMINATE || flags == UNI_MAXLEN_TERMINATE) {
			num_chars++;
		}
		if ( flags == UNI_STR_DBLTERMINATE )
			num_chars += 2;
	}

	str->uni_max_len = num_chars;
	str->offset = 0;
	str->uni_str_len = num_chars;
	if ( num_chars && ((flags == UNI_MAXLEN_TERMINATE) || (flags == UNI_BROKEN_NON_NULL)) )
		str->uni_max_len++;
}

/*******************************************************************
 Inits a UNISTR4 structure.
********************************************************************/

void init_unistr4(UNISTR4 *uni4, const char *buf, enum unistr2_term_codes flags)
{
	uni4->string = TALLOC_P( get_talloc_ctx(), UNISTR2 );
	if (!uni4->string) {
		smb_panic("init_unistr4: talloc fail\n");
		return;
	}
	init_unistr2( uni4->string, buf, flags );

	uni4->length = 2 * (uni4->string->uni_str_len);
	uni4->size   = 2 * (uni4->string->uni_max_len);
}

void init_unistr4_w( TALLOC_CTX *ctx, UNISTR4 *uni4, const smb_ucs2_t *buf )
{
	uni4->string = TALLOC_P( ctx, UNISTR2 );
	if (!uni4->string) {
		smb_panic("init_unistr4_w: talloc fail\n");
		return;
	}
	init_unistr2_w( ctx, uni4->string, buf );

	uni4->length = 2 * (uni4->string->uni_str_len);
	uni4->size   = 2 * (uni4->string->uni_max_len);
}

/** 
 *  Inits a UNISTR2 structure.
 *  @param  ctx talloc context to allocate string on
 *  @param  str pointer to string to create
 *  @param  buf UCS2 null-terminated buffer to init from
*/

void init_unistr2_w(TALLOC_CTX *ctx, UNISTR2 *str, const smb_ucs2_t *buf)
{
	uint32 len = buf ? strlen_w(buf) : 0;

	ZERO_STRUCTP(str);

	/* set up string lengths. */
	str->uni_max_len = len;
	str->offset = 0;
	str->uni_str_len = len;

	str->buffer = TALLOC_ZERO_ARRAY(ctx, uint16, len + 1);
	if (str->buffer == NULL) {
		smb_panic("init_unistr2_w: talloc fail\n");
		return;
	}
	
	/*
	 * don't move this test above ! The UNISTR2 must be initialized !!!
	 * jfm, 7/7/2001.
	 */
	if (buf==NULL)
		return;
	
	/* Yes, this is a strncpy( foo, bar, strlen(bar)) - but as
           long as the buffer above is talloc()ed correctly then this
           is the correct thing to do */
	strncpy_w(str->buffer, buf, len + 1);
}

/*******************************************************************
 Inits a UNISTR2 structure from a UNISTR
********************************************************************/

void init_unistr2_from_unistr(UNISTR2 *to, const UNISTR *from)
{
	uint32 i;

	/* the destination UNISTR2 should never be NULL.
	   if it is it is a programming error */

	/* if the source UNISTR is NULL, then zero out
	   the destination string and return */
	ZERO_STRUCTP (to);
	if ((from == NULL) || (from->buffer == NULL))
		return;

	/* get the length; UNISTR must be NULL terminated */
	i = 0;
	while ((from->buffer)[i]!='\0')
		i++;
	i++;	/* one more to catch the terminating NULL */
		/* is this necessary -- jerry?  I need to think */

	/* set up string lengths; uni_max_len is set to i+1
           because we need to account for the final NULL termination */
	to->uni_max_len = i;
	to->offset = 0;
	to->uni_str_len = i;

	/* allocate the space and copy the string buffer */
	to->buffer = TALLOC_ZERO_ARRAY(get_talloc_ctx(), uint16, i);
	if (to->buffer == NULL)
		smb_panic("init_unistr2_from_unistr: malloc fail\n");
	memcpy(to->buffer, from->buffer, i*sizeof(uint16));
	return;
}

/*******************************************************************
  Inits a UNISTR2 structure from a DATA_BLOB.
  The length of the data_blob must count the bytes of the buffer.
  Copies the blob data.
********************************************************************/

void init_unistr2_from_datablob(UNISTR2 *str, DATA_BLOB *blob) 
{
	/* Allocs the unistring */
	init_unistr2(str, NULL, UNI_FLAGS_NONE);
	
	/* Sets the values */
	str->uni_str_len = blob->length / sizeof(uint16);
	str->uni_max_len = str->uni_str_len;
	str->offset = 0;
	if (blob->length) {
		str->buffer = (uint16 *) memdup(blob->data, blob->length);
	} else {
		str->buffer = NULL;
	}
	if ((str->buffer == NULL) && (blob->length > 0)) {
		smb_panic("init_unistr2_from_datablob: malloc fail\n");
	}
}

/*******************************************************************
 UNISTR2* are a little different in that the pointer and the UNISTR2
 are not necessarily read/written back to back.  So we break it up 
 into 2 separate functions.
 See SPOOL_USER_1 in include/rpc_spoolss.h for an example.
********************************************************************/

BOOL prs_io_unistr2_p(const char *desc, prs_struct *ps, int depth, UNISTR2 **uni2)
{
	uint32 data_p;

	/* caputure the pointer value to stream */

	data_p = *uni2 ? 0xf000baaa : 0;

	if ( !prs_uint32("ptr", ps, depth, &data_p ))
		return False;

	/* we're done if there is no data */

	if ( !data_p )
		return True;

	if (UNMARSHALLING(ps)) {
		if ( !(*uni2 = PRS_ALLOC_MEM(ps, UNISTR2, 1)) )
			return False;
	}

	return True;
}

/*******************************************************************
 now read/write the actual UNISTR2.  Memory for the UNISTR2 (but
 not UNISTR2.buffer) has been allocated previously by prs_unistr2_p()
********************************************************************/

BOOL prs_io_unistr2(const char *desc, prs_struct *ps, int depth, UNISTR2 *uni2 )
{
	/* just return true if there is no pointer to deal with.
	   the memory must have been previously allocated on unmarshalling
	   by prs_unistr2_p() */

	if ( !uni2 )
		return True;

	/* just pass off to smb_io_unstr2() passing the uni2 address as 
	   the pointer (like you would expect) */

	return smb_io_unistr2( desc, uni2, uni2 ? 1 : 0, ps, depth );
}

/*******************************************************************
 Reads or writes a UNISTR2 structure.
 XXXX NOTE: UNISTR2 structures need NOT be null-terminated.
   the uni_str_len member tells you how long the string is;
   the uni_max_len member tells you how large the buffer is.
********************************************************************/

BOOL smb_io_unistr2(const char *desc, UNISTR2 *uni2, uint32 buffer, prs_struct *ps, int depth)
{
	if (uni2 == NULL)
		return False;

	if (buffer) {

		prs_debug(ps, depth, desc, "smb_io_unistr2");
		depth++;

		if(!prs_align(ps))
			return False;
		
		if(!prs_uint32("uni_max_len", ps, depth, &uni2->uni_max_len))
			return False;
		if(!prs_uint32("offset     ", ps, depth, &uni2->offset))
			return False;
		if(!prs_uint32("uni_str_len", ps, depth, &uni2->uni_str_len))
			return False;

		/* buffer advanced by indicated length of string
		   NOT by searching for null-termination */
		if(!prs_unistr2(True, "buffer     ", ps, depth, uni2))
			return False;

	} else {

		prs_debug(ps, depth, desc, "smb_io_unistr2 - NULL");
		depth++;
		memset((char *)uni2, '\0', sizeof(*uni2));

	}

	return True;
}

/*******************************************************************
 now read/write UNISTR4
********************************************************************/

BOOL prs_unistr4(const char *desc, prs_struct *ps, int depth, UNISTR4 *uni4)
{
	void *ptr;
	prs_debug(ps, depth, desc, "prs_unistr4");
	depth++;

	if ( !prs_uint16("length", ps, depth, &uni4->length ))
		return False;
	if ( !prs_uint16("size", ps, depth, &uni4->size ))
		return False;
		
	ptr = uni4->string;

	if ( !prs_pointer( desc, ps, depth, &ptr, sizeof(UNISTR2), (PRS_POINTER_CAST)prs_io_unistr2 ) )
		return False;

	uni4->string = (UNISTR2 *)ptr;
	
	return True;
}

/*******************************************************************
 now read/write UNISTR4 header
********************************************************************/

BOOL prs_unistr4_hdr(const char *desc, prs_struct *ps, int depth, UNISTR4 *uni4)
{
	prs_debug(ps, depth, desc, "prs_unistr4_hdr");
	depth++;

	if ( !prs_uint16("length", ps, depth, &uni4->length) )
		return False;
	if ( !prs_uint16("size", ps, depth, &uni4->size) )
		return False;
	if ( !prs_io_unistr2_p(desc, ps, depth, &uni4->string) )
		return False;
		
	return True;
}

/*******************************************************************
 now read/write UNISTR4 string
********************************************************************/

BOOL prs_unistr4_str(const char *desc, prs_struct *ps, int depth, UNISTR4 *uni4)
{
	prs_debug(ps, depth, desc, "prs_unistr4_str");
	depth++;

	if ( !prs_io_unistr2(desc, ps, depth, uni4->string) )
		return False;
		
	return True;
}

/*******************************************************************
 Reads or writes a UNISTR4_ARRAY structure.
********************************************************************/

BOOL prs_unistr4_array(const char *desc, prs_struct *ps, int depth, UNISTR4_ARRAY *array )
{
	unsigned int i;

	prs_debug(ps, depth, desc, "prs_unistr4_array");
	depth++;

	if(!prs_uint32("count", ps, depth, &array->count))
		return False;

	if ( array->count == 0 ) 
		return True;
	
	if (UNMARSHALLING(ps)) {
		if ( !(array->strings = TALLOC_ZERO_ARRAY( get_talloc_ctx(), UNISTR4, array->count)) )
			return False;
	}
	
	/* write the headers and then the actual string buffer */
	
	for ( i=0; i<array->count; i++ ) {
		if ( !prs_unistr4_hdr( "string", ps, depth, &array->strings[i]) )
			return False;
	}

	for (i=0;i<array->count;i++) {
		if ( !prs_unistr4_str("string", ps, depth, &array->strings[i]) ) 
			return False;
	}
	
	return True;
}

/********************************************************************
  initialise a UNISTR_ARRAY from a char**
********************************************************************/

BOOL init_unistr4_array( UNISTR4_ARRAY *array, uint32 count, const char **strings )
{
	unsigned int i;

	array->count = count;

	if ( array->count == 0 )
		return True;

	/* allocate memory for the array of UNISTR4 objects */

	if ( !(array->strings = TALLOC_ZERO_ARRAY(get_talloc_ctx(), UNISTR4, count )) )
		return False;

	for ( i=0; i<count; i++ ) 
		init_unistr4( &array->strings[i], strings[i], UNI_STR_TERMINATE );

	return True;
}

BOOL smb_io_lockout_string_hdr(const char *desc, HDR_LOCKOUT_STRING *hdr_account_lockout, prs_struct *ps, int depth)
{
	prs_debug(ps, depth, desc, "smb_io_lockout_string_hdr");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!prs_uint16("size", ps, depth, &hdr_account_lockout->size))
		return False;
	if(!prs_uint16("length", ps, depth, &hdr_account_lockout->length))
		return False;
	if(!prs_uint32("buffer", ps, depth, &hdr_account_lockout->buffer))
		return False;

	return True;
}

BOOL smb_io_account_lockout_str(const char *desc, LOCKOUT_STRING *account_lockout, uint32 buffer, prs_struct *ps, int depth)
{
	prs_debug(ps, depth, desc, "smb_io_account_lockout_string");
	depth++;

	if(!prs_uint32("array_size", ps, depth, &account_lockout->array_size))
		return False;

	if(!prs_uint32("offset", ps, depth, &account_lockout->offset))
		return False;
	if(!prs_uint32("length", ps, depth, &account_lockout->length))
		return False;

	if (!prs_uint64("lockout_duration", ps, depth, &account_lockout->lockout_duration))
		return False;
	if (!prs_uint64("reset_count", ps, depth, &account_lockout->reset_count))
		return False;
	if (!prs_uint32("bad_attempt_lockout", ps, depth, &account_lockout->bad_attempt_lockout))
		return False;
	if (!prs_uint32("dummy", ps, depth, &account_lockout->dummy))
		return False;
#if 0
	if(!prs_uint16s (False, "bindata", ps, depth, &account_lockout->bindata, length))
		return False;
#endif

	return True;
}

/*******************************************************************
 Inits a DOM_RID structure.
********************************************************************/

void init_dom_rid(DOM_RID *prid, uint32 rid, uint16 type, uint32 idx)
{
	prid->type    = type;
	prid->rid     = rid;
	prid->rid_idx = idx;
}

/*******************************************************************
 Reads or writes a DOM_RID structure.
********************************************************************/

BOOL smb_io_dom_rid(const char *desc, DOM_RID *rid, prs_struct *ps, int depth)
{
	if (rid == NULL)
		return False;

	prs_debug(ps, depth, desc, "smb_io_dom_rid");
	depth++;

	if(!prs_align(ps))
		return False;
   
	if(!prs_uint16("type   ", ps, depth, &rid->type))
		return False;
	if(!prs_align(ps))
		return False;
	if(!prs_uint32("rid    ", ps, depth, &rid->rid))
		return False;
	if(!prs_uint32("rid_idx", ps, depth, &rid->rid_idx))
		return False;

	return True;
}

/*******************************************************************
 Reads or writes a DOM_RID2 structure.
********************************************************************/

BOOL smb_io_dom_rid2(const char *desc, DOM_RID2 *rid, prs_struct *ps, int depth)
{
	if (rid == NULL)
		return False;

	prs_debug(ps, depth, desc, "smb_io_dom_rid2");
	depth++;

	if(!prs_align(ps))
		return False;
   
	if(!prs_uint16("type   ", ps, depth, &rid->type))
		return False;
	if(!prs_align(ps))
		return False;
	if(!prs_uint32("rid    ", ps, depth, &rid->rid))
		return False;
	if(!prs_uint32("rid_idx", ps, depth, &rid->rid_idx))
		return False;
	if(!prs_uint32("unknown", ps, depth, &rid->unknown))
		return False;

	return True;
}


/*******************************************************************
creates a DOM_RID3 structure.
********************************************************************/

void init_dom_rid3(DOM_RID3 *rid3, uint32 rid, uint8 type)
{
    rid3->rid      = rid;
    rid3->type1    = type;
    rid3->ptr_type = 0x1; /* non-zero, basically. */
    rid3->type2    = 0x1;
    rid3->unk      = type;
}

/*******************************************************************
reads or writes a DOM_RID3 structure.
********************************************************************/

BOOL smb_io_dom_rid3(const char *desc, DOM_RID3 *rid3, prs_struct *ps, int depth)
{
	if (rid3 == NULL)
		return False;

	prs_debug(ps, depth, desc, "smb_io_dom_rid3");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!prs_uint32("rid     ", ps, depth, &rid3->rid))
		return False;
	if(!prs_uint32("type1   ", ps, depth, &rid3->type1))
		return False;
	if(!prs_uint32("ptr_type", ps, depth, &rid3->ptr_type))
		return False;
	if(!prs_uint32("type2   ", ps, depth, &rid3->type2))
		return False;
	if(!prs_uint32("unk     ", ps, depth, &rid3->unk))
		return False;

	return True;
}

/*******************************************************************
 Inits a DOM_RID4 structure.
********************************************************************/

void init_dom_rid4(DOM_RID4 *rid4, uint16 unknown, uint16 attr, uint32 rid)
{
    rid4->unknown = unknown;
    rid4->attr    = attr;
    rid4->rid     = rid;
}

/*******************************************************************
 Inits a DOM_CLNT_SRV structure.
********************************************************************/

static void init_clnt_srv(DOM_CLNT_SRV *logcln, const char *logon_srv, const char *comp_name)
{
	DEBUG(5,("init_clnt_srv: %d\n", __LINE__));

	if (logon_srv != NULL) {
		logcln->undoc_buffer = 1;
		init_unistr2(&logcln->uni_logon_srv, logon_srv, UNI_STR_TERMINATE);
	} else {
		logcln->undoc_buffer = 0;
	}

	if (comp_name != NULL) {
		logcln->undoc_buffer2 = 1;
		init_unistr2(&logcln->uni_comp_name, comp_name, UNI_STR_TERMINATE);
	} else {
		logcln->undoc_buffer2 = 0;
	}
}

/*******************************************************************
 Inits or writes a DOM_CLNT_SRV structure.
********************************************************************/

BOOL smb_io_clnt_srv(const char *desc, DOM_CLNT_SRV *logcln, prs_struct *ps, int depth)
{
	if (logcln == NULL)
		return False;

	prs_debug(ps, depth, desc, "smb_io_clnt_srv");
	depth++;

	if(!prs_align(ps))
		return False;
	
	if(!prs_uint32("undoc_buffer ", ps, depth, &logcln->undoc_buffer))
		return False;

	if (logcln->undoc_buffer != 0) {
		if(!smb_io_unistr2("unistr2", &logcln->uni_logon_srv, logcln->undoc_buffer, ps, depth))
			return False;
	}

	if(!prs_align(ps))
		return False;

	if(!prs_uint32("undoc_buffer2", ps, depth, &logcln->undoc_buffer2))
		return False;

	if (logcln->undoc_buffer2 != 0) {
		if(!smb_io_unistr2("unistr2", &logcln->uni_comp_name, logcln->undoc_buffer2, ps, depth))
			return False;
	}

	return True;
}

/*******************************************************************
 Inits a DOM_LOG_INFO structure.
********************************************************************/

void init_log_info(DOM_LOG_INFO *loginfo, const char *logon_srv, const char *acct_name,
		uint16 sec_chan, const char *comp_name)
{
	DEBUG(5,("make_log_info %d\n", __LINE__));

	loginfo->undoc_buffer = 1;

	init_unistr2(&loginfo->uni_logon_srv, logon_srv, UNI_STR_TERMINATE);
	init_unistr2(&loginfo->uni_acct_name, acct_name, UNI_STR_TERMINATE);

	loginfo->sec_chan = sec_chan;

	init_unistr2(&loginfo->uni_comp_name, comp_name, UNI_STR_TERMINATE);
}

/*******************************************************************
 Reads or writes a DOM_LOG_INFO structure.
********************************************************************/

BOOL smb_io_log_info(const char *desc, DOM_LOG_INFO *loginfo, prs_struct *ps, int depth)
{
	if (loginfo == NULL)
		return False;

	prs_debug(ps, depth, desc, "smb_io_log_info");
	depth++;

	if(!prs_align(ps))
		return False;
	
	if(!prs_uint32("undoc_buffer", ps, depth, &loginfo->undoc_buffer))
		return False;

	if(!smb_io_unistr2("unistr2", &loginfo->uni_logon_srv, True, ps, depth))
		return False;
	if(!smb_io_unistr2("unistr2", &loginfo->uni_acct_name, True, ps, depth))
		return False;

	if(!prs_uint16("sec_chan", ps, depth, &loginfo->sec_chan))
		return False;

	if(!smb_io_unistr2("unistr2", &loginfo->uni_comp_name, True, ps, depth))
		return False;

	return True;
}

/*******************************************************************
 Reads or writes a DOM_CHAL structure.
********************************************************************/

BOOL smb_io_chal(const char *desc, DOM_CHAL *chal, prs_struct *ps, int depth)
{
	if (chal == NULL)
		return False;

	prs_debug(ps, depth, desc, "smb_io_chal");
	depth++;
	
	if(!prs_uint8s (False, "data", ps, depth, chal->data, 8))
		return False;

	return True;
}

/*******************************************************************
 Reads or writes a DOM_CRED structure.
********************************************************************/

BOOL smb_io_cred(const char *desc,  DOM_CRED *cred, prs_struct *ps, int depth)
{
	if (cred == NULL)
		return False;

	prs_debug(ps, depth, desc, "smb_io_cred");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!smb_io_chal ("", &cred->challenge, ps, depth))
		return False;

	if(!smb_io_utime("", &cred->timestamp, ps, depth))
		return False;

	return True;
}

/*******************************************************************
 Inits a DOM_CLNT_INFO2 structure.
********************************************************************/

void init_clnt_info2(DOM_CLNT_INFO2 *clnt,
				const char *logon_srv, const char *comp_name,
				const DOM_CRED *clnt_cred)
{
	DEBUG(5,("make_clnt_info: %d\n", __LINE__));

	init_clnt_srv(&clnt->login, logon_srv, comp_name);

	if (clnt_cred != NULL) {
		clnt->ptr_cred = 1;
		memcpy(&clnt->cred, clnt_cred, sizeof(clnt->cred));
	} else {
		clnt->ptr_cred = 0;
	}
}

/*******************************************************************
 Reads or writes a DOM_CLNT_INFO2 structure.
********************************************************************/

BOOL smb_io_clnt_info2(const char *desc, DOM_CLNT_INFO2 *clnt, prs_struct *ps, int depth)
{
	if (clnt == NULL)
		return False;

	prs_debug(ps, depth, desc, "smb_io_clnt_info2");
	depth++;

	if(!prs_align(ps))
		return False;
	
	if(!smb_io_clnt_srv("", &clnt->login, ps, depth))
		return False;

	if(!prs_align(ps))
		return False;
	
	if(!prs_uint32("ptr_cred", ps, depth, &clnt->ptr_cred))
		return False;
	if(!smb_io_cred("", &clnt->cred, ps, depth))
		return False;

	return True;
}

/*******************************************************************
 Inits a DOM_CLNT_INFO structure.
********************************************************************/

void init_clnt_info(DOM_CLNT_INFO *clnt,
		const char *logon_srv, const char *acct_name,
		uint16 sec_chan, const char *comp_name,
		const DOM_CRED *cred)
{
	DEBUG(5,("make_clnt_info\n"));

	init_log_info(&clnt->login, logon_srv, acct_name, sec_chan, comp_name);
	memcpy(&clnt->cred, cred, sizeof(clnt->cred));
}

/*******************************************************************
 Reads or writes a DOM_CLNT_INFO structure.
********************************************************************/

BOOL smb_io_clnt_info(const char *desc,  DOM_CLNT_INFO *clnt, prs_struct *ps, int depth)
{
	if (clnt == NULL)
		return False;

	prs_debug(ps, depth, desc, "smb_io_clnt_info");
	depth++;

	if(!prs_align(ps))
		return False;
	
	if(!smb_io_log_info("", &clnt->login, ps, depth))
		return False;
	if(!smb_io_cred("", &clnt->cred, ps, depth))
		return False;

	return True;
}

/*******************************************************************
 Inits a DOM_LOGON_ID structure.
********************************************************************/

void init_logon_id(DOM_LOGON_ID *logonid, uint32 log_id_low, uint32 log_id_high)
{
	DEBUG(5,("make_logon_id: %d\n", __LINE__));

	logonid->low  = log_id_low;
	logonid->high = log_id_high;
}

/*******************************************************************
 Reads or writes a DOM_LOGON_ID structure.
********************************************************************/

BOOL smb_io_logon_id(const char *desc, DOM_LOGON_ID *logonid, prs_struct *ps, int depth)
{
	if (logonid == NULL)
		return False;

	prs_debug(ps, depth, desc, "smb_io_logon_id");
	depth++;

	if(!prs_align(ps))
		return False;
	
	if(!prs_uint32("low ", ps, depth, &logonid->low ))
		return False;
	if(!prs_uint32("high", ps, depth, &logonid->high))
		return False;

	return True;
}

/*******************************************************************
 Inits an OWF_INFO structure.
********************************************************************/

void init_owf_info(OWF_INFO *hash, const uint8 data[16])
{
	DEBUG(5,("init_owf_info: %d\n", __LINE__));
	
	if (data != NULL)
		memcpy(hash->data, data, sizeof(hash->data));
	else
		memset((char *)hash->data, '\0', sizeof(hash->data));
}

/*******************************************************************
 Reads or writes an OWF_INFO structure.
********************************************************************/

BOOL smb_io_owf_info(const char *desc, OWF_INFO *hash, prs_struct *ps, int depth)
{
	if (hash == NULL)
		return False;

	prs_debug(ps, depth, desc, "smb_io_owf_info");
	depth++;

	if(!prs_align(ps))
		return False;
	
	if(!prs_uint8s (False, "data", ps, depth, hash->data, 16))
		return False;

	return True;
}

/*******************************************************************
 Reads or writes a DOM_GID structure.
********************************************************************/

BOOL smb_io_gid(const char *desc,  DOM_GID *gid, prs_struct *ps, int depth)
{
	if (gid == NULL)
		return False;

	prs_debug(ps, depth, desc, "smb_io_gid");
	depth++;

	if(!prs_align(ps))
		return False;
	
	if(!prs_uint32("g_rid", ps, depth, &gid->g_rid))
		return False;
	if(!prs_uint32("attr ", ps, depth, &gid->attr))
		return False;

	return True;
}

/*******************************************************************
 Reads or writes an POLICY_HND structure.
********************************************************************/

BOOL smb_io_pol_hnd(const char *desc, POLICY_HND *pol, prs_struct *ps, int depth)
{
	if (pol == NULL)
		return False;

	prs_debug(ps, depth, desc, "smb_io_pol_hnd");
	depth++;

	if(!prs_align(ps))
		return False;

	if(UNMARSHALLING(ps))
		ZERO_STRUCTP(pol);
	
	if (!prs_uint32("data1", ps, depth, &pol->data1))
		return False;
	if (!prs_uint32("data2", ps, depth, &pol->data2))
		return False;
	if (!prs_uint16("data3", ps, depth, &pol->data3))
		return False;
	if (!prs_uint16("data4", ps, depth, &pol->data4))
		return False;
	if(!prs_uint8s (False, "data5", ps, depth, pol->data5, sizeof(pol->data5)))
		return False;

	return True;
}

/*******************************************************************
 Create a UNISTR3.
********************************************************************/

void init_unistr3(UNISTR3 *str, const char *buf)
{
	if (buf == NULL) {
		str->uni_str_len=0;
		str->str.buffer = NULL;
		return;
	}

	str->uni_str_len = strlen(buf) + 1;

	str->str.buffer = TALLOC_ZERO_ARRAY(get_talloc_ctx(), uint16, str->uni_str_len);
	if (str->str.buffer == NULL)
		smb_panic("init_unistr3: malloc fail\n");

	rpcstr_push((char *)str->str.buffer, buf, str->uni_str_len * sizeof(uint16), STR_TERMINATE);
}

/*******************************************************************
 Reads or writes a UNISTR3 structure.
********************************************************************/

BOOL smb_io_unistr3(const char *desc, UNISTR3 *name, prs_struct *ps, int depth)
{
	if (name == NULL)
		return False;

	prs_debug(ps, depth, desc, "smb_io_unistr3");
	depth++;

	if(!prs_align(ps))
		return False;
	
	if(!prs_uint32("uni_str_len", ps, depth, &name->uni_str_len))
		return False;
		
	/* we're done if there is no string */
	
	if ( name->uni_str_len == 0 )
		return True;

	/* don't know if len is specified by uni_str_len member... */
	/* assume unicode string is unicode-null-terminated, instead */

	if(!prs_unistr3(True, "unistr", name, ps, depth))
		return False;

	return True;
}

/*******************************************************************
 Stream a uint64_struct
 ********************************************************************/
BOOL prs_uint64(const char *name, prs_struct *ps, int depth, UINT64_S *data64)
{
	return prs_uint32(name, ps, depth+1, &data64->low) &&
		prs_uint32(name, ps, depth+1, &data64->high);
}

/*******************************************************************
reads or writes a BUFHDR2 structure.
********************************************************************/
BOOL smb_io_bufhdr2(const char *desc, BUFHDR2 *hdr, prs_struct *ps, int depth)
{
	prs_debug(ps, depth, desc, "smb_io_bufhdr2");
	depth++;

	prs_align(ps);
	prs_uint32("info_level", ps, depth, &(hdr->info_level));
	prs_uint32("length    ", ps, depth, &(hdr->length    ));
	prs_uint32("buffer    ", ps, depth, &(hdr->buffer    ));

	return True;
}

/*******************************************************************
reads or writes a BUFHDR4 structure.
********************************************************************/
BOOL smb_io_bufhdr4(const char *desc, BUFHDR4 *hdr, prs_struct *ps, int depth)
{
	prs_debug(ps, depth, desc, "smb_io_bufhdr4");
	depth++;

	prs_align(ps);
	prs_uint32("size", ps, depth, &hdr->size);
	prs_uint32("buffer", ps, depth, &hdr->buffer);

	return True;
}

/*******************************************************************
reads or writes a RPC_DATA_BLOB structure.
********************************************************************/

BOOL smb_io_rpc_blob(const char *desc, RPC_DATA_BLOB *blob, prs_struct *ps, int depth)
{
	prs_debug(ps, depth, desc, "smb_io_rpc_blob");
	depth++;

	prs_align(ps);
	if ( !prs_uint32("buf_len", ps, depth, &blob->buf_len) )
		return False;

	if ( blob->buf_len == 0 )
		return True;

	if (UNMARSHALLING(ps)) {
		blob->buffer = PRS_ALLOC_MEM(ps, uint8, blob->buf_len);
		if (!blob->buffer) {
			return False;
		}
	}

	if ( !prs_uint8s(True, "buffer", ps, depth, blob->buffer, blob->buf_len) )
		return False;

	return True;
}

/*******************************************************************
creates a UNIHDR structure.
********************************************************************/

BOOL make_uni_hdr(UNIHDR *hdr, int len)
{
	if (hdr == NULL)
	{
		return False;
	}
	hdr->uni_str_len = 2 * len;
	hdr->uni_max_len = 2 * len;
	hdr->buffer      = len != 0 ? 1 : 0;

	return True;
}

/*******************************************************************
creates a BUFHDR2 structure.
********************************************************************/
BOOL make_bufhdr2(BUFHDR2 *hdr, uint32 info_level, uint32 length, uint32 buffer)
{
	hdr->info_level = info_level;
	hdr->length     = length;
	hdr->buffer     = buffer;

	return True;
}

/*******************************************************************
return the length of a UNISTR string.
********************************************************************/  

uint32 str_len_uni(UNISTR *source)
{
 	uint32 i=0;

	if (!source->buffer)
		return 0;

	while (source->buffer[i])
		i++;

	return i;
}


