/* 
   Samba Unix/Linux SMB client library 
   net help commands
   Copyright (C) 2002 Jim McDonough (jmcd@us.ibm.com)

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
#include "utils/net.h"

int net_common_methods_usage(int argc, const char**argv)
{
	d_printf("Valid methods: (auto-detected if not specified)\n");
	d_printf("\tads\t\t\t\tActive Directory (LDAP/Kerberos)\n");
	d_printf("\trpc\t\t\t\tDCE-RPC\n");
	d_printf("\trap\t\t\t\tRAP (older systems)\n");
	d_printf("\n");
	return 0;
}

int net_common_flags_usage(int argc, const char **argv)
{
	d_printf("Valid targets: choose one (none defaults to localhost)\n");
	d_printf("\t-S or --server=<server>\t\tserver name\n");
	d_printf("\t-I or --ipaddress=<ipaddr>\taddress of target server\n");
	d_printf("\t-w or --workgroup=<wg>\t\ttarget workgroup or domain\n");

	d_printf("\n");
	d_printf("Valid miscellaneous options are:\n"); /* misc options */
	d_printf("\t-p or --port=<port>\t\tconnection port on target\n");
	d_printf("\t-W or --myworkgroup=<wg>\tclient workgroup\n");
	d_printf("\t-d or --debuglevel=<level>\tdebug level (0-10)\n");
	d_printf("\t-n or --myname=<name>\t\tclient name\n");
	d_printf("\t-U or --user=<name>\t\tuser name\n");
	d_printf("\t-s or --configfile=<path>\tpathname of smb.conf file\n");
	d_printf("\t-l or --long\t\t\tDisplay full information\n");
	d_printf("\t-V or --version\t\t\tPrint samba version information\n");
	d_printf("\t-P or --machine-pass\t\tAuthenticate as machine account\n");
	return -1;
}

static int help_usage(int argc, const char **argv)
{
	d_printf(
"\n"\
"Usage: net help <function>\n"\
"\n"\
"Valid functions are:\n"\
"  RPC RAP ADS FILE SHARE SESSION SERVER DOMAIN PRINTQ USER GROUP VALIDATE\n"\
"  GROUPMEMBER ADMIN SERVICE PASSWORD TIME LOOKUP GETLOCALSID SETLOCALSID\n"\
"  SETDOMAINSID CHANGESCRETPW LOOKUP SAM\n");
	return -1;
}

int net_help_user(int argc, const char **argv)
{
	d_printf("\nnet [<method>] user [misc. options] [targets]"\
		 "\n\tList users\n\n");
	d_printf("net [<method>] user DELETE <name> [misc. options] [targets]"\
		 "\n\tDelete specified user\n");
	d_printf("\nnet [<method>] user INFO <name> [misc. options] [targets]"\
		 "\n\tList the domain groups of the specified user\n");
	d_printf("\nnet [<method>] user ADD <name> [password] [-c container] "\
		 "[-F user flags] [misc. options]"\
		 " [targets]\n\tAdd specified user\n");
	d_printf("\nnet [<method>] user RENAME <oldusername> <newusername>"\
		 " [targets]\n\tRename specified user\n\n");


	net_common_methods_usage(argc, argv);
	net_common_flags_usage(argc, argv);
	d_printf("\t-C or --comment=<comment>\tdescriptive comment (for add only)\n");
	d_printf("\t-c or --container=<container>\tLDAP container, defaults to cn=Users (for add in ADS only)\n");
	return -1;
}

int net_help_group(int argc, const char **argv)
{
	d_printf("net [<method>] group [misc. options] [targets]"\
		 "\n\tList user groups\n\n");
	d_printf("net rpc group LIST [global|local|builtin]* [misc. options]"\
		 "\n\tList specific user groups\n\n");
	d_printf("net [<method>] group DELETE <name> "\
		 "[misc. options] [targets]"\
		 "\n\tDelete specified group\n");
	d_printf("\nnet [<method>] group ADD <name> [-C comment] [-c container]"\
		 " [misc. options] [targets]\n\tCreate specified group\n");
	d_printf("\nnet rpc group MEMBERS <name>\n\tList Group Members\n\n");
	d_printf("\nnet rpc group ADDMEM <group> <member>\n\tAdd Group Members\n\n");
	d_printf("\nnet rpc group DELMEM <group> <member>\n\tDelete Group Members\n\n");
	net_common_methods_usage(argc, argv);
	net_common_flags_usage(argc, argv);
	d_printf("\t-C or --comment=<comment>\tdescriptive comment (for add only)\n");
	d_printf("\t-c or --container=<container>\tLDAP container, defaults to cn=Users (for add in ADS only)\n");
	d_printf("\t-L or --localgroup\t\tWhen adding groups, create a local group (alias)\n");
	return -1;
}

int net_help_join(int argc, const char **argv)
{
	d_printf("\nnet [<method>] join [misc. options]\n"
		 "\tjoins this server to a domain\n");
	d_printf("Valid methods: (auto-detected if not specified)\n");
	d_printf("\tads\t\t\t\tActive Directory (LDAP/Kerberos)\n");
	d_printf("\trpc\t\t\t\tDCE-RPC\n");
	net_common_flags_usage(argc, argv);
	return -1;
}

int net_help_share(int argc, const char **argv)
{
	d_printf(
	 "\nnet [<method>] share [misc. options] [targets] \n"
	 "\tenumerates all exported resources (network shares) "
	 "on target server\n\n"
	 "net [<method>] share ADD <name=serverpath> [misc. options] [targets]"
	"\n\tadds a share from a server (makes the export active)\n\n"
	"net [<method>] share DELETE <sharename> [misc. options] [targets]"
	"\n\tdeletes a share from a server (makes the export inactive)\n\n"
	"net [<method>] share ALLOWEDUSERS [<filename>] "
	"[misc. options] [targets]"
	"\n\tshows a list of all shares together with all users allowed to"
	"\n\taccess them. This needs the output of 'net usersidlist' on"
	"\n\tstdin or in <filename>.\n\n"
	 "net [<method>] share MIGRATE FILES <sharename> [misc. options] [targets]"
	 "\n\tMigrates files from remote to local server\n\n"
	 "net [<method>] share MIGRATE SHARES <sharename> [misc. options] [targets]"
	 "\n\tMigrates shares from remote to local server\n\n"
	 "net [<method>] share MIGRATE SECURITY <sharename> [misc. options] [targets]"
	 "\n\tMigrates share-ACLs from remote to local server\n\n" 
	 "net [<method>] share MIGRATE ALL <sharename> [misc. options] [targets]"
	 "\n\tMigrates shares (including directories, files) from remote\n"
	 "\tto local server\n\n"
	);
	net_common_methods_usage(argc, argv);
	net_common_flags_usage(argc, argv);
	d_printf(
	 "\t-C or --comment=<comment>\tdescriptive comment (for add only)\n"
	 "\t-M or --maxusers=<num>\t\tmax users allowed for share\n"
	 "\t      --acls\t\t\tcopies ACLs as well\n"
	 "\t      --attrs\t\t\tcopies DOS Attributes as well\n"
	 "\t      --timestamps\t\tpreserve timestamps while copying files\n"
	 "\t      --destination\t\tmigration target server (default: localhost)\n"
	 "\t-e or --exclude\t\t\tlist of shares to be excluded from mirroring\n"
	 "\t-v or --verbose\t\t\tgive verbose output\n");
	return -1;
}

int net_help_file(int argc, const char **argv)
{
	d_printf("net [<method>] file [misc. options] [targets]\n"\
		 "\tlists all open files on file server\n\n");
	d_printf("net [<method>] file USER <username> "\
		 "[misc. options] [targets]"\
		 "\n\tlists all files opened by username on file server\n\n");
	d_printf("net [<method>] file CLOSE <id> [misc. options] [targets]\n"\
		 "\tcloses specified file on target server\n\n");
	d_printf("net [rap] file INFO <id> [misc. options] [targets]\n"\
		 "\tdisplays information about the specified open file\n");

	net_common_methods_usage(argc, argv);
	net_common_flags_usage(argc, argv);
	return -1;
}

int net_help_printer(int argc, const char **argv)
{
	d_printf("net rpc printer LIST [printer] [misc. options] [targets]\n"\
		 "\tlists all printers on print-server\n\n");
	d_printf("net rpc printer DRIVER [printer] [misc. options] [targets]\n"\
		 "\tlists all printer-drivers on print-server\n\n");
	d_printf("net rpc printer PUBLISH action [printer] [misc. options] [targets]\n"\
		 "\tpublishes printer settings in Active Directory\n"
		 "\taction can be one of PUBLISH, UPDATE, UNPUBLISH or LIST\n\n");
	d_printf("net rpc printer MIGRATE PRINTERS [printer] [misc. options] [targets]"\
		 "\n\tmigrates printers from remote to local server\n\n");
	d_printf("net rpc printer MIGRATE SETTINGS [printer] [misc. options] [targets]"\
		 "\n\tmigrates printer-settings from remote to local server\n\n");
	d_printf("net rpc printer MIGRATE DRIVERS [printer] [misc. options] [targets]"\
		 "\n\tmigrates printer-drivers from remote to local server\n\n");
	d_printf("net rpc printer MIGRATE FORMS [printer] [misc. options] [targets]"\
		 "\n\tmigrates printer-forms from remote to local server\n\n");
	d_printf("net rpc printer MIGRATE SECURITY [printer] [misc. options] [targets]"\
		 "\n\tmigrates printer-ACLs from remote to local server\n\n");
	d_printf("net rpc printer MIGRATE ALL [printer] [misc. options] [targets]"\
		 "\n\tmigrates drivers, forms, queues, settings and acls from\n"\
		 "\tremote to local print-server\n\n");
	net_common_methods_usage(argc, argv);
	net_common_flags_usage(argc, argv);
	d_printf(
	 "\t-v or --verbose\t\t\tgive verbose output\n"
	 "\t      --destination\t\tmigration target server (default: localhost)\n");

	return -1;
}


int net_help_status(int argc, const char **argv)
{
	d_printf("  net status sessions [parseable] "
		 "Show list of open sessions\n");
	d_printf("  net status shares [parseable]   "
		 "Show list of open shares\n");
	return -1;
}

static int net_usage(int argc, const char **argv)
{
	d_printf("  net time\t\tto view or set time information\n"\
		 "  net lookup\t\tto lookup host name or ip address\n"\
		 "  net user\t\tto manage users\n"\
		 "  net group\t\tto manage groups\n"\
		 "  net sam\t\tto edit the local user database directly\n"\
		 "  net lookup\t\tto look up various things\n"\
		 "  net groupmap\t\tto manage group mappings\n"\
		 "  net join\t\tto join a domain\n"\
		 "  net cache\t\tto operate on cache tdb file\n"\
		 "  net getlocalsid [NAME]\tto get the SID for local name\n"\
		 "  net setlocalsid SID\tto set the local domain SID\n"\
		 "  net setdomainsid SID\tto set the domain SID on member servers\n"\
		 "  net changesecretpw\tto change the machine password in the local secrets database only\n"\
		 "                    \tthis requires the -f flag as a safety barrier\n"\
		 "  net status\t\tShow server status\n"\
		"  net usersidlist\tto get a list of all users with their SIDs\n"
		"  net usershare\t\tto add, delete and list locally user-modifiable shares\n"
		 "\n"\
		 "  net ads <command>\tto run ADS commands\n"\
		 "  net rap <command>\tto run RAP (pre-RPC) commands\n"\
		 "  net rpc <command>\tto run RPC commands\n"\
		 "\n"\
		 "Type \"net help <option>\" to get more information on that option\n");
	net_common_flags_usage(argc, argv);
	return -1;
}

/*
  handle "net help *" subcommands
*/
int net_help(int argc, const char **argv)
{
	struct functable func[] = {
		{"ADS", net_ads_help},	
		{"RAP", net_rap_help},
		{"RPC", net_rpc_help},

		{"FILE", net_help_file},
		{"SHARE", net_help_share},
		{"SESSION", net_rap_session_usage},
		{"SERVER", net_rap_server_usage},
		{"DOMAIN", net_rap_domain_usage},
		{"PRINTQ", net_rap_printq_usage},
		{"USER", net_help_user},
		{"GROUP", net_help_group},
		{"GROUPMAP", net_help_groupmap},
		{"JOIN", net_help_join},
		{"VALIDATE", net_rap_validate_usage},
		{"GROUPMEMBER", net_rap_groupmember_usage},
		{"ADMIN", net_rap_admin_usage},
		{"SERVICE", net_rap_service_usage},
		{"PASSWORD", net_rap_password_usage},
		{"TIME", net_time_usage},
		{"LOOKUP", net_lookup_usage},
		{"USERSHARE", net_usershare_usage},
		{"USERSIDLIST", net_usersidlist_usage},
#ifdef WITH_FAKE_KASERVER
		{"AFS", net_help_afs},
#endif

		{"HELP", help_usage},
		{NULL, NULL}};

	return net_run_function(argc, argv, func, net_usage);
}
