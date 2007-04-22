/*
 * LinksBoks
 * Copyright (c) 2003-2005 ysbox
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

#ifdef __XBOX__

#ifndef _XBOX_DNS_H_
#define _XBOX_DNS_H_

/* DNS stuff (borrowed from XBMP) */
struct  hostent {
       char    * h_name;		/* official name of host */
       char    **h_aliases;		/* alias list */
       short   h_addrtype;		/* host address type */
       short   h_length;		/* length of address */
       char    *h_addr_list[4];	/* list of addresses */
#define h_addr  h_addr_list[0]          /* address, for backward compat */
};

int _cdecl gethostname( char *name, int namelen );
struct hostent* _cdecl gethostbyname( const char *name );

#endif

#endif