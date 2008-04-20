/* 
   Unix SMB/CIFS implementation.
   NT Domain Authentication SMB / MSRPC client
   Copyright (C) Andrew Tridgell 1994-1997
   Copyright (C) Luke Kenneth Casson Leighton 1996-1997
   Copyright (C) Simo Sorce 2001
   
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
#include "rpcclient.h"

/*
 * keys.  of the form:
 * ----
 *
 * [HKLM]|[HKU]\[parent_keyname_components]\[subkey]|[value]
 *
 * reg_getsubkey() splits this down into:
 * [HKLM]|[HKU]\[parent_keyname_components] and [subkey]|[value]
 *
 * do_reg_connect() splits the left side down further into:
 * [HKLM]|[HKU] and [parent_keyname_components].
 *
 * HKLM is short for HKEY_LOCAL_MACHINE
 * HKU  is short for HKEY_USERS
 *
 * oh, and HKEY stands for "Hive Key".
 *
 */

#if 0 /* This whole file need to be rewritten for the current rpcclient interface */

/****************************************************************************
nt registry enum
****************************************************************************/
static void cmd_reg_enum(struct client_info *info)
{
	BOOL res = True;
	BOOL res1 = True;
	BOOL res2 = True;
	int i;

	POLICY_HND key_pol;
	fstring full_keyname;
	fstring key_name;
	uint32 reg_type;

	/*
	 * query key info
	 */

	fstring key_class;
	uint32 max_class_len = 0;
	uint32 num_subkeys;
	uint32 max_subkeylen;
	uint32 max_subkeysize; 
	uint32 num_values;
	uint32 max_valnamelen;
	uint32 max_valbufsize;
	uint32 sec_desc;
	NTTIME mod_time;

	/*
	 * unknown 0x1a request
	 */

	uint32 unk_1a_response;

	DEBUG(5, ("cmd_reg_enum: smb_cli->fd:%d\n", smb_cli->fd));

	if (!next_token_nr(NULL, full_keyname, NULL, sizeof(full_keyname)))
	{
		fprintf(out_hnd, "regenum <key_name>\n");
		return;
	}

	if (!reg_split_key(full_keyname, &reg_type, key_name)) {
		fprintf(out_hnd, "Unknown registry hive '%s'\n", key_name);
		return;
	}

	/* open WINREG session. */
	res = res ? cli_nt_session_open(smb_cli, PI_WINREG) : False;

	/* open registry receive a policy handle */
	res = res ? do_reg_connect(smb_cli, full_keyname, key_name,
				&info->dom.reg_pol_connect) : False;

	if ((*key_name) != 0)
	{
		/* open an entry */
		res1 = res  ? do_reg_open_entry(smb_cli, &info->dom.reg_pol_connect,
					 key_name, 0x02000000, &key_pol) : False;
	}
	else
	{
		memcpy(&key_pol, &info->dom.reg_pol_connect, sizeof(key_pol));
	}

	res1 = res1 ? do_reg_query_key(smb_cli,
				&key_pol,
				key_class, &max_class_len,
	                        &num_subkeys, &max_subkeylen, &max_subkeysize,
				&num_values, &max_valnamelen, &max_valbufsize,
	                        &sec_desc, &mod_time) : False;

	if (res1 && num_subkeys > 0)
	{
		fprintf(out_hnd,"Subkeys\n");
		fprintf(out_hnd,"-------\n");
	}

	for (i = 0; i < num_subkeys; i++)
	{
		/*
		 * enumerate key
		 */

		fstring enum_name;
		uint32 enum_unk1;
		uint32 enum_unk2;
		time_t key_mod_time;

		/* unknown 1a it */
		res2 = res1 ? do_reg_getversion(smb_cli, &key_pol,
					&unk_1a_response) : False;

		if (res2 && unk_1a_response != 5)
		{
			fprintf(out_hnd,"Unknown 1a response: %x\n", unk_1a_response);
		}

		/* enum key */
		res2 = res2 ? do_reg_enum_key(smb_cli, &key_pol,
					i, enum_name,
					&enum_unk1, &enum_unk2,
					&key_mod_time) : False;
		
		if (res2)
		{
			display_reg_key_info(out_hnd, ACTION_HEADER   , enum_name, key_mod_time);
			display_reg_key_info(out_hnd, ACTION_ENUMERATE, enum_name, key_mod_time);
			display_reg_key_info(out_hnd, ACTION_FOOTER   , enum_name, key_mod_time);
		}

	}

	if (num_values > 0)
	{
		fprintf(out_hnd,"Key Values\n");
		fprintf(out_hnd,"----------\n");
	}

	for (i = 0; i < num_values; i++)
	{
		/*
		 * enumerate key
		 */

		uint32 val_type;
		REGVAL_BUFFER value;
		fstring val_name;

		/* unknown 1a it */
		res2 = res1 ? do_reg_getversion(smb_cli, &key_pol,
					&unk_1a_response) : False;

		if (res2 && unk_1a_response != 5)
		{
			fprintf(out_hnd,"Unknown 1a response: %x\n", unk_1a_response);
		}

		/* enum key */
		res2 = res2 ? do_reg_enum_val(smb_cli, &key_pol,
					i, max_valnamelen, max_valbufsize,
		                        val_name, &val_type, &value) : False;
		
		if (res2)
		{
			display_reg_value_info(out_hnd, ACTION_HEADER   , val_name, val_type, &value);
			display_reg_value_info(out_hnd, ACTION_ENUMERATE, val_name, val_type, &value);
			display_reg_value_info(out_hnd, ACTION_FOOTER   , val_name, val_type, &value);
		}
	}

	/* close the handles */
	if ((*key_name) != 0)
	{
		res1 = res1 ? do_reg_close(smb_cli, &key_pol) : False;
	}
	res  = res  ? do_reg_close(smb_cli, &info->dom.reg_pol_connect) : False;

	/* close the session */
	cli_nt_session_close(smb_cli);

	if (res && res1 && res2)
	{
		DEBUG(5,("cmd_reg_enum: query succeeded\n"));
	}
	else
	{
		DEBUG(5,("cmd_reg_enum: query failed\n"));
	}
}

/****************************************************************************
nt registry query key
****************************************************************************/
static void cmd_reg_query_key(struct client_info *info)
{
	BOOL res = True;
	BOOL res1 = True;

	POLICY_HND key_pol;
	fstring full_keyname;
	fstring key_name;

	/*
	 * query key info
	 */

	fstring key_class;
	uint32 key_class_len = 0;
	uint32 num_subkeys;
	uint32 max_subkeylen;
	uint32 max_subkeysize; 
	uint32 num_values;
	uint32 max_valnamelen;
	uint32 max_valbufsize;
	uint32 sec_desc;
	NTTIME mod_time;

	DEBUG(5, ("cmd_reg_enum: smb_cli->fd:%d\n", smb_cli->fd));

	if (!next_token_nr(NULL, full_keyname, NULL, sizeof(full_keyname)))
	{
		fprintf(out_hnd, "regquery key_name\n");
		return;
	}

	/* open WINREG session. */
	res = res ? cli_nt_session_open(smb_cli, PI_WINREG) : False;

	/* open registry receive a policy handle */
	res = res ? do_reg_connect(smb_cli, full_keyname, key_name,
				&info->dom.reg_pol_connect) : False;

	if ((*key_name) != 0)
	{
		/* open an entry */
		res1 = res  ? do_reg_open_entry(smb_cli, &info->dom.reg_pol_connect,
					 key_name, 0x02000000, &key_pol) : False;
	}
	else
	{
		memcpy(&key_pol, &info->dom.reg_pol_connect, sizeof(key_pol));
	}

	res1 = res1 ? do_reg_query_key(smb_cli,
				&key_pol,
				key_class, &key_class_len,
	                        &num_subkeys, &max_subkeylen, &max_subkeysize,
				&num_values, &max_valnamelen, &max_valbufsize,
	                        &sec_desc, &mod_time) : False;

	if (res1 && key_class_len != 0)
	{
		res1 = res1 ? do_reg_query_key(smb_cli,
				&key_pol,
				key_class, &key_class_len,
	                        &num_subkeys, &max_subkeylen, &max_subkeysize,
				&num_values, &max_valnamelen, &max_valbufsize,
	                        &sec_desc, &mod_time) : False;
	}

	if (res1)
	{
		fprintf(out_hnd,"Registry Query Info Key\n");
		fprintf(out_hnd,"key class: %s\n", key_class);
		fprintf(out_hnd,"subkeys, max_len, max_size: %d %d %d\n", num_subkeys, max_subkeylen, max_subkeysize);
		fprintf(out_hnd,"vals, max_len, max_size: 0x%x 0x%x 0x%x\n", num_values, max_valnamelen, max_valbufsize);
		fprintf(out_hnd,"sec desc: 0x%x\n", sec_desc);
		fprintf(out_hnd,"mod time: %s\n", http_timestring(nt_time_to_unix(&mod_time)));
	}

	/* close the handles */
	if ((*key_name) != 0)
	{
		res1 = res1 ? do_reg_close(smb_cli, &key_pol) : False;
	}
	res  = res  ? do_reg_close(smb_cli, &info->dom.reg_pol_connect) : False;

	/* close the session */
	cli_nt_session_close(smb_cli);

	if (res && res1)
	{
		DEBUG(5,("cmd_reg_query: query succeeded\n"));
	}
	else
	{
		DEBUG(5,("cmd_reg_query: query failed\n"));
	}
}

/****************************************************************************
nt registry create value
****************************************************************************/
static void cmd_reg_set_val(struct client_info *info)
{
	BOOL res = True;
	BOOL res3 = True;
	BOOL res4 = True;

	POLICY_HND parent_pol;
	fstring full_keyname;
	fstring keyname;
	fstring parent_name;
	fstring val_name;
	fstring tmp;
	uint32 val_type;
	RPC_DATA_BLOB value;

#if 0
	uint32 unk_0;
	uint32 unk_1;
	/* query it */
	res1 = res1 ? do_reg_query_info(smb_cli, &val_pol,
	                        val_name, *val_type) : False;
#endif

	DEBUG(5, ("cmd_reg_set_val: smb_cli->fd:%d\n", smb_cli->fd));

	if (!next_token_nr(NULL, full_keyname, NULL, sizeof(full_keyname)))
	{
		fprintf(out_hnd, "regcreate <val_name> <val_type> <val>\n");
		return;
	}

	reg_get_subkey(full_keyname, keyname, val_name);

	if (keyname[0] == 0 || val_name[0] == 0)
	{
		fprintf(out_hnd, "invalid key name\n");
		return;
	}
	
	if (!next_token_nr(NULL, tmp, NULL, sizeof(tmp)))
	{
		fprintf(out_hnd, "regcreate <val_name> <val_type (1|4)> <val>\n");
		return;
	}

	val_type = atoi(tmp);

	if (val_type != 1 && val_type != 3 && val_type != 4)
	{
		fprintf(out_hnd, "val_type 1=UNISTR, 3=BYTES, 4=DWORD supported\n");
		return;
	}

	if (!next_token_nr(NULL, tmp, NULL, sizeof(tmp)))
	{
		fprintf(out_hnd, "regcreate <val_name> <val_type (1|4)> <val>\n");
		return;
	}

	switch (val_type)
	{
		case 0x01: /* UNISTR */
		{
			init_rpc_blob_str(&value, tmp, strlen(tmp)+1);
			break;
		}
		case 0x03: /* BYTES */
		{
			init_rpc_blob_hex(&value, tmp);
			break;
		}
		case 0x04: /* DWORD */
		{
			uint32 tmp_val;
			if (strnequal(tmp, "0x", 2))
			{
				tmp_val = strtol(tmp, (char**)NULL, 16);
			}
			else
			{
				tmp_val = strtol(tmp, (char**)NULL, 10);
			}
			init_rpc_blob_uint32(&value, tmp_val);
			break;
		}
		default:
		{
			fprintf(out_hnd, "i told you i only deal with UNISTR, DWORD and BYTES!\n");
			return;
		}
	}
		
	DEBUG(10,("key data:\n"));
	dump_data(10, (char *)value.buffer, value.buf_len);

	/* open WINREG session. */
	res = res ? cli_nt_session_open(smb_cli, PI_WINREG) : False;

	/* open registry receive a policy handle */
	res = res ? do_reg_connect(smb_cli, keyname, parent_name,
				&info->dom.reg_pol_connect) : False;

	if ((*val_name) != 0)
	{
		/* open an entry */
		res3 = res  ? do_reg_open_entry(smb_cli, &info->dom.reg_pol_connect,
					 parent_name, 0x02000000, &parent_pol) : False;
	}
	else
	{
		memcpy(&parent_pol, &info->dom.reg_pol_connect, sizeof(parent_pol));
	}

	/* create an entry */
	res4 = res3 ? do_reg_set_val(smb_cli, &parent_pol,
				 val_name, val_type, &value) : False;

	/* flush the modified key */
	res4 = res4 ? do_reg_flush_key(smb_cli, &parent_pol) : False;

	/* close the val handle */
	if ((*val_name) != 0)
	{
		res3 = res3 ? do_reg_close(smb_cli, &parent_pol) : False;
	}

	/* close the registry handles */
	res  = res  ? do_reg_close(smb_cli, &info->dom.reg_pol_connect) : False;

	/* close the session */
	cli_nt_session_close(smb_cli);

	if (res && res3 && res4)
	{
		DEBUG(5,("cmd_reg_set_val: query succeeded\n"));
		fprintf(out_hnd,"OK\n");
	}
	else
	{
		DEBUG(5,("cmd_reg_set_val: query failed\n"));
	}
}

/****************************************************************************
nt registry delete value
****************************************************************************/
static void cmd_reg_delete_val(struct client_info *info)
{
	BOOL res = True;
	BOOL res3 = True;
	BOOL res4 = True;

	POLICY_HND parent_pol;
	fstring full_keyname;
	fstring keyname;
	fstring parent_name;
	fstring val_name;

	DEBUG(5, ("cmd_reg_delete_val: smb_cli->fd:%d\n", smb_cli->fd));

	if (!next_token_nr(NULL, full_keyname, NULL, sizeof(full_keyname)))
	{
		fprintf(out_hnd, "regdelete <val_name>\n");
		return;
	}

	reg_get_subkey(full_keyname, keyname, val_name);

	if (keyname[0] == 0 || val_name[0] == 0)
	{
		fprintf(out_hnd, "invalid key name\n");
		return;
	}
	
	/* open WINREG session. */
	res = res ? cli_nt_session_open(smb_cli, PI_WINREG) : False;

	/* open registry receive a policy handle */
	res = res ? do_reg_connect(smb_cli, keyname, parent_name,
				&info->dom.reg_pol_connect) : False;

	if ((*val_name) != 0)
	{
		/* open an entry */
		res3 = res  ? do_reg_open_entry(smb_cli, &info->dom.reg_pol_connect,
					 parent_name, 0x02000000, &parent_pol) : False;
	}
	else
	{
		memcpy(&parent_pol, &info->dom.reg_pol_connect, sizeof(parent_pol));
	}

	/* delete an entry */
	res4 = res3 ? do_reg_delete_val(smb_cli, &parent_pol, val_name) : False;

	/* flush the modified key */
	res4 = res4 ? do_reg_flush_key(smb_cli, &parent_pol) : False;

	/* close the key handle */
	res3 = res3 ? do_reg_close(smb_cli, &parent_pol) : False;

	/* close the registry handles */
	res  = res  ? do_reg_close(smb_cli, &info->dom.reg_pol_connect) : False;

	/* close the session */
	cli_nt_session_close(smb_cli);

	if (res && res3 && res4)
	{
		DEBUG(5,("cmd_reg_delete_val: query succeeded\n"));
		fprintf(out_hnd,"OK\n");
	}
	else
	{
		DEBUG(5,("cmd_reg_delete_val: query failed\n"));
	}
}

/****************************************************************************
nt registry delete key
****************************************************************************/
static void cmd_reg_delete_key(struct client_info *info)
{
	BOOL res = True;
	BOOL res3 = True;
	BOOL res4 = True;

	POLICY_HND parent_pol;
	fstring full_keyname;
	fstring parent_name;
	fstring key_name;
	fstring subkey_name;

	DEBUG(5, ("cmd_reg_delete_key: smb_cli->fd:%d\n", smb_cli->fd));

	if (!next_token_nr(NULL, full_keyname, NULL, sizeof(full_keyname)))
	{
		fprintf(out_hnd, "regdeletekey <key_name>\n");
		return;
	}

	reg_get_subkey(full_keyname, parent_name, subkey_name);

	if (parent_name[0] == 0 || subkey_name[0] == 0)
	{
		fprintf(out_hnd, "invalid key name\n");
		return;
	}
	
	/* open WINREG session. */
	res = res ? cli_nt_session_open(smb_cli, PI_WINREG) : False;

	/* open registry receive a policy handle */
	res = res ? do_reg_connect(smb_cli, parent_name, key_name,
				&info->dom.reg_pol_connect) : False;

	if ((*key_name) != 0)
	{
		/* open an entry */
		res3 = res  ? do_reg_open_entry(smb_cli, &info->dom.reg_pol_connect,
					 key_name, 0x02000000, &parent_pol) : False;
	}
	else
	{
		memcpy(&parent_pol, &info->dom.reg_pol_connect, sizeof(parent_pol));
	}

	/* create an entry */
	res4 = res3 ? do_reg_delete_key(smb_cli, &parent_pol, subkey_name) : False;

	/* flush the modified key */
	res4 = res4 ? do_reg_flush_key(smb_cli, &parent_pol) : False;

	/* close the key handle */
	if ((*key_name) != 0)
	{
		res3 = res3 ? do_reg_close(smb_cli, &parent_pol) : False;
	}

	/* close the registry handles */
	res  = res  ? do_reg_close(smb_cli, &info->dom.reg_pol_connect) : False;

	/* close the session */
	cli_nt_session_close(smb_cli);

	if (res && res3 && res4)
	{
		DEBUG(5,("cmd_reg_delete_key: query succeeded\n"));
		fprintf(out_hnd,"OK\n");
	}
	else
	{
		DEBUG(5,("cmd_reg_delete_key: query failed\n"));
	}
}

/****************************************************************************
nt registry create key
****************************************************************************/
static void cmd_reg_create_key(struct client_info *info)
{
	BOOL res = True;
	BOOL res3 = True;
	BOOL res4 = True;

	POLICY_HND parent_pol;
	POLICY_HND key_pol;
	fstring full_keyname;
	fstring parent_key;
	fstring parent_name;
	fstring key_name;
	fstring key_class;
	SEC_ACCESS sam_access;

	DEBUG(5, ("cmd_reg_create_key: smb_cli->fd:%d\n", smb_cli->fd));

	if (!next_token_nr(NULL, full_keyname, NULL, sizeof(full_keyname)))
	{
		fprintf(out_hnd, "regcreate <key_name> [key_class]\n");
		return;
	}

	reg_get_subkey(full_keyname, parent_key, key_name);

	if (parent_key[0] == 0 || key_name[0] == 0)
	{
		fprintf(out_hnd, "invalid key name\n");
		return;
	}
	
	if (!next_token_nr(NULL, key_class, NULL, sizeof(key_class)))
	{
		memset(key_class, 0, sizeof(key_class));
	}

	/* set access permissions */
	sam_access.mask = SEC_RIGHTS_READ;

	/* open WINREG session. */
	res = res ? cli_nt_session_open(smb_cli, PI_WINREG) : False;

	/* open registry receive a policy handle */
	res = res ? do_reg_connect(smb_cli, parent_key, parent_name,
				&info->dom.reg_pol_connect) : False;

	if ((*parent_name) != 0)
	{
		/* open an entry */
		res3 = res  ? do_reg_open_entry(smb_cli, &info->dom.reg_pol_connect,
					 parent_name, 0x02000000, &parent_pol) : False;
	}
	else
	{
		memcpy(&parent_pol, &info->dom.reg_pol_connect, sizeof(parent_pol));
	}

	/* create an entry */
	res4 = res3 ? do_reg_create_key(smb_cli, &parent_pol,
				 key_name, key_class, &sam_access, &key_pol) : False;

	/* flush the modified key */
	res4 = res4 ? do_reg_flush_key(smb_cli, &parent_pol) : False;

	/* close the key handle */
	res4 = res4 ? do_reg_close(smb_cli, &key_pol) : False;

	/* close the key handle */
	if ((*parent_name) != 0)
	{
		res3 = res3 ? do_reg_close(smb_cli, &parent_pol) : False;
	}

	/* close the registry handles */
	res  = res  ? do_reg_close(smb_cli, &info->dom.reg_pol_connect) : False;

	/* close the session */
	cli_nt_session_close(smb_cli);

	if (res && res3 && res4)
	{
		DEBUG(5,("cmd_reg_create_key: query succeeded\n"));
		fprintf(out_hnd,"OK\n");
	}
	else
	{
		DEBUG(5,("cmd_reg_create_key: query failed\n"));
	}
}

/****************************************************************************
nt registry security info
****************************************************************************/
static void cmd_reg_test_key_sec(struct client_info *info)
{
	BOOL res = True;
	BOOL res3 = True;
	BOOL res4 = True;

	POLICY_HND key_pol;
	fstring full_keyname;
	fstring key_name;

	/*
	 * security info
	 */

	uint32 sec_buf_size;
	SEC_DESC_BUF *psdb;

	DEBUG(5, ("cmd_reg_get_key_sec: smb_cli->fd:%d\n", smb_cli->fd));

	if (!next_token_nr(NULL, full_keyname, NULL, sizeof(full_keyname)))
	{
		fprintf(out_hnd, "reggetsec <key_name>\n");
		return;
	}

	/* open WINREG session. */
	res = res ? cli_nt_session_open(smb_cli, PI_WINREG) : False;

	/* open registry receive a policy handle */
	res = res ? do_reg_connect(smb_cli, full_keyname, key_name,
				&info->dom.reg_pol_connect) : False;

	if ((*key_name) != 0)
	{
		/* open an entry */
		res3 = res  ? do_reg_open_entry(smb_cli, &info->dom.reg_pol_connect,
					 key_name, 0x02000000, &key_pol) : False;
	}
	else
	{
		memcpy(&key_pol, &info->dom.reg_pol_connect, sizeof(key_pol));
	}

	/* open an entry */
	res3 = res ? do_reg_open_entry(smb_cli, &info->dom.reg_pol_connect,
				 key_name, 0x02000000, &key_pol) : False;

	/* query key sec info.  first call sets sec_buf_size. */

	sec_buf_size = 0;
	res4 = res3 ? do_reg_get_key_sec(smb_cli, &key_pol,
				&sec_buf_size, &psdb) : False;
	
	free_sec_desc_buf(&psdb);

	res4 = res4 ? do_reg_get_key_sec(smb_cli, &key_pol,
				&sec_buf_size, &psdb) : False;

	if (res4 && psdb->len > 0 && psdb->sec != NULL)
	{
		display_sec_desc(out_hnd, ACTION_HEADER   , psdb->sec);
		display_sec_desc(out_hnd, ACTION_ENUMERATE, psdb->sec);
		display_sec_desc(out_hnd, ACTION_FOOTER   , psdb->sec);

		res4 = res4 ? do_reg_set_key_sec(smb_cli, &key_pol, psdb) : False;
	}

	free_sec_desc_buf(&psdb);

	/* close the key handle */
	if ((*key_name) != 0)
	{
		res3 = res3 ? do_reg_close(smb_cli, &key_pol) : False;
	}

	/* close the registry handles */
	res  = res  ? do_reg_close(smb_cli, &info->dom.reg_pol_connect) : False;

	/* close the session */
	cli_nt_session_close(smb_cli);

	if (res && res3 && res4)
	{
		DEBUG(5,("cmd_reg_test2: query succeeded\n"));
		fprintf(out_hnd,"Registry Test2\n");
	}
	else
	{
		DEBUG(5,("cmd_reg_test2: query failed\n"));
	}
}

/****************************************************************************
nt registry security info
****************************************************************************/
static void cmd_reg_get_key_sec(struct client_info *info)
{
	BOOL res = True;
	BOOL res3 = True;
	BOOL res4 = True;

	POLICY_HND key_pol;
	fstring full_keyname;
	fstring key_name;

	/*
	 * security info
	 */

	uint32 sec_buf_size;
	SEC_DESC_BUF *psdb;

	DEBUG(5, ("cmd_reg_get_key_sec: smb_cli->fd:%d\n", smb_cli->fd));

	if (!next_token_nr(NULL, full_keyname, NULL, sizeof(full_keyname)))
	{
		fprintf(out_hnd, "reggetsec <key_name>\n");
		return;
	}

	/* open WINREG session. */
	res = res ? cli_nt_session_open(smb_cli, PI_WINREG) : False;

	/* open registry receive a policy handle */
	res = res ? do_reg_connect(smb_cli, full_keyname, key_name,
				&info->dom.reg_pol_connect) : False;

	if ((*key_name) != 0)
	{
		/* open an entry */
		res3 = res  ? do_reg_open_entry(smb_cli, &info->dom.reg_pol_connect,
					 key_name, 0x02000000, &key_pol) : False;
	}
	else
	{
		memcpy(&key_pol, &info->dom.reg_pol_connect, sizeof(key_pol));
	}

	/* open an entry */
	res3 = res ? do_reg_open_entry(smb_cli, &info->dom.reg_pol_connect,
				 key_name, 0x02000000, &key_pol) : False;

	/* Get the size. */
	sec_buf_size = 0;
	res4 = res3 ? do_reg_get_key_sec(smb_cli, &key_pol,
				&sec_buf_size, &psdb) : False;
	
	free_sec_desc_buf(&psdb);

	res4 = res4 ? do_reg_get_key_sec(smb_cli, &key_pol,
				&sec_buf_size, &psdb) : False;

	if (res4 && psdb->len > 0 && psdb->sec != NULL)
	{
		display_sec_desc(out_hnd, ACTION_HEADER   , psdb->sec);
		display_sec_desc(out_hnd, ACTION_ENUMERATE, psdb->sec);
		display_sec_desc(out_hnd, ACTION_FOOTER   , psdb->sec);
	}

	free_sec_desc_buf(&psdb);

	/* close the key handle */
	if ((*key_name) != 0)
	{
		res3 = res3 ? do_reg_close(smb_cli, &key_pol) : False;
	}

	/* close the registry handles */
	res  = res  ? do_reg_close(smb_cli, &info->dom.reg_pol_connect) : False;

	/* close the session */
	cli_nt_session_close(smb_cli);

	if (res && res3 && res4)
	{
		DEBUG(5,("cmd_reg_get_key_sec: query succeeded\n"));
	}
	else
	{
		DEBUG(5,("cmd_reg_get_key_sec: query failed\n"));
	}
}


/****************************************************************************
nt registry shutdown
****************************************************************************/
static NTSTATUS cmd_reg_shutdown(struct cli_state *cli, TALLOC_CTX *mem_ctx,
                                 int argc, const char **argv)
{
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;
	fstring msg;
	uint32 timeout = 20;
	BOOL force = False;
	BOOL reboot = False;
	int opt;

	*msg = 0;
	optind = 0; /* TODO: test if this hack works on other systems too --simo */

	while ((opt = getopt(argc, argv, "m:t:rf")) != EOF)
	{
		/*fprintf (stderr, "[%s]\n", argv[argc-1]);*/
	
		switch (opt)
		{
			case 'm':
				fstrcpy(msg, optarg);
				/*fprintf (stderr, "[%s|%s]\n", optarg, msg);*/
				break;

			case 't':
				timeout = atoi(optarg);
				/*fprintf (stderr, "[%s|%d]\n", optarg, timeout);*/
				break;

			case 'r':
				reboot = True;
				break;

			case 'f':
				force = True;
				break;

		}
	}

	/* create an entry */
	result = werror_to_ntstatus(cli_reg_shutdown(cli, mem_ctx, msg, timeout, reboot, force));

	if (NT_STATUS_IS_OK(result))
		DEBUG(5,("cmd_reg_shutdown: query succeeded\n"));
	else
		DEBUG(5,("cmd_reg_shutdown: query failed\n"));

	return result;
}

/****************************************************************************
abort a shutdown
****************************************************************************/
static NTSTATUS cmd_reg_abort_shutdown(struct cli_state *cli, 
                                       TALLOC_CTX *mem_ctx, int argc, 
                                       const char **argv)
{
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;

	result = werror_to_ntstatus(cli_reg_abort_shutdown(cli, mem_ctx));

	if (NT_STATUS_IS_OK(result))
		DEBUG(5,("cmd_reg_abort_shutdown: query succeeded\n"));
	else
		DEBUG(5,("cmd_reg_abort_shutdown: query failed\n"));

	return result;
}

#endif	 /* This whole file need to be rewritten for the cirrent rpcclient interface */


/* List of commands exported by this module */
struct cmd_set reg_commands[] = {

	{ "REG"  },
#if 0
	{ "shutdown", RPC_RTYPE_NTSTATUS, cmd_reg_shutdown, NULL, PI_WINREG, "Remote Shutdown",
				"syntax: shutdown [-m message] [-t timeout] [-r] [-h] [-f] (-r == reboot, -h == halt, -f == force)" },
				
	{ "abortshutdown", RPC_RTYPE_NTSTATUS, cmd_reg_abort_shutdown, NULL, PI_WINREG, "Abort Shutdown",
				"syntax: abortshutdown" },
	{ "regenum",		cmd_reg_enum,			"Registry Enumeration", "<keyname>" },
	{ "regdeletekey",	cmd_reg_delete_key,		"Registry Key Delete", "<keyname>" },
	{ "regcreatekey",	cmd_reg_create_key,		"Registry Key Create", "<keyname> [keyclass]" },
	{ "regqueryval",	cmd_reg_query_info,		"Registry Value Query", "<valname>" },
	{ "regquerykey",	cmd_reg_query_key,		"Registry Key Query", "<keyname>" },
	{ "regdeleteval",	cmd_reg_delete_val,		"Registry Value Delete", "<valname>" },
	{ "regsetval",	        cmd_reg_set_val,		"Registry Key Create", "<valname> <valtype> <value>" },
	{ "reggetsec",		cmd_reg_get_key_sec,		"Registry Key Security", "<keyname>" },
	{ "regtestsec",		cmd_reg_test_key_sec,		"Test Registry Key Security", "<keyname>" },
#endif
	{ NULL }
};
