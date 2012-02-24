#ifndef _EMU_SOCKET_EMU_SOCKET_H_
#define _EMU_SOCKET_EMU_SOCKET_H_

/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#define SOCK_RAW 3

#define EAFNOSUPPORT WSAEAFNOSUPPORT

#define INET_ADDRSTRLEN 16

struct mphostent {
	  char *h_name;	      /* official name of host */
	  char **h_aliases;   /* alias list */
	  short  h_addrtype;	  /* host address type	*/
	  short  h_length;	    /* length of	address	*/
	  char **h_addr_list; /* list of addresses	from name server */
};

#ifndef IN_MULTICAST
#define IN_MULTICAST(i)	    (((i) & 0xf0000000U) == 0xe0000000U)
#endif

#ifndef IN_EXPERIMENTAL
#define IN_EXPERIMENTAL(i)  (((i) & 0xe0000000U) == 0xe0000000U)
#endif

#ifndef IN_LOOPBACKNET
#define IN_LOOPBACKNET	    127
#endif

/*
typedef struct addrinfo
{
    int                 ai_flags;       // AI_PASSIVE, AI_CANONNAME, AI_NUMERICHOST
    int                 ai_family;      // PF_xxx
    int                 ai_socktype;    // SOCK_xxx
    int                 ai_protocol;    // 0 or IPPROTO_xxx for IPv4 and IPv6
    size_t              ai_addrlen;     // Length of ai_addr
    char *              ai_canonname;   // Canonical name for nodename
    struct sockaddr *   ai_addr;        // Binary address
    struct addrinfo *   ai_next;        // Next structure in linked list
}
ADDRINFOA, *PADDRINFOA;
*/
#endif //_EMU_SOCKET_EMU_SOCKET_H_
