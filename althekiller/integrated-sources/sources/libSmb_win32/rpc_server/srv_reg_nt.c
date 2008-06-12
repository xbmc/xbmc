/* 
 *  Unix SMB/CIFS implementation.
 *  RPC Pipe client / server routines
 *  Copyright (C) Andrew Tridgell               1992-1997.
 *  Copyright (C) Luke Kenneth Casson Leighton  1996-1997.
 *  Copyright (C) Paul Ashton                        1997.
 *  Copyright (C) Jeremy Allison                     2001.
 *  Copyright (C) Gerald Carter                      2002-2005.
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

/* Implementation of registry functions. */

#include "includes.h"
#include "regfio.h"

#undef DBGC_CLASS
#define DBGC_CLASS DBGC_RPC_SRV

#define OUR_HANDLE(hnd) (((hnd)==NULL)?"NULL":(IVAL((hnd)->data5,4)==(uint32)sys_getpid()?"OURS":"OTHER")), \
((unsigned int)IVAL((hnd)->data5,4)),((unsigned int)sys_getpid())

static struct generic_mapping reg_generic_map = 
	{ REG_KEY_READ, REG_KEY_WRITE, REG_KEY_EXECUTE, REG_KEY_ALL };


/******************************************************************
 free() function for REGISTRY_KEY
 *****************************************************************/
 
static void free_regkey_info(void *ptr)
{
	regkey_close_internal( (REGISTRY_KEY*)ptr );
}

/******************************************************************
 Find a registry key handle and return a REGISTRY_KEY
 *****************************************************************/

static REGISTRY_KEY *find_regkey_index_by_hnd(pipes_struct *p, POLICY_HND *hnd)
{
	REGISTRY_KEY *regkey = NULL;

	if(!find_policy_by_hnd(p,hnd,(void **)(void *)&regkey)) {
		DEBUG(2,("find_regkey_index_by_hnd: Registry Key not found: "));
		return NULL;
	}

	return regkey;
}


/*******************************************************************
 Function for open a new registry handle and creating a handle 
 Note that P should be valid & hnd should already have space
 
 When we open a key, we store the full path to the key as 
 HK[LM|U]\<key>\<key>\...
 *******************************************************************/
 
static WERROR open_registry_key( pipes_struct *p, POLICY_HND *hnd, 
                                 REGISTRY_KEY **keyinfo, REGISTRY_KEY *parent,
				 const char *subkeyname, uint32 access_desired  )
{
	pstring         keypath;
	int		path_len;
	WERROR     	result = WERR_OK;

	/* create a full registry path and strip any trailing '\' 
	   characters */
	   
	pstr_sprintf( keypath, "%s%s%s", 
		parent ? parent->name : "",
		parent ? "\\" : "", 
		subkeyname );
	
	path_len = strlen( keypath );
	if ( path_len && keypath[path_len-1] == '\\' )
		keypath[path_len-1] = '\0';
	
	/* now do the internal open */
		
	result = regkey_open_internal( keyinfo, keypath, p->pipe_user.nt_user_token, access_desired );
	if ( !W_ERROR_IS_OK(result) )
		return result;
	
	if ( !create_policy_hnd( p, hnd, free_regkey_info, *keyinfo ) ) {
		result = WERR_BADFILE; 
		regkey_close_internal( *keyinfo );
	}
	
	return result;
}

/*******************************************************************
 Function for open a new registry handle and creating a handle 
 Note that P should be valid & hnd should already have space
 *******************************************************************/

static BOOL close_registry_key(pipes_struct *p, POLICY_HND *hnd)
{
	REGISTRY_KEY *regkey = find_regkey_index_by_hnd(p, hnd);
	
	if ( !regkey ) {
		DEBUG(2,("close_registry_key: Invalid handle (%s:%u:%u)\n", OUR_HANDLE(hnd)));
		return False;
	}
	
	close_policy_hnd(p, hnd);
	
	return True;
}

/********************************************************************
 retrieve information about the subkeys
 *******************************************************************/
 
static BOOL get_subkey_information( REGISTRY_KEY *key, uint32 *maxnum, uint32 *maxlen )
{
	int 		num_subkeys, i;
	uint32 		max_len;
	REGSUBKEY_CTR 	*subkeys;
	uint32 		len;
	
	if ( !key )
		return False;

	if ( !(subkeys = TALLOC_ZERO_P( NULL, REGSUBKEY_CTR )) )
		return False;

	if ( fetch_reg_keys( key, subkeys ) == -1 )
		return False;

	/* find the longest string */
	
	max_len = 0;
	num_subkeys = regsubkey_ctr_numkeys( subkeys );
	
	for ( i=0; i<num_subkeys; i++ ) {
		len = strlen( regsubkey_ctr_specific_key(subkeys, i) );
		max_len = MAX(max_len, len);
	}

	*maxnum = num_subkeys;
	*maxlen = max_len*2;
	
	TALLOC_FREE( subkeys );
	
	return True;
}

/********************************************************************
 retrieve information about the values.  
 *******************************************************************/
 
static BOOL get_value_information( REGISTRY_KEY *key, uint32 *maxnum, 
                                    uint32 *maxlen, uint32 *maxsize )
{
	REGVAL_CTR 	*values;
	REGISTRY_VALUE	*val;
	uint32 		sizemax, lenmax;
	int 		i, num_values;
	
	if ( !key )
		return False;

	if ( !(values = TALLOC_ZERO_P( NULL, REGVAL_CTR )) )
		return False;
	
	if ( fetch_reg_values( key, values ) == -1 )
		return False;
	
	lenmax = sizemax = 0;
	num_values = regval_ctr_numvals( values );
	
	val = regval_ctr_specific_value( values, 0 );
	
	for ( i=0; i<num_values && val; i++ ) 
	{
		lenmax  = MAX(lenmax,  val->valuename ? strlen(val->valuename)+1 : 0 );
		sizemax = MAX(sizemax, val->size );
		
		val = regval_ctr_specific_value( values, i );
	}

	*maxnum   = num_values;
	*maxlen   = lenmax;
	*maxsize  = sizemax;
	
	TALLOC_FREE( values );
	
	return True;
}


/********************************************************************
 reg_close
 ********************************************************************/

WERROR _reg_close(pipes_struct *p, REG_Q_CLOSE *q_u, REG_R_CLOSE *r_u)
{
	/* close the policy handle */

	if (!close_registry_key(p, &q_u->pol))
		return WERR_BADFID; 

	return WERR_OK;
}

/*******************************************************************
 ********************************************************************/

WERROR _reg_open_hklm(pipes_struct *p, REG_Q_OPEN_HIVE *q_u, REG_R_OPEN_HIVE *r_u)
{
	REGISTRY_KEY *keyinfo;
	
	return open_registry_key( p, &r_u->pol, &keyinfo, NULL, KEY_HKLM, q_u->access );
}

/*******************************************************************
 ********************************************************************/

WERROR _reg_open_hkpd(pipes_struct *p, REG_Q_OPEN_HIVE *q_u, REG_R_OPEN_HIVE *r_u)
{
	REGISTRY_KEY *keyinfo;
	
	return open_registry_key( p, &r_u->pol, &keyinfo, NULL, KEY_HKPD, q_u->access );
}

/*******************************************************************
 ********************************************************************/

WERROR _reg_open_hkpt(pipes_struct *p, REG_Q_OPEN_HIVE *q_u, REG_R_OPEN_HIVE *r_u)
{
	REGISTRY_KEY *keyinfo;
	
	return open_registry_key( p, &r_u->pol, &keyinfo, NULL, KEY_HKPT, q_u->access );
}

/*******************************************************************
 ********************************************************************/

WERROR _reg_open_hkcr(pipes_struct *p, REG_Q_OPEN_HIVE *q_u, REG_R_OPEN_HIVE *r_u)
{
	REGISTRY_KEY *keyinfo;
	
	return open_registry_key( p, &r_u->pol, &keyinfo, NULL, KEY_HKCR, q_u->access );
}

/*******************************************************************
 ********************************************************************/

WERROR _reg_open_hku(pipes_struct *p, REG_Q_OPEN_HIVE *q_u, REG_R_OPEN_HIVE *r_u)
{
	REGISTRY_KEY *keyinfo;
	
	return open_registry_key( p, &r_u->pol, &keyinfo, NULL, KEY_HKU, q_u->access );
}

/*******************************************************************
 reg_reply_open_entry
 ********************************************************************/

WERROR _reg_open_entry(pipes_struct *p, REG_Q_OPEN_ENTRY *q_u, REG_R_OPEN_ENTRY *r_u)
{
	fstring name;
	REGISTRY_KEY *parent = find_regkey_index_by_hnd(p, &q_u->pol);
	REGISTRY_KEY *newkey = NULL;
	uint32 check_rights;

	if ( !parent )
		return WERR_BADFID;

	rpcstr_pull( name, q_u->name.string->buffer, sizeof(name), q_u->name.string->uni_str_len*2, 0 );
	
	/* check granted access first; what is the correct mask here? */

	check_rights = ( SEC_RIGHTS_ENUM_SUBKEYS|
                         SEC_RIGHTS_CREATE_SUBKEY|
			 SEC_RIGHTS_QUERY_VALUE|
			 SEC_RIGHTS_SET_VALUE);

	if ( !(parent->access_granted & check_rights) ) {
	        DEBUG(8,("Rights check failed, parent had %04x, check_rights %04x\n",parent->access_granted, check_rights));
		return WERR_ACCESS_DENIED;
	}
	
	/* 
	 * very crazy, but regedit.exe on Win2k will attempt to call 
	 * REG_OPEN_ENTRY with a keyname of "".  We should return a new 
	 * (second) handle here on the key->name.  regedt32.exe does 
	 * not do this stupidity.   --jerry
	 */
	 
	return open_registry_key( p, &r_u->handle, &newkey, parent, name, q_u->access );
}

/*******************************************************************
 reg_reply_info
 ********************************************************************/

WERROR _reg_query_value(pipes_struct *p, REG_Q_QUERY_VALUE *q_u, REG_R_QUERY_VALUE *r_u)
{
	WERROR			status = WERR_BADFILE;
	fstring 		name;
	REGISTRY_KEY 		*regkey = find_regkey_index_by_hnd( p, &q_u->pol );
	REGISTRY_VALUE		*val = NULL;
	REGVAL_CTR		*regvals;
	int			i;

	if ( !regkey )
		return WERR_BADFID;
		
	DEBUG(7,("_reg_info: policy key name = [%s]\n", regkey->name));
	DEBUG(7,("_reg_info: policy key type = [%08x]\n", regkey->type));
	
	rpcstr_pull(name, q_u->name.string->buffer, sizeof(name), q_u->name.string->uni_str_len*2, 0);

	DEBUG(5,("_reg_info: looking up value: [%s]\n", name));

	if ( !(regvals = TALLOC_ZERO_P( p->mem_ctx, REGVAL_CTR )) ) 
		return WERR_NOMEM;
	
	/* Handle QueryValue calls on HKEY_PERFORMANCE_DATA */
	if(regkey->type == REG_KEY_HKPD) 
	{
		if(strequal(name, "Global"))
		{
			uint32 outbuf_len;
			prs_struct prs_hkpd;
			prs_init(&prs_hkpd, q_u->bufsize, p->mem_ctx, MARSHALL);
			status = reg_perfcount_get_hkpd(&prs_hkpd, q_u->bufsize, &outbuf_len, NULL);
			regval_ctr_addvalue(regvals, "HKPD", REG_BINARY,
					    prs_hkpd.data_p, outbuf_len);
			val = dup_registry_value(regval_ctr_specific_value(regvals, 0));
			prs_mem_free(&prs_hkpd);
		}
		else if(strequal(name, "Counter 009"))
		{
			uint32 base_index;
			uint32 buffer_size;
			char *buffer;
			
			buffer = NULL;
			base_index = reg_perfcount_get_base_index();
			buffer_size = reg_perfcount_get_counter_names(base_index, &buffer);
			regval_ctr_addvalue(regvals, "Counter 009", 
					    REG_MULTI_SZ, buffer, buffer_size);
			
			val = dup_registry_value(regval_ctr_specific_value(regvals, 0));
			
			if(buffer_size > 0)
			{
				SAFE_FREE(buffer);
				status = WERR_OK;
			}
		}
		else if(strequal(name, "Explain 009"))
		{		
			uint32 base_index;
			uint32 buffer_size;
			char *buffer;
			
			buffer = NULL;
			base_index = reg_perfcount_get_base_index();
			buffer_size = reg_perfcount_get_counter_help(base_index, &buffer);
			regval_ctr_addvalue(regvals, "Explain 009", 
					    REG_MULTI_SZ, buffer, buffer_size);
			
			val = dup_registry_value(regval_ctr_specific_value(regvals, 0));
			
			if(buffer_size > 0)
			{
				SAFE_FREE(buffer);
				status = WERR_OK;
			}
		}
		else if(isdigit(name[0]))
		{
			/* we probably have a request for a specific object here */
			uint32 outbuf_len;
			prs_struct prs_hkpd;
			prs_init(&prs_hkpd, q_u->bufsize, p->mem_ctx, MARSHALL);
			status = reg_perfcount_get_hkpd(&prs_hkpd, q_u->bufsize, &outbuf_len, name);
			regval_ctr_addvalue(regvals, "HKPD", REG_BINARY,
					    prs_hkpd.data_p, outbuf_len);
			
			val = dup_registry_value(regval_ctr_specific_value(regvals, 0));
			prs_mem_free(&prs_hkpd);
		}
		else
		{
			DEBUG(3,("Unsupported key name [%s] for HKPD.\n", name));
			return WERR_BADFILE;
		}
	}
	/* HKPT calls can be handled out of reg_dynamic.c with the hkpt_params handler */
	else
	{
	    for ( i=0; fetch_reg_values_specific(regkey, &val, i); i++ ) 
	    {
		DEBUG(10,("_reg_info: Testing value [%s]\n", val->valuename));
		if ( strequal( val->valuename, name ) ) {
			DEBUG(10,("_reg_info: Found match for value [%s]\n", name));
			status = WERR_OK;
			break;
		}
		
		free_registry_value( val );
	    }
	}

	init_reg_r_query_value(q_u->ptr_buf, r_u, val, status);
	
	TALLOC_FREE( regvals );
	free_registry_value( val );

	return status;
}

/*****************************************************************************
 Implementation of REG_QUERY_KEY
 ****************************************************************************/
 
WERROR _reg_query_key(pipes_struct *p, REG_Q_QUERY_KEY *q_u, REG_R_QUERY_KEY *r_u)
{
	WERROR 	status = WERR_OK;
	REGISTRY_KEY	*regkey = find_regkey_index_by_hnd( p, &q_u->pol );
	
	if ( !regkey )
		return WERR_BADFID; 
	
	if ( !get_subkey_information( regkey, &r_u->num_subkeys, &r_u->max_subkeylen ) ) {
		DEBUG(0,("_reg_query_key: get_subkey_information() failed!\n"));
		return WERR_ACCESS_DENIED;
	}
		
	if ( !get_value_information( regkey, &r_u->num_values, &r_u->max_valnamelen, &r_u->max_valbufsize ) ) {
		DEBUG(0,("_reg_query_key: get_value_information() failed!\n"));
		return WERR_ACCESS_DENIED;	
	}

		
	r_u->sec_desc = 0x00000078;	/* size for key's sec_desc */
	
	/* Win9x set this to 0x0 since it does not keep timestamps.
	   Doing the same here for simplicity   --jerry */
	   
	ZERO_STRUCT(r_u->mod_time);	

	return status;
}


/*****************************************************************************
 Implementation of REG_GETVERSION
 ****************************************************************************/
 
WERROR _reg_getversion(pipes_struct *p, REG_Q_GETVERSION *q_u, REG_R_GETVERSION *r_u)
{
	WERROR 	status = WERR_OK;
	REGISTRY_KEY	*regkey = find_regkey_index_by_hnd( p, &q_u->pol );
	
	if ( !regkey )
		return WERR_BADFID;
	
	r_u->win_version = 0x00000005;	/* Windows 2000 registry API version */
	
	return status;
}


/*****************************************************************************
 Implementation of REG_ENUM_KEY
 ****************************************************************************/
 
WERROR _reg_enum_key(pipes_struct *p, REG_Q_ENUM_KEY *q_u, REG_R_ENUM_KEY *r_u)
{
	WERROR 	status = WERR_OK;
	REGISTRY_KEY	*regkey = find_regkey_index_by_hnd( p, &q_u->pol );
	char		*subkey = NULL;
	
	
	if ( !regkey )
		return WERR_BADFID; 

	DEBUG(8,("_reg_enum_key: enumerating key [%s]\n", regkey->name));
	
	if ( !fetch_reg_keys_specific( regkey, &subkey, q_u->key_index ) )
	{
		status = WERR_NO_MORE_ITEMS;
		goto done;
	}
	
	DEBUG(10,("_reg_enum_key: retrieved subkey named [%s]\n", subkey));
	
	/* subkey has the string name now */
	
	init_reg_r_enum_key( r_u, subkey );
	
done:	
	SAFE_FREE( subkey );
	return status;
}

/*****************************************************************************
 Implementation of REG_ENUM_VALUE
 ****************************************************************************/
 
WERROR _reg_enum_value(pipes_struct *p, REG_Q_ENUM_VALUE *q_u, REG_R_ENUM_VALUE *r_u)
{
	WERROR 	status = WERR_OK;
	REGISTRY_KEY	*regkey = find_regkey_index_by_hnd( p, &q_u->pol );
	REGISTRY_VALUE	*val;
	
	
	if ( !regkey )
		return WERR_BADFID; 

	DEBUG(8,("_reg_enum_value: enumerating values for key [%s]\n", regkey->name));

	if ( !fetch_reg_values_specific( regkey, &val, q_u->val_index ) ) {
		status = WERR_NO_MORE_ITEMS;
		goto done;
	}

#if 0	/* JERRY TEST CODE */
	if ( val->type == REG_MULTI_SZ ) {
		char **str;
		int num_strings = regval_convert_multi_sz( (uint16*)regval_data_p(val), regval_size(val), &str );
		uint16 *buffer;
		size_t buf_size;
		
		
		if ( num_strings )
			buf_size = regval_build_multi_sz( str, &buffer );
		
		TALLOC_FREE( str );
		TALLOC_FREE( buffer );
	}
#endif
	
	DEBUG(10,("_reg_enum_value: retrieved value named  [%s]\n", val->valuename));
	
	/* subkey has the string name now */
	
	init_reg_r_enum_val( r_u, val );

done:	
	free_registry_value( val );
	
	return status;
}


/*******************************************************************
 reg_shutdwon
 ********************************************************************/

WERROR _reg_shutdown(pipes_struct *p, REG_Q_SHUTDOWN *q_u, REG_R_SHUTDOWN *r_u)
{
	REG_Q_SHUTDOWN_EX q_u_ex;
	REG_R_SHUTDOWN_EX r_u_ex;
	
	/* copy fields (including stealing memory) */
	
	q_u_ex.server  = q_u->server;
	q_u_ex.message = q_u->message;
	q_u_ex.timeout = q_u->timeout;
	q_u_ex.force   = q_u->force;
	q_u_ex.reboot  = q_u->reboot;
	q_u_ex.reason  = 0x0; 	/* don't care for now */
	
	/* thunk down to _reg_shutdown_ex() (just returns a status) */
	
	return _reg_shutdown_ex( p, &q_u_ex, &r_u_ex );
}

/*******************************************************************
 reg_shutdown_ex
 ********************************************************************/

#define SHUTDOWN_R_STRING "-r"
#define SHUTDOWN_F_STRING "-f"


WERROR _reg_shutdown_ex(pipes_struct *p, REG_Q_SHUTDOWN_EX *q_u, REG_R_SHUTDOWN_EX *r_u)
{
	pstring shutdown_script;
	pstring message;
	pstring chkmsg;
	fstring timeout;
	fstring reason;
	fstring r;
	fstring f;
	int ret;
	BOOL can_shutdown;
	

 	pstrcpy(shutdown_script, lp_shutdown_script());
	
	if ( !*shutdown_script )
		return WERR_ACCESS_DENIED;

	/* pull the message string and perform necessary sanity checks on it */

	pstrcpy( message, "" );
	if ( q_u->message ) {
		UNISTR2 *msg_string = q_u->message->string;
		
		rpcstr_pull( message, msg_string->buffer, sizeof(message), msg_string->uni_str_len*2, 0 );
	}
	alpha_strcpy (chkmsg, message, NULL, sizeof(message));
		
	fstr_sprintf(timeout, "%d", q_u->timeout);
	fstr_sprintf(r, (q_u->reboot) ? SHUTDOWN_R_STRING : "");
	fstr_sprintf(f, (q_u->force) ? SHUTDOWN_F_STRING : "");
	fstr_sprintf( reason, "%d", q_u->reason );

	all_string_sub( shutdown_script, "%z", chkmsg, sizeof(shutdown_script) );
	all_string_sub( shutdown_script, "%t", timeout, sizeof(shutdown_script) );
	all_string_sub( shutdown_script, "%r", r, sizeof(shutdown_script) );
	all_string_sub( shutdown_script, "%f", f, sizeof(shutdown_script) );
	all_string_sub( shutdown_script, "%x", reason, sizeof(shutdown_script) );

	can_shutdown = user_has_privileges( p->pipe_user.nt_user_token, &se_remote_shutdown );
		
	/* IF someone has privs, run the shutdown script as root. OTHERWISE run it as not root
	   Take the error return from the script and provide it as the Windows return code. */
	   
	/********** BEGIN SeRemoteShutdownPrivilege BLOCK **********/
	
	if ( can_shutdown ) 
		become_root();

	ret = smbrun( shutdown_script, NULL );
		
	if ( can_shutdown )
		unbecome_root();

	/********** END SeRemoteShutdownPrivilege BLOCK **********/
	
	DEBUG(3,("_reg_shutdown_ex: Running the command `%s' gave %d\n",
		shutdown_script, ret));
		

	return (ret == 0) ? WERR_OK : WERR_ACCESS_DENIED;
}




/*******************************************************************
 reg_abort_shutdwon
 ********************************************************************/

WERROR _reg_abort_shutdown(pipes_struct *p, REG_Q_ABORT_SHUTDOWN *q_u, REG_R_ABORT_SHUTDOWN *r_u)
{
	pstring abort_shutdown_script;
	int ret;
	BOOL can_shutdown;

	pstrcpy(abort_shutdown_script, lp_abort_shutdown_script());

	if ( !*abort_shutdown_script )
		return WERR_ACCESS_DENIED;
		
	can_shutdown = user_has_privileges( p->pipe_user.nt_user_token, &se_remote_shutdown );
		
	/********** BEGIN SeRemoteShutdownPrivilege BLOCK **********/
	
	if ( can_shutdown )
		become_root();
		
	ret = smbrun( abort_shutdown_script, NULL );
	
	if ( can_shutdown )
		unbecome_root();
		
	/********** END SeRemoteShutdownPrivilege BLOCK **********/

	DEBUG(3,("_reg_abort_shutdown: Running the command `%s' gave %d\n",
		abort_shutdown_script, ret));
		

	return (ret == 0) ? WERR_OK : WERR_ACCESS_DENIED;
}

/*******************************************************************
 ********************************************************************/

static int validate_reg_filename( pstring fname )
{
	char *p;
	int num_services = lp_numservices();
	int snum;
	pstring share_path;
	pstring unix_fname;
	
	/* convert to a unix path, stripping the C:\ along the way */
	
	if ( !(p = valid_share_pathname( fname ) ))
		return -1;

	/* has to exist within a valid file share */
			
	for ( snum=0; snum<num_services; snum++ ) {
	
		if ( !lp_snum_ok(snum) || lp_print_ok(snum) )
			continue;
		
		pstrcpy( share_path, lp_pathname(snum) );

		/* make sure we have a path (e.g. [homes] ) */

		if ( strlen( share_path ) == 0 )
			continue;

		if ( strncmp( share_path, p, strlen( share_path )) == 0 )
			break;
	}
	
	/* p and fname are overlapping memory so copy out and back in again */
	
	pstrcpy( unix_fname, p );
	pstrcpy( fname, unix_fname );
	
	return (snum < num_services) ? snum : -1;
}

/*******************************************************************
 Note: topkeypat is the *full* path that this *key will be 
 loaded into (including the name of the key)
 ********************************************************************/

static WERROR reg_load_tree( REGF_FILE *regfile, const char *topkeypath,
                             REGF_NK_REC *key )
{
	REGF_NK_REC *subkey;
	REGISTRY_KEY registry_key;
	REGVAL_CTR *values;
	REGSUBKEY_CTR *subkeys;
	int i;
	pstring path;
	WERROR result = WERR_OK;
	
	/* initialize the REGISTRY_KEY structure */
	
	if ( !(registry_key.hook = reghook_cache_find(topkeypath)) ) {
		DEBUG(0,("reg_load_tree: Failed to assigned a REGISTRY_HOOK to [%s]\n",
			topkeypath ));
		return WERR_BADFILE;
	}
	pstrcpy( registry_key.name, topkeypath );
	
	/* now start parsing the values and subkeys */

	if ( !(subkeys = TALLOC_ZERO_P( regfile->mem_ctx, REGSUBKEY_CTR )) )
		return WERR_NOMEM;
	
	if ( !(values = TALLOC_ZERO_P( subkeys, REGVAL_CTR )) )
		return WERR_NOMEM;

	/* copy values into the REGVAL_CTR */
	
	for ( i=0; i<key->num_values; i++ ) {
		regval_ctr_addvalue( values, key->values[i].valuename, key->values[i].type,
			(char*)key->values[i].data, (key->values[i].data_size & ~VK_DATA_IN_OFFSET) );
	}

	/* copy subkeys into the REGSUBKEY_CTR */
	
	key->subkey_index = 0;
	while ( (subkey = regfio_fetch_subkey( regfile, key )) ) {
		regsubkey_ctr_addkey( subkeys, subkey->keyname );
	}
	
	/* write this key and values out */
	
	if ( !store_reg_values( &registry_key, values ) 
		|| !store_reg_keys( &registry_key, subkeys ) )
	{
		DEBUG(0,("reg_load_tree: Failed to load %s!\n", topkeypath));
		result = WERR_REG_IO_FAILURE;
	}
	
	TALLOC_FREE( subkeys );
	
	if ( !W_ERROR_IS_OK(result) )
		return result;
	
	/* now continue to load each subkey registry tree */

	key->subkey_index = 0;
	while ( (subkey = regfio_fetch_subkey( regfile, key )) ) {
		pstr_sprintf( path, "%s%s%s", topkeypath, "\\", subkey->keyname );
		result = reg_load_tree( regfile, path, subkey );
		if ( !W_ERROR_IS_OK(result) )
			break;
	}

	return result;
}

/*******************************************************************
 ********************************************************************/

static WERROR restore_registry_key ( REGISTRY_KEY *krecord, const char *fname )
{
	REGF_FILE *regfile;
	REGF_NK_REC *rootkey;
	WERROR result;
		
	/* open the registry file....fail if the file already exists */
	
	if ( !(regfile = regfio_open( fname, (O_RDONLY), 0 )) ) {
                DEBUG(0,("backup_registry_key: failed to open \"%s\" (%s)\n", 
			fname, strerror(errno) ));
		return ( ntstatus_to_werror(map_nt_error_from_unix( errno )) );
        }
	
	/* get the rootkey from the regf file and then load the tree
	   via recursive calls */
	   
	if ( !(rootkey = regfio_rootkey( regfile )) ) {
		regfio_close( regfile );
		return WERR_REG_FILE_INVALID;
	}
	
	result = reg_load_tree( regfile, krecord->name, rootkey );
		
	/* cleanup */
	
	regfio_close( regfile );
	
	return result;
}

/*******************************************************************
 ********************************************************************/

WERROR _reg_restore_key(pipes_struct *p, REG_Q_RESTORE_KEY  *q_u, REG_R_RESTORE_KEY *r_u)
{
	REGISTRY_KEY	*regkey = find_regkey_index_by_hnd( p, &q_u->pol );
	pstring         filename;
	int             snum;
	
	if ( !regkey )
		return WERR_BADFID; 

	rpcstr_pull(filename, q_u->filename.string->buffer, sizeof(filename), q_u->filename.string->uni_str_len*2, STR_TERMINATE);

	DEBUG(8,("_reg_restore_key: verifying restore of key [%s] from \"%s\"\n", regkey->name, filename));

	if ( (snum = validate_reg_filename( filename )) == -1 )
		return WERR_OBJECT_PATH_INVALID;
		
	/* user must posses SeRestorePrivilege for this this proceed */
	
	if ( !user_has_privileges( p->pipe_user.nt_user_token, &se_restore ) )
		return WERR_ACCESS_DENIED;
		
	DEBUG(2,("_reg_restore_key: Restoring [%s] from %s in share %s\n", regkey->name, filename, lp_servicename(snum) ));

	return restore_registry_key( regkey, filename );
}

/********************************************************************
********************************************************************/

static WERROR reg_write_tree( REGF_FILE *regfile, const char *keypath,
                              REGF_NK_REC *parent, SEC_DESC *sec_desc )
{
	REGF_NK_REC *key;
	REGVAL_CTR *values;
	REGSUBKEY_CTR *subkeys;
	int i, num_subkeys;
	pstring key_tmp;
	char *keyname, *parentpath;
	pstring subkeypath;
	char *subkeyname;
	REGISTRY_KEY registry_key;
	WERROR result = WERR_OK;
	
	if ( !regfile )
		return WERR_GENERAL_FAILURE;
		
	if ( !keypath )
		return WERR_OBJECT_PATH_INVALID;
		
	/* split up the registry key path */
	
	pstrcpy( key_tmp, keypath );
	if ( !reg_split_key( key_tmp, &parentpath, &keyname ) )
		return WERR_OBJECT_PATH_INVALID;

	if ( !keyname )
		keyname = parentpath;

	/* we need a REGISTRY_KEY object here to enumerate subkeys and values */
	
	ZERO_STRUCT( registry_key );
	pstrcpy( registry_key.name, keypath );
	if ( !(registry_key.hook = reghook_cache_find( registry_key.name )) )
		return WERR_BADFILE;

	
	/* lookup the values and subkeys */
	
	if ( !(subkeys = TALLOC_ZERO_P( regfile->mem_ctx, REGSUBKEY_CTR )) )
		return WERR_NOMEM;

	if ( !(values = TALLOC_ZERO_P( subkeys, REGVAL_CTR )) )
		return WERR_NOMEM;

	fetch_reg_keys( &registry_key, subkeys );
	fetch_reg_values( &registry_key, values );

	/* write out this key */
		
	if ( !(key = regfio_write_key( regfile, keyname, values, subkeys, sec_desc, parent )) ) {
		result = WERR_CAN_NOT_COMPLETE;
		goto done;
	}

	/* write each one of the subkeys out */

	num_subkeys = regsubkey_ctr_numkeys( subkeys );
	for ( i=0; i<num_subkeys; i++ ) {
		subkeyname = regsubkey_ctr_specific_key( subkeys, i );
		pstr_sprintf( subkeypath, "%s\\%s", keypath, subkeyname );
		result = reg_write_tree( regfile, subkeypath, key, sec_desc );
		if ( !W_ERROR_IS_OK(result) )
			goto done;
	}

	DEBUG(6,("reg_write_tree: wrote key [%s]\n", keypath ));

done:
	TALLOC_FREE( subkeys );

	return result;
}

/*******************************************************************
 ********************************************************************/

static WERROR make_default_reg_sd( TALLOC_CTX *ctx, SEC_DESC **psd )
{
	DOM_SID adm_sid, owner_sid;
	SEC_ACE ace[2];         /* at most 2 entries */
	SEC_ACCESS mask;
	SEC_ACL *psa = NULL;
	size_t sd_size;

	/* set the owner to BUILTIN\Administrator */

	sid_copy(&owner_sid, &global_sid_Builtin);
	sid_append_rid(&owner_sid, DOMAIN_USER_RID_ADMIN );
	

	/* basic access for Everyone */

	init_sec_access(&mask, reg_generic_map.generic_execute | reg_generic_map.generic_read );
	init_sec_ace(&ace[0], &global_sid_World, SEC_ACE_TYPE_ACCESS_ALLOWED, mask, 0);

	/* add Full Access 'BUILTIN\Administrators' */

	init_sec_access(&mask, reg_generic_map.generic_all);
	sid_copy(&adm_sid, &global_sid_Builtin);
	sid_append_rid(&adm_sid, BUILTIN_ALIAS_RID_ADMINS);
	init_sec_ace(&ace[1], &adm_sid, SEC_ACE_TYPE_ACCESS_ALLOWED, mask, 0);

        /* create the security descriptor */

        if ((psa = make_sec_acl(ctx, NT4_ACL_REVISION, 2, ace)) == NULL)
                return WERR_NOMEM;

        if ((*psd = make_sec_desc(ctx, SEC_DESC_REVISION, SEC_DESC_SELF_RELATIVE, &owner_sid, NULL, NULL, psa, &sd_size)) == NULL)
                return WERR_NOMEM;

	return WERR_OK;
}

/*******************************************************************
 ********************************************************************/

static WERROR backup_registry_key ( REGISTRY_KEY *krecord, const char *fname )
{
	REGF_FILE *regfile;
	WERROR result;
	SEC_DESC *sd = NULL;
	
	/* open the registry file....fail if the file already exists */
	
	if ( !(regfile = regfio_open( fname, (O_RDWR|O_CREAT|O_EXCL), (S_IREAD|S_IWRITE) )) ) {
                DEBUG(0,("backup_registry_key: failed to open \"%s\" (%s)\n", 
			fname, strerror(errno) ));
		return ( ntstatus_to_werror(map_nt_error_from_unix( errno )) );
        }
	
	if ( !W_ERROR_IS_OK(result = make_default_reg_sd( regfile->mem_ctx, &sd )) ) {
		regfio_close( regfile );
		return result;
	}
		
	/* write the registry tree to the file  */
	
	result = reg_write_tree( regfile, krecord->name, NULL, sd );
		
	/* cleanup */
	
	regfio_close( regfile );
	
	return result;
}

/*******************************************************************
 ********************************************************************/

WERROR _reg_save_key(pipes_struct *p, REG_Q_SAVE_KEY  *q_u, REG_R_SAVE_KEY *r_u)
{
	REGISTRY_KEY	*regkey = find_regkey_index_by_hnd( p, &q_u->pol );
	pstring         filename;
	int             snum;
	
	if ( !regkey )
		return WERR_BADFID; 

	rpcstr_pull(filename, q_u->filename.string->buffer, sizeof(filename), q_u->filename.string->uni_str_len*2, STR_TERMINATE);

	DEBUG(8,("_reg_save_key: verifying backup of key [%s] to \"%s\"\n", regkey->name, filename));
	
	if ( (snum = validate_reg_filename( filename )) == -1 )
		return WERR_OBJECT_PATH_INVALID;
		
	DEBUG(2,("_reg_save_key: Saving [%s] to %s in share %s\n", regkey->name, filename, lp_servicename(snum) ));
		
	return backup_registry_key( regkey, filename );
}

/*******************************************************************
 ********************************************************************/

WERROR _reg_create_key_ex(pipes_struct *p, REG_Q_CREATE_KEY_EX *q_u, REG_R_CREATE_KEY_EX *r_u)
{
	REGISTRY_KEY *parent = find_regkey_index_by_hnd(p, &q_u->handle);
	REGISTRY_KEY *newparentinfo, *keyinfo;
	POLICY_HND newparent_handle;
	REGSUBKEY_CTR *subkeys;
	BOOL write_result;
	pstring name;
	WERROR result;

	if ( !parent )
		return WERR_BADFID;
		
	rpcstr_pull( name, q_u->name.string->buffer, sizeof(name), q_u->name.string->uni_str_len*2, 0 );
	
	/* ok.  Here's what we do.  */

	if ( strrchr( name, '\\' ) ) {
		pstring newkeyname;
		char *ptr;
		
		/* (1) check for enumerate rights on the parent handle.  CLients can try 
		       create things like 'SOFTWARE\Samba' on the HKLM handle. 
		   (2) open the path to the child parent key if necessary */
	
		if ( !(parent->access_granted & SEC_RIGHTS_ENUM_SUBKEYS) )
			return WERR_ACCESS_DENIED;
		
		pstrcpy( newkeyname, name );
		ptr = strrchr( newkeyname, '\\' );
		*ptr = '\0';

		result = open_registry_key( p, &newparent_handle, &newparentinfo, 
			parent, newkeyname, (REG_KEY_READ|REG_KEY_WRITE) );
			
		if ( !W_ERROR_IS_OK(result) )
			return result;

		/* copy the new key name (just the lower most keyname) */

		pstrcpy( name, ptr+1 );
	}
	else {
		/* use the existing open key information */
		newparentinfo = parent;
		memcpy( &newparent_handle, &q_u->handle, sizeof(POLICY_HND) );
	}
	
	/* (3) check for create subkey rights on the correct parent */
	
	if ( !(newparentinfo->access_granted & SEC_RIGHTS_CREATE_SUBKEY) ) {
		result = WERR_ACCESS_DENIED;
		goto done;
	}	
		
	if ( !(subkeys = TALLOC_ZERO_P( p->mem_ctx, REGSUBKEY_CTR )) ) {
		result = WERR_NOMEM;
		goto done;
	}

	/* (4) lookup the current keys and add the new one */
	
	fetch_reg_keys( newparentinfo, subkeys );
	regsubkey_ctr_addkey( subkeys, name );
	
	/* now write to the registry backend */
	
	write_result = store_reg_keys( newparentinfo, subkeys );
	
	TALLOC_FREE( subkeys );

	if ( !write_result )
		return WERR_REG_IO_FAILURE;
		
	/* (5) open the new key and return the handle.  Note that it is probably 
	   not correct to grant full access on this open handle. */
	
	result = open_registry_key( p, &r_u->handle, &keyinfo, newparentinfo, name, REG_KEY_READ );
	keyinfo->access_granted = REG_KEY_ALL;

done:
	/* close any intermediate key handles */
	
	if ( newparentinfo != parent )
		close_registry_key( p, &newparent_handle );
		
	return result;
}


/*******************************************************************
 ********************************************************************/

WERROR _reg_set_value(pipes_struct *p, REG_Q_SET_VALUE  *q_u, REG_R_SET_VALUE *r_u)
{
	REGISTRY_KEY *key = find_regkey_index_by_hnd(p, &q_u->handle);
	REGVAL_CTR *values;
	BOOL write_result;
	fstring valuename;

	if ( !key )
		return WERR_BADFID;
		
	/* access checks first */
	
	if ( !(key->access_granted & SEC_RIGHTS_SET_VALUE) )
		return WERR_ACCESS_DENIED;
		
	rpcstr_pull( valuename, q_u->name.string->buffer, sizeof(valuename), q_u->name.string->uni_str_len*2, 0 );

	/* verify the name */

	if ( !*valuename )
		return WERR_INVALID_PARAM;

	DEBUG(8,("_reg_set_value: Setting value for [%s:%s]\n", key->name, valuename));
		
	if ( !(values = TALLOC_ZERO_P( p->mem_ctx, REGVAL_CTR )) )
		return WERR_NOMEM; 
	
	/* lookup the current values and add the new one */
	
	fetch_reg_values( key, values );
	
	regval_ctr_addvalue( values, valuename, q_u->type, (char*)q_u->value.buffer, q_u->value.buf_len );
	
	/* now write to the registry backend */
	
	write_result = store_reg_values( key, values );
	
	TALLOC_FREE( values );
	
	if ( !write_result )
		return WERR_REG_IO_FAILURE;
		
	return WERR_OK;
}

/*******************************************************************
 ********************************************************************/

WERROR _reg_delete_key(pipes_struct *p, REG_Q_DELETE_KEY  *q_u, REG_R_DELETE_KEY *r_u)
{
	REGISTRY_KEY *parent = find_regkey_index_by_hnd(p, &q_u->handle);
	REGISTRY_KEY *newparentinfo = NULL;
	POLICY_HND newparent_handle;
	REGSUBKEY_CTR *subkeys;
	BOOL write_result;
	pstring name;
	WERROR result;

	if ( !parent )
		return WERR_BADFID;

	/* MSDN says parent the handle must have been opened with DELETE access */

	/* (1) check for delete rights on the parent */
	
	if ( !(parent->access_granted & STD_RIGHT_DELETE_ACCESS) ) {
		result = WERR_ACCESS_DENIED;
		goto done;
	}
		
	rpcstr_pull( name, q_u->name.string->buffer, sizeof(name), q_u->name.string->uni_str_len*2, 0 );
		
	/* ok.  Here's what we do.  */

	if ( strrchr( name, '\\' ) ) {
		pstring newkeyname;
		char *ptr;
		
		/* (2) open the path to the child parent key if necessary */
		/* split the registry path and save the subkeyname */
	
		pstrcpy( newkeyname, name );
		ptr = strrchr( newkeyname, '\\' );
		*ptr = '\0';
		pstrcpy( name, ptr+1 );

		result = open_registry_key( p, &newparent_handle, &newparentinfo, parent, newkeyname, (REG_KEY_READ|REG_KEY_WRITE) );
		if ( !W_ERROR_IS_OK(result) )
			return result;
	}
	else {
		/* use the existing open key information */
		newparentinfo = parent;
	}
	
	if ( !(subkeys = TALLOC_ZERO_P( p->mem_ctx, REGSUBKEY_CTR )) ) {
		result = WERR_NOMEM;
		goto done;
	}
	
	/* lookup the current keys and delete the new one */
	
	fetch_reg_keys( newparentinfo, subkeys );
	
	regsubkey_ctr_delkey( subkeys, name );
	
	/* now write to the registry backend */
	
	write_result = store_reg_keys( newparentinfo, subkeys );
	
	TALLOC_FREE( subkeys );

	result = write_result ? WERR_OK : WERR_REG_IO_FAILURE;
	
done:
	/* close any intermediate key handles */
	
	if ( newparentinfo != parent )
		close_registry_key( p, &newparent_handle );

	return result;
}


/*******************************************************************
 ********************************************************************/

WERROR _reg_delete_value(pipes_struct *p, REG_Q_DELETE_VALUE  *q_u, REG_R_DELETE_VALUE *r_u)
{
	REGISTRY_KEY *key = find_regkey_index_by_hnd(p, &q_u->handle);
	REGVAL_CTR *values;
	BOOL write_result;
	fstring valuename;
	
	if ( !key )
		return WERR_BADFID;
		
	/* access checks first */
	
	if ( !(key->access_granted & SEC_RIGHTS_SET_VALUE) )
		return WERR_ACCESS_DENIED;

	rpcstr_pull( valuename, q_u->name.string->buffer, sizeof(valuename), q_u->name.string->uni_str_len*2, 0 );

	if ( !*valuename )
		return WERR_INVALID_PARAM;

	DEBUG(8,("_reg_delete_value: Setting value for [%s:%s]\n", key->name, valuename));

	if ( !(values = TALLOC_ZERO_P( p->mem_ctx, REGVAL_CTR )) )
		return WERR_NOMEM;
	
	/* lookup the current values and add the new one */
	
	fetch_reg_values( key, values );
	
	regval_ctr_delvalue( values, valuename );
	
	/* now write to the registry backend */
	
	write_result = store_reg_values( key, values );
	
	TALLOC_FREE( values );
	
	if ( !write_result )
		return WERR_REG_IO_FAILURE;
		
	return WERR_OK;
}

/*******************************************************************
 ********************************************************************/

WERROR _reg_get_key_sec(pipes_struct *p, REG_Q_GET_KEY_SEC  *q_u, REG_R_GET_KEY_SEC *r_u)
{
	REGISTRY_KEY *key = find_regkey_index_by_hnd(p, &q_u->handle);

	if ( !key )
		return WERR_BADFID;
		
	/* access checks first */
	
	if ( !(key->access_granted & STD_RIGHT_READ_CONTROL_ACCESS) )
		return WERR_ACCESS_DENIED;
		
	return WERR_ACCESS_DENIED;
}

/*******************************************************************
 ********************************************************************/

WERROR _reg_set_key_sec(pipes_struct *p, REG_Q_SET_KEY_SEC  *q_u, REG_R_SET_KEY_SEC *r_u)
{
	REGISTRY_KEY *key = find_regkey_index_by_hnd(p, &q_u->handle);

	if ( !key )
		return WERR_BADFID;
		
	/* access checks first */
	
	if ( !(key->access_granted & STD_RIGHT_WRITE_DAC_ACCESS) )
		return WERR_ACCESS_DENIED;
		
	return WERR_ACCESS_DENIED;
}
