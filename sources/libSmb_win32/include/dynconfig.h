/* 
   Unix SMB/CIFS implementation.
   Copyright (C) 2001 by Martin Pool <mbp@samba.org>
   Copyright (C) 2003 by Jim McDonough <jmcd@us.ibm.com>
   
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

/**
 * @file dynconfig.h
 *
 * @brief Exported global configurations.
 **/

extern char const *dyn_SBINDIR,
	*dyn_BINDIR,
	*dyn_SWATDIR;

extern pstring dyn_CONFIGFILE;
extern pstring dyn_LOGFILEBASE, dyn_LMHOSTSFILE;
extern pstring dyn_LIBDIR;
extern fstring dyn_SHLIBEXT;
extern pstring dyn_LOCKDIR;
extern pstring dyn_PIDDIR;
extern pstring dyn_SMB_PASSWD_FILE;
extern pstring dyn_PRIVATE_DIR;
