/* 
   Samba Unix/Linux SMB client library 
   Distributed SMB/CIFS Server Management Utility 
   Copyright (C) 2001 Andrew Bartlett (abartlet@samba.org)

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
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

/* 
 * A function of this type is passed to the '
 * run_rpc_command' wrapper.  Must go before the net_proto.h 
 * include
 */

typedef NTSTATUS (*rpc_command_fn)(const DOM_SID *,
				const char *, 
				struct cli_state *cli,
				struct rpc_pipe_client *,
				TALLOC_CTX *,
				int,
				const char **);

typedef struct copy_clistate {
	TALLOC_CTX *mem_ctx;
	struct cli_state *cli_share_src;
	struct cli_state *cli_share_dst;
	char *cwd;
	uint16 attribute;
}copy_clistate;

struct rpc_sh_ctx {
	struct cli_state *cli;

	DOM_SID *domain_sid;
	char *domain_name;

	const char *whoami;
	const char *thiscmd;
	struct rpc_sh_cmd *cmds;
	struct rpc_sh_ctx *parent;
};

struct rpc_sh_cmd {
	const char *name;
	struct rpc_sh_cmd *(*sub)(TALLOC_CTX *mem_ctx,
				  struct rpc_sh_ctx *ctx);
	int pipe_idx;
	NTSTATUS (*fn)(TALLOC_CTX *mem_ctx, struct rpc_sh_ctx *ctx,
		       struct rpc_pipe_client *pipe_hnd,
		       int argc, const char **argv);
	const char *help;
};

enum netdom_domain_t { ND_TYPE_NT4, ND_TYPE_AD };

/* INCLUDE FILES */

#include "utils/net_proto.h"
 
/* MACROS & DEFINES */

#define NET_FLAGS_MASTER 			0x00000001
#define NET_FLAGS_DMB 				0x00000002
#define NET_FLAGS_LOCALHOST_DEFAULT_INSANE 	0x00000004 	/* Would it be insane to set 'localhost'
								   as the default remote host for this
								   operation?  For example, localhost
								   is insane for a 'join' operation.  */
#define NET_FLAGS_PDC 				0x00000008	/* PDC only */ 
#define NET_FLAGS_ANONYMOUS 			0x00000010	/* use an anonymous connection */
#define NET_FLAGS_NO_PIPE 			0x00000020	/* don't open an RPC pipe */

/* net share operation modes */
#define NET_MODE_SHARE_MIGRATE 1

extern int opt_maxusers;
extern const char *opt_comment;
extern const char *opt_container;
extern int opt_flags;

extern const char *opt_comment;

extern const char *opt_target_workgroup;
extern const char *opt_workgroup;
extern int opt_long_list_entries;
extern int opt_verbose;
extern int opt_reboot;
extern int opt_force;
extern int opt_machine_pass;
extern int opt_timeout;
extern const char *opt_host;
extern const char *opt_user_name;
extern const char *opt_password;
extern BOOL opt_user_specified;

extern BOOL opt_localgroup;
extern BOOL opt_domaingroup;
extern const char *opt_newntname;
extern int opt_rid;
extern int opt_acls;
extern int opt_attrs;
extern int opt_timestamps;
extern const char *opt_exclude;
extern const char *opt_destination;

extern BOOL opt_have_ip;
extern struct in_addr opt_dest_ip;

extern const char *share_type[];

/* Structure for mapping accounts to groups */
/* Array element is the group rid */
typedef struct _groupmap {
	uint32 rid;
	uint32 gidNumber;
	fstring sambaSID;
	fstring group_dn;
} GROUPMAP;

typedef struct _accountmap {
	uint32 rid;
	fstring cn;
} ACCOUNTMAP;
