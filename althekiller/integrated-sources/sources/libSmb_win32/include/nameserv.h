#ifndef _NAMESERV_H_
#define _NAMESERV_H_
/* 
   Unix SMB/CIFS implementation.
   NBT netbios header - version 2
   Copyright (C) Andrew Tridgell 1994-1998
   
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

#define INFO_VERSION	"INFO/version"
#define INFO_COUNT	"INFO/num_entries"
#define INFO_ID_HIGH	"INFO/id_high"
#define INFO_ID_LOW	"INFO/id_low"
#define ENTRY_PREFIX 	"ENTRY/"

#define PERMANENT_TTL 0

/* NTAS uses 2, NT uses 1, WfWg uses 0 */
#define MAINTAIN_LIST    2
#define ELECTION_VERSION 1

#define MAX_DGRAM_SIZE (576) /* tcp/ip datagram limit is 576 bytes */
#define MIN_DGRAM_SIZE 12

/*********************************************************
 Types of reply packet.
**********************************************************/

enum netbios_reply_type_code { NMB_QUERY, NMB_STATUS, NMB_REG, NMB_REG_REFRESH,
                               NMB_REL, NMB_WAIT_ACK, NMB_MULTIHOMED_REG,
                               WINS_REG, WINS_QUERY };

/* From rfc1002, 4.2.1.2 */
/* Question types. */
#define QUESTION_TYPE_NB_QUERY  0x20
#define QUESTION_TYPE_NB_STATUS 0x21

/* Question class */
#define QUESTION_CLASS_IN  0x1

/* Opcode definitions */
#define NMB_NAME_QUERY_OPCODE       0x0
#define NMB_NAME_REG_OPCODE         0x05 /* see rfc1002.txt 4.2.2,3,5,6,7,8 */
#define NMB_NAME_RELEASE_OPCODE     0x06 /* see rfc1002.txt 4.2.9,10,11 */
#define NMB_WACK_OPCODE             0x07 /* see rfc1002.txt 4.2.16 */
/* Ambiguity in rfc1002 about which of these is correct. */
/* WinNT uses 8 by default but can be made to use 9. */
#define NMB_NAME_REFRESH_OPCODE_8   0x08 /* see rfc1002.txt 4.2.4 */
#define NMB_NAME_REFRESH_OPCODE_9   0x09 /* see rfc1002.txt 4.2.4 */
#define NMB_NAME_MULTIHOMED_REG_OPCODE 0x0F /* Invented by Microsoft. */

/* XXXX what about all the other types?? 0x1, 0x2, 0x3, 0x4, 0x8? */

/* Resource record types. rfc1002 4.2.1.3 */
#define RR_TYPE_A                  0x1
#define RR_TYPE_NS                 0x2
#define RR_TYPE_NULL               0xA
#define RR_TYPE_NB                0x20
#define RR_TYPE_NBSTAT            0x21

/* Resource record class. */
#define RR_CLASS_IN                0x1

/* NetBIOS flags */
#define NB_GROUP  0x80
#define NB_PERM   0x02
#define NB_ACTIVE 0x04
#define NB_CONFL  0x08
#define NB_DEREG  0x10
#define NB_BFLAG  0x00 /* Broadcast node type. */
#define NB_PFLAG  0x20 /* Point-to-point node type. */
#define NB_MFLAG  0x40 /* Mixed bcast & p-p node type. */
#define NB_HFLAG  0x60 /* Microsoft 'hybrid' node type. */
#define NB_NODETYPEMASK 0x60
/* Mask applied to outgoing NetBIOS flags. */
#define NB_FLGMSK 0xE0

/* The wins flags. Looks like the nbflags ! */
#define WINS_UNIQUE	0x00 /* Unique record */
#define WINS_NGROUP	0x01 /* Normal Group eg: 1B */
#define WINS_SGROUP	0x02 /* Special Group eg: 1C */
#define WINS_MHOMED	0x03 /* MultiHomed */

#define WINS_ACTIVE	0x00 /* active record */
#define WINS_RELEASED	0x04 /* released record */
#define WINS_TOMBSTONED 0x08 /* tombstoned record */
#define WINS_DELETED	0x0C /* deleted record */

#define WINS_STATE_MASK	0x0C

#define WINS_LOCAL	0x00 /* local record */
#define WINS_REMOTE	0x10 /* remote record */

#define WINS_BNODE	0x00 /* Broadcast node */
#define WINS_PNODE	0x20 /* PtP node */
#define WINS_MNODE	0x40 /* Mixed node */
#define WINS_HNODE	0x60 /* Hybrid node */

#define WINS_NONSTATIC	0x00 /* dynamic record */
#define WINS_STATIC	0x80 /* static record */

#define WINS_STATE_ACTIVE(p) (((p)->data.wins_flags & WINS_STATE_MASK) == WINS_ACTIVE)


/* NetBIOS flag identifier. */
#define NAME_GROUP(p)  ((p)->data.nb_flags & NB_GROUP)
#define NAME_BFLAG(p) (((p)->data.nb_flags & NB_NODETYPEMASK) == NB_BFLAG)
#define NAME_PFLAG(p) (((p)->data.nb_flags & NB_NODETYPEMASK) == NB_PFLAG)
#define NAME_MFLAG(p) (((p)->data.nb_flags & NB_NODETYPEMASK) == NB_MFLAG)
#define NAME_HFLAG(p) (((p)->data.nb_flags & NB_NODETYPEMASK) == NB_HFLAG)

/* Samba name state for a name in a namelist. */
#define NAME_IS_ACTIVE(p)        ((p)->data.nb_flags & NB_ACTIVE)
#define NAME_IN_CONFLICT(p)      ((p)->data.nb_flags & NB_CONFL)
#define NAME_IS_DEREGISTERING(p) ((p)->data.nb_flags & NB_DEREG)

/* Error codes for NetBIOS requests. */
#define FMT_ERR   0x1       /* Packet format error. */
#define SRV_ERR   0x2       /* Internal server error. */
#define NAM_ERR   0x3       /* Name does not exist. */
#define IMP_ERR   0x4       /* Request not implemented. */
#define RFS_ERR   0x5       /* Request refused. */
#define ACT_ERR   0x6       /* Active error - name owned by another host. */
#define CFT_ERR   0x7       /* Name in conflict error. */

#define REFRESH_TIME (15*60)
#define NAME_POLL_REFRESH_TIME (5*60)
#define NAME_POLL_INTERVAL 15

/* Workgroup state identifiers. */
#define AM_POTENTIAL_MASTER_BROWSER(work) ((work)->mst_state == MST_POTENTIAL)
#define AM_LOCAL_MASTER_BROWSER(work) ((work)->mst_state == MST_BROWSER)
#define AM_DOMAIN_MASTER_BROWSER(work) ((work)->dom_state == DOMAIN_MST)
#define AM_DOMAIN_MEMBER(work) ((work)->log_state == LOGON_SRV)

/* Microsoft browser NetBIOS name. */
#define MSBROWSE "\001\002__MSBROWSE__\002"

/* Mail slots. */
#define BROWSE_MAILSLOT    "\\MAILSLOT\\BROWSE"
#define NET_LOGON_MAILSLOT "\\MAILSLOT\\NET\\NETLOGON"
#define NT_LOGON_MAILSLOT  "\\MAILSLOT\\NET\\NTLOGON"
#define LANMAN_MAILSLOT    "\\MAILSLOT\\LANMAN"

/* Samba definitions for find_name_on_subnet(). */
#define FIND_ANY_NAME   0
#define FIND_SELF_NAME  1

/*
 * The different name types that can be in namelists.
 *
 * SELF_NAME should only be on the broadcast and unicast subnets.
 * LMHOSTS_NAME should only be in the remote_broadcast_subnet.
 * REGISTER_NAME, DNS_NAME, DNSFAIL_NAME should only be in the wins_server_subnet.
 * WINS_PROXY_NAME should only be on the broadcast subnets.
 * PERMANENT_NAME can be on all subnets except remote_broadcast_subnet.
 *
 */

enum name_source {LMHOSTS_NAME, REGISTER_NAME, SELF_NAME, DNS_NAME, 
                  DNSFAIL_NAME, PERMANENT_NAME, WINS_PROXY_NAME};
enum node_type {B_NODE=0, P_NODE=1, M_NODE=2, NBDD_NODE=3};
enum packet_type {NMB_PACKET, DGRAM_PACKET};

enum master_state {
	MST_NONE,
	MST_POTENTIAL,
	MST_BACKUP,
	MST_MSB,
	MST_BROWSER,
	MST_UNBECOMING_MASTER
};

enum domain_state {
	DOMAIN_NONE,
	DOMAIN_WAIT,
	DOMAIN_MST
};

enum logon_state {
	LOGON_NONE,
	LOGON_WAIT,
	LOGON_SRV
};

struct subnet_record;

struct nmb_data {
	uint16 nb_flags;         /* Netbios flags. */
	int num_ips;             /* Number of ip entries. */
	struct in_addr *ip;      /* The ip list for this name. */

	enum name_source source; /* Where the name came from. */

	time_t death_time; /* The time the record must be removed (do not remove if 0). */
	time_t refresh_time; /* The time the record should be refreshed. */
  
	SMB_BIG_UINT id;		/* unique id */
	struct in_addr wins_ip;	/* the adress of the wins server this record comes from */

	int wins_flags;		/* similar to the netbios flags but different ! */
};

/* This structure represents an entry in a local netbios name list. */
struct name_record {
	struct name_record *prev, *next;
	struct subnet_record *subnet;
	struct nmb_name       name;    /* The netbios name. */
	struct nmb_data       data;    /* The netbios data. */
};

/* Browser cache for synchronising browse lists. */
struct browse_cache_record {
	struct browse_cache_record *prev, *next;
	unstring        lmb_name;
	unstring        work_group;
	struct in_addr ip;
	time_t         sync_time;
	time_t         death_time; /* The time the record must be removed. */
};

/* This is used to hold the list of servers in my domain, and is
   contained within lists of domains. */

struct server_record {
	struct server_record *next;
	struct server_record *prev;

	struct subnet_record *subnet;

	struct server_info_struct serv;
	time_t death_time;  
};

/* A workgroup structure. It contains a list of servers. */
struct work_record {
	struct work_record *next;
	struct work_record *prev;

	struct subnet_record *subnet;

	struct server_record *serverlist;

	/* Stage of development from non-local-master up to local-master browser. */
	enum master_state mst_state;

	/* Stage of development from non-domain-master to domain-master browser. */
	enum domain_state dom_state;

  	/* Stage of development from non-logon-server to logon server. */
	enum logon_state log_state;

	/* Work group info. */
	unstring work_group;
	int     token;        /* Used when communicating with backup browsers. */
	unstring local_master_browser_name;      /* Current local master browser. */

	/* Announce info. */
	time_t lastannounce_time;
	int announce_interval;
	BOOL    needannounce;

	/* Timeout time for this workgroup. 0 means permanent. */
	time_t death_time;  

	/* Election info */
	BOOL    RunningElection;
	BOOL    needelection;
	int     ElectionCount;
	uint32  ElectionCriterion;

	/* Domain master browser info. Used for efficient syncs. */
	struct nmb_name dmb_name;
	struct in_addr dmb_addr;
};

/* typedefs needed to define copy & free functions for userdata. */
struct userdata_struct;

typedef struct userdata_struct * (*userdata_copy_fn)(struct userdata_struct *);
typedef void (*userdata_free_fn)(struct userdata_struct *);

/* Structure to define any userdata passed around. */

struct userdata_struct {
	userdata_copy_fn copy_fn;
	userdata_free_fn free_fn;
	unsigned int userdata_len;
	char data[16]; /* 16 is to ensure alignment/padding on all systems */
};

struct response_record;
struct packet_struct;
struct res_rec;

/* typedef to define the function called when this response packet comes in. */
typedef void (*response_function)(struct subnet_record *, struct response_record *,
                                  struct packet_struct *);

/* typedef to define the function called when this response record times out. */
typedef void (*timeout_response_function)(struct subnet_record *,
                                          struct response_record *);

/* typedef to define the function called when the request that caused this
   response record to be created is successful. */
typedef void (*success_function)(struct subnet_record *, struct userdata_struct *, ...);

/* typedef to define the function called when the request that caused this
   response record to be created is unsuccessful. */
typedef void (*fail_function)(struct subnet_record *, struct response_record *, ...);

/* List of typedefs for success and fail functions of the different query
   types. Used to catch any compile time prototype errors. */

typedef void (*register_name_success_function)( struct subnet_record *,
                                                struct userdata_struct *,
                                                struct nmb_name *,
                                                uint16,
                                                int,
                                                struct in_addr);
typedef void (*register_name_fail_function)( struct subnet_record *,
                                             struct response_record *,
                                             struct nmb_name *);

typedef void (*release_name_success_function)( struct subnet_record *,
                                               struct userdata_struct *, 
                                               struct nmb_name *,
                                               struct in_addr);
typedef void (*release_name_fail_function)( struct subnet_record *,
                                            struct response_record *, 
                                            struct nmb_name *);

typedef void (*refresh_name_success_function)( struct subnet_record *,
                                               struct userdata_struct *, 
                                               struct nmb_name *,
                                               uint16,
                                               int,
                                               struct in_addr);
typedef void (*refresh_name_fail_function)( struct subnet_record *,
                                            struct response_record *,
                                            struct nmb_name *);

typedef void (*query_name_success_function)( struct subnet_record *,
                                             struct userdata_struct *,
                                             struct nmb_name *,
                                             struct in_addr,
                                             struct res_rec *answers);

typedef void (*query_name_fail_function)( struct subnet_record *,
                                          struct response_record *,    
                                          struct nmb_name *,
                                          int);  

typedef void (*node_status_success_function)( struct subnet_record *,
                                              struct userdata_struct *,
                                              struct res_rec *,
                                              struct in_addr);
typedef void (*node_status_fail_function)( struct subnet_record *,
                                           struct response_record *);

/* Initiated name queries are recorded in this list to track any responses. */

struct response_record {
	struct response_record *next;
	struct response_record *prev;

	uint16 response_id;

	/* Callbacks for packets received or not. */ 
	response_function resp_fn;
	timeout_response_function timeout_fn;

	/* Callbacks for the request succeeding or not. */
	success_function success_fn;
	fail_function fail_fn;
 
	struct packet_struct *packet;

	struct userdata_struct *userdata;

	int num_msgs;

	time_t repeat_time;
	time_t repeat_interval;
	int    repeat_count;

	/* Recursion protection. */
	BOOL in_expiration_processing;
};

/* A subnet structure. It contains a list of workgroups and netbios names. */

/*
   B nodes will have their own, totally separate subnet record, with their
   own netbios name set. These do NOT interact with other subnet records'
   netbios names.
*/

enum subnet_type {
	NORMAL_SUBNET              = 0,  /* Subnet listed in interfaces list. */
	UNICAST_SUBNET             = 1,  /* Subnet for unicast packets. */
	REMOTE_BROADCAST_SUBNET    = 2,  /* Subnet for remote broadcasts. */
	WINS_SERVER_SUBNET         = 3   /* Only created if we are a WINS server. */
};

struct subnet_record {
	struct subnet_record *next;
	struct subnet_record *prev;

	char  *subnet_name;      /* For Debug identification. */
	enum subnet_type type;   /* To catagorize the subnet. */

	struct work_record     *workgrouplist; /* List of workgroups. */
	struct name_record     *namelist;   /* List of netbios names. */
	struct response_record *responselist;  /* List of responses expected. */

	BOOL namelist_changed;
	BOOL work_changed;

	struct in_addr bcast_ip;
	struct in_addr mask_ip;
	struct in_addr myip;
	int nmb_sock;               /* socket to listen for unicast 137. */
	int dgram_sock;             /* socket to listen for unicast 138. */
};

/* A resource record. */
struct res_rec {
	struct nmb_name rr_name;
	int rr_type;
	int rr_class;
	int ttl;
	int rdlength;
	char rdata[MAX_DGRAM_SIZE];
};

/* Define these so we can pass info back to caller of name_query */
#define NM_FLAGS_RS 0x80 /* Response. Cheat     */
#define NM_FLAGS_AA 0x40 /* Authoritative       */
#define NM_FLAGS_TC 0x20 /* Truncated           */
#define NM_FLAGS_RD 0x10 /* Recursion Desired   */
#define NM_FLAGS_RA 0x08 /* Recursion Available */
#define NM_FLAGS_B  0x01 /* Broadcast           */

/* An nmb packet. */
struct nmb_packet {
	struct {
		int name_trn_id;
		int opcode;
		BOOL response;
		struct {
			BOOL bcast;
			BOOL recursion_available;
			BOOL recursion_desired;
			BOOL trunc;
			BOOL authoritative;
		} nm_flags;
		int rcode;
		int qdcount;
		int ancount;
		int nscount;
		int arcount;
	} header;

	struct {
		struct nmb_name question_name;
		int question_type;
		int question_class;
	} question;

	struct res_rec *answers;
	struct res_rec *nsrecs;
	struct res_rec *additional;
};

/* msg_type field options - from rfc1002. */

#define DGRAM_UNIQUE 0x10
#define DGRAM_GROUP 0x11
#define DGRAM_BROADCAST 0x12
#define DGRAM_ERROR 0x13
#define DGRAM_QUERY_REQUEST 0x14
#define DGRAM_POSITIVE_QUERY_RESPONSE 0x15
#define DGRAM_NEGATIVE_QUERT_RESPONSE 0x16

/* A datagram - this normally contains SMB data in the data[] array. */

struct dgram_packet {
	struct {
		int msg_type;
		struct {
			enum node_type node_type;
			BOOL first;
			BOOL more;
		} flags;
		int dgm_id;
		struct in_addr source_ip;
		int source_port;
		int dgm_length;
		int packet_offset;
	} header;
	struct nmb_name source_name;
	struct nmb_name dest_name;
	int datasize;
	char data[MAX_DGRAM_SIZE];
};

/* Define a structure used to queue packets. This will be a linked
 list of nmb packets. */

struct packet_struct
{
	struct packet_struct *next;
	struct packet_struct *prev;
	BOOL locked;
	struct in_addr ip;
	int port;
	int fd;
	time_t timestamp;
	enum packet_type packet_type;
	union {
		struct nmb_packet nmb;
		struct dgram_packet dgram;
	} packet;
};

/* NETLOGON opcodes */

#define QUERYFORPDC	 7 /* Query for PDC. */
#define SAM_UAS_CHANGE  10 /* Announce change to UAS or SAM. */
#define QUERYFORPDC_R	12 /* Response to Query for PDC. */
#define SAMLOGON	18
#define SAMLOGON_R	19
#define SAMLOGON_UNK_R	21
#define SAMLOGON_AD_UNK_R 23
#define SAMLOGON_AD_R   25

/* Ids for netbios packet types. */

#define ANN_HostAnnouncement         1
#define ANN_AnnouncementRequest      2
#define ANN_Election                 8
#define ANN_GetBackupListReq         9
#define ANN_GetBackupListResp       10
#define ANN_BecomeBackup            11
#define ANN_DomainAnnouncement      12
#define ANN_MasterAnnouncement      13
#define ANN_ResetBrowserState       14
#define ANN_LocalMasterAnnouncement 15


/* Broadcast packet announcement intervals, in minutes. */

/* Attempt to add domain logon and domain master names. */
#define CHECK_TIME_ADD_DOM_NAMES 5 

/* Search for master browsers of workgroups samba knows about, 
   except default. */
#define CHECK_TIME_MST_BROWSE       5 

/* Request backup browser announcements from other servers. */
#define CHECK_TIME_ANNOUNCE_BACKUP 15

/* Request host announcements from other servers: min and max of interval. */
#define CHECK_TIME_MIN_HOST_ANNCE   3
#define CHECK_TIME_MAX_HOST_ANNCE  12

/* Announce as master to WINS server and any Primary Domain Controllers. */
#define CHECK_TIME_MST_ANNOUNCE    15

/* Time between syncs from domain master browser to local master browsers. */
#define CHECK_TIME_DMB_TO_LMB_SYNC    15

/* Do all remote announcements this often. */
#define REMOTE_ANNOUNCE_INTERVAL 180

/* what is the maximum period between name refreshes. Note that this only
   affects non-permanent self names (in seconds) */
#define MAX_REFRESH_TIME (60*20)

/* The Extinction interval: 4 days, time a node will stay in released state  */
#define EXTINCTION_INTERVAL (4*24*60*60)

/* The Extinction time-out: 1 day, time a node will stay in deleted state */
#define EXTINCTION_TIMEOUT (24*60*60)

/* Macro's to enumerate subnets either with or without
   the UNICAST subnet. */

extern struct subnet_record *subnetlist;
extern struct subnet_record *unicast_subnet;
extern struct subnet_record *wins_server_subnet;
extern struct subnet_record *remote_broadcast_subnet;

#define FIRST_SUBNET subnetlist
#define NEXT_SUBNET_EXCLUDING_UNICAST(x) ((x)->next)
#define NEXT_SUBNET_INCLUDING_UNICAST(x) (get_next_subnet_maybe_unicast((x)))

/* wins replication record used between nmbd and wrepld */
typedef struct _WINS_RECORD {
	char name[17];
	char type;
	int nb_flags;
	int wins_flags;
	SMB_BIG_UINT id;
	int num_ips;
	struct in_addr ip[25];
	struct in_addr wins_ip;
} WINS_RECORD;

/* To be removed. */
enum state_type { TEST };
#endif /* _NAMESERV_H_ */
