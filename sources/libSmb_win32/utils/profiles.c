/* 
   Samba Unix/Linux SMB client utility profiles.c 
   
   Copyright (C) Richard Sharpe, <rsharpe@richardsharpe.com>   2002 
   Copyright (C) Jelmer Vernooij (conversion to popt)          2003 
   Copyright (C) Gerald (Jerry) Carter                         2005 

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
#include "regfio.h"

/* GLOBAL VARIABLES */

DOM_SID old_sid, new_sid;
int change = 0, new_val = 0;


/********************************************************************
********************************************************************/

static BOOL swap_sid_in_acl( SEC_DESC *sd, DOM_SID *s1, DOM_SID *s2 )
{
	SEC_ACL *acl = sd->dacl;
	int i;
	BOOL update = False;

	if ( sid_equal( sd->owner_sid, s1 ) ) {
		sid_copy( sd->owner_sid, s2 );
		update = True;
	}

	if ( sid_equal( sd->grp_sid, s1 ) ) {
		sid_copy( sd->grp_sid, s2 );
		update = True;
	}

	for ( i=0; i<acl->num_aces; i++ ) {
		if ( sid_equal( &acl->ace[i].trustee, s1 ) ) {
			sid_copy( &acl->ace[i].trustee, s2 );
			update = True;
		}
	}

	return update;
}

/********************************************************************
********************************************************************/

static BOOL copy_registry_tree( REGF_FILE *infile, REGF_NK_REC *nk,
                                REGF_NK_REC *parent, REGF_FILE *outfile,
                                const char *parentpath  )
{
	REGF_NK_REC *key, *subkey;
	SEC_DESC *new_sd;
	REGVAL_CTR *values;
	REGSUBKEY_CTR *subkeys;
	int i;
	pstring path;

	/* swap out the SIDs in the security descriptor */

	if ( !(new_sd = dup_sec_desc( outfile->mem_ctx, nk->sec_desc->sec_desc )) ) {
		fprintf( stderr, "Failed to copy security descriptor!\n" );
		return False;
	}

	if ( swap_sid_in_acl( new_sd, &old_sid, &new_sid ) )
		DEBUG(2,("Updating ACL for %s\n", nk->keyname ));

	if ( !(subkeys = TALLOC_ZERO_P( NULL, REGSUBKEY_CTR )) ) {
		DEBUG(0,("copy_registry_tree: talloc() failure!\n"));
		return False;
	}

	if ( !(values = TALLOC_ZERO_P( subkeys, REGVAL_CTR )) ) {
		DEBUG(0,("copy_registry_tree: talloc() failure!\n"));
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

	key = regfio_write_key( outfile, nk->keyname, values, subkeys, new_sd, parent );

	/* write each one of the subkeys out */

	pstr_sprintf( path, "%s%s%s", parentpath, parent ? "\\" : "", nk->keyname );
	
	nk->subkey_index = 0;
	while ( (subkey = regfio_fetch_subkey( infile, nk )) ) {
		if ( !copy_registry_tree( infile, subkey, key, outfile, path ) )
			return False;
	}

	/* values is a talloc()'d child of subkeys here so just throw it all away */

	TALLOC_FREE( subkeys );

	DEBUG(1,("[%s]\n", path));

	return True;
}

/*********************************************************************
*********************************************************************/

int main( int argc, char *argv[] )
{
	int opt;
	REGF_FILE *infile, *outfile;
	REGF_NK_REC *nk;
	pstring orig_filename, new_filename;
	struct poptOption long_options[] = {
		POPT_AUTOHELP
		{ "change-sid", 'c', POPT_ARG_STRING, NULL, 'c', "Provides SID to change" },
		{ "new-sid", 'n', POPT_ARG_STRING, NULL, 'n', "Provides SID to change to" },
		POPT_COMMON_SAMBA
		POPT_COMMON_VERSION
		POPT_TABLEEND
	};
	poptContext pc;

	load_case_tables();

	/* setup logging options */

	setup_logging( "profiles", True );
	dbf = x_stderr;
	x_setbuf( x_stderr, NULL );

	pc = poptGetContext("profiles", argc, (const char **)argv, long_options, 
		POPT_CONTEXT_KEEP_FIRST);

	poptSetOtherOptionHelp(pc, "<profilefile>");

	/* Now, process the arguments */

	while ((opt = poptGetNextOpt(pc)) != -1) {
		switch (opt) {
		case 'c':
			change = 1;
			if (!string_to_sid(&old_sid, poptGetOptArg(pc))) {
				fprintf(stderr, "Argument to -c should be a SID in form of S-1-5-...\n");
				poptPrintUsage(pc, stderr, 0);
				exit(254);
			}
			break;

		case 'n':
			new_val = 1;
			if (!string_to_sid(&new_sid, poptGetOptArg(pc))) {
				fprintf(stderr, "Argument to -n should be a SID in form of S-1-5-...\n");
				poptPrintUsage(pc, stderr, 0);
				exit(253);
			}
			break;

		}
	}

	poptGetArg(pc); 

	if (!poptPeekArg(pc)) {
		poptPrintUsage(pc, stderr, 0);
		exit(1);
	}

	if ((!change && new_val) || (change && !new_val)) {
		fprintf(stderr, "You must specify both -c and -n if one or the other is set!\n");
		poptPrintUsage(pc, stderr, 0);
		exit(252);
	}

	pstrcpy( orig_filename, poptPeekArg(pc) );
	pstr_sprintf( new_filename, "%s.new", orig_filename );
	
	if ( !(infile = regfio_open( orig_filename, O_RDONLY, 0 )) ) {
		fprintf( stderr, "Failed to open %s!\n", orig_filename );
		fprintf( stderr, "Error was (%s)\n", strerror(errno) );
		exit (1);
	}
	
	if ( !(outfile = regfio_open( new_filename, (O_RDWR|O_CREAT|O_TRUNC), (S_IREAD|S_IWRITE) )) ) {
		fprintf( stderr, "Failed to open new file %s!\n", new_filename );
		fprintf( stderr, "Error was (%s)\n", strerror(errno) );
		exit (1);
	}
	
	/* actually do the update now */
	
	if ((nk = regfio_rootkey( infile )) == NULL) {
		fprintf(stderr, "Could not get rootkey\n");
		exit(3);
	}
	
	if ( !copy_registry_tree( infile, nk, NULL, outfile, "" ) ) {
		fprintf(stderr, "Failed to write updated registry file!\n");
		exit(2);
	}
	
	/* cleanup */
	
	regfio_close( infile );
	regfio_close( outfile );

	poptFreeContext(pc);

	return( 0 );
}
