/* 
   Unix SMB/CIFS implementation.
   NT Domain Authentication SMB / MSRPC client
   Copyright (C) Andrew Tridgell                 1994-1997,
   Copyright (C) Luke Kenneth Casson Leighton    1996-1997,
   Copyright (C) Simo Sorce                      2001,
   Copyright (C) Jim McDonough (jmcd@us.ibm.com) 2003.
   
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
#include "rpcclient.h"

#if 0	/* don't uncomment this unless you remove the getopt() calls */
	/* use net rpc shutdown instead */

/****************************************************************************
nt shutdown init
****************************************************************************/
static NTSTATUS cmd_shutdown_init(struct cli_state *cli, TALLOC_CTX *mem_ctx,
				  int argc, const char **argv)
{
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;
	fstring msg;
	uint32 timeout = 20;
	BOOL force = False;
	BOOL reboot = False;
	int opt;

	*msg = 0;
	optind = 0; /* TODO: test if this hack works on other systems too --simo */

	while ((opt = getopt(argc, argv, "m:t:rf")) != EOF)
	{
		/*fprintf (stderr, "[%s]\n", argv[argc-1]);*/
	
		switch (opt)
		{
			case 'm':
				fstrcpy(msg, optarg);
				/*fprintf (stderr, "[%s|%s]\n", optarg, msg);*/
				break;

			case 't':
				timeout = atoi(optarg);
				/*fprintf (stderr, "[%s|%d]\n", optarg, timeout);*/
				break;

			case 'r':
				reboot = True;
				break;

			case 'f':
				force = True;
				break;

		}
	}

	/* create an entry */
	result = cli_shutdown_init(cli, mem_ctx, msg, timeout, reboot, force);

	if (NT_STATUS_IS_OK(result))
		DEBUG(5,("cmd_shutdown_init: query succeeded\n"));
	else
		DEBUG(5,("cmd_shutdown_init: query failed\n"));

	return result;
}

/****************************************************************************
abort a shutdown
****************************************************************************/
static NTSTATUS cmd_shutdown_abort(struct cli_state *cli, 
				   TALLOC_CTX *mem_ctx, int argc, 
				   const char **argv)
{
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;

	result = cli_shutdown_abort(cli, mem_ctx);

	if (NT_STATUS_IS_OK(result))
		DEBUG(5,("cmd_shutdown_abort: query succeeded\n"));
	else
		DEBUG(5,("cmd_shutdown_abort: query failed\n"));

	return result;
}
#endif


/* List of commands exported by this module */
struct cmd_set shutdown_commands[] = {

	{ "SHUTDOWN"  },

#if 0
	{ "shutdowninit", RPC_RTYPE_NTSTATUS, cmd_shutdown_init, NULL, PI_SHUTDOWN, "Remote Shutdown (over shutdown pipe)",
				"syntax: shutdown [-m message] [-t timeout] [-r] [-h] [-f] (-r == reboot, -h == halt, -f == force)" },
				
	{ "shutdownabort", RPC_RTYPE_NTSTATUS, cmd_shutdown_abort, NULL, PI_SHUTDOWN, "Abort Shutdown (over shutdown pipe)",
				"syntax: shutdownabort" },
#endif
	{ NULL }
};
