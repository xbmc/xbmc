/* 
   Unix SMB/CIFS implementation.
   server specific string routines
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

#define srvstr_pull(base_ptr, dest, src, dest_len, src_len, flags) \
    pull_string(base_ptr, dest, src, dest_len, src_len, flags)

/* pull a string from the smb_buf part of a packet. In this case the
   string can either be null terminated or it can be terminated by the
   end of the smbbuf area 
*/

#define srvstr_pull_buf(inbuf, dest, src, dest_len, flags) \
    pull_string(inbuf, dest, src, dest_len, smb_bufrem(inbuf, src), flags)

