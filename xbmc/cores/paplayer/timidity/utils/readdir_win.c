/*
    TiMidity++ -- MIDI to WAVE converter and player
    Copyright (C) 1999-2002 Masanao Izumo <mo@goice.co.jp>
    Copyright (C) 1995 Tuukka Toivonen <tt@cgs.fi>

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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    readdir.c
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */
#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif
#ifndef NO_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif

#include "timidity.h"
#include "common.h"

#ifdef __W32__
#include "readdir.h"

/**********************************************************************
 * Implement dirent-style opendir/readdir/closedir on Window 95/NT
 *
 * Functions defined are opendir(), readdir() and closedir() with the
 * same prototypes as the normal dirent.h implementation.
 *
 * Does not implement telldir(), seekdir(), rewinddir() or scandir().
 * The dirent struct is compatible with Unix, except that d_ino is
 * always 1 and d_off is made up as we go along.
 *
 * The DIR typedef is not compatible with Unix.
 **********************************************************************/

API_EXPORT(DIR *) opendir(const char *dir)
{
    DIR *dp;
    char *filespec;
    long handle;
    int index;

    filespec = safe_malloc(strlen(dir) + 2 + 1);
    strcpy(filespec, dir);
    index = strlen(filespec) - 1;
    if (index >= 0 && (filespec[index] == '/' || filespec[index] == '\\'))
        filespec[index] = '\0';
    strcat(filespec, "/*");

    dp = (DIR *)safe_malloc(sizeof(DIR));
    dp->offset = 0;
    dp->finished = 0;
    dp->dir = safe_strdup(dir);

    if ((handle = _findfirst(filespec, &(dp->fileinfo))) < 0) {
        if (errno == ENOENT)
            dp->finished = 1;
        else
        return NULL;
    }

    dp->handle = handle;
    free(filespec);

    return dp;
}

API_EXPORT(struct dirent *) readdir(DIR *dp)
{
    if (!dp || dp->finished) return NULL;

    if (dp->offset != 0) {
        if (_findnext(dp->handle, &(dp->fileinfo)) < 0) {
            dp->finished = 1;
            return NULL;
        }
    }
    dp->offset++;

#ifdef __BORLANDC__
/* Borland C++ */
    strncpy(dp->dent.d_name, dp->fileinfo.ff_name, _MAX_FNAME);
#else
    strncpy(dp->dent.d_name, dp->fileinfo.name, _MAX_FNAME);
#endif /* __BORLANDC__ */

    dp->dent.d_name[_MAX_FNAME] = '\0';
    dp->dent.d_ino = 1;
    dp->dent.d_reclen = strlen(dp->dent.d_name);
    dp->dent.d_off = dp->offset;

    return &(dp->dent);
}

API_EXPORT(int) closedir(DIR *dp)
{
    if (!dp) return 0;

#ifndef __BORLANDC__
    _findclose(dp->handle);
#endif
    if (dp->dir) free(dp->dir);
    if (dp) free(dp);

    return 0;
}
#endif /* __W32__ */
