/* 
 *  Unix SMB/CIFS implementation.
 *  Virtual Windows Registry Layer
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

/**********************************************************************

 Note that the REGSUB_CTR and REGVAL_CTR objects *must* be talloc()'d
 since the methods use the object pointer as the talloc context for 
 internal private data.

 There is no longer a regXXX_ctr_intit() and regXXX_ctr_destroy()
 pair of functions.  Simply TALLOC_ZERO_P() and TALLOC_FREE() the 
 object.

 **********************************************************************/

/***********************************************************************
 Add a new key to the array
 **********************************************************************/

int regsubkey_ctr_addkey( REGSUBKEY_CTR *ctr, const char *keyname )
{
	if ( !keyname )
		return ctr->num_subkeys;

	/* make sure the keyname is not already there */

	if ( regsubkey_ctr_key_exists( ctr, keyname ) )
		return ctr->num_subkeys;
		
	/* allocate a space for the char* in the array */
		
	if (ctr->subkeys == NULL) {
		ctr->subkeys = TALLOC_P(ctr, char *);
	} else {
		ctr->subkeys = TALLOC_REALLOC_ARRAY(ctr, ctr->subkeys, char *, ctr->num_subkeys+1);
	}

	if (!ctr->subkeys) {
		ctr->num_subkeys = 0;
		return 0;
	}

	/* allocate the string and save it in the array */
	
	ctr->subkeys[ctr->num_subkeys] = talloc_strdup( ctr, keyname );
	ctr->num_subkeys++;
	
	return ctr->num_subkeys;
}
 
 /***********************************************************************
 Add a new key to the array
 **********************************************************************/

int regsubkey_ctr_delkey( REGSUBKEY_CTR *ctr, const char *keyname )
{
	int i;

	if ( !keyname )
		return ctr->num_subkeys;

	/* make sure the keyname is actually already there */

	for ( i=0; i<ctr->num_subkeys; i++ ) {
		if ( strequal( ctr->subkeys[i], keyname ) )
			break;
	}
	
	if ( i == ctr->num_subkeys )
		return ctr->num_subkeys;

	/* update if we have any keys left */
	ctr->num_subkeys--;
	if ( i < ctr->num_subkeys )
		memmove( &ctr->subkeys[i], &ctr->subkeys[i+1], sizeof(char*) * (ctr->num_subkeys-i) );
	
	return ctr->num_subkeys;
}

/***********************************************************************
 Check for the existance of a key
 **********************************************************************/

BOOL regsubkey_ctr_key_exists( REGSUBKEY_CTR *ctr, const char *keyname )
{
	int 	i;
	
	if (!ctr->subkeys) {
		return False;
	}

	for ( i=0; i<ctr->num_subkeys; i++ ) {
		if ( strequal( ctr->subkeys[i],keyname ) )
			return True;
	}
	
	return False;
}

/***********************************************************************
 How many keys does the container hold ?
 **********************************************************************/

int regsubkey_ctr_numkeys( REGSUBKEY_CTR *ctr )
{
	return ctr->num_subkeys;
}

/***********************************************************************
 Retreive a specific key string
 **********************************************************************/

char* regsubkey_ctr_specific_key( REGSUBKEY_CTR *ctr, uint32 key_index )
{
	if ( ! (key_index < ctr->num_subkeys) )
		return NULL;
		
	return ctr->subkeys[key_index];
}

/*
 * Utility functions for REGVAL_CTR
 */

/***********************************************************************
 How many keys does the container hold ?
 **********************************************************************/

int regval_ctr_numvals( REGVAL_CTR *ctr )
{
	return ctr->num_values;
}

/***********************************************************************
 allocate memory for and duplicate a REGISTRY_VALUE.
 This is malloc'd memory so the caller should free it when done
 **********************************************************************/

REGISTRY_VALUE* dup_registry_value( REGISTRY_VALUE *val )
{
	REGISTRY_VALUE 	*copy = NULL;
	
	if ( !val )
		return NULL;
	
	if ( !(copy = SMB_MALLOC_P( REGISTRY_VALUE)) ) {
		DEBUG(0,("dup_registry_value: malloc() failed!\n"));
		return NULL;
	}
	
	/* copy all the non-pointer initial data */
	
	memcpy( copy, val, sizeof(REGISTRY_VALUE) );
	
	copy->size = 0;
	copy->data_p = NULL;
	
	if ( val->data_p && val->size ) 
	{
		if ( !(copy->data_p = memdup( val->data_p, val->size )) ) {
			DEBUG(0,("dup_registry_value: memdup() failed for [%d] bytes!\n",
				val->size));
			SAFE_FREE( copy );
			return NULL;
		}
		copy->size = val->size;
	}
	
	return copy;	
}

/**********************************************************************
 free the memory allocated to a REGISTRY_VALUE 
 *********************************************************************/
 
void free_registry_value( REGISTRY_VALUE *val )
{
	if ( !val )
		return;
		
	SAFE_FREE( val->data_p );
	SAFE_FREE( val );
	
	return;
}

/**********************************************************************
 *********************************************************************/

uint8* regval_data_p( REGISTRY_VALUE *val )
{
	return val->data_p;
}

/**********************************************************************
 *********************************************************************/

uint32 regval_size( REGISTRY_VALUE *val )
{
	return val->size;
}

/**********************************************************************
 *********************************************************************/

char* regval_name( REGISTRY_VALUE *val )
{
	return val->valuename;
}

/**********************************************************************
 *********************************************************************/

uint32 regval_type( REGISTRY_VALUE *val )
{
	return val->type;
}

/***********************************************************************
 Retreive a pointer to a specific value.  Caller shoud dup the structure
 since this memory will go away when the ctr is free()'d
 **********************************************************************/

REGISTRY_VALUE* regval_ctr_specific_value( REGVAL_CTR *ctr, uint32 idx )
{
	if ( !(idx < ctr->num_values) )
		return NULL;
		
	return ctr->values[idx];
}

/***********************************************************************
 Check for the existance of a value
 **********************************************************************/

BOOL regval_ctr_key_exists( REGVAL_CTR *ctr, const char *value )
{
	int 	i;
	
	for ( i=0; i<ctr->num_values; i++ ) {
		if ( strequal( ctr->values[i]->valuename, value) )
			return True;
	}
	
	return False;
}
/***********************************************************************
 Add a new registry value to the array
 **********************************************************************/

int regval_ctr_addvalue( REGVAL_CTR *ctr, const char *name, uint16 type, 
                         const char *data_p, size_t size )
{
	if ( !name )
		return ctr->num_values;

	/* Delete the current value (if it exists) and add the new one */

	regval_ctr_delvalue( ctr, name );

	/* allocate a slot in the array of pointers */
		
	if (  ctr->num_values == 0 ) {
		ctr->values = TALLOC_P( ctr, REGISTRY_VALUE *);
	} else {
		ctr->values = TALLOC_REALLOC_ARRAY( ctr, ctr->values, REGISTRY_VALUE *, ctr->num_values+1 );
	}

	if (!ctr->values) {
		ctr->num_values = 0;
		return 0;
	}

	/* allocate a new value and store the pointer in the arrya */
		
	ctr->values[ctr->num_values] = TALLOC_P( ctr, REGISTRY_VALUE);
	if (!ctr->values[ctr->num_values]) {
		ctr->num_values = 0;
		return 0;
	}

	/* init the value */
	
	fstrcpy( ctr->values[ctr->num_values]->valuename, name );
	ctr->values[ctr->num_values]->type = type;
	ctr->values[ctr->num_values]->data_p = TALLOC_MEMDUP( ctr, data_p, size );
	ctr->values[ctr->num_values]->size = size;
	ctr->num_values++;

	return ctr->num_values;
}

/***********************************************************************
 Add a new registry value to the array
 **********************************************************************/

int regval_ctr_copyvalue( REGVAL_CTR *ctr, REGISTRY_VALUE *val )
{
	if ( val ) {
		/* allocate a slot in the array of pointers */
		
		if (  ctr->num_values == 0 ) {
			ctr->values = TALLOC_P( ctr, REGISTRY_VALUE *);
		} else {
			ctr->values = TALLOC_REALLOC_ARRAY( ctr, ctr->values, REGISTRY_VALUE *, ctr->num_values+1 );
		}

		if (!ctr->values) {
			ctr->num_values = 0;
			return 0;
		}

		/* allocate a new value and store the pointer in the arrya */
		
		ctr->values[ctr->num_values] = TALLOC_P( ctr, REGISTRY_VALUE);
		if (!ctr->values[ctr->num_values]) {
			ctr->num_values = 0;
			return 0;
		}

		/* init the value */
	
		fstrcpy( ctr->values[ctr->num_values]->valuename, val->valuename );
		ctr->values[ctr->num_values]->type = val->type;
		ctr->values[ctr->num_values]->data_p = TALLOC_MEMDUP( ctr, val->data_p, val->size );
		ctr->values[ctr->num_values]->size = val->size;
		ctr->num_values++;
	}

	return ctr->num_values;
}

/***********************************************************************
 Delete a single value from the registry container.
 No need to free memory since it is talloc'd.
 **********************************************************************/

int regval_ctr_delvalue( REGVAL_CTR *ctr, const char *name )
{
	int 	i;
	
	for ( i=0; i<ctr->num_values; i++ ) {
		if ( strequal( ctr->values[i]->valuename, name ) )
			break;
	}
	
	/* just return if we don't find it */
	
	if ( i == ctr->num_values )
		return ctr->num_values;
	
	/* If 'i' was not the last element, just shift everything down one */
	ctr->num_values--;
	if ( i < ctr->num_values )
		memmove( &ctr->values[i], &ctr->values[i+1], sizeof(REGISTRY_VALUE*)*(ctr->num_values-i) );
	
	return ctr->num_values;
}

/***********************************************************************
 Retrieve single value from the registry container.
 No need to free memory since it is talloc'd.
 **********************************************************************/

REGISTRY_VALUE* regval_ctr_getvalue( REGVAL_CTR *ctr, const char *name )
{
	int 	i;
	
	/* search for the value */
	
	for ( i=0; i<ctr->num_values; i++ ) {
		if ( strequal( ctr->values[i]->valuename, name ) )
			return ctr->values[i];
	}
	
	return NULL;
}

/***********************************************************************
 return the data_p as a uint32
 **********************************************************************/

uint32 regval_dword( REGISTRY_VALUE *val )
{
	uint32 data;
	
	data = IVAL( regval_data_p(val), 0 );
	
	return data;
}

/***********************************************************************
 return the data_p as a character string
 **********************************************************************/

char* regval_sz( REGISTRY_VALUE *val )
{
	static pstring data;

	rpcstr_pull( data, regval_data_p(val), sizeof(data), regval_size(val), 0 );
	
	return data;
}
