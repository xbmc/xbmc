/* 
   Unix SMB/CIFS implementation.
   low level tdb backup and restore utility
   Copyright (C) Andrew Tridgell              2002

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

/*

  This program is meant for backup/restore of tdb databases. Typical usage would be:
     tdbbackup *.tdb
  when Samba shuts down cleanly, which will make a backup of all the local databases
  to *.bak files. Then on Samba startup you would use:
     tdbbackup -v *.tdb
  and this will check the databases for corruption and if corruption is detected then
  the backup will be restored.

  You may also like to do a backup on a regular basis while Samba is
  running, perhaps using cron.

  The reason this program is needed is to cope with power failures
  while Samba is running. A power failure could lead to database
  corruption and Samba will then not start correctly.

  Note that many of the databases in Samba are transient and thus
  don't need to be backed up, so you can optimise the above a little
  by only running the backup on the critical databases.

 */

#ifdef STANDALONE
#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <ctype.h>
#include <signal.h>

#else

#include "includes.h"

#endif

#include "tdb.h"
#include "tdbback.h"

extern int optind;
extern char *optarg;

/*
  see if one file is newer than another
*/
static int file_newer(const char *fname1, const char *fname2)
{
	struct stat st1, st2;
	if (stat(fname1, &st1) != 0) {
		return 0;
	}
	if (stat(fname2, &st2) != 0) {
		return 1;
	}
	return (st1.st_mtime > st2.st_mtime);
}

static void usage(void)
{
	printf("Usage: tdbbackup [options] <fname...>\n\n");
	printf("   -h            this help message\n");
	printf("   -s suffix     set the backup suffix\n");
	printf("   -v            verify mode (restore if corrupt)\n");
}
		

 int main(int argc, char *argv[])
{
	int i;
	int ret = 0;
	int c;
	int verify = 0;
	const char *suffix = ".bak";

	while ((c = getopt(argc, argv, "vhs:")) != -1) {
		switch (c) {
		case 'h':
			usage();
			exit(0);
		case 'v':
			verify = 1;
			break;
		case 's':
			suffix = optarg;
			break;
		}
	}

	argc -= optind;
	argv += optind;

	if (argc < 1) {
		usage();
		exit(1);
	}

	for (i=0; i<argc; i++) {
		const char *fname = argv[i];
		char *bak_name;

		bak_name = add_suffix(fname, suffix);

		if (verify) {
			if (verify_tdb(fname, bak_name) != 0) {
				ret = 1;
			}
		} else {
			if (file_newer(fname, bak_name) &&
			    backup_tdb(fname, bak_name) != 0) {
				ret = 1;
			}
		}

		free(bak_name);
	}

	return ret;
}
