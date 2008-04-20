/* 
   Unix SMB/CIFS implementation.
   NBT client - used to lookup netbios names
   Copyright (C) Andrew Tridgell 1994-1998
   Copyright (C) Jelmer Vernooij 2003 (Conversion to popt)
   
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

extern BOOL AllowDebugChange;

static BOOL give_flags = False;
static BOOL use_bcast = True;
static BOOL got_bcast = False;
static struct in_addr bcast_addr;
static BOOL recursion_desired = False;
static BOOL translate_addresses = False;
static int ServerFD= -1;
static int RootPort = False;
static BOOL find_status=False;

/****************************************************************************
  open the socket communication
  **************************************************************************/
static BOOL open_sockets(void)
{
  ServerFD = open_socket_in( SOCK_DGRAM,
                             (RootPort ? 137 : 0),
                             (RootPort ?   0 : 3),
                             interpret_addr(lp_socket_address()), True );

  if (ServerFD == -1)
    return(False);

  set_socket_options( ServerFD, "SO_BROADCAST" );

  DEBUG(3, ("Socket opened.\n"));
  return True;
}

/****************************************************************************
turn a node status flags field into a string
****************************************************************************/
static char *node_status_flags(unsigned char flags)
{
	static fstring ret;
	fstrcpy(ret,"");
	
	fstrcat(ret, (flags & 0x80) ? "<GROUP> " : "        ");
	if ((flags & 0x60) == 0x00) fstrcat(ret,"B ");
	if ((flags & 0x60) == 0x20) fstrcat(ret,"P ");
	if ((flags & 0x60) == 0x40) fstrcat(ret,"M ");
	if ((flags & 0x60) == 0x60) fstrcat(ret,"H ");
	if (flags & 0x10) fstrcat(ret,"<DEREGISTERING> ");
	if (flags & 0x08) fstrcat(ret,"<CONFLICT> ");
	if (flags & 0x04) fstrcat(ret,"<ACTIVE> ");
	if (flags & 0x02) fstrcat(ret,"<PERMANENT> ");
	
	return ret;
}

/****************************************************************************
turn the NMB Query flags into a string
****************************************************************************/
static char *query_flags(int flags)
{
	static fstring ret1;
	fstrcpy(ret1, "");

	if (flags & NM_FLAGS_RS) fstrcat(ret1, "Response ");
	if (flags & NM_FLAGS_AA) fstrcat(ret1, "Authoritative ");
	if (flags & NM_FLAGS_TC) fstrcat(ret1, "Truncated ");
	if (flags & NM_FLAGS_RD) fstrcat(ret1, "Recursion_Desired ");
	if (flags & NM_FLAGS_RA) fstrcat(ret1, "Recursion_Available ");
	if (flags & NM_FLAGS_B)  fstrcat(ret1, "Broadcast ");

	return ret1;
}

/****************************************************************************
do a node status query
****************************************************************************/
static void do_node_status(int fd, const char *name, int type, struct in_addr ip)
{
	struct nmb_name nname;
	int count, i, j;
	NODE_STATUS_STRUCT *status;
	struct node_status_extra extra;
	fstring cleanname;

	d_printf("Looking up status of %s\n",inet_ntoa(ip));
	make_nmb_name(&nname, name, type);
	status = node_status_query(fd,&nname,ip, &count, &extra);
	if (status) {
		for (i=0;i<count;i++) {
			pull_ascii_fstring(cleanname, status[i].name);
			for (j=0;cleanname[j];j++) {
				if (!isprint((int)cleanname[j])) cleanname[j] = '.';
			}
			d_printf("\t%-15s <%02x> - %s\n",
			       cleanname,status[i].type,
			       node_status_flags(status[i].flags));
		}
		d_printf("\n\tMAC Address = %02X-%02X-%02X-%02X-%02X-%02X\n",
				extra.mac_addr[0], extra.mac_addr[1],
				extra.mac_addr[2], extra.mac_addr[3],
				extra.mac_addr[4], extra.mac_addr[5]);
		d_printf("\n");
		SAFE_FREE(status);
	} else {
		d_printf("No reply from %s\n\n",inet_ntoa(ip));
	}
}


/****************************************************************************
send out one query
****************************************************************************/
static BOOL query_one(const char *lookup, unsigned int lookup_type)
{
	int j, count, flags = 0;
	struct in_addr *ip_list=NULL;

	if (got_bcast) {
		d_printf("querying %s on %s\n", lookup, inet_ntoa(bcast_addr));
		ip_list = name_query(ServerFD,lookup,lookup_type,use_bcast,
				     use_bcast?True:recursion_desired,
				     bcast_addr,&count, &flags, NULL);
	} else {
		struct in_addr *bcast;
		for (j=iface_count() - 1;
		     !ip_list && j >= 0;
		     j--) {
			bcast = iface_n_bcast(j);
			d_printf("querying %s on %s\n", 
			       lookup, inet_ntoa(*bcast));
			ip_list = name_query(ServerFD,lookup,lookup_type,
					     use_bcast,
					     use_bcast?True:recursion_desired,
					     *bcast,&count, &flags, NULL);
		}
	}

	if (!ip_list) return False;

	if (give_flags)
	  d_printf("Flags: %s\n", query_flags(flags));

	for (j=0;j<count;j++) {
		if (translate_addresses) {
			struct hostent *host = gethostbyaddr((char *)&ip_list[j], sizeof(ip_list[j]), AF_INET);
			if (host) {
				d_printf("%s, ", host -> h_name);
			}
		}
		d_printf("%s %s<%02x>\n",inet_ntoa(ip_list[j]),lookup, lookup_type);
		/* We can only do find_status if the ip address returned
		   was valid - ie. name_query returned true.
		 */
		if (find_status) {
			do_node_status(ServerFD, lookup, lookup_type, ip_list[j]);
		}
	}

	safe_free(ip_list);

	return (ip_list != NULL);
}


/****************************************************************************
  main program
****************************************************************************/
int main(int argc,char *argv[])
{
  int opt;
  unsigned int lookup_type = 0x0;
  fstring lookup;
  static BOOL find_master=False;
  static BOOL lookup_by_ip = False;
  poptContext pc;

  struct poptOption long_options[] = {
	  POPT_AUTOHELP
	  { "broadcast", 'B', POPT_ARG_STRING, NULL, 'B', "Specify address to use for broadcasts", "BROADCAST-ADDRESS" },
	  { "flags", 'f', POPT_ARG_VAL, &give_flags, True, "List the NMB flags returned" },
	  { "unicast", 'U', POPT_ARG_STRING, NULL, 'U', "Specify address to use for unicast" },
	  { "master-browser", 'M', POPT_ARG_VAL, &find_master, True, "Search for a master browser" },
	  { "recursion", 'R', POPT_ARG_VAL, &recursion_desired, True, "Set recursion desired in package" },
	  { "status", 'S', POPT_ARG_VAL, &find_status, True, "Lookup node status as well" },
	  { "translate", 'T', POPT_ARG_NONE, NULL, 'T', "Translate IP addresses into names" },
	  { "root-port", 'r', POPT_ARG_VAL, &RootPort, True, "Use root port 137 (Win95 only replies to this)" },
	  { "lookup-by-ip", 'A', POPT_ARG_VAL, &lookup_by_ip, True, "Do a node status on <name> as an IP Address" },
	  POPT_COMMON_SAMBA
	  POPT_COMMON_CONNECTION
	  { 0, 0, 0, 0 }
  };
	
  *lookup = 0;

  load_case_tables();

  setup_logging(argv[0],True);

  pc = poptGetContext("nmblookup", argc, (const char **)argv, long_options, 
					  POPT_CONTEXT_KEEP_FIRST);

  poptSetOtherOptionHelp(pc, "<NODE> ...");

  while ((opt = poptGetNextOpt(pc)) != -1) {
	  switch (opt) {
	  case 'B':
		  bcast_addr = *interpret_addr2(poptGetOptArg(pc));
		  got_bcast = True;
		  use_bcast = True;
		  break;
	  case 'U':
		  bcast_addr = *interpret_addr2(poptGetOptArg(pc));
		  got_bcast = True;
		  use_bcast = False;
		  break;
	  case 'T':
		  translate_addresses = !translate_addresses;
		  break;
	  }
  }

  poptGetArg(pc); /* Remove argv[0] */

  if(!poptPeekArg(pc)) { 
	  poptPrintUsage(pc, stderr, 0);
	  exit(1);
  }

  if (!lp_load(dyn_CONFIGFILE,True,False,False,True)) {
	  fprintf(stderr, "Can't load %s - run testparm to debug it\n", dyn_CONFIGFILE);
  }

  load_interfaces();
  if (!open_sockets()) return(1);

  while(poptPeekArg(pc))
  {
	  char *p;
	  struct in_addr ip;

	  fstrcpy(lookup,poptGetArg(pc));

	  if(lookup_by_ip)
	  {
		  ip = *interpret_addr2(lookup);
		  fstrcpy(lookup,"*");
		  do_node_status(ServerFD, lookup, lookup_type, ip);
		  continue;
	  }

	  if (find_master) {
		  if (*lookup == '-') {
			  fstrcpy(lookup,"\01\02__MSBROWSE__\02");
			  lookup_type = 1;
		  } else {
			  lookup_type = 0x1d;
		  }
	  }

	  p = strchr_m(lookup,'#');
	  if (p) {
		  *p = '\0';
		  sscanf(++p,"%x",&lookup_type);
	  }

	  if (!query_one(lookup, lookup_type)) {
		  d_printf( "name_query failed to find name %s", lookup );
		  if( 0 != lookup_type )
			  d_printf( "#%02x", lookup_type );
		  d_printf( "\n" );
	  }
  }

  poptFreeContext(pc);

  return(0);
}
