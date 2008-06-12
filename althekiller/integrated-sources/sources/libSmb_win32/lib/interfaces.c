/* 
   Unix SMB/CIFS implementation.
   return a list of network interfaces
   Copyright (C) Andrew Tridgell 1998
   
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


/* working out the interfaces for a OS is an incredibly non-portable
   thing. We have several possible implementations below, and autoconf
   tries each of them to see what works

   Note that this file does _not_ include includes.h. That is so this code
   can be called directly from the autoconf tests. That also means
   this code cannot use any of the normal Samba debug stuff or defines.
   This is standalone code.

*/
#ifdef _XBOX
#ifdef _WIN32
#include <windows.h>
#else
#include <xtl.h>
#endif
#else
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <net/if.h>
#endif

#ifdef AUTOCONF_TEST
struct iface_struct {
	char name[16];
	struct in_addr ip;
	struct in_addr netmask;
};
#else
#include "config.h"
#include "interfaces.h"
#endif

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#ifndef SIOCGIFCONF
#ifdef HAVE_SYS_SOCKIO_H
#include <sys/sockio.h>
#endif
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif

#ifdef __COMPAR_FN_T
#define QSORT_CAST (__compar_fn_t)
#endif

#ifndef QSORT_CAST
#define QSORT_CAST (int (*)(const void *, const void *))
#endif

#if HAVE_IFACE_IFCONF

/* this works for Linux 2.2, Solaris 2.5, SunOS4, HPUX 10.20, OSF1
   V4.0, Ultrix 4.4, SCO Unix 3.2, IRIX 6.4 and FreeBSD 3.2.

   It probably also works on any BSD style system.  */

/****************************************************************************
  get the netmask address for a local interface
****************************************************************************/
static int _get_interfaces(struct iface_struct *ifaces, int max_interfaces)
{  
	struct ifconf ifc;
	char buff[8192];
	int fd, i, n;
	struct ifreq *ifr=NULL;
	int total = 0;
	struct in_addr ipaddr;
	struct in_addr nmask;
	char *iname;

	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		return -1;
	}
  
	ifc.ifc_len = sizeof(buff);
	ifc.ifc_buf = buff;

	if (ioctl(fd, SIOCGIFCONF, &ifc) != 0) {
		close(fd);
		return -1;
	} 

	ifr = ifc.ifc_req;
  
	n = ifc.ifc_len / sizeof(struct ifreq);

	/* Loop through interfaces, looking for given IP address */
	for (i=n-1;i>=0 && total < max_interfaces;i--) {
		if (ioctl(fd, SIOCGIFADDR, &ifr[i]) != 0) {
			continue;
		}

		iname = ifr[i].ifr_name;
		ipaddr = (*(struct sockaddr_in *)&ifr[i].ifr_addr).sin_addr;

		if (ioctl(fd, SIOCGIFFLAGS, &ifr[i]) != 0) {
			continue;
		}  

		if (!(ifr[i].ifr_flags & IFF_UP)) {
			continue;
		}

		if (ioctl(fd, SIOCGIFNETMASK, &ifr[i]) != 0) {
			continue;
		}  

		nmask = ((struct sockaddr_in *)&ifr[i].ifr_addr)->sin_addr;

		strncpy(ifaces[total].name, iname, sizeof(ifaces[total].name)-1);
		ifaces[total].name[sizeof(ifaces[total].name)-1] = 0;
		ifaces[total].ip = ipaddr;
		ifaces[total].netmask = nmask;
		total++;
	}

	close(fd);

	return total;
}  

#elif HAVE_IFACE_IFREQ

#ifndef I_STR
#include <sys/stropts.h>
#endif

/****************************************************************************
this should cover most of the streams based systems
Thanks to Andrej.Borsenkow@mow.siemens.ru for several ideas in this code
****************************************************************************/
static int _get_interfaces(struct iface_struct *ifaces, int max_interfaces)
{
	struct ifreq ifreq;
	struct strioctl strioctl;
	char buff[8192];
	int fd, i, n;
	struct ifreq *ifr=NULL;
	int total = 0;
	struct in_addr ipaddr;
	struct in_addr nmask;
	char *iname;

	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		return -1;
	}
  
	strioctl.ic_cmd = SIOCGIFCONF;
	strioctl.ic_dp  = buff;
	strioctl.ic_len = sizeof(buff);
	if (ioctl(fd, I_STR, &strioctl) < 0) {
		close(fd);
		return -1;
	} 

	/* we can ignore the possible sizeof(int) here as the resulting
	   number of interface structures won't change */
	n = strioctl.ic_len / sizeof(struct ifreq);

	/* we will assume that the kernel returns the length as an int
           at the start of the buffer if the offered size is a
           multiple of the structure size plus an int */
	if (n*sizeof(struct ifreq) + sizeof(int) == strioctl.ic_len) {
		ifr = (struct ifreq *)(buff + sizeof(int));  
	} else {
		ifr = (struct ifreq *)buff;  
	}

	/* Loop through interfaces */

	for (i = 0; i<n && total < max_interfaces; i++) {
		ifreq = ifr[i];
  
		strioctl.ic_cmd = SIOCGIFFLAGS;
		strioctl.ic_dp  = (char *)&ifreq;
		strioctl.ic_len = sizeof(struct ifreq);
		if (ioctl(fd, I_STR, &strioctl) != 0) {
			continue;
		}
		
		if (!(ifreq.ifr_flags & IFF_UP)) {
			continue;
		}

		strioctl.ic_cmd = SIOCGIFADDR;
		strioctl.ic_dp  = (char *)&ifreq;
		strioctl.ic_len = sizeof(struct ifreq);
		if (ioctl(fd, I_STR, &strioctl) != 0) {
			continue;
		}

		ipaddr = (*(struct sockaddr_in *) &ifreq.ifr_addr).sin_addr;
		iname = ifreq.ifr_name;

		strioctl.ic_cmd = SIOCGIFNETMASK;
		strioctl.ic_dp  = (char *)&ifreq;
		strioctl.ic_len = sizeof(struct ifreq);
		if (ioctl(fd, I_STR, &strioctl) != 0) {
			continue;
		}

		nmask = ((struct sockaddr_in *)&ifreq.ifr_addr)->sin_addr;

		strncpy(ifaces[total].name, iname, sizeof(ifaces[total].name)-1);
		ifaces[total].name[sizeof(ifaces[total].name)-1] = 0;
		ifaces[total].ip = ipaddr;
		ifaces[total].netmask = nmask;

		total++;
	}

	close(fd);

	return total;
}

#elif HAVE_IFACE_AIX

/****************************************************************************
this one is for AIX (tested on 4.2)
****************************************************************************/
static int _get_interfaces(struct iface_struct *ifaces, int max_interfaces)
{
	char buff[8192];
	int fd, i;
	struct ifconf ifc;
	struct ifreq *ifr=NULL;
	struct in_addr ipaddr;
	struct in_addr nmask;
	char *iname;
	int total = 0;

	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		return -1;
	}


	ifc.ifc_len = sizeof(buff);
	ifc.ifc_buf = buff;

	if (ioctl(fd, SIOCGIFCONF, &ifc) != 0) {
		close(fd);
		return -1;
	}

	ifr = ifc.ifc_req;

	/* Loop through interfaces */
	i = ifc.ifc_len;

	while (i > 0 && total < max_interfaces) {
		unsigned inc;

		inc = ifr->ifr_addr.sa_len;

		if (ioctl(fd, SIOCGIFADDR, ifr) != 0) {
			goto next;
		}

		ipaddr = (*(struct sockaddr_in *) &ifr->ifr_addr).sin_addr;
		iname = ifr->ifr_name;

		if (ioctl(fd, SIOCGIFFLAGS, ifr) != 0) {
			goto next;
		}

		if (!(ifr->ifr_flags & IFF_UP)) {
			goto next;
		}

		if (ioctl(fd, SIOCGIFNETMASK, ifr) != 0) {
			goto next;
		}

		nmask = ((struct sockaddr_in *)&ifr->ifr_addr)->sin_addr;

		strncpy(ifaces[total].name, iname, sizeof(ifaces[total].name)-1);
		ifaces[total].name[sizeof(ifaces[total].name)-1] = 0;
		ifaces[total].ip = ipaddr;
		ifaces[total].netmask = nmask;

		total++;

	next:
		/*
		 * Patch from Archie Cobbs (archie@whistle.com).  The
		 * addresses in the SIOCGIFCONF interface list have a
		 * minimum size. Usually this doesn't matter, but if
		 * your machine has tunnel interfaces, etc. that have
		 * a zero length "link address", this does matter.  */

		if (inc < sizeof(ifr->ifr_addr))
			inc = sizeof(ifr->ifr_addr);
		inc += IFNAMSIZ;

		ifr = (struct ifreq*) (((char*) ifr) + inc);
		i -= inc;
	}
  

	close(fd);
	return total;
}

#elif _XBOX

static char smb_xbox_interface_ip[25];
static char smb_xbox_interface_subnet[25];

void set_xbox_interface(char* ip, char* subnet)
{
	strcpy(smb_xbox_interface_ip, ip);
	strcpy(smb_xbox_interface_subnet, subnet);
}

static int _get_interfaces(struct iface_struct *ifaces, int max_interfaces)
{ 
	struct in_addr ipaddr;
	struct in_addr nmask;
	int total = 0;
	char *iname;

	/* Loop through interfaces, looking for given IP address */
	iname = "eth0";
	ipaddr.S_un.S_addr = inet_addr(smb_xbox_interface_ip); // fill ip address
	nmask.S_un.S_addr = inet_addr(smb_xbox_interface_subnet);  // fill subnet mask

	strncpy(ifaces[total].name, iname, sizeof(ifaces[total].name)-1);
	ifaces[total].name[sizeof(ifaces[total].name)-1] = 0;
	ifaces[total].ip = ipaddr;
	ifaces[total].netmask = nmask;
	total++;

	return total;
}

#else /* a dummy version */
static int _get_interfaces(struct iface_struct *ifaces, int max_interfaces)
{
	return -1;
}
#endif


static int iface_comp(struct iface_struct *i1, struct iface_struct *i2)
{
	int r;
	r = strcmp(i1->name, i2->name);
	if (r) return r;
	r = ntohl(i1->ip.s_addr) - ntohl(i2->ip.s_addr);
	if (r) return r;
	r = ntohl(i1->netmask.s_addr) - ntohl(i2->netmask.s_addr);
	return r;
}

/* this wrapper is used to remove duplicates from the interface list generated
   above */
int get_interfaces(struct iface_struct *ifaces, int max_interfaces);

int get_interfaces(struct iface_struct *ifaces, int max_interfaces)
{
	int total, i, j;

	total = _get_interfaces(ifaces, max_interfaces);
	if (total <= 0) return total;

	/* now we need to remove duplicates */
	qsort(ifaces, total, sizeof(ifaces[0]), QSORT_CAST iface_comp);

	for (i=1;i<total;) {
		if (iface_comp(&ifaces[i-1], &ifaces[i]) == 0) {
			for (j=i-1;j<total-1;j++) {
				ifaces[j] = ifaces[j+1];
			}
			total--;
		} else {
			i++;
		}
	}

	return total;
}


#ifdef AUTOCONF_TEST
/* this is the autoconf driver to test get_interfaces() */

#define MAX_INTERFACES 128

 int main()
{
	struct iface_struct ifaces[MAX_INTERFACES];
	int total = get_interfaces(ifaces, MAX_INTERFACES);
	int i;

	printf("got %d interfaces:\n", total);
	if (total <= 0) exit(1);

	for (i=0;i<total;i++) {
		printf("%-10s ", ifaces[i].name);
		printf("IP=%s ", inet_ntoa(ifaces[i].ip));
		printf("NETMASK=%s\n", inet_ntoa(ifaces[i].netmask));
	}
	return 0;
}
#endif
