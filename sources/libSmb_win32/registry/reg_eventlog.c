
/* 
 *  Unix SMB/CIFS implementation.
 *  Virtual Windows Registry Layer
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


/**********************************************************************
 for an eventlog, add in the default values
*********************************************************************/

BOOL eventlog_init_keys( void )
{
	/* Find all of the eventlogs, add keys for each of them */
	const char **elogs = lp_eventlog_list(  );
	pstring evtlogpath;
	pstring evtfilepath;
	REGSUBKEY_CTR *subkeys;
	REGVAL_CTR *values;
	uint32 uiMaxSize;
	uint32 uiRetention;
	uint32 uiCategoryCount;
	UNISTR2 data;

	while ( elogs && *elogs ) {
		if ( !( subkeys = TALLOC_ZERO_P( NULL, REGSUBKEY_CTR ) ) ) {
			DEBUG( 0, ( "talloc() failure!\n" ) );
			return False;
		}
		regdb_fetch_keys( KEY_EVENTLOG, subkeys );
		regsubkey_ctr_addkey( subkeys, *elogs );
		if ( !regdb_store_keys( KEY_EVENTLOG, subkeys ) )
			return False;
		TALLOC_FREE( subkeys );

		/* add in the key of form KEY_EVENTLOG/Application */
		DEBUG( 5,
		       ( "Adding key of [%s] to path of [%s]\n", *elogs,
			 KEY_EVENTLOG ) );

		slprintf( evtlogpath, sizeof( evtlogpath ) - 1, "%s\\%s",
			  KEY_EVENTLOG, *elogs );
		/* add in the key of form KEY_EVENTLOG/Application/Application */
		DEBUG( 5,
		       ( "Adding key of [%s] to path of [%s]\n", *elogs,
			 evtlogpath ) );
		if ( !( subkeys = TALLOC_ZERO_P( NULL, REGSUBKEY_CTR ) ) ) {
			DEBUG( 0, ( "talloc() failure!\n" ) );
			return False;
		}
		regdb_fetch_keys( evtlogpath, subkeys );
		regsubkey_ctr_addkey( subkeys, *elogs );

		if ( !regdb_store_keys( evtlogpath, subkeys ) )
			return False;
		TALLOC_FREE( subkeys );

		/* now add the values to the KEY_EVENTLOG/Application form key */
		if ( !( values = TALLOC_ZERO_P( NULL, REGVAL_CTR ) ) ) {
			DEBUG( 0, ( "talloc() failure!\n" ) );
			return False;
		}
		DEBUG( 5,
		       ( "Storing values to eventlog path of [%s]\n",
			 evtlogpath ) );
		regdb_fetch_values( evtlogpath, values );


		if ( !regval_ctr_key_exists( values, "MaxSize" ) ) {

			/* assume we have none, add them all */

			/* hard code some initial values */

			/* uiDisplayNameId = 0x00000100; */
			uiMaxSize = 0x00080000;
			uiRetention = 0x93A80;

			regval_ctr_addvalue( values, "MaxSize", REG_DWORD,
					     ( char * ) &uiMaxSize,
					     sizeof( uint32 ) );

			regval_ctr_addvalue( values, "Retention", REG_DWORD,
					     ( char * ) &uiRetention,
					     sizeof( uint32 ) );
			init_unistr2( &data, *elogs, UNI_STR_TERMINATE );

			regval_ctr_addvalue( values, "PrimaryModule", REG_SZ,
					     ( char * ) data.buffer,
					     data.uni_str_len *
					     sizeof( uint16 ) );
			init_unistr2( &data, *elogs, UNI_STR_TERMINATE );

			regval_ctr_addvalue( values, "Sources", REG_MULTI_SZ,
					     ( char * ) data.buffer,
					     data.uni_str_len *
					     sizeof( uint16 ) );

			pstr_sprintf( evtfilepath, "%%SystemRoot%%\\system32\\config\\%s.tdb", *elogs );
			init_unistr2( &data, evtfilepath, UNI_STR_TERMINATE );
			regval_ctr_addvalue( values, "File", REG_EXPAND_SZ, ( char * ) data.buffer,
					     data.uni_str_len * sizeof( uint16 ) );
			regdb_store_values( evtlogpath, values );

		}

		TALLOC_FREE( values );

		/* now do the values under KEY_EVENTLOG/Application/Application */
		slprintf( evtlogpath, sizeof( evtlogpath ) - 1, "%s\\%s\\%s",
			  KEY_EVENTLOG, *elogs, *elogs );
		if ( !( values = TALLOC_ZERO_P( NULL, REGVAL_CTR ) ) ) {
			DEBUG( 0, ( "talloc() failure!\n" ) );
			return False;
		}
		DEBUG( 5,
		       ( "Storing values to eventlog path of [%s]\n",
			 evtlogpath ) );
		regdb_fetch_values( evtlogpath, values );
		if ( !regval_ctr_key_exists( values, "CategoryCount" ) ) {

			/* hard code some initial values */

			uiCategoryCount = 0x00000007;
			regval_ctr_addvalue( values, "CategoryCount",
					     REG_DWORD,
					     ( char * ) &uiCategoryCount,
					     sizeof( uint32 ) );
			init_unistr2( &data,
				      "%SystemRoot%\\system32\\eventlog.dll",
				      UNI_STR_TERMINATE );

			regval_ctr_addvalue( values, "CategoryMessageFile",
					     REG_EXPAND_SZ,
					     ( char * ) data.buffer,
					     data.uni_str_len *
					     sizeof( uint16 ) );
			regdb_store_values( evtlogpath, values );
		}
		TALLOC_FREE( values );
		elogs++;
	}

	return True;

}

/*********************************************************************
 for an eventlog, add in a source name. If the eventlog doesn't 
 exist (not in the list) do nothing.   If a source for the log 
 already exists, change the information (remove, replace)
*********************************************************************/

BOOL eventlog_add_source( const char *eventlog, const char *sourcename,
			  const char *messagefile )
{
	/* Find all of the eventlogs, add keys for each of them */
	/* need to add to the value KEY_EVENTLOG/<eventlog>/Sources string (Creating if necessary)
	   need to add KEY of source to KEY_EVENTLOG/<eventlog>/<source> */

	const char **elogs = lp_eventlog_list(  );
	char **wrklist, **wp;
	pstring evtlogpath;
	REGSUBKEY_CTR *subkeys;
	REGVAL_CTR *values;
	REGISTRY_VALUE *rval;
	UNISTR2 data;
	uint16 *msz_wp;
	int mbytes, ii;
	BOOL already_in;
	int i;
	int numsources;

	for ( i = 0; elogs[i]; i++ ) {
		if ( strequal( elogs[i], eventlog ) )
			break;
	}

	if ( !elogs[i] ) {
		DEBUG( 0,
		       ( "Eventlog [%s] not found in list of valid event logs\n",
			 eventlog ) );
		return False;	/* invalid named passed in */
	}

	/* have to assume that the evenlog key itself exists at this point */
	/* add in a key of [sourcename] under the eventlog key */

	/* todo add to Sources */

	if ( !( values = TALLOC_ZERO_P( NULL, REGVAL_CTR ) ) ) {
		DEBUG( 0, ( "talloc() failure!\n" ) );
		return False;
	}

	pstr_sprintf( evtlogpath, "%s\\%s", KEY_EVENTLOG, eventlog );

	regdb_fetch_values( evtlogpath, values );


	if ( !( rval = regval_ctr_getvalue( values, "Sources" ) ) ) {
		DEBUG( 0, ( "No Sources value for [%s]!\n", eventlog ) );
		return False;
	}
	/* perhaps this adding a new string to a multi_sz should be a fn? */
	/* check to see if it's there already */

	if ( rval->type != REG_MULTI_SZ ) {
		DEBUG( 0,
		       ( "Wrong type for Sources, should be REG_MULTI_SZ\n" ) );
		return False;
	}
	/* convert to a 'regulah' chars to do some comparisons */

	already_in = False;
	wrklist = NULL;
	dump_data( 1, (const char *)rval->data_p, rval->size );
	if ( ( numsources =
	       regval_convert_multi_sz( ( uint16 * ) rval->data_p, rval->size,
					&wrklist ) ) > 0 ) {

		ii = numsources;
		/* see if it's in there already */
		wp = wrklist;

		while ( ii && wp && *wp ) {
			if ( strequal( *wp, sourcename ) ) {
				DEBUG( 5,
				       ( "Source name [%s] already in list for [%s] \n",
					 sourcename, eventlog ) );
				already_in = True;
				break;
			}
			wp++;
			ii--;
		}
	} else {
		if ( numsources < 0 ) {
			DEBUG( 3, ( "problem in getting the sources\n" ) );
			return False;
		}
		DEBUG( 3,
		       ( "Nothing in the sources list, this might be a problem\n" ) );
	}

	wp = wrklist;

	if ( !already_in ) {
		/* make a new list with an additional entry; copy values, add another */
		wp = TALLOC_ARRAY( NULL, char *, numsources + 2 );

		if ( !wp ) {
			DEBUG( 0, ( "talloc() failed \n" ) );
			return False;
		}
		memcpy( wp, wrklist, sizeof( char * ) * numsources );
		*( wp + numsources ) = ( char * ) sourcename;
		*( wp + numsources + 1 ) = NULL;
		mbytes = regval_build_multi_sz( wp, &msz_wp );
		dump_data( 1, ( char * ) msz_wp, mbytes );
		regval_ctr_addvalue( values, "Sources", REG_MULTI_SZ,
				     ( char * ) msz_wp, mbytes );
		regdb_store_values( evtlogpath, values );
		TALLOC_FREE( msz_wp );
	} else {
		DEBUG( 3,
		       ( "Source name [%s] found in existing list of sources\n",
			 sourcename ) );
	}
	TALLOC_FREE( values );
	if ( wrklist )
		TALLOC_FREE( wrklist );	/*  */

	if ( !( subkeys = TALLOC_ZERO_P( NULL, REGSUBKEY_CTR ) ) ) {
		DEBUG( 0, ( "talloc() failure!\n" ) );
		return False;
	}
	pstr_sprintf( evtlogpath, "%s\\%s", KEY_EVENTLOG, eventlog );

	regdb_fetch_keys( evtlogpath, subkeys );

	if ( !regsubkey_ctr_key_exists( subkeys, sourcename ) ) {
		DEBUG( 5,
		       ( " Source name [%s] for eventlog [%s] didn't exist, adding \n",
			 sourcename, eventlog ) );
		regsubkey_ctr_addkey( subkeys, sourcename );
		if ( !regdb_store_keys( evtlogpath, subkeys ) )
			return False;
	}
	TALLOC_FREE( subkeys );

	/* at this point KEY_EVENTLOG/<eventlog>/<sourcename> key is in there. Now need to add EventMessageFile */

	/* now allocate room for the source's subkeys */

	if ( !( subkeys = TALLOC_ZERO_P( NULL, REGSUBKEY_CTR ) ) ) {
		DEBUG( 0, ( "talloc() failure!\n" ) );
		return False;
	}
	slprintf( evtlogpath, sizeof( evtlogpath ) - 1, "%s\\%s\\%s",
		  KEY_EVENTLOG, eventlog, sourcename );

	regdb_fetch_keys( evtlogpath, subkeys );

	/* now add the values to the KEY_EVENTLOG/Application form key */
	if ( !( values = TALLOC_ZERO_P( NULL, REGVAL_CTR ) ) ) {
		DEBUG( 0, ( "talloc() failure!\n" ) );
		return False;
	}
	DEBUG( 5,
	       ( "Storing EventMessageFile [%s] to eventlog path of [%s]\n",
		 messagefile, evtlogpath ) );

	regdb_fetch_values( evtlogpath, values );

	init_unistr2( &data, messagefile, UNI_STR_TERMINATE );

	regval_ctr_addvalue( values, "EventMessageFile", REG_SZ,
			     ( char * ) data.buffer,
			     data.uni_str_len * sizeof( uint16 ) );
	regdb_store_values( evtlogpath, values );

	TALLOC_FREE( values );

	return True;
}
