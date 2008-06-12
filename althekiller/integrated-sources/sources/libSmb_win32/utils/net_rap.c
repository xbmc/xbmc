/* 
   Samba Unix/Linux SMB client library 
   Distributed SMB/CIFS Server Management Utility 
   Copyright (C) 2001 Steve French  (sfrench@us.ibm.com)
   Copyright (C) 2001 Jim McDonough (jmcd@us.ibm.com)
   Copyright (C) 2001 Andrew Tridgell (tridge@samba.org)
   Copyright (C) 2001 Andrew Bartlett (abartlet@samba.org)

   Originally written by Steve and Jim. Largely rewritten by tridge in
   November 2001.

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

#include "includes.h"
#include "utils/net.h"

/* The following messages were for error checking that is not properly 
   reported at the moment.  Which should be reinstated? */
#define ERRMSG_TARGET_WG_NOT_VALID      "\nTarget workgroup option not valid "\
					"except on net rap server command, ignored"
#define ERRMSG_INVALID_HELP_OPTION	"\nInvalid help option\n"

#define ERRMSG_BOTH_SERVER_IPADDRESS    "\nTarget server and IP address both "\
  "specified. Do not set both at the same time.  The target IP address was used\n"

const char *share_type[] = {
  "Disk",
  "Print",
  "Dev",
  "IPC"
};

static int errmsg_not_implemented(void)
{
	d_printf("\nNot implemented\n");
	return 0;
}

int net_rap_file_usage(int argc, const char **argv)
{
	return net_help_file(argc, argv);
}

/***************************************************************************
  list info on an open file
***************************************************************************/
static void file_fn(const char * pPath, const char * pUser, uint16 perms, 
		    uint16 locks, uint32 id)
{
	d_printf("%-7.1d %-20.20s 0x%-4.2x %-6.1d %s\n",
		 id, pUser, perms, locks, pPath);
}

static void one_file_fn(const char *pPath, const char *pUser, uint16 perms, 
			uint16 locks, uint32 id)
{
	d_printf("File ID          %d\n"\
		 "User name        %s\n"\
		 "Locks            0x%-4.2x\n"\
		 "Path             %s\n"\
		 "Permissions      0x%x\n",
		 id, pUser, locks, pPath, perms);
}


static int rap_file_close(int argc, const char **argv)
{
	struct cli_state *cli;
	int ret;
	if (argc == 0) {
		d_printf("\nMissing fileid of file to close\n\n");
		return net_rap_file_usage(argc, argv);
	}

	if (!(cli = net_make_ipc_connection(0))) 
                return -1;

	ret = cli_NetFileClose(cli, atoi(argv[0]));
	cli_shutdown(cli);
	return ret;
}

static int rap_file_info(int argc, const char **argv)
{
	struct cli_state *cli;
	int ret;
	if (argc == 0)
		return net_rap_file_usage(argc, argv);
	
	if (!(cli = net_make_ipc_connection(0))) 
                return -1;

	ret = cli_NetFileGetInfo(cli, atoi(argv[0]), one_file_fn);
	cli_shutdown(cli);
	return ret;
}

static int rap_file_user(int argc, const char **argv)
{
	if (argc == 0)
		return net_rap_file_usage(argc, argv);

	d_fprintf(stderr, "net rap file user not implemented yet\n");
	return -1;
}

int net_rap_file(int argc, const char **argv)
{
	struct functable func[] = {
		{"CLOSE", rap_file_close},
		{"USER", rap_file_user},
		{"INFO", rap_file_info},
		{NULL, NULL}
	};
	
	if (argc == 0) {
		struct cli_state *cli;
		int ret;
		
		if (!(cli = net_make_ipc_connection(0))) 
                        return -1;

		/* list open files */
		d_printf(
		 "\nEnumerating open files on remote server:\n\n"\
		 "\nFileId  Opened by            Perms  Locks  Path \n"\
		 "------  ---------            -----  -----  ---- \n");
		ret = cli_NetFileEnum(cli, NULL, NULL, file_fn);
		cli_shutdown(cli);
		return ret;
	}
	
	return net_run_function(argc, argv, func, net_rap_file_usage);
}
		       
int net_rap_share_usage(int argc, const char **argv)
{
	return net_help_share(argc, argv);
}

static void long_share_fn(const char *share_name, uint32 type, 
			  const char *comment, void *state)
{
	d_printf("%-12s %-8.8s %-50s\n",
		 share_name, share_type[type], comment);
}

static void share_fn(const char *share_name, uint32 type, 
		     const char *comment, void *state)
{
	d_printf("%s\n", share_name);
}

static int rap_share_delete(int argc, const char **argv)
{
	struct cli_state *cli;
	int ret;
	
	if (argc == 0) {
		d_printf("\n\nShare name not specified\n");
		return net_rap_share_usage(argc, argv);
	}

	if (!(cli = net_make_ipc_connection(0))) 
                return -1;

	ret = cli_NetShareDelete(cli, argv[0]);
	cli_shutdown(cli);
	return ret;
}

static int rap_share_add(int argc, const char **argv)
{
	struct cli_state *cli;
	int ret;
	
	RAP_SHARE_INFO_2 sinfo;
	char *p;
	char *sharename;

	if (argc == 0) {
		d_printf("\n\nShare name not specified\n");
		return net_rap_share_usage(argc, argv);
	}
			
	if (!(cli = net_make_ipc_connection(0))) 
                return -1;

	sharename = SMB_STRDUP(argv[0]);
	p = strchr(sharename, '=');
	if (p == NULL) {
		d_printf("Server path not specified\n");
		return net_rap_share_usage(argc, argv);
	}
	*p = 0;
	strlcpy(sinfo.share_name, sharename, sizeof(sinfo.share_name));
	sinfo.reserved1 = '\0';
	sinfo.share_type = 0;
	sinfo.comment = smb_xstrdup(opt_comment);
	sinfo.perms = 0;
	sinfo.maximum_users = opt_maxusers;
	sinfo.active_users = 0;
	sinfo.path = p+1;
	memset(sinfo.password, '\0', sizeof(sinfo.password));
	sinfo.reserved2 = '\0';
	
	ret = cli_NetShareAdd(cli, &sinfo);
	cli_shutdown(cli);
	return ret;
}


int net_rap_share(int argc, const char **argv)
{
	struct functable func[] = {
		{"DELETE", rap_share_delete},
		{"CLOSE", rap_share_delete},
		{"ADD", rap_share_add},
		{NULL, NULL}
	};

	if (argc == 0) {
		struct cli_state *cli;
		int ret;
		
		if (!(cli = net_make_ipc_connection(0))) 
			return -1;
		
		if (opt_long_list_entries) {
			d_printf(
	"\nEnumerating shared resources (exports) on remote server:\n\n"\
	"\nShare name   Type     Description\n"\
	"----------   ----     -----------\n");
			ret = cli_RNetShareEnum(cli, long_share_fn, NULL);
		} else {
			ret = cli_RNetShareEnum(cli, share_fn, NULL);
		}
		cli_shutdown(cli);
		return ret;
	}

	return net_run_function(argc, argv, func, net_rap_share_usage);
}
		    
		
int net_rap_session_usage(int argc, const char **argv)
{
	d_printf(
	 "\nnet rap session [misc. options] [targets]"\
	 "\n\tenumerates all active SMB/CIFS sessions on target server\n");
	d_printf(
	 "\nnet rap session DELETE <client_name> [misc. options] [targets] \n"\
	 "\tor"\
	 "\nnet rap session CLOSE <client_name> [misc. options] [targets]"\
	 "\n\tDeletes (closes) a session from specified client to server\n");
	d_printf(
	"\nnet rap session INFO <client_name>"\
	"\n\tEnumerates all open files in specified session\n");

	net_common_flags_usage(argc, argv);
	return -1;
}
    
static void list_sessions_func(char *wsname, char *username, uint16 conns,
			uint16 opens, uint16 users, uint32 sess_time,
			uint32 idle_time, uint32 user_flags, char *clitype)
{
	int hrs = idle_time / 3600;
	int min = (idle_time / 60) % 60;
	int sec = idle_time % 60;
	
	d_printf("\\\\%-18.18s %-20.20s %-18.18s %5d %2.2d:%2.2d:%2.2d\n",
		 wsname, username, clitype, opens, hrs, min, sec);
}

static void display_session_func(const char *wsname, const char *username, 
				 uint16 conns, uint16 opens, uint16 users, 
				 uint32 sess_time, uint32 idle_time, 
				 uint32 user_flags, const char *clitype)
{
	int ihrs = idle_time / 3600;
	int imin = (idle_time / 60) % 60;
	int isec = idle_time % 60;
	int shrs = sess_time / 3600;
	int smin = (sess_time / 60) % 60;
	int ssec = sess_time % 60;
	d_printf("User name       %-20.20s\n"\
		 "Computer        %-20.20s\n"\
		 "Guest logon     %-20.20s\n"\
		 "Client Type     %-40.40s\n"\
		 "Sess time       %2.2d:%2.2d:%2.2d\n"\
		 "Idle time       %2.2d:%2.2d:%2.2d\n", 
		 username, wsname, 
		 (user_flags&0x0)?"yes":"no", clitype,
		 shrs, smin, ssec, ihrs, imin, isec);
}

static void display_conns_func(uint16 conn_id, uint16 conn_type, uint16 opens,
			       uint16 users, uint32 conn_time,
			       const char *username, const char *netname)
{
	d_printf("%-14.14s %-8.8s %5d\n",
		 netname, share_type[conn_type], opens);
}

static int rap_session_info(int argc, const char **argv)
{
	const char *sessname;
	struct cli_state *cli;
	int ret;
	
	if (!(cli = net_make_ipc_connection(0))) 
                return -1;

	if (argc == 0) 
                return net_rap_session_usage(argc, argv);

	sessname = argv[0];

	ret = cli_NetSessionGetInfo(cli, sessname, display_session_func);
	if (ret < 0) {
		cli_shutdown(cli);
                return ret;
	}

	d_printf("Share name     Type     # Opens\n-------------------------"\
		 "-----------------------------------------------------\n");
	ret = cli_NetConnectionEnum(cli, sessname, display_conns_func);
	cli_shutdown(cli);
	return ret;
}

static int rap_session_delete(int argc, const char **argv)
{
	struct cli_state *cli;
	int ret;
	
	if (!(cli = net_make_ipc_connection(0))) 
                return -1;

	if (argc == 0) 
                return net_rap_session_usage(argc, argv);

	ret = cli_NetSessionDel(cli, argv[0]);
	cli_shutdown(cli);
	return ret;
}

int net_rap_session(int argc, const char **argv)
{
	struct functable func[] = {
		{"INFO", rap_session_info},
		{"DELETE", rap_session_delete},
		{"CLOSE", rap_session_delete},
		{NULL, NULL}
	};

	if (argc == 0) {
		struct cli_state *cli;
		int ret;
		
		if (!(cli = net_make_ipc_connection(0))) 
			return -1;

		d_printf("Computer             User name            "\
			 "Client Type        Opens Idle time\n"\
			 "------------------------------------------"\
			 "------------------------------------\n");
		ret = cli_NetSessionEnum(cli, list_sessions_func);

		cli_shutdown(cli);
		return ret;
	}

	return net_run_function(argc, argv, func, net_rap_session_usage);
}
	
/****************************************************************************
list a server name
****************************************************************************/
static void display_server_func(const char *name, uint32 m,
				const char *comment, void * reserved)
{
	d_printf("\t%-16.16s     %s\n", name, comment);
}


int net_rap_server_usage(int argc, const char **argv)
{
	d_printf("net rap server [misc. options] [target]\n\t"\
		 "lists the servers in the specified domain or workgroup.\n");
	d_printf("\n\tIf domain is not specified, it uses the current"\
		 " domain or workgroup as\n\tthe default.\n");

	net_common_flags_usage(argc, argv);
	return -1;
}
		    
int net_rap_server(int argc, const char **argv)
{
	struct cli_state *cli;
	int ret;
	
	if (!(cli = net_make_ipc_connection(0))) 
                return -1;

	d_printf("\nEnumerating servers in this domain or workgroup: \n\n"\
		 "\tServer name          Server description\n"\
		 "\t-------------        ----------------------------\n");

	ret = cli_NetServerEnum(cli, cli->server_domain, SV_TYPE_ALL, 
				display_server_func,NULL); 
	cli_shutdown(cli);
	return ret;	
}
		      
int net_rap_domain_usage(int argc, const char **argv)
{
	d_printf("net rap domain [misc. options] [target]\n\tlists the"\
		 " domains or workgroups visible on the current network\n");

	net_common_flags_usage(argc, argv);
	return -1;
}

		  
int net_rap_domain(int argc, const char **argv)
{
	struct cli_state *cli;
	int ret;
	
	if (!(cli = net_make_ipc_connection(0))) 
                return -1;

	d_printf("\nEnumerating domains:\n\n"\
		 "\tDomain name          Server name of Browse Master\n"\
		 "\t-------------        ----------------------------\n");

	ret = cli_NetServerEnum(cli, cli->server_domain, SV_TYPE_DOMAIN_ENUM,
				display_server_func,NULL);	
	cli_shutdown(cli);
	return ret;	
}
		      
int net_rap_printq_usage(int argc, const char **argv)
{
	d_printf(
	 "net rap printq [misc. options] [targets]\n"\
	 "\tor\n"\
	 "net rap printq list [<queue_name>] [misc. options] [targets]\n"\
	 "\tlists the specified queue and jobs on the target server.\n"\
	 "\tIf the queue name is not specified, all queues are listed.\n\n");
	d_printf(
	 "net rap printq delete [<queue name>] [misc. options] [targets]\n"\
	 "\tdeletes the specified job number on the target server, or the\n"\
	 "\tprinter queue if no job number is specified\n");

	net_common_flags_usage(argc, argv);

	return -1;
}	

static void enum_queue(const char *queuename, uint16 pri, uint16 start, 
		       uint16 until, const char *sep, const char *pproc, 
		       const char *dest, const char *qparms, 
		       const char *qcomment, uint16 status, uint16 jobcount) 
{
	d_printf("%-17.17s Queue %5d jobs                      ",
		 queuename, jobcount);

	switch (status) {
	case 0:
		d_printf("*Printer Active*\n");
		break;
	case 1:
		d_printf("*Printer Paused*\n");
		break;
	case 2:
		d_printf("*Printer error*\n");
		break;
	case 3:
		d_printf("*Delete Pending*\n");
		break;
	default:
		d_printf("**UNKNOWN STATUS**\n");
	}
}

static void enum_jobs(uint16 jobid, const char *ownername, 
		      const char *notifyname, const char *datatype,
		      const char *jparms, uint16 pos, uint16 status, 
		      const char *jstatus, unsigned int submitted, unsigned int jobsize, 
		      const char *comment)
{
	d_printf("     %-23.23s %5d %9d            ",
		 ownername, jobid, jobsize);
	switch (status) {
	case 0:
		d_printf("Waiting\n");
		break;
	case 1:
		d_printf("Held in queue\n");
		break;
	case 2:
		d_printf("Spooling\n");
		break;
	case 3:
		d_printf("Printing\n");
		break;
	default:
		d_printf("**UNKNOWN STATUS**\n");
	}
}

#define PRINTQ_ENUM_DISPLAY \
    "Print queues at \\\\%s\n\n"\
    "Name                         Job #      Size            Status\n\n"\
    "------------------------------------------------------------------"\
    "-------------\n"

static int rap_printq_info(int argc, const char **argv)
{
	struct cli_state *cli;
	int ret;
	
	if (argc == 0) 
                return net_rap_printq_usage(argc, argv);

	if (!(cli = net_make_ipc_connection(0))) 
                return -1;

	d_printf(PRINTQ_ENUM_DISPLAY, cli->desthost); /* list header */
	ret = cli_NetPrintQGetInfo(cli, argv[0], enum_queue, enum_jobs);
	cli_shutdown(cli);
	return ret;
}

static int rap_printq_delete(int argc, const char **argv)
{
	struct cli_state *cli;
	int ret;
	
	if (argc == 0) 
                return net_rap_printq_usage(argc, argv);

	if (!(cli = net_make_ipc_connection(0))) 
                return -1;

	ret = cli_printjob_del(cli, atoi(argv[0]));
	cli_shutdown(cli);
	return ret;
}

int net_rap_printq(int argc, const char **argv)
{
	struct cli_state *cli;
	int ret;
	
	struct functable func[] = {
		{"INFO", rap_printq_info},
		{"DELETE", rap_printq_delete},
		{NULL, NULL}
	};

	if (argc == 0) {
		if (!(cli = net_make_ipc_connection(0))) 
			return -1;

		d_printf(PRINTQ_ENUM_DISPLAY, cli->desthost); /* list header */
		ret = cli_NetPrintQEnum(cli, enum_queue, enum_jobs);
		cli_shutdown(cli);
		return ret;
	}

	return net_run_function(argc, argv, func, net_rap_printq_usage);
}

	
static int net_rap_user_usage(int argc, const char **argv)
{
	return net_help_user(argc, argv);
} 
	
static void user_fn(const char *user_name, void *state)
{
	d_printf("%-21.21s\n", user_name);
}

static void long_user_fn(const char *user_name, const char *comment,
			 const char * home_dir, const char * logon_script, 
			 void *state)
{
	d_printf("%-21.21s %s\n",
		 user_name, comment);
}

static void group_member_fn(const char *user_name, void *state)
{
	d_printf("%-21.21s\n", user_name);
}

static int rap_user_delete(int argc, const char **argv)
{
	struct cli_state *cli;
	int ret;
	
	if (argc == 0) {
		d_printf("\n\nUser name not specified\n");
                return net_rap_user_usage(argc, argv);
	}

	if (!(cli = net_make_ipc_connection(0))) 
                return -1;

	ret = cli_NetUserDelete(cli, argv[0]);
	cli_shutdown(cli);
	return ret;
}

static int rap_user_add(int argc, const char **argv)
{
	struct cli_state *cli;
	int ret;
	RAP_USER_INFO_1 userinfo;

	if (argc == 0) {
		d_printf("\n\nUser name not specified\n");
                return net_rap_user_usage(argc, argv);
	}

	if (!(cli = net_make_ipc_connection(0)))
                return -1;
			
	safe_strcpy(userinfo.user_name, argv[0], sizeof(userinfo.user_name)-1);
	if (opt_flags == -1) 
                opt_flags = 0x21; 
			
	userinfo.userflags = opt_flags;
	userinfo.reserved1 = '\0';
	userinfo.comment = smb_xstrdup(opt_comment);
	userinfo.priv = 1; 
	userinfo.home_dir = NULL;
	userinfo.logon_script = NULL;

	ret = cli_NetUserAdd(cli, &userinfo);

	cli_shutdown(cli);
	return ret;
}

static int rap_user_info(int argc, const char **argv)
{
	struct cli_state *cli;
	int ret;
	if (argc == 0) {
		d_printf("\n\nUser name not specified\n");
                return net_rap_user_usage(argc, argv);
	}

	if (!(cli = net_make_ipc_connection(0)))
                return -1;

	ret = cli_NetUserGetGroups(cli, argv[0], group_member_fn, NULL);
	cli_shutdown(cli);
	return ret;
}

int net_rap_user(int argc, const char **argv)
{
	int ret = -1;
	struct functable func[] = {
		{"ADD", rap_user_add},
		{"INFO", rap_user_info},
		{"DELETE", rap_user_delete},
		{NULL, NULL}
	};

	if (argc == 0) {
		struct cli_state *cli;
		if (!(cli = net_make_ipc_connection(0)))
                        goto done;
		if (opt_long_list_entries) {
			d_printf("\nUser name             Comment"\
				 "\n-----------------------------\n");
			ret = cli_RNetUserEnum(cli, long_user_fn, NULL);
			cli_shutdown(cli);
			goto done;
		}
		ret = cli_RNetUserEnum0(cli, user_fn, NULL); 
		cli_shutdown(cli);
		goto done;
	}

	ret = net_run_function(argc, argv, func, net_rap_user_usage);
 done:
	if (ret != 0) {
		DEBUG(1, ("Net user returned: %d\n", ret));
	}
	return ret;
}


int net_rap_group_usage(int argc, const char **argv)
{
	return net_help_group(argc, argv);
}

static void long_group_fn(const char *group_name, const char *comment,
			  void *state)
{
	d_printf("%-21.21s %s\n", group_name, comment);
}

static void group_fn(const char *group_name, void *state)
{
	d_printf("%-21.21s\n", group_name);
}

static int rap_group_delete(int argc, const char **argv)
{
	struct cli_state *cli;
	int ret;
	if (argc == 0) {
		d_printf("\n\nGroup name not specified\n");
                return net_rap_group_usage(argc, argv);
	}

	if (!(cli = net_make_ipc_connection(0)))
                return -1;

	ret = cli_NetGroupDelete(cli, argv[0]);
	cli_shutdown(cli);
	return ret;
}

static int rap_group_add(int argc, const char **argv)
{
	struct cli_state *cli;
	int ret;
	RAP_GROUP_INFO_1 grinfo;

	if (argc == 0) {
		d_printf("\n\nGroup name not specified\n");
                return net_rap_group_usage(argc, argv);
	}

	if (!(cli = net_make_ipc_connection(0)))
                return -1;
			
	/* BB check for length 21 or smaller explicitly ? BB */
	safe_strcpy(grinfo.group_name, argv[0], sizeof(grinfo.group_name)-1);
	grinfo.reserved1 = '\0';
	grinfo.comment = smb_xstrdup(opt_comment);
	
	ret = cli_NetGroupAdd(cli, &grinfo);
	cli_shutdown(cli);
	return ret;
}

int net_rap_group(int argc, const char **argv)
{
	struct functable func[] = {
		{"ADD", rap_group_add},
		{"DELETE", rap_group_delete},
		{NULL, NULL}
	};

	if (argc == 0) {
		struct cli_state *cli;
		int ret;
		if (!(cli = net_make_ipc_connection(0)))
                        return -1;
		if (opt_long_list_entries) {
			d_printf("Group name            Comment\n");
			d_printf("-----------------------------\n");
			ret = cli_RNetGroupEnum(cli, long_group_fn, NULL);
			cli_shutdown(cli);
			return ret;
		}
		ret = cli_RNetGroupEnum0(cli, group_fn, NULL); 
		cli_shutdown(cli);
		return ret;
	}

	return net_run_function(argc, argv, func, net_rap_group_usage);
}

int net_rap_groupmember_usage(int argc, const char **argv)
{
	d_printf(
	 "net rap groupmember LIST <group> [misc. options] [targets]"\
	 "\n\t Enumerate users in a group\n"\
	 "\nnet rap groupmember DELETE <group> <user> [misc. options] "\
	 "[targets]\n\t Delete sepcified user from specified group\n"\
	 "\nnet rap groupmember ADD <group> <user> [misc. options] [targets]"\
	 "\n\t Add specified user to specified group\n");

	net_common_flags_usage(argc, argv);
	return -1;
}


static int rap_groupmember_add(int argc, const char **argv)
{
	struct cli_state *cli;
	int ret;
	if (argc != 2) {
		d_printf("\n\nGroup or user name not specified\n");
                return net_rap_groupmember_usage(argc, argv);
	}

	if (!(cli = net_make_ipc_connection(0)))
                return -1;

	ret = cli_NetGroupAddUser(cli, argv[0], argv[1]);
	cli_shutdown(cli);
	return ret;
}

static int rap_groupmember_delete(int argc, const char **argv)
{
	struct cli_state *cli;
	int ret;
	if (argc != 2) {
		d_printf("\n\nGroup or user name not specified\n");
                return net_rap_groupmember_usage(argc, argv);
	}
	
	if (!(cli = net_make_ipc_connection(0)))
                return -1;

	ret = cli_NetGroupDelUser(cli, argv[0], argv[1]);
	cli_shutdown(cli);
	return ret;
}

static int rap_groupmember_list(int argc, const char **argv)
{
	struct cli_state *cli;
	int ret;
	if (argc == 0) {
		d_printf("\n\nGroup name not specified\n");
                return net_rap_groupmember_usage(argc, argv);
	}

	if (!(cli = net_make_ipc_connection(0)))
                return -1;

	ret = cli_NetGroupGetUsers(cli, argv[0], group_member_fn, NULL ); 
	cli_shutdown(cli);
	return ret;
}

int net_rap_groupmember(int argc, const char **argv)
{
	struct functable func[] = {
		{"ADD", rap_groupmember_add},
		{"LIST", rap_groupmember_list},
		{"DELETE", rap_groupmember_delete},
		{NULL, NULL}
	};
	
	return net_run_function(argc, argv, func, net_rap_groupmember_usage);
}

int net_rap_validate_usage(int argc, const char **argv)
{
	d_printf("net rap validate <username> [password]\n"\
		 "\tValidate user and password to check whether they"\
		 " can access target server or domain\n");

	net_common_flags_usage(argc, argv);
	return -1;
}

int net_rap_validate(int argc, const char **argv)
{
	return errmsg_not_implemented();
}

int net_rap_service_usage(int argc, const char **argv)
{
	d_printf("net rap service [misc. options] [targets] \n"\
		 "\tlists all running service daemons on target server\n");
	d_printf("\nnet rap service START <name> [service startup arguments]"\
		 " [misc. options] [targets]"\
		 "\n\tStart named service on remote server\n");
	d_printf("\nnet rap service STOP <name> [misc. options] [targets]\n"\
		 "\n\tStop named service on remote server\n");
    
	net_common_flags_usage(argc, argv);
	return -1;
}

static int rap_service_start(int argc, const char **argv)
{
	return errmsg_not_implemented();
}

static int rap_service_stop(int argc, const char **argv)
{
	return errmsg_not_implemented();
}

static void service_fn(const char *service_name, const char *dummy,
		       void *state)
{
	d_printf("%-21.21s\n", service_name);
}

int net_rap_service(int argc, const char **argv)
{
	struct functable func[] = {
		{"START", rap_service_start},
		{"STOP", rap_service_stop},
		{NULL, NULL}
	};

	if (argc == 0) {
		struct cli_state *cli;
		int ret;
		if (!(cli = net_make_ipc_connection(0))) 
			return -1;

		if (opt_long_list_entries) {
			d_printf("Service name          Comment\n");
			d_printf("-----------------------------\n");
			ret = cli_RNetServiceEnum(cli, long_group_fn, NULL);
		}
		ret = cli_RNetServiceEnum(cli, service_fn, NULL); 
		cli_shutdown(cli);
		return ret;
	}

	return net_run_function(argc, argv, func, net_rap_service_usage);
}

int net_rap_password_usage(int argc, const char **argv)
{
	d_printf(
	 "net rap password <user> <oldpwo> <newpw> [misc. options] [target]\n"\
	 "\tchanges the password for the specified user at target\n"); 
	
	return -1;
}


int net_rap_password(int argc, const char **argv)
{
	struct cli_state *cli;
	int ret;
	
	if (argc < 3) 
                return net_rap_password_usage(argc, argv);

	if (!(cli = net_make_ipc_connection(0))) 
                return -1;

	/* BB Add check for password lengths? */
	ret = cli_oem_change_password(cli, argv[0], argv[2], argv[1]);
	cli_shutdown(cli);
	return ret;
}

int net_rap_admin_usage(int argc, const char **argv)
{
	d_printf(
   "net rap admin <remote command> [cmd args [env]] [misc. options] [targets]"\
   "\n\texecutes a remote command on an os/2 target server\n"); 
	
	return -1;
}


int net_rap_admin(int argc, const char **argv)
{
	return errmsg_not_implemented();
}

/* The help subsystem for the RAP subcommand */

int net_rap_usage(int argc, const char **argv)
{
	d_printf("  net rap domain \tto list domains \n"\
		 "  net rap file \t\tto list open files on a server \n"\
		 "  net rap group \tto list user groups  \n"\
		 "  net rap groupmember \tto list users in a group \n"\
		 "  net rap password \tto change the password of a user\n"\
		 "  net rap printq \tto list the print queues on a server\n"\
		 "  net rap server \tto list servers in a domain\n"\
		 "  net rap session \tto list clients with open sessions to a server\n"\
		 "  net rap share \tto list shares exported by a server\n"\
		 "  net rap user \t\tto list users\n"\
		 "  net rap validate \tto check whether a user and the corresponding password are valid\n"\
		 "  net rap help\n"\
		 "\nType \"net help <option>\" to get more information on that option\n\n");

	net_common_flags_usage(argc, argv);
	return -1;
}

/*
  handle "net rap help *" subcommands
*/
int net_rap_help(int argc, const char **argv)
{
	struct functable func[] = {
		{"FILE", net_rap_file_usage},
		{"SHARE", net_rap_share_usage},
		{"SESSION", net_rap_session_usage},
		{"SERVER", net_rap_server_usage},
		{"DOMAIN", net_rap_domain_usage},
		{"PRINTQ", net_rap_printq_usage},
		{"USER", net_rap_user_usage},
		{"GROUP", net_rap_group_usage},
		{"VALIDATE", net_rap_validate_usage},
		{"GROUPMEMBER", net_rap_groupmember_usage},
		{"ADMIN", net_rap_admin_usage},
		{"SERVICE", net_rap_service_usage},
		{"PASSWORD", net_rap_password_usage},
		{NULL, NULL}};

	return net_run_function(argc, argv, func, net_rap_usage);
}

/* Entry-point for all the RAP functions. */

int net_rap(int argc, const char **argv)
{
	struct functable func[] = {
		{"FILE", net_rap_file},
		{"SHARE", net_rap_share},
		{"SESSION", net_rap_session},
		{"SERVER", net_rap_server},
		{"DOMAIN", net_rap_domain},
		{"PRINTQ", net_rap_printq},
		{"USER", net_rap_user},
		{"GROUP", net_rap_group},
		{"VALIDATE", net_rap_validate},
		{"GROUPMEMBER", net_rap_groupmember},
		{"ADMIN", net_rap_admin},
		{"SERVICE", net_rap_service},	
		{"PASSWORD", net_rap_password},
		{"HELP", net_rap_help},
		{NULL, NULL}
	};
	
	return net_run_function(argc, argv, func, net_rap_usage);
}

