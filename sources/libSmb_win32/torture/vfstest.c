/* 
   Unix SMB/CIFS implementation.
   VFS module tester

   Copyright (C) Simo Sorce 2002
   Copyright (C) Eric Lorimer 2002
   Copyright (C) Jelmer Vernooij 2002,2003

   Most of this code was ripped off of rpcclient.
   Copyright (C) Tim Potter 2000-2001

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
#include "vfstest.h"

/* List to hold groups of commands */
static struct cmd_list {
	struct cmd_list *prev, *next;
	struct cmd_set *cmd_set;
} *cmd_list;

extern pstring user_socket_options;

/****************************************************************************
handle completion of commands for readline
****************************************************************************/
static char **completion_fn(const char *text, int start, int end)
{
#define MAX_COMPLETIONS 100
	char **matches;
	int i, count=0;
	struct cmd_list *commands = cmd_list;

	if (start) 
		return NULL;

	/* make sure we have a list of valid commands */
	if (!commands) 
		return NULL;

	matches = SMB_MALLOC_ARRAY(char *, MAX_COMPLETIONS);
	if (!matches) return NULL;

	matches[count++] = SMB_STRDUP(text);
	if (!matches[0]) return NULL;

	while (commands && count < MAX_COMPLETIONS-1) 
	{
		if (!commands->cmd_set)
			break;
		
		for (i=0; commands->cmd_set[i].name; i++)
		{
			if ((strncmp(text, commands->cmd_set[i].name, strlen(text)) == 0) &&
				commands->cmd_set[i].fn) 
			{
				matches[count] = SMB_STRDUP(commands->cmd_set[i].name);
				if (!matches[count]) 
					return NULL;
				count++;
			}
		}
		
		commands = commands->next;
		
	}

	if (count == 2) {
		SAFE_FREE(matches[0]);
		matches[0] = SMB_STRDUP(matches[1]);
	}
	matches[count] = NULL;
	return matches;
}

static char* next_command(char** cmdstr)
{
	static pstring 		command;
	char			*p;
	
	if (!cmdstr || !(*cmdstr))
		return NULL;
	
	p = strchr_m(*cmdstr, ';');
	if (p)
		*p = '\0';
	pstrcpy(command, *cmdstr);
	*cmdstr = p;
	
	return command;
}

/* Load specified configuration file */
static NTSTATUS cmd_conf(struct vfs_state *vfs, TALLOC_CTX *mem_ctx,
			int argc, const char **argv)
{
	if (argc != 2) {
		printf("Usage: %s <smb.conf>\n", argv[0]);
		return NT_STATUS_OK;
	}

	if (!lp_load(argv[1], False, True, False, True)) {
		printf("Error loading \"%s\"\n", argv[1]);
		return NT_STATUS_OK;
	}

	printf("\"%s\" successfully loaded\n", argv[1]);
	return NT_STATUS_OK;
}
	
/* Display help on commands */
static NTSTATUS cmd_help(struct vfs_state *vfs, TALLOC_CTX *mem_ctx,
			 int argc, const char **argv)
{
	struct cmd_list *tmp;
	struct cmd_set *tmp_set;

	/* Usage */
	if (argc > 2) {
		printf("Usage: %s [command]\n", argv[0]);
		return NT_STATUS_OK;
	}

	/* Help on one command */

	if (argc == 2) {
		for (tmp = cmd_list; tmp; tmp = tmp->next) {
			
			tmp_set = tmp->cmd_set;

			while(tmp_set->name) {
				if (strequal(argv[1], tmp_set->name)) {
					if (tmp_set->usage &&
					    tmp_set->usage[0])
						printf("%s\n", tmp_set->usage);
					else
						printf("No help for %s\n", tmp_set->name);

					return NT_STATUS_OK;
				}

				tmp_set++;
			}
		}

		printf("No such command: %s\n", argv[1]);
		return NT_STATUS_OK;
	}

	/* List all commands */

	for (tmp = cmd_list; tmp; tmp = tmp->next) {

		tmp_set = tmp->cmd_set;

		while(tmp_set->name) {

			printf("%15s\t\t%s\n", tmp_set->name,
			       tmp_set->description ? tmp_set->description:
			       "");

			tmp_set++;
		}
	}

	return NT_STATUS_OK;
}

/* Change the debug level */
static NTSTATUS cmd_debuglevel(struct vfs_state *vfs, TALLOC_CTX *mem_ctx, int argc, const char **argv)
{
	if (argc > 2) {
		printf("Usage: %s [debuglevel]\n", argv[0]);
		return NT_STATUS_OK;
	}

	if (argc == 2) {
		DEBUGLEVEL = atoi(argv[1]);
	}

	printf("debuglevel is %d\n", DEBUGLEVEL);

	return NT_STATUS_OK;
}

static NTSTATUS cmd_freemem(struct vfs_state *vfs, TALLOC_CTX *mem_ctx, int argc, const char **argv)
{
	/* Cleanup */
	talloc_destroy(mem_ctx);
	mem_ctx = NULL;
	vfs->data = NULL;
	vfs->data_size = 0;
	return NT_STATUS_OK;
}

static NTSTATUS cmd_quit(struct vfs_state *vfs, TALLOC_CTX *mem_ctx, int argc, const char **argv)
{
	/* Cleanup */
	talloc_destroy(mem_ctx);

	exit(0);
	return NT_STATUS_OK; /* NOTREACHED */
}

static struct cmd_set vfstest_commands[] = {

	{ "GENERAL OPTIONS" },

	{ "conf", 	cmd_conf, 	"Load smb configuration file", "conf <smb.conf>" },
	{ "help", 	cmd_help, 	"Get help on commands", "" },
	{ "?", 		cmd_help, 	"Get help on commands", "" },
	{ "debuglevel", cmd_debuglevel, "Set debug level", "" },
	{ "freemem",	cmd_freemem,	"Free currently allocated buffers", "" },
	{ "exit", 	cmd_quit, 	"Exit program", "" },
	{ "quit", 	cmd_quit, 	"Exit program", "" },

	{ NULL }
};

static struct cmd_set separator_command[] = {
	{ "---------------", NULL,	"----------------------" },
	{ NULL }
};


extern struct cmd_set vfs_commands[];
static struct cmd_set *vfstest_command_list[] = {
	vfstest_commands,
	vfs_commands,
	NULL
};

static void add_command_set(struct cmd_set *cmd_set)
{
	struct cmd_list *entry;

	if (!(entry = SMB_MALLOC_P(struct cmd_list))) {
		DEBUG(0, ("out of memory\n"));
		return;
	}

	ZERO_STRUCTP(entry);

	entry->cmd_set = cmd_set;
	DLIST_ADD(cmd_list, entry);
}

static NTSTATUS do_cmd(struct vfs_state *vfs, struct cmd_set *cmd_entry, char *cmd)
{
	const char *p = cmd;
	char **argv = NULL;
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;
	pstring buf;
	TALLOC_CTX *mem_ctx = NULL;
	int argc = 0, i;

	/* Count number of arguments first time through the loop then
	   allocate memory and strdup them. */

 again:
	while(next_token(&p, buf, " ", sizeof(buf))) {
		if (argv) {
			argv[argc] = SMB_STRDUP(buf);
		}
		
		argc++;
	}
				
	if (!argv) {

		/* Create argument list */

		argv = SMB_MALLOC_ARRAY(char *, argc);
		memset(argv, 0, sizeof(char *) * argc);

		if (!argv) {
			fprintf(stderr, "out of memory\n");
			result = NT_STATUS_NO_MEMORY;
			goto done;
		}
					
		p = cmd;
		argc = 0;
					
		goto again;
	}

	/* Call the function */

	if (cmd_entry->fn) {

		if (mem_ctx == NULL) {
			/* Create mem_ctx */
			if (!(mem_ctx = talloc_init("do_cmd"))) {
		       		DEBUG(0, ("talloc_init() failed\n"));
				goto done;
			}
		}

		/* Run command */
		result = cmd_entry->fn(vfs, mem_ctx, argc, (const char **)argv);

	} else {
		fprintf (stderr, "Invalid command\n");
		goto done;
	}

 done:
						
	/* Cleanup */

	if (argv) {
		for (i = 0; i < argc; i++)
			SAFE_FREE(argv[i]);
	
		SAFE_FREE(argv);
	}
	
	return result;
}

/* Process a command entered at the prompt or as part of -c */
static NTSTATUS process_cmd(struct vfs_state *vfs, char *cmd)
{
	struct cmd_list *temp_list;
	BOOL found = False;
	pstring buf;
	const char *p = cmd;
	NTSTATUS result = NT_STATUS_OK;
	int len = 0;

	if (cmd[strlen(cmd) - 1] == '\n')
		cmd[strlen(cmd) - 1] = '\0';

	if (!next_token(&p, buf, " ", sizeof(buf))) {
		return NT_STATUS_OK;
	}

	/* strip the trainly \n if it exsists */
	len = strlen(buf);
	if (buf[len-1] == '\n')
		buf[len-1] = '\0';

	/* Search for matching commands */

	for (temp_list = cmd_list; temp_list; temp_list = temp_list->next) {
		struct cmd_set *temp_set = temp_list->cmd_set;

		while(temp_set->name) {
			if (strequal(buf, temp_set->name)) {
				found = True;
				result = do_cmd(vfs, temp_set, cmd);

				goto done;
			}
			temp_set++;
		}
	}

 done:
	if (!found && buf[0]) {
		printf("command not found: %s\n", buf);
		return NT_STATUS_OK;
	}

	if (!NT_STATUS_IS_OK(result)) {
		printf("result was %s\n", nt_errstr(result));
	}

	return result;
}

static void process_file(struct vfs_state *pvfs, char *filename) {
	FILE *file;
	char command[3 * PATH_MAX];

	if (*filename == '-') {
		file = stdin;
	} else {
		file = fopen(filename, "r");
		if (file == NULL) {
			printf("vfstest: error reading file (%s)!", filename);
			printf("errno n.%d: %s", errno, strerror(errno));
			exit(-1);
		}
	}

	while (fgets(command, 3 * PATH_MAX, file) != NULL) {
		process_cmd(pvfs, command);
	}
}

void exit_server(const char *reason)
{
	DEBUG(3,("Server exit (%s)\n", (reason ? reason : "")));
	exit(0);
}

void exit_server_cleanly(const char *const reason)
{
	exit_server("normal exit");
}

static int server_fd = -1;
int last_message = -1;

int smbd_server_fd(void)
{
		return server_fd;
}

void reload_printers(void)
{
	return;
}

/****************************************************************************
 Reload the services file.
**************************************************************************/

BOOL reload_services(BOOL test)
{
	BOOL ret;
	
	if (lp_loaded()) {
		pstring fname;
		pstrcpy(fname,lp_configfile());
		if (file_exist(fname, NULL) &&
		    !strcsequal(fname, dyn_CONFIGFILE)) {
			pstrcpy(dyn_CONFIGFILE, fname);
			test = False;
		}
	}

	reopen_logs();

	if (test && !lp_file_list_changed())
		return(True);

	lp_killunused(conn_snum_used);
	
	ret = lp_load(dyn_CONFIGFILE, False, False, True, True);

	/* perhaps the config filename is now set */
	if (!test)
		reload_services(True);

	reopen_logs();

	load_interfaces();

	{
		if (smbd_server_fd() != -1) {      
			set_socket_options(smbd_server_fd(),"SO_KEEPALIVE");
			set_socket_options(smbd_server_fd(), user_socket_options);
		}
	}

	mangle_reset_cache();
	reset_stat_cache();

	/* this forces service parameters to be flushed */
	set_current_service(NULL,0,True);

	return (ret);
}

/* Main function */

int main(int argc, char *argv[])
{
	static char		*cmdstr = NULL;
	struct cmd_set 		**cmd_set;
	static struct vfs_state vfs;
	int i;
	static char		*filename = NULL;

	/* make sure the vars that get altered (4th field) are in
	   a fixed location or certain compilers complain */
	poptContext pc;
	struct poptOption long_options[] = {
		POPT_AUTOHELP
		{"file",	'f', POPT_ARG_STRING,	&filename, 0, },
		{"command",	'c', POPT_ARG_STRING,	&cmdstr, 0, "Execute specified list of commands" },
		POPT_COMMON_SAMBA
		POPT_TABLEEND
	};

	load_case_tables();

	setlinebuf(stdout);

	pc = poptGetContext("vfstest", argc, (const char **) argv,
			    long_options, 0);
	
	while(poptGetNextOpt(pc) != -1);


	poptFreeContext(pc);

	/* TODO: check output */
	reload_services(False);

	/* the following functions are part of the Samba debugging
	   facilities.  See lib/debug.c */
	setup_logging("vfstest", True);
	
	/* Load command lists */

	cmd_set = vfstest_command_list;

	while(*cmd_set) {
		add_command_set(*cmd_set);
		add_command_set(separator_command);
		cmd_set++;
	}

	/* some basic initialization stuff */
	sec_init();
	conn_init();
	vfs.conn = conn_new();
	string_set(&vfs.conn->user,"vfstest");
	for (i=0; i < 1024; i++)
		vfs.files[i] = NULL;

	/* some advanced initiliazation stuff */
	smbd_vfs_init(vfs.conn);

	/* Do we have a file input? */
	if (filename && filename[0]) {
		process_file(&vfs, filename);
		return 0;
	}

	/* Do anything specified with -c */
	if (cmdstr && cmdstr[0]) {
		char    *cmd;
		char    *p = cmdstr;
 
		while((cmd=next_command(&p)) != NULL) {
			process_cmd(&vfs, cmd);
		}
		
		return 0;
	}

	/* Loop around accepting commands */

	while(1) {
		pstring prompt;
		char *line;

		slprintf(prompt, sizeof(prompt) - 1, "vfstest $> ");

		line = smb_readline(prompt, NULL, completion_fn);

		if (line == NULL)
			break;

		if (line[0] != '\n')
			process_cmd(&vfs, line);
	}
	
	free(vfs.conn);
	return 0;
}
