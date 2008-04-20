/* 
 *  Unix SMB/CIFS implementation.
 *  Group Policy Object Support
 *  Copyright (C) Guenther Deschner 2005
 *  
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *  
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "includes.h"

#define GPT_INI_SECTION_GENERAL "General"
#define GPT_INI_PARAMETER_VERSION "Version"
#define GPT_INI_PARAMETER_DISPLAYNAME "displayName"

struct gpt_ini {
	uint32 version;
	const char *display_name;
};

static uint32 version;

static BOOL do_section(const char *section)
{
	DEBUG(10,("do_section: %s\n", section));

	return True;
}

static BOOL do_parameter(const char *parameter, const char *value)
{
	DEBUG(10,("do_parameter: %s, %s\n", parameter, value));
	
	if (strequal(parameter, GPT_INI_PARAMETER_VERSION)) {
		version = atoi(value);
	}
	return True;
}

NTSTATUS ads_gpo_get_sysvol_gpt_version(ADS_STRUCT *ads, 
					TALLOC_CTX *mem_ctx, 
					const char *filesyspath, 
					uint32 *sysvol_version)
{
	NTSTATUS status;
	const char *path;
	struct cli_state *cli;
	int fnum;
	fstring tok;
	static int io_bufsize = 64512;
	int read_size = io_bufsize;
	char *data = NULL;
	off_t start = 0;
	off_t nread = 0;
	int handle = 0;
	const char *local_file;

	*sysvol_version = 0;

	next_token(&filesyspath, tok, "\\", sizeof(tok));
	next_token(&filesyspath, tok, "\\", sizeof(tok));

	path = talloc_asprintf(mem_ctx, "\\%s\\gpt.ini", filesyspath);
	if (path == NULL) {
		return NT_STATUS_NO_MEMORY;
	}

	local_file = talloc_asprintf(mem_ctx, "%s/%s", lock_path("gpo_cache"), "gpt.ini");
	if (local_file == NULL) {
		return NT_STATUS_NO_MEMORY;
	}

	/* FIXME: walk down the dfs tree instead */
	status = cli_full_connection(&cli, global_myname(), 
				     ads->config.ldap_server_name,
				     NULL, 0,
				     "SYSVOL", "A:",
				     ads->auth.user_name, NULL, ads->auth.password,
				     CLI_FULL_CONNECTION_USE_KERBEROS,
				     Undefined, NULL);
	if (!NT_STATUS_IS_OK(status)) {
		return status;
	}

	fnum = cli_open(cli, path, O_RDONLY, DENY_NONE);
	if (fnum == -1) {
		return NT_STATUS_NO_SUCH_FILE;
	}


	data = (char *)SMB_MALLOC(read_size);
	if (data == NULL) {
		return NT_STATUS_NO_MEMORY;
	}

	handle = sys_open(local_file, O_WRONLY|O_CREAT|O_TRUNC, 0644);

	if (handle == -1) {
		return NT_STATUS_NO_SUCH_FILE;
	}
	 
	while (1) {

		int n = cli_read(cli, fnum, data, nread + start, read_size);

		if (n <= 0)
			break;

		if (write(handle, data, n) != n) {
			break;
		}

		nread += n;
	}

	cli_close(cli, fnum);

	if (!pm_process(local_file, do_section, do_parameter)) {
		return NT_STATUS_INVALID_PARAMETER;
	}

	*sysvol_version = version;

	SAFE_FREE(data);

	cli_shutdown(cli);

	return NT_STATUS_OK;
}

/*

perfectly parseable with pm_process() :))

[Unicode]
Unicode=yes
[System Access]
MinimumPasswordAge = 1
MaximumPasswordAge = 42
MinimumPasswordLength = 7
PasswordComplexity = 1
PasswordHistorySize = 24
LockoutBadCount = 0
RequireLogonToChangePassword = 0
ForceLogoffWhenHourExpire = 0
ClearTextPassword = 0
[Kerberos Policy]
MaxTicketAge = 10
MaxRenewAge = 7
MaxServiceAge = 600
MaxClockSkew = 5
TicketValidateClient = 1
[Version]
signature="$CHICAGO$"
Revision=1
*/
