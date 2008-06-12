/* 
   Unix SMB/CIFS implementation.
   RPC Pipe client
 
   Copyright (C) Andrew Tridgell              1992-2000,
   Copyright (C) Jeremy Allison                    1999 - 2005
   Copyright (C) Simo Sorce                        2001
   Copyright (C) Jeremy Cooper                     2004
   Copyright (C) Gerald (Jerry) Carter             2005
   
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
#include "rpc_client.h"

/* Shutdown a server */

/*******************************************************************
 internal connect to a registry hive root (open a registry policy)
*******************************************************************/

static WERROR rpccli_reg_open_hive_int(struct rpc_pipe_client *cli,
                                      TALLOC_CTX *mem_ctx, uint16 op_code,
                                      const char *op_name,
                                      uint32 access_mask, POLICY_HND *hnd)
{
	REG_Q_OPEN_HIVE in;
	REG_R_OPEN_HIVE out;
	prs_struct qbuf, rbuf;

	ZERO_STRUCT(in);
	ZERO_STRUCT(out);

	init_reg_q_open_hive(&in, access_mask);

	CLI_DO_RPC_WERR( cli, mem_ctx, PI_WINREG, op_code, 
	            in, out, 
	            qbuf, rbuf,
	            reg_io_q_open_hive,
	            reg_io_r_open_hive, 
	            WERR_GENERAL_FAILURE );

	if ( !W_ERROR_IS_OK( out.status ) )
		return out.status;

	memcpy( hnd, &out.pol, sizeof(POLICY_HND) );

	return out.status;
}

/*******************************************************************
 connect to a registry hive root (open a registry policy)
*******************************************************************/

WERROR rpccli_reg_connect(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx,
                         uint32 reg_type, uint32 access_mask,
                         POLICY_HND *reg_hnd)
{	uint16 op_code;
	const char *op_name;

	ZERO_STRUCTP(reg_hnd);

	switch (reg_type)
	{
	case HKEY_CLASSES_ROOT:
		op_code = REG_OPEN_HKCR;
		op_name = "REG_OPEN_HKCR";
		break;
	case HKEY_LOCAL_MACHINE:
		op_code = REG_OPEN_HKLM;
		op_name = "REG_OPEN_HKLM";
		break;
	case HKEY_USERS:
		op_code = REG_OPEN_HKU;
		op_name = "REG_OPEN_HKU";
		break;
	case HKEY_PERFORMANCE_DATA:
		op_code = REG_OPEN_HKPD;
		op_name = "REG_OPEN_HKPD";
		break;
	default:
		return WERR_INVALID_PARAM;
	}

	return rpccli_reg_open_hive_int(cli, mem_ctx, op_code, op_name,
                                     access_mask, reg_hnd);
}


/*******************************************************************
*******************************************************************/

WERROR rpccli_reg_shutdown(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx,
                          const char *msg, uint32 timeout, BOOL do_reboot,
			  BOOL force)
{
	REG_Q_SHUTDOWN in;
	REG_R_SHUTDOWN out;
	prs_struct qbuf, rbuf;

	if (msg == NULL) 
		return WERR_INVALID_PARAM;

	ZERO_STRUCT (in);
	ZERO_STRUCT (out);

	/* Marshall data and send request */

	init_reg_q_shutdown(&in, msg, timeout, do_reboot, force);

	CLI_DO_RPC_WERR( cli, mem_ctx, PI_WINREG, REG_SHUTDOWN, 
	            in, out, 
	            qbuf, rbuf,
	            reg_io_q_shutdown,
	            reg_io_r_shutdown, 
	            WERR_GENERAL_FAILURE );

	return out.status;
}

/*******************************************************************
*******************************************************************/

WERROR rpccli_reg_abort_shutdown(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx)
{
	REG_Q_ABORT_SHUTDOWN in;
	REG_R_ABORT_SHUTDOWN out;
	prs_struct qbuf, rbuf;

	ZERO_STRUCT (in);
	ZERO_STRUCT (out);

	CLI_DO_RPC_WERR( cli, mem_ctx, PI_WINREG, REG_ABORT_SHUTDOWN, 
	            in, out, 
	            qbuf, rbuf,
	            reg_io_q_abort_shutdown,
	            reg_io_r_abort_shutdown, 
	            WERR_GENERAL_FAILURE );

	return out.status;	
}


/****************************************************************************
do a REG Unknown 0xB command.  sent after a create key or create value.
this might be some sort of "sync" or "refresh" command, sent after
modification of the registry...
****************************************************************************/

WERROR rpccli_reg_flush_key(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx,
                           POLICY_HND *hnd)
{
	REG_Q_FLUSH_KEY in;
	REG_R_FLUSH_KEY out;
	prs_struct qbuf, rbuf;

	ZERO_STRUCT (in);
	ZERO_STRUCT (out);
	
	init_reg_q_flush_key(&in, hnd);

	CLI_DO_RPC_WERR( cli, mem_ctx, PI_WINREG, REG_FLUSH_KEY, 
	            in, out, 
	            qbuf, rbuf,
	            reg_io_q_flush_key,
	            reg_io_r_flush_key, 
	            WERR_GENERAL_FAILURE );
		    
	return out.status;
}

/****************************************************************************
do a REG Query Key
****************************************************************************/

WERROR rpccli_reg_query_key(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx,
                           POLICY_HND *hnd,
                           char *key_class, uint32 *class_len,
                           uint32 *num_subkeys, uint32 *max_subkeylen,
                           uint32 *max_classlen, uint32 *num_values,
                           uint32 *max_valnamelen, uint32 *max_valbufsize,
                           uint32 *sec_desc, NTTIME *mod_time)
{
	REG_Q_QUERY_KEY in;
	REG_R_QUERY_KEY out;
	prs_struct qbuf, rbuf;
	uint32 saved_class_len = *class_len;

	ZERO_STRUCT (in);
	ZERO_STRUCT (out);

	init_reg_q_query_key( &in, hnd, key_class );

	CLI_DO_RPC_WERR( cli, mem_ctx, PI_WINREG, REG_QUERY_KEY, 
	            in, out, 
	            qbuf, rbuf,
	            reg_io_q_query_key,
	            reg_io_r_query_key, 
	            WERR_GENERAL_FAILURE );

	if ( W_ERROR_EQUAL( out.status, WERR_MORE_DATA ) ) {
		ZERO_STRUCT (in);

		*class_len = out.key_class.string->uni_max_len;
		if ( *class_len > saved_class_len )
			return out.status;
			
		/* set a string of spaces and NULL terminate */

		memset( key_class, (int)' ', *class_len );
		key_class[*class_len] = '\0';
		
		init_reg_q_query_key( &in, hnd, key_class );

		ZERO_STRUCT (out);

		CLI_DO_RPC_WERR( cli, mem_ctx, PI_WINREG, REG_QUERY_KEY, 
		            in, out, 
		            qbuf, rbuf,
		            reg_io_q_query_key,
		            reg_io_r_query_key, 
		            WERR_GENERAL_FAILURE );
	}
	
	if ( !W_ERROR_IS_OK( out.status ) )
		return out.status;

	*class_len      = out.key_class.string->uni_max_len;
	unistr2_to_ascii(key_class, out.key_class.string, saved_class_len-1);
	*num_subkeys    = out.num_subkeys   ;
	*max_subkeylen  = out.max_subkeylen ;
	*num_values     = out.num_values    ;
	*max_valnamelen = out.max_valnamelen;
	*max_valbufsize = out.max_valbufsize;
	*sec_desc       = out.sec_desc      ;
	*mod_time       = out.mod_time      ;
	/* Maybe: *max_classlen = out.reserved; */

	return out.status;
}

/****************************************************************************
****************************************************************************/

WERROR rpccli_reg_getversion(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx,
                            POLICY_HND *hnd, uint32 *version)
{
	REG_Q_GETVERSION in;
	REG_R_GETVERSION out;
	prs_struct qbuf, rbuf;

	ZERO_STRUCT (in);
	ZERO_STRUCT (out);
	
	init_reg_q_getversion(&in, hnd);

	CLI_DO_RPC_WERR( cli, mem_ctx, PI_WINREG, REG_GETVERSION, 
	            in, out, 
	            qbuf, rbuf,
	            reg_io_q_getversion,
	            reg_io_r_getversion, 
	            WERR_GENERAL_FAILURE );
		    

	if ( !W_ERROR_IS_OK( out.status ) )
		return out.status;
		
	*version = out.win_version;

	return out.status;
}

/****************************************************************************
do a REG Query Info
****************************************************************************/

WERROR rpccli_reg_query_value(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx,
                           POLICY_HND *hnd, const char *val_name,
                           uint32 *type, REGVAL_BUFFER *buffer)
{
	REG_Q_QUERY_VALUE in;
	REG_R_QUERY_VALUE out;
	prs_struct qbuf, rbuf;

	ZERO_STRUCT (in);
	ZERO_STRUCT (out);
	
	init_reg_q_query_value(&in, hnd, val_name, buffer);

	CLI_DO_RPC_WERR( cli, mem_ctx, PI_WINREG, REG_QUERY_VALUE, 
	            in, out, 
	            qbuf, rbuf,
	            reg_io_q_query_value,
	            reg_io_r_query_value, 
	            WERR_GENERAL_FAILURE );
		    

	if ( !W_ERROR_IS_OK( out.status ) )
		return out.status;
		
	*type   = *out.type;
	*buffer = *out.value;

	return out.status;
}

/****************************************************************************
do a REG Set Key Security 
****************************************************************************/

WERROR rpccli_reg_set_key_sec(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx,
                             POLICY_HND *hnd, uint32 sec_info,
                             size_t secdesc_size, SEC_DESC *sec_desc)
{
	REG_Q_SET_KEY_SEC in;
	REG_R_SET_KEY_SEC out;
	prs_struct qbuf, rbuf;
	SEC_DESC_BUF *sec_desc_buf;

	ZERO_STRUCT (in);
	ZERO_STRUCT (out);
	
	/* Flatten the security descriptor */
	
	if ( !(sec_desc_buf = make_sec_desc_buf(mem_ctx, secdesc_size, sec_desc)) )
		return WERR_GENERAL_FAILURE;
		
	init_reg_q_set_key_sec(&in, hnd, sec_info, sec_desc_buf);

	CLI_DO_RPC_WERR( cli, mem_ctx, PI_WINREG, REG_SET_KEY_SEC, 
	            in, out, 
	            qbuf, rbuf,
	            reg_io_q_set_key_sec,
	            reg_io_r_set_key_sec, 
	            WERR_GENERAL_FAILURE );
		    

	return out.status;
}


/****************************************************************************
do a REG Query Key Security 
****************************************************************************/

WERROR rpccli_reg_get_key_sec(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx,
                             POLICY_HND *hnd, uint32 sec_info,
                             uint32 *sec_buf_size, SEC_DESC_BUF *sec_buf)
{
	REG_Q_GET_KEY_SEC in;
	REG_R_GET_KEY_SEC out;
	prs_struct qbuf, rbuf;

	ZERO_STRUCT (in);
	ZERO_STRUCT (out);
	
	init_reg_q_get_key_sec(&in, hnd, sec_info, *sec_buf_size, sec_buf);

	CLI_DO_RPC_WERR( cli, mem_ctx, PI_WINREG, REG_GET_KEY_SEC, 
	            in, out, 
	            qbuf, rbuf,
	            reg_io_q_get_key_sec,
	            reg_io_r_get_key_sec, 
	            WERR_GENERAL_FAILURE );
		    

	/* this might be able to return WERR_MORE_DATA, I'm not sure */
	
	if ( !W_ERROR_IS_OK( out.status ) )
		return out.status;
	
	sec_buf       = out.data;
	*sec_buf_size = out.data->len;
		
	return out.status;	
}

/****************************************************************************
do a REG Delete Value
****************************************************************************/

WERROR rpccli_reg_delete_val(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx,
                            POLICY_HND *hnd, char *val_name)
{
	REG_Q_DELETE_VALUE in;
	REG_R_DELETE_VALUE out;
	prs_struct qbuf, rbuf;

	ZERO_STRUCT (in);
	ZERO_STRUCT (out);
	
	init_reg_q_delete_val(&in, hnd, val_name);

	CLI_DO_RPC_WERR( cli, mem_ctx, PI_WINREG, REG_DELETE_VALUE, 
	            in, out, 
	            qbuf, rbuf,
	            reg_io_q_delete_value,
	            reg_io_r_delete_value, 
	            WERR_GENERAL_FAILURE );
	
	return out.status;
}

/****************************************************************************
do a REG Delete Key
****************************************************************************/

WERROR rpccli_reg_delete_key(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx,
                            POLICY_HND *hnd, char *key_name)
{
	REG_Q_DELETE_KEY in;
	REG_R_DELETE_KEY out;
	prs_struct qbuf, rbuf;

	ZERO_STRUCT (in);
	ZERO_STRUCT (out);
	
	init_reg_q_delete_key(&in, hnd, key_name);

	CLI_DO_RPC_WERR( cli, mem_ctx, PI_WINREG, REG_DELETE_KEY, 
	            in, out, 
	            qbuf, rbuf,
	            reg_io_q_delete_key,
	            reg_io_r_delete_key, 
	            WERR_GENERAL_FAILURE );
	
	return out.status;
}

/****************************************************************************
do a REG Create Key
****************************************************************************/

WERROR rpccli_reg_create_key_ex(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx,
                            POLICY_HND *hnd, char *key_name, char *key_class,
                            uint32 access_desired, POLICY_HND *key)
{
	REG_Q_CREATE_KEY_EX in;
	REG_R_CREATE_KEY_EX out;
	prs_struct qbuf, rbuf;
	SEC_DESC *sec;
	SEC_DESC_BUF *sec_buf;
	size_t sec_len;

	ZERO_STRUCT (in);
	ZERO_STRUCT (out);
	
	if ( !(sec = make_sec_desc(mem_ctx, 1, SEC_DESC_SELF_RELATIVE,
		NULL, NULL, NULL, NULL, &sec_len)) ) {
		return WERR_GENERAL_FAILURE;
	}
				 
	if ( !(sec_buf = make_sec_desc_buf(mem_ctx, sec_len, sec)) )
		return WERR_GENERAL_FAILURE;

	init_reg_q_create_key_ex(&in, hnd, key_name, key_class, access_desired, sec_buf);

	CLI_DO_RPC_WERR( cli, mem_ctx, PI_WINREG, REG_CREATE_KEY_EX, 
	            in, out, 
	            qbuf, rbuf,
	            reg_io_q_create_key_ex,
	            reg_io_r_create_key_ex, 
	            WERR_GENERAL_FAILURE );
		    

	if ( !W_ERROR_IS_OK( out.status ) )
		return out.status;
	
	memcpy( key, &out.handle, sizeof(POLICY_HND) );
	
	return out.status;
}

/****************************************************************************
do a REG Enum Key
****************************************************************************/

WERROR rpccli_reg_enum_key(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx,
                          POLICY_HND *hnd, int key_index, fstring key_name,
                          fstring class_name, time_t *mod_time)
{
	REG_Q_ENUM_KEY in;
	REG_R_ENUM_KEY out;
	prs_struct qbuf, rbuf;

	ZERO_STRUCT (in);
	ZERO_STRUCT (out);
	
	init_reg_q_enum_key(&in, hnd, key_index);

	CLI_DO_RPC_WERR( cli, mem_ctx, PI_WINREG, REG_ENUM_KEY, 
	            in, out, 
	            qbuf, rbuf,
	            reg_io_q_enum_key,
	            reg_io_r_enum_key, 
	            WERR_GENERAL_FAILURE );

	if ( !W_ERROR_IS_OK(out.status) )
		return out.status;

	if ( out.keyname.string )
		rpcstr_pull( key_name, out.keyname.string->buffer, sizeof(fstring), -1, STR_TERMINATE );
	else
		fstrcpy( key_name, "(Default)" );

	if ( out.classname && out.classname->string )
		rpcstr_pull( class_name, out.classname->string->buffer, sizeof(fstring), -1, STR_TERMINATE );
	else
		fstrcpy( class_name, "" );

	*mod_time   = nt_time_to_unix(out.time);

	return out.status;
}

/****************************************************************************
do a REG Create Value
****************************************************************************/

WERROR rpccli_reg_set_val(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx,
                            POLICY_HND *hnd, char *val_name, uint32 type,
                            RPC_DATA_BLOB *data)
{
	REG_Q_SET_VALUE in;
	REG_R_SET_VALUE out;
	prs_struct qbuf, rbuf;

	ZERO_STRUCT (in);
	ZERO_STRUCT (out);
	
	init_reg_q_set_val(&in, hnd, val_name, type, data);

	CLI_DO_RPC_WERR( cli, mem_ctx, PI_WINREG, REG_SET_VALUE, 
	            in, out, 
	            qbuf, rbuf,
	            reg_io_q_set_value,
	            reg_io_r_set_value, 
	            WERR_GENERAL_FAILURE );

	return out.status;
}

/****************************************************************************
do a REG Enum Value
****************************************************************************/

WERROR rpccli_reg_enum_val(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx,
                          POLICY_HND *hnd, int idx,
                          fstring val_name, uint32 *type, REGVAL_BUFFER *value)
{
	REG_Q_ENUM_VALUE in;
	REG_R_ENUM_VALUE out;
	prs_struct qbuf, rbuf;

	ZERO_STRUCT (in);
	ZERO_STRUCT (out);
	
	init_reg_q_enum_val(&in, hnd, idx, 0x0100, 0x1000);

	CLI_DO_RPC_WERR( cli, mem_ctx, PI_WINREG, REG_ENUM_VALUE, 
	            in, out, 
	            qbuf, rbuf,
	            reg_io_q_enum_val,
	            reg_io_r_enum_val, 
	            WERR_GENERAL_FAILURE );

	if ( W_ERROR_EQUAL(out.status, WERR_MORE_DATA) ) {

		ZERO_STRUCT (in);

		init_reg_q_enum_val(&in, hnd, idx, 0x0100, *out.buffer_len1);

		ZERO_STRUCT (out);

		CLI_DO_RPC_WERR( cli, mem_ctx, PI_WINREG, REG_ENUM_VALUE, 
		            in, out, 
		            qbuf, rbuf,
		            reg_io_q_enum_val,
		            reg_io_r_enum_val, 
		            WERR_GENERAL_FAILURE );
	}

	if ( !W_ERROR_IS_OK(out.status) )
		return out.status;

	unistr2_to_ascii(val_name, out.name.string, sizeof(fstring)-1);
	*type      = *out.type;
	*value     = *out.value;
	
	return out.status;
}

/****************************************************************************
****************************************************************************/

WERROR rpccli_reg_open_entry(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx,
                            POLICY_HND *hnd, char *key_name,
                            uint32 access_desired, POLICY_HND *key_hnd)
{
	REG_Q_OPEN_ENTRY in;
	REG_R_OPEN_ENTRY out;
	prs_struct qbuf, rbuf;

	ZERO_STRUCT (in);
	ZERO_STRUCT (out);
	
	init_reg_q_open_entry(&in, hnd, key_name, access_desired);

	CLI_DO_RPC_WERR( cli, mem_ctx, PI_WINREG, REG_OPEN_ENTRY, 
	            in, out, 
	            qbuf, rbuf,
	            reg_io_q_open_entry,
	            reg_io_r_open_entry, 
	            WERR_GENERAL_FAILURE );

	if ( !W_ERROR_IS_OK( out.status ) )
		return out.status;

	memcpy( key_hnd, &out.handle, sizeof(POLICY_HND) );
	
	return out.status;
}

/****************************************************************************
****************************************************************************/

WERROR rpccli_reg_close(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx,
                       POLICY_HND *hnd)
{
	REG_Q_CLOSE in;
	REG_R_CLOSE out;
	prs_struct qbuf, rbuf;

	ZERO_STRUCT (in);
	ZERO_STRUCT (out);
	
	init_reg_q_close(&in, hnd);

	CLI_DO_RPC_WERR( cli, mem_ctx, PI_WINREG, REG_CLOSE, 
	            in, out, 
	            qbuf, rbuf,
	            reg_io_q_close,
	            reg_io_r_close, 
	            WERR_GENERAL_FAILURE );
	
	return out.status;
}

/****************************************************************************
do a REG Query Info
****************************************************************************/

WERROR rpccli_reg_save_key(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx,
                         POLICY_HND *hnd, const char *filename )
{
	REG_Q_SAVE_KEY in;
	REG_R_SAVE_KEY out;
	prs_struct qbuf, rbuf;

	ZERO_STRUCT (in);
	ZERO_STRUCT (out);
	
	init_q_reg_save_key( &in, hnd, filename );

	CLI_DO_RPC_WERR( cli, mem_ctx, PI_WINREG, REG_SAVE_KEY, 
	            in, out, 
	            qbuf, rbuf,
	            reg_io_q_save_key,
	            reg_io_r_save_key,
	            WERR_GENERAL_FAILURE );

	return out.status;
}


/* 
 #################################################################
  Utility functions
 #################################################################
 */

/*****************************************************************
 Splits out the start of the key (HKLM or HKU) and the rest of the key.
*****************************************************************/  

BOOL reg_split_hive(const char *full_keyname, uint32 *reg_type, pstring key_name)
{
	pstring tmp;

	if (!next_token(&full_keyname, tmp, "\\", sizeof(tmp)))
		return False;

	(*reg_type) = 0;

	DEBUG(10, ("reg_split_key: hive %s\n", tmp));

	if (strequal(tmp, "HKLM") || strequal(tmp, "HKEY_LOCAL_MACHINE"))
		(*reg_type) = HKEY_LOCAL_MACHINE;
	else if (strequal(tmp, "HKCR") || strequal(tmp, "HKEY_CLASSES_ROOT"))
		(*reg_type) = HKEY_CLASSES_ROOT;
	else if (strequal(tmp, "HKU") || strequal(tmp, "HKEY_USERS"))
		(*reg_type) = HKEY_USERS;
	else if (strequal(tmp, "HKPD")||strequal(tmp, "HKEY_PERFORMANCE_DATA"))
		(*reg_type) = HKEY_PERFORMANCE_DATA;
	else {
		DEBUG(10,("reg_split_key: unrecognised hive key %s\n", tmp));
		return False;
	}
	
	if (next_token(&full_keyname, tmp, "\n\r", sizeof(tmp)))
		pstrcpy(key_name, tmp);
	else
		key_name[0] = 0;

	DEBUG(10, ("reg_split_key: name %s\n", key_name));

	return True;
}
