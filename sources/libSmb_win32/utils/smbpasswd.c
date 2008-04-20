/*
 * Unix SMB/CIFS implementation. 
 * Copyright (C) Jeremy Allison 1995-1998
 * Copyright (C) Tim Potter     2001
 * 
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 * 
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 675
 * Mass Ave, Cambridge, MA 02139, USA.  */

#include "includes.h"

extern BOOL AllowDebugChange;

/*
 * Next two lines needed for SunOS and don't
 * hurt anything else...
 */
extern char *optarg;
extern int optind;

/* forced running in root-mode */
static BOOL got_username = False;
static BOOL stdin_passwd_get = False;
static fstring user_name;
static char *new_passwd = NULL;
static const char *remote_machine = NULL;

static fstring ldap_secret;


/*********************************************************
 Print command usage on stderr and die.
**********************************************************/
static void usage(void)
{
	printf("When run by root:\n");
	printf("    smbpasswd [options] [username]\n");
	printf("otherwise:\n");
	printf("    smbpasswd [options]\n\n");

	printf("options:\n");
	printf("  -L                   local mode (must be first option)\n");
	printf("  -h                   print this usage message\n");
	printf("  -s                   use stdin for password prompt\n");
	printf("  -c smb.conf file     Use the given path to the smb.conf file\n");
	printf("  -D LEVEL             debug level\n");
	printf("  -r MACHINE           remote machine\n");
	printf("  -U USER              remote username\n");

	printf("extra options when run by root or in local mode:\n");
	printf("  -a                   add user\n");
	printf("  -d                   disable user\n");
	printf("  -e                   enable user\n");
	printf("  -i                   interdomain trust account\n");
	printf("  -m                   machine trust account\n");
	printf("  -n                   set no password\n");
	printf("  -W                   use stdin ldap admin password\n");
	printf("  -w PASSWORD          ldap admin password\n");
	printf("  -x                   delete user\n");
	printf("  -R ORDER             name resolve order\n");

	exit(1);
}

static void set_line_buffering(FILE *f)
{
	setvbuf(f, NULL, _IOLBF, 0);
}

/*******************************************************************
 Process command line options
 ******************************************************************/

static int process_options(int argc, char **argv, int local_flags)
{
	int ch;
	pstring configfile;
	pstrcpy(configfile, dyn_CONFIGFILE);

	local_flags |= LOCAL_SET_PASSWORD;

	ZERO_STRUCT(user_name);

	user_name[0] = '\0';

	while ((ch = getopt(argc, argv, "c:axdehminjr:sw:R:D:U:LW")) != EOF) {
		switch(ch) {
		case 'L':
			local_flags |= LOCAL_AM_ROOT;
			break;
		case 'c':
			pstrcpy(configfile,optarg);
			break;
		case 'a':
			local_flags |= LOCAL_ADD_USER;
			break;
		case 'x':
			local_flags |= LOCAL_DELETE_USER;
			local_flags &= ~LOCAL_SET_PASSWORD;
			break;
		case 'd':
			local_flags |= LOCAL_DISABLE_USER;
			local_flags &= ~LOCAL_SET_PASSWORD;
			break;
		case 'e':
			local_flags |= LOCAL_ENABLE_USER;
			local_flags &= ~LOCAL_SET_PASSWORD;
			break;
		case 'm':
			local_flags |= LOCAL_TRUST_ACCOUNT;
			break;
		case 'i':
			local_flags |= LOCAL_INTERDOM_ACCOUNT;
			break;
		case 'j':
			d_printf("See 'net join' for this functionality\n");
			exit(1);
			break;
		case 'n':
			local_flags |= LOCAL_SET_NO_PASSWORD;
			local_flags &= ~LOCAL_SET_PASSWORD;
			new_passwd = smb_xstrdup("NO PASSWORD");
			break;
		case 'r':
			remote_machine = optarg;
			break;
		case 's':
			set_line_buffering(stdin);
			set_line_buffering(stdout);
			set_line_buffering(stderr);
			stdin_passwd_get = True;
			break;
		case 'w':
			local_flags |= LOCAL_SET_LDAP_ADMIN_PW;
			fstrcpy(ldap_secret, optarg);
			break;
		case 'R':
			lp_set_name_resolve_order(optarg);
			break;
		case 'D':
			DEBUGLEVEL = atoi(optarg);
			break;
		case 'U': {
			got_username = True;
			fstrcpy(user_name, optarg);
			break;
		case 'W':
			local_flags |= LOCAL_SET_LDAP_ADMIN_PW;
			*ldap_secret = '\0';
			break;
		}
		case 'h':
		default:
			usage();
		}
	}
	
	argc -= optind;
	argv += optind;

	switch(argc) {
	case 0:
		if (!got_username)
			fstrcpy(user_name, "");
		break;
	case 1:
		if (!(local_flags & LOCAL_AM_ROOT)) {
			usage();
		} else {
			if (got_username) {
				usage();
			} else {
				fstrcpy(user_name, argv[0]);
			}
		}
		break;
	default:
		usage();
	}

	if (!lp_load(configfile,True,False,False,True)) {
		fprintf(stderr, "Can't load %s - run testparm to debug it\n", 
			dyn_CONFIGFILE);
		exit(1);
	}

	return local_flags;
}

/*************************************************************
 Utility function to prompt for new password.
*************************************************************/
static char *prompt_for_new_password(BOOL stdin_get)
{
	char *p;
	fstring new_pw;

	ZERO_ARRAY(new_pw);
 
	p = get_pass("New SMB password:", stdin_get);

	fstrcpy(new_pw, p);
	SAFE_FREE(p);

	p = get_pass("Retype new SMB password:", stdin_get);

	if (strcmp(p, new_pw)) {
		fprintf(stderr, "Mismatch - password unchanged.\n");
		ZERO_ARRAY(new_pw);
		SAFE_FREE(p);
		return NULL;
	}

	return p;
}


/*************************************************************
 Change a password either locally or remotely.
*************************************************************/

static NTSTATUS password_change(const char *remote_mach, char *username, 
				char *old_passwd, char *new_pw,
				int local_flags)
{
	NTSTATUS ret;
	pstring err_str;
	pstring msg_str;

	if (remote_mach != NULL) {
		if (local_flags & (LOCAL_ADD_USER|LOCAL_DELETE_USER|LOCAL_DISABLE_USER|LOCAL_ENABLE_USER|
							LOCAL_TRUST_ACCOUNT|LOCAL_SET_NO_PASSWORD)) {
			/* these things can't be done remotely yet */
			return NT_STATUS_UNSUCCESSFUL;
		}
		ret = remote_password_change(remote_mach, username, 
					     old_passwd, new_pw, err_str, sizeof(err_str));
		if(*err_str)
			fprintf(stderr, "%s", err_str);
		return ret;
	}
	
	ret = local_password_change(username, local_flags, new_pw, 
				     err_str, sizeof(err_str), msg_str, sizeof(msg_str));

	if(*msg_str)
		printf("%s", msg_str);
	if(*err_str)
		fprintf(stderr, "%s", err_str);

	return ret;
}

/*******************************************************************
 Store the LDAP admin password in secrets.tdb
 ******************************************************************/
static BOOL store_ldap_admin_pw (char* pw)
{	
	if (!pw) 
		return False;

	if (!secrets_init())
		return False;
	
	return secrets_store_ldap_pw(lp_ldap_admin_dn(), pw);
}


/*************************************************************
 Handle password changing for root.
*************************************************************/

static int process_root(int local_flags)
{
	struct passwd  *pwd;
	int result = 0;
	char *old_passwd = NULL;

	if (local_flags & LOCAL_SET_LDAP_ADMIN_PW) {
		char *ldap_admin_dn = lp_ldap_admin_dn();
		if ( ! *ldap_admin_dn ) {
			DEBUG(0,("ERROR: 'ldap admin dn' not defined! Please check your smb.conf\n"));
			goto done;
		}

		printf("Setting stored password for \"%s\" in secrets.tdb\n", ldap_admin_dn);
		if ( ! *ldap_secret ) {
			new_passwd = prompt_for_new_password(stdin_passwd_get);
			fstrcpy(ldap_secret, new_passwd);
		}
		if (!store_ldap_admin_pw(ldap_secret)) {
			DEBUG(0,("ERROR: Failed to store the ldap admin password!\n"));
		}
		goto done;
	}

	/* Ensure passdb startup(). */
	if(!initialize_password_db(False)) {
		DEBUG(0, ("Failed to open passdb!\n"));
		exit(1);
	}
		
	/* Ensure we have a SAM sid. */
	get_global_sam_sid();

	/*
	 * Ensure both add/delete user are not set
	 * Ensure add/delete user and either remote machine or join domain are
	 * not both set.
	 */	
	if(((local_flags & (LOCAL_ADD_USER|LOCAL_DELETE_USER)) == (LOCAL_ADD_USER|LOCAL_DELETE_USER)) || 
	   ((local_flags & (LOCAL_ADD_USER|LOCAL_DELETE_USER)) && 
		(remote_machine != NULL))) {
		usage();
	}
	
	/* Only load interfaces if we are doing network operations. */

	if (remote_machine) {
		load_interfaces();
	}

	if (!user_name[0] && (pwd = getpwuid_alloc(NULL, geteuid()))) {
		fstrcpy(user_name, pwd->pw_name);
		TALLOC_FREE(pwd);
	} 

	if (!user_name[0]) {
		fprintf(stderr,"You must specify a username\n");
		exit(1);
	}

	if (local_flags & LOCAL_TRUST_ACCOUNT) {
		/* add the $ automatically */
		static fstring buf;

		/*
		 * Remove any trailing '$' before we
		 * generate the initial machine password.
		 */

		if (user_name[strlen(user_name)-1] == '$') {
			user_name[strlen(user_name)-1] = 0;
		}

		if (local_flags & LOCAL_ADD_USER) {
		        SAFE_FREE(new_passwd);
			new_passwd = smb_xstrdup(user_name);
			strlower_m(new_passwd);
		}

		/*
		 * Now ensure the username ends in '$' for
		 * the machine add.
		 */

		slprintf(buf, sizeof(buf)-1, "%s$", user_name);
		fstrcpy(user_name, buf);
	} else if (local_flags & LOCAL_INTERDOM_ACCOUNT) {
		static fstring buf;

		if ((local_flags & LOCAL_ADD_USER) && (new_passwd == NULL)) {
			/*
			 * Prompt for trusting domain's account password
			 */
			new_passwd = prompt_for_new_password(stdin_passwd_get);
			if(!new_passwd) {
				fprintf(stderr, "Unable to get newpassword.\n");
				exit(1);
			}
		}
		
		/* prepare uppercased and '$' terminated username */
		slprintf(buf, sizeof(buf) - 1, "%s$", user_name);
		fstrcpy(user_name, buf);
		
	} else {
		
		if (remote_machine != NULL) {
			old_passwd = get_pass("Old SMB password:",stdin_passwd_get);
		}
		
		if (!(local_flags & LOCAL_SET_PASSWORD)) {
			
			/*
			 * If we are trying to enable a user, first we need to find out
			 * if they are using a modern version of the smbpasswd file that
			 * disables a user by just writing a flag into the file. If so
			 * then we can re-enable a user without prompting for a new
			 * password. If not (ie. they have a no stored password in the
			 * smbpasswd file) then we need to prompt for a new password.
			 */
			
			if(local_flags & LOCAL_ENABLE_USER) {
				struct samu *sampass = NULL;
				
				sampass = samu_new( NULL );
				if (!sampass) {
					fprintf(stderr, "talloc fail for struct samu.\n");
					exit(1);
				}
				if (!pdb_getsampwnam(sampass, user_name)) {
					fprintf(stderr, "Failed to find user %s in passdb backend.\n",
						user_name );
					exit(1);
				}

				if(pdb_get_nt_passwd(sampass) == NULL) {
					local_flags |= LOCAL_SET_PASSWORD;
				}
				TALLOC_FREE(sampass);
			}
		}
		
		if((local_flags & LOCAL_SET_PASSWORD) && (new_passwd == NULL)) {
			new_passwd = prompt_for_new_password(stdin_passwd_get);
			
			if(!new_passwd) {
				fprintf(stderr, "Unable to get new password.\n");
				exit(1);
			}
		}
	}

	if (!NT_STATUS_IS_OK(password_change(remote_machine, user_name,
					     old_passwd, new_passwd,
					     local_flags))) {
		fprintf(stderr,"Failed to modify password entry for user %s\n", user_name);
		result = 1;
		goto done;
	} 

	if(remote_machine) {
		printf("Password changed for user %s on %s.\n", user_name, remote_machine );
	} else if(!(local_flags & (LOCAL_ADD_USER|LOCAL_DISABLE_USER|LOCAL_ENABLE_USER|LOCAL_DELETE_USER|LOCAL_SET_NO_PASSWORD|LOCAL_SET_PASSWORD))) {
		struct samu *sampass = NULL;
		
		sampass = samu_new( NULL );
		if (!sampass) {
			fprintf(stderr, "talloc fail for struct samu.\n");
			exit(1);
		}

		if (!pdb_getsampwnam(sampass, user_name)) {
			fprintf(stderr, "Failed to find user %s in passdb backend.\n",
				user_name );
			exit(1);
		}

		printf("Password changed for user %s.", user_name );
		if(pdb_get_acct_ctrl(sampass)&ACB_DISABLED) {
			printf(" User has disabled flag set.");
		}
		if(pdb_get_acct_ctrl(sampass) & ACB_PWNOTREQ) {
			printf(" User has no password flag set.");
		}
		printf("\n");
		TALLOC_FREE(sampass);
	}

 done:
	SAFE_FREE(new_passwd);
	return result;
}


/*************************************************************
 Handle password changing for non-root.
*************************************************************/

static int process_nonroot(int local_flags)
{
	struct passwd  *pwd = NULL;
	int result = 0;
	char *old_pw = NULL;
	char *new_pw = NULL;

	if (local_flags & ~(LOCAL_AM_ROOT | LOCAL_SET_PASSWORD)) {
		/* Extra flags that we can't honor non-root */
		usage();
	}

	if (!user_name[0]) {
		pwd = getpwuid_alloc(NULL, getuid());
		if (pwd) {
			fstrcpy(user_name,pwd->pw_name);
			TALLOC_FREE(pwd);
		} else {
			fprintf(stderr, "smbpasswd: cannot lookup user name for uid %u\n", (unsigned int)getuid());
			exit(1);
		}
	}
	
	/*
	 * A non-root user is always setting a password
	 * via a remote machine (even if that machine is
	 * localhost).
	 */	

	load_interfaces(); /* Delayed from main() */

	if (remote_machine == NULL) {
		remote_machine = "127.0.0.1";
	}

	if (remote_machine != NULL) {
		old_pw = get_pass("Old SMB password:",stdin_passwd_get);
	}
	
	if (!new_passwd) {
		new_pw = prompt_for_new_password(stdin_passwd_get);
	}
	else
		new_pw = smb_xstrdup(new_passwd);
	
	if (!new_pw) {
		fprintf(stderr, "Unable to get new password.\n");
		exit(1);
	}

	if (!NT_STATUS_IS_OK(password_change(remote_machine, user_name, old_pw,
					     new_pw, 0))) {
		fprintf(stderr,"Failed to change password for %s\n", user_name);
		result = 1;
		goto done;
	}

	printf("Password changed for user %s\n", user_name);

 done:
	SAFE_FREE(old_pw);
	SAFE_FREE(new_pw);

	return result;
}



/*********************************************************
 Start here.
**********************************************************/
int main(int argc, char **argv)
{	
	int local_flags = 0;
	
	AllowDebugChange = False;

#if defined(HAVE_SET_AUTH_PARAMETERS)
	set_auth_parameters(argc, argv);
#endif /* HAVE_SET_AUTH_PARAMETERS */

	if (getuid() == 0) {
		local_flags = LOCAL_AM_ROOT;
	}

	load_case_tables();

	local_flags = process_options(argc, argv, local_flags);

	setup_logging("smbpasswd", True);
	
	/*
	 * Set the machine NETBIOS name if not already
	 * set from the config file. 
	 */ 
    
	if (!init_names())
		return 1;

	/* Check the effective uid - make sure we are not setuid */
	if (is_setuid_root()) {
		fprintf(stderr, "smbpasswd must *NOT* be setuid root.\n");
		exit(1);
	}

	if (local_flags & LOCAL_AM_ROOT) {
		secrets_init();
		return process_root(local_flags);
	} 

	return process_nonroot(local_flags);
}
