
/*
 * Samba Unix/Linux SMB client utility 
 * Write Eventlog records to a tdb, perform other eventlog related functions
 *
 *
 * Copyright (C) Brian Moran                2005.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */


#include "includes.h"

#undef  DBGC_CLASS
#define DBGC_CLASS DBGC_UTIL_EVENTLOG


extern int optind;
extern char *optarg;

int opt_debug = 0;

static void usage( char *s )
{
	printf( "\nUsage: %s [OPTION]\n\n", s );
	printf( " -o write <Eventlog Name> \t\t\t\t\tWrites records to eventlog from STDIN\n" );
	printf( " -o addsource <EventlogName> <sourcename> <msgfileDLLname> \tAdds the specified source & DLL eventlog registry entry\n" );
	printf( "\nMiscellaneous options:\n" );
	printf( " -d\t\t\t\t\t\t\t\tturn debug on\n" );
	printf( " -h\t\t\t\t\t\t\t\tdisplay help\n\n" );
}

static void display_eventlog_names( void )
{
	const char **elogs;
	int i;

	elogs = lp_eventlog_list(  );
	printf( "Active eventlog names (from smb.conf):\n" );
	printf( "--------------------------------------\n" );
	if ( elogs ) {
		for ( i = 0; elogs[i]; i++ ) {
			printf( "\t%s\n", elogs[i] );
		}
	} 
	else
		printf( "\t<None specified>\n");
}

int DoAddSourceCommand( int argc, char **argv, BOOL debugflag, char *exename )
{

	if ( argc < 3 ) {
		printf( "need more arguments:\n" );
		printf( "-o addsource EventlogName SourceName /path/to/EventMessageFile.dll\n" );
		return -1;
	}
	/* must open the registry before we access it */
	if ( !regdb_init(  ) ) {
		printf( "Can't open the registry.\n" );
		return -1;
	}

	if ( !eventlog_add_source( argv[0], argv[1], argv[2] ) )
		return -2;
	return 0;
}

int DoWriteCommand( int argc, char **argv, BOOL debugflag, char *exename )
{
	FILE *f1;
	char *argfname;
	ELOG_TDB *etdb;

	/* fixed constants are bad bad bad  */
	pstring linein;
	BOOL is_eor;
	Eventlog_entry ee;
	int rcnum;

	f1 = stdin;
	if ( !f1 ) {
		printf( "Can't open STDIN\n" );
		return -1;
	}

	if ( debugflag ) {
		printf( "Starting write for eventlog [%s]\n", argv[0] );
		display_eventlog_names(  );
	}

	argfname = argv[0];

	if ( !( etdb = elog_open_tdb( argfname, False ) ) ) {
		printf( "can't open the eventlog TDB (%s)\n", argfname );
		return -1;
	}

	ZERO_STRUCT( ee );	/* MUST initialize between records */

	while ( !feof( f1 ) ) {
		fgets( linein, sizeof( linein ) - 1, f1 );
		linein[strlen( linein ) - 1] = 0;	/* whack the line delimiter */

		if ( debugflag )
			printf( "Read line [%s]\n", linein );

		is_eor = False;


		parse_logentry( ( char * ) &linein, &ee, &is_eor );
		/* should we do something with the return code? */

		if ( is_eor ) {
			fixup_eventlog_entry( &ee );

			if ( opt_debug )
				printf( "record number [%d], tg [%d] , tw [%d]\n", ee.record.record_number, ee.record.time_generated, ee.record.time_written );

			if ( ee.record.time_generated != 0 ) {

				/* printf("Writing to the event log\n"); */

				rcnum = write_eventlog_tdb( ELOG_TDB_CTX(etdb), &ee );
				if ( !rcnum ) {
					printf( "Can't write to the event log\n" );
				} else {
					if ( opt_debug )
						printf( "Wrote record %d\n",
							rcnum );
				}
			} else {
				if ( opt_debug )
					printf( "<null record>\n" );
			}
			ZERO_STRUCT( ee );	/* MUST initialize between records */
		}
	}

	elog_close_tdb( etdb , False );

	return 0;
}

/* would be nice to use the popT stuff here, however doing so forces us to drag in a lot of other infrastructure */

int main( int argc, char *argv[] )
{
	int opt, rc;
	char *exename;


	fstring opname;

	load_case_tables();

	opt_debug = 0;		/* todo set this from getopts */

	lp_load( dyn_CONFIGFILE, True, False, False, True);

	exename = argv[0];

	/* default */

	fstrcpy( opname, "write" );	/* the default */

#if 0				/* TESTING CODE */
	eventlog_add_source( "System", "TestSourceX", "SomeTestPathX" );
#endif
	while ( ( opt = getopt( argc, argv, "dho:" ) ) != EOF ) {
		switch ( opt ) {

		case 'o':
			fstrcpy( opname, optarg );
			break;

		case 'h':
			usage( exename );
			display_eventlog_names(  );
			exit( 0 );
			break;

		case 'd':
			opt_debug = 1;
			break;
		}
	}

	argc -= optind;
	argv += optind;

	if ( argc < 1 ) {
		printf( "\nNot enough arguments!\n" );
		usage( exename );
		exit( 1 );
	}

	/*  note that the separate command types should call usage if they need to... */
	while ( 1 ) {
		if ( !StrCaseCmp( opname, "addsource" ) ) {
			rc = DoAddSourceCommand( argc, argv, opt_debug,
						 exename );
			break;
		}
		if ( !StrCaseCmp( opname, "write" ) ) {
			rc = DoWriteCommand( argc, argv, opt_debug, exename );
			break;
		}
		printf( "unknown command [%s]\n", opname );
		usage( exename );
		exit( 1 );
		break;
	}
	return rc;
}
