/*
 * Unix SMB/CIFS implementation. 
 * clitar file format
 * Copyright (C) Andrew Tridgell              2000
 * 
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 * 
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 675
 * Mass Ave, Cambridge, MA 02139, USA.  
 */

#ifndef _CLITAR_H
#define _CLITAR_H

#define TBLOCK 512
#define NAMSIZ 100
union hblock {
  char dummy[TBLOCK];
  struct header {
    char name[NAMSIZ];
    char mode[8];
    char uid[8];
    char gid[8];
    char size[12];
    char mtime[12];
    char chksum[8];
    char linkflag;
    char linkname[NAMSIZ];
  } dbuf;
};

#endif /* _CLITAR_H */
