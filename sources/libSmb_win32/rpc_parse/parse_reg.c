/*
 *  Unix SMB/CIFS implementation.
 *  RPC Pipe client / server routines
 *  Copyright (C) Andrew Tridgell              1992-1997,
 *  Copyright (C) Luke Kenneth Casson Leighton 1996-1997,
 *  Copyright (C) Paul Ashton                       1997.
 *  Copyright (C) Marc Jacobsen                     1999.
 *  Copyright (C) Simo Sorce                        2000.
 *  Copyright (C) Jeremy Cooper                     2004
 *  Copyright (C) Gerald Carter                     2002-2005.
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

/*******************************************************************
 Fill in a REGVAL_BUFFER for the data given a REGISTRY_VALUE
 *******************************************************************/

uint32 reg_init_regval_buffer( REGVAL_BUFFER *buf2, REGISTRY_VALUE *val )
{
	uint32		real_size = 0;
	
	if ( !buf2 || !val )
		return 0;
		
	real_size = regval_size(val);
	init_regval_buffer( buf2, (unsigned char*)regval_data_p(val), real_size );

	return real_size;
}

/*******************************************************************
 Inits a hive connect request structure
********************************************************************/

void init_reg_q_open_hive( REG_Q_OPEN_HIVE *q_o, uint32 access_desired )
{
	
	q_o->server = TALLOC_P( get_talloc_ctx(), uint16);
	if (!q_o->server) {
		smb_panic("init_reg_q_open_hive: talloc fail.\n");
		return;
	}
	*q_o->server = 0x1;
	
	q_o->access = access_desired;
}

/*******************************************************************
Marshalls a hive connect request
********************************************************************/

BOOL reg_io_q_open_hive(const char *desc, REG_Q_OPEN_HIVE *q_u,
                        prs_struct *ps, int depth)
{
	prs_debug(ps, depth, desc, "reg_io_q_open_hive");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!prs_pointer("server", ps, depth, (void**)&q_u->server, sizeof(uint16), (PRS_POINTER_CAST)prs_uint16))
		return False;

	if(!prs_align(ps))
		return False;
	if(!prs_uint32("access", ps, depth, &q_u->access))
		return False;

	return True;
}


/*******************************************************************
Unmarshalls a hive connect response
********************************************************************/

BOOL reg_io_r_open_hive(const char *desc,  REG_R_OPEN_HIVE *r_u,
                        prs_struct *ps, int depth)
{
	if ( !r_u )
		return False;

	prs_debug(ps, depth, desc, "reg_io_r_open_hive");
	depth++;

	if(!prs_align(ps))
		return False;
	
	if(!smb_io_pol_hnd("", &r_u->pol, ps, depth))
		return False;

	if(!prs_werror("status", ps, depth, &r_u->status))
		return False;

	return True;
}

/*******************************************************************
 Inits a structure.
********************************************************************/

void init_reg_q_flush_key(REG_Q_FLUSH_KEY *q_u, POLICY_HND *pol)
{
	memcpy(&q_u->pol, pol, sizeof(q_u->pol));
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL reg_io_q_flush_key(const char *desc,  REG_Q_FLUSH_KEY *q_u, prs_struct *ps, int depth)
{
	if ( !q_u )
		return False;

	prs_debug(ps, depth, desc, "reg_io_q_flush_key");
	depth++;

	if(!prs_align(ps))
		return False;
	
	if(!smb_io_pol_hnd("", &q_u->pol, ps, depth))
		return False;

	return True;
}

/*******************************************************************
Unmarshalls a registry key flush response
********************************************************************/

BOOL reg_io_r_flush_key(const char *desc,  REG_R_FLUSH_KEY *r_u,
                        prs_struct *ps, int depth)
{
	if ( !r_u )
		return False;

	prs_debug(ps, depth, desc, "reg_io_r_flush_key");
	depth++;

	if(!prs_align(ps))
		return False;
	
	if(!prs_werror("status", ps, depth, &r_u->status))
		return False;

	return True;
}

/*******************************************************************
reads or writes SEC_DESC_BUF and SEC_DATA structures.
********************************************************************/

static BOOL reg_io_hdrbuf_sec(uint32 ptr, uint32 *ptr3, BUFHDR *hdr_sec,
                              SEC_DESC_BUF *data, prs_struct *ps, int depth)
{
	if (ptr != 0) {
		uint32 hdr_offset;
		uint32 old_offset;
		if(!smb_io_hdrbuf_pre("hdr_sec", hdr_sec, ps, depth,
		                      &hdr_offset))
			return False;

		old_offset = prs_offset(ps);

		if (ptr3 != NULL) {
			if(!prs_uint32("ptr3", ps, depth, ptr3))
				return False;
		}

		if (ptr3 == NULL || *ptr3 != 0) {
 			/* JRA - this next line is probably wrong... */
			if(!sec_io_desc_buf("data   ", &data, ps, depth))
				return False;
		}

		if(!smb_io_hdrbuf_post("hdr_sec", hdr_sec, ps, depth,
		                       hdr_offset, data->max_len, data->len))
				return False;
		if(!prs_set_offset(ps, old_offset + data->len +
		                       sizeof(uint32) * ((ptr3 != NULL) ? 5 : 3)))
			return False;

		if(!prs_align(ps))
			return False;
	}

	return True;
}

/*******************************************************************
 Inits a registry key create request
********************************************************************/

void init_reg_q_create_key_ex(REG_Q_CREATE_KEY_EX *q_c, POLICY_HND *hnd,
                           char *name, char *key_class, uint32 access_desired,
                           SEC_DESC_BUF *sec_buf)
{
	ZERO_STRUCTP(q_c);

	memcpy(&q_c->handle, hnd, sizeof(q_c->handle));


	init_unistr4( &q_c->name, name, UNI_STR_TERMINATE );
	init_unistr4( &q_c->key_class, key_class, UNI_STR_TERMINATE );

	q_c->access = access_desired;

	q_c->sec_info = TALLOC_P( get_talloc_ctx(), uint32 );
	if (!q_c->sec_info) {
		smb_panic("init_reg_q_create_key_ex: talloc fail\n");
		return;
	}
	*q_c->sec_info = DACL_SECURITY_INFORMATION | SACL_SECURITY_INFORMATION;

	q_c->data = sec_buf;
	q_c->ptr2 = 1;
	init_buf_hdr(&q_c->hdr_sec, sec_buf->len, sec_buf->len);
	q_c->ptr3 = 1;
	q_c->disposition = TALLOC_P( get_talloc_ctx(), uint32 );
	if (!q_c->disposition) {
		smb_panic("init_reg_q_create_key_ex: talloc fail\n");
		return;
	}
}

/*******************************************************************
Marshalls a registry key create request
********************************************************************/

BOOL reg_io_q_create_key_ex(const char *desc,  REG_Q_CREATE_KEY_EX *q_u,
                         prs_struct *ps, int depth)
{
	if ( !q_u )
		return False;

	prs_debug(ps, depth, desc, "reg_io_q_create_key_ex");
	depth++;

	if(!prs_align(ps))
		return False;
	
	if(!smb_io_pol_hnd("", &q_u->handle, ps, depth))
		return False;

	if(!prs_unistr4 ("name", ps, depth, &q_u->name))
		return False;
	if(!prs_align(ps))
		return False;

	if(!prs_unistr4 ("key_class", ps, depth, &q_u->key_class))
		return False;
	if(!prs_align(ps))
		return False;

	if(!prs_uint32("options", ps, depth, &q_u->options))
		return False;
	if(!prs_uint32("access", ps, depth, &q_u->access))
		return False;

	if(!prs_pointer("sec_info", ps, depth, (void**)&q_u->sec_info, sizeof(uint32), (PRS_POINTER_CAST)prs_uint32))
		return False;

	if ( q_u->sec_info ) {
		if(!prs_uint32("ptr2", ps, depth, &q_u->ptr2))
			return False;
		if(!reg_io_hdrbuf_sec(q_u->ptr2, &q_u->ptr3, &q_u->hdr_sec, q_u->data, ps, depth))
			return False;
	}

	if(!prs_pointer("disposition", ps, depth, (void**)&q_u->disposition, sizeof(uint32), (PRS_POINTER_CAST)prs_uint32))
		return False;

	return True;
}

/*******************************************************************
Unmarshalls a registry key create response
********************************************************************/

BOOL reg_io_r_create_key_ex(const char *desc,  REG_R_CREATE_KEY_EX *r_u,
                         prs_struct *ps, int depth)
{
	if ( !r_u )
		return False;

	prs_debug(ps, depth, desc, "reg_io_r_create_key_ex");
	depth++;

	if(!prs_align(ps))
		return False;
	
	if(!smb_io_pol_hnd("", &r_u->handle, ps, depth))
		return False;
	if(!prs_uint32("disposition", ps, depth, &r_u->disposition))
		return False;

	if(!prs_werror("status", ps, depth, &r_u->status))
		return False;

	return True;
}


/*******************************************************************
 Inits a structure.
********************************************************************/

void init_reg_q_delete_val(REG_Q_DELETE_VALUE *q_c, POLICY_HND *hnd,
                           char *name)
{
	ZERO_STRUCTP(q_c);

	memcpy(&q_c->handle, hnd, sizeof(q_c->handle));
	init_unistr4(&q_c->name, name, UNI_STR_TERMINATE);
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL reg_io_q_delete_value(const char *desc, REG_Q_DELETE_VALUE *q_u,
                         prs_struct *ps, int depth)
{
	if ( !q_u )
		return False;

	prs_debug(ps, depth, desc, "reg_io_q_delete_value");
	depth++;

	if(!prs_align(ps))
		return False;
	
	if(!smb_io_pol_hnd("", &q_u->handle, ps, depth))
		return False;

	if(!prs_unistr4("name", ps, depth, &q_u->name))
		return False;

	return True;
}


/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL reg_io_r_delete_value(const char *desc,  REG_R_DELETE_VALUE *r_u,
                         prs_struct *ps, int depth)
{
	if ( !r_u )
		return False;

	prs_debug(ps, depth, desc, "reg_io_r_delete_value");
	depth++;

	if(!prs_align(ps))
		return False;
	
	if(!prs_werror("status", ps, depth, &r_u->status))
		return False;

	return True;
}

/*******************************************************************
 Inits a structure.
********************************************************************/

void init_reg_q_delete_key(REG_Q_DELETE_KEY *q_c, POLICY_HND *hnd,
                           char *name)
{
	ZERO_STRUCTP(q_c);

	memcpy(&q_c->handle, hnd, sizeof(q_c->handle));

	init_unistr4(&q_c->name, name, UNI_STR_TERMINATE);
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL reg_io_q_delete_key(const char *desc,  REG_Q_DELETE_KEY *q_u,
                         prs_struct *ps, int depth)
{
	if ( !q_u )
		return False;

	prs_debug(ps, depth, desc, "reg_io_q_delete_key");
	depth++;

	if(!prs_align(ps))
		return False;
	
	if(!smb_io_pol_hnd("", &q_u->handle, ps, depth))
		return False;

	if(!prs_unistr4("", ps, depth, &q_u->name))
		return False;

	return True;
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL reg_io_r_delete_key(const char *desc,  REG_R_DELETE_KEY *r_u, prs_struct *ps, int depth)
{
	if ( !r_u )
		return False;

	prs_debug(ps, depth, desc, "reg_io_r_delete_key");
	depth++;

	if(!prs_align(ps))
		return False;
	
	if(!prs_werror("status", ps, depth, &r_u->status))
		return False;

	return True;
}

/*******************************************************************
 Inits a structure.
********************************************************************/

void init_reg_q_query_key(REG_Q_QUERY_KEY *q_o, POLICY_HND *hnd, const char *key_class)
{
	ZERO_STRUCTP(q_o);

	memcpy(&q_o->pol, hnd, sizeof(q_o->pol));
	init_unistr4(&q_o->key_class, key_class, UNI_STR_TERMINATE);
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL reg_io_q_query_key(const char *desc,  REG_Q_QUERY_KEY *q_u, prs_struct *ps, int depth)
{
	if ( !q_u )
		return False;

	prs_debug(ps, depth, desc, "reg_io_q_query_key");
	depth++;

	if(!prs_align(ps))
		return False;
	
	if(!smb_io_pol_hnd("", &q_u->pol, ps, depth))
		return False;
	if(!prs_unistr4("key_class", ps, depth, &q_u->key_class))
		return False;

	return True;
}


/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL reg_io_r_query_key(const char *desc,  REG_R_QUERY_KEY *r_u, prs_struct *ps, int depth)
{
	if ( !r_u )
		return False;

	prs_debug(ps, depth, desc, "reg_io_r_query_key");
	depth++;

	if(!prs_align(ps))
		return False;
	
	if(!prs_unistr4("key_class", ps, depth, &r_u->key_class))
		return False;

	if(!prs_align(ps))
		return False;

	if(!prs_uint32("num_subkeys   ", ps, depth, &r_u->num_subkeys))
		return False;
	if(!prs_uint32("max_subkeylen ", ps, depth, &r_u->max_subkeylen))
		return False;
	if(!prs_uint32("reserved      ", ps, depth, &r_u->reserved))
		return False;
	if(!prs_uint32("num_values    ", ps, depth, &r_u->num_values))
		return False;
	if(!prs_uint32("max_valnamelen", ps, depth, &r_u->max_valnamelen))
		return False;
	if(!prs_uint32("max_valbufsize", ps, depth, &r_u->max_valbufsize))
		return False;
	if(!prs_uint32("sec_desc      ", ps, depth, &r_u->sec_desc))
		return False;
	if(!smb_io_time("mod_time     ", &r_u->mod_time, ps, depth))
		return False;

	if(!prs_werror("status", ps, depth, &r_u->status))
		return False;

	return True;
}

/*******************************************************************
 Inits a structure.
********************************************************************/

void init_reg_q_getversion(REG_Q_GETVERSION *q_o, POLICY_HND *hnd)
{
	memcpy(&q_o->pol, hnd, sizeof(q_o->pol));
}


/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL reg_io_q_getversion(const char *desc,  REG_Q_GETVERSION *q_u, prs_struct *ps, int depth)
{
	if ( !q_u )
		return False;

	prs_debug(ps, depth, desc, "reg_io_q_getversion");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!smb_io_pol_hnd("", &q_u->pol, ps, depth))
		return False;

	return True;
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL reg_io_r_getversion(const char *desc,  REG_R_GETVERSION *r_u, prs_struct *ps, int depth)
{
	if ( !r_u )
		return False;

	prs_debug(ps, depth, desc, "reg_io_r_getversion");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!prs_uint32("win_version", ps, depth, &r_u->win_version))
		return False;
	if(!prs_werror("status" , ps, depth, &r_u->status))
		return False;

	return True;
}


/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL reg_io_q_restore_key(const char *desc,  REG_Q_RESTORE_KEY *q_u, prs_struct *ps, int depth)
{
	if ( !q_u )
		return False;

	prs_debug(ps, depth, desc, "reg_io_q_restore_key");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!smb_io_pol_hnd("", &q_u->pol, ps, depth))
		return False;

	if(!prs_unistr4("filename", ps, depth, &q_u->filename))
		return False;

	if(!prs_uint32("flags", ps, depth, &q_u->flags))
		return False;

	return True;
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL reg_io_r_restore_key(const char *desc,  REG_R_RESTORE_KEY *r_u, prs_struct *ps, int depth)
{
	if ( !r_u )
		return False;

	prs_debug(ps, depth, desc, "reg_io_r_restore_key");
	depth++;

	if(!prs_align(ps))
		return False;
	
	if(!prs_werror("status" , ps, depth, &r_u->status))
		return False;

	return True;
}

/*******************************************************************
********************************************************************/

void init_q_reg_save_key( REG_Q_SAVE_KEY *q_u, POLICY_HND *handle, const char *fname )
{
	memcpy(&q_u->pol, handle, sizeof(q_u->pol));
	init_unistr4( &q_u->filename, fname, UNI_STR_TERMINATE );
	q_u->sec_attr = NULL;
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL reg_io_q_save_key(const char *desc,  REG_Q_SAVE_KEY *q_u, prs_struct *ps, int depth)
{
	if ( !q_u )
		return False;

	prs_debug(ps, depth, desc, "reg_io_q_save_key");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!smb_io_pol_hnd("", &q_u->pol, ps, depth))
		return False;

	if(!prs_unistr4("filename", ps, depth, &q_u->filename))
		return False;

#if 0	/* reg_io_sec_attr() */
	if(!prs_uint32("unknown", ps, depth, &q_u->unknown))
		return False;
#endif

	return True;
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL reg_io_r_save_key(const char *desc,  REG_R_SAVE_KEY *r_u, prs_struct *ps, int depth)
{
	if ( !r_u )
		return False;

	prs_debug(ps, depth, desc, "reg_io_r_save_key");
	depth++;

	if(!prs_align(ps))
		return False;
	
	if(!prs_werror("status" , ps, depth, &r_u->status))
		return False;

	return True;
}

/*******************************************************************
 Inits an REG_Q_CLOSE structure.
********************************************************************/

void init_reg_q_close(REG_Q_CLOSE *q_c, POLICY_HND *hnd)
{
	DEBUG(5,("init_reg_q_close\n"));

	memcpy(&q_c->pol, hnd, sizeof(q_c->pol));
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL reg_io_q_close(const char *desc,  REG_Q_CLOSE *q_u, prs_struct *ps, int depth)
{
	if (q_u == NULL)
		return False;

	prs_debug(ps, depth, desc, "reg_io_q_close");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!smb_io_pol_hnd("", &q_u->pol, ps, depth))
		return False;
	if(!prs_align(ps))
		return False;

	return True;
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL reg_io_r_close(const char *desc,  REG_R_CLOSE *r_u, prs_struct *ps, int depth)
{
	if ( !r_u )
		return False;

	prs_debug(ps, depth, desc, "reg_io_r_close");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!smb_io_pol_hnd("", &r_u->pol, ps, depth))
		return False;
	if(!prs_align(ps))
		return False;

	if(!prs_werror("status", ps, depth, &r_u->status))
		return False;

	return True;
}

/*******************************************************************
makes a structure.
********************************************************************/

void init_reg_q_set_key_sec(REG_Q_SET_KEY_SEC *q_u, POLICY_HND *pol,
                            uint32 sec_info, SEC_DESC_BUF *sec_desc_buf)
{
	memcpy(&q_u->handle, pol, sizeof(q_u->handle));

	q_u->sec_info = sec_info;

	q_u->ptr = 1;
	init_buf_hdr(&q_u->hdr_sec, sec_desc_buf->len, sec_desc_buf->len);
	q_u->data = sec_desc_buf;
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL reg_io_q_set_key_sec(const char *desc,  REG_Q_SET_KEY_SEC *q_u, prs_struct *ps, int depth)
{
	if ( !q_u )
		return False;

	prs_debug(ps, depth, desc, "reg_io_q_set_key_sec");
	depth++;

	if(!prs_align(ps))
		return False;
	
	if(!smb_io_pol_hnd("", &q_u->handle, ps, depth))
		return False;

	if(!prs_uint32("sec_info", ps, depth, &q_u->sec_info))
		return False;
	if(!prs_uint32("ptr    ", ps, depth, &q_u->ptr))
		return False;

	if(!reg_io_hdrbuf_sec(q_u->ptr, NULL, &q_u->hdr_sec, q_u->data, ps, depth))
		return False;

	return True;
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL reg_io_r_set_key_sec(const char *desc, REG_R_SET_KEY_SEC *q_u, prs_struct *ps, int depth)
{
	if ( !q_u )
		return False;

	prs_debug(ps, depth, desc, "reg_io_r_set_key_sec");
	depth++;

	if(!prs_align(ps))
		return False;
	
	if(!prs_werror("status", ps, depth, &q_u->status))
		return False;

	return True;
}


/*******************************************************************
makes a structure.
********************************************************************/

void init_reg_q_get_key_sec(REG_Q_GET_KEY_SEC *q_u, POLICY_HND *pol, 
                            uint32 sec_info, uint32 sec_buf_size,
                            SEC_DESC_BUF *psdb)
{
	memcpy(&q_u->handle, pol, sizeof(q_u->handle));

	q_u->sec_info = sec_info;

	q_u->ptr = psdb != NULL ? 1 : 0;
	q_u->data = psdb;

	init_buf_hdr(&q_u->hdr_sec, sec_buf_size, 0);
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL reg_io_q_get_key_sec(const char *desc,  REG_Q_GET_KEY_SEC *q_u, prs_struct *ps, int depth)
{
	if ( !q_u )
		return False;

	prs_debug(ps, depth, desc, "reg_io_q_get_key_sec");
	depth++;

	if(!prs_align(ps))
		return False;
	
	if(!smb_io_pol_hnd("", &q_u->handle, ps, depth))
		return False;

	if(!prs_uint32("sec_info", ps, depth, &q_u->sec_info))
		return False;
	if(!prs_uint32("ptr     ", ps, depth, &q_u->ptr))
		return False;

	if(!reg_io_hdrbuf_sec(q_u->ptr, NULL, &q_u->hdr_sec, q_u->data, ps, depth))
		return False;

	return True;
}

#if 0
/*******************************************************************
makes a structure.
********************************************************************/
 void init_reg_r_get_key_sec(REG_R_GET_KEY_SEC *r_i, POLICY_HND *pol, 
				uint32 buf_len, uint8 *buf,
				NTSTATUS status)
{
	r_i->ptr = 1;
	init_buf_hdr(&r_i->hdr_sec, buf_len, buf_len);
	init_sec_desc_buf(r_i->data, buf_len, 1);

	r_i->status = status; /* 0x0000 0000 or 0x0000 007a */
}
#endif 

/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL reg_io_r_get_key_sec(const char *desc,  REG_R_GET_KEY_SEC *q_u, prs_struct *ps, int depth)
{
	if ( !q_u )
		return False;

	prs_debug(ps, depth, desc, "reg_io_r_get_key_sec");
	depth++;

	if(!prs_align(ps))
		return False;
	
	if(!prs_uint32("ptr      ", ps, depth, &q_u->ptr))
		return False;

	if (q_u->ptr != 0) {
		if(!smb_io_hdrbuf("", &q_u->hdr_sec, ps, depth))
			return False;
		if(!sec_io_desc_buf("", &q_u->data, ps, depth))
			return False;
		if(!prs_align(ps))
			return False;
	}

	if(!prs_werror("status", ps, depth, &q_u->status))
		return False;

	return True;
}

/*******************************************************************
makes a structure.
********************************************************************/

BOOL init_reg_q_query_value(REG_Q_QUERY_VALUE *q_u, POLICY_HND *pol, const char *val_name,
                     REGVAL_BUFFER *value_output)
{
        if (q_u == NULL)
                return False;

        q_u->pol = *pol;

        init_unistr4(&q_u->name, val_name, UNI_STR_TERMINATE);

        q_u->ptr_reserved = 1;
        q_u->ptr_buf = 1;

        q_u->ptr_bufsize = 1;
        q_u->bufsize = value_output->buf_max_len;
        q_u->buf_unk = 0;

        q_u->unk1 = 0;
        q_u->ptr_buflen = 1;
        q_u->buflen = value_output->buf_max_len; 

        q_u->ptr_buflen2 = 1;
        q_u->buflen2 = 0;

        return True;
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL reg_io_q_query_value(const char *desc,  REG_Q_QUERY_VALUE *q_u, prs_struct *ps, int depth)
{
	if ( !q_u )
		return False;

	prs_debug(ps, depth, desc, "reg_io_q_query_value");
	depth++;

	if(!prs_align(ps))
		return False;
	
	if(!smb_io_pol_hnd("", &q_u->pol, ps, depth))
		return False;
	if(!prs_unistr4("name", ps, depth, &q_u->name))
		return False;

	if(!prs_align(ps))
		return False;
	
	if(!prs_uint32("ptr_reserved", ps, depth, &(q_u->ptr_reserved)))
		return False;

	if(!prs_uint32("ptr_buf", ps, depth, &(q_u->ptr_buf)))
		return False;

	if(q_u->ptr_buf) {
		if(!prs_uint32("ptr_bufsize", ps, depth, &(q_u->ptr_bufsize)))
			return False;
		if(!prs_uint32("bufsize", ps, depth, &(q_u->bufsize)))
			return False;
		if(!prs_uint32("buf_unk", ps, depth, &(q_u->buf_unk)))
			return False;
	}

	if(!prs_uint32("unk1", ps, depth, &(q_u->unk1)))
		return False;

	if(!prs_uint32("ptr_buflen", ps, depth, &(q_u->ptr_buflen)))
		return False;

	if (q_u->ptr_buflen) {
		if(!prs_uint32("buflen", ps, depth, &(q_u->buflen)))
			return False;
		if(!prs_uint32("ptr_buflen2", ps, depth, &(q_u->ptr_buflen2)))
			return False;
		if(!prs_uint32("buflen2", ps, depth, &(q_u->buflen2)))
			return False;
	}

 	return True;
}

/*******************************************************************
 Inits a structure.
 New version to replace older init_reg_r_query_value()
********************************************************************/

BOOL init_reg_r_query_value(uint32 include_keyval, REG_R_QUERY_VALUE *r_u,
		     REGISTRY_VALUE *val, WERROR status)
{
	uint32			buf_len = 0;
	REGVAL_BUFFER		buf2;
		
	if( !r_u || !val )
		return False;
	
	r_u->type = TALLOC_P( get_talloc_ctx(), uint32 );
	if (!r_u->type) {
		return False;
	}
	*r_u->type = val->type;

	buf_len = reg_init_regval_buffer( &buf2, val );
	
	r_u->buf_max_len = TALLOC_P( get_talloc_ctx(), uint32 );
	if (!r_u->buf_max_len) {
		return False;
	}
	*r_u->buf_max_len = buf_len;

	r_u->buf_len = TALLOC_P( get_talloc_ctx(), uint32 );
	if (!r_u->buf_len) {
		return False;
	}
	*r_u->buf_len = buf_len;
	
	/* if include_keyval is not set, don't send the key value, just
	   the buflen data. probably used by NT5 to allocate buffer space - SK */

	if ( include_keyval ) {
		r_u->value = TALLOC_P( get_talloc_ctx(), REGVAL_BUFFER );
		if (!r_u->value) {
			return False;
		}
		/* steal the memory */
		*r_u->value = buf2;
	}

	r_u->status = status;

	return True;
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL reg_io_r_query_value(const char *desc, REG_R_QUERY_VALUE *r_u, prs_struct *ps, int depth)
{
	if ( !r_u )
		return False;

	prs_debug(ps, depth, desc, "reg_io_r_query_value");
	depth++;

	if(!prs_align(ps))
		return False;
	
	if ( !prs_pointer("type", ps, depth, (void**)&r_u->type, sizeof(uint32), (PRS_POINTER_CAST)prs_uint32))
		return False;

	if ( !prs_pointer("value", ps, depth, (void**)&r_u->value, sizeof(REGVAL_BUFFER), (PRS_POINTER_CAST)smb_io_regval_buffer))
		return False;
	if(!prs_align(ps))
		return False;

	if ( !prs_pointer("buf_max_len", ps, depth, (void**)&r_u->buf_max_len, sizeof(uint32), (PRS_POINTER_CAST)prs_uint32))
		return False;
	if ( !prs_pointer("buf_len", ps, depth, (void**)&r_u->buf_len, sizeof(uint32), (PRS_POINTER_CAST)prs_uint32))
		return False;

	if(!prs_werror("status", ps, depth, &r_u->status))
		return False;

 	return True;
}

/*******************************************************************
makes a structure.
********************************************************************/

void init_reg_q_enum_val(REG_Q_ENUM_VALUE *q_u, POLICY_HND *pol,
				uint32 val_idx,
				uint32 max_name_len, uint32 max_buf_len)
{
	ZERO_STRUCTP(q_u);

	memcpy(&q_u->pol, pol, sizeof(q_u->pol));

	q_u->val_index = val_idx;

	q_u->name.size = max_name_len*2;
	q_u->name.string = TALLOC_ZERO_P( get_talloc_ctx(), UNISTR2 );
	if (!q_u->name.string) {
		smb_panic("init_reg_q_enum_val: talloc fail\n");
		return;
	}
	q_u->name.string->uni_max_len = max_name_len;
	
	q_u->type = TALLOC_P( get_talloc_ctx(), uint32 );
	if (!q_u->type) {
		smb_panic("init_reg_q_enum_val: talloc fail\n");
		return;
	}
	*q_u->type = 0x0;

	q_u->value = TALLOC_ZERO_P( get_talloc_ctx(), REGVAL_BUFFER );
	if (!q_u->value) {
		smb_panic("init_reg_q_enum_val: talloc fail\n");
		return;
	}
		
	q_u->value->buf_max_len = max_buf_len;

	q_u->buffer_len = TALLOC_P( get_talloc_ctx(), uint32 );
	if (!q_u->buffer_len) {
		smb_panic("init_reg_q_enum_val: talloc fail\n");
		return;
	}
	*q_u->buffer_len = max_buf_len;

	q_u->name_len = TALLOC_P( get_talloc_ctx(), uint32 );
	if (!q_u->name_len) {
		smb_panic("init_reg_q_enum_val: talloc fail\n");
		return;
	}
	*q_u->name_len = 0x0;
}

/*******************************************************************
makes a structure.
********************************************************************/

void init_reg_r_enum_val(REG_R_ENUM_VALUE *r_u, REGISTRY_VALUE *val )
{
	uint32 real_size;
	
	ZERO_STRUCTP(r_u);

	/* value name */

	DEBUG(10,("init_reg_r_enum_val: Valuename => [%s]\n", val->valuename));
	
	init_unistr4( &r_u->name, val->valuename, UNI_STR_TERMINATE);
		
	/* type */
	
	r_u->type = TALLOC_P( get_talloc_ctx(), uint32 );
	if (!r_u->type) {
		smb_panic("init_reg_r_enum_val: talloc fail\n");
		return;
	}
	*r_u->type = val->type;

	/* REG_SZ & REG_MULTI_SZ must be converted to UNICODE */
	
	r_u->value = TALLOC_P( get_talloc_ctx(), REGVAL_BUFFER );
	if (!r_u->value) {
		smb_panic("init_reg_r_enum_val: talloc fail\n");
		return;
	}
	real_size = reg_init_regval_buffer( r_u->value, val );
	
	/* lengths */

	r_u->buffer_len1 = TALLOC_P( get_talloc_ctx(), uint32 );
	if (!r_u->buffer_len1) {
		smb_panic("init_reg_r_enum_val: talloc fail\n");
		return;
	}
	*r_u->buffer_len1 = real_size;
	r_u->buffer_len2 = TALLOC_P( get_talloc_ctx(), uint32 );
	if (!r_u->buffer_len2) {
		smb_panic("init_reg_r_enum_val: talloc fail\n");
		return;
	}
	*r_u->buffer_len2 = real_size;
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL reg_io_q_enum_val(const char *desc,  REG_Q_ENUM_VALUE *q_u, prs_struct *ps, int depth)
{
	if (q_u == NULL)
		return False;

	prs_debug(ps, depth, desc, "reg_io_q_enum_val");
	depth++;

	if(!prs_align(ps))
		return False;
	
	if(!smb_io_pol_hnd("", &q_u->pol, ps, depth))
		return False;
	
	if(!prs_uint32("val_index", ps, depth, &q_u->val_index))
		return False;
		
	if(!prs_unistr4("name", ps, depth, &q_u->name ))
		return False;
	if(!prs_align(ps))
		return False;

	if(!prs_pointer("type", ps, depth, (void**)&q_u->type, sizeof(uint32), (PRS_POINTER_CAST)prs_uint32))
		return False;

	if ( !prs_pointer("value", ps, depth, (void**)&q_u->value, sizeof(REGVAL_BUFFER), (PRS_POINTER_CAST)smb_io_regval_buffer))
		return False;
	if(!prs_align(ps))
		return False;

	if(!prs_pointer("buffer_len", ps, depth, (void**)&q_u->buffer_len, sizeof(uint32), (PRS_POINTER_CAST)prs_uint32))
		return False;
	if(!prs_pointer("name_len", ps, depth, (void**)&q_u->name_len, sizeof(uint32), (PRS_POINTER_CAST)prs_uint32))
		return False;

	return True;
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL reg_io_r_enum_val(const char *desc,  REG_R_ENUM_VALUE *r_u, prs_struct *ps, int depth)
{
	if ( !r_u )
		return False;

	prs_debug(ps, depth, desc, "reg_io_r_enum_val");
	depth++;

	if(!prs_align(ps))
		return False;
	
	if(!prs_unistr4("name", ps, depth, &r_u->name ))
		return False;
	if(!prs_align(ps))
		return False;

	if(!prs_pointer("type", ps, depth, (void**)&r_u->type, sizeof(uint32), (PRS_POINTER_CAST)prs_uint32))
		return False;

	if ( !prs_pointer("value", ps, depth, (void**)&r_u->value, sizeof(REGVAL_BUFFER), (PRS_POINTER_CAST)smb_io_regval_buffer))
		return False;
	if(!prs_align(ps))
		return False;

	if(!prs_pointer("buffer_len1", ps, depth, (void**)&r_u->buffer_len1, sizeof(uint32), (PRS_POINTER_CAST)prs_uint32))
		return False;
	if(!prs_pointer("buffer_len2", ps, depth, (void**)&r_u->buffer_len2, sizeof(uint32), (PRS_POINTER_CAST)prs_uint32))
		return False;

	if(!prs_werror("status", ps, depth, &r_u->status))
		return False;

	return True;
}

/*******************************************************************
makes a structure.
********************************************************************/

void init_reg_q_set_val(REG_Q_SET_VALUE *q_u, POLICY_HND *pol,
				char *val_name, uint32 type,
				RPC_DATA_BLOB *val)
{
	ZERO_STRUCTP(q_u);

	memcpy(&q_u->handle, pol, sizeof(q_u->handle));

	init_unistr4(&q_u->name, val_name, UNI_STR_TERMINATE);
	
	q_u->type      = type;
	q_u->value     = *val;
	q_u->size      = val->buf_len;
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL reg_io_q_set_value(const char *desc,  REG_Q_SET_VALUE *q_u, prs_struct *ps, int depth)
{
	if (q_u == NULL)
		return False;

	prs_debug(ps, depth, desc, "reg_io_q_set_value");
	depth++;

	if(!prs_align(ps))
		return False;
	
	if(!smb_io_pol_hnd("", &q_u->handle, ps, depth))
		return False;
	
	if(!prs_unistr4("name", ps, depth, &q_u->name ))
		return False;
	if(!prs_align(ps))
		return False;

	if(!prs_uint32("type", ps, depth, &q_u->type))
		return False;

	if(!smb_io_rpc_blob("value", &q_u->value, ps, depth ))
		return False;
	if(!prs_align(ps))
		return False;

	if(!prs_uint32("size", ps, depth, &q_u->size))
		return False;

	return True;
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL reg_io_r_set_value(const char *desc,  REG_R_SET_VALUE *q_u, prs_struct *ps, int depth)
{
	if ( !q_u )
		return False;

	prs_debug(ps, depth, desc, "reg_io_r_set_value");
	depth++;

	if(!prs_align(ps))
		return False;
	
	if(!prs_werror("status", ps, depth, &q_u->status))
		return False;

	return True;
}

/*******************************************************************
makes a structure.
********************************************************************/

void init_reg_q_enum_key(REG_Q_ENUM_KEY *q_u, POLICY_HND *pol, uint32 key_idx)
{
	memcpy(&q_u->pol, pol, sizeof(q_u->pol));

	q_u->key_index = key_idx;
	q_u->key_name_len = 0;
	q_u->unknown_1 = 0x0414;

	q_u->ptr1 = 1;
	q_u->unknown_2 = 0x0000020A;
	memset(q_u->pad1, 0, sizeof(q_u->pad1));

	q_u->ptr2 = 1;
	memset(q_u->pad2, 0, sizeof(q_u->pad2));

	q_u->ptr3 = 1;
	unix_to_nt_time(&q_u->time, 0);            /* current time? */
}

/*******************************************************************
makes a reply structure.
********************************************************************/

void init_reg_r_enum_key(REG_R_ENUM_KEY *r_u, char *subkey )
{
	if ( !r_u )
		return;
		
	init_unistr4( &r_u->keyname, subkey, UNI_STR_TERMINATE );
	r_u->classname = TALLOC_ZERO_P( get_talloc_ctx(), UNISTR4 );
	if (!r_u->classname) {
		smb_panic("init_reg_r_enum_key: talloc fail\n");
		return;
	}
	r_u->time = TALLOC_ZERO_P( get_talloc_ctx(), NTTIME );
	if (!r_u->time) {
		smb_panic("init_reg_r_enum_key: talloc fail\n");
		return;
	}
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL reg_io_q_enum_key(const char *desc,  REG_Q_ENUM_KEY *q_u, prs_struct *ps, int depth)
{
	if (q_u == NULL)
		return False;

	prs_debug(ps, depth, desc, "reg_io_q_enum_key");
	depth++;

	if(!prs_align(ps))
		return False;
	
	if(!smb_io_pol_hnd("", &q_u->pol, ps, depth))
		return False;
	
	if(!prs_uint32("key_index", ps, depth, &q_u->key_index))
		return False;
	if(!prs_uint16("key_name_len", ps, depth, &q_u->key_name_len))
		return False;
	if(!prs_uint16("unknown_1", ps, depth, &q_u->unknown_1))
		return False;

	if(!prs_uint32("ptr1", ps, depth, &q_u->ptr1))
		return False;

	if (q_u->ptr1 != 0) {
		if(!prs_uint32("unknown_2", ps, depth, &q_u->unknown_2))
			return False;
		if(!prs_uint8s(False, "pad1", ps, depth, q_u->pad1, sizeof(q_u->pad1)))
			return False;
	}

	if(!prs_uint32("ptr2", ps, depth, &q_u->ptr2))
		return False;

	if (q_u->ptr2 != 0) {
		if(!prs_uint8s(False, "pad2", ps, depth, q_u->pad2, sizeof(q_u->pad2)))
			return False;
	}

	if(!prs_uint32("ptr3", ps, depth, &q_u->ptr3))
		return False;

	if (q_u->ptr3 != 0) {
		if(!smb_io_time("", &q_u->time, ps, depth))
			return False;
	}

	return True;
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL reg_io_r_enum_key(const char *desc,  REG_R_ENUM_KEY *q_u, prs_struct *ps, int depth)
{
	if ( !q_u )
		return False;

	prs_debug(ps, depth, desc, "reg_io_r_enum_key");
	depth++;

	if(!prs_align(ps))
		return False;
	if ( !prs_unistr4( "keyname", ps, depth, &q_u->keyname ) )
		return False;
	
	if(!prs_align(ps))
		return False;
	if (!prs_pointer("class", ps, depth, (void**)&q_u->classname, sizeof(UNISTR4), (PRS_POINTER_CAST)prs_unistr4))
		return False;

	if(!prs_align(ps))
		return False;
	if (!prs_pointer("time", ps, depth, (void**)&q_u->time, sizeof(NTTIME), (PRS_POINTER_CAST)smb_io_nttime))
		return False;

	if(!prs_align(ps))
		return False;
	if(!prs_werror("status", ps, depth, &q_u->status))
		return False;

	return True;
}

/*******************************************************************
makes a structure.
********************************************************************/

void init_reg_q_open_entry(REG_Q_OPEN_ENTRY *q_u, POLICY_HND *pol,
				char *key_name, uint32 access_desired)
{
	memcpy(&q_u->pol, pol, sizeof(q_u->pol));

	init_unistr4(&q_u->name, key_name, UNI_STR_TERMINATE);

	q_u->unknown_0 = 0x00000000;
	q_u->access = access_desired;
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL reg_io_q_open_entry(const char *desc,  REG_Q_OPEN_ENTRY *q_u, prs_struct *ps, int depth)
{
	if ( !q_u )
		return False;

	prs_debug(ps, depth, desc, "reg_io_q_open_entry");
	depth++;

	if(!prs_align(ps))
		return False;
	
	if(!smb_io_pol_hnd("", &q_u->pol, ps, depth))
		return False;
	if(!prs_unistr4("name", ps, depth, &q_u->name))
		return False;

	if(!prs_align(ps))
		return False;
	
	if(!prs_uint32("unknown_0        ", ps, depth, &q_u->unknown_0))
		return False;
	if(!prs_uint32("access", ps, depth, &q_u->access))
		return False;

	return True;
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL reg_io_r_open_entry(const char *desc,  REG_R_OPEN_ENTRY *r_u, prs_struct *ps, int depth)
{
	if ( !r_u )
		return False;

	prs_debug(ps, depth, desc, "reg_io_r_open_entry");
	depth++;

	if(!prs_align(ps))
		return False;
	
	if(!smb_io_pol_hnd("handle", &r_u->handle, ps, depth))
		return False;

	if(!prs_werror("status", ps, depth, &r_u->status))
		return False;

	return True;
}

/*******************************************************************
Inits a structure.
********************************************************************/

void init_reg_q_shutdown(REG_Q_SHUTDOWN *q_u, const char *msg,
			uint32 timeout, BOOL do_reboot, BOOL force)
{
	q_u->server = TALLOC_P( get_talloc_ctx(), uint16 );
	if (!q_u->server) {
		smb_panic("init_reg_q_shutdown: talloc fail\n");
		return;
	}
	*q_u->server = 0x1;

	q_u->message = TALLOC_ZERO_P( get_talloc_ctx(), UNISTR4 );
	if (!q_u->message) {
		smb_panic("init_reg_q_shutdown: talloc fail\n");
		return;
	}

	if ( msg && *msg ) { 
		init_unistr4( q_u->message, msg, UNI_FLAGS_NONE );

		/* Win2000 is apparently very sensitive to these lengths */
		/* do a special case here */

		q_u->message->string->uni_max_len++;
		q_u->message->size += 2;

	}

	q_u->timeout = timeout;

	q_u->reboot = do_reboot ? 1 : 0;
	q_u->force = force ? 1 : 0;
}

/*******************************************************************
Inits a REG_Q_SHUTDOWN_EX structure.
********************************************************************/

void init_reg_q_shutdown_ex(REG_Q_SHUTDOWN_EX * q_u_ex, const char *msg,
			uint32 timeout, BOOL do_reboot, BOOL force, uint32 reason)
{
	REG_Q_SHUTDOWN q_u;
	
	ZERO_STRUCT( q_u );
	
	init_reg_q_shutdown( &q_u, msg, timeout, do_reboot, force );
	
	/* steal memory */
	
	q_u_ex->server  = q_u.server;
	q_u_ex->message = q_u.message;
	
	q_u_ex->reboot  = q_u.reboot;
	q_u_ex->force   = q_u.force;
	
	q_u_ex->reason = reason;
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL reg_io_q_shutdown(const char *desc, REG_Q_SHUTDOWN *q_u, prs_struct *ps,
		       int depth)
{
	if ( !q_u )
		return False;

	prs_debug(ps, depth, desc, "reg_io_q_shutdown");
	depth++;

	if (!prs_align(ps))
		return False;

	if (!prs_pointer("server", ps, depth, (void**)&q_u->server, sizeof(uint16), (PRS_POINTER_CAST)prs_uint16))
		return False;
	if (!prs_align(ps))
		return False;

	if (!prs_pointer("message", ps, depth, (void**)&q_u->message, sizeof(UNISTR4), (PRS_POINTER_CAST)prs_unistr4))
		return False;

	if (!prs_align(ps))
		return False;

	if (!prs_uint32("timeout", ps, depth, &(q_u->timeout)))
		return False;

	if (!prs_uint8("force  ", ps, depth, &(q_u->force)))
		return False;
	if (!prs_uint8("reboot ", ps, depth, &(q_u->reboot)))
		return False;


	return True;
}

/*******************************************************************
reads or writes a structure.
********************************************************************/
BOOL reg_io_r_shutdown(const char *desc, REG_R_SHUTDOWN *r_u, prs_struct *ps,
		       int depth)
{
	if ( !r_u )
		return False;

	prs_debug(ps, depth, desc, "reg_io_r_shutdown");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!prs_werror("status", ps, depth, &r_u->status))
		return False;

	return True;
}

/*******************************************************************
reads or writes a REG_Q_SHUTDOWN_EX structure.
********************************************************************/

BOOL reg_io_q_shutdown_ex(const char *desc, REG_Q_SHUTDOWN_EX *q_u, prs_struct *ps,
		       int depth)
{
	if ( !q_u )
		return False;

	prs_debug(ps, depth, desc, "reg_io_q_shutdown_ex");
	depth++;

	if (!prs_align(ps))
		return False;

	if (!prs_pointer("server", ps, depth, (void**)&q_u->server, sizeof(uint16), (PRS_POINTER_CAST)prs_uint16))
		return False;
	if (!prs_align(ps))
		return False;

	if (!prs_pointer("message", ps, depth, (void**)&q_u->message, sizeof(UNISTR4), (PRS_POINTER_CAST)prs_unistr4))
		return False;

	if (!prs_align(ps))
		return False;

	if (!prs_uint32("timeout", ps, depth, &(q_u->timeout)))
		return False;

	if (!prs_uint8("force  ", ps, depth, &(q_u->force)))
		return False;
	if (!prs_uint8("reboot ", ps, depth, &(q_u->reboot)))
		return False;

	if (!prs_align(ps))
		return False;
	if (!prs_uint32("reason", ps, depth, &(q_u->reason)))
		return False;


	return True;
}

/*******************************************************************
reads or writes a REG_R_SHUTDOWN_EX structure.
********************************************************************/
BOOL reg_io_r_shutdown_ex(const char *desc, REG_R_SHUTDOWN_EX *r_u, prs_struct *ps,
		       int depth)
{
	if ( !r_u )
		return False;

	prs_debug(ps, depth, desc, "reg_io_r_shutdown_ex");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!prs_werror("status", ps, depth, &r_u->status))
		return False;

	return True;
}

/*******************************************************************
Inits a structure.
********************************************************************/

void init_reg_q_abort_shutdown(REG_Q_ABORT_SHUTDOWN *q_u)
{
	q_u->server = TALLOC_P( get_talloc_ctx(), uint16 );
	if (!q_u->server) {
		smb_panic("init_reg_q_abort_shutdown: talloc fail\n");
		return;
	}
	*q_u->server = 0x1;
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL reg_io_q_abort_shutdown(const char *desc, REG_Q_ABORT_SHUTDOWN *q_u,
			     prs_struct *ps, int depth)
{
	if ( !q_u )
		return False;

	prs_debug(ps, depth, desc, "reg_io_q_abort_shutdown");
	depth++;

	if (!prs_align(ps))
		return False;

	if (!prs_pointer("server", ps, depth, (void**)&q_u->server, sizeof(uint16), (PRS_POINTER_CAST)prs_uint16))
		return False;
	if (!prs_align(ps))
		return False;

	return True;
}

/*******************************************************************
reads or writes a structure.
********************************************************************/
BOOL reg_io_r_abort_shutdown(const char *desc, REG_R_ABORT_SHUTDOWN *r_u,
			     prs_struct *ps, int depth)
{
	if ( !r_u )
		return False;

	prs_debug(ps, depth, desc, "reg_io_r_abort_shutdown");
	depth++;

	if (!prs_align(ps))
		return False;

	if (!prs_werror("status", ps, depth, &r_u->status))
		return False;

	return True;
}
