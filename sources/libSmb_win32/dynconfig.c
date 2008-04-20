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

#include "includes.h"

/**
 * @file dynconfig.c
 *
 * @brief Global configurations, initialized to configured defaults.
 *
 * This file should be the only file that depends on path
 * configuration (--prefix, etc), so that if ./configure is re-run,
 * all programs will be appropriately updated.  Everything else in
 * Samba should import extern variables from here, rather than relying
 * on preprocessor macros.
 *
 * Eventually some of these may become even more variable, so that
 * they can for example consistently be set across the whole of Samba
 * by command-line parameters, config file entries, or environment
 * variables.
 *
 * @todo Perhaps eventually these should be merged into the parameter
 * table?  There's kind of a chicken-and-egg situation there...
 **/

char const *dyn_SBINDIR = SBINDIR,
	*dyn_BINDIR = BINDIR,
	*dyn_SWATDIR = SWATDIR;

pstring dyn_CONFIGFILE = CONFIGFILE; /**< Location of smb.conf file. **/

/** Log file directory. **/
pstring dyn_LOGFILEBASE = LOGFILEBASE;

/** Statically configured LanMan hosts. **/
pstring dyn_LMHOSTSFILE = LMHOSTSFILE;

/**
 * @brief Samba library directory.
 *
 * @sa lib_path() to get the path to a file inside the LIBDIR.
 **/
pstring dyn_LIBDIR = LIBDIR;
fstring dyn_SHLIBEXT = SHLIBEXT;

/**
 * @brief Directory holding lock files.
 *
 * Not writable, but used to set a default in the parameter table.
 **/
pstring dyn_LOCKDIR = LOCKDIR;
pstring dyn_PIDDIR  = PIDDIR;

pstring dyn_SMB_PASSWD_FILE = SMB_PASSWD_FILE;
pstring dyn_PRIVATE_DIR = PRIVATE_DIR;
