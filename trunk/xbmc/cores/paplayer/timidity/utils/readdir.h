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

    readdir.h

    Structures and types used to implement opendir/readdir/closedir
    on Windows 95/NT.
*/

#ifndef ___READDIR_H_
#define ___READDIR_H_

#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

/* Borland C++ */
#ifdef __BORLANDC__
#include <dir.h>
#endif /* __BORLANDC__ */

#if 0
#define API_EXPORT(type)    __declspec(dllexport) type __stdcall
#else
#define API_EXPORT(type)    type
#endif

/* struct dirent - same as Unix */
struct dirent {
    long d_ino;			/* inode (always 1 in WIN32) */
    off_t d_off;		/* offset to this dirent */
    unsigned short d_reclen;	/* length of d_name */
    char d_name[_MAX_FNAME+1];	/* filename (null terminated) */
};

/* typedef DIR - not the same as Unix */
typedef struct {
    long handle;		/* _findfirst/_findnext handle */
    short offset;		/* offset into directory */
    short finished;		/* 1 if there are not more files */
#ifdef __BORLANDC__
    struct ffblk       fileinfo;/* from _findfirst/_findnext */
#else
    struct _finddata_t fileinfo;/* from _findfirst/_findnext */
#endif /* __BORLANDC__ */

    char *dir;			/* the dir we are reading */
    struct dirent dent;		/* the dirent to return */
} DIR;

/* Function prototypes */
extern API_EXPORT(DIR *) opendir(const char *);
extern API_EXPORT(struct dirent *) readdir(DIR *);
extern API_EXPORT(int) closedir(DIR *);

#ifdef __BORLANDC__
#define _findfirst(x,y) findfirst((x),(y),0)
#define _findnext(x,y) findnext((y))
#endif /* __BORLANDC__ */

#endif /* ___READDIR_H_ */
