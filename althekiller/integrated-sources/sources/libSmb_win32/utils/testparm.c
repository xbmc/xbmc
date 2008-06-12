/* 
   Unix SMB/CIFS implementation.
   Test validity of smb.conf
   Copyright (C) Karl Auer 1993, 1994-1998

   Extensively modified by Andrew Tridgell, 1995
   Converted to popt by Jelmer Vernooij (jelmer@nl.linux.org), 2002
   
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

/*
 * Testbed for loadparm.c/params.c
 *
 * This module simply loads a specified configuration file and
 * if successful, dumps it's contents to stdout. Note that the
 * operation is performed with DEBUGLEVEL at 3.
 *
 * Useful for a quick 'syntax check' of a configuration file.
 *
 */

#include "includes.h"

extern BOOL AllowDebugChange;

/***********************************************
 Here we do a set of 'hard coded' checks for bad
 configuration settings.
************************************************/

static int do_global_checks(void)
{
	int ret = 0;
	SMB_STRUCT_STAT st;

	if (lp_security() >= SEC_DOMAIN && !lp_encrypted_passwords()) {
		fprintf(stderr, "ERROR: in 'security=domain' mode the 'encrypt passwords' parameter must always be set to 'true'.\n");
		ret = 1;
	}

	if (lp_wins_support() && lp_wins_server_list()) {
		fprintf(stderr, "ERROR: both 'wins support = true' and 'wins server = <server list>' \
cannot be set in the smb.conf file. nmbd will abort with this setting.\n");
		ret = 1;
	}

	if (!directory_exist(lp_lockdir(), &st)) {
		fprintf(stderr, "ERROR: lock directory %s does not exist\n",
		       lp_lockdir());
		ret = 1;
	} else if ((st.st_mode & 0777) != 0755) {
		fprintf(stderr, "WARNING: lock directory %s should have permissions 0755 for browsing to work\n",
		       lp_lockdir());
		ret = 1;
	}

	if (!directory_exist(lp_piddir(), &st)) {
		fprintf(stderr, "ERROR: pid directory %s does not exist\n",
		       lp_piddir());
		ret = 1;
	}

	if (lp_passdb_expand_explicit()) {
		fprintf(stderr, "WARNING: passdb expand explicit = yes is "
			"deprecated\n");
	}

	/*
	 * Password server sanity checks.
	 */

	if((lp_security() == SEC_SERVER || lp_security() >= SEC_DOMAIN) && !lp_passwordserver()) {
		pstring sec_setting;
		if(lp_security() == SEC_SERVER)
			pstrcpy(sec_setting, "server");
		else if(lp_security() == SEC_DOMAIN)
			pstrcpy(sec_setting, "domain");

		fprintf(stderr, "ERROR: The setting 'security=%s' requires the 'password server' parameter be set \
to a valid password server.\n", sec_setting );
		ret = 1;
	}

	
	/*
	 * Password chat sanity checks.
	 */

	if(lp_security() == SEC_USER && lp_unix_password_sync()) {

		/*
		 * Check that we have a valid lp_passwd_program() if not using pam.
		 */

#ifdef WITH_PAM
		if (!lp_pam_password_change()) {
#endif

			if(lp_passwd_program() == NULL) {
				fprintf( stderr, "ERROR: the 'unix password sync' parameter is set and there is no valid 'passwd program' \
parameter.\n" );
				ret = 1;
			} else {
				pstring passwd_prog;
				pstring truncated_prog;
				const char *p;

				pstrcpy( passwd_prog, lp_passwd_program());
				p = passwd_prog;
				*truncated_prog = '\0';
				next_token(&p, truncated_prog, NULL, sizeof(pstring));

				if(access(truncated_prog, F_OK) == -1) {
					fprintf(stderr, "ERROR: the 'unix password sync' parameter is set and the 'passwd program' (%s) \
cannot be executed (error was %s).\n", truncated_prog, strerror(errno) );
					ret = 1;
				}

             }

#ifdef WITH_PAM
		}
#endif

		if(lp_passwd_chat() == NULL) {
			fprintf(stderr, "ERROR: the 'unix password sync' parameter is set and there is no valid 'passwd chat' \
parameter.\n");
			ret = 1;
		} else 
		/* check if there's a %u parameter present */
		if(strstr_m(lp_passwd_program(), "%u") == NULL) {
			fprintf(stderr, "ERROR: the 'passwd program' (%s) requires a '%%u' parameter.\n", lp_passwd_program());
			ret = 1;
		}

		/*
		 * Check that we have a valid script and that it hasn't
		 * been written to expect the old password.
		 */

		if(lp_encrypted_passwords()) {
			if(strstr_m( lp_passwd_chat(), "%o")!=NULL) {
				fprintf(stderr, "ERROR: the 'passwd chat' script [%s] expects to use the old plaintext password \
via the %%o substitution. With encrypted passwords this is not possible.\n", lp_passwd_chat() );
				ret = 1;
			}
		}
	}

	if (strlen(lp_winbind_separator()) != 1) {
		fprintf(stderr,"ERROR: the 'winbind separator' parameter must be a single character.\n");
		ret = 1;
	}

	if (*lp_winbind_separator() == '+') {
		fprintf(stderr,"'winbind separator = +' might cause problems with group membership.\n");
	}

	if (lp_algorithmic_rid_base() < BASE_RID) {
		/* Try to prevent admin foot-shooting, we can't put algorithmic
		   rids below 1000, that's the 'well known RIDs' on NT */
		fprintf(stderr,"'algorithmic rid base' must be equal to or above %lu\n", BASE_RID);
	}

	if (lp_algorithmic_rid_base() & 1) {
		fprintf(stderr,"'algorithmic rid base' must be even.\n");
	}

#ifndef HAVE_DLOPEN
	if (lp_preload_modules()) {
		fprintf(stderr,"WARNING: 'preload modules = ' set while loading plugins not supported.\n");
	}
#endif

	if (!lp_passdb_backend()) {
		fprintf(stderr,"ERROR: passdb backend must have a value or be left out\n");
	}

	return ret;
}   

 int main(int argc, const char *argv[])
{
	const char *config_file = dyn_CONFIGFILE;
	int s;
	static BOOL silent_mode = False;
	static BOOL show_all_parameters = False;
	int ret = 0;
	poptContext pc;
	static const char *term_code = "";
	static char *parameter_name = NULL;
	static const char *section_name = NULL;
	static char *new_local_machine = NULL;
	const char *cname;
	const char *caddr;
	static int show_defaults;

	struct poptOption long_options[] = {
		POPT_AUTOHELP
		{"suppress-prompt", 's', POPT_ARG_VAL, &silent_mode, 1, "Suppress prompt for enter"},
		{"verbose", 'v', POPT_ARG_NONE, &show_defaults, 1, "Show default options too"},
		{"server", 'L',POPT_ARG_STRING, &new_local_machine, 0, "Set %%L macro to servername\n"},
		{"encoding", 't', POPT_ARG_STRING, &term_code, 0, "Print parameters with encoding"},
		{"show-all-parameters", '\0', POPT_ARG_VAL, &show_all_parameters, True, "Show the parameters, type, possible values" },
		{"parameter-name", '\0', POPT_ARG_STRING, &parameter_name, 0, "Limit testparm to a named parameter" },
		{"section-name", '\0', POPT_ARG_STRING, &section_name, 0, "Limit testparm to a named section" },
		POPT_COMMON_VERSION
		POPT_TABLEEND
	};

	load_case_tables();

	pc = poptGetContext(NULL, argc, argv, long_options, 
			    POPT_CONTEXT_KEEP_FIRST);
	poptSetOtherOptionHelp(pc, "[OPTION...] <config-file> [host-name] [host-ip]");

	while(poptGetNextOpt(pc) != -1);

	if (show_all_parameters) {
		show_parameter_list();
		exit(0);
	}

	setup_logging(poptGetArg(pc), True);

	if (poptPeekArg(pc)) 
		config_file = poptGetArg(pc);

	cname = poptGetArg(pc);
	caddr = poptGetArg(pc);

	if ( cname && ! caddr ) {
		printf ( "ERROR: You must specify both a machine name and an IP address.\n" );
		return(1);
	}

	if (new_local_machine) {
		set_local_machine_name(new_local_machine, True);
	}

	dbf = x_stderr;
	DEBUGLEVEL = 2;
	AllowDebugChange = False;

	fprintf(stderr,"Load smb config files from %s\n",config_file);

	if (!lp_load(config_file,False,True,False,True)) {
		fprintf(stderr,"Error loading services.\n");
		return(1);
	}

	fprintf(stderr,"Loaded services file OK.\n");

	ret = do_global_checks();

	for (s=0;s<1000;s++) {
		if (VALID_SNUM(s))
			if (strlen(lp_servicename(s)) > 12) {
				fprintf(stderr, "WARNING: You have some share names that are longer than 12 characters.\n" );
				fprintf(stderr, "These may not be accessible to some older clients.\n" );
				fprintf(stderr, "(Eg. Windows9x, WindowsMe, and smbclient prior to Samba 3.0.)\n" );
				break;
			}
	}

	for (s=0;s<1000;s++) {
		if (VALID_SNUM(s)) {
			const char **deny_list = lp_hostsdeny(s);
			const char **allow_list = lp_hostsallow(s);
			int i;
			if(deny_list) {
				for (i=0; deny_list[i]; i++) {
					char *hasstar = strchr_m(deny_list[i], '*');
					char *hasquery = strchr_m(deny_list[i], '?');
					if(hasstar || hasquery) {
						fprintf(stderr,"Invalid character %c in hosts deny list (%s) for service %s.\n",
							   hasstar ? *hasstar : *hasquery, deny_list[i], lp_servicename(s) );
					}
				}
			}

			if(allow_list) {
				for (i=0; allow_list[i]; i++) {
					char *hasstar = strchr_m(allow_list[i], '*');
					char *hasquery = strchr_m(allow_list[i], '?');
					if(hasstar || hasquery) {
						fprintf(stderr,"Invalid character %c in hosts allow list (%s) for service %s.\n",
							   hasstar ? *hasstar : *hasquery, allow_list[i], lp_servicename(s) );
					}
				}
			}

			if(lp_level2_oplocks(s) && !lp_oplocks(s)) {
				fprintf(stderr,"Invalid combination of parameters for service %s. \
					   Level II oplocks can only be set if oplocks are also set.\n",
					   lp_servicename(s) );
			}

			if (lp_map_hidden(s) && !(lp_create_mask(s) & S_IXOTH)) {
				fprintf(stderr,"Invalid combination of parameters for service %s. \
					   Map hidden can only work if create mask includes octal 01 (S_IXOTH).\n",
					   lp_servicename(s) );
			}
			if (lp_map_hidden(s) && (lp_force_create_mode(s) & S_IXOTH)) {
				fprintf(stderr,"Invalid combination of parameters for service %s. \
					   Map hidden can only work if force create mode excludes octal 01 (S_IXOTH).\n",
					   lp_servicename(s) );
			}
			if (lp_map_system(s) && !(lp_create_mask(s) & S_IXGRP)) {
				fprintf(stderr,"Invalid combination of parameters for service %s. \
					   Map system can only work if create mask includes octal 010 (S_IXGRP).\n",
					   lp_servicename(s) );
			}
			if (lp_map_system(s) && (lp_force_create_mode(s) & S_IXGRP)) {
				fprintf(stderr,"Invalid combination of parameters for service %s. \
					   Map system can only work if force create mode excludes octal 010 (S_IXGRP).\n",
					   lp_servicename(s) );
			}
#ifdef HAVE_CUPS
			if (lp_printing(s) == PRINT_CUPS && *(lp_printcommand(s)) != '\0') {
				 fprintf(stderr,"Warning: Service %s defines a print command, but \
print command parameter is ignored when using CUPS libraries.\n",
					   lp_servicename(s) );
			}
#endif
		}
	}


	if (!section_name && !parameter_name) {
		fprintf(stderr,"Server role: %s\n", server_role_str(lp_server_role()));
	}

	if (!cname) {
		if (!silent_mode) {
			fprintf(stderr,"Press enter to see a dump of your service definitions\n");
			fflush(stdout);
			getc(stdin);
		}
		if (parameter_name || section_name) {
			BOOL isGlobal = False;
			s = GLOBAL_SECTION_SNUM;

			if (!section_name) {
				section_name = GLOBAL_NAME;
				isGlobal = True;
			} else if ((isGlobal=!strwicmp(section_name, GLOBAL_NAME)) == 0 &&
				 (s=lp_servicenumber(section_name)) == -1) {
					fprintf(stderr,"Unknown section %s\n",
						section_name);
					return(1);
			}
			if (parameter_name) {
				if (!dump_a_parameter( s, parameter_name, stdout, isGlobal)) {
					fprintf(stderr,"Parameter %s unknown for section %s\n",
						parameter_name, section_name);
					return(1);
				}
			} else {
				if (isGlobal == True)
					lp_dump(stdout, show_defaults, 0);
				else
					lp_dump_one(stdout, show_defaults, s);
			}
			return(ret);
		}

		lp_dump(stdout, show_defaults, lp_numservices());
	}

	if(cname && caddr){
		/* this is totally ugly, a real `quick' hack */
		for (s=0;s<1000;s++) {
			if (VALID_SNUM(s)) {
				if (allow_access(lp_hostsdeny(-1), lp_hostsallow(-1), cname, caddr)
				    && allow_access(lp_hostsdeny(s), lp_hostsallow(s), cname, caddr)) {
					fprintf(stderr,"Allow connection from %s (%s) to %s\n",
						   cname,caddr,lp_servicename(s));
				} else {
					fprintf(stderr,"Deny connection from %s (%s) to %s\n",
						   cname,caddr,lp_servicename(s));
				}
			}
		}
	}
	return(ret);
}

