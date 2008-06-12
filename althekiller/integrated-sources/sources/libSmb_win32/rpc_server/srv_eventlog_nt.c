/* 
 *  Unix SMB/CIFS implementation.
 *  RPC Pipe client / server routines
 *  Copyright (C) Marcin Krzysztof Porwit    2005,
 *  Copyright (C) Brian Moran                2005,
 *  Copyright (C) Gerald (Jerry) Carter      2005.
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

#undef  DBGC_CLASS
#define DBGC_CLASS DBGC_RPC_SRV

typedef struct {
	char *logname;
	ELOG_TDB *etdb;
	uint32 current_record;
	uint32 num_records;
	uint32 oldest_entry;
	uint32 flags;
	uint32 access_granted;
} EVENTLOG_INFO;

/********************************************************************
 ********************************************************************/

static void free_eventlog_info( void *ptr )
{
	EVENTLOG_INFO *elog = (EVENTLOG_INFO *)ptr;
	
	if ( elog->etdb )
		elog_close_tdb( elog->etdb, False );
	
	TALLOC_FREE( elog );
}

/********************************************************************
 ********************************************************************/

static EVENTLOG_INFO *find_eventlog_info_by_hnd( pipes_struct * p,
						POLICY_HND * handle )
{
	EVENTLOG_INFO *info;

	if ( !find_policy_by_hnd( p, handle, (void **)(void *)&info ) ) {
		DEBUG( 2,
		       ( "find_eventlog_info_by_hnd: eventlog not found.\n" ) );
		return NULL;
	}

	return info;
}

/********************************************************************
********************************************************************/

static BOOL elog_check_access( EVENTLOG_INFO *info, NT_USER_TOKEN *token )
{
	char *tdbname = elog_tdbname( info->logname );
	SEC_DESC *sec_desc;
	BOOL ret;
	NTSTATUS ntstatus;
	
	if ( !tdbname ) 
		return False;
	
	/* get the security descriptor for the file */
	
	sec_desc = get_nt_acl_no_snum( info, tdbname );
	SAFE_FREE( tdbname );
	
	if ( !sec_desc ) {
		DEBUG(5,("elog_check_access: Unable to get NT ACL for %s\n", 
			tdbname));
		return False;
	}
	
	/* root free pass */

	if ( geteuid() == sec_initial_uid() ) {
		DEBUG(5,("elog_check_access: using root's token\n"));
		token = get_root_nt_token();
	}

	/* run the check, try for the max allowed */
	
	ret = se_access_check( sec_desc, token, MAXIMUM_ALLOWED_ACCESS,
		&info->access_granted, &ntstatus );
		
	if ( sec_desc )
		TALLOC_FREE( sec_desc );
		
	if ( !ret ) {
		DEBUG(8,("elog_check_access: se_access_check() return %s\n",
			nt_errstr( ntstatus)));
		return False;
	}
	
	/* we have to have READ permission for a successful open */
	
	return ( info->access_granted & SA_RIGHT_FILE_READ_DATA );
}

/********************************************************************
 ********************************************************************/

static BOOL elog_validate_logname( const char *name )
{
	int i;
	const char **elogs = lp_eventlog_list();
	
	for ( i=0; elogs[i]; i++ ) {
		if ( strequal( name, elogs[i] ) )
			return True;
	}
	
	return False;
}

/********************************************************************
********************************************************************/

static BOOL get_num_records_hook( EVENTLOG_INFO * info )
{
	int next_record;
	int oldest_record;

	if ( !info->etdb ) {
		DEBUG( 10, ( "No open tdb for %s\n", info->logname ) );
		return False;
	}

	/* lock the tdb since we have to get 2 records */

	tdb_lock_bystring_with_timeout( ELOG_TDB_CTX(info->etdb), EVT_NEXT_RECORD, 1 );
	next_record = tdb_fetch_int32( ELOG_TDB_CTX(info->etdb), EVT_NEXT_RECORD);
	oldest_record = tdb_fetch_int32( ELOG_TDB_CTX(info->etdb), EVT_OLDEST_ENTRY);
	tdb_unlock_bystring( ELOG_TDB_CTX(info->etdb), EVT_NEXT_RECORD);

	DEBUG( 8,
	       ( "Oldest Record %d; Next Record %d\n", oldest_record,
		 next_record ) );

	info->num_records = ( next_record - oldest_record );
	info->oldest_entry = oldest_record;

	return True;
}

/********************************************************************
 ********************************************************************/

static BOOL get_oldest_entry_hook( EVENTLOG_INFO * info )
{
	/* it's the same thing */
	return get_num_records_hook( info );
}

/********************************************************************
 ********************************************************************/

static NTSTATUS elog_open( pipes_struct * p, const char *logname, POLICY_HND *hnd )
{
	EVENTLOG_INFO *elog;
	
	/* first thing is to validate the eventlog name */
	
	if ( !elog_validate_logname( logname ) )
		return NT_STATUS_OBJECT_PATH_INVALID;
	
	if ( !(elog = TALLOC_ZERO_P( NULL, EVENTLOG_INFO )) )
		return NT_STATUS_NO_MEMORY;
		
	elog->logname = talloc_strdup( elog, logname );
	
	/* Open the tdb first (so that we can create any new tdbs if necessary).
	   We have to do this as root and then use an internal access check 
	   on the file permissions since you can only have a tdb open once
	   in a single process */

	become_root();
	elog->etdb = elog_open_tdb( elog->logname, False );
	unbecome_root();

	if ( !elog->etdb ) {
		/* according to MSDN, if the logfile cannot be found, we should
		  default to the "Application" log */
	
		if ( !strequal( logname, ELOG_APPL ) ) {
		
			TALLOC_FREE( elog->logname );
			
			elog->logname = talloc_strdup( elog, ELOG_APPL );			

			/* do the access check */
			if ( !elog_check_access( elog, p->pipe_user.nt_user_token ) ) {
				TALLOC_FREE( elog );
				return NT_STATUS_ACCESS_DENIED;
			}
	
			become_root();
			elog->etdb = elog_open_tdb( elog->logname, False );
			unbecome_root();
		}	
		
		if ( !elog->etdb ) {
			TALLOC_FREE( elog );
			return NT_STATUS_ACCESS_DENIED;	/* ??? */		
		}
	}
	
	/* now do the access check.  Close the tdb if we fail here */

	if ( !elog_check_access( elog, p->pipe_user.nt_user_token ) ) {
		elog_close_tdb( elog->etdb, False );
		TALLOC_FREE( elog );
		return NT_STATUS_ACCESS_DENIED;
	}
	
	/* create the policy handle */
	
	if ( !create_policy_hnd
	     ( p, hnd, free_eventlog_info, ( void * ) elog ) ) {
		free_eventlog_info( elog );
		return NT_STATUS_NO_MEMORY;
	}

	/* set the initial current_record pointer */

	if ( !get_oldest_entry_hook( elog ) ) {
		DEBUG(3,("elog_open: Successfully opened eventlog but can't "
			"get any information on internal records!\n"));
	}	

	elog->current_record = elog->oldest_entry;

	return NT_STATUS_OK;
}

/********************************************************************
 ********************************************************************/

static NTSTATUS elog_close( pipes_struct *p, POLICY_HND *hnd )
{
        if ( !( close_policy_hnd( p, hnd ) ) ) {
                return NT_STATUS_INVALID_HANDLE;
        }

	return NT_STATUS_OK;
}

/*******************************************************************
 *******************************************************************/

static int elog_size( EVENTLOG_INFO *info )
{
	if ( !info || !info->etdb ) {
		DEBUG(0,("elog_size: Invalid info* structure!\n"));
		return 0;
	}

	return elog_tdb_size( ELOG_TDB_CTX(info->etdb), NULL, NULL );
}

/********************************************************************
  For the given tdb, get the next eventlog record into the passed 
  Eventlog_entry.  returns NULL if it can't get the record for some reason.
 ********************************************************************/

Eventlog_entry *get_eventlog_record( prs_struct * ps, TDB_CONTEXT * tdb,
				     int recno, Eventlog_entry * ee )
{
	TDB_DATA ret, key;

	int srecno;
	int reclen;
	int len;

	pstring *wpsource, *wpcomputer, *wpsid, *wpstrs, *puserdata;

	key.dsize = sizeof( int32 );

	srecno = recno;
	key.dptr = ( char * ) &srecno;

	ret = tdb_fetch( tdb, key );

	if ( ret.dsize == 0 ) {
		DEBUG( 8,
		       ( "Can't find a record for the key, record %d\n",
			 recno ) );
		return NULL;
	}

	len = tdb_unpack( ret.dptr, ret.dsize, "d", &reclen );

	DEBUG( 10, ( "Unpacking record %d, size is %d\n", srecno, len ) );

	if ( !len )
		return NULL;

	/* ee = PRS_ALLOC_MEM(ps, Eventlog_entry, 1); */

	if ( !ee )
		return NULL;

	len = tdb_unpack( ret.dptr, ret.dsize, "ddddddwwwwddddddBBdBBBd",
			  &ee->record.length, &ee->record.reserved1,
			  &ee->record.record_number,
			  &ee->record.time_generated,
			  &ee->record.time_written, &ee->record.event_id,
			  &ee->record.event_type, &ee->record.num_strings,
			  &ee->record.event_category, &ee->record.reserved2,
			  &ee->record.closing_record_number,
			  &ee->record.string_offset,
			  &ee->record.user_sid_length,
			  &ee->record.user_sid_offset,
			  &ee->record.data_length, &ee->record.data_offset,
			  &ee->data_record.source_name_len, &wpsource,
			  &ee->data_record.computer_name_len, &wpcomputer,
			  &ee->data_record.sid_padding,
			  &ee->record.user_sid_length, &wpsid,
			  &ee->data_record.strings_len, &wpstrs,
			  &ee->data_record.user_data_len, &puserdata,
			  &ee->data_record.data_padding );
	DEBUG( 10,
	       ( "Read record %d, len in tdb was %d\n",
		 ee->record.record_number, len ) );

	/* have to do the following because the tdb_unpack allocs a buff, stuffs a pointer to the buff
	   into it's 2nd argment for 'B' */

	if ( wpcomputer )
		memcpy( ee->data_record.computer_name, wpcomputer,
			ee->data_record.computer_name_len );
	if ( wpsource )
		memcpy( ee->data_record.source_name, wpsource,
			ee->data_record.source_name_len );

	if ( wpsid )
		memcpy( ee->data_record.sid, wpsid,
			ee->record.user_sid_length );
	if ( wpstrs )
		memcpy( ee->data_record.strings, wpstrs,
			ee->data_record.strings_len );

	/* note that userdata is a pstring */
	if ( puserdata )
		memcpy( ee->data_record.user_data, puserdata,
			ee->data_record.user_data_len );

	SAFE_FREE( wpcomputer );
	SAFE_FREE( wpsource );
	SAFE_FREE( wpsid );
	SAFE_FREE( wpstrs );
	SAFE_FREE( puserdata );

	DEBUG( 10, ( "get_eventlog_record: read back %d\n", len ) );
	DEBUG( 10,
	       ( "get_eventlog_record: computer_name %d is ",
		 ee->data_record.computer_name_len ) );
	SAFE_FREE( ret.dptr );
	return ee;
}

/********************************************************************
 note that this can only be called AFTER the table is constructed, 
 since it uses the table to find the tdb handle
 ********************************************************************/

static BOOL sync_eventlog_params( EVENTLOG_INFO *info )
{
	pstring path;
	uint32 uiMaxSize;
	uint32 uiRetention;
	REGISTRY_KEY *keyinfo;
	REGISTRY_VALUE *val;
	REGVAL_CTR *values;
	WERROR wresult;
	char *elogname = info->logname;

	DEBUG( 4, ( "sync_eventlog_params with %s\n", elogname ) );

	if ( !info->etdb ) {
		DEBUG( 4, ( "No open tdb! (%s)\n", info->logname ) );
		return False;
	}
	/* set resonable defaults.  512Kb on size and 1 week on time */

	uiMaxSize = 0x80000;
	uiRetention = 604800;

	/* the general idea is to internally open the registry 
	   key and retreive the values.  That way we can continue 
	   to use the same fetch/store api that we use in 
	   srv_reg_nt.c */

	pstr_sprintf( path, "%s/%s", KEY_EVENTLOG, elogname );

	wresult =
		regkey_open_internal( &keyinfo, path, get_root_nt_token(  ),
				      REG_KEY_READ );

	if ( !W_ERROR_IS_OK( wresult ) ) {
		DEBUG( 4,
		       ( "sync_eventlog_params: Failed to open key [%s] (%s)\n",
			 path, dos_errstr( wresult ) ) );
		return False;
	}

	if ( !( values = TALLOC_ZERO_P( keyinfo, REGVAL_CTR ) ) ) {
		TALLOC_FREE( keyinfo );
		DEBUG( 0, ( "control_eventlog_hook: talloc() failed!\n" ) );

		return False;
	}
	fetch_reg_values( keyinfo, values );

	if ( ( val = regval_ctr_getvalue( values, "Retention" ) ) != NULL )
		uiRetention = IVAL( regval_data_p( val ), 0 );

	if ( ( val = regval_ctr_getvalue( values, "MaxSize" ) ) != NULL )
		uiMaxSize = IVAL( regval_data_p( val ), 0 );

	regkey_close_internal( keyinfo );

	tdb_store_int32( ELOG_TDB_CTX(info->etdb), EVT_MAXSIZE, uiMaxSize );
	tdb_store_int32( ELOG_TDB_CTX(info->etdb), EVT_RETENTION, uiRetention );

	return True;
}

/********************************************************************
 ********************************************************************/

static Eventlog_entry *read_package_entry( prs_struct * ps,
					   EVENTLOG_Q_READ_EVENTLOG * q_u,
					   EVENTLOG_R_READ_EVENTLOG * r_u,
					   Eventlog_entry * entry )
{
	uint8 *offset;
	Eventlog_entry *ee_new = NULL;

	ee_new = PRS_ALLOC_MEM( ps, Eventlog_entry, 1 );
	if ( ee_new == NULL ) {
		return NULL;
	}

	entry->data_record.sid_padding =
		( ( 4 -
		    ( ( entry->data_record.source_name_len +
			entry->data_record.computer_name_len ) % 4 ) ) % 4 );
	entry->data_record.data_padding =
		( 4 -
		  ( ( entry->data_record.strings_len +
		      entry->data_record.user_data_len ) % 4 ) ) % 4;
	entry->record.length = sizeof( Eventlog_record );
	entry->record.length += entry->data_record.source_name_len;
	entry->record.length += entry->data_record.computer_name_len;
	if ( entry->record.user_sid_length == 0 ) {
		/* Should not pad to a DWORD boundary for writing out the sid if there is
		   no SID, so just propagate the padding to pad the data */
		entry->data_record.data_padding +=
			entry->data_record.sid_padding;
		entry->data_record.sid_padding = 0;
	}
	DEBUG( 10,
	       ( "sid_padding is [%d].\n", entry->data_record.sid_padding ) );
	DEBUG( 10,
	       ( "data_padding is [%d].\n",
		 entry->data_record.data_padding ) );

	entry->record.length += entry->data_record.sid_padding;
	entry->record.length += entry->record.user_sid_length;
	entry->record.length += entry->data_record.strings_len;
	entry->record.length += entry->data_record.user_data_len;
	entry->record.length += entry->data_record.data_padding;
	/* need another copy of length at the end of the data */
	entry->record.length += sizeof( entry->record.length );
	DEBUG( 10,
	       ( "entry->record.length is [%d].\n", entry->record.length ) );
	entry->data =
		PRS_ALLOC_MEM( ps, uint8,
			       entry->record.length -
			       sizeof( Eventlog_record ) -
			       sizeof( entry->record.length ) );
	if ( entry->data == NULL ) {
		return NULL;
	}
	offset = entry->data;
	memcpy( offset, &( entry->data_record.source_name ),
		entry->data_record.source_name_len );
	offset += entry->data_record.source_name_len;
	memcpy( offset, &( entry->data_record.computer_name ),
		entry->data_record.computer_name_len );
	offset += entry->data_record.computer_name_len;
	/* SID needs to be DWORD-aligned */
	offset += entry->data_record.sid_padding;
	entry->record.user_sid_offset =
		sizeof( Eventlog_record ) + ( offset - entry->data );
	memcpy( offset, &( entry->data_record.sid ),
		entry->record.user_sid_length );
	offset += entry->record.user_sid_length;
	/* Now do the strings */
	entry->record.string_offset =
		sizeof( Eventlog_record ) + ( offset - entry->data );
	memcpy( offset, &( entry->data_record.strings ),
		entry->data_record.strings_len );
	offset += entry->data_record.strings_len;
	/* Now do the data */
	entry->record.data_length = entry->data_record.user_data_len;
	entry->record.data_offset =
		sizeof( Eventlog_record ) + ( offset - entry->data );
	memcpy( offset, &( entry->data_record.user_data ),
		entry->data_record.user_data_len );
	offset += entry->data_record.user_data_len;

	memcpy( &( ee_new->record ), &entry->record,
		sizeof( Eventlog_record ) );
	memcpy( &( ee_new->data_record ), &entry->data_record,
		sizeof( Eventlog_data_record ) );
	ee_new->data = entry->data;

	return ee_new;
}

/********************************************************************
 ********************************************************************/

static BOOL add_record_to_resp( EVENTLOG_R_READ_EVENTLOG * r_u,
				Eventlog_entry * ee_new )
{
	Eventlog_entry *insert_point;

	insert_point = r_u->entry;

	if ( NULL == insert_point ) {
		r_u->entry = ee_new;
		ee_new->next = NULL;
	} else {
		while ( ( NULL != insert_point->next ) ) {
			insert_point = insert_point->next;
		}
		ee_new->next = NULL;
		insert_point->next = ee_new;
	}
	r_u->num_records++;
	r_u->num_bytes_in_resp += ee_new->record.length;

	return True;
}

/********************************************************************
 ********************************************************************/

NTSTATUS _eventlog_open_eventlog( pipes_struct * p,
				EVENTLOG_Q_OPEN_EVENTLOG * q_u,
				EVENTLOG_R_OPEN_EVENTLOG * r_u )
{
	fstring servername, logname;
	EVENTLOG_INFO *info;
	NTSTATUS result;

	fstrcpy( servername, "" );
	if ( q_u->servername.string ) {
		rpcstr_pull( servername, q_u->servername.string->buffer,
			     sizeof( servername ),
			     q_u->servername.string->uni_str_len * 2, 0 );
	}

	fstrcpy( logname, "" );
	if ( q_u->logname.string ) {
		rpcstr_pull( logname, q_u->logname.string->buffer,
			     sizeof( logname ),
			     q_u->logname.string->uni_str_len * 2, 0 );
	}
	
	DEBUG( 10,("_eventlog_open_eventlog: Server [%s], Log [%s]\n",
		servername, logname ));
		
	/* according to MSDN, if the logfile cannot be found, we should
	  default to the "Application" log */
	  
	if ( !NT_STATUS_IS_OK( result = elog_open( p, logname, &r_u->handle )) )
		return result;

	if ( !(info = find_eventlog_info_by_hnd( p, &r_u->handle )) ) {
		DEBUG(0,("_eventlog_open_eventlog: eventlog (%s) opened but unable to find handle!\n",
			logname ));
		elog_close( p, &r_u->handle );
		return NT_STATUS_INVALID_HANDLE;
	}

	DEBUG(10,("_eventlog_open_eventlog: Size [%d]\n", elog_size( info )));

	sync_eventlog_params( info );
	prune_eventlog( ELOG_TDB_CTX(info->etdb) );

	return NT_STATUS_OK;
}

/********************************************************************
 This call still needs some work
 ********************************************************************/

NTSTATUS _eventlog_clear_eventlog( pipes_struct * p,
				 EVENTLOG_Q_CLEAR_EVENTLOG * q_u,
				 EVENTLOG_R_CLEAR_EVENTLOG * r_u )
{
	EVENTLOG_INFO *info = find_eventlog_info_by_hnd( p, &q_u->handle );
	pstring backup_file_name;

	if ( !info )
		return NT_STATUS_INVALID_HANDLE;

	pstrcpy( backup_file_name, "" );
	if ( q_u->backupfile.string ) {
		rpcstr_pull( backup_file_name, q_u->backupfile.string->buffer,
			     sizeof( backup_file_name ),
			     q_u->backupfile.string->uni_str_len * 2, 0 );

		DEBUG(8,( "_eventlog_clear_eventlog: Using [%s] as the backup "
			"file name for log [%s].",
			 backup_file_name, info->logname ) );
	}

	/* check for WRITE access to the file */

	if ( !(info->access_granted&SA_RIGHT_FILE_WRITE_DATA) )
		return NT_STATUS_ACCESS_DENIED;

	/* Force a close and reopen */

	elog_close_tdb( info->etdb, True ); 
	become_root();
	info->etdb = elog_open_tdb( info->logname, True );
	unbecome_root();

	if ( !info->etdb )
		return NT_STATUS_ACCESS_DENIED;

	return NT_STATUS_OK;
}

/********************************************************************
 ********************************************************************/

NTSTATUS _eventlog_close_eventlog( pipes_struct * p,
				 EVENTLOG_Q_CLOSE_EVENTLOG * q_u,
				 EVENTLOG_R_CLOSE_EVENTLOG * r_u )
{
	return elog_close( p, &q_u->handle );
}

/********************************************************************
 ********************************************************************/

NTSTATUS _eventlog_read_eventlog( pipes_struct * p,
				EVENTLOG_Q_READ_EVENTLOG * q_u,
				EVENTLOG_R_READ_EVENTLOG * r_u )
{
	EVENTLOG_INFO *info = find_eventlog_info_by_hnd( p, &q_u->handle );
	Eventlog_entry entry, *ee_new;
	uint32 num_records_read = 0;
	prs_struct *ps;
	int bytes_left, record_number;
	uint32 elog_read_type, elog_read_dir;

	if (info == NULL) {
		return NT_STATUS_INVALID_HANDLE;
	}

	info->flags = q_u->flags;
	ps = &p->out_data.rdata;

	bytes_left = q_u->max_read_size;

	if ( !info->etdb ) 
		return NT_STATUS_ACCESS_DENIED;
		
	/* check for valid flags.  Can't use the sequential and seek flags together */

	elog_read_type = q_u->flags & (EVENTLOG_SEQUENTIAL_READ|EVENTLOG_SEEK_READ);
	elog_read_dir = q_u->flags & (EVENTLOG_FORWARDS_READ|EVENTLOG_BACKWARDS_READ);

	if ( elog_read_type == (EVENTLOG_SEQUENTIAL_READ|EVENTLOG_SEEK_READ) 
		||  elog_read_dir == (EVENTLOG_FORWARDS_READ|EVENTLOG_BACKWARDS_READ) )
	{
		DEBUG(3,("_eventlog_read_eventlog: Invalid flags [0x%x] for ReadEventLog\n", q_u->flags));
		return NT_STATUS_INVALID_PARAMETER;
	}

	/* a sequential read should ignore the offset */

	if ( elog_read_type & EVENTLOG_SEQUENTIAL_READ )
		record_number = info->current_record;
	else 
		record_number = q_u->offset;

	while ( bytes_left > 0 ) {

		/* assume that when the record fetch fails, that we are done */

		if ( !get_eventlog_record ( ps, ELOG_TDB_CTX(info->etdb), record_number, &entry ) ) 
			break;

		DEBUG( 8, ( "Retrieved record %d\n", record_number ) );
			       
		/* Now see if there is enough room to add */

		if ( !(ee_new = read_package_entry( ps, q_u, r_u,&entry )) )
			return NT_STATUS_NO_MEMORY;

		if ( r_u->num_bytes_in_resp + ee_new->record.length > q_u->max_read_size ) {
			r_u->bytes_in_next_record = ee_new->record.length;

			/* response would be too big to fit in client-size buffer */
				
			bytes_left = 0;
			break;
		}
			
		add_record_to_resp( r_u, ee_new );
		bytes_left -= ee_new->record.length;
		ZERO_STRUCT( entry );
		num_records_read = r_u->num_records - num_records_read;
				
		DEBUG( 10, ( "_eventlog_read_eventlog: read [%d] records for a total "
			"of [%d] records using [%d] bytes out of a max of [%d].\n",
			 num_records_read, r_u->num_records,
			 r_u->num_bytes_in_resp,
			 q_u->max_read_size ) );

		if ( info->flags & EVENTLOG_FORWARDS_READ )
			record_number++;
		else
			record_number--;
		
		/* update the eventlog record pointer */
		
		info->current_record = record_number;
	}

	/* crazy by WinXP uses NT_STATUS_BUFFER_TOO_SMALL to 
	   say when there are no more records */

	return (num_records_read ? NT_STATUS_OK : NT_STATUS_BUFFER_TOO_SMALL);
}

/********************************************************************
 ********************************************************************/

NTSTATUS _eventlog_get_oldest_entry( pipes_struct * p,
				   EVENTLOG_Q_GET_OLDEST_ENTRY * q_u,
				   EVENTLOG_R_GET_OLDEST_ENTRY * r_u )
{
	EVENTLOG_INFO *info = find_eventlog_info_by_hnd( p, &q_u->handle );

	if (info == NULL) {
		return NT_STATUS_INVALID_HANDLE;
	}

	if ( !( get_oldest_entry_hook( info ) ) )
		return NT_STATUS_ACCESS_DENIED;

	r_u->oldest_entry = info->oldest_entry;

	return NT_STATUS_OK;
}

/********************************************************************
 ********************************************************************/

NTSTATUS _eventlog_get_num_records( pipes_struct * p,
				  EVENTLOG_Q_GET_NUM_RECORDS * q_u,
				  EVENTLOG_R_GET_NUM_RECORDS * r_u )
{
	EVENTLOG_INFO *info = find_eventlog_info_by_hnd( p, &q_u->handle );

	if (info == NULL) {
		return NT_STATUS_INVALID_HANDLE;
	}

	if ( !( get_num_records_hook( info ) ) )
		return NT_STATUS_ACCESS_DENIED;

	r_u->num_records = info->num_records;

	return NT_STATUS_OK;
}
