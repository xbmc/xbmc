/* 
   Unix SMB/CIFS implementation.
   stdio replacement
   Copyright (C) Andrew Tridgell 2001
   
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

#ifndef _XFILE_H_
#define _XFILE_H_
/*
  see xfile.c for explanations
*/

typedef struct {
	int fd;
	char *buf;
	char *next;
	int bufsize;
	int bufused;
	int open_flags;
	int buftype;
	int flags;
} XFILE;

extern XFILE *x_stdin, *x_stdout, *x_stderr;

/* buffering type */
#define X_IOFBF 0
#define X_IOLBF 1
#define X_IONBF 2

#define x_getc(f) x_fgetc(f)

int x_vfprintf(XFILE *f, const char *format, va_list ap) PRINTF_ATTRIBUTE(2, 0);
int x_fprintf(XFILE *f, const char *format, ...) PRINTF_ATTRIBUTE(2, 3);
#endif /* _XFILE_H_ */
