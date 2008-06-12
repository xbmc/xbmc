/* 
   Unix SMB/CIFS implementation.
   multiple interface handling
   Copyright (C) Andrew Tridgell 1992-1998
   
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

static struct iface_struct *probed_ifaces;
static int total_probed;

struct in_addr allones_ip;
struct in_addr loopback_ip;

static struct interface *local_interfaces;

#define ALLONES  ((uint32)0xFFFFFFFF)
#ifdef _XBOX
#define MKBCADDR(_IP, _NM) ((uint32)0xFFFFFFFF) // broadcast adres = 255.255.255.255
#else if
#define MKBCADDR(_IP, _NM) ((_IP & _NM) | (_NM ^ ALLONES))
#endif

#define MKNETADDR(_IP, _NM) (_IP & _NM)

/****************************************************************************
Try and find an interface that matches an ip. If we cannot, return NULL
  **************************************************************************/
static struct interface *iface_find(struct in_addr ip, BOOL CheckMask)
{
	struct interface *i;
	if (is_zero_ip(ip)) return local_interfaces;

	for (i=local_interfaces;i;i=i->next)
		if (CheckMask) {
			if (same_net(i->ip,ip,i->nmask)) return i;
		} else if ((i->ip).s_addr == ip.s_addr) return i;

	return NULL;
}


/****************************************************************************
add an interface to the linked list of interfaces
****************************************************************************/
static void add_interface(struct in_addr ip, struct in_addr nmask)
{
	struct interface *iface;
	if (iface_find(ip, False)) {
		DEBUG(3,("not adding duplicate interface %s\n",inet_ntoa(ip)));
		return;
	}

#if !defined(__s390__)
	if (ip_equal(nmask, allones_ip)) {
		DEBUG(3,("not adding non-broadcast interface %s\n",inet_ntoa(ip)));
		return;
	}
#endif

	iface = SMB_MALLOC_P(struct interface);
	if (!iface) return;
	
	ZERO_STRUCTPN(iface);

	iface->ip = ip;
	iface->nmask = nmask;
	iface->bcast.s_addr = MKBCADDR(iface->ip.s_addr, iface->nmask.s_addr);

	DLIST_ADD(local_interfaces, iface);

	DEBUG(2,("added interface ip=%s ",inet_ntoa(iface->ip)));
	DEBUG(2,("bcast=%s ",inet_ntoa(iface->bcast)));
	DEBUG(2,("nmask=%s\n",inet_ntoa(iface->nmask)));	     
}



/****************************************************************************
interpret a single element from a interfaces= config line 

This handles the following different forms:

1) wildcard interface name
2) DNS name
3) IP/masklen
4) ip/mask
5) bcast/mask
****************************************************************************/
static void interpret_interface(char *token)
{
	struct in_addr ip, nmask;
	char *p;
	int i, added=0;

        zero_ip(&ip);
        zero_ip(&nmask);
	
	/* first check if it is an interface name */
	for (i=0;i<total_probed;i++) {
		if (gen_fnmatch(token, probed_ifaces[i].name) == 0) {
			add_interface(probed_ifaces[i].ip,
				      probed_ifaces[i].netmask);
			added = 1;
		}
	}
	if (added) return;

	/* maybe it is a DNS name */
	p = strchr_m(token,'/');
	if (!p) {
		ip = *interpret_addr2(token);
		for (i=0;i<total_probed;i++) {
			if (ip.s_addr == probed_ifaces[i].ip.s_addr &&
			    !ip_equal(allones_ip, probed_ifaces[i].netmask)) {
				add_interface(probed_ifaces[i].ip,
					      probed_ifaces[i].netmask);
				return;
			}
		}
		DEBUG(2,("can't determine netmask for %s\n", token));
		return;
	}

	/* parse it into an IP address/netmasklength pair */
	*p = 0;
	ip = *interpret_addr2(token);
	*p++ = '/';

	if (strlen(p) > 2) {
		nmask = *interpret_addr2(p);
	} else {
		nmask.s_addr = htonl(((ALLONES >> atoi(p)) ^ ALLONES));
	}

	/* maybe the first component was a broadcast address */
	if (ip.s_addr == MKBCADDR(ip.s_addr, nmask.s_addr) ||
	    ip.s_addr == MKNETADDR(ip.s_addr, nmask.s_addr)) {
		for (i=0;i<total_probed;i++) {
			if (same_net(ip, probed_ifaces[i].ip, nmask)) {
				add_interface(probed_ifaces[i].ip, nmask);
				return;
			}
		}
		DEBUG(2,("Can't determine ip for broadcast address %s\n", token));
		return;
	}

	add_interface(ip, nmask);
}


/****************************************************************************
load the list of network interfaces
****************************************************************************/
void load_interfaces(void)
{
	const char **ptr;
	int i;
	struct iface_struct ifaces[MAX_INTERFACES];

	ptr = lp_interfaces();

	allones_ip = *interpret_addr2("255.255.255.255");
	loopback_ip = *interpret_addr2("127.0.0.1");

	SAFE_FREE(probed_ifaces);

	/* dump the current interfaces if any */
	while (local_interfaces) {
		struct interface *iface = local_interfaces;
		DLIST_REMOVE(local_interfaces, local_interfaces);
		ZERO_STRUCTPN(iface);
		SAFE_FREE(iface);
	}

	/* probe the kernel for interfaces */
	total_probed = get_interfaces(ifaces, MAX_INTERFACES);

	if (total_probed > 0) {
		probed_ifaces = memdup(ifaces, sizeof(ifaces[0])*total_probed);
	}

	/* if we don't have a interfaces line then use all broadcast capable 
	   interfaces except loopback */
	if (!ptr || !*ptr || !**ptr) {
		if (total_probed <= 0) {
			DEBUG(0,("ERROR: Could not determine network interfaces, you must use a interfaces config line\n"));
			exit(1);
		}
		for (i=0;i<total_probed;i++) {
			if (
#if !defined(__s390__)
			    probed_ifaces[i].netmask.s_addr != allones_ip.s_addr &&
#endif
			    probed_ifaces[i].ip.s_addr != loopback_ip.s_addr) {
				add_interface(probed_ifaces[i].ip, 
					      probed_ifaces[i].netmask);
			}
		}
		return;
	}

	if (ptr) {
		while (*ptr) {
			char *ptr_cpy = SMB_STRDUP(*ptr);
			if (ptr_cpy) {
				interpret_interface(ptr_cpy);
				free(ptr_cpy);
			}
			ptr++;
		}
	}

	if (!local_interfaces) {
		DEBUG(0,("WARNING: no network interfaces found\n"));
	}
}


/****************************************************************************
return True if the list of probed interfaces has changed
****************************************************************************/
BOOL interfaces_changed(void)
{
	int n;
	struct iface_struct ifaces[MAX_INTERFACES];

	n = get_interfaces(ifaces, MAX_INTERFACES);

	if ((n > 0 )&& (n != total_probed ||
	    memcmp(ifaces, probed_ifaces, sizeof(ifaces[0])*n))) {
		return True;
	}
	
	return False;
}


/****************************************************************************
  check if an IP is one of mine
  **************************************************************************/
BOOL ismyip(struct in_addr ip)
{
	struct interface *i;
	for (i=local_interfaces;i;i=i->next)
		if (ip_equal(i->ip,ip)) return True;
	return False;
}

/****************************************************************************
  check if a packet is from a local (known) net
  **************************************************************************/
BOOL is_local_net(struct in_addr from)
{
	struct interface *i;
	for (i=local_interfaces;i;i=i->next) {
		if((from.s_addr & i->nmask.s_addr) == 
		   (i->ip.s_addr & i->nmask.s_addr))
			return True;
	}
	return False;
}

/****************************************************************************
  how many interfaces do we have
  **************************************************************************/
int iface_count(void)
{
	int ret = 0;
	struct interface *i;

	for (i=local_interfaces;i;i=i->next)
		ret++;
	return ret;
}

/****************************************************************************
  return the Nth interface
  **************************************************************************/
struct interface *get_interface(int n)
{ 
	struct interface *i;
  
	for (i=local_interfaces;i && n;i=i->next)
		n--;

	if (i) return i;
	return NULL;
}

/****************************************************************************
  return IP of the Nth interface
  **************************************************************************/
struct in_addr *iface_n_ip(int n)
{
	struct interface *i;
  
	for (i=local_interfaces;i && n;i=i->next)
		n--;

	if (i) return &i->ip;
	return NULL;
}

/****************************************************************************
  return bcast of the Nth interface
  **************************************************************************/
struct in_addr *iface_n_bcast(int n)
{
	struct interface *i;
  
	for (i=local_interfaces;i && n;i=i->next)
		n--;

	if (i) return &i->bcast;
	return NULL;
}


/* these 3 functions return the ip/bcast/nmask for the interface
   most appropriate for the given ip address. If they can't find
   an appropriate interface they return the requested field of the
   first known interface. */

struct in_addr *iface_ip(struct in_addr ip)
{
	struct interface *i = iface_find(ip, True);
	return(i ? &i->ip : &local_interfaces->ip);
}

/*
  return True if a IP is directly reachable on one of our interfaces
*/
BOOL iface_local(struct in_addr ip)
{
	return iface_find(ip, True) ? True : False;
}
