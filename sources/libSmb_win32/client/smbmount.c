/* 
   Unix SMB/CIFS implementation.
   SMBFS mount program
   Copyright (C) Andrew Tridgell 1999
   
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

#include <mntent.h>
#include <asm/types.h>
#include <linux/smb_fs.h>

extern BOOL in_client;
extern pstring user_socket_options;

static pstring credentials;
static pstring my_netbios_name;
static pstring password;
static pstring username;
static pstring workgroup;
static pstring mpoint;
static pstring service;
static pstring options;

static struct in_addr dest_ip;
static BOOL have_ip;
static int smb_port = 0;
static BOOL got_user;
static BOOL got_pass;
static uid_t mount_uid;
static gid_t mount_gid;
static int mount_ro;
static unsigned mount_fmask;
static unsigned mount_dmask;
static BOOL use_kerberos;
/* TODO: Add code to detect smbfs version in kernel */
static BOOL status32_smbfs = False;
static BOOL smbfs_has_unicode = False;
static BOOL smbfs_has_lfs = False;

static void usage(void);

static void exit_parent(int sig)
{
	/* parent simply exits when child says go... */
	exit(0);
}

static void daemonize(void)
{
	int j, status;
	pid_t child_pid;

	signal( SIGTERM, exit_parent );

	if ((child_pid = sys_fork()) < 0) {
		DEBUG(0,("could not fork\n"));
	}

	if (child_pid > 0) {
		while( 1 ) {
			j = waitpid( child_pid, &status, 0 );
			if( j < 0 ) {
				if( EINTR == errno ) {
					continue;
				}
				status = errno;
			}
			break;
		}

		/* If we get here - the child exited with some error status */
		if (WIFSIGNALED(status))
			exit(128 + WTERMSIG(status));
		else
			exit(WEXITSTATUS(status));
	}

	signal( SIGTERM, SIG_DFL );
	chdir("/");
}

static void close_our_files(int client_fd)
{
	int i;
	struct rlimit limits;

	getrlimit(RLIMIT_NOFILE,&limits);
	for (i = 0; i< limits.rlim_max; i++) {
		if (i == client_fd)
			continue;
		close(i);
	}
}

static void usr1_handler(int x)
{
	return;
}


/***************************************************** 
return a connection to a server
*******************************************************/
static struct cli_state *do_connection(char *the_service)
{
	struct cli_state *c;
	struct nmb_name called, calling;
	char *server_n;
	struct in_addr ip;
	pstring server;
	char *share;

	if (the_service[0] != '\\' || the_service[1] != '\\') {
		usage();
		exit(1);
	}

	pstrcpy(server, the_service+2);
	share = strchr_m(server,'\\');
	if (!share) {
		usage();
		exit(1);
	}
	*share = 0;
	share++;

	server_n = server;

	make_nmb_name(&calling, my_netbios_name, 0x0);
	make_nmb_name(&called , server, 0x20);

 again:
        zero_ip(&ip);
	if (have_ip) ip = dest_ip;

	/* have to open a new connection */
	if (!(c=cli_initialise(NULL)) || (cli_set_port(c, smb_port) != smb_port) ||
	    !cli_connect(c, server_n, &ip)) {
		DEBUG(0,("%d: Connection to %s failed\n", sys_getpid(), server_n));
		if (c) {
			cli_shutdown(c);
		}
		return NULL;
	}

	/* SPNEGO doesn't work till we get NTSTATUS error support */
	/* But it is REQUIRED for kerberos authentication */
	if(!use_kerberos) c->use_spnego = False;

	/* The kernel doesn't yet know how to sign it's packets */
	c->sign_info.allow_smb_signing = False;

	/* Use kerberos authentication if specified */
	c->use_kerberos = use_kerberos;

	if (!cli_session_request(c, &calling, &called)) {
		char *p;
		DEBUG(0,("%d: session request to %s failed (%s)\n", 
			 sys_getpid(), called.name, cli_errstr(c)));
		cli_shutdown(c);
		if ((p=strchr_m(called.name, '.'))) {
			*p = 0;
			goto again;
		}
		if (strcmp(called.name, "*SMBSERVER")) {
			make_nmb_name(&called , "*SMBSERVER", 0x20);
			goto again;
		}
		return NULL;
	}

	DEBUG(4,("%d: session request ok\n", sys_getpid()));

	if (!cli_negprot(c)) {
		DEBUG(0,("%d: protocol negotiation failed\n", sys_getpid()));
		cli_shutdown(c);
		return NULL;
	}

	if (!got_pass) {
		char *pass = getpass("Password: ");
		if (pass) {
			pstrcpy(password, pass);
		}
	}

	/* This should be right for current smbfs. Future versions will support
	  large files as well as unicode and oplocks. */
  	c->capabilities &= ~(CAP_NT_SMBS | CAP_NT_FIND | CAP_LEVEL_II_OPLOCKS);
  	if (!smbfs_has_lfs)
  		c->capabilities &= ~CAP_LARGE_FILES;
  	if (!smbfs_has_unicode)
  		c->capabilities &= ~CAP_UNICODE;
	if (!status32_smbfs) {
  		c->capabilities &= ~CAP_STATUS32;
		c->force_dos_errors = True;
	}

	if (!cli_session_setup(c, username, 
			       password, strlen(password),
			       password, strlen(password),
			       workgroup)) {
		/* if a password was not supplied then try again with a
			null username */
		if (password[0] || !username[0] ||
				!cli_session_setup(c, "", "", 0, "", 0, workgroup)) {
			DEBUG(0,("%d: session setup failed: %s\n",
				sys_getpid(), cli_errstr(c)));
			cli_shutdown(c);
			return NULL;
		}
		DEBUG(0,("Anonymous login successful\n"));
	}

	DEBUG(4,("%d: session setup ok\n", sys_getpid()));

	if (!cli_send_tconX(c, share, "?????",
			    password, strlen(password)+1)) {
		DEBUG(0,("%d: tree connect failed: %s\n",
			 sys_getpid(), cli_errstr(c)));
		cli_shutdown(c);
		return NULL;
	}

	DEBUG(4,("%d: tconx ok\n", sys_getpid()));

	got_pass = True;

	return c;
}


/****************************************************************************
unmount smbfs  (this is a bailout routine to clean up if a reconnect fails)
	Code blatently stolen from smbumount.c
		-mhw-
****************************************************************************/
static void smb_umount(char *mount_point)
{
	int fd;
        struct mntent *mnt;
        FILE* mtab;
        FILE* new_mtab;

	/* Programmers Note:
		This routine only gets called to the scene of a disaster
		to shoot the survivors...  A connection that was working
		has now apparently failed.  We have an active mount point
		(presumably) that we need to dump.  If we get errors along
		the way - make some noise, but we are already turning out
		the lights to exit anyways...
	*/
        if (umount(mount_point) != 0) {
                DEBUG(0,("%d: Could not umount %s: %s\n",
			 sys_getpid(), mount_point, strerror(errno)));
                return;
        }

        if ((fd = open(MOUNTED"~", O_RDWR|O_CREAT|O_EXCL, 0600)) == -1) {
                DEBUG(0,("%d: Can't get "MOUNTED"~ lock file", sys_getpid()));
                return;
        }

        close(fd);
	
        if ((mtab = setmntent(MOUNTED, "r")) == NULL) {
                DEBUG(0,("%d: Can't open " MOUNTED ": %s\n",
			 sys_getpid(), strerror(errno)));
                return;
        }

#define MOUNTED_TMP MOUNTED".tmp"

        if ((new_mtab = setmntent(MOUNTED_TMP, "w")) == NULL) {
                DEBUG(0,("%d: Can't open " MOUNTED_TMP ": %s\n",
			 sys_getpid(), strerror(errno)));
                endmntent(mtab);
                return;
        }

        while ((mnt = getmntent(mtab)) != NULL) {
                if (strcmp(mnt->mnt_dir, mount_point) != 0) {
                        addmntent(new_mtab, mnt);
                }
        }

        endmntent(mtab);

        if (fchmod (fileno (new_mtab), S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH) < 0) {
                DEBUG(0,("%d: Error changing mode of %s: %s\n",
			 sys_getpid(), MOUNTED_TMP, strerror(errno)));
                return;
        }

        endmntent(new_mtab);

        if (rename(MOUNTED_TMP, MOUNTED) < 0) {
                DEBUG(0,("%d: Cannot rename %s to %s: %s\n",
			 sys_getpid(), MOUNTED, MOUNTED_TMP, strerror(errno)));
                return;
        }

        if (unlink(MOUNTED"~") == -1) {
                DEBUG(0,("%d: Can't remove "MOUNTED"~", sys_getpid()));
                return;
        }
}


/*
 * Call the smbfs ioctl to install a connection socket,
 * then wait for a signal to reconnect. Note that we do
 * not exit after open_sockets() or send_login() errors,
 * as the smbfs mount would then have no way to recover.
 */
static void send_fs_socket(char *the_service, char *mount_point, struct cli_state *c)
{
	int fd, closed = 0, res = 1;
	pid_t parentpid = getppid();
	struct smb_conn_opt conn_options;

	memset(&conn_options, 0, sizeof(conn_options));

	while (1) {
		if ((fd = open(mount_point, O_RDONLY)) < 0) {
			DEBUG(0,("mount.smbfs[%d]: can't open %s\n",
				 sys_getpid(), mount_point));
			break;
		}

		conn_options.fd = c->fd;
		conn_options.protocol = c->protocol;
		conn_options.case_handling = SMB_CASE_DEFAULT;
		conn_options.max_xmit = c->max_xmit;
		conn_options.server_uid = c->vuid;
		conn_options.tid = c->cnum;
		conn_options.secmode = c->sec_mode;
		conn_options.rawmode = 0;
		conn_options.sesskey = c->sesskey;
		conn_options.maxraw = 0;
		conn_options.capabilities = c->capabilities;
		conn_options.serverzone = c->serverzone/60;

		res = ioctl(fd, SMB_IOC_NEWCONN, &conn_options);
		if (res != 0) {
			DEBUG(0,("mount.smbfs[%d]: ioctl failed, res=%d\n",
				 sys_getpid(), res));
			close(fd);
			break;
		}

		if (parentpid) {
			/* Ok...  We are going to kill the parent.  Now
				is the time to break the process group... */
			setsid();
			/* Send a signal to the parent to terminate */
			kill(parentpid, SIGTERM);
			parentpid = 0;
		}

		close(fd);

		/* This looks wierd but we are only closing the userspace
		   side, the connection has already been passed to smbfs and 
		   it has increased the usage count on the socket.

		   If we don't do this we will "leak" sockets and memory on
		   each reconnection we have to make. */
		c->smb_rw_error = DO_NOT_DO_TDIS;
		cli_shutdown(c);
		c = NULL;

		if (!closed) {
			/* close the name cache so that close_our_files() doesn't steal its FD */
			namecache_shutdown();

			/* redirect stdout & stderr since we can't know that
			   the library functions we use are using DEBUG. */
			if ( (fd = open("/dev/null", O_WRONLY)) < 0)
				DEBUG(2,("mount.smbfs: can't open /dev/null\n"));
			close_our_files(fd);
			if (fd >= 0) {
				dup2(fd, STDOUT_FILENO);
				dup2(fd, STDERR_FILENO);
				close(fd);
			}

			/* here we are no longer interactive */
			set_remote_machine_name("smbmount", False);	/* sneaky ... */
			setup_logging("mount.smbfs", False);
			reopen_logs();
			DEBUG(0, ("mount.smbfs: entering daemon mode for service %s, pid=%d\n", the_service, sys_getpid()));

			closed = 1;
		}

		/* Wait for a signal from smbfs ... but don't continue
                   until we actually get a new connection. */
		while (!c) {
			CatchSignal(SIGUSR1, &usr1_handler);
			pause();
			DEBUG(2,("mount.smbfs[%d]: got signal, getting new socket\n", sys_getpid()));
			c = do_connection(the_service);
		}
	}

	smb_umount(mount_point);
	DEBUG(2,("mount.smbfs[%d]: exit\n", sys_getpid()));
	exit(1);
}


/**
 * Mount a smbfs
 **/
static void init_mount(void)
{
	char mount_point[PATH_MAX+1];
	pstring tmp;
	pstring svc2;
	struct cli_state *c;
	char *args[20];
	int i, status;

	if (realpath(mpoint, mount_point) == NULL) {
		fprintf(stderr, "Could not resolve mount point %s\n", mpoint);
		return;
	}


	c = do_connection(service);
	if (!c) {
		fprintf(stderr,"SMB connection failed\n");
		exit(1);
	}

	/*
		Set up to return as a daemon child and wait in the parent
		until the child say it's ready...
	*/
	daemonize();

	pstrcpy(svc2, service);
	string_replace(svc2, '\\','/');
	string_replace(svc2, ' ','_');

	memset(args, 0, sizeof(args[0])*20);

	i=0;
	args[i++] = "smbmnt";

	args[i++] = mount_point;
	args[i++] = "-s";
	args[i++] = svc2;

	if (mount_ro) {
		args[i++] = "-r";
	}
	if (mount_uid) {
		slprintf(tmp, sizeof(tmp)-1, "%d", mount_uid);
		args[i++] = "-u";
		args[i++] = smb_xstrdup(tmp);
	}
	if (mount_gid) {
		slprintf(tmp, sizeof(tmp)-1, "%d", mount_gid);
		args[i++] = "-g";
		args[i++] = smb_xstrdup(tmp);
	}
	if (mount_fmask) {
		slprintf(tmp, sizeof(tmp)-1, "0%o", mount_fmask);
		args[i++] = "-f";
		args[i++] = smb_xstrdup(tmp);
	}
	if (mount_dmask) {
		slprintf(tmp, sizeof(tmp)-1, "0%o", mount_dmask);
		args[i++] = "-d";
		args[i++] = smb_xstrdup(tmp);
	}
	if (options) {
		args[i++] = "-o";
		args[i++] = options;
	}

	if (sys_fork() == 0) {
		char *smbmnt_path;

		asprintf(&smbmnt_path, "%s/smbmnt", dyn_BINDIR);
		
		if (file_exist(smbmnt_path, NULL)) {
			execv(smbmnt_path, args);
			fprintf(stderr,
				"smbfs/init_mount: execv of %s failed. Error was %s.",
				smbmnt_path, strerror(errno));
		} else {
			execvp("smbmnt", args);
			fprintf(stderr,
				"smbfs/init_mount: execv of %s failed. Error was %s.",
				"smbmnt", strerror(errno));
		}
		free(smbmnt_path);
		exit(1);
	}

	if (waitpid(-1, &status, 0) == -1) {
		fprintf(stderr,"waitpid failed: Error was %s", strerror(errno) );
		/* FIXME: do some proper error handling */
		exit(1);
	}

	if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
		fprintf(stderr,"smbmnt failed: %d\n", WEXITSTATUS(status));
		/* FIXME: do some proper error handling */
		exit(1);
	} else if (WIFSIGNALED(status)) {
		fprintf(stderr, "smbmnt killed by signal %d\n", WTERMSIG(status));
		exit(1);
	}

	/* Ok...  This is the rubicon for that mount point...  At any point
	   after this, if the connections fail and can not be reconstructed
	   for any reason, we will have to unmount the mount point.  There
	   is no exit from the next call...
	*/
	send_fs_socket(service, mount_point, c);
}


/****************************************************************************
get a password from a a file or file descriptor
exit on failure (from smbclient, move to libsmb or shared .c file?)
****************************************************************************/
static void get_password_file(void)
{
	int fd = -1;
	char *p;
	BOOL close_it = False;
	pstring spec;
	char pass[128];

	if ((p = getenv("PASSWD_FD")) != NULL) {
		pstrcpy(spec, "descriptor ");
		pstrcat(spec, p);
		sscanf(p, "%d", &fd);
		close_it = False;
	} else if ((p = getenv("PASSWD_FILE")) != NULL) {
		fd = sys_open(p, O_RDONLY, 0);
		pstrcpy(spec, p);
		if (fd < 0) {
			fprintf(stderr, "Error opening PASSWD_FILE %s: %s\n",
				spec, strerror(errno));
			exit(1);
		}
		close_it = True;
	}

	for(p = pass, *p = '\0'; /* ensure that pass is null-terminated */
	    p && p - pass < sizeof(pass);) {
		switch (read(fd, p, 1)) {
		case 1:
			if (*p != '\n' && *p != '\0') {
				*++p = '\0'; /* advance p, and null-terminate pass */
				break;
			}
		case 0:
			if (p - pass) {
				*p = '\0'; /* null-terminate it, just in case... */
				p = NULL; /* then force the loop condition to become false */
				break;
			} else {
				fprintf(stderr, "Error reading password from file %s: %s\n",
					spec, "empty password\n");
				exit(1);
			}

		default:
			fprintf(stderr, "Error reading password from file %s: %s\n",
				spec, strerror(errno));
			exit(1);
		}
	}
	pstrcpy(password, pass);
	if (close_it)
		close(fd);
}

/****************************************************************************
get username and password from a credentials file
exit on failure (from smbclient, move to libsmb or shared .c file?)
****************************************************************************/
static void read_credentials_file(char *filename)
{
	FILE *auth;
	fstring buf;
	uint16 len = 0;
	char *ptr, *val, *param;

	if ((auth=sys_fopen(filename, "r")) == NULL)
	{
		/* fail if we can't open the credentials file */
		DEBUG(0,("ERROR: Unable to open credentials file!\n"));
		exit (-1);
	}

	while (!feof(auth))
	{
		/* get a line from the file */
		if (!fgets (buf, sizeof(buf), auth))
			continue;
		len = strlen(buf);

		if ((len) && (buf[len-1]=='\n'))
		{
			buf[len-1] = '\0';
			len--;
		}
		if (len == 0)
			continue;

		/* break up the line into parameter & value.
		   will need to eat a little whitespace possibly */
		param = buf;
		if (!(ptr = strchr (buf, '=')))
			continue;
		val = ptr+1;
		*ptr = '\0';

		/* eat leading white space */
		while ((*val!='\0') && ((*val==' ') || (*val=='\t')))
			val++;

		if (strwicmp("password", param) == 0)
		{
			pstrcpy(password, val);
			got_pass = True;
		}
		else if (strwicmp("username", param) == 0) {
			pstrcpy(username, val);
		}

		memset(buf, 0, sizeof(buf));
	}
	fclose(auth);
}


/****************************************************************************
usage on the program
****************************************************************************/
static void usage(void)
{
	printf("Usage: mount.smbfs service mountpoint [-o options,...]\n");

	printf("Version %s\n\n",SAMBA_VERSION_STRING);

	printf(
"Options:\n\
      username=<arg>                  SMB username\n\
      password=<arg>                  SMB password\n\
      credentials=<filename>          file with username/password\n\
      krb                             use kerberos (active directory)\n\
      netbiosname=<arg>               source NetBIOS name\n\
      uid=<arg>                       mount uid or username\n\
      gid=<arg>                       mount gid or groupname\n\
      port=<arg>                      remote SMB port number\n\
      fmask=<arg>                     file umask\n\
      dmask=<arg>                     directory umask\n\
      debug=<arg>                     debug level\n\
      ip=<arg>                        destination host or IP address\n\
      workgroup=<arg>                 workgroup on destination\n\
      sockopt=<arg>                   TCP socket options\n\
      scope=<arg>                     NetBIOS scope\n\
      iocharset=<arg>                 Linux charset (iso8859-1, utf8)\n\
      codepage=<arg>                  server codepage (cp850)\n\
      unicode                         use unicode when communicating with server\n\
      lfs                             large file system support\n\
      ttl=<arg>                       dircache time to live\n\
      guest                           don't prompt for a password\n\
      ro                              mount read-only\n\
      rw                              mount read-write\n\
\n\
This command is designed to be run from within /bin/mount by giving\n\
the option '-t smbfs'. For example:\n\
  mount -t smbfs -o username=tridge,password=foobar //fjall/test /data/test\n\
");
}


/****************************************************************************
  Argument parsing for mount.smbfs interface
  mount will call us like this:
    mount.smbfs device mountpoint -o <options>
  
  <options> is never empty, containing at least rw or ro
 ****************************************************************************/
static void parse_mount_smb(int argc, char **argv)
{
	int opt;
	char *opts;
	char *opteq;
	extern char *optarg;
	int val;
	char *p;

	/* FIXME: This function can silently fail if the arguments are
	 * not in the expected order.

	> The arguments syntax of smbmount 2.2.3a (smbfs of Debian stable)
	> requires that one gives "-o" before further options like username=...
	> . Without -o, the username=.. setting is *silently* ignored. I've
	> spent about an hour trying to find out why I couldn't log in now..

	*/


	if (argc < 2 || argv[1][0] == '-') {
		usage();
		exit(1);
	}
	
	pstrcpy(service, argv[1]);
	pstrcpy(mpoint, argv[2]);

	/* Convert any '/' characters in the service name to
	   '\' characters */
	string_replace(service, '/','\\');
	argc -= 2;
	argv += 2;

	opt = getopt(argc, argv, "o:");
	if(opt != 'o') {
		return;
	}

	options[0] = 0;
	p = options;

	/*
	 * option parsing from nfsmount.c (util-linux-2.9u)
	 */
        for (opts = strtok(optarg, ","); opts; opts = strtok(NULL, ",")) {
		DEBUG(3, ("opts: %s\n", opts));
                if ((opteq = strchr_m(opts, '='))) {
                        val = atoi(opteq + 1);
                        *opteq = '\0';

                        if (!strcmp(opts, "username") || 
			    !strcmp(opts, "logon")) {
				char *lp;
				got_user = True;
				pstrcpy(username,opteq+1);
				if ((lp=strchr_m(username,'%'))) {
					*lp = 0;
					pstrcpy(password,lp+1);
					got_pass = True;
					memset(strchr_m(opteq+1,'%')+1,'X',strlen(password));
				}
				if ((lp=strchr_m(username,'/'))) {
					*lp = 0;
					pstrcpy(workgroup,lp+1);
				}
			} else if(!strcmp(opts, "passwd") ||
				  !strcmp(opts, "password")) {
				pstrcpy(password,opteq+1);
				got_pass = True;
				memset(opteq+1,'X',strlen(password));
			} else if(!strcmp(opts, "credentials")) {
				pstrcpy(credentials,opteq+1);
			} else if(!strcmp(opts, "netbiosname")) {
				pstrcpy(my_netbios_name,opteq+1);
			} else if(!strcmp(opts, "uid")) {
				mount_uid = nametouid(opteq+1);
			} else if(!strcmp(opts, "gid")) {
				mount_gid = nametogid(opteq+1);
			} else if(!strcmp(opts, "port")) {
				smb_port = val;
			} else if(!strcmp(opts, "fmask")) {
				mount_fmask = strtol(opteq+1, NULL, 8);
			} else if(!strcmp(opts, "dmask")) {
				mount_dmask = strtol(opteq+1, NULL, 8);
			} else if(!strcmp(opts, "debug")) {
				DEBUGLEVEL = val;
			} else if(!strcmp(opts, "ip")) {
				dest_ip = *interpret_addr2(opteq+1);
				if (is_zero_ip(dest_ip)) {
					fprintf(stderr,"Can't resolve address %s\n", opteq+1);
					exit(1);
				}
				have_ip = True;
			} else if(!strcmp(opts, "workgroup")) {
				pstrcpy(workgroup,opteq+1);
			} else if(!strcmp(opts, "sockopt")) {
				pstrcpy(user_socket_options,opteq+1);
			} else if(!strcmp(opts, "scope")) {
				set_global_scope(opteq+1);
			} else {
				slprintf(p, sizeof(pstring) - (p - options) - 1, "%s=%s,", opts, opteq+1);
				p += strlen(p);
			}
		} else {
			val = 1;
			if(!strcmp(opts, "nocaps")) {
				fprintf(stderr, "Unhandled option: %s\n", opteq+1);
				exit(1);
			} else if(!strcmp(opts, "guest")) {
				*password = '\0';
				got_pass = True;
			} else if(!strcmp(opts, "krb")) {
#ifdef HAVE_KRB5

				use_kerberos = True;
				if(!status32_smbfs)
					fprintf(stderr, "Warning: kerberos support will only work for samba servers\n");
#else
				fprintf(stderr,"No kerberos support compiled in\n");
				exit(1);
#endif
			} else if(!strcmp(opts, "rw")) {
				mount_ro = 0;
			} else if(!strcmp(opts, "ro")) {
				mount_ro = 1;
			} else if(!strcmp(opts, "unicode")) {
				smbfs_has_unicode = True;
			} else if(!strcmp(opts, "lfs")) {
				smbfs_has_lfs = True;
			} else {
				strncpy(p, opts, sizeof(pstring) - (p - options) - 1);
				p += strlen(opts);
				*p++ = ',';
				*p = 0;
			}
		}
	}

	if (!*service) {
		usage();
		exit(1);
	}

	if (p != options) {
		*(p-1) = 0;	/* remove trailing , */
		DEBUG(3,("passthrough options '%s'\n", options));
	}
}

/****************************************************************************
  main program
****************************************************************************/
 int main(int argc,char *argv[])
{
	extern char *optarg;
	extern int optind;
	char *p;

	DEBUGLEVEL = 1;

	load_case_tables();

	/* here we are interactive, even if run from autofs */
	setup_logging("mount.smbfs",True);

#if 0 /* JRA - Urban says not needed ? */
	/* CLI_FORCE_ASCII=false makes smbmount negotiate unicode. The default
	   is to not announce any unicode capabilities as current smbfs does
	   not support it. */
	p = getenv("CLI_FORCE_ASCII");
	if (p && !strcmp(p, "false"))
		unsetenv("CLI_FORCE_ASCII");
	else
		setenv("CLI_FORCE_ASCII", "true", 1);
#endif

	in_client = True;   /* Make sure that we tell lp_load we are */

	if (getenv("USER")) {
		pstrcpy(username,getenv("USER"));

		if ((p=strchr_m(username,'%'))) {
			*p = 0;
			pstrcpy(password,p+1);
			got_pass = True;
			memset(strchr_m(getenv("USER"),'%')+1,'X',strlen(password));
		}
		strupper_m(username);
	}

	if (getenv("PASSWD")) {
		pstrcpy(password,getenv("PASSWD"));
		got_pass = True;
	}

	if (getenv("PASSWD_FD") || getenv("PASSWD_FILE")) {
		get_password_file();
		got_pass = True;
	}

	if (*username == 0 && getenv("LOGNAME")) {
		pstrcpy(username,getenv("LOGNAME"));
	}

	if (!lp_load(dyn_CONFIGFILE,True,False,False,True)) {
		fprintf(stderr, "Can't load %s - run testparm to debug it\n", 
			dyn_CONFIGFILE);
	}

	parse_mount_smb(argc, argv);

	if (use_kerberos && !got_user) {
		got_pass = True;
	}

	if (*credentials != 0) {
		read_credentials_file(credentials);
	}

	DEBUG(3,("mount.smbfs started (version %s)\n", SAMBA_VERSION_STRING));

	if (*workgroup == 0) {
		pstrcpy(workgroup,lp_workgroup());
	}

	load_interfaces();
	if (!*my_netbios_name) {
		pstrcpy(my_netbios_name, myhostname());
	}
	strupper_m(my_netbios_name);

	init_mount();
	return 0;
}
