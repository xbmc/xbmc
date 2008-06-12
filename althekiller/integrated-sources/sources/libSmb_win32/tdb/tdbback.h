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

char *add_suffix(const char *name, const char *suffix);
int backup_tdb(const char *old_name, const char *new_name);
int verify_tdb(const char *fname, const char *bak_name);
