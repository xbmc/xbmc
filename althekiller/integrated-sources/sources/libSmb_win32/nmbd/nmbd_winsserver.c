/* 
   Unix SMB/CIFS implementation.
   NBT netbios routines and daemon - version 2

   Copyright (C) Jeremy Allison 1994-2005

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
   
   Converted to store WINS data in a tdb. Dec 2005. JRA.
*/

#include "includes.h"

#define WINS_LIST "wins.dat"
#define WINS_VERSION 1
#define WINSDB_VERSION 1

/****************************************************************************
 We don't store the NetBIOS scope in the wins.tdb. We key off the (utf8) netbios
 name (65 bytes with the last byte being the name type).
*****************************************************************************/

TDB_CONTEXT *wins_tdb;

/****************************************************************************
 Delete all the temporary name records on the in-memory linked list.
*****************************************************************************/

static void wins_delete_all_tmp_in_memory_records(void)
{
	struct name_record *nr = NULL;
	struct name_record *nrnext = NULL;

	/* Delete all temporary name records on the wins subnet linked list. */
	for( nr = wins_server_subnet->namelist; nr; nr = nrnext) {
		nrnext = nr->next;
		DLIST_REMOVE(wins_server_subnet->namelist, nr);
		SAFE_FREE(nr->data.ip);
		SAFE_FREE(nr);
	}
}

/****************************************************************************
 Convert a wins.tdb record to a struct name_record. Add in our global_scope().
*****************************************************************************/

static struct name_record *wins_record_to_name_record(TDB_DATA key, TDB_DATA data)
{
	struct name_record *namerec = NULL;
	uint16 nb_flags;
	unsigned char nr_src;
	uint32 death_time, refresh_time;
	uint32 id_low, id_high;
	uint32 saddr;
	uint32 wins_flags;
	uint32 num_ips;
	size_t len;
	int i;

	if (data.dptr == NULL || data.dsize == 0) {
		return NULL;
	}

	/* Min size is "wbddddddd" + 1 ip address (4). */
	if (data.dsize < 2 + 1 + (7*4) + 4) {
		return NULL;
	}

	len = tdb_unpack(data.dptr, data.dsize,
			"wbddddddd",
                        &nb_flags,
                        &nr_src,
                        &death_time,
                        &refresh_time,
                        &id_low,
                        &id_high,
                        &saddr,
                        &wins_flags,
                        &num_ips );

	namerec = SMB_MALLOC_P(struct name_record);
	if (!namerec) {
		return NULL;
	}
	ZERO_STRUCTP(namerec);

	namerec->data.ip = SMB_MALLOC_ARRAY(struct in_addr, num_ips);
	if (!namerec->data.ip) {
		SAFE_FREE(namerec);
		return NULL;
	}

	namerec->subnet = wins_server_subnet;
	push_ascii_nstring(namerec->name.name, key.dptr);
	namerec->name.name_type = key.dptr[sizeof(unstring)];
	/* Add the scope. */
	push_ascii(namerec->name.scope, global_scope(), 64, STR_TERMINATE);

        /* We're using a byte-by-byte compare, so we must be sure that
         * unused space doesn't have garbage in it.
         */
                                                                                                                               
        for( i = strlen( namerec->name.name ); i < sizeof( namerec->name.name ); i++ ) {
                namerec->name.name[i] = '\0';
        }
        for( i = strlen( namerec->name.scope ); i < sizeof( namerec->name.scope ); i++ ) {
                namerec->name.scope[i] = '\0';
        }

	namerec->data.nb_flags = nb_flags;
	namerec->data.source = (enum name_source)nr_src;
	namerec->data.death_time = (time_t)death_time;
	namerec->data.refresh_time = (time_t)refresh_time;
	namerec->data.id = id_low;
#if defined(HAVE_LONGLONG)
	namerec->data.id |= ((SMB_BIG_UINT)id_high << 32);
#endif
	namerec->data.wins_ip.s_addr = saddr;
	namerec->data.wins_flags = wins_flags,
	namerec->data.num_ips = num_ips;

	for (i = 0; i < num_ips; i++) {
		namerec->data.ip[i].s_addr = IVAL(data.dptr, len + (i*4));
	}

	return namerec;
}

/****************************************************************************
 Convert a struct name_record to a wins.tdb record. Ignore the scope.
*****************************************************************************/

static TDB_DATA name_record_to_wins_record(const struct name_record *namerec)
{
	TDB_DATA data;
	size_t len = 0;
	int i;
	uint32 id_low = (namerec->data.id & 0xFFFFFFFF);
#if defined(HAVE_LONGLONG)
	uint32 id_high = (namerec->data.id >> 32) & 0xFFFFFFFF;
#else
	uint32 id_high = 0;
#endif

	ZERO_STRUCT(data);

	len = (2 + 1 + (7*4)); /* "wbddddddd" */
	len += (namerec->data.num_ips * 4);

	data.dptr = SMB_MALLOC(len);
	if (!data.dptr) {
		return data;
	}
	data.dsize = len;

	len = tdb_pack(data.dptr, data.dsize, "wbddddddd",
                        namerec->data.nb_flags,
                        (unsigned char)namerec->data.source,
                        (uint32)namerec->data.death_time,
                        (uint32)namerec->data.refresh_time,
                        id_low,
                        id_high,
                        (uint32)namerec->data.wins_ip.s_addr,
                        (uint32)namerec->data.wins_flags,
                        (uint32)namerec->data.num_ips );

	for (i = 0; i < namerec->data.num_ips; i++) {
		SIVAL(data.dptr, len + (i*4), namerec->data.ip[i].s_addr);
	}

	return data;
}

/****************************************************************************
 Create key. Key is UNIX codepage namestring (usually utf8 64 byte len) with 1 byte type.
*****************************************************************************/

static TDB_DATA name_to_key(const struct nmb_name *nmbname)
{
	static char keydata[sizeof(unstring) + 1];
	TDB_DATA key;

	memset(keydata, '\0', sizeof(keydata));

	pull_ascii_nstring(keydata, sizeof(unstring), nmbname->name);
	strupper_m(keydata);
	keydata[sizeof(unstring)] = nmbname->name_type;
	key.dptr = keydata;
	key.dsize = sizeof(keydata);

	return key;
}

/****************************************************************************
 Lookup a given name in the wins.tdb and create a temporary malloc'ed data struct
 on the linked list. We will free this later in XXXX().
*****************************************************************************/

struct name_record *find_name_on_wins_subnet(const struct nmb_name *nmbname, BOOL self_only)
{
	TDB_DATA data, key;
	struct name_record *nr = NULL;
	struct name_record *namerec = NULL;

	if (!wins_tdb) {
		return NULL;
	}

	key = name_to_key(nmbname);
	data = tdb_fetch(wins_tdb, key);

	if (data.dsize == 0) {
		return NULL;
	}

	namerec = wins_record_to_name_record(key, data);

	/* done with the this */

	SAFE_FREE( data.dptr );

	if (!namerec) {
		return NULL;
	}

	/* Self names only - these include permanent names. */
	if( self_only && (namerec->data.source != SELF_NAME) && (namerec->data.source != PERMANENT_NAME) ) {
		DEBUG( 9, ( "find_name_on_wins_subnet: self name %s NOT FOUND\n", nmb_namestr(nmbname) ) );
		SAFE_FREE(namerec->data.ip);
		SAFE_FREE(namerec);
		return NULL;
	}

	/* Search for this name record on the list. Replace it if found. */

	for( nr = wins_server_subnet->namelist; nr; nr = nr->next) {
		if (memcmp(nmbname->name, nr->name.name, 16) == 0) {
			/* Delete it. */
			DLIST_REMOVE(wins_server_subnet->namelist, nr);
			SAFE_FREE(nr->data.ip);
			SAFE_FREE(nr);
			break;
		}
	}
	
	DLIST_ADD(wins_server_subnet->namelist, namerec);
	return namerec;
}

/****************************************************************************
 Overwrite or add a given name in the wins.tdb.
*****************************************************************************/

static BOOL store_or_replace_wins_namerec(const struct name_record *namerec, int tdb_flag)
{
	TDB_DATA key, data;
	int ret;

	if (!wins_tdb) {
		return False;
	}

	key = name_to_key(&namerec->name);
	data = name_record_to_wins_record(namerec);

	if (data.dptr == NULL) {
		return False;
	}

	ret = tdb_store(wins_tdb, key, data, tdb_flag);

	SAFE_FREE(data.dptr);
	return (ret == 0) ? True : False;
}

/****************************************************************************
 Overwrite a given name in the wins.tdb.
*****************************************************************************/

BOOL wins_store_changed_namerec(const struct name_record *namerec)
{
	return store_or_replace_wins_namerec(namerec, TDB_REPLACE);
}

/****************************************************************************
 Primary interface into creating and overwriting records in the wins.tdb.
*****************************************************************************/

BOOL add_name_to_wins_subnet(const struct name_record *namerec)
{
	return store_or_replace_wins_namerec(namerec, TDB_INSERT);
}

/****************************************************************************
 Delete a given name in the tdb and remove the temporary malloc'ed data struct
 on the linked list.
*****************************************************************************/

BOOL remove_name_from_wins_namelist(struct name_record *namerec)
{
	TDB_DATA key;
	int ret;

	if (!wins_tdb) {
		return False;
	}

	key = name_to_key(&namerec->name);
	ret = tdb_delete(wins_tdb, key);

	DLIST_REMOVE(wins_server_subnet->namelist, namerec);

	/* namerec must be freed by the caller */

	return (ret == 0) ? True : False;
}

/****************************************************************************
 Dump out the complete namelist.
*****************************************************************************/

static int traverse_fn(TDB_CONTEXT *tdb, TDB_DATA kbuf, TDB_DATA dbuf, void *state)
{
	struct name_record *namerec = NULL;
	XFILE *fp = (XFILE *)state;

	if (kbuf.dsize != sizeof(unstring) + 1) {
		return 0;
	}

	namerec = wins_record_to_name_record(kbuf, dbuf);
	if (!namerec) {
		return 0;
	}

	dump_name_record(namerec, fp);

	SAFE_FREE(namerec->data.ip);
	SAFE_FREE(namerec);
	return 0;
}

void dump_wins_subnet_namelist(XFILE *fp)
{
	tdb_traverse(wins_tdb, traverse_fn, (void *)fp);
}

/****************************************************************************
 Change the wins owner address in the record.
*****************************************************************************/

static void update_wins_owner(struct name_record *namerec, struct in_addr wins_ip)
{
	namerec->data.wins_ip=wins_ip;
}

/****************************************************************************
 Create the wins flags based on the nb flags and the input value.
*****************************************************************************/

static void update_wins_flag(struct name_record *namerec, int flags)
{
	namerec->data.wins_flags=0x0;

	/* if it's a group, it can be a normal or a special one */
	if (namerec->data.nb_flags & NB_GROUP) {
		if (namerec->name.name_type==0x1C) {
			namerec->data.wins_flags|=WINS_SGROUP;
		} else {
			if (namerec->data.num_ips>1) {
				namerec->data.wins_flags|=WINS_SGROUP;
			} else {
				namerec->data.wins_flags|=WINS_NGROUP;
			}
		}
	} else {
		/* can be unique or multi-homed */
		if (namerec->data.num_ips>1) {
			namerec->data.wins_flags|=WINS_MHOMED;
		} else {
			namerec->data.wins_flags|=WINS_UNIQUE;
		}
	}

	/* the node type are the same bits */
	namerec->data.wins_flags|=namerec->data.nb_flags&NB_NODETYPEMASK;

	/* the static bit is elsewhere */
	if (namerec->data.death_time == PERMANENT_TTL) {
		namerec->data.wins_flags|=WINS_STATIC;
	}

	/* and add the given bits */
	namerec->data.wins_flags|=flags;

	DEBUG(8,("update_wins_flag: nbflags: 0x%x, ttl: 0x%d, flags: 0x%x, winsflags: 0x%x\n", 
		 namerec->data.nb_flags, (int)namerec->data.death_time, flags, namerec->data.wins_flags));
}

/****************************************************************************
 Return the general ID value and increase it if requested.
*****************************************************************************/

static void get_global_id_and_update(SMB_BIG_UINT *current_id, BOOL update)
{
	/*
	 * it's kept as a static here, to prevent people from messing
	 * with the value directly
	 */

	static SMB_BIG_UINT general_id = 1;

	DEBUG(5,("get_global_id_and_update: updating version ID: %d\n", (int)general_id));
	
	*current_id = general_id;
	
	if (update) {
		general_id++;
	}
}

/****************************************************************************
 Possibly call the WINS hook external program when a WINS change is made.
 Also stores the changed record back in the wins_tdb.
*****************************************************************************/

static void wins_hook(const char *operation, struct name_record *namerec, int ttl)
{
	pstring command;
	char *cmd = lp_wins_hook();
	char *p, *namestr;
	int i;

	wins_store_changed_namerec(namerec);

	if (!cmd || !*cmd) {
		return;
	}

	for (p=namerec->name.name; *p; p++) {
		if (!(isalnum((int)*p) || strchr_m("._-",*p))) {
			DEBUG(3,("not calling wins hook for invalid name %s\n", nmb_namestr(&namerec->name)));
			return;
		}
	}
	
	/* Use the name without the nametype (and scope) appended */

	namestr = nmb_namestr(&namerec->name);
	if ((p = strchr(namestr, '<'))) {
		*p = 0;
	}

	p = command;
	p += slprintf(p, sizeof(command)-1, "%s %s %s %02x %d", 
		      cmd,
		      operation, 
		      namestr,
		      namerec->name.name_type,
		      ttl);

	for (i=0;i<namerec->data.num_ips;i++) {
		p += slprintf(p, sizeof(command) - (p-command) -1, " %s", inet_ntoa(namerec->data.ip[i]));
	}

	DEBUG(3,("calling wins hook for %s\n", nmb_namestr(&namerec->name)));
	smbrun(command, NULL);
}

/****************************************************************************
Determine if this packet should be allocated to the WINS server.
*****************************************************************************/

BOOL packet_is_for_wins_server(struct packet_struct *packet)
{
	struct nmb_packet *nmb = &packet->packet.nmb;

	/* Only unicast packets go to a WINS server. */
	if((wins_server_subnet == NULL) || (nmb->header.nm_flags.bcast == True)) {
		DEBUG(10, ("packet_is_for_wins_server: failing WINS test #1.\n"));
		return False;
	}

	/* Check for node status requests. */
	if (nmb->question.question_type != QUESTION_TYPE_NB_QUERY) {
		return False;
	}

	switch(nmb->header.opcode) { 
		/*
		 * A WINS server issues WACKS, not receives them.
		 */
		case NMB_WACK_OPCODE:
			DEBUG(10, ("packet_is_for_wins_server: failing WINS test #2 (WACK).\n"));
			return False;
		/*
		 * A WINS server only processes registration and
		 * release requests, not responses.
		 */
		case NMB_NAME_REG_OPCODE:
		case NMB_NAME_MULTIHOMED_REG_OPCODE:
		case NMB_NAME_REFRESH_OPCODE_8: /* ambiguity in rfc1002 about which is correct. */
		case NMB_NAME_REFRESH_OPCODE_9: /* WinNT uses 8 by default. */
			if(nmb->header.response) {
				DEBUG(10, ("packet_is_for_wins_server: failing WINS test #3 (response = 1).\n"));
				return False;
			}
			break;

		case NMB_NAME_RELEASE_OPCODE:
			if(nmb->header.response) {
				DEBUG(10, ("packet_is_for_wins_server: failing WINS test #4 (response = 1).\n"));
				return False;
			}
			break;

		/*
		 * Only process unicast name queries with rd = 1.
		 */
		case NMB_NAME_QUERY_OPCODE:
			if(!nmb->header.response && !nmb->header.nm_flags.recursion_desired) {
				DEBUG(10, ("packet_is_for_wins_server: failing WINS test #5 (response = 1).\n"));
				return False;
			}
			break;
	}

	return True;
}

/****************************************************************************
Utility function to decide what ttl to give a register/refresh request.
*****************************************************************************/

static int get_ttl_from_packet(struct nmb_packet *nmb)
{
	int ttl = nmb->additional->ttl;

	if (ttl < lp_min_wins_ttl()) {
		ttl = lp_min_wins_ttl();
	}

	if (ttl > lp_max_wins_ttl()) {
		ttl = lp_max_wins_ttl();
	}

	return ttl;
}

/****************************************************************************
Load or create the WINS database.
*****************************************************************************/

BOOL initialise_wins(void)
{
	time_t time_now = time(NULL);
	XFILE *fp;
	pstring line;

	if(!lp_we_are_a_wins_server()) {
		return True;
	}

	/* Open the wins.tdb. */
	wins_tdb = tdb_open_log(lock_path("wins.tdb"), 0, TDB_DEFAULT|TDB_CLEAR_IF_FIRST, O_CREAT|O_RDWR, 0600);
	if (!wins_tdb) {
		DEBUG(0,("initialise_wins: failed to open wins.tdb. Error was %s\n",
			strerror(errno) ));
		return False;
	}

	tdb_store_int32(wins_tdb, "WINSDB_VERSION", WINSDB_VERSION);

	add_samba_names_to_subnet(wins_server_subnet);

	if((fp = x_fopen(lock_path(WINS_LIST),O_RDONLY,0)) == NULL) {
		DEBUG(2,("initialise_wins: Can't open wins database file %s. Error was %s\n",
			WINS_LIST, strerror(errno) ));
		return True;
	}

	while (!x_feof(fp)) {
		pstring name_str, ip_str, ttl_str, nb_flags_str;
		unsigned int num_ips;
		pstring name;
		struct in_addr *ip_list;
		int type = 0;
		int nb_flags;
		int ttl;
		const char *ptr;
		char *p;
		BOOL got_token;
		BOOL was_ip;
		int i;
		unsigned int hash;
		int version;

		/* Read a line from the wins.dat file. Strips whitespace
			from the beginning and end of the line.  */
		if (!fgets_slash(line,sizeof(pstring),fp))
			continue;
      
		if (*line == '#')
			continue;

		if (strncmp(line,"VERSION ", 8) == 0) {
			if (sscanf(line,"VERSION %d %u", &version, &hash) != 2 ||
						version != WINS_VERSION) {
				DEBUG(0,("Discarding invalid wins.dat file [%s]\n",line));
				x_fclose(fp);
				return True;
			}
			continue;
		}

		ptr = line;

		/* 
		 * Now we handle multiple IP addresses per name we need
		 * to iterate over the line twice. The first time to
		 * determine how many IP addresses there are, the second
		 * time to actually parse them into the ip_list array.
		 */

		if (!next_token(&ptr,name_str,NULL,sizeof(name_str))) {
			DEBUG(0,("initialise_wins: Failed to parse name when parsing line %s\n", line ));
			continue;
		}

		if (!next_token(&ptr,ttl_str,NULL,sizeof(ttl_str))) {
			DEBUG(0,("initialise_wins: Failed to parse time to live when parsing line %s\n", line ));
			continue;
		}

		/*
		 * Determine the number of IP addresses per line.
		 */
		num_ips = 0;
		do {
			got_token = next_token(&ptr,ip_str,NULL,sizeof(ip_str));
			was_ip = False;

			if(got_token && strchr(ip_str, '.')) {
				num_ips++;
				was_ip = True;
			}
		} while( got_token && was_ip);

		if(num_ips == 0) {
			DEBUG(0,("initialise_wins: Missing IP address when parsing line %s\n", line ));
			continue;
		}

		if(!got_token) {
			DEBUG(0,("initialise_wins: Missing nb_flags when parsing line %s\n", line ));
			continue;
		}

		/* Allocate the space for the ip_list. */
		if((ip_list = SMB_MALLOC_ARRAY( struct in_addr, num_ips)) == NULL) {
			DEBUG(0,("initialise_wins: Malloc fail !\n"));
			x_fclose(fp);
			return False;
		}
 
		/* Reset and re-parse the line. */
		ptr = line;
		next_token(&ptr,name_str,NULL,sizeof(name_str)); 
		next_token(&ptr,ttl_str,NULL,sizeof(ttl_str));
		for(i = 0; i < num_ips; i++) {
			next_token(&ptr, ip_str, NULL, sizeof(ip_str));
			ip_list[i] = *interpret_addr2(ip_str);
		}
		next_token(&ptr,nb_flags_str,NULL, sizeof(nb_flags_str));

		/* 
		 * Deal with SELF or REGISTER name encoding. Default is REGISTER
		 * for compatibility with old nmbds.
		 */

		if(nb_flags_str[strlen(nb_flags_str)-1] == 'S') {
			DEBUG(5,("initialise_wins: Ignoring SELF name %s\n", line));
			SAFE_FREE(ip_list);
			continue;
		}
      
		if(nb_flags_str[strlen(nb_flags_str)-1] == 'R') {
			nb_flags_str[strlen(nb_flags_str)-1] = '\0';
		}
      
		/* Netbios name. # divides the name from the type (hex): netbios#xx */
		pstrcpy(name,name_str);
      
		if((p = strchr(name,'#')) != NULL) {
			*p = 0;
			sscanf(p+1,"%x",&type);
		}
      
		/* Decode the netbios flags (hex) and the time-to-live (in seconds). */
		sscanf(nb_flags_str,"%x",&nb_flags);
		sscanf(ttl_str,"%d",&ttl);

		/* add all entries that have 60 seconds or more to live */
		if ((ttl - 60) > time_now || ttl == PERMANENT_TTL) {
			if(ttl != PERMANENT_TTL) {
				ttl -= time_now;
			}
    
			DEBUG( 4, ("initialise_wins: add name: %s#%02x ttl = %d first IP %s flags = %2x\n",
				name, type, ttl, inet_ntoa(ip_list[0]), nb_flags));

			(void)add_name_to_subnet( wins_server_subnet, name, type, nb_flags, 
					ttl, REGISTER_NAME, num_ips, ip_list );
		} else {
			DEBUG(4, ("initialise_wins: not adding name (ttl problem) "
				"%s#%02x ttl = %d first IP %s flags = %2x\n",
				name, type, ttl, inet_ntoa(ip_list[0]), nb_flags));
		}

		SAFE_FREE(ip_list);
	} 
    
	x_fclose(fp);
	return True;
}

/****************************************************************************
Send a WINS WACK (Wait ACKnowledgement) response.
**************************************************************************/

static void send_wins_wack_response(int ttl, struct packet_struct *p)
{
	struct nmb_packet *nmb = &p->packet.nmb;
	unsigned char rdata[2];

	rdata[0] = rdata[1] = 0;

	/* Taken from nmblib.c - we need to send back almost
		identical bytes from the requesting packet header. */

	rdata[0] = (nmb->header.opcode & 0xF) << 3;
	if (nmb->header.nm_flags.authoritative && nmb->header.response) {
		rdata[0] |= 0x4;
	}
	if (nmb->header.nm_flags.trunc) {
		rdata[0] |= 0x2;
	}
	if (nmb->header.nm_flags.recursion_desired) {
		rdata[0] |= 0x1;
	}
	if (nmb->header.nm_flags.recursion_available && nmb->header.response) {
		rdata[1] |= 0x80;
	}
	if (nmb->header.nm_flags.bcast) {
		rdata[1] |= 0x10;
	}

	reply_netbios_packet(p,                                /* Packet to reply to. */
				0,                             /* Result code. */
				NMB_WAIT_ACK,                  /* nmbd type code. */
				NMB_WACK_OPCODE,               /* opcode. */
				ttl,                           /* ttl. */
				(char *)rdata,                 /* data to send. */
				2);                            /* data length. */
}

/****************************************************************************
Send a WINS name registration response.
**************************************************************************/

static void send_wins_name_registration_response(int rcode, int ttl, struct packet_struct *p)
{
	struct nmb_packet *nmb = &p->packet.nmb;
	char rdata[6];

	memcpy(&rdata[0], &nmb->additional->rdata[0], 6);

	reply_netbios_packet(p,                                /* Packet to reply to. */
				rcode,                         /* Result code. */
				WINS_REG,                      /* nmbd type code. */
				NMB_NAME_REG_OPCODE,           /* opcode. */
				ttl,                           /* ttl. */
				rdata,                         /* data to send. */
				6);                            /* data length. */
}

/***********************************************************************
 Deal with a name refresh request to a WINS server.
************************************************************************/

void wins_process_name_refresh_request( struct subnet_record *subrec,
                                        struct packet_struct *p )
{
	struct nmb_packet *nmb = &p->packet.nmb;
	struct nmb_name *question = &nmb->question.question_name;
	BOOL bcast = nmb->header.nm_flags.bcast;
	uint16 nb_flags = get_nb_flags(nmb->additional->rdata);
	BOOL group = (nb_flags & NB_GROUP) ? True : False;
	struct name_record *namerec = NULL;
	int ttl = get_ttl_from_packet(nmb);
	struct in_addr from_ip;
	struct in_addr our_fake_ip = *interpret_addr2("0.0.0.0");

	putip( (char *)&from_ip, &nmb->additional->rdata[2] );

	if(bcast) {
		/*
		 * We should only get unicast name refresh packets here.
		 * Anyone trying to refresh broadcast should not be going
		 * to a WINS server.  Log an error here.
		 */
		if( DEBUGLVL( 0 ) ) {
			dbgtext( "wins_process_name_refresh_request: " );
			dbgtext( "Broadcast name refresh request received " );
			dbgtext( "for name %s ", nmb_namestr(question) );
			dbgtext( "from IP %s ", inet_ntoa(from_ip) );
			dbgtext( "on subnet %s.  ", subrec->subnet_name );
			dbgtext( "Error - Broadcasts should not be sent " );
			dbgtext( "to a WINS server\n" );
		}
		return;
	}

	if( DEBUGLVL( 3 ) ) {
		dbgtext( "wins_process_name_refresh_request: " );
		dbgtext( "Name refresh for name %s IP %s\n",
			 nmb_namestr(question), inet_ntoa(from_ip) );
	}

	/* 
	 * See if the name already exists.
	 * If not, handle it as a name registration and return.
	 */
	namerec = find_name_on_subnet(subrec, question, FIND_ANY_NAME);

	/*
	 * If this is a refresh request and the name doesn't exist then
	 * treat it like a registration request. This allows us to recover 
	 * from errors (tridge)
	 */
	if(namerec == NULL) {
		if( DEBUGLVL( 3 ) ) {
			dbgtext( "wins_process_name_refresh_request: " );
			dbgtext( "Name refresh for name %s ",
				 nmb_namestr( question ) );
			dbgtext( "and the name does not exist.  Treating " );
			dbgtext( "as registration.\n" );
		}
		wins_process_name_registration_request(subrec,p);
		return;
	}

	/*
	 * if the name is present but not active, simply remove it
	 * and treat the refresh request as a registration & return.
	 */
	if (namerec != NULL && !WINS_STATE_ACTIVE(namerec)) {
		if( DEBUGLVL( 5 ) ) {
			dbgtext( "wins_process_name_refresh_request: " );
			dbgtext( "Name (%s) in WINS ", nmb_namestr(question) );
			dbgtext( "was not active - removing it.\n" );
		}
		remove_name_from_namelist( subrec, namerec );
		namerec = NULL;
		wins_process_name_registration_request( subrec, p );
		return;
	}

	/*
	 * Check that the group bits for the refreshing name and the
	 * name in our database match.  If not, refuse the refresh.
	 * [crh:  Why RFS_ERR instead of ACT_ERR? Is this what MS does?]
	 */
	if( (namerec != NULL) &&
	    ( (group && !NAME_GROUP(namerec))
	   || (!group && NAME_GROUP(namerec)) ) ) {
		if( DEBUGLVL( 3 ) ) {
			dbgtext( "wins_process_name_refresh_request: " );
			dbgtext( "Name %s ", nmb_namestr(question) );
			dbgtext( "group bit = %s does not match ",
				 group ? "True" : "False" );
			dbgtext( "group bit in WINS for this name.\n" );
		}
		send_wins_name_registration_response(RFS_ERR, 0, p);
		return;
	}

	/*
	 * For a unique name check that the person refreshing the name is
	 * one of the registered IP addresses. If not - fail the refresh.
	 * Do the same for group names with a type of 0x1c.
	 * Just return success for unique 0x1d refreshes. For normal group
	 * names update the ttl and return success.
	 */
	if( (!group || (group && (question->name_type == 0x1c)))
			&& find_ip_in_name_record(namerec, from_ip) ) {
		/*
		 * Update the ttl.
		 */
		update_name_ttl(namerec, ttl);

		/*
		 * if the record is a replica:
		 * we take ownership and update the version ID.
		 */
		if (!ip_equal(namerec->data.wins_ip, our_fake_ip)) {
			update_wins_owner(namerec, our_fake_ip);
			get_global_id_and_update(&namerec->data.id, True);
		}

		send_wins_name_registration_response(0, ttl, p);
		wins_hook("refresh", namerec, ttl);
		return;
	} else if((group && (question->name_type == 0x1c))) {
		/*
		 * Added by crh for bug #1079.
		 * Fix from Bert Driehuis
		 */
		if( DEBUGLVL( 3 ) ) {
			dbgtext( "wins_process_name_refresh_request: " );
			dbgtext( "Name refresh for name %s, ",
				 nmb_namestr(question) );
			dbgtext( "but IP address %s ", inet_ntoa(from_ip) );
			dbgtext( "is not yet associated with " );
			dbgtext( "that name. Treating as registration.\n" );
		}
		wins_process_name_registration_request(subrec,p);
		return;
	} else if(group) {
		/* 
		 * Normal groups are all registered with an IP address of
		 * 255.255.255.255  so we can't search for the IP address.
	 	 */
		update_name_ttl(namerec, ttl);
		wins_hook("refresh", namerec, ttl);
		send_wins_name_registration_response(0, ttl, p);
		return;
	} else if(!group && (question->name_type == 0x1d)) {
		/*
		 * Special name type - just pretend the refresh succeeded.
		 */
		send_wins_name_registration_response(0, ttl, p);
		return;
	} else {
		/*
		 * Fail the refresh.
		 */
		if( DEBUGLVL( 3 ) ) {
			dbgtext( "wins_process_name_refresh_request: " );
			dbgtext( "Name refresh for name %s with IP %s ",
				 nmb_namestr(question), inet_ntoa(from_ip) );
			dbgtext( "and is IP is not known to the name.\n" );
		}
		send_wins_name_registration_response(RFS_ERR, 0, p);
		return;
	}
}

/***********************************************************************
 Deal with a name registration request query success to a client that
 owned the name.

 We have a locked pointer to the original packet stashed away in the
 userdata pointer. The success here is actually a failure as it means
 the client we queried wants to keep the name, so we must return
 a registration failure to the original requestor.
************************************************************************/

static void wins_register_query_success(struct subnet_record *subrec,
                                             struct userdata_struct *userdata,
                                             struct nmb_name *question_name,
                                             struct in_addr ip,
                                             struct res_rec *answers)
{
	struct packet_struct *orig_reg_packet;

	memcpy((char *)&orig_reg_packet, userdata->data, sizeof(struct packet_struct *));

	DEBUG(3,("wins_register_query_success: Original client at IP %s still wants the \
name %s. Rejecting registration request.\n", inet_ntoa(ip), nmb_namestr(question_name) ));

	send_wins_name_registration_response(RFS_ERR, 0, orig_reg_packet);

	orig_reg_packet->locked = False;
	free_packet(orig_reg_packet);
}

/***********************************************************************
 Deal with a name registration request query failure to a client that
 owned the name.

 We have a locked pointer to the original packet stashed away in the
 userdata pointer. The failure here is actually a success as it means
 the client we queried didn't want to keep the name, so we can remove
 the old name record and then successfully add the new name.
************************************************************************/

static void wins_register_query_fail(struct subnet_record *subrec,
                                          struct response_record *rrec,
                                          struct nmb_name *question_name,
                                          int rcode)
{
	struct userdata_struct *userdata = rrec->userdata;
	struct packet_struct *orig_reg_packet;
	struct name_record *namerec = NULL;

	memcpy((char *)&orig_reg_packet, userdata->data, sizeof(struct packet_struct *));

	/*
	 * We want to just add the name, as we now know the original owner
	 * didn't want it. But we can't just do that as an arbitary
	 * amount of time may have taken place between the name query
	 * request and this timeout/error response. So we check that
	 * the name still exists and is in the same state - if so
	 * we remove it and call wins_process_name_registration_request()
	 * as we know it will do the right thing now.
	 */

	namerec = find_name_on_subnet(subrec, question_name, FIND_ANY_NAME);

	if ((namerec != NULL) && (namerec->data.source == REGISTER_NAME) &&
			ip_equal(rrec->packet->ip, *namerec->data.ip)) {
		remove_name_from_namelist( subrec, namerec);
		namerec = NULL;
	}

	if(namerec == NULL) {
		wins_process_name_registration_request(subrec, orig_reg_packet);
	} else {
		DEBUG(2,("wins_register_query_fail: The state of the WINS database changed between "
			"querying for name %s in order to replace it and this reply.\n",
			nmb_namestr(question_name) ));
	}

	orig_reg_packet->locked = False;
	free_packet(orig_reg_packet);
}

/***********************************************************************
 Deal with a name registration request to a WINS server.

 Use the following pseudocode :

 registering_group
     |
     |
     +--------name exists
     |                  |
     |                  |
     |                  +--- existing name is group
     |                  |                      |
     |                  |                      |
     |                  |                      +--- add name (return).
     |                  |
     |                  |
     |                  +--- exiting name is unique
     |                                         |
     |                                         |
     |                                         +--- query existing owner (return).
     |
     |
     +--------name doesn't exist
                        |
                        |
                        +--- add name (return).

 registering_unique
     |
     |
     +--------name exists
     |                  |
     |                  |
     |                  +--- existing name is group 
     |                  |                      |
     |                  |                      |
     |                  |                      +--- fail add (return).
     |                  | 
     |                  |
     |                  +--- exiting name is unique
     |                                         |
     |                                         |
     |                                         +--- query existing owner (return).
     |
     |
     +--------name doesn't exist
                        |
                        |
                        +--- add name (return).

 As can be seen from the above, the two cases may be collapsed onto each
 other with the exception of the case where the name already exists and
 is a group name. This case we handle with an if statement.
 
************************************************************************/

void wins_process_name_registration_request(struct subnet_record *subrec,
                                            struct packet_struct *p)
{
	unstring name;
	struct nmb_packet *nmb = &p->packet.nmb;
	struct nmb_name *question = &nmb->question.question_name;
	BOOL bcast = nmb->header.nm_flags.bcast;
	uint16 nb_flags = get_nb_flags(nmb->additional->rdata);
	int ttl = get_ttl_from_packet(nmb);
	struct name_record *namerec = NULL;
	struct in_addr from_ip;
	BOOL registering_group_name = (nb_flags & NB_GROUP) ? True : False;
	struct in_addr our_fake_ip = *interpret_addr2("0.0.0.0");

	putip((char *)&from_ip,&nmb->additional->rdata[2]);

	if(bcast) {
		/*
		 * We should only get unicast name registration packets here.
		 * Anyone trying to register broadcast should not be going to a WINS
		 * server. Log an error here.
		 */

		DEBUG(0,("wins_process_name_registration_request: broadcast name registration request \
received for name %s from IP %s on subnet %s. Error - should not be sent to WINS server\n",
			nmb_namestr(question), inet_ntoa(from_ip), subrec->subnet_name));
		return;
	}

	DEBUG(3,("wins_process_name_registration_request: %s name registration for name %s \
IP %s\n", registering_group_name ? "Group" : "Unique", nmb_namestr(question), inet_ntoa(from_ip) ));

	/*
	 * See if the name already exists.
	 */

	namerec = find_name_on_subnet(subrec, question, FIND_ANY_NAME);

	/*
	 * if the record exists but NOT in active state,
	 * consider it dead.
	 */
	if ( (namerec != NULL) && !WINS_STATE_ACTIVE(namerec)) {
		DEBUG(5,("wins_process_name_registration_request: Name (%s) in WINS was \
not active - removing it.\n", nmb_namestr(question) ));
		remove_name_from_namelist( subrec, namerec );
		namerec = NULL;
	}

	/*
	 * Deal with the case where the name found was a dns entry.
	 * Remove it as we now have a NetBIOS client registering the
	 * name.
	 */

	if( (namerec != NULL) && ( (namerec->data.source == DNS_NAME) || (namerec->data.source == DNSFAIL_NAME) ) ) {
		DEBUG(5,("wins_process_name_registration_request: Name (%s) in WINS was \
a dns lookup - removing it.\n", nmb_namestr(question) ));
		remove_name_from_namelist( subrec, namerec );
		namerec = NULL;
	}

	/*
	 * Reject if the name exists and is not a REGISTER_NAME.
	 * (ie. Don't allow any static names to be overwritten.
	 */

	if((namerec != NULL) && (namerec->data.source != REGISTER_NAME)) {
		DEBUG( 3, ( "wins_process_name_registration_request: Attempt \
to register name %s. Name already exists in WINS with source type %d.\n",
			nmb_namestr(question), namerec->data.source ));
		send_wins_name_registration_response(RFS_ERR, 0, p);
		return;
	}

	/*
	 * Special policy decisions based on MS documentation.
	 * 1). All group names (except names ending in 0x1c) are added as 255.255.255.255.
	 * 2). All unique names ending in 0x1d are ignored, although a positive response is sent.
	 */

	/*
	 * A group name is always added as the local broadcast address, except
	 * for group names ending in 0x1c.
	 * Group names with type 0x1c are registered with individual IP addresses.
	 */

	if(registering_group_name && (question->name_type != 0x1c)) {
		from_ip = *interpret_addr2("255.255.255.255");
	}

	/*
	 * Ignore all attempts to register a unique 0x1d name, although return success.
	 */

	if(!registering_group_name && (question->name_type == 0x1d)) {
		DEBUG(3,("wins_process_name_registration_request: Ignoring request \
to register name %s from IP %s.\n", nmb_namestr(question), inet_ntoa(p->ip) ));
		send_wins_name_registration_response(0, ttl, p);
		return;
	}

	/*
	 * Next two cases are the 'if statement' mentioned above.
	 */

	if((namerec != NULL) && NAME_GROUP(namerec)) {
		if(registering_group_name) {
			/*
			 * If we are adding a group name, the name exists and is also a group entry just add this
			 * IP address to it and update the ttl.
			 */

			DEBUG(3,("wins_process_name_registration_request: Adding IP %s to group name %s.\n",
				inet_ntoa(from_ip), nmb_namestr(question) ));

			/* 
			 * Check the ip address is not already in the group.
			 */

			if(!find_ip_in_name_record(namerec, from_ip)) {
				add_ip_to_name_record(namerec, from_ip);
				/* we need to update the record for replication */
				get_global_id_and_update(&namerec->data.id, True);

				/*
				 * if the record is a replica, we must change
				 * the wins owner to us to make the replication updates
				 * it on the other wins servers.
				 * And when the partner will receive this record,
				 * it will update its own record.
				 */

				update_wins_owner(namerec, our_fake_ip);
			}
			update_name_ttl(namerec, ttl);
			wins_hook("refresh", namerec, ttl);
			send_wins_name_registration_response(0, ttl, p);
			return;
		} else {

			/*
			 * If we are adding a unique name, the name exists in the WINS db 
			 * and is a group name then reject the registration.
			 *
			 * explanation: groups have a higher priority than unique names.
			 */

			DEBUG(3,("wins_process_name_registration_request: Attempt to register name %s. Name \
already exists in WINS as a GROUP name.\n", nmb_namestr(question) ));
			send_wins_name_registration_response(RFS_ERR, 0, p);
			return;
		} 
	}

	/*
	 * From here on down we know that if the name exists in the WINS db it is
	 * a unique name, not a group name.
	 */

	/* 
	 * If the name exists and is one of our names then check the
	 * registering IP address. If it's not one of ours then automatically
	 * reject without doing the query - we know we will reject it.
	 */

	if ( namerec != NULL ) {
		pull_ascii_nstring(name, sizeof(name), namerec->name.name);
		if( is_myname(name) ) {
			if(!ismyip(from_ip)) {
				DEBUG(3,("wins_process_name_registration_request: Attempt to register name %s. Name \
is one of our (WINS server) names. Denying registration.\n", nmb_namestr(question) ));
				send_wins_name_registration_response(RFS_ERR, 0, p);
				return;
			} else {
				/*
				 * It's one of our names and one of our IP's - update the ttl.
				 */
				update_name_ttl(namerec, ttl);
				wins_hook("refresh", namerec, ttl);
				send_wins_name_registration_response(0, ttl, p);
				return;
			}
		}
	} else {
		name[0] = '\0';
	}

	/*
	 * If the name exists and it is a unique registration and the registering IP 
	 * is the same as the (single) already registered IP then just update the ttl.
	 *
	 * But not if the record is an active replica. IF it's a replica, it means it can be
	 * the same client which has moved and not yet expired. So we don't update
	 * the ttl in this case and go beyond to do a WACK and query the old client
	 */

	if( !registering_group_name
			&& (namerec != NULL)
			&& (namerec->data.num_ips == 1)
			&& ip_equal( namerec->data.ip[0], from_ip )
			&& ip_equal(namerec->data.wins_ip, our_fake_ip) ) {
		update_name_ttl( namerec, ttl );
		wins_hook("refresh", namerec, ttl);
		send_wins_name_registration_response( 0, ttl, p );
		return;
	}

	/*
	 * Finally if the name exists do a query to the registering machine 
	 * to see if they still claim to have the name.
	 */

	if( namerec != NULL ) {
		long *ud[(sizeof(struct userdata_struct) + sizeof(struct packet_struct *))/sizeof(long *) + 1];
		struct userdata_struct *userdata = (struct userdata_struct *)ud;

		/*
		 * First send a WACK to the registering machine.
		 */

		send_wins_wack_response(60, p);

		/*
		 * When the reply comes back we need the original packet.
		 * Lock this so it won't be freed and then put it into
		 * the userdata structure.
		 */

		p->locked = True;

		userdata = (struct userdata_struct *)ud;

		userdata->copy_fn = NULL;
		userdata->free_fn = NULL;
		userdata->userdata_len = sizeof(struct packet_struct *);
		memcpy(userdata->data, (char *)&p, sizeof(struct packet_struct *) );

		/*
		 * Use the new call to send a query directly to an IP address.
		 * This sends the query directly to the IP address, and ensures
		 * the recursion desired flag is not set (you were right Luke :-).
		 * This function should *only* be called from the WINS server
		 * code. JRA.
		 */

		pull_ascii_nstring(name, sizeof(name), question->name);
		query_name_from_wins_server( *namerec->data.ip,
				name,
				question->name_type, 
				wins_register_query_success,
				wins_register_query_fail,
				userdata );
		return;
	}

	/*
	 * Name did not exist - add it.
	 */

	pull_ascii_nstring(name, sizeof(name), question->name);
	add_name_to_subnet( subrec, name, question->name_type,
			nb_flags, ttl, REGISTER_NAME, 1, &from_ip);

	if ((namerec = find_name_on_subnet(subrec, question, FIND_ANY_NAME))) {
		get_global_id_and_update(&namerec->data.id, True);
		update_wins_owner(namerec, our_fake_ip);
		update_wins_flag(namerec, WINS_ACTIVE);
		wins_hook("add", namerec, ttl);
	}

	send_wins_name_registration_response(0, ttl, p);
}

/***********************************************************************
 Deal with a mutihomed name query success to the machine that
 requested the multihomed name registration.

 We have a locked pointer to the original packet stashed away in the
 userdata pointer.
************************************************************************/

static void wins_multihomed_register_query_success(struct subnet_record *subrec,
                                             struct userdata_struct *userdata,
                                             struct nmb_name *question_name,
                                             struct in_addr ip,
                                             struct res_rec *answers)
{
	struct packet_struct *orig_reg_packet;
	struct nmb_packet *nmb;
	struct name_record *namerec = NULL;
	struct in_addr from_ip;
	int ttl;
	struct in_addr our_fake_ip = *interpret_addr2("0.0.0.0");

	memcpy((char *)&orig_reg_packet, userdata->data, sizeof(struct packet_struct *));

	nmb = &orig_reg_packet->packet.nmb;

	putip((char *)&from_ip,&nmb->additional->rdata[2]);
	ttl = get_ttl_from_packet(nmb);

	/*
	 * We want to just add the new IP, as we now know the requesting
	 * machine claims to own it. But we can't just do that as an arbitary
	 * amount of time may have taken place between the name query
	 * request and this response. So we check that
	 * the name still exists and is in the same state - if so
	 * we just add the extra IP and update the ttl.
	 */

	namerec = find_name_on_subnet(subrec, question_name, FIND_ANY_NAME);

	if( (namerec == NULL) || (namerec->data.source != REGISTER_NAME) || !WINS_STATE_ACTIVE(namerec) ) {
		DEBUG(3,("wins_multihomed_register_query_success: name %s is not in the correct state to add \
a subsequent IP address.\n", nmb_namestr(question_name) ));
		send_wins_name_registration_response(RFS_ERR, 0, orig_reg_packet);

		orig_reg_packet->locked = False;
		free_packet(orig_reg_packet);

		return;
	}

	if(!find_ip_in_name_record(namerec, from_ip)) {
		add_ip_to_name_record(namerec, from_ip);
	}

	get_global_id_and_update(&namerec->data.id, True);
	update_wins_owner(namerec, our_fake_ip);
	update_wins_flag(namerec, WINS_ACTIVE);
	update_name_ttl(namerec, ttl);
	wins_hook("add", namerec, ttl);
	send_wins_name_registration_response(0, ttl, orig_reg_packet);

	orig_reg_packet->locked = False;
	free_packet(orig_reg_packet);
}

/***********************************************************************
 Deal with a name registration request query failure to a client that
 owned the name.

 We have a locked pointer to the original packet stashed away in the
 userdata pointer.
************************************************************************/

static void wins_multihomed_register_query_fail(struct subnet_record *subrec,
                                          struct response_record *rrec,
                                          struct nmb_name *question_name,
                                          int rcode)
{
	struct userdata_struct *userdata = rrec->userdata;
	struct packet_struct *orig_reg_packet;

	memcpy((char *)&orig_reg_packet, userdata->data, sizeof(struct packet_struct *));

	DEBUG(3,("wins_multihomed_register_query_fail: Registering machine at IP %s failed to answer \
query successfully for name %s.\n", inet_ntoa(orig_reg_packet->ip), nmb_namestr(question_name) ));
	send_wins_name_registration_response(RFS_ERR, 0, orig_reg_packet);

	orig_reg_packet->locked = False;
	free_packet(orig_reg_packet);
	return;
}

/***********************************************************************
 Deal with a multihomed name registration request to a WINS server.
 These cannot be group name registrations.
***********************************************************************/

void wins_process_multihomed_name_registration_request( struct subnet_record *subrec,
                                                        struct packet_struct *p)
{
	struct nmb_packet *nmb = &p->packet.nmb;
	struct nmb_name *question = &nmb->question.question_name;
	BOOL bcast = nmb->header.nm_flags.bcast;
	uint16 nb_flags = get_nb_flags(nmb->additional->rdata);
	int ttl = get_ttl_from_packet(nmb);
	struct name_record *namerec = NULL;
	struct in_addr from_ip;
	BOOL group = (nb_flags & NB_GROUP) ? True : False;
	struct in_addr our_fake_ip = *interpret_addr2("0.0.0.0");
	unstring qname;

	putip((char *)&from_ip,&nmb->additional->rdata[2]);

	if(bcast) {
		/*
		 * We should only get unicast name registration packets here.
		 * Anyone trying to register broadcast should not be going to a WINS
		 * server. Log an error here.
		 */

		DEBUG(0,("wins_process_multihomed_name_registration_request: broadcast name registration request \
received for name %s from IP %s on subnet %s. Error - should not be sent to WINS server\n",
			nmb_namestr(question), inet_ntoa(from_ip), subrec->subnet_name));
		return;
	}

	/*
	 * Only unique names should be registered multihomed.
	 */

	if(group) {
		DEBUG(0,("wins_process_multihomed_name_registration_request: group name registration request \
received for name %s from IP %s on subnet %s. Errror - group names should not be multihomed.\n",
			nmb_namestr(question), inet_ntoa(from_ip), subrec->subnet_name));
		return;
	}

	DEBUG(3,("wins_process_multihomed_name_registration_request: name registration for name %s \
IP %s\n", nmb_namestr(question), inet_ntoa(from_ip) ));

	/*
	 * Deal with policy regarding 0x1d names.
	 */

	if(question->name_type == 0x1d) {
		DEBUG(3,("wins_process_multihomed_name_registration_request: Ignoring request \
to register name %s from IP %s.", nmb_namestr(question), inet_ntoa(p->ip) ));
		send_wins_name_registration_response(0, ttl, p);  
		return;
	}

	/*
	 * See if the name already exists.
	 */

	namerec = find_name_on_subnet(subrec, question, FIND_ANY_NAME);

	/*
	 * if the record exists but NOT in active state,
	 * consider it dead.
	 */

	if ((namerec != NULL) && !WINS_STATE_ACTIVE(namerec)) {
		DEBUG(5,("wins_process_multihomed_name_registration_request: Name (%s) in WINS was not active - removing it.\n", nmb_namestr(question)));
		remove_name_from_namelist(subrec, namerec);
		namerec = NULL;
	}
  
	/*
	 * Deal with the case where the name found was a dns entry.
	 * Remove it as we now have a NetBIOS client registering the
	 * name.
	 */

	if( (namerec != NULL) && ( (namerec->data.source == DNS_NAME) || (namerec->data.source == DNSFAIL_NAME) ) ) {
		DEBUG(5,("wins_process_multihomed_name_registration_request: Name (%s) in WINS was a dns lookup \
- removing it.\n", nmb_namestr(question) ));
		remove_name_from_namelist( subrec, namerec);
		namerec = NULL;
	}

	/*
	 * Reject if the name exists and is not a REGISTER_NAME.
	 * (ie. Don't allow any static names to be overwritten.
	 */

	if( (namerec != NULL) && (namerec->data.source != REGISTER_NAME) ) {
		DEBUG( 3, ( "wins_process_multihomed_name_registration_request: Attempt \
to register name %s. Name already exists in WINS with source type %d.\n",
			nmb_namestr(question), namerec->data.source ));
		send_wins_name_registration_response(RFS_ERR, 0, p);
		return;
	}

	/*
	 * Reject if the name exists and is a GROUP name and is active.
	 */

	if((namerec != NULL) && NAME_GROUP(namerec) && WINS_STATE_ACTIVE(namerec)) {
		DEBUG(3,("wins_process_multihomed_name_registration_request: Attempt to register name %s. Name \
already exists in WINS as a GROUP name.\n", nmb_namestr(question) ));
		send_wins_name_registration_response(RFS_ERR, 0, p);
		return;
	} 

	/*
	 * From here on down we know that if the name exists in the WINS db it is
	 * a unique name, not a group name.
	 */

	/*
	 * If the name exists and is one of our names then check the
	 * registering IP address. If it's not one of ours then automatically
	 * reject without doing the query - we know we will reject it.
	 */

	if((namerec != NULL) && (is_myname(namerec->name.name)) ) {
		if(!ismyip(from_ip)) {
			DEBUG(3,("wins_process_multihomed_name_registration_request: Attempt to register name %s. Name \
is one of our (WINS server) names. Denying registration.\n", nmb_namestr(question) ));
			send_wins_name_registration_response(RFS_ERR, 0, p);
			return;
		} else {
			/*
			 * It's one of our names and one of our IP's. Ensure the IP is in the record and
			 *  update the ttl. Update the version ID to force replication.
			 */
			update_name_ttl(namerec, ttl);

			if(!find_ip_in_name_record(namerec, from_ip)) {
				get_global_id_and_update(&namerec->data.id, True);
				update_wins_owner(namerec, our_fake_ip);
				update_wins_flag(namerec, WINS_ACTIVE);

				add_ip_to_name_record(namerec, from_ip);
			}

			wins_hook("refresh", namerec, ttl);
			send_wins_name_registration_response(0, ttl, p);
			return;
		}
	}

	/*
	 * If the name exists and is active, check if the IP address is already registered
	 * to that name. If so then update the ttl and reply success.
	 */

	if((namerec != NULL) && find_ip_in_name_record(namerec, from_ip) && WINS_STATE_ACTIVE(namerec)) {
		update_name_ttl(namerec, ttl);

		/*
		 * If it's a replica, we need to become the wins owner
		 * to force the replication
		 */
		if (!ip_equal(namerec->data.wins_ip, our_fake_ip)) {
			get_global_id_and_update(&namerec->data.id, True);
			update_wins_owner(namerec, our_fake_ip);
			update_wins_flag(namerec, WINS_ACTIVE);
		}
    
		wins_hook("refresh", namerec, ttl);
		send_wins_name_registration_response(0, ttl, p);
		return;
	}

	/*
	 * If the name exists do a query to the owner
	 * to see if they still want the name.
	 */

	if(namerec != NULL) {
		long *ud[(sizeof(struct userdata_struct) + sizeof(struct packet_struct *))/sizeof(long *) + 1];
		struct userdata_struct *userdata = (struct userdata_struct *)ud;

		/*
		 * First send a WACK to the registering machine.
		 */

		send_wins_wack_response(60, p);

		/*
		 * When the reply comes back we need the original packet.
		 * Lock this so it won't be freed and then put it into
		 * the userdata structure.
		 */

		p->locked = True;

		userdata = (struct userdata_struct *)ud;

		userdata->copy_fn = NULL;
		userdata->free_fn = NULL;
		userdata->userdata_len = sizeof(struct packet_struct *);
		memcpy(userdata->data, (char *)&p, sizeof(struct packet_struct *) );

		/* 
		 * Use the new call to send a query directly to an IP address.
		 * This sends the query directly to the IP address, and ensures
		 * the recursion desired flag is not set (you were right Luke :-).
		 * This function should *only* be called from the WINS server
		 * code. JRA.
		 *
		 * Note that this packet is sent to the current owner of the name,
		 * not the person who sent the packet 
		 */

		pull_ascii_nstring( qname, sizeof(qname), question->name);
		query_name_from_wins_server( namerec->data.ip[0],
				qname,
				question->name_type, 
				wins_multihomed_register_query_success,
				wins_multihomed_register_query_fail,
				userdata );

		return;
	}

	/*
	 * Name did not exist - add it.
	 */

	pull_ascii_nstring( qname, sizeof(qname), question->name);
	add_name_to_subnet( subrec, qname, question->name_type,
			nb_flags, ttl, REGISTER_NAME, 1, &from_ip);

	if ((namerec = find_name_on_subnet(subrec, question, FIND_ANY_NAME))) {
		get_global_id_and_update(&namerec->data.id, True);
		update_wins_owner(namerec, our_fake_ip);
		update_wins_flag(namerec, WINS_ACTIVE);
		wins_hook("add", namerec, ttl);
	}

	send_wins_name_registration_response(0, ttl, p);
}

/***********************************************************************
 Fetch all *<1b> names from the WINS db and store on the namelist.
***********************************************************************/

static int fetch_1b_traverse_fn(TDB_CONTEXT *tdb, TDB_DATA kbuf, TDB_DATA dbuf, void *state)
{
	struct name_record *namerec = NULL;

	if (kbuf.dsize != sizeof(unstring) + 1) {
		return 0;
	}

	/* Filter out all non-1b names. */
	if (kbuf.dptr[sizeof(unstring)] != 0x1b) {
		return 0;
	}

	namerec = wins_record_to_name_record(kbuf, dbuf);
	if (!namerec) {
		return 0;
	}

	DLIST_ADD(wins_server_subnet->namelist, namerec);
	return 0;
}

void fetch_all_active_wins_1b_names(void)
{
	tdb_traverse(wins_tdb, fetch_1b_traverse_fn, NULL);
}

/***********************************************************************
 Deal with the special name query for *<1b>.
***********************************************************************/
   
static void process_wins_dmb_query_request(struct subnet_record *subrec,  
                                           struct packet_struct *p)
{  
	struct name_record *namerec = NULL;
	char *prdata;
	int num_ips;

	/*
	 * Go through all the ACTIVE names in the WINS db looking for those
	 * ending in <1b>. Use this to calculate the number of IP
	 * addresses we need to return.
	 */

	num_ips = 0;

	/* First, clear the in memory list - we're going to re-populate
	   it with the tdb_traversal in fetch_all_active_wins_1b_names. */

	wins_delete_all_tmp_in_memory_records();

	fetch_all_active_wins_1b_names();

	for( namerec = subrec->namelist; namerec; namerec = namerec->next ) {
		if( WINS_STATE_ACTIVE(namerec) && namerec->name.name_type == 0x1b) {
			num_ips += namerec->data.num_ips;
		}
	}

	if(num_ips == 0) {
		/*
		 * There are no 0x1b names registered. Return name query fail.
		 */
		send_wins_name_query_response(NAM_ERR, p, NULL);
		return;
	}

	if((prdata = (char *)SMB_MALLOC( num_ips * 6 )) == NULL) {
		DEBUG(0,("process_wins_dmb_query_request: Malloc fail !.\n"));
		return;
	}

	/*
	 * Go through all the names again in the WINS db looking for those
	 * ending in <1b>. Add their IP addresses into the list we will
	 * return.
	 */ 

	num_ips = 0;
	for( namerec = subrec->namelist; namerec; namerec = namerec->next ) {
		if( WINS_STATE_ACTIVE(namerec) && namerec->name.name_type == 0x1b) {
			int i;
			for(i = 0; i < namerec->data.num_ips; i++) {
				set_nb_flags(&prdata[num_ips * 6],namerec->data.nb_flags);
				putip((char *)&prdata[(num_ips * 6) + 2], &namerec->data.ip[i]);
				num_ips++;
			}
		}
	}

	/*
	 * Send back the reply containing the IP list.
	 */

	reply_netbios_packet(p,                                /* Packet to reply to. */
				0,                             /* Result code. */
				WINS_QUERY,                    /* nmbd type code. */
				NMB_NAME_QUERY_OPCODE,         /* opcode. */
				lp_min_wins_ttl(),             /* ttl. */
				prdata,                        /* data to send. */
				num_ips*6);                    /* data length. */

	SAFE_FREE(prdata);
}

/****************************************************************************
Send a WINS name query response.
**************************************************************************/

void send_wins_name_query_response(int rcode, struct packet_struct *p, 
                                          struct name_record *namerec)
{
	char rdata[6];
	char *prdata = rdata;
	int reply_data_len = 0;
	int ttl = 0;
	int i;

	memset(rdata,'\0',6);

	if(rcode == 0) {
		ttl = (namerec->data.death_time != PERMANENT_TTL) ?  namerec->data.death_time - p->timestamp : lp_max_wins_ttl();

		/* Copy all known ip addresses into the return data. */
		/* Optimise for the common case of one IP address so we don't need a malloc. */

		if( namerec->data.num_ips == 1 ) {
			prdata = rdata;
		} else {
			if((prdata = (char *)SMB_MALLOC( namerec->data.num_ips * 6 )) == NULL) {
				DEBUG(0,("send_wins_name_query_response: malloc fail !\n"));
				return;
			}
		}

		for(i = 0; i < namerec->data.num_ips; i++) {
			set_nb_flags(&prdata[i*6],namerec->data.nb_flags);
			putip((char *)&prdata[2+(i*6)], &namerec->data.ip[i]);
		}

		sort_query_replies(prdata, i, p->ip);
		reply_data_len = namerec->data.num_ips * 6;
	}

	reply_netbios_packet(p,                                /* Packet to reply to. */
				rcode,                         /* Result code. */
				WINS_QUERY,                    /* nmbd type code. */
				NMB_NAME_QUERY_OPCODE,         /* opcode. */
				ttl,                           /* ttl. */
				prdata,                        /* data to send. */
				reply_data_len);               /* data length. */

	if(prdata != rdata) {
		SAFE_FREE(prdata);
	}
}

/***********************************************************************
 Deal with a name query.
***********************************************************************/

void wins_process_name_query_request(struct subnet_record *subrec, 
                                     struct packet_struct *p)
{
	struct nmb_packet *nmb = &p->packet.nmb;
	struct nmb_name *question = &nmb->question.question_name;
	struct name_record *namerec = NULL;
	unstring qname;

	DEBUG(3,("wins_process_name_query: name query for name %s from IP %s\n", 
		nmb_namestr(question), inet_ntoa(p->ip) ));

	/*
	 * Special name code. If the queried name is *<1b> then search
	 * the entire WINS database and return a list of all the IP addresses
	 * registered to any <1b> name. This is to allow domain master browsers
	 * to discover other domains that may not have a presence on their subnet.
	 */

	pull_ascii_nstring(qname, sizeof(qname), question->name);
	if(strequal( qname, "*") && (question->name_type == 0x1b)) {
		process_wins_dmb_query_request( subrec, p);
		return;
	}

	namerec = find_name_on_subnet(subrec, question, FIND_ANY_NAME);

	if(namerec != NULL) {
		/*
		 * If the name is not anymore in active state then reply not found.
		 * it's fair even if we keep it in the cache for days.
		 */
		if (!WINS_STATE_ACTIVE(namerec)) {
			DEBUG(3,("wins_process_name_query: name query for name %s - name expired. Returning fail.\n",
				nmb_namestr(question) ));
			send_wins_name_query_response(NAM_ERR, p, namerec);
			return;
		}

		/* 
		 * If it's a DNSFAIL_NAME then reply name not found.
		 */

		if( namerec->data.source == DNSFAIL_NAME ) {
			DEBUG(3,("wins_process_name_query: name query for name %s returning DNS fail.\n",
				nmb_namestr(question) ));
			send_wins_name_query_response(NAM_ERR, p, namerec);
			return;
		}

		/*
		 * If the name has expired then reply name not found.
		 */

		if( (namerec->data.death_time != PERMANENT_TTL) && (namerec->data.death_time < p->timestamp) ) {
			DEBUG(3,("wins_process_name_query: name query for name %s - name expired. Returning fail.\n",
					nmb_namestr(question) ));
			send_wins_name_query_response(NAM_ERR, p, namerec);
			return;
		}

		DEBUG(3,("wins_process_name_query: name query for name %s returning first IP %s.\n",
				nmb_namestr(question), inet_ntoa(namerec->data.ip[0]) ));

		send_wins_name_query_response(0, p, namerec);
		return;
	}

	/* 
	 * Name not found in WINS - try a dns query if it's a 0x20 name.
	 */

	if(lp_dns_proxy() && ((question->name_type == 0x20) || question->name_type == 0)) {
		DEBUG(3,("wins_process_name_query: name query for name %s not found - doing dns lookup.\n",
				nmb_namestr(question) ));

		queue_dns_query(p, question);
		return;
	}

	/*
	 * Name not found - return error.
	 */

	send_wins_name_query_response(NAM_ERR, p, NULL);
}

/****************************************************************************
Send a WINS name release response.
**************************************************************************/

static void send_wins_name_release_response(int rcode, struct packet_struct *p)
{
	struct nmb_packet *nmb = &p->packet.nmb;
	char rdata[6];

	memcpy(&rdata[0], &nmb->additional->rdata[0], 6);

	reply_netbios_packet(p,                               /* Packet to reply to. */
				rcode,                        /* Result code. */
				NMB_REL,                      /* nmbd type code. */
				NMB_NAME_RELEASE_OPCODE,      /* opcode. */
				0,                            /* ttl. */
				rdata,                        /* data to send. */
				6);                           /* data length. */
}

/***********************************************************************
 Deal with a name release.
***********************************************************************/

void wins_process_name_release_request(struct subnet_record *subrec,
                                       struct packet_struct *p)
{
	struct nmb_packet *nmb = &p->packet.nmb;
	struct nmb_name *question = &nmb->question.question_name;
	BOOL bcast = nmb->header.nm_flags.bcast;
	uint16 nb_flags = get_nb_flags(nmb->additional->rdata);
	struct name_record *namerec = NULL;
	struct in_addr from_ip;
	BOOL releasing_group_name = (nb_flags & NB_GROUP) ? True : False;;

	putip((char *)&from_ip,&nmb->additional->rdata[2]);

	if(bcast) {
		/*
		 * We should only get unicast name registration packets here.
		 * Anyone trying to register broadcast should not be going to a WINS
		 * server. Log an error here.
		 */

		DEBUG(0,("wins_process_name_release_request: broadcast name registration request \
received for name %s from IP %s on subnet %s. Error - should not be sent to WINS server\n",
			nmb_namestr(question), inet_ntoa(from_ip), subrec->subnet_name));
		return;
	}
  
	DEBUG(3,("wins_process_name_release_request: %s name release for name %s \
IP %s\n", releasing_group_name ? "Group" : "Unique", nmb_namestr(question), inet_ntoa(from_ip) ));
    
	/*
	 * Deal with policy regarding 0x1d names.
	 */

	if(!releasing_group_name && (question->name_type == 0x1d)) {
		DEBUG(3,("wins_process_name_release_request: Ignoring request \
to release name %s from IP %s.", nmb_namestr(question), inet_ntoa(p->ip) ));
		send_wins_name_release_response(0, p);
		return;
	}

	/*
	 * See if the name already exists.
	 */
    
	namerec = find_name_on_subnet(subrec, question, FIND_ANY_NAME);

	if( (namerec == NULL) || ((namerec != NULL) && (namerec->data.source != REGISTER_NAME)) ) {
		send_wins_name_release_response(NAM_ERR, p);
		return;
	}

	/* 
	 * Check that the sending machine has permission to release this name.
	 * If it's a group name not ending in 0x1c then just say yes and let
	 * the group time out.
	 */

	if(releasing_group_name && (question->name_type != 0x1c)) {
		send_wins_name_release_response(0, p);
		return;
	}

	/* 
	 * Check that the releasing node is on the list of IP addresses
	 * for this name. Disallow the release if not.
	 */

	if(!find_ip_in_name_record(namerec, from_ip)) {
		DEBUG(3,("wins_process_name_release_request: Refusing request to \
release name %s as IP %s is not one of the known IP's for this name.\n",
			nmb_namestr(question), inet_ntoa(from_ip) ));
		send_wins_name_release_response(NAM_ERR, p);
		return;
	}

	/*
	 * Check if the record is active. IF it's already released
	 * or tombstoned, refuse the release.
	 */

	if (!WINS_STATE_ACTIVE(namerec)) {
		DEBUG(3,("wins_process_name_release_request: Refusing request to \
release name %s as this record is not active anymore.\n", nmb_namestr(question) ));
		send_wins_name_release_response(NAM_ERR, p);
		return;
	}    

	/*
	 * Check if the record is a 0x1c group
	 * and has more then one ip
	 * remove only this address.
	 */

	if(releasing_group_name && (question->name_type == 0x1c) && (namerec->data.num_ips > 1)) {
		remove_ip_from_name_record(namerec, from_ip);
		DEBUG(3,("wins_process_name_release_request: Remove IP %s from NAME: %s\n",
				inet_ntoa(from_ip),nmb_namestr(question)));
		wins_hook("delete", namerec, 0);
		send_wins_name_release_response(0, p);
		return;
	}

	/* 
	 * Send a release response.
	 * Flag the name as released and update the ttl
	 */

	namerec->data.wins_flags |= WINS_RELEASED;
	update_name_ttl(namerec, EXTINCTION_INTERVAL);

	wins_hook("delete", namerec, 0);
	send_wins_name_release_response(0, p);
}

/*******************************************************************
 WINS time dependent processing.
******************************************************************/

static int wins_processing_traverse_fn(TDB_CONTEXT *tdb, TDB_DATA kbuf, TDB_DATA dbuf, void *state)
{
	time_t t = *(time_t *)state;
	BOOL store_record = False;
	struct name_record *namerec = NULL;
	struct in_addr our_fake_ip = *interpret_addr2("0.0.0.0");

	if (kbuf.dsize != sizeof(unstring) + 1) {
		return 0;
	}

	namerec = wins_record_to_name_record(kbuf, dbuf);
	if (!namerec) {
		return 0;
	}

	if( (namerec->data.death_time != PERMANENT_TTL) && (namerec->data.death_time < t) ) {
		if( namerec->data.source == SELF_NAME ) {
			DEBUG( 3, ( "wins_processing_traverse_fn: Subnet %s not expiring SELF name %s\n", 
			           wins_server_subnet->subnet_name, nmb_namestr(&namerec->name) ) );
			namerec->data.death_time += 300;
			store_record = True;
			goto done;
		} else if (namerec->data.source == DNS_NAME || namerec->data.source == DNSFAIL_NAME) {
			DEBUG(3,("wins_processing_traverse_fn: deleting timed out DNS name %s\n",
					nmb_namestr(&namerec->name)));
			remove_name_from_wins_namelist(namerec );
			goto done;
		}

		/* handle records, samba is the wins owner */
		if (ip_equal(namerec->data.wins_ip, our_fake_ip)) {
			switch (namerec->data.wins_flags | WINS_STATE_MASK) {
				case WINS_ACTIVE:
					namerec->data.wins_flags&=~WINS_STATE_MASK;
					namerec->data.wins_flags|=WINS_RELEASED;
					namerec->data.death_time = t + EXTINCTION_INTERVAL;
					DEBUG(3,("wins_processing_traverse_fn: expiring %s\n",
						nmb_namestr(&namerec->name)));
					store_record = True;
					goto done;
				case WINS_RELEASED:
					namerec->data.wins_flags&=~WINS_STATE_MASK;
					namerec->data.wins_flags|=WINS_TOMBSTONED;
					namerec->data.death_time = t + EXTINCTION_TIMEOUT;
					get_global_id_and_update(&namerec->data.id, True);
					DEBUG(3,("wins_processing_traverse_fn: tombstoning %s\n",
						nmb_namestr(&namerec->name)));
					store_record = True;
					goto done;
				case WINS_TOMBSTONED:
					DEBUG(3,("wins_processing_traverse_fn: deleting %s\n",
						nmb_namestr(&namerec->name)));
					remove_name_from_wins_namelist(namerec );
					goto done;
			}
		} else {
			switch (namerec->data.wins_flags | WINS_STATE_MASK) {
				case WINS_ACTIVE:
					/* that's not as MS says it should be */
					namerec->data.wins_flags&=~WINS_STATE_MASK;
					namerec->data.wins_flags|=WINS_TOMBSTONED;
					namerec->data.death_time = t + EXTINCTION_TIMEOUT;
					DEBUG(3,("wins_processing_traverse_fn: tombstoning %s\n",
						nmb_namestr(&namerec->name)));
					store_record = True;
					goto done;
				case WINS_TOMBSTONED:
					DEBUG(3,("wins_processing_traverse_fn: deleting %s\n",
						nmb_namestr(&namerec->name)));
					remove_name_from_wins_namelist(namerec );
					goto done;
				case WINS_RELEASED:
					DEBUG(0,("wins_processing_traverse_fn: %s is in released state and\
we are not the wins owner !\n", nmb_namestr(&namerec->name)));
					goto done;
			}
		}
	}

  done:

	if (store_record) {
		wins_store_changed_namerec(namerec);
	}

	SAFE_FREE(namerec->data.ip);
	SAFE_FREE(namerec);

	return 0;
}

/*******************************************************************
 Time dependent wins processing.
******************************************************************/

void initiate_wins_processing(time_t t)
{
	static time_t lasttime = 0;

	if (!lasttime) {
		lasttime = t;
	}
	if (t - lasttime < 20) {
		return;
	}

	if(!lp_we_are_a_wins_server()) {
		lasttime = t;
		return;
	}

	tdb_traverse(wins_tdb, wins_processing_traverse_fn, &t);

	wins_delete_all_tmp_in_memory_records();

	wins_write_database(t, True);

	lasttime = t;
}

/*******************************************************************
 Write out one record.
******************************************************************/

void wins_write_name_record(struct name_record *namerec, XFILE *fp)
{
	int i;
	struct tm *tm;

	DEBUGADD(4,("%-19s ", nmb_namestr(&namerec->name) ));

	if( namerec->data.death_time != PERMANENT_TTL ) {
		char *ts, *nl;

		tm = localtime(&namerec->data.death_time);
		if (!tm) {
			return;
		}
		ts = asctime(tm);
		if (!ts) {
			return;
		}
		nl = strrchr( ts, '\n' );
		if( NULL != nl ) {
			*nl = '\0';
		}
		DEBUGADD(4,("TTL = %s  ", ts ));
	} else {
		DEBUGADD(4,("TTL = PERMANENT                 "));
	}

	for (i = 0; i < namerec->data.num_ips; i++) {
		DEBUGADD(4,("%15s ", inet_ntoa(namerec->data.ip[i]) ));
	}
	DEBUGADD(4,("%2x\n", namerec->data.nb_flags ));

	if( namerec->data.source == REGISTER_NAME ) {
		unstring name;
		pull_ascii_nstring(name, sizeof(name), namerec->name.name);
		x_fprintf(fp, "\"%s#%02x\" %d ", name,namerec->name.name_type, /* Ignore scope. */
			(int)namerec->data.death_time);

		for (i = 0; i < namerec->data.num_ips; i++)
			x_fprintf( fp, "%s ", inet_ntoa( namerec->data.ip[i] ) );
		x_fprintf( fp, "%2xR\n", namerec->data.nb_flags );
	}
}

/*******************************************************************
 Write out the current WINS database.
******************************************************************/

static int wins_writedb_traverse_fn(TDB_CONTEXT *tdb, TDB_DATA kbuf, TDB_DATA dbuf, void *state)
{
	struct name_record *namerec = NULL;
	XFILE *fp = (XFILE *)state;

	if (kbuf.dsize != sizeof(unstring) + 1) {
		return 0;
	}

	namerec = wins_record_to_name_record(kbuf, dbuf);
	if (!namerec) {
		return 0;
	}

	wins_write_name_record(namerec, fp);

	SAFE_FREE(namerec->data.ip);
	SAFE_FREE(namerec);
	return 0;
}


void wins_write_database(time_t t, BOOL background)
{
	static time_t last_write_time = 0;
	pstring fname, fnamenew;

	XFILE *fp;
   
	if (background) {
		if (!last_write_time) {
			last_write_time = t;
		}
		if (t - last_write_time < 120) {
			return;
		}

	}

	if(!lp_we_are_a_wins_server()) {
		return;
	}

	/* We will do the writing in a child process to ensure that the parent doesn't block while this is done */
	if (background) {
		CatchChild();
		if (sys_fork()) {
			return;
		}
		if (tdb_reopen(wins_tdb)) {
			DEBUG(0,("wins_write_database: tdb_reopen failed. Error was %s\n",
				strerror(errno)));
			return;
		}
	}

	slprintf(fname,sizeof(fname)-1,"%s/%s", lp_lockdir(), WINS_LIST);
	all_string_sub(fname,"//", "/", 0);
	slprintf(fnamenew,sizeof(fnamenew)-1,"%s.%u", fname, (unsigned int)sys_getpid());

	if((fp = x_fopen(fnamenew,O_WRONLY|O_CREAT,0644)) == NULL) {
		DEBUG(0,("wins_write_database: Can't open %s. Error was %s\n", fnamenew, strerror(errno)));
		if (background) {
			_exit(0);
		}
		return;
	}

	DEBUG(4,("wins_write_database: Dump of WINS name list.\n"));

	x_fprintf(fp,"VERSION %d %u\n", WINS_VERSION, 0);
 
	tdb_traverse(wins_tdb, wins_writedb_traverse_fn, fp);

	x_fclose(fp);
	chmod(fnamenew,0644);
	unlink(fname);
	rename(fnamenew,fname);
	if (background) {
		_exit(0);
	}
}

#if 0
	Until winsrepl is done.
/****************************************************************************
 Process a internal Samba message receiving a wins record.
***************************************************************************/

void nmbd_wins_new_entry(int msg_type, struct process_id src,
			 void *buf, size_t len)
{
	WINS_RECORD *record;
	struct name_record *namerec = NULL;
	struct name_record *new_namerec = NULL;
	struct nmb_name question;
	BOOL overwrite=False;
	struct in_addr our_fake_ip = *interpret_addr2("0.0.0.0");
	int i;

	if (buf==NULL) {
		return;
	}
	
	/* Record should use UNIX codepage. Ensure this is so in the wrepld code. JRA. */
	record=(WINS_RECORD *)buf;
	
	make_nmb_name(&question, record->name, record->type);

	namerec = find_name_on_subnet(wins_server_subnet, &question, FIND_ANY_NAME);

	/* record doesn't exist, add it */
	if (namerec == NULL) {
		DEBUG(3,("nmbd_wins_new_entry: adding new replicated record: %s<%02x> for wins server: %s\n", 
			  record->name, record->type, inet_ntoa(record->wins_ip)));

		new_namerec=add_name_to_subnet( wins_server_subnet,
						record->name,
						record->type,
						record->nb_flags, 
						EXTINCTION_INTERVAL,
						REGISTER_NAME,
						record->num_ips,
						record->ip);

		if (new_namerec!=NULL) {
				update_wins_owner(new_namerec, record->wins_ip);
				update_wins_flag(new_namerec, record->wins_flags);
				new_namerec->data.id=record->id;

				wins_server_subnet->namelist_changed = True;
			}
	}

	/* check if we have a conflict */
	if (namerec != NULL) {
		/* both records are UNIQUE */
		if (namerec->data.wins_flags&WINS_UNIQUE && record->wins_flags&WINS_UNIQUE) {

			/* the database record is a replica */
			if (!ip_equal(namerec->data.wins_ip, our_fake_ip)) {
				if (namerec->data.wins_flags&WINS_ACTIVE && record->wins_flags&WINS_TOMBSTONED) {
					if (ip_equal(namerec->data.wins_ip, record->wins_ip))
						overwrite=True;
				} else
					overwrite=True;
			} else {
			/* we are the wins owner of the database record */
				/* the 2 records have the same IP address */
				if (ip_equal(namerec->data.ip[0], record->ip[0])) {
					if (namerec->data.wins_flags&WINS_ACTIVE && record->wins_flags&WINS_TOMBSTONED)
						get_global_id_and_update(&namerec->data.id, True);
					else
						overwrite=True;
				
				} else {
				/* the 2 records have different IP address */
					if (namerec->data.wins_flags&WINS_ACTIVE) {
						if (record->wins_flags&WINS_TOMBSTONED)
							get_global_id_and_update(&namerec->data.id, True);
						if (record->wins_flags&WINS_ACTIVE)
							/* send conflict challenge to the replica node */
							;
					} else
						overwrite=True;
				}

			}
		}
		
		/* the replica is a standard group */
		if (record->wins_flags&WINS_NGROUP || record->wins_flags&WINS_SGROUP) {
			/* if the database record is unique and active force a name release */
			if (namerec->data.wins_flags&WINS_UNIQUE)
				/* send a release name to the unique node */
				;
			overwrite=True;
		
		}
	
		/* the replica is a special group */
		if (record->wins_flags&WINS_SGROUP && namerec->data.wins_flags&WINS_SGROUP) {
			if (namerec->data.wins_flags&WINS_ACTIVE) {
				for (i=0; i<record->num_ips; i++)
					if(!find_ip_in_name_record(namerec, record->ip[i]))
						add_ip_to_name_record(namerec, record->ip[i]);
			} else {
				overwrite=True;
			}
		}
		
		/* the replica is a multihomed host */
		
		/* I'm giving up on multi homed. Too much complex to understand */
		
		if (record->wins_flags&WINS_MHOMED) {
			if (! (namerec->data.wins_flags&WINS_ACTIVE)) {
				if ( !(namerec->data.wins_flags&WINS_RELEASED) && !(namerec->data.wins_flags&WINS_NGROUP))
					overwrite=True;
			}
			else {
				if (ip_equal(record->wins_ip, namerec->data.wins_ip))
					overwrite=True;
				
				if (ip_equal(namerec->data.wins_ip, our_fake_ip))
					if (namerec->data.wins_flags&WINS_UNIQUE)
						get_global_id_and_update(&namerec->data.id, True);
				
			}
			
			if (record->wins_flags&WINS_ACTIVE && namerec->data.wins_flags&WINS_ACTIVE)
				if (namerec->data.wins_flags&WINS_UNIQUE ||
				    namerec->data.wins_flags&WINS_MHOMED)
					if (ip_equal(record->wins_ip, namerec->data.wins_ip))
						overwrite=True;
				
		}

		if (overwrite == False)
			DEBUG(3, ("nmbd_wins_new_entry: conflict in adding record: %s<%02x> from wins server: %s\n", 
				  record->name, record->type, inet_ntoa(record->wins_ip)));
		else {
			DEBUG(3, ("nmbd_wins_new_entry: replacing record: %s<%02x> from wins server: %s\n", 
				  record->name, record->type, inet_ntoa(record->wins_ip)));

			/* remove the old record and add a new one */
			remove_name_from_namelist( wins_server_subnet, namerec );
			new_namerec=add_name_to_subnet( wins_server_subnet, record->name, record->type, record->nb_flags, 
						EXTINCTION_INTERVAL, REGISTER_NAME, record->num_ips, record->ip);
			if (new_namerec!=NULL) {
				update_wins_owner(new_namerec, record->wins_ip);
				update_wins_flag(new_namerec, record->wins_flags);
				new_namerec->data.id=record->id;

				wins_server_subnet->namelist_changed = True;
			}

			wins_server_subnet->namelist_changed = True;
		}

	}
}
#endif
