/* 
   Unix SMB/CIFS implementation.
   SMB parameters and setup
   Copyright (C) Andrew Tridgell 1992-1997
   Copyright (C) Luke Kenneth Casson Leighton 1996-1997
   Copyright (C) Paul Ashton 1997
   
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

#ifndef _DCE_RPC_H /* _DCE_RPC_H */
#define _DCE_RPC_H 

/* DCE/RPC packet types */

enum RPC_PKT_TYPE {
	RPC_REQUEST  = 0x00, 	/* Ordinary request. */
	RPC_PING     = 0x01,	/* Connectionless is server alive ? */
	RPC_RESPONSE = 0x02,	/* Ordinary reply. */
	RPC_FAULT    = 0x03,	/* Fault in processing of call. */
	RPC_WORKING  = 0x04,	/* Connectionless reply to a ping when server busy. */
	RPC_NOCALL   = 0x05,	/* Connectionless reply to a ping when server has lost part of clients call. */
	RPC_REJECT   = 0x06,	/* Refuse a request with a code. */
	RPC_ACK      = 0x07,	/* Connectionless client to server code. */
	RPC_CL_CANCEL= 0x08,	/* Connectionless cancel. */
	RPC_FACK     = 0x09,	/* Connectionless fragment ack. Both client and server send. */
	RPC_CANCEL_ACK = 0x0A,	/* Server ACK to client cancel request. */
	RPC_BIND     = 0x0B,	/* Bind to interface. */
	RPC_BINDACK  = 0x0C,	/* Server ack of bind. */
	RPC_BINDNACK = 0x0D,	/* Server nack of bind. */
	RPC_ALTCONT  = 0x0E,	/* Alter auth. */
	RPC_ALTCONTRESP = 0x0F,	/* Reply to alter auth. */
	RPC_AUTH3    = 0x10, 	/* not the real name!  this is undocumented! */
	RPC_SHUTDOWN = 0x11,	/* Server to client request to shutdown. */
	RPC_CO_CANCEL= 0x12,	/* Connection-oriented cancel request. */
	RPC_ORPHANED = 0x13	/* Client telling server it's aborting a partially sent request or telling
				   server to stop sending replies. */
};

/* DCE/RPC flags */
#define RPC_FLG_FIRST 0x01
#define RPC_FLG_LAST  0x02
#define RPC_FLG_NOCALL 0x20


#define SMBD_NTLMSSP_NEG_FLAGS 0x000082b1 /* ALWAYS_SIGN|NEG_NTLM|NEG_LM|NEG_SEAL|NEG_SIGN|NEG_UNICODE */

/* NTLMSSP signature version */
#define NTLMSSP_SIGN_VERSION 0x01

/* DCE RPC auth types - extended by Microsoft. */
#define RPC_ANONYMOUS_AUTH_TYPE    0
#define RPC_AUTH_TYPE_KRB5_1	   1
#define RPC_SPNEGO_AUTH_TYPE       9 
#define RPC_NTLMSSP_AUTH_TYPE     10
#define RPC_KRB5_AUTH_TYPE        16 /* Not yet implemented. */ 
#define RPC_SCHANNEL_AUTH_TYPE    68 /* 0x44 */

/* DCE-RPC standard identifiers to indicate 
   signing or sealing of an RPC pipe */
#define RPC_AUTH_LEVEL_NONE      1
#define RPC_AUTH_LEVEL_CONNECT   2
#define RPC_AUTH_LEVEL_CALL      3
#define RPC_AUTH_LEVEL_PACKET    4
#define RPC_AUTH_LEVEL_INTEGRITY 5
#define RPC_AUTH_LEVEL_PRIVACY   6

#if 0
#define RPC_PIPE_AUTH_SIGN_LEVEL 0x5
#define RPC_PIPE_AUTH_SEAL_LEVEL 0x6
#endif

#define DCERPC_FAULT_OP_RNG_ERROR	0x1c010002
#define DCERPC_FAULT_UNK_IF		0x1c010003
#define DCERPC_FAULT_INVALID_TAG	0x1c000006
#define DCERPC_FAULT_CONTEXT_MISMATCH	0x1c00001a
#define DCERPC_FAULT_OTHER		0x00000001
#define DCERPC_FAULT_ACCESS_DENIED	0x00000005
#define DCERPC_FAULT_CANT_PERFORM	0x000006d8
#define DCERPC_FAULT_NDR		0x000006f7


/* Netlogon schannel auth type and level */
#define SCHANNEL_SIGN_SIGNATURE { 0x77, 0x00, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00 }
#define SCHANNEL_SEAL_SIGNATURE { 0x77, 0x00, 0x7a, 0x00, 0xff, 0xff, 0x00, 0x00 }

#define RPC_AUTH_SCHANNEL_SIGN_OR_SEAL_CHK_LEN 	0x20
#define RPC_AUTH_SCHANNEL_SIGN_ONLY_CHK_LEN 	0x18


#define NETLOGON_NEG_ARCFOUR			0x00000004
#define NETLOGON_NEG_128BIT			0x00004000
#define NETLOGON_NEG_SCHANNEL			0x40000000

/* The 7 here seems to be required to get Win2k not to downgrade us
   to NT4.  Actually, anything other than 1ff would seem to do... */
#define NETLOGON_NEG_AUTH2_FLAGS 0x000701ff
#define NETLOGON_NEG_DOMAIN_TRUST_ACCOUNT	0x2010b000
 
/* these are the flags that ADS clients use */
#define NETLOGON_NEG_AUTH2_ADS_FLAGS (0x200fbffb | NETLOGON_NEG_ARCFOUR | NETLOGON_NEG_128BIT | NETLOGON_NEG_SCHANNEL)

enum schannel_direction {
	SENDER_IS_INITIATOR,
	SENDER_IS_ACCEPTOR
};

/* Maximum size of the signing data in a fragment. */
#define RPC_MAX_SIGN_SIZE 0x20 /* 32 */

/* Maximum PDU fragment size. */
/* #define MAX_PDU_FRAG_LEN 0x1630		this is what wnt sets */
#define RPC_MAX_PDU_FRAG_LEN 0x10b8			/* this is what w2k sets */

/* RPC_IFACE */
typedef struct rpc_iface_info {
	struct uuid uuid;  /* 16 bytes of rpc interface identification */
	uint32 version;    /* the interface version number */
} RPC_IFACE;

#define RPC_IFACE_LEN (UUID_SIZE + 4)

struct pipe_id_info {
	/* the names appear not to matter: the syntaxes _do_ matter */

	const char *client_pipe;
	RPC_IFACE abstr_syntax; /* this one is the abstract syntax id */

	const char *server_pipe;  /* this one is the secondary syntax name */
	RPC_IFACE trans_syntax; /* this one is the primary syntax id */
};

/* RPC_HDR - dce rpc header */
typedef struct rpc_hdr_info {
	uint8  major; /* 5 - RPC major version */
	uint8  minor; /* 0 - RPC minor version */
	uint8  pkt_type; /* RPC_PKT_TYPE - RPC response packet */
	uint8  flags; /* DCE/RPC flags */
	uint8  pack_type[4]; /* 0x1000 0000 - little-endian packed data representation */
	uint16 frag_len; /* fragment length - data size (bytes) inc header and tail. */
	uint16 auth_len; /* 0 - authentication length  */
	uint32 call_id; /* call identifier.  matches 12th uint32 of incoming RPC data. */
} RPC_HDR;

#define RPC_HEADER_LEN 16

/* RPC_HDR_REQ - ms request rpc header */
typedef struct rpc_hdr_req_info {
	uint32 alloc_hint;   /* allocation hint - data size (bytes) minus header and tail. */
	uint16 context_id;   /* presentation context identifier */
	uint16  opnum;       /* opnum */
} RPC_HDR_REQ;

#define RPC_HDR_REQ_LEN 8

/* RPC_HDR_RESP - ms response rpc header */
typedef struct rpc_hdr_resp_info {
	uint32 alloc_hint;   /* allocation hint - data size (bytes) minus header and tail. */
	uint16 context_id;   /* 0 - presentation context identifier */
	uint8  cancel_count; /* 0 - cancel count */
	uint8  reserved;     /* 0 - reserved. */
} RPC_HDR_RESP;

#define RPC_HDR_RESP_LEN 8

/* RPC_HDR_FAULT - fault rpc header */
typedef struct rpc_hdr_fault_info {
	NTSTATUS status;
	uint32 reserved; /* 0x0000 0000 */
} RPC_HDR_FAULT;

#define RPC_HDR_FAULT_LEN 8

/* this seems to be the same string name depending on the name of the pipe,
 * but is more likely to be linked to the interface name
 * "srvsvc", "\\PIPE\\ntsvcs"
 * "samr", "\\PIPE\\lsass"
 * "wkssvc", "\\PIPE\\wksvcs"
 * "NETLOGON", "\\PIPE\\NETLOGON"
 */
/* RPC_ADDR_STR */
typedef struct rpc_addr_info {
	uint16 len;   /* length of the string including null terminator */
	fstring str; /* the string above in single byte, null terminated form */
} RPC_ADDR_STR;

/* RPC_HDR_BBA - bind acknowledge, and alter context response. */
typedef struct rpc_hdr_bba_info {
	uint16 max_tsize;       /* maximum transmission fragment size (0x1630) */
	uint16 max_rsize;       /* max receive fragment size (0x1630) */
	uint32 assoc_gid;       /* associated group id (0x0) */
} RPC_HDR_BBA;

#define RPC_HDR_BBA_LEN 8

/* RPC_HDR_AUTH */
typedef struct rpc_hdr_auth_info {
	uint8 auth_type; /* See XXX_AUTH_TYPE above. */
	uint8 auth_level; /* See RPC_PIPE_AUTH_XXX_LEVEL above. */
	uint8 auth_pad_len;
	uint8 auth_reserved;
	uint32 auth_context_id;
} RPC_HDR_AUTH;

#define RPC_HDR_AUTH_LEN 8

/* this is TEMPORARILY coded up as a specific structure */
/* this structure comes after the bind request */
/* RPC_AUTH_SCHANNEL_NEG */
typedef struct rpc_auth_schannel_neg_info {
	uint32 type1; 	/* Always zero ? */
	uint32 type2;	/* Types 0x3 and 0x13 seen. Check AcquireSecurityContext() docs.... */
	fstring domain; /* calling workstations's domain */
	fstring myname; /* calling workstation's name */
} RPC_AUTH_SCHANNEL_NEG;

/* attached to the end of encrypted rpc requests and responses */
/* RPC_AUTH_SCHANNEL_CHK */
typedef struct rpc_auth_schannel_chk_info {
	uint8 sig  [8]; /* 77 00 7a 00 ff ff 00 00 */
	uint8 packet_digest[8]; /* checksum over the packet, MD5'ed with session key */
	uint8 seq_num[8]; /* verifier, seq num */
	uint8 confounder[8]; /* random 8-byte nonce */
} RPC_AUTH_SCHANNEL_CHK;

typedef struct rpc_context {
	uint16 context_id;		/* presentation context identifier. */
	uint8 num_transfer_syntaxes;	/* the number of syntaxes */
	RPC_IFACE abstract;		/* num and vers. of interface client is using */
	RPC_IFACE *transfer;		/* Array of transfer interfaces. */
} RPC_CONTEXT;

/* RPC_BIND_REQ - ms req bind */
typedef struct rpc_bind_req_info {
	RPC_HDR_BBA bba;
	uint8 num_contexts;    /* the number of contexts */
	RPC_CONTEXT *rpc_context;
} RPC_HDR_RB;

/* 
 * The following length is 8 bytes RPC_HDR_BBA_LEN + 
 * 4 bytes size of context count +
 * (context_count * (4 bytes of context_id, size of transfer syntax count + RPC_IFACE_LEN bytes +
 *                    (transfer_syntax_count * RPC_IFACE_LEN bytes)))
 */

#define RPC_HDR_RB_LEN(rpc_hdr_rb) (RPC_HDR_BBA_LEN + 4 + \
	((rpc_hdr_rb)->num_contexts) * (4 + RPC_IFACE_LEN + (((rpc_hdr_rb)->rpc_context->num_transfer_syntaxes)*RPC_IFACE_LEN)))

/* RPC_RESULTS - can only cope with one reason, right now... */
typedef struct rpc_results_info {
	/* uint8[] # 4-byte alignment padding, against SMB header */

	uint8 num_results; /* the number of results (0x01) */

	/* uint8[] # 4-byte alignment padding, against SMB header */

	uint16 result; /* result (0x00 = accept) */
	uint16 reason; /* reason (0x00 = no reason specified) */
} RPC_RESULTS;

/* RPC_HDR_BA */
typedef struct rpc_hdr_ba_info {
	RPC_HDR_BBA bba;

	RPC_ADDR_STR addr    ;  /* the secondary address string, as described earlier */
	RPC_RESULTS  res     ; /* results and reasons */
	RPC_IFACE    transfer; /* the transfer syntax from the request */
} RPC_HDR_BA;

/* RPC_AUTH_VERIFIER */
typedef struct rpc_auth_verif_info {
	fstring signature; /* "NTLMSSP".. Ok, not quite anymore */
	uint32  msg_type; /* NTLMSSP_MESSAGE_TYPE (1,2,3) and 5 for schannel */
} RPC_AUTH_VERIFIER;

#endif /* _DCE_RPC_H */
