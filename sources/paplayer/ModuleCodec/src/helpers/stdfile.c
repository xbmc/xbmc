/*  _______         ____    __         ___    ___
 * \    _  \       \    /  \  /       \   \  /   /       '   '  '
 *  |  | \  \       |  |    ||         |   \/   |         .      .
 *  |  |  |  |      |  |    ||         ||\  /|  |
 *  |  |  |  |      |  |    ||         || \/ |  |         '  '  '
 *  |  |  |  |      |  |    ||         ||    |  |         .      .
 *  |  |_/  /        \  \__//          ||    |  |
 * /_______/ynamic    \____/niversal  /__\  /____\usic   /|  .  . ibliotheque
 *                                                      /  \
 *                                                     / .  \
 * stdfile.c - stdio file input module.               / / \  \
 *                                                   | <  /   \_
 * By entheh.                                        |  \/ /\   /
 *                                                    \_  /  > /
 *                                                      | \ / /
 *                                                      |  ' /
 *                                                       \__/
 */

#include <stdio.h>

#include "dumb.h"



static void *dumb_stdfile_open(const char *filename)
{
	return fopen(filename, "rb");
}



static int dumb_stdfile_skip(void *f, long n)
{
	return fseek(f, n, SEEK_CUR);
}



static int dumb_stdfile_getc(void *f)
{
	return fgetc(f);
}



static long dumb_stdfile_getnc(char *ptr, long n, void *f)
{
	return fread(ptr, 1, n, f);
}



static void dumb_stdfile_close(void *f)
{
	fclose(f);
}



static DUMBFILE_SYSTEM stdfile_dfs = {
	&dumb_stdfile_open,
	&dumb_stdfile_skip,
	&dumb_stdfile_getc,
	&dumb_stdfile_getnc,
	&dumb_stdfile_close
};



void dumb_register_stdfiles(void)
{
	register_dumbfile_system(&stdfile_dfs);
}



static DUMBFILE_SYSTEM stdfile_dfs_leave_open = {
	NULL,
	&dumb_stdfile_skip,
	&dumb_stdfile_getc,
	&dumb_stdfile_getnc,
	NULL
};



DUMBFILE *dumbfile_open_stdfile(FILE *p)
{
	DUMBFILE *d = dumbfile_open_ex(p, &stdfile_dfs_leave_open);

	return d;
}
