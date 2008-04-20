/* 
 *  Unix SMB/CIFS implementation.
 *  Virtual Windows Registry Layer (utility functions)
 *  Copyright (C) Gerald Carter                     2002-2005
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

/* Implementation of registry frontend view functions. */

#include "includes.h"

#undef DBGC_CLASS
#define DBGC_CLASS DBGC_RPC_SRV

/***********************************************************************
 Utility function for splitting the base path of a registry path off
 by setting base and new_path to the apprapriate offsets withing the
 path.
 
 WARNING!!  Does modify the original string!
 ***********************************************************************/

BOOL reg_split_path( char *path, char **base, char **new_path )
{
	char *p;
	
	*new_path = *base = NULL;
	
	if ( !path)
		return False;
	
	*base = path;
	
	p = strchr( path, '\\' );
	
	if ( p ) {
		*p = '\0';
		*new_path = p+1;
	}
	
	return True;
}


/***********************************************************************
 Utility function for splitting the base path of a registry path off
 by setting base and new_path to the appropriate offsets withing the
 path.
 
 WARNING!!  Does modify the original string!
 ***********************************************************************/

BOOL reg_split_key( char *path, char **base, char **key )
{
	char *p;
	
	*key = *base = NULL;
	
	if ( !path)
		return False;
	
	*base = path;
	
	p = strrchr( path, '\\' );
	
	if ( p ) {
		*p = '\0';
		*key = p+1;
	}
	
	return True;
}


/**********************************************************************
 The full path to the registry key is used as database after the 
 \'s are converted to /'s.  Key string is also normalized to UPPER
 case. 
**********************************************************************/

void normalize_reg_path( pstring keyname )
{
	pstring_sub( keyname, "\\", "/" );
	strupper_m( keyname  );
}

/**********************************************************************
 move to next non-delimter character
*********************************************************************/

char* reg_remaining_path( const char *key )
{
	static pstring new_path;
	char *p;
	
	if ( !key || !*key )
		return NULL;

	pstrcpy( new_path, key );
	/* normalize_reg_path( new_path ); */
	
	if ( !(p = strchr( new_path, '\\' )) ) 
	{
		if ( !(p = strchr( new_path, '/' )) )
			p = new_path;
		else 
			p++;
	}
	else
		p++;
		
	return p;
}

/**********************************************************************
*********************************************************************/

int regval_convert_multi_sz( uint16 *multi_string, size_t byte_len, char ***values )
{
	char **sz;
	int i;
	int num_strings = 0;
	fstring buffer;
	uint16 *wp;
	size_t multi_len = byte_len / 2;
	
	if ( !multi_string || !values )
		return 0;

	*values = NULL;

	/* just count the NULLs */
	
	for ( i=0; (i<multi_len-1) && !(multi_string[i]==0x0 && multi_string[i+1]==0x0); i++ ) {
		/* peek ahead */
		if ( multi_string[i+1] == 0x0 )
			num_strings++;
	}

	if ( num_strings == 0 )
		return 0;
	
	if ( !(sz = TALLOC_ARRAY( NULL, char*, num_strings+1 )) ) {
		DEBUG(0,("reg_convert_multi_sz: talloc() failed!\n"));
		return -1;
	}

	wp = multi_string;
	
	for ( i=0; i<num_strings; i++ ) {
		rpcstr_pull( buffer, wp, sizeof(buffer), -1, STR_TERMINATE );
		sz[i] = talloc_strdup( sz, buffer );
		
		/* skip to the next string NULL and then one more */
		while ( *wp )
			wp++;
		wp++;
	}
	
	/* tag the array off with an empty string */
	sz[i] = '\0';
	
	*values = sz;
	
	return num_strings;
}

/**********************************************************************
 Returns number of bytes, not number of unicode characters
*********************************************************************/

size_t regval_build_multi_sz( char **values, uint16 **buffer )
{
	int i;
	size_t buf_size = 0;
	uint16 *buf, *b;
	UNISTR2 sz;

	if ( !values || !buffer )
		return 0;
	
	/* go ahead and alloc some space */
	
	if ( !(buf = TALLOC_ARRAY( NULL, uint16, 2 )) ) {
		DEBUG(0,("regval_build_multi_sz: talloc() failed!\n"));
		return 0;
	}
	
	for ( i=0; values[i]; i++ ) {
		ZERO_STRUCT( sz );
		/* DEBUG(0,("regval_build_multi_sz: building [%s]\n",values[i])); */
		init_unistr2( &sz, values[i], UNI_STR_TERMINATE );
		
		/* Alloc some more memory.  Always add one one to account for the 
		   double NULL termination */
		   
		b = TALLOC_REALLOC_ARRAY( NULL, buf, uint16, buf_size+sz.uni_str_len+1 );
		if ( !b ) {
			DEBUG(0,("regval_build_multi_sz: talloc() reallocation error!\n"));
			TALLOC_FREE( buffer );
			return 0;
		}
		buf = b;

		/* copy the unistring2 buffer and increment the size */	
		/* dump_data(1,sz.buffer,sz.uni_str_len*2); */
		memcpy( buf+buf_size, sz.buffer, sz.uni_str_len*2 );
		buf_size += sz.uni_str_len;
		
		/* cleanup rather than leaving memory hanging around */
		TALLOC_FREE( sz.buffer );
	}
	
	buf[buf_size++] = 0x0;

	*buffer = buf;

	/* return number of bytes */
	return buf_size*2;
}


