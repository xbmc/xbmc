/* external.h
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
#ifndef __external_h__
#define __external_h__

#include "srtypes.h"

#define MAX_EXT_LINE_LEN 255

typedef struct external_process External_Process;
struct external_process
{
#if defined (WIN32)
    HANDLE mypipe;   /* read from child stdout */
    HANDLE hproc;
    DWORD pid;
#else
    int mypipe[2];   /* 0 is for parent reading, 1 is for parent writing */
    pid_t pid;
#endif
    int line_buf_idx;
    char line_buf[MAX_EXT_LINE_LEN];
    char album_buf[MAX_EXT_LINE_LEN];
    char artist_buf[MAX_EXT_LINE_LEN];
    char title_buf[MAX_EXT_LINE_LEN];
    char metadata_buf[MAX_EXT_LINE_LEN];
};

External_Process* spawn_external (char* cmd);
int read_external (External_Process* ep, TRACK_INFO* ti);
void close_external (External_Process** epp);

#endif
