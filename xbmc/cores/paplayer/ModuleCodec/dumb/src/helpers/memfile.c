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
 * memfile.c - Module for reading data from           / / \  \
 *             memory using a DUMBFILE.              | <  /   \_
 *                                                   |  \/ /\   /
 * By entheh.                                         \_  /  > /
 *                                                      | \ / /
 *                                                      |  ' /
 *                                                       \__/
 */

#include <stdlib.h>
#include <string.h>

#include "dumb.h"



typedef struct MEMFILE MEMFILE;

struct MEMFILE
{
	const char *ptr;
	long left;
};



static int dumb_memfile_skip(void *f, long n)
{
	MEMFILE *m = f;
	if (n > m->left) return -1;
	m->ptr += n;
	m->left -= n;
	return 0;
}



static int dumb_memfile_getc(void *f)
{
	MEMFILE *m = f;
	if (m->left <= 0) return -1;
	m->left--;
	return *(const unsigned char *)m->ptr++;
}



static long dumb_memfile_getnc(char *ptr, long n, void *f)
{
	MEMFILE *m = f;
	if (n > m->left) n = m->left;
	memcpy(ptr, m->ptr, n);
	m->ptr += n;
	m->left -= n;
	return n;
}



static void dumb_memfile_close(void *f)
{
	free(f);
}



static DUMBFILE_SYSTEM memfile_dfs = {
	NULL,
	&dumb_memfile_skip,
	&dumb_memfile_getc,
	&dumb_memfile_getnc,
	&dumb_memfile_close
};



DUMBFILE *dumbfile_open_memory(const char *data, long size)
{
	MEMFILE *m = malloc(sizeof(*m));
	if (!m) return NULL;

	m->ptr = data;
	m->left = size;

	return dumbfile_open_ex(m, &memfile_dfs);
}
