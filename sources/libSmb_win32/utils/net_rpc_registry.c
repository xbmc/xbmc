/* 
   Samba Unix/Linux SMB client library 
   Distributed SMB/CIFS Server Management Utility 
   Copyright (C) Gerald (Jerry) Carter          2005

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
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */
 
#include "includes.h"
#include "utils/net.h"
#include "regfio.h"
#include "reg_objects.h"

/********************************************************************
********************************************************************/

char* dump_regval_type( uint32 type )
{
	static fstring string;
	
	switch (type) {
	case REG_SZ:
		fstrcpy( string, "REG_SZ" );
		break;
	case REG_MULTI_SZ:
		fstrcpy( string, "REG_MULTI_SZ" );
		break;
	case REG_EXPAND_SZ:
		fstrcpy( string, "REG_EXPAND_SZ" );
		break;
	case REG_DWORD:
		fstrcpy( string, "REG_DWORD" );
		break;
	case REG_BINARY:
		fstrcpy( string, "REG_BINARY" );
		break;
	default:
		fstr_sprintf( string, "UNKNOWN [%d]", type );
	}
	
	return string;
}
/********************************************************************
********************************************************************/

void dump_regval_buffer( uint32 type, REGVAL_BUFFER *buffer )
{
	pstring string;
	uint32 value;
	
	switch (type) {
	case REG_SZ:
		rpcstr_pull( string, buffer->buffer, sizeof(string), -1, STR_TERMINATE );
		d_printf("%s\n", string);
		break;
	case REG_MULTI_SZ:
		d_printf("\n");
		break;
	case REG_DWORD:
		value = IVAL( buffer->buffer, 0 );
		d_printf( "0x%x\n", value );
		break;
	case REG_BINARY:
		d_printf("\n");
		break;
	
	
	default:
		d_printf( "\tUnknown type [%d]\n", type );
	}
}

/********************************************************************
********************************************************************/

static NTSTATUS rpc_registry_enumerate_internal(const DOM_SID *domain_sid,
						const char *domain_name, 
						struct cli_state *cli,
						struct rpc_pipe_client *pipe_hnd,
						TALLOC_CTX *mem_ctx, 
						int argc,
						const char **argv )
{
	WERROR result = WERR_GENERAL_FAILURE;
	uint32 hive;
	pstring subpath;
	POLICY_HND pol_hive, pol_key; 
	uint32 idx;
	
	if (argc != 1 ) {
		d_printf("Usage:    net rpc enumerate <path> [recurse]\n");
		d_printf("Example:  net rpc enumerate 'HKLM\\Software\\Samba'\n");
		return NT_STATUS_OK;
	}
	
	if ( !reg_split_hive( argv[0], &hive, subpath ) ) {
		d_fprintf(stderr, "invalid registry path\n");
		return NT_STATUS_OK;
	}
	
	/* open the top level hive and then the registry key */
	
	result = rpccli_reg_connect(pipe_hnd, mem_ctx, hive, MAXIMUM_ALLOWED_ACCESS, &pol_hive );
	if ( !W_ERROR_IS_OK(result) ) {
		d_fprintf(stderr, "Unable to connect to remote registry\n");
		return werror_to_ntstatus(result);
	}
	
	if ( strlen( subpath ) != 0 ) {
		result = rpccli_reg_open_entry(pipe_hnd, mem_ctx, &pol_hive, subpath, MAXIMUM_ALLOWED_ACCESS, &pol_key );
		if ( !W_ERROR_IS_OK(result) ) {
			d_fprintf(stderr, "Unable to open [%s]\n", argv[0]);
			return werror_to_ntstatus(result);
		}
	}
	
	/* get the subkeys */
	
	result = WERR_OK;
	idx = 0;
	while ( W_ERROR_IS_OK(result) ) {
		time_t modtime;
		fstring keyname, classname;
		
		result = rpccli_reg_enum_key(pipe_hnd, mem_ctx, &pol_key, idx, 
			keyname, classname, &modtime );
			
		if ( W_ERROR_EQUAL(result, WERR_NO_MORE_ITEMS) ) {
			result = WERR_OK;
			break;
		}
			
		d_printf("Keyname   = %s\n", keyname );
		d_printf("Classname = %s\n", classname );
		d_printf("Modtime   = %s\n", http_timestring(modtime) );
		d_printf("\n" );

		idx++;
	}

	if ( !W_ERROR_IS_OK(result) )
		goto out;
	
	/* get the values */
	
	result = WERR_OK;
	idx = 0;
	while ( W_ERROR_IS_OK(result) ) {
		uint32 type;
		fstring name;
		REGVAL_BUFFER value;
		
		fstrcpy( name, "" );
		ZERO_STRUCT( value );
		
		result = rpccli_reg_enum_val(pipe_hnd, mem_ctx, &pol_key, idx, 
			name, &type, &value );
			
		if ( W_ERROR_EQUAL(result, WERR_NO_MORE_ITEMS) ) {
			result = WERR_OK;
			break;
		}
			
		d_printf("Valuename  = %s\n", name );
		d_printf("Type       = %s\n", dump_regval_type(type) );
		d_printf("Data       = " );
		dump_regval_buffer( type, &value );
		d_printf("\n" );

		idx++;
	}
	
	
out:
	/* cleanup */
	
	if ( strlen( subpath ) != 0 )
		rpccli_reg_close(pipe_hnd, mem_ctx, &pol_key );
	rpccli_reg_close(pipe_hnd, mem_ctx, &pol_hive );

	return werror_to_ntstatus(result);
}

/********************************************************************
********************************************************************/

static int rpc_registry_enumerate( int argc, const char **argv )
{
	return run_rpc_command( NULL, PI_WINREG, 0, 
		rpc_registry_enumerate_internal, argc, argv );
}

/********************************************************************
********************************************************************/

static NTSTATUS rpc_registry_save_internal(const DOM_SID *domain_sid,
					const char *domain_name, 
					struct cli_state *cli,
					struct rpc_pipe_client *pipe_hnd,
					TALLOC_CTX *mem_ctx, 
					int argc,
					const char **argv )
{
	WERROR result = WERR_GENERAL_FAILURE;
	uint32 hive;
	pstring subpath;
	POLICY_HND pol_hive, pol_key; 
	
	if (argc != 2 ) {
		d_printf("Usage:    net rpc backup <path> <file> \n");
		return NT_STATUS_OK;
	}
	
	if ( !reg_split_hive( argv[0], &hive, subpath ) ) {
		d_fprintf(stderr, "invalid registry path\n");
		return NT_STATUS_OK;
	}
	
	/* open the top level hive and then the registry key */
	
	result = rpccli_reg_connect(pipe_hnd, mem_ctx, hive, MAXIMUM_ALLOWED_ACCESS, &pol_hive );
	if ( !W_ERROR_IS_OK(result) ) {
		d_fprintf(stderr, "Unable to connect to remote registry\n");
		return werror_to_ntstatus(result);
	}
	
	result = rpccli_reg_open_entry(pipe_hnd, mem_ctx, &pol_hive, subpath, MAXIMUM_ALLOWED_ACCESS, &pol_key );
	if ( !W_ERROR_IS_OK(result) ) {
		d_fprintf(stderr, "Unable to open [%s]\n", argv[0]);
		return werror_to_ntstatus(result);
	}
	
	result = rpccli_reg_save_key(pipe_hnd, mem_ctx, &pol_key, argv[1] );
	if ( !W_ERROR_IS_OK(result) ) {
		d_fprintf(stderr, "Unable to save [%s] to %s:%s\n", argv[0], cli->desthost, argv[1]);
	}
	
	
	/* cleanup */
	
	rpccli_reg_close(pipe_hnd, mem_ctx, &pol_key );
	rpccli_reg_close(pipe_hnd, mem_ctx, &pol_hive );

	return werror_to_ntstatus(result);
}

/********************************************************************
********************************************************************/

static int rpc_registry_save( int argc, const char **argv )
{
	return run_rpc_command( NULL, PI_WINREG, 0, 
		rpc_registry_save_internal, argc, argv );
}


/********************************************************************
********************************************************************/

static void dump_values( REGF_NK_REC *nk )
{
	int i, j;
	pstring data_str;
	uint32 data_size, data;

	if ( !nk->values )
		return;

	for ( i=0; i<nk->num_values; i++ ) {
		d_printf( "\"%s\" = ", nk->values[i].valuename ? nk->values[i].valuename : "(default)" );
		d_printf( "(%s) ", dump_regval_type( nk->values[i].type ) );

		data_size = nk->values[i].data_size & ~VK_DATA_IN_OFFSET;
		switch ( nk->values[i].type ) {
			case REG_SZ:
				rpcstr_pull( data_str, nk->values[i].data, sizeof(data_str), -1, STR_TERMINATE );
				d_printf( "%s", data_str );
				break;
			case REG_MULTI_SZ:
			case REG_EXPAND_SZ:
				for ( j=0; j<data_size; j++ ) {
					d_printf( "%c", nk->values[i].data[j] );
				}
				break;
			case REG_DWORD:
				data = IVAL( nk->values[i].data, 0 );
				d_printf("0x%x", data );
				break;
			case REG_BINARY:
				for ( j=0; j<data_size; j++ ) {
					d_printf( "%x", nk->values[i].data[j] );
				}
				break;
			default:
				d_printf("unknown");
				break;
		}

		d_printf( "\n" );
	}

}

/********************************************************************
********************************************************************/

static BOOL dump_registry_tree( REGF_FILE *file, REGF_NK_REC *nk, const char *parent )
{
	REGF_NK_REC *key;
	pstring regpath;

	/* depth first dump of the registry tree */

	while ( (key = regfio_fetch_subkey( file, nk )) ) {
		pstr_sprintf( regpath, "%s\\%s", parent, key->keyname );
		d_printf("[%s]\n", regpath );
		dump_values( key );
		d_printf("\n");
		dump_registry_tree( file, key, regpath );
	}

	return True;
}

/********************************************************************
********************************************************************/

static BOOL write_registry_tree( REGF_FILE *infile, REGF_NK_REC *nk, 
                                 REGF_NK_REC *parent, REGF_FILE *outfile,
			         const char *parentpath )
{
	REGF_NK_REC *key, *subkey;
	REGVAL_CTR *values;
	REGSUBKEY_CTR *subkeys;
	int i;
	pstring path;

	if ( !( subkeys = TALLOC_ZERO_P( infile->mem_ctx, REGSUBKEY_CTR )) ) {
		DEBUG(0,("write_registry_tree: talloc() failed!\n"));
		return False;
	}

	if ( !(values = TALLOC_ZERO_P( subkeys, REGVAL_CTR )) ) {
		DEBUG(0,("write_registry_tree: talloc() failed!\n"));
		return False;
	}

	/* copy values into the REGVAL_CTR */
	
	for ( i=0; i<nk->num_values; i++ ) {
		regval_ctr_addvalue( values, nk->values[i].valuename, nk->values[i].type,
			(const char *)nk->values[i].data, (nk->values[i].data_size & ~VK_DATA_IN_OFFSET) );
	}

	/* copy subkeys into the REGSUBKEY_CTR */
	
	while ( (subkey = regfio_fetch_subkey( infile, nk )) ) {
		regsubkey_ctr_addkey( subkeys, subkey->keyname );
	}
	
	key = regfio_write_key( outfile, nk->keyname, values, subkeys, nk->sec_desc->sec_desc, parent );

	/* write each one of the subkeys out */

	pstr_sprintf( path, "%s%s%s", parentpath, parent ? "\\" : "", nk->keyname );
	nk->subkey_index = 0;
	while ( (subkey = regfio_fetch_subkey( infile, nk )) ) {
		write_registry_tree( infile, subkey, key, outfile, path );
	}

	TALLOC_FREE( subkeys );

	d_printf("[%s]\n", path );
	
	return True;
}

/********************************************************************
********************************************************************/

static int rpc_registry_dump( int argc, const char **argv )
{
	REGF_FILE   *registry;
	REGF_NK_REC *nk;
	
	if (argc != 1 ) {
		d_printf("Usage:    net rpc dump <file> \n");
		return 0;
	}
	
	d_printf("Opening %s....", argv[0]);
	if ( !(registry = regfio_open( argv[0], O_RDONLY, 0)) ) {
		d_fprintf(stderr, "Failed to open %s for reading\n", argv[0]);
		return 1;
	}
	d_printf("ok\n");
	
	/* get the root of the registry file */
	
	if ((nk = regfio_rootkey( registry )) == NULL) {
		d_fprintf(stderr, "Could not get rootkey\n");
		regfio_close( registry );
		return 1;
	}
	d_printf("[%s]\n", nk->keyname);
	dump_values( nk );
	d_printf("\n");

	dump_registry_tree( registry, nk, nk->keyname );

#if 0
	talloc_report_full( registry->mem_ctx, stderr );
#endif	
	d_printf("Closing registry...");
	regfio_close( registry );
	d_printf("ok\n");

	return 0;
}

/********************************************************************
********************************************************************/

static int rpc_registry_copy( int argc, const char **argv )
{
	REGF_FILE   *infile = NULL, *outfile = NULL;
	REGF_NK_REC *nk;
	int result = 1;
	
	if (argc != 2 ) {
		d_printf("Usage:    net rpc copy <srcfile> <newfile>\n");
		return 0;
	}
	
	d_printf("Opening %s....", argv[0]);
	if ( !(infile = regfio_open( argv[0], O_RDONLY, 0 )) ) {
		d_fprintf(stderr, "Failed to open %s for reading\n", argv[0]);
		return 1;
	}
	d_printf("ok\n");

	d_printf("Opening %s....", argv[1]);
	if ( !(outfile = regfio_open( argv[1], (O_RDWR|O_CREAT|O_TRUNC), (S_IREAD|S_IWRITE) )) ) {
		d_fprintf(stderr, "Failed to open %s for writing\n", argv[1]);
		goto out;
	}
	d_printf("ok\n");
	
	/* get the root of the registry file */
	
	if ((nk = regfio_rootkey( infile )) == NULL) {
		d_fprintf(stderr, "Could not get rootkey\n");
		goto out;
	}
	d_printf("RootKey: [%s]\n", nk->keyname);

	write_registry_tree( infile, nk, NULL, outfile, "" );

	result = 0;

out:

	d_printf("Closing %s...", argv[1]);
	if (outfile) {
		regfio_close( outfile );
	}
	d_printf("ok\n");

	d_printf("Closing %s...", argv[0]);
	if (infile) {
		regfio_close( infile );
	}
	d_printf("ok\n");

	return( result);
}

/********************************************************************
********************************************************************/

static int net_help_registry( int argc, const char **argv )
{
	d_printf("net rpc registry enumerate <path> [recurse]  Enumerate the subkeya and values for a given registry path\n");
	d_printf("net rpc registry save <path> <file>          Backup a registry tree to a file on the server\n");
	d_printf("net rpc registry dump <file>                 Dump the contents of a registry file to stdout\n");
	
	return -1;
}

/********************************************************************
********************************************************************/

int net_rpc_registry(int argc, const char **argv) 
{
	struct functable func[] = {
		{"enumerate", rpc_registry_enumerate},
		{"save",      rpc_registry_save},
		{"dump",      rpc_registry_dump},
		{"copy",      rpc_registry_copy},
		{NULL, NULL}
	};
	
	if ( argc )
		return net_run_function( argc, argv, func, net_help_registry );
		
	return net_help_registry( argc, argv );
}
