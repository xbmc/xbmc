/* 
   Unix SMB/CIFS implementation.
   SMB client
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

#include "includes.h"

#ifndef REGISTER
#define REGISTER 0
#endif

extern pstring global_myname;

extern pstring user_socket_options;


extern file_info def_finfo;

#define CNV_LANG(s) dos2unix_format(s,False)
#define CNV_INPUT(s) unix2dos_format(s,True)

static struct cli_state smbcli;
struct cli_state *smb_cli = &smbcli;

FILE *out_hnd;

static pstring password; /* local copy only, if one is entered */

/****************************************************************************
initialise smb client structure
****************************************************************************/
void rpcclient_init(void)
{
	memset((char *)smb_cli, '\0', sizeof(smb_cli));
	cli_initialise(smb_cli);
	smb_cli->capabilities |= CAP_NT_SMBS;
}

/****************************************************************************
make smb client connection
****************************************************************************/
static BOOL rpcclient_connect(struct client_info *info)
{
	struct nmb_name calling;
	struct nmb_name called;

	make_nmb_name(&called , dns_to_netbios_name(info->dest_host ), info->name_type);
	make_nmb_name(&calling, dns_to_netbios_name(info->myhostname), 0x0);

	if (!cli_establish_connection(smb_cli, 
	                          info->dest_host, &info->dest_ip, 
	                          &calling, &called,
	                          info->share, info->svc_type,
	                          False, True))
	{
		DEBUG(0,("rpcclient_connect: connection failed\n"));
		cli_shutdown(smb_cli);
		return False;
	}

	return True;
}

/****************************************************************************
stop the smb connection(s?)
****************************************************************************/
static void rpcclient_stop(void)
{
	cli_shutdown(smb_cli);
}

/****************************************************************************
  log in as an nt user, log out again. 
****************************************************************************/
void run_enums_test(int num_ops, struct client_info *cli_info, struct cli_state *cli)
{
	pstring cmd;
	int i;

	/* establish connections.  nothing to stop these being re-established. */
	rpcclient_connect(cli_info);

	DEBUG(5,("rpcclient_connect: cli->fd:%d\n", cli->fd));
	if (cli->fd <= 0)
	{
		fprintf(out_hnd, "warning: connection could not be established to %s<%02x>\n",
		                 cli_info->dest_host, cli_info->name_type);
		return;
	}
	
	for (i = 0; i < num_ops; i++)
	{
		set_first_token("");
		cmd_srv_enum_sess(cli_info);
		set_first_token("");
		cmd_srv_enum_shares(cli_info);
		set_first_token("");
		cmd_srv_enum_files(cli_info);

		if (password[0] != 0)
		{
			slprintf(cmd, sizeof(cmd)-1, "1");
			set_first_token(cmd);
		}
		else
		{
			set_first_token("");
		}
		cmd_srv_enum_conn(cli_info);
	}

	rpcclient_stop();

}

/****************************************************************************
  log in as an nt user, log out again. 
****************************************************************************/
void run_ntlogin_test(int num_ops, struct client_info *cli_info, struct cli_state *cli)
{
	pstring cmd;
	int i;

	/* establish connections.  nothing to stop these being re-established. */
	rpcclient_connect(cli_info);

	DEBUG(5,("rpcclient_connect: cli->fd:%d\n", cli->fd));
	if (cli->fd <= 0)
	{
		fprintf(out_hnd, "warning: connection could not be established to %s<%02x>\n",
		                 cli_info->dest_host, cli_info->name_type);
		return;
	}
	
	for (i = 0; i < num_ops; i++)
	{
		slprintf(cmd, sizeof(cmd)-1, "%s %s", cli->user_name, password);
		set_first_token(cmd);

		cmd_netlogon_login_test(cli_info);
	}

	rpcclient_stop();

}

/****************************************************************************
  runs n simultaneous functions.
****************************************************************************/
static void create_procs(int nprocs, int numops, 
		struct client_info *cli_info, struct cli_state *cli,
		void (*fn)(int, struct client_info *, struct cli_state *))
{
	int i, status;

	for (i=0;i<nprocs;i++)
	{
		if (fork() == 0)
		{
			pid_t mypid = getpid();
			sys_srandom(mypid ^ time(NULL));
			fn(numops, cli_info, cli);
			fflush(out_hnd);
			_exit(0);
		}
	}

	for (i=0;i<nprocs;i++)
	{
		waitpid(0, &status, 0);
	}
}
/****************************************************************************
usage on the program - OUT OF DATE!
****************************************************************************/
static void usage(char *pname)
{
  fprintf(out_hnd, "Usage: %s service <password> [-d debuglevel] [-l log] ",
	   pname);

  fprintf(out_hnd, "\nVersion %s\n",SAMBA_VERSION_STRING);
  fprintf(out_hnd, "\t-d debuglevel         set the debuglevel\n");
  fprintf(out_hnd, "\t-l log basename.      Basename for log/debug files\n");
  fprintf(out_hnd, "\t-n netbios name.      Use this name as my netbios name\n");
  fprintf(out_hnd, "\t-m max protocol       set the max protocol level\n");
  fprintf(out_hnd, "\t-I dest IP            use this IP to connect to\n");
  fprintf(out_hnd, "\t-E                    write messages to stderr instead of stdout\n");
  fprintf(out_hnd, "\t-U username           set the network username\n");
  fprintf(out_hnd, "\t-W workgroup          set the workgroup name\n");
  fprintf(out_hnd, "\t-t terminal code      terminal i/o code {sjis|euc|jis7|jis8|junet|hex}\n");
  fprintf(out_hnd, "\n");
}

enum client_action
{
	CLIENT_NONE,
	CLIENT_IPC,
	CLIENT_SVC
};

/****************************************************************************
  main program
****************************************************************************/
 int main(int argc,char *argv[])
{
	char *pname = argv[0];
	int opt;
	extern char *optarg;
	extern int optind;
	pstring term_code;
	BOOL got_pass = False;
	char *cmd_str="";
	enum client_action cli_action = CLIENT_NONE;
	int nprocs = 1;
	int numops = 100;
	pstring logfile;

	struct client_info cli_info;

	out_hnd = stdout;

	rpcclient_init();

#ifdef KANJI
	pstrcpy(term_code, KANJI);
#else /* KANJI */
	*term_code = 0;
#endif /* KANJI */

	if (!lp_load(dyn_CONFIGFILE,True, False, False, True))
	{
		fprintf(stderr, "Can't load %s - run testparm to debug it\n", dyn_CONFIGFILE);
	}

	DEBUGLEVEL = 0;

	cli_info.put_total_size = 0;
	cli_info.put_total_time_ms = 0;
	cli_info.get_total_size = 0;
	cli_info.get_total_time_ms = 0;

	cli_info.dir_total = 0;
	cli_info.newer_than = 0;
	cli_info.archive_level = 0;
	cli_info.print_mode = 1;

	cli_info.translation = False;
	cli_info.recurse_dir = False;
	cli_info.lowercase = False;
	cli_info.prompt = True;
	cli_info.abort_mget = True;

	cli_info.dest_ip.s_addr = 0;
	cli_info.name_type = 0x20;

	pstrcpy(cli_info.cur_dir , "\\");
	pstrcpy(cli_info.file_sel, "");
	pstrcpy(cli_info.base_dir, "");
	pstrcpy(smb_cli->domain, "");
	pstrcpy(smb_cli->user_name, "");
	pstrcpy(cli_info.myhostname, "");
	pstrcpy(cli_info.dest_host, "");

	pstrcpy(cli_info.svc_type, "A:");
	pstrcpy(cli_info.share, "");
	pstrcpy(cli_info.service, "");

	ZERO_STRUCT(cli_info.dom.level3_sid);
	pstrcpy(cli_info.dom.level3_dom, "");
	ZERO_STRUCT(cli_info.dom.level5_sid);
	pstrcpy(cli_info.dom.level5_dom, "");

	{
		int i;
		for (i=0; i<PI_MAX_PIPES; i++)
			smb_cli->pipes[i].fnum   = 0xffff;
	}

	setup_logging(pname, True);

	if (!get_myname(global_myname))
	{
		fprintf(stderr, "Failed to get my hostname.\n");
	}

	password[0] = 0;

	if (argc < 2)
	{
		usage(pname);
		exit(1);
	}

	if (*argv[1] != '-')
	{
		pstrcpy(cli_info.service, argv[1]);  
		/* Convert any '/' characters in the service name to '\' characters */
		string_replace( cli_info.service, '/','\\');
		argc--;
		argv++;

		DEBUG(1,("service: %s\n", cli_info.service));

		if (count_chars(cli_info.service,'\\') < 3)
		{
			usage(pname);
			printf("\n%s: Not enough '\\' characters in service\n", cli_info.service);
			exit(1);
		}

		/*
		if (count_chars(cli_info.service,'\\') > 3)
		{
			usage(pname);
			printf("\n%s: Too many '\\' characters in service\n", cli_info.service);
			exit(1);
		}
		*/

		if (argc > 1 && (*argv[1] != '-'))
		{
			got_pass = True;
			pstrcpy(password,argv[1]);  
			memset(argv[1],'X',strlen(argv[1]));
			argc--;
			argv++;
		}

		cli_action = CLIENT_SVC;
	}

	while ((opt = getopt(argc, argv,"s:O:M:S:i:N:o:n:d:l:hI:EB:U:L:t:m:W:T:D:c:")) != EOF)
	{
		switch (opt)
		{
			case 'm':
			{
				/* FIXME ... max_protocol seems to be funny here */

				int max_protocol = 0;
				max_protocol = interpret_protocol(optarg,max_protocol);
				fprintf(stderr, "max protocol not currently supported\n");
				break;
			}

			case 'O':
			{
				pstrcpy(user_socket_options,optarg);
				break;	
			}

			case 'S':
			{
				pstrcpy(cli_info.dest_host,optarg);
				strupper_m(cli_info.dest_host);
				cli_action = CLIENT_IPC;
				break;
			}

			case 'i':
			{
				pstrcpy(scope, optarg);
				break;
			}

			case 'U':
			{
				char *lp;
				pstrcpy(smb_cli->user_name,optarg);
				if ((lp=strchr_m(smb_cli->user_name,'%')))
				{
					*lp = 0;
					pstrcpy(password,lp+1);
					got_pass = True;
					memset(strchr_m(optarg,'%')+1,'X',strlen(password));
				}
				break;
			}

			case 'W':
			{
				pstrcpy(smb_cli->domain,optarg);
				break;
			}

			case 'E':
			{
				dbf = x_stderr;
				break;
			}

			case 'I':
			{
				cli_info.dest_ip = *interpret_addr2(optarg);
				if (is_zero_ip(cli_info.dest_ip))
				{
					exit(1);
				}
				break;
			}

			case 'N':
			{
				nprocs = atoi(optarg);
				break;
			}

			case 'o':
			{
				numops = atoi(optarg);
				break;
			}

			case 'n':
			{
				fstrcpy(global_myname, optarg);
				break;
			}

			case 'd':
			{
				if (*optarg == 'A')
					DEBUGLEVEL = 10000;
				else
					DEBUGLEVEL = atoi(optarg);
				break;
			}

			case 'l':
			{
				slprintf(logfile, sizeof(logfile)-1,
				         "%s.client",optarg);
				lp_set_logfile(logfile);
				break;
			}

			case 'c':
			{
				cmd_str = optarg;
				got_pass = True;
				break;
			}

			case 'h':
			{
				usage(pname);
				exit(0);
				break;
			}

			case 's':
			{
				pstrcpy(dyn_CONFIGFILE, optarg);
				break;
			}

			case 't':
			{
				pstrcpy(term_code, optarg);
				break;
			}

			default:
			{
				usage(pname);
				exit(1);
				break;
			}
		}
	}

	if (cli_action == CLIENT_NONE)
	{
		usage(pname);
		exit(1);
	}

	strupper_m(global_myname);
	fstrcpy(cli_info.myhostname, global_myname);

	DEBUG(3,("%s client started (version %s)\n",timestring(False),SAMBA_VERSION_STRING));

	if (*smb_cli->domain == 0)
	{
		pstrcpy(smb_cli->domain,lp_workgroup());
	}
	strupper_m(smb_cli->domain);

	load_interfaces();

	if (cli_action == CLIENT_IPC)
	{
		pstrcpy(cli_info.share, "IPC$");
		pstrcpy(cli_info.svc_type, "IPC");
	}

	fstrcpy(cli_info.mach_acct, cli_info.myhostname);
	strupper_m(cli_info.mach_acct);
	fstrcat(cli_info.mach_acct, "$");

	/* set the password cache info */
	if (got_pass)
	{
		if (password[0] == 0)
		{
			pwd_set_nullpwd(&(smb_cli->pwd));
		}
		else
		{
			pwd_make_lm_nt_16(&(smb_cli->pwd), password); /* generate 16 byte hashes */
		}
	}
	else 
	{
		char *pwd = getpass("Enter Password:");
		safe_strcpy(password, pwd, sizeof(password));
		pwd_make_lm_nt_16(&(smb_cli->pwd), password); /* generate 16 byte hashes */
	}

	create_procs(nprocs, numops, &cli_info, smb_cli, run_enums_test);

	if (password[0] != 0)
	{
		create_procs(nprocs, numops, &cli_info, smb_cli, run_ntlogin_test);
	}

	fflush(out_hnd);

	return(0);
}
