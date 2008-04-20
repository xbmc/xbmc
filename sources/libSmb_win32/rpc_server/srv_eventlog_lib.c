/* 
 *  Unix SMB/CIFS implementation.
 *  Eventlog utility  routines
 *  Copyright (C) Marcin Krzysztof Porwit    2005,
 *  Copyright (C) Brian Moran                2005.
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

/* maintain a list of open eventlog tdbs with reference counts */

static ELOG_TDB *open_elog_list;

/********************************************************************
 Init an Eventlog TDB, and return it. If null, something bad 
 happened.
********************************************************************/

TDB_CONTEXT *elog_init_tdb( char *tdbfilename )
{
	TDB_CONTEXT *tdb;

	DEBUG(10,("elog_init_tdb: Initializing eventlog tdb (%s)\n",
		tdbfilename));

	tdb = tdb_open_log( tdbfilename, 0, TDB_DEFAULT, 
		O_RDWR|O_CREAT|O_TRUNC, 0660 );

	if ( !tdb ) {
		DEBUG( 0, ( "Can't open tdb for [%s]\n", tdbfilename ) );
		return NULL;
	}

	/* initialize with defaults, copy real values in here from registry */

	tdb_store_int32( tdb, EVT_OLDEST_ENTRY, 1 );
	tdb_store_int32( tdb, EVT_NEXT_RECORD, 1 );
	tdb_store_int32( tdb, EVT_MAXSIZE, 0x80000 );
	tdb_store_int32( tdb, EVT_RETENTION, 0x93A80 );

	tdb_store_int32( tdb, EVT_VERSION, EVENTLOG_DATABASE_VERSION_V1 );

	return tdb;
}

/********************************************************************
 make the tdb file name for an event log, given destination buffer 
 and size. Caller must free memory.
********************************************************************/

char *elog_tdbname( const char *name )
{
	fstring path;
	char *tdb_fullpath;
	char *eventlogdir = lock_path( "eventlog" );
	
	pstr_sprintf( path, "%s/%s.tdb", eventlogdir, name );
	strlower_m( path );
	tdb_fullpath = SMB_STRDUP( path );
	
	return tdb_fullpath;
}


/********************************************************************
 this function is used to count up the number of bytes in a 
 particular TDB
********************************************************************/

struct trav_size_struct {
	int size;
	int rec_count;
};

static int eventlog_tdb_size_fn( TDB_CONTEXT * tdb, TDB_DATA key, TDB_DATA data,
			  void *state )
{
	struct trav_size_struct	 *tsize = state;
	
	tsize->size += data.dsize;
	tsize->rec_count++;
	
	return 0;
}

/********************************************************************
 returns the size of the eventlog, and if MaxSize is a non-null 
 ptr, puts the MaxSize there. This is purely a way not to have yet 
 another function that solely reads the maxsize of the eventlog. 
 Yeah, that's it.
********************************************************************/

int elog_tdb_size( TDB_CONTEXT * tdb, int *MaxSize, int *Retention )
{
	struct trav_size_struct tsize;
	
	if ( !tdb )
		return 0;
		
	ZERO_STRUCT( tsize );

	tdb_traverse( tdb, eventlog_tdb_size_fn, &tsize );

	if ( MaxSize != NULL ) {
		*MaxSize = tdb_fetch_int32( tdb, EVT_MAXSIZE );
	}

	if ( Retention != NULL ) {
		*Retention = tdb_fetch_int32( tdb, EVT_RETENTION );
	}

	DEBUG( 1,
	       ( "eventlog size: [%d] for [%d] records\n", tsize.size,
		 tsize.rec_count ) );
	return tsize.size;
}

/********************************************************************
 Discard early event logs until we have enough for 'needed' bytes...
 NO checking done beforehand to see that we actually need to do 
 this, and it's going to pluck records one-by-one. So, it's best 
 to determine that this needs to be done before doing it.  

 Setting whack_by_date to True indicates that eventlogs falling 
 outside of the retention range need to go...
 
 return True if we made enough room to accommodate needed bytes
********************************************************************/

BOOL make_way_for_eventlogs( TDB_CONTEXT * the_tdb, int32 needed,
			     BOOL whack_by_date )
{
	int start_record, i, new_start;
	int end_record;
	int nbytes, reclen, len, Retention, MaxSize;
	int tresv1, trecnum, timegen, timewr;
	TDB_DATA key, ret;
	TALLOC_CTX *mem_ctx = NULL;
	time_t current_time, exp_time;

	/* discard some eventlogs */

	/* read eventlogs from oldest_entry -- there can't be any discontinuity in recnos,
	   although records not necessarily guaranteed to have successive times */
	/* */
	mem_ctx = talloc_init( "make_way_for_eventlogs" );	/* Homage to BPG */

	if ( mem_ctx == NULL )
		return False;	/* can't allocate memory indicates bigger problems */
	/* lock */
	tdb_lock_bystring_with_timeout( the_tdb, EVT_NEXT_RECORD, 1 );
	/* read */
	end_record = tdb_fetch_int32( the_tdb, EVT_NEXT_RECORD );
	start_record = tdb_fetch_int32( the_tdb, EVT_OLDEST_ENTRY );
	Retention = tdb_fetch_int32( the_tdb, EVT_RETENTION );
	MaxSize = tdb_fetch_int32( the_tdb, EVT_MAXSIZE );

	time( &current_time );

	/* calculate ... */
	exp_time = current_time - Retention;	/* discard older than exp_time */

	/* todo - check for sanity in next_record */
	nbytes = 0;

	DEBUG( 3,
	       ( "MaxSize [%d] Retention [%d] Current Time [%d]  exp_time [%d]\n",
		 MaxSize, Retention, (uint32)current_time, (uint32)exp_time ) );
	DEBUG( 3,
	       ( "Start Record [%d] End Record [%d]\n", start_record,
		 end_record ) );

	for ( i = start_record; i < end_record; i++ ) {
		/* read a record, add the amt to nbytes */
		key.dsize = sizeof( int32 );
		key.dptr = ( char * ) ( int32 * ) & i;
		ret = tdb_fetch( the_tdb, key );
		if ( ret.dsize == 0 ) {
			DEBUG( 8,
			       ( "Can't find a record for the key, record [%d]\n",
				 i ) );
			tdb_unlock_bystring( the_tdb, EVT_NEXT_RECORD );
			return False;
		}
		nbytes += ret.dsize;	/* note this includes overhead */

		len = tdb_unpack( ret.dptr, ret.dsize, "ddddd", &reclen,
				  &tresv1, &trecnum, &timegen, &timewr );
		if (len == -1) {
			DEBUG( 10,("make_way_for_eventlogs: tdb_unpack failed.\n"));
			tdb_unlock_bystring( the_tdb, EVT_NEXT_RECORD );
			return False;
		}

		DEBUG( 8,
		       ( "read record %d, record size is [%d], total so far [%d]\n",
			 i, reclen, nbytes ) );

		SAFE_FREE( ret.dptr );

		/* note that other servers may just stop writing records when the size limit
		   is reached, and there are no records older than 'retention'. This doesn't 
		   like a very useful thing to do, so instead we whack (as in sleeps with the 
		   fishes) just enough records to fit the what we need.  This behavior could
		   be changed to 'match', if the need arises. */

		if ( !whack_by_date && ( nbytes >= needed ) )
			break;	/* done */
		if ( whack_by_date && ( timegen >= exp_time ) )
			break;	/* done */
	}

	DEBUG( 3,
	       ( "nbytes [%d] needed [%d] start_record is [%d], should be set to [%d]\n",
		 nbytes, needed, start_record, i ) );
	/* todo - remove eventlog entries here and set starting record to start_record... */
	new_start = i;
	if ( start_record != new_start ) {
		for ( i = start_record; i < new_start; i++ ) {
			key.dsize = sizeof( int32 );
			key.dptr = ( char * ) ( int32 * ) & i;
			tdb_delete( the_tdb, key );
		}

		tdb_store_int32( the_tdb, EVT_OLDEST_ENTRY, new_start );
	}
	tdb_unlock_bystring( the_tdb, EVT_NEXT_RECORD );
	return True;
}

/********************************************************************
  some hygiene for an eventlog - see how big it is, and then 
  calculate how many bytes we need to remove                   
********************************************************************/

BOOL prune_eventlog( TDB_CONTEXT * tdb )
{
	int MaxSize, Retention, CalcdSize;

	if ( !tdb ) {
		DEBUG( 4, ( "No eventlog tdb handle\n" ) );
		return False;
	}

	CalcdSize = elog_tdb_size( tdb, &MaxSize, &Retention );
	DEBUG( 3,
	       ( "Calculated size [%d] MaxSize [%d]\n", CalcdSize,
		 MaxSize ) );

	if ( CalcdSize > MaxSize ) {
		return make_way_for_eventlogs( tdb, CalcdSize - MaxSize,
					       False );
	}

	return make_way_for_eventlogs( tdb, 0, True );
}

/********************************************************************
********************************************************************/

BOOL can_write_to_eventlog( TDB_CONTEXT * tdb, int32 needed )
{
	int calcd_size;
	int MaxSize, Retention;

	/* see if we can write to the eventlog -- do a policy enforcement */
	if ( !tdb )
		return False;	/* tdb is null, so we can't write to it */


	if ( needed < 0 )
		return False;
	MaxSize = 0;
	Retention = 0;

	calcd_size = elog_tdb_size( tdb, &MaxSize, &Retention );

	if ( calcd_size <= MaxSize )
		return True;	/* you betcha */
	if ( calcd_size + needed < MaxSize )
		return True;

	if ( Retention == 0xffffffff ) {
		return False;	/* see msdn - we can't write no room, discard */
	}
	/*
	   note don't have to test, but always good to show intent, in case changes needed
	   later
	 */

	if ( Retention == 0x00000000 ) {
		/* discard record(s) */
		/* todo  - decide when to remove a bunch vs. just what we need... */
		return make_way_for_eventlogs( tdb, calcd_size - MaxSize,
					       True );
	}

	return make_way_for_eventlogs( tdb, calcd_size - MaxSize, False );
}

/*******************************************************************
*******************************************************************/

ELOG_TDB *elog_open_tdb( char *logname, BOOL force_clear )
{
	TDB_CONTEXT *tdb = NULL;
	uint32 vers_id;
	ELOG_TDB *ptr;
	char *tdbfilename;
	pstring tdbpath;
	ELOG_TDB *tdb_node = NULL;
	char *eventlogdir;

	/* first see if we have an open context */
	
	for ( ptr=open_elog_list; ptr; ptr=ptr->next ) {
		if ( strequal( ptr->name, logname ) ) {
			ptr->ref_count++;

			/* trick to alow clearing of the eventlog tdb.
			   The force_clear flag should imply that someone
			   has done a force close.  So make sure the tdb 
			   is NULL.  If this is a normal open, then just 
			   return the existing reference */

			if ( force_clear ) {
				SMB_ASSERT( ptr->tdb == NULL );
				break;
			}
			else
				return ptr;
		}
	}
	
	/* make sure that the eventlog dir exists */
	
	eventlogdir = lock_path( "eventlog" );
	if ( !directory_exist( eventlogdir, NULL ) )
		mkdir( eventlogdir, 0755 );	
	
	/* get the path on disk */
	
	tdbfilename = elog_tdbname( logname );
	pstrcpy( tdbpath, tdbfilename );
	SAFE_FREE( tdbfilename );

	DEBUG(7,("elog_open_tdb: Opening %s...(force_clear == %s)\n", 
		tdbpath, force_clear?"True":"False" ));
		
	/* the tdb wasn't already open or this is a forced clear open */

	if ( !force_clear ) {

		tdb = tdb_open_log( tdbpath, 0, TDB_DEFAULT, O_RDWR , 0 );	
		if ( tdb ) {
			vers_id = tdb_fetch_int32( tdb, EVT_VERSION );

			if ( vers_id != EVENTLOG_DATABASE_VERSION_V1 ) {
				DEBUG(1,("elog_open_tdb: Invalid version [%d] on file [%s].\n",
					vers_id, tdbpath));
				tdb_close( tdb );
				tdb = elog_init_tdb( tdbpath );
			}
		}
	}
	
	if ( !tdb )
		tdb = elog_init_tdb( tdbpath );
	
	/* if we got a valid context, then add it to the list */
	
	if ( tdb ) {
		/* on a forced clear, just reset the tdb context if we already
		   have an open entry in the list */

		if ( ptr ) {
			ptr->tdb = tdb;
			return ptr;
		}

		if ( !(tdb_node = TALLOC_ZERO_P( NULL, ELOG_TDB)) ) {
			DEBUG(0,("elog_open_tdb: talloc() failure!\n"));
			tdb_close( tdb );
			return NULL;
		}
		
		tdb_node->name = talloc_strdup( tdb_node, logname );
		tdb_node->tdb = tdb;
		tdb_node->ref_count = 1;
		
		DLIST_ADD( open_elog_list, tdb_node );
	}

	return tdb_node;
}

/*******************************************************************
 Wrapper to handle reference counts to the tdb
*******************************************************************/

int elog_close_tdb( ELOG_TDB *etdb, BOOL force_close )
{
	TDB_CONTEXT *tdb;

	if ( !etdb )
		return 0;
		
	etdb->ref_count--;
	
	SMB_ASSERT( etdb->ref_count >= 0 );

	if ( etdb->ref_count == 0 ) {
		tdb = etdb->tdb;
		DLIST_REMOVE( open_elog_list, etdb );
		TALLOC_FREE( etdb );
		return tdb_close( tdb );
	}
	
	if ( force_close ) {
		tdb = etdb->tdb;
		etdb->tdb = NULL;
		return tdb_close( tdb );
	}

	return 0;
}


/*******************************************************************
 write an eventlog entry. Note that we have to lock, read next 
 eventlog, increment, write, write the record, unlock 
 
 coming into this, ee has the eventlog record, and the auxilliary date 
 (computer name, etc.) filled into the other structure. Before packing 
 into a record, this routine will calc the appropriate padding, etc., 
 and then blast out the record in a form that can be read back in
*******************************************************************/
 
#define MARGIN 512

int write_eventlog_tdb( TDB_CONTEXT * the_tdb, Eventlog_entry * ee )
{
	int32 next_record;
	uint8 *packed_ee;
	TALLOC_CTX *mem_ctx = NULL;
	TDB_DATA kbuf, ebuf;
	uint32 n_packed;

	if ( !ee )
		return 0;

	mem_ctx = talloc_init( "write_eventlog_tdb" );

	if ( mem_ctx == NULL )
		return 0;

	if ( !ee )
		return 0;
	/* discard any entries that have bogus time, which usually indicates a bogus entry as well. */
	if ( ee->record.time_generated == 0 )
		return 0;

	/* todo - check for sanity in next_record */

	fixup_eventlog_entry( ee );

	if ( !can_write_to_eventlog( the_tdb, ee->record.length ) ) {
		DEBUG( 3, ( "Can't write to Eventlog, no room \n" ) );
		talloc_destroy( mem_ctx );
		return 0;
	}

	/* alloc mem for the packed version */
	packed_ee = TALLOC( mem_ctx, ee->record.length + MARGIN );
	if ( !packed_ee ) {
		talloc_destroy( mem_ctx );
		return 0;
	}

	/* need to read the record number and insert it into the entry here */

	/* lock */
	tdb_lock_bystring_with_timeout( the_tdb, EVT_NEXT_RECORD, 1 );
	/* read */
	next_record = tdb_fetch_int32( the_tdb, EVT_NEXT_RECORD );

	n_packed =
		tdb_pack( (char *)packed_ee, ee->record.length + MARGIN,
			  "ddddddwwwwddddddBBdBBBd", ee->record.length,
			  ee->record.reserved1, next_record,
			  ee->record.time_generated, ee->record.time_written,
			  ee->record.event_id, ee->record.event_type,
			  ee->record.num_strings, ee->record.event_category,
			  ee->record.reserved2,
			  ee->record.closing_record_number,
			  ee->record.string_offset,
			  ee->record.user_sid_length,
			  ee->record.user_sid_offset, ee->record.data_length,
			  ee->record.data_offset,
			  ee->data_record.source_name_len,
			  ee->data_record.source_name,
			  ee->data_record.computer_name_len,
			  ee->data_record.computer_name,
			  ee->data_record.sid_padding,
			  ee->record.user_sid_length, ee->data_record.sid,
			  ee->data_record.strings_len,
			  ee->data_record.strings,
			  ee->data_record.user_data_len,
			  ee->data_record.user_data,
			  ee->data_record.data_padding );

	/*DEBUG(3,("write_eventlog_tdb: packed into  %d\n",n_packed)); */

	/* increment the record count */

	kbuf.dsize = sizeof( int32 );
	kbuf.dptr = (char * ) & next_record;

	ebuf.dsize = n_packed;
	ebuf.dptr = (char *)packed_ee;

	if ( tdb_store( the_tdb, kbuf, ebuf, 0 ) ) {
		/* DEBUG(1,("write_eventlog_tdb: Can't write record %d to eventlog\n",next_record)); */
		tdb_unlock_bystring( the_tdb, EVT_NEXT_RECORD );
		talloc_destroy( mem_ctx );
		return 0;
	}
	next_record++;
	tdb_store_int32( the_tdb, EVT_NEXT_RECORD, next_record );
	tdb_unlock_bystring( the_tdb, EVT_NEXT_RECORD );
	talloc_destroy( mem_ctx );
	return ( next_record - 1 );
}

/*******************************************************************
 calculate the correct fields etc for an eventlog entry
*******************************************************************/

void fixup_eventlog_entry( Eventlog_entry * ee )
{
	/* fix up the eventlog entry structure as necessary */

	ee->data_record.sid_padding =
		( ( 4 -
		    ( ( ee->data_record.source_name_len +
			ee->data_record.computer_name_len ) % 4 ) ) % 4 );
	ee->data_record.data_padding =
		( 4 -
		  ( ( ee->data_record.strings_len +
		      ee->data_record.user_data_len ) % 4 ) ) % 4;
	ee->record.length = sizeof( Eventlog_record );
	ee->record.length += ee->data_record.source_name_len;
	ee->record.length += ee->data_record.computer_name_len;
	if ( ee->record.user_sid_length == 0 ) {
		/* Should not pad to a DWORD boundary for writing out the sid if there is
		   no SID, so just propagate the padding to pad the data */
		ee->data_record.data_padding += ee->data_record.sid_padding;
		ee->data_record.sid_padding = 0;
	}
	/* DEBUG(10, ("sid_padding is [%d].\n", ee->data_record.sid_padding)); */
	/* DEBUG(10, ("data_padding is [%d].\n", ee->data_record.data_padding)); */

	ee->record.length += ee->data_record.sid_padding;
	ee->record.length += ee->record.user_sid_length;
	ee->record.length += ee->data_record.strings_len;
	ee->record.length += ee->data_record.user_data_len;
	ee->record.length += ee->data_record.data_padding;
	/* need another copy of length at the end of the data */
	ee->record.length += sizeof( ee->record.length );
}

/********************************************************************
 Note that it's a pretty good idea to initialize the Eventlog_entry 
 structure to zero's before calling parse_logentry on an batch of 
 lines that may resolve to a record.  ALSO, it's a good idea to 
 remove any linefeeds (that's EOL to you and me) on the lines 
 going in.
********************************************************************/

BOOL parse_logentry( char *line, Eventlog_entry * entry, BOOL * eor )
{
	char *start = NULL, *stop = NULL;
	pstring temp;
	int temp_len = 0;

	start = line;

	/* empty line signyfiying record delimeter, or we're at the end of the buffer */
	if ( start == NULL || strlen( start ) == 0 ) {
		DEBUG( 6,
		       ( "parse_logentry: found end-of-record indicator.\n" ) );
		*eor = True;
		return True;
	}
	if ( !( stop = strchr( line, ':' ) ) ) {
		return False;
	}

	DEBUG( 6, ( "parse_logentry: trying to parse [%s].\n", line ) );

	if ( 0 == strncmp( start, "LEN", stop - start ) ) {
		/* This will get recomputed later anyway -- probably not necessary */
		entry->record.length = atoi( stop + 1 );
	} else if ( 0 == strncmp( start, "RS1", stop - start ) ) {
		/* For now all these reserved entries seem to have the same value,
		   which can be hardcoded to int(1699505740) for now */
		entry->record.reserved1 = atoi( stop + 1 );
	} else if ( 0 == strncmp( start, "RCN", stop - start ) ) {
		entry->record.record_number = atoi( stop + 1 );
	} else if ( 0 == strncmp( start, "TMG", stop - start ) ) {
		entry->record.time_generated = atoi( stop + 1 );
	} else if ( 0 == strncmp( start, "TMW", stop - start ) ) {
		entry->record.time_written = atoi( stop + 1 );
	} else if ( 0 == strncmp( start, "EID", stop - start ) ) {
		entry->record.event_id = atoi( stop + 1 );
	} else if ( 0 == strncmp( start, "ETP", stop - start ) ) {
		if ( strstr( start, "ERROR" ) ) {
			entry->record.event_type = EVENTLOG_ERROR_TYPE;
		} else if ( strstr( start, "WARNING" ) ) {
			entry->record.event_type = EVENTLOG_WARNING_TYPE;
		} else if ( strstr( start, "INFO" ) ) {
			entry->record.event_type = EVENTLOG_INFORMATION_TYPE;
		} else if ( strstr( start, "AUDIT_SUCCESS" ) ) {
			entry->record.event_type = EVENTLOG_AUDIT_SUCCESS;
		} else if ( strstr( start, "AUDIT_FAILURE" ) ) {
			entry->record.event_type = EVENTLOG_AUDIT_FAILURE;
		} else if ( strstr( start, "SUCCESS" ) ) {
			entry->record.event_type = EVENTLOG_SUCCESS;
		} else {
			/* some other eventlog type -- currently not defined in MSDN docs, so error out */
			return False;
		}
	}

/*
  else if(0 == strncmp(start, "NST", stop - start))
  {
  entry->record.num_strings = atoi(stop + 1);
  }
*/
	else if ( 0 == strncmp( start, "ECT", stop - start ) ) {
		entry->record.event_category = atoi( stop + 1 );
	} else if ( 0 == strncmp( start, "RS2", stop - start ) ) {
		entry->record.reserved2 = atoi( stop + 1 );
	} else if ( 0 == strncmp( start, "CRN", stop - start ) ) {
		entry->record.closing_record_number = atoi( stop + 1 );
	} else if ( 0 == strncmp( start, "USL", stop - start ) ) {
		entry->record.user_sid_length = atoi( stop + 1 );
	} else if ( 0 == strncmp( start, "SRC", stop - start ) ) {
		memset( temp, 0, sizeof( temp ) );
		stop++;
		while ( isspace( stop[0] ) ) {
			stop++;
		}
		temp_len = strlen( stop );
		strncpy( temp, stop, temp_len );
		rpcstr_push( ( void * ) ( entry->data_record.source_name ),
			     temp, sizeof( entry->data_record.source_name ),
			     STR_TERMINATE );
		entry->data_record.source_name_len =
			( strlen_w( entry->data_record.source_name ) * 2 ) +
			2;
	} else if ( 0 == strncmp( start, "SRN", stop - start ) ) {
		memset( temp, 0, sizeof( temp ) );
		stop++;
		while ( isspace( stop[0] ) ) {
			stop++;
		}
		temp_len = strlen( stop );
		strncpy( temp, stop, temp_len );
		rpcstr_push( ( void * ) ( entry->data_record.computer_name ),
			     temp, sizeof( entry->data_record.computer_name ),
			     STR_TERMINATE );
		entry->data_record.computer_name_len =
			( strlen_w( entry->data_record.computer_name ) * 2 ) +
			2;
	} else if ( 0 == strncmp( start, "SID", stop - start ) ) {
		memset( temp, 0, sizeof( temp ) );
		stop++;
		while ( isspace( stop[0] ) ) {
			stop++;
		}
		temp_len = strlen( stop );
		strncpy( temp, stop, temp_len );
		rpcstr_push( ( void * ) ( entry->data_record.sid ), temp,
			     sizeof( entry->data_record.sid ),
			     STR_TERMINATE );
		entry->record.user_sid_length =
			( strlen_w( entry->data_record.sid ) * 2 ) + 2;
	} else if ( 0 == strncmp( start, "STR", stop - start ) ) {
		/* skip past initial ":" */
		stop++;
		/* now skip any other leading whitespace */
		while ( isspace( stop[0] ) ) {
			stop++;
		}
		temp_len = strlen( stop );
		memset( temp, 0, sizeof( temp ) );
		strncpy( temp, stop, temp_len );
		rpcstr_push( ( void * ) ( entry->data_record.strings +
					  ( entry->data_record.strings_len / 2 ) ),
			     temp,
			     sizeof( entry->data_record.strings ) -
			     ( entry->data_record.strings_len / 2 ), STR_TERMINATE );
		entry->data_record.strings_len += ( temp_len * 2 ) + 2;
		entry->record.num_strings++;
	} else if ( 0 == strncmp( start, "DAT", stop - start ) ) {
		/* skip past initial ":" */
		stop++;
		/* now skip any other leading whitespace */
		while ( isspace( stop[0] ) ) {
			stop++;
		}
		entry->data_record.user_data_len = strlen( stop );
		memset( entry->data_record.user_data, 0,
			sizeof( entry->data_record.user_data ) );
		if ( entry->data_record.user_data_len > 0 ) {
			/* copy no more than the first 1024 bytes */
			if ( entry->data_record.user_data_len >
			     sizeof( entry->data_record.user_data ) )
				entry->data_record.user_data_len =
					sizeof( entry->data_record.
						user_data );
			memcpy( entry->data_record.user_data, stop,
				entry->data_record.user_data_len );
		}
	} else {
		/* some other eventlog entry -- not implemented, so dropping on the floor */
		DEBUG( 10, ( "Unknown entry [%s]. Ignoring.\n", line ) );
		/* For now return true so that we can keep on parsing this mess. Eventually
		   we will return False here. */
		return True;
	}
	return True;
}
