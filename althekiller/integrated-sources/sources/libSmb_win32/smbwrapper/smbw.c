/* 
   Unix SMB/CIFS implementation.
   SMB wrapper functions
   Copyright (C) Andrew Tridgell 1998
   
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
#include "realcalls.h"

pstring smbw_cwd;

static struct smbw_file *smbw_files;
static struct smbw_server *smbw_srvs;

struct bitmap *smbw_file_bmap;

fstring smbw_prefix = SMBW_PREFIX;

int smbw_busy=0;

/* needs to be here because of dumb include files on some systems */
int creat_bits = O_WRONLY|O_CREAT|O_TRUNC;


/***************************************************** 
initialise structures
*******************************************************/
void smbw_init(void)
{
	extern BOOL in_client;
	static int initialised;
	char *p;
	int eno;
	pstring line;

	if (initialised) return;
	initialised = 1;

	eno = errno;

	smbw_busy++;

	DEBUGLEVEL = 0;
	setup_logging("smbsh",True);

	dbf = x_stderr;

	if ((p=smbw_getshared("LOGFILE"))) {
		dbf = sys_fopen(p, "a");
	}

	smbw_file_bmap = bitmap_allocate(SMBW_MAX_OPEN);
	if (!smbw_file_bmap) {
		exit(1);
	}

	in_client = True;

	load_interfaces();

	if ((p=smbw_getshared("SERVICESF"))) {
		pstrcpy(dyn_CONFIGFILE, p);
	}

	lp_load(dyn_CONFIGFILE,True,False,False,True);

	if (!init_names())
		exit(1);

	if ((p=smbw_getshared("DEBUG"))) {
		DEBUGLEVEL = atoi(p);
	}

	if ((p=smbw_getshared("RESOLVE_ORDER"))) {
		lp_set_name_resolve_order(p);
	}

	if ((p=smbw_getshared("PREFIX"))) {
		slprintf(smbw_prefix,sizeof(fstring)-1, "/%s/", p);
		all_string_sub(smbw_prefix,"//", "/", 0);
		DEBUG(2,("SMBW_PREFIX is %s\n", smbw_prefix));
	}

	slprintf(line,sizeof(line)-1,"PWD_%d", (int)getpid());
	
	p = smbw_getshared(line);
	if (!p) {
		sys_getwd(smbw_cwd);
	}
	pstrcpy(smbw_cwd, p);
	DEBUG(4,("Initial cwd is %s\n", smbw_cwd));

	smbw_busy--;

	set_maxfiles(SMBW_MAX_OPEN);

	BlockSignals(True,SIGPIPE);

	errno = eno;
}

/***************************************************** 
determine if a file descriptor is a smb one
*******************************************************/
int smbw_fd(int fd)
{
	if (smbw_busy) return 0;
	return smbw_file_bmap && bitmap_query(smbw_file_bmap, fd);
}

/***************************************************** 
determine if a file descriptor is an internal smbw fd
*******************************************************/
int smbw_local_fd(int fd)
{
	struct smbw_server *srv;
	
	smbw_init();

	if (smbw_busy) return 0;
	if (smbw_shared_fd(fd)) return 1;

	for (srv=smbw_srvs;srv;srv=srv->next) {
		if (srv->cli.fd == fd) return 1;
	}

	return 0;
}

/***************************************************** 
a crude inode number generator
*******************************************************/
ino_t smbw_inode(const char *name)
{
	if (!*name) return 2;
	return (ino_t)str_checksum(name);
}

/***************************************************** 
remove redundent stuff from a filename
*******************************************************/
void clean_fname(char *name)
{
	char *p, *p2;
	int l;
	int modified = 1;

	if (!name) return;

	while (modified) {
		modified = 0;

		DEBUG(5,("cleaning %s\n", name));

		if ((p=strstr(name,"/./"))) {
			modified = 1;
			while (*p) {
				p[0] = p[2];
				p++;
			}
		}

		if ((p=strstr(name,"//"))) {
			modified = 1;
			while (*p) {
				p[0] = p[1];
				p++;
			}
		}

		if (strcmp(name,"/../")==0) {
			modified = 1;
			name[1] = 0;
		}

		if ((p=strstr(name,"/../"))) {
			modified = 1;
			for (p2=(p>name?p-1:p);p2>name;p2--) {
				if (p2[0] == '/') break;
			}
			while (*p2) {
				p2[0] = p2[3];
				p2++;
			}
		}

		if (strcmp(name,"/..")==0) {
			modified = 1;
			name[1] = 0;
		}

		l = strlen(name);
		p = l>=3?(name+l-3):name;
		if (strcmp(p,"/..")==0) {
			modified = 1;
			for (p2=p-1;p2>name;p2--) {
				if (p2[0] == '/') break;
			}
			if (p2==name) {
				p[0] = '/';
				p[1] = 0;
			} else {
				p2[0] = 0;
			}
		}

		l = strlen(name);
		p = l>=2?(name+l-2):name;
		if (strcmp(p,"/.")==0) {
			if (p == name) {
				p[1] = 0;
			} else {
				p[0] = 0;
			}
		}

		if (strncmp(p=name,"./",2) == 0) {      
			modified = 1;
			do {
				p[0] = p[2];
			} while (*p++);
		}

		l = strlen(p=name);
		if (l > 1 && p[l-1] == '/') {
			modified = 1;
			p[l-1] = 0;
		}
	}
}



/***************************************************** 
find a workgroup (any workgroup!) that has a master 
browser on the local network
*******************************************************/
static char *smbw_find_workgroup(void)
{
	fstring server;
	char *p;
	struct in_addr *ip_list = NULL;
	int count = 0;
	int i;

	/* first off see if an existing workgroup name exists */
	p = smbw_getshared("WORKGROUP");
	if (!p) p = lp_workgroup();
	
	slprintf(server, sizeof(server), "%s#1D", p);
	if (smbw_server(server, "IPC$")) return p;

	/* go looking for workgroups */
	if (!name_resolve_bcast(MSBROWSE, 1, &ip_list, &count)) {
		DEBUG(1,("No workgroups found!"));
		return p;
	}

	for (i=0;i<count;i++) {
		static fstring name;
		if (name_status_find("*", 0, 0x1d, ip_list[i], name)) {
			slprintf(server, sizeof(server), "%s#1D", name);
			if (smbw_server(server, "IPC$")) {
				smbw_setshared("WORKGROUP", name);
				SAFE_FREE(ip_list);
				return name;
			}
		}
	}

	SAFE_FREE(ip_list);

	return p;
}

/***************************************************** 
parse a smb path into its components. 
server is one of
  1) the name of the SMB server
  2) WORKGROUP#1D for share listing
  3) WORKGROUP#__ for workgroup listing
share is the share on the server to query
path is the SMB path on the server
return the full path (ie. add cwd if needed)
*******************************************************/
char *smbw_parse_path(const char *fname, char *server, char *share, char *path)
{
	static pstring s;
	char *p;
	int len;
	fstring workgroup;

	/* add cwd if necessary */
	if (fname[0] != '/') {
		slprintf(s, sizeof(s), "%s/%s", smbw_cwd, fname);
	} else {
		pstrcpy(s, fname);
	}
	clean_fname(s);

	/* see if it has the right prefix */
	len = strlen(smbw_prefix)-1;
	if (strncmp(s,smbw_prefix,len) || 
	    (s[len] != '/' && s[len] != 0)) return s;

	/* ok, its for us. Now parse out the workgroup, share etc. */
	p = s+len;
	if (*p == '/') p++;
	if (!next_token(&p, workgroup, "/", sizeof(fstring))) {
		/* we're in /smb - give a list of workgroups */
		slprintf(server,sizeof(fstring), "%s#01", smbw_find_workgroup());
		fstrcpy(share,"IPC$");
		pstrcpy(path,"");
		return s;
	}

	if (!next_token(&p, server, "/", sizeof(fstring))) {
		/* we are in /smb/WORKGROUP */
		slprintf(server,sizeof(fstring), "%s#1D", workgroup);
		fstrcpy(share,"IPC$");
		pstrcpy(path,"");
	}

	if (!next_token(&p, share, "/", sizeof(fstring))) {
		/* we are in /smb/WORKGROUP/SERVER */
		fstrcpy(share,"IPC$");
		pstrcpy(path,"");
	}

	pstrcpy(path, p);

	all_string_sub(path, "/", "\\", 0);

	return s;
}

/***************************************************** 
determine if a path name (possibly relative) is in the 
smb name space
*******************************************************/
int smbw_path(const char *path)
{
	fstring server, share;
	pstring s;
	char *cwd;
	int len;

	if(!path)
		return 0;

	/* this is needed to prevent recursion with the BSD malloc which
	   opens /etc/malloc.conf on the first call */
	if (strncmp(path,"/etc/", 5) == 0) {
		return 0;
	}

	smbw_init();

	len = strlen(smbw_prefix)-1;

	if (path[0] == '/' && strncmp(path,smbw_prefix,len)) {
		return 0;
	}

	if (smbw_busy) return 0;

	DEBUG(3,("smbw_path(%s)\n", path));

	cwd = smbw_parse_path(path, server, share, s);

	if (strncmp(cwd,smbw_prefix,len) == 0 &&
	    (cwd[len] == '/' || cwd[len] == 0)) {
		return 1;
	}

	return 0;
}

/***************************************************** 
return a unix errno from a SMB error pair
*******************************************************/
int smbw_errno(struct cli_state *c)
{
	return cli_errno(c);
}

/* Return a username and password given a server and share name */

void get_envvar_auth_data(char *server, char *share, char **workgroup,
			  char **username, char **password)
{
	/* Fall back to shared memory/environment variables */

	*username = smbw_getshared("USER");
	if (!*username) *username = getenv("USER");
	if (!*username) *username = "guest";

	*workgroup = smbw_getshared("WORKGROUP");
	if (!*workgroup) *workgroup = lp_workgroup();

	*password = smbw_getshared("PASSWORD");
	if (!*password) *password = "";
}

static smbw_get_auth_data_fn get_auth_data_fn = get_envvar_auth_data;

/*****************************************************
set the get auth data function
******************************************************/
void smbw_set_auth_data_fn(smbw_get_auth_data_fn fn)
{
	get_auth_data_fn = fn;
}

/***************************************************** 
return a connection to a server (existing or new)
*******************************************************/
struct smbw_server *smbw_server(char *server, char *share)
{
	struct smbw_server *srv=NULL;
	struct cli_state c;
	char *username;
	char *password;
	char *workgroup;
	struct nmb_name called, calling;
	char *p, *server_n = server;
	fstring group;
	pstring ipenv;
	struct in_addr ip;

        zero_ip(&ip);
	ZERO_STRUCT(c);

	get_auth_data_fn(server, share, &workgroup, &username, &password);

	/* try to use an existing connection */
	for (srv=smbw_srvs;srv;srv=srv->next) {
		if (strcmp(server,srv->server_name)==0 &&
		    strcmp(share,srv->share_name)==0 &&
		    strcmp(workgroup,srv->workgroup)==0 &&
		    strcmp(username, srv->username) == 0) 
			return srv;
	}

	if (server[0] == 0) {
		errno = EPERM;
		return NULL;
	}

	make_nmb_name(&calling, global_myname(), 0x0);
	make_nmb_name(&called , server, 0x20);

	DEBUG(4,("server_n=[%s] server=[%s]\n", server_n, server));

	if ((p=strchr_m(server_n,'#')) && 
	    (strcmp(p+1,"1D")==0 || strcmp(p+1,"01")==0)) {
		struct in_addr sip;
		pstring s;

		fstrcpy(group, server_n);
		p = strchr_m(group,'#');
		*p = 0;
		
		/* cache the workgroup master lookup */
		slprintf(s,sizeof(s)-1,"MASTER_%s", group);
		if (!(server_n = smbw_getshared(s))) {
			if (!find_master_ip(group, &sip)) {
				errno = ENOENT;
				return NULL;
			}
			fstrcpy(group, inet_ntoa(sip));
			server_n = group;
			smbw_setshared(s,server_n);
		}
	}

	DEBUG(4,(" -> server_n=[%s] server=[%s]\n", server_n, server));

 again:
	slprintf(ipenv,sizeof(ipenv)-1,"HOST_%s", server_n);

        zero_ip(&ip);
	if ((p=smbw_getshared(ipenv))) {
		ip = *(interpret_addr2(p));
	}

	/* have to open a new connection */
	if (!cli_initialise(&c) || !cli_connect(&c, server_n, &ip)) {
		errno = ENOENT;
		return NULL;
	}

	if (!cli_session_request(&c, &calling, &called)) {
		cli_shutdown(&c);
		if (strcmp(called.name, "*SMBSERVER")) {
			make_nmb_name(&called , "*SMBSERVER", 0x20);
			goto again;
		}
		errno = ENOENT;
		return NULL;
	}

	DEBUG(4,(" session request ok\n"));

	if (!cli_negprot(&c)) {
		cli_shutdown(&c);
		errno = ENOENT;
		return NULL;
	}

	if (!cli_session_setup(&c, username, 
			       password, strlen(password),
			       password, strlen(password),
			       workgroup) &&
	    /* try an anonymous login if it failed */
	    !cli_session_setup(&c, "", "", 1,"", 0, workgroup)) {
		cli_shutdown(&c);
		errno = EPERM;
		return NULL;
	}

	DEBUG(4,(" session setup ok\n"));

	if (!cli_send_tconX(&c, share, "?????",
			    password, strlen(password)+1)) {
		errno = smbw_errno(&c);
		cli_shutdown(&c);
		return NULL;
	}

	smbw_setshared(ipenv,inet_ntoa(ip));
	
	DEBUG(4,(" tconx ok\n"));

	srv = SMB_MALLOC_P(struct smbw_server);
	if (!srv) {
		errno = ENOMEM;
		goto failed;
	}

	ZERO_STRUCTP(srv);

	srv->cli = c;

	srv->dev = (dev_t)(str_checksum(server) ^ str_checksum(share));

	srv->server_name = SMB_STRDUP(server);
	if (!srv->server_name) {
		errno = ENOMEM;
		goto failed;
	}

	srv->share_name = SMB_STRDUP(share);
	if (!srv->share_name) {
		errno = ENOMEM;
		goto failed;
	}

	srv->workgroup = SMB_STRDUP(workgroup);
	if (!srv->workgroup) {
		errno = ENOMEM;
		goto failed;
	}

	srv->username = SMB_STRDUP(username);
	if (!srv->username) {
		errno = ENOMEM;
		goto failed;
	}

	/* some programs play with file descriptors fairly intimately. We
	   try to get out of the way by duping to a high fd number */
	if (fcntl(SMBW_CLI_FD + srv->cli.fd, F_GETFD) && errno == EBADF) {
		if (dup2(srv->cli.fd,SMBW_CLI_FD+srv->cli.fd) == 
		    srv->cli.fd+SMBW_CLI_FD) {
			close(srv->cli.fd);
			srv->cli.fd += SMBW_CLI_FD;
		}
	}

	DLIST_ADD(smbw_srvs, srv);

	return srv;

 failed:
	cli_shutdown(&c);
	if (!srv) return NULL;

	SAFE_FREE(srv->server_name);
	SAFE_FREE(srv->share_name);
	SAFE_FREE(srv);
	return NULL;
}


/***************************************************** 
map a fd to a smbw_file structure
*******************************************************/
struct smbw_file *smbw_file(int fd)
{
	struct smbw_file *file;

	for (file=smbw_files;file;file=file->next) {
		if (file->fd == fd) return file;
	}
	return NULL;
}

/***************************************************** 
a wrapper for open()
*******************************************************/
int smbw_open(const char *fname, int flags, mode_t mode)
{
	fstring server, share;
	pstring path;
	struct smbw_server *srv=NULL;
	int eno=0, fd = -1;
	struct smbw_file *file=NULL;

	smbw_init();

	if (!fname) {
		errno = EINVAL;
		return -1;
	}

	smbw_busy++;	

	/* work out what server they are after */
	smbw_parse_path(fname, server, share, path);

	/* get a connection to the server */
	srv = smbw_server(server, share);
	if (!srv) {
		/* smbw_server sets errno */
		goto failed;
	}

	if (path[strlen(path)-1] == '\\') {
		fd = -1;
	} else {
		fd = cli_open(&srv->cli, path, flags, DENY_NONE);
	}
	if (fd == -1) {
		/* it might be a directory. Maybe we should use chkpath? */
		eno = smbw_errno(&srv->cli);
		fd = smbw_dir_open(fname);
		if (fd == -1) errno = eno;
		smbw_busy--;
		return fd;
	}

	file = SMB_MALLOC_P(struct smbw_file);
	if (!file) {
		errno = ENOMEM;
		goto failed;
	}

	ZERO_STRUCTP(file);

	file->f = SMB_MALLOC_P(struct smbw_filedes);
	if (!file->f) {
		errno = ENOMEM;
		goto failed;
	}

	ZERO_STRUCTP(file->f);

	file->f->cli_fd = fd;
	file->f->fname = SMB_STRDUP(path);
	if (!file->f->fname) {
		errno = ENOMEM;
		goto failed;
	}
	file->srv = srv;
	file->fd = open(SMBW_DUMMY, O_WRONLY);
	if (file->fd == -1) {
		errno = EMFILE;
		goto failed;
	}

	if (bitmap_query(smbw_file_bmap, file->fd)) {
		DEBUG(0,("ERROR: fd used in smbw_open\n"));
		errno = EIO;
		goto failed;
	}

	file->f->ref_count=1;

	bitmap_set(smbw_file_bmap, file->fd);

	DLIST_ADD(smbw_files, file);

	DEBUG(4,("opened %s\n", fname));

	smbw_busy--;
	return file->fd;

 failed:
	if (fd != -1) {
		cli_close(&srv->cli, fd);
	}
	if (file) {
		if (file->f) {
			SAFE_FREE(file->f->fname);
			SAFE_FREE(file->f);
		}
		SAFE_FREE(file);
	}
	smbw_busy--;
	return -1;
}


/***************************************************** 
a wrapper for pread()
*******************************************************/
ssize_t smbw_pread(int fd, void *buf, size_t count, off_t ofs)
{
	struct smbw_file *file;
	int ret;

	smbw_busy++;

	file = smbw_file(fd);
	if (!file) {
		errno = EBADF;
		smbw_busy--;
		return -1;
	}
	
	ret = cli_read(&file->srv->cli, file->f->cli_fd, buf, ofs, count);

	if (ret == -1) {
		errno = smbw_errno(&file->srv->cli);
		smbw_busy--;
		return -1;
	}

	smbw_busy--;
	return ret;
}

/***************************************************** 
a wrapper for read()
*******************************************************/
ssize_t smbw_read(int fd, void *buf, size_t count)
{
	struct smbw_file *file;
	int ret;

	DEBUG(4,("smbw_read(%d, %d)\n", fd, (int)count));

	smbw_busy++;

	file = smbw_file(fd);
	if (!file) {
		errno = EBADF;
		smbw_busy--;
		return -1;
	}
	
	ret = cli_read(&file->srv->cli, file->f->cli_fd, buf, 
		       file->f->offset, count);

	if (ret == -1) {
		errno = smbw_errno(&file->srv->cli);
		smbw_busy--;
		return -1;
	}

	file->f->offset += ret;
	
	DEBUG(4,(" -> %d\n", ret));

	smbw_busy--;
	return ret;
}

	

/***************************************************** 
a wrapper for write()
*******************************************************/
ssize_t smbw_write(int fd, void *buf, size_t count)
{
	struct smbw_file *file;
	int ret;

	smbw_busy++;

	file = smbw_file(fd);
	if (!file) {
		errno = EBADF;
		smbw_busy--;
		return -1;
	}
	
	ret = cli_write(&file->srv->cli, file->f->cli_fd, 0, buf, file->f->offset, count);

	if (ret == -1) {
		errno = smbw_errno(&file->srv->cli);
		smbw_busy--;
		return -1;
	}

	file->f->offset += ret;

	smbw_busy--;
	return ret;
}

/***************************************************** 
a wrapper for pwrite()
*******************************************************/
ssize_t smbw_pwrite(int fd, void *buf, size_t count, off_t ofs)
{
	struct smbw_file *file;
	int ret;

	smbw_busy++;

	file = smbw_file(fd);
	if (!file) {
		errno = EBADF;
		smbw_busy--;
		return -1;
	}
	
	ret = cli_write(&file->srv->cli, file->f->cli_fd, 0, buf, ofs, count);

	if (ret == -1) {
		errno = smbw_errno(&file->srv->cli);
		smbw_busy--;
		return -1;
	}

	smbw_busy--;
	return ret;
}

/***************************************************** 
a wrapper for close()
*******************************************************/
int smbw_close(int fd)
{
	struct smbw_file *file;

	smbw_busy++;

	file = smbw_file(fd);
	if (!file) {
		int ret = smbw_dir_close(fd);
		smbw_busy--;
		return ret;
	}
	
	if (file->f->ref_count == 1 &&
	    !cli_close(&file->srv->cli, file->f->cli_fd)) {
		errno = smbw_errno(&file->srv->cli);
		smbw_busy--;
		return -1;
	}


	bitmap_clear(smbw_file_bmap, file->fd);
	close(file->fd);
	
	DLIST_REMOVE(smbw_files, file);

	file->f->ref_count--;
	if (file->f->ref_count == 0) {
		SAFE_FREE(file->f->fname);
		SAFE_FREE(file->f);
	}
	ZERO_STRUCTP(file);
	SAFE_FREE(file);
	
	smbw_busy--;

	return 0;
}


/***************************************************** 
a wrapper for fcntl()
*******************************************************/
int smbw_fcntl(int fd, int cmd, long arg)
{
	return 0;
}


/***************************************************** 
a wrapper for access()
*******************************************************/
int smbw_access(const char *name, int mode)
{
	struct stat st;

	DEBUG(4,("smbw_access(%s, 0x%x)\n", name, mode));

	if (smbw_stat(name, &st)) return -1;

	if (((mode & R_OK) && !(st.st_mode & S_IRUSR)) ||
	    ((mode & W_OK) && !(st.st_mode & S_IWUSR)) ||
	    ((mode & X_OK) && !(st.st_mode & S_IXUSR))) {
		errno = EACCES;
		return -1;
	}
	
	return 0;
}

/***************************************************** 
a wrapper for realink() - needed for correct errno setting
*******************************************************/
int smbw_readlink(const char *path, char *buf, size_t bufsize)
{
	struct stat st;
	int ret;

	ret = smbw_stat(path, &st);
	if (ret != 0) {
		DEBUG(4,("readlink(%s) failed\n", path));
		return -1;
	}
	
	/* it exists - say it isn't a link */
	DEBUG(4,("readlink(%s) not a link\n", path));

	errno = EINVAL;
	return -1;
}


/***************************************************** 
a wrapper for unlink()
*******************************************************/
int smbw_unlink(const char *fname)
{
	struct smbw_server *srv;
	fstring server, share;
	pstring path;

	if (!fname) {
		errno = EINVAL;
		return -1;
	}

	smbw_init();

	smbw_busy++;

	/* work out what server they are after */
	smbw_parse_path(fname, server, share, path);

	/* get a connection to the server */
	srv = smbw_server(server, share);
	if (!srv) {
		/* smbw_server sets errno */
		goto failed;
	}

	if (strncmp(srv->cli.dev, "LPT", 3) == 0) {
		int job = smbw_stat_printjob(srv, path, NULL, NULL);
		if (job == -1) {
			goto failed;
		}
		if (cli_printjob_del(&srv->cli, job) != 0) {
			goto failed;
		}
	} else if (!cli_unlink(&srv->cli, path)) {
		errno = smbw_errno(&srv->cli);
		goto failed;
	}

	smbw_busy--;
	return 0;

 failed:
	smbw_busy--;
	return -1;
}


/***************************************************** 
a wrapper for rename()
*******************************************************/
int smbw_rename(const char *oldname, const char *newname)
{
	struct smbw_server *srv;
	fstring server1, share1;
	pstring path1;
	fstring server2, share2;
	pstring path2;

	if (!oldname || !newname) {
		errno = EINVAL;
		return -1;
	}

	smbw_init();

	DEBUG(4,("smbw_rename(%s,%s)\n", oldname, newname));

	smbw_busy++;

	/* work out what server they are after */
	smbw_parse_path(oldname, server1, share1, path1);
	smbw_parse_path(newname, server2, share2, path2);

	if (strcmp(server1, server2) || strcmp(share1, share2)) {
		/* can't cross filesystems */
		errno = EXDEV;
		return -1;
	}

	/* get a connection to the server */
	srv = smbw_server(server1, share1);
	if (!srv) {
		/* smbw_server sets errno */
		goto failed;
	}

	if (!cli_rename(&srv->cli, path1, path2)) {
		int eno = smbw_errno(&srv->cli);
		if (eno != EEXIST ||
		    !cli_unlink(&srv->cli, path2) ||
		    !cli_rename(&srv->cli, path1, path2)) {
			errno = eno;
			goto failed;
		}
	}

	smbw_busy--;
	return 0;

 failed:
	smbw_busy--;
	return -1;
}


/***************************************************** 
a wrapper for utime and utimes
*******************************************************/
static int smbw_settime(const char *fname, time_t t)
{
	struct smbw_server *srv;
	fstring server, share;
	pstring path;
	uint16 mode;

	if (!fname) {
		errno = EINVAL;
		return -1;
	}

	smbw_init();

	smbw_busy++;

	/* work out what server they are after */
	smbw_parse_path(fname, server, share, path);

	/* get a connection to the server */
	srv = smbw_server(server, share);
	if (!srv) {
		/* smbw_server sets errno */
		goto failed;
	}

	if (!cli_getatr(&srv->cli, path, &mode, NULL, NULL)) {
		errno = smbw_errno(&srv->cli);
		goto failed;
	}

	if (!cli_setatr(&srv->cli, path, mode, t)) {
		/* some servers always refuse directory changes */
		if (!(mode & aDIR)) {
			errno = smbw_errno(&srv->cli);
			goto failed;
		}
	}

	smbw_busy--;
	return 0;

 failed:
	smbw_busy--;
	return -1;
}

/***************************************************** 
a wrapper for utime 
*******************************************************/
int smbw_utime(const char *fname, void *buf)
{
	struct utimbuf *tbuf = (struct utimbuf *)buf;
	return smbw_settime(fname, tbuf?tbuf->modtime:time(NULL));
}

/***************************************************** 
a wrapper for utime 
*******************************************************/
int smbw_utimes(const char *fname, void *buf)
{
	struct timeval *tbuf = (struct timeval *)buf;
	return smbw_settime(fname, tbuf?tbuf->tv_sec:time(NULL));
}


/***************************************************** 
a wrapper for chown()
*******************************************************/
int smbw_chown(const char *fname, uid_t owner, gid_t group)
{
	struct smbw_server *srv;
	fstring server, share;
	pstring path;
	uint16 mode;

	if (!fname) {
		errno = EINVAL;
		return -1;
	}

	smbw_init();

	smbw_busy++;

	/* work out what server they are after */
	smbw_parse_path(fname, server, share, path);

	/* get a connection to the server */
	srv = smbw_server(server, share);
	if (!srv) {
		/* smbw_server sets errno */
		goto failed;
	}

	if (!cli_getatr(&srv->cli, path, &mode, NULL, NULL)) {
		errno = smbw_errno(&srv->cli);
		goto failed;
	}
	
	/* assume success */

	smbw_busy--;
	return 0;

 failed:
	smbw_busy--;
	return -1;
}

/***************************************************** 
a wrapper for chmod()
*******************************************************/
int smbw_chmod(const char *fname, mode_t newmode)
{
	struct smbw_server *srv;
	fstring server, share;
	pstring path;
	uint32 mode;

	if (!fname) {
		errno = EINVAL;
		return -1;
	}

	smbw_init();

	smbw_busy++;

	/* work out what server they are after */
	smbw_parse_path(fname, server, share, path);

	/* get a connection to the server */
	srv = smbw_server(server, share);
	if (!srv) {
		/* smbw_server sets errno */
		goto failed;
	}

	mode = 0;

	if (!(newmode & (S_IWUSR | S_IWGRP | S_IWOTH))) mode |= aRONLY;
	if ((newmode & S_IXUSR) && lp_map_archive(-1)) mode |= aARCH;
	if ((newmode & S_IXGRP) && lp_map_system(-1)) mode |= aSYSTEM;
	if ((newmode & S_IXOTH) && lp_map_hidden(-1)) mode |= aHIDDEN;

	if (!cli_setatr(&srv->cli, path, mode, 0)) {
		errno = smbw_errno(&srv->cli);
		goto failed;
	}
	
	smbw_busy--;
	return 0;

 failed:
	smbw_busy--;
	return -1;
}

/***************************************************** 
a wrapper for lseek()
*******************************************************/
off_t smbw_lseek(int fd, off_t offset, int whence)
{
	struct smbw_file *file;
	SMB_OFF_T size;

	smbw_busy++;

	file = smbw_file(fd);
	if (!file) {
		off_t ret = smbw_dir_lseek(fd, offset, whence);
		smbw_busy--;
		return ret;
	}

	switch (whence) {
	case SEEK_SET:
		file->f->offset = offset;
		break;
	case SEEK_CUR:
		file->f->offset += offset;
		break;
	case SEEK_END:
		if (!cli_qfileinfo(&file->srv->cli, file->f->cli_fd, 
				   NULL, &size, NULL, NULL, NULL, 
				   NULL, NULL) &&
		    !cli_getattrE(&file->srv->cli, file->f->cli_fd, 
				  NULL, &size, NULL, NULL, NULL)) {
			errno = EINVAL;
			smbw_busy--;
			return -1;
		}
		file->f->offset = size + offset;
		break;
	}

	smbw_busy--;
	return file->f->offset;
}


/***************************************************** 
a wrapper for dup()
*******************************************************/
int smbw_dup(int fd)
{
	int fd2;
	struct smbw_file *file, *file2;

	smbw_busy++;

	file = smbw_file(fd);
	if (!file) {
		errno = EBADF;
		goto failed;
	}

	fd2 = dup(file->fd);
	if (fd2 == -1) {
		goto failed;
	}

	if (bitmap_query(smbw_file_bmap, fd2)) {
		DEBUG(0,("ERROR: fd already open in dup!\n"));
		errno = EIO;
		goto failed;
	}

	file2 = SMB_MALLOC_P(struct smbw_file);
	if (!file2) {
		close(fd2);
		errno = ENOMEM;
		goto failed;
	}

	ZERO_STRUCTP(file2);

	*file2 = *file;
	file2->fd = fd2;

	file->f->ref_count++;

	bitmap_set(smbw_file_bmap, fd2);
	
	DLIST_ADD(smbw_files, file2);
	
	smbw_busy--;
	return fd2;

 failed:
	smbw_busy--;
	return -1;
}


/***************************************************** 
a wrapper for dup2()
*******************************************************/
int smbw_dup2(int fd, int fd2)
{
	struct smbw_file *file, *file2;

	smbw_busy++;

	file = smbw_file(fd);
	if (!file) {
		errno = EBADF;
		goto failed;
	}

	if (bitmap_query(smbw_file_bmap, fd2)) {
		DEBUG(0,("ERROR: fd already open in dup2!\n"));
		errno = EIO;
		goto failed;
	}

	if (dup2(file->fd, fd2) != fd2) {
		goto failed;
	}

	file2 = SMB_MALLOC_P(struct smbw_file);
	if (!file2) {
		close(fd2);
		errno = ENOMEM;
		goto failed;
	}

	ZERO_STRUCTP(file2);

	*file2 = *file;
	file2->fd = fd2;

	file->f->ref_count++;

	bitmap_set(smbw_file_bmap, fd2);
	
	DLIST_ADD(smbw_files, file2);
	
	smbw_busy--;
	return fd2;

 failed:
	smbw_busy--;
	return -1;
}


/***************************************************** 
close a connection to a server
*******************************************************/
static void smbw_srv_close(struct smbw_server *srv)
{
	smbw_busy++;

	cli_shutdown(&srv->cli);

	SAFE_FREE(srv->server_name);
	SAFE_FREE(srv->share_name);

	DLIST_REMOVE(smbw_srvs, srv);

	ZERO_STRUCTP(srv);

	SAFE_FREE(srv);
	
	smbw_busy--;
}

/***************************************************** 
when we fork we have to close all connections and files
in the child
*******************************************************/
int smbw_fork(void)
{
	pid_t child;
	int p[2];
	char c=0;
	pstring line;

	struct smbw_file *file, *next_file;
	struct smbw_server *srv, *next_srv;

	if (pipe(p)) return real_fork();

	child = real_fork();

	if (child) {
		/* block the parent for a moment until the sockets are
                   closed */
		close(p[1]);
		read(p[0], &c, 1);
		close(p[0]);
		return child;
	}

	close(p[0]);

	/* close all files */
	for (file=smbw_files;file;file=next_file) {
		next_file = file->next;
		close(file->fd);
	}

	/* close all server connections */
	for (srv=smbw_srvs;srv;srv=next_srv) {
		next_srv = srv->next;
		smbw_srv_close(srv);
	}

	slprintf(line,sizeof(line)-1,"PWD_%d", (int)getpid());
	smbw_setshared(line,smbw_cwd);

	/* unblock the parent */
	write(p[1], &c, 1);
	close(p[1]);

	/* and continue in the child */
	return 0;
}

#ifndef NO_ACL_WRAPPER
/***************************************************** 
say no to acls
*******************************************************/
 int smbw_acl(const char *pathp, int cmd, int nentries, aclent_t *aclbufp)
{
	if (cmd == GETACL || cmd == GETACLCNT) return 0;
	errno = ENOSYS;
	return -1;
}
#endif

#ifndef NO_FACL_WRAPPER
/***************************************************** 
say no to acls
*******************************************************/
 int smbw_facl(int fd, int cmd, int nentries, aclent_t *aclbufp)
{
	if (cmd == GETACL || cmd == GETACLCNT) return 0;
	errno = ENOSYS;
	return -1;
}
#endif

#ifdef HAVE_EXPLICIT_LARGEFILE_SUPPORT
#ifdef HAVE_STAT64
/* this can't be in wrapped.c because of include conflicts */
 void stat64_convert(struct stat *st, struct stat64 *st64)
{
	st64->st_size = st->st_size;
	st64->st_mode = st->st_mode;
	st64->st_ino = st->st_ino;
	st64->st_dev = st->st_dev;
	st64->st_rdev = st->st_rdev;
	st64->st_nlink = st->st_nlink;
	st64->st_uid = st->st_uid;
	st64->st_gid = st->st_gid;
	st64->st_atime = st->st_atime;
	st64->st_mtime = st->st_mtime;
	st64->st_ctime = st->st_ctime;
#ifdef HAVE_STAT_ST_BLKSIZE
	st64->st_blksize = st->st_blksize;
#endif
#ifdef HAVE_STAT_ST_BLOCKS
	st64->st_blocks = st->st_blocks;
#endif
}
#endif

#ifdef HAVE_READDIR64
 void dirent64_convert(struct dirent *d, struct dirent64 *d64)
{
	d64->d_ino = d->d_ino;
	d64->d_off = d->d_off;
	d64->d_reclen = d->d_reclen;
	pstrcpy(d64->d_name, d->d_name);
}
#endif
#endif


#ifdef HAVE___XSTAT
/* Definition of `struct stat' used in the linux kernel..  */
struct kernel_stat {
	unsigned short int st_dev;
	unsigned short int __pad1;
	unsigned long int st_ino;
	unsigned short int st_mode;
	unsigned short int st_nlink;
	unsigned short int st_uid;
	unsigned short int st_gid;
	unsigned short int st_rdev;
	unsigned short int __pad2;
	unsigned long int st_size;
	unsigned long int st_blksize;
	unsigned long int st_blocks;
	unsigned long int st_atime_;
	unsigned long int __unused1;
	unsigned long int st_mtime_;
	unsigned long int __unused2;
	unsigned long int st_ctime_;
	unsigned long int __unused3;
	unsigned long int __unused4;
	unsigned long int __unused5;
};

/*
 * Prototype for gcc in 'fussy' mode.
 */
 void xstat_convert(int vers, struct stat *st, struct kernel_stat *kbuf);
 void xstat_convert(int vers, struct stat *st, struct kernel_stat *kbuf)
{
#ifdef _STAT_VER_LINUX_OLD
	if (vers == _STAT_VER_LINUX_OLD) {
		memcpy(st, kbuf, sizeof(*st));
		return;
	}
#endif

	ZERO_STRUCTP(st);

	st->st_dev = kbuf->st_dev;
	st->st_ino = kbuf->st_ino;
	st->st_mode = kbuf->st_mode;
	st->st_nlink = kbuf->st_nlink;
	st->st_uid = kbuf->st_uid;
	st->st_gid = kbuf->st_gid;
	st->st_rdev = kbuf->st_rdev;
	st->st_size = kbuf->st_size;
#ifdef HAVE_STAT_ST_BLKSIZE
	st->st_blksize = kbuf->st_blksize;
#endif
#ifdef HAVE_STAT_ST_BLOCKS
	st->st_blocks = kbuf->st_blocks;
#endif
	st->st_atime = kbuf->st_atime_;
	st->st_mtime = kbuf->st_mtime_;
	st->st_ctime = kbuf->st_ctime_;
}
#endif
