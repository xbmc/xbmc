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

	Macintosh interface for TiMidity
	by T.Nogami	<t-nogami@happy.email.ne.jp>
	
    mac_util.h
*/

#ifndef	MACUTIL__
#define	MACUTIL__

OSErr	GetFullPath( const FSSpec*, Str255 fullPath);
void	StopAlertMessage(Str255);

void	SetDialogItemValue(DialogPtr dialog, short item, short value);
short	GetDialogItemValue(DialogPtr dialog, short item );
void	SetDialogTEValue(DialogRef dialog, short item, int value);
int	GetDialogTEValue(DialogRef dialog, short item );
short	ToggleDialogItem(DialogPtr dialog, short item );
void	myGetDialogItemText(DialogPtr theDialog, short itemNo, Str255 s);
void	mySetDialogItemText(DialogRef theDialog, short itemNo, const Str255 text);
void	SetDialogControlTitle(DialogRef theDialog, short itemNo, const Str255 text);
void	SetDialogItemHilite(DialogRef dialog, short item, short value);
void	mac_TransPathSeparater(const char str[], char out[]);
void	LDeselectAll(ListHandle);
void	TEReadFile(char* filename, TEHandle te);

#include <errno.h>
#include <stdio.h>

/* CodeWarrior dose not have these macro, so I wrote */

#define	ENOENT	9990
#define	EINVAL	9991
#define	EINTR	9992
#define EPERM   9993
#define EACCES  9994
#define ENOTDIR 9995
#define ENOSPC  dskFulErr

/*this function is very tricky. 
  Replace sys_errlist[errno] --> "error no.(errorno)"
  Because CodeWarrior does not support sys_errlist[].
  If your compiler supports, you need not applend this file.
*/
//char** sys_errlist_();
//#define sys_errlist sys_errlist_()
char* strdup(const char*);
//char* strncasecmp(const char*, const char*, int);
#define	strcasecmp mac_strcasecmp
//#define	strncasecmp mac_strncasecmp
int mac_strcasecmp(const char *s1, const char *s2);
int mac_strncasecmp(const char *s1, const char *s2, size_t n );
int strtailcasecmp(const char *s1, const char *s2);
char*	mac_fgets( char *buf, int n, FILE* file);
void*	mac_memchr( const void *s, int c, size_t n);

/* hacking standard function */
#define fgets  mac_fgets
#define memchr mac_memchr

#endif	/*MACUTIL__*/
