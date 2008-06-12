/* 
   Unix SMB/CIFS implementation.
   SMB wrapper directory functions
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

extern pstring smbw_cwd;
extern fstring smbw_prefix;

static struct smbw_dir *smbw_dirs;

extern struct bitmap *smbw_file_bmap;

extern int smbw_busy;

/***************************************************** 
map a fd to a smbw_dir structure
*******************************************************/
struct smbw_dir *smbw_dir(int fd)
{
	struct smbw_dir *dir;

	for (dir=smbw_dirs;dir;dir=dir->next) {
		if (dir->fd == fd) return dir;
	}
	return NULL;
}

/***************************************************** 
check if a DIR* is one of ours
*******************************************************/
int smbw_dirp(DIR *dirp)
{
	struct smbw_dir *d = (struct smbw_dir *)dirp;
	struct smbw_dir *dir;

	for (dir=smbw_dirs;dir;dir=dir->next) {
		if (dir == d) return 1;
	}
	return 0;
}

/***************************************************** 
free a smbw_dir structure and all entries
*******************************************************/
static void free_dir(struct smbw_dir *dir)
{
	if(!dir) return;

	SAFE_FREE(dir->list);
	SAFE_FREE(dir->path);
	ZERO_STRUCTP(dir);
	SAFE_FREE(dir);
}

static struct smbw_dir *cur_dir;

/***************************************************** 
add a entry to a directory listing
*******************************************************/
static void smbw_dir_add(const char *mntpnt, struct file_info *finfo, const char *mask, 
			 void *state)
{
	struct file_info *cdl;

	DEBUG(5,("%s\n", finfo->name));

	if (cur_dir->malloced == cur_dir->count) {
		cdl = (struct file_info *)Realloc(cur_dir->list, 
							    sizeof(cur_dir->list[0])*
							    (cur_dir->count+100), True);
		if (!cdl) {
			/* oops */
			return;
		}
		cur_dir->list = cdl;
		cur_dir->malloced += 100;
	}

	cur_dir->list[cur_dir->count] = *finfo;
	cur_dir->count++;
}

/***************************************************** 
add a entry to a directory listing
*******************************************************/
static void smbw_share_add(const char *share, uint32 type, 
			   const char *comment, void *state)
{
	struct file_info finfo;

	if (strcmp(share,"IPC$") == 0) return;

	ZERO_STRUCT(finfo);

	pstrcpy(finfo.name, share);
	finfo.mode = aRONLY | aDIR;	

	smbw_dir_add("\\", &finfo, NULL, NULL);
}


/***************************************************** 
add a server to a directory listing
*******************************************************/
static void smbw_server_add(const char *name, uint32 type, 
			    const char *comment, void *state)
{
	struct file_info finfo;

	ZERO_STRUCT(finfo);

	pstrcpy(finfo.name, name);
	finfo.mode = aRONLY | aDIR;	

	smbw_dir_add("\\", &finfo, NULL, NULL);
}


/***************************************************** 
add a entry to a directory listing
*******************************************************/
static void smbw_printjob_add(struct print_job_info *job)
{
	struct file_info finfo;

	ZERO_STRUCT(finfo);

	pstrcpy(finfo.name, job->name);
	finfo.mode = aRONLY | aDIR;	
	finfo.mtime = job->t;
	finfo.atime = job->t;
	finfo.ctime = job->t;
	finfo.uid = nametouid(job->user);
	finfo.mode = aRONLY;
	finfo.size = job->size;

	smbw_dir_add("\\", &finfo, NULL, NULL);
}


/***************************************************** 
open a directory on the server
*******************************************************/
int smbw_dir_open(const char *fname)
{
	fstring server, share;
	pstring path;
	struct smbw_server *srv=NULL;
	struct smbw_dir *dir=NULL;
	pstring mask;
	int fd;
	char *s, *p;

	if (!fname) {
		errno = EINVAL;
		return -1;
	}

	smbw_init();

	/* work out what server they are after */
	s = smbw_parse_path(fname, server, share, path);

	DEBUG(4,("dir_open share=%s\n", share));

	/* get a connection to the server */
	srv = smbw_server(server, share);
	if (!srv) {
		/* smbw_server sets errno */
		goto failed;
	}

	dir = SMB_MALLOC_P(struct smbw_dir);
	if (!dir) {
		errno = ENOMEM;
		goto failed;
	}

	ZERO_STRUCTP(dir);

	cur_dir = dir;

	slprintf(mask, sizeof(mask)-1, "%s\\*", path);
	all_string_sub(mask,"\\\\","\\",0);

	if ((p=strstr(srv->server_name,"#01"))) {
		*p = 0;
		smbw_server_add(".",0,"", NULL);
		smbw_server_add("..",0,"", NULL);
		smbw_NetServerEnum(&srv->cli, srv->server_name, 
				   SV_TYPE_DOMAIN_ENUM, smbw_server_add, NULL);
		*p = '#';
	} else if ((p=strstr(srv->server_name,"#1D"))) {
		DEBUG(4,("doing NetServerEnum\n"));
		*p = 0;
		smbw_server_add(".",0,"", NULL);
		smbw_server_add("..",0,"", NULL);
		smbw_NetServerEnum(&srv->cli, srv->server_name, SV_TYPE_ALL,
				   smbw_server_add, NULL);
		*p = '#';
	} else if ((strcmp(srv->cli.dev,"IPC") == 0) || (strequal(share,"IPC$"))) {
		DEBUG(4,("doing NetShareEnum\n"));
		smbw_share_add(".",0,"", NULL);
		smbw_share_add("..",0,"", NULL);
		if (smbw_RNetShareEnum(&srv->cli, smbw_share_add, NULL) < 0) {
			errno = smbw_errno(&srv->cli);
			goto failed;
		}
	} else if (strncmp(srv->cli.dev,"LPT",3) == 0) {
		smbw_share_add(".",0,"", NULL);
		smbw_share_add("..",0,"", NULL);
		if (cli_print_queue(&srv->cli, smbw_printjob_add) < 0) {
			errno = smbw_errno(&srv->cli);
			goto failed;
		}
	} else {
#if 0
		if (strcmp(path,"\\") == 0) {
			smbw_share_add(".",0,"");
			smbw_share_add("..",0,"");
		}
#endif
		if (cli_list(&srv->cli, mask, aHIDDEN|aSYSTEM|aDIR, 
			     smbw_dir_add, NULL) < 0) {
			errno = smbw_errno(&srv->cli);
			goto failed;
		}
	}

	cur_dir = NULL;
	
	fd = open(SMBW_DUMMY, O_WRONLY);
	if (fd == -1) {
		errno = EMFILE;
		goto failed;
	}

	if (bitmap_query(smbw_file_bmap, fd)) {
		DEBUG(0,("ERROR: fd used in smbw_dir_open\n"));
		errno = EIO;
		goto failed;
	}

	DLIST_ADD(smbw_dirs, dir);
	
	bitmap_set(smbw_file_bmap, fd);

	dir->fd = fd;
	dir->srv = srv;
	dir->path = SMB_STRDUP(s);

	DEBUG(4,("  -> %d\n", dir->count));

	return dir->fd;

 failed:
	free_dir(dir);
	
	return -1;
}

/***************************************************** 
a wrapper for fstat() on a directory
*******************************************************/
int smbw_dir_fstat(int fd, struct stat *st)
{
	struct smbw_dir *dir;

	dir = smbw_dir(fd);
	if (!dir) {
		errno = EBADF;
		return -1;
	}

	ZERO_STRUCTP(st);

	smbw_setup_stat(st, "", dir->count*DIRP_SIZE, aDIR);

	st->st_dev = dir->srv->dev;

	return 0;
}

/***************************************************** 
close a directory handle
*******************************************************/
int smbw_dir_close(int fd)
{
	struct smbw_dir *dir;

	dir = smbw_dir(fd);
	if (!dir) {
		errno = EBADF;
		return -1;
	}

	bitmap_clear(smbw_file_bmap, dir->fd);
	close(dir->fd);
	
	DLIST_REMOVE(smbw_dirs, dir);

	free_dir(dir);

	return 0;
}

/***************************************************** 
a wrapper for getdents()
*******************************************************/
int smbw_getdents(unsigned int fd, struct dirent *dirp, int count)
{
	struct smbw_dir *dir;
	int n=0;

	smbw_busy++;

	dir = smbw_dir(fd);
	if (!dir) {
		errno = EBADF;
		smbw_busy--;
		return -1;
	}

	while (count>=DIRP_SIZE && (dir->offset < dir->count)) {
#if HAVE_DIRENT_D_OFF
		dirp->d_off = (dir->offset+1)*DIRP_SIZE;
#endif
		dirp->d_reclen = DIRP_SIZE;
		fstrcpy(&dirp->d_name[0], dir->list[dir->offset].name);
		dirp->d_ino = smbw_inode(dir->list[dir->offset].name);
		dir->offset++;
		count -= dirp->d_reclen;
#if HAVE_DIRENT_D_OFF
		if (dir->offset == dir->count) {
			dirp->d_off = -1;
		}
#endif
		dirp = (struct dirent *)(((char *)dirp) + DIRP_SIZE);
		n++;
	}

	smbw_busy--;
	return n*DIRP_SIZE;
}


/***************************************************** 
a wrapper for chdir()
*******************************************************/
int smbw_chdir(const char *name)
{
	struct smbw_server *srv;
	fstring server, share;
	pstring path;
	uint16 mode = aDIR;
	char *cwd;
	int len;

	smbw_init();

	len = strlen(smbw_prefix);

	if (smbw_busy) return real_chdir(name);

	smbw_busy++;

	if (!name) {
		errno = EINVAL;
		goto failed;
	}

	DEBUG(4,("smbw_chdir(%s)\n", name));

	/* work out what server they are after */
	cwd = smbw_parse_path(name, server, share, path);

	/* a special case - accept cd to /smb */
	if (strncmp(cwd, smbw_prefix, len-1) == 0 &&
	    cwd[len-1] == 0) {
		goto success1;
	}

	if (strncmp(cwd,smbw_prefix,strlen(smbw_prefix))) {
		if (real_chdir(cwd) == 0) {
			goto success2;
		}
		goto failed;
	}

	/* get a connection to the server */
	srv = smbw_server(server, share);
	if (!srv) {
		/* smbw_server sets errno */
		goto failed;
	}

	if (strncmp(srv->cli.dev,"IPC",3) && 
	    !strequal(share, "IPC$") &&
	    strncmp(srv->cli.dev,"LPT",3) &&
	    !smbw_getatr(srv, path, 
			 &mode, NULL, NULL, NULL, NULL, NULL)) {
		errno = smbw_errno(&srv->cli);
		goto failed;
	}

	if (!(mode & aDIR)) {
		errno = ENOTDIR;
		goto failed;
	}

 success1:
	/* we don't want the old directory to be busy */
	real_chdir("/");

 success2:

	DEBUG(4,("set SMBW_CWD to %s\n", cwd));

	pstrcpy(smbw_cwd, cwd);

	smbw_busy--;
	return 0;

 failed:
	smbw_busy--;
	return -1;
}


/***************************************************** 
a wrapper for lseek() on directories
*******************************************************/
off_t smbw_dir_lseek(int fd, off_t offset, int whence)
{
	struct smbw_dir *dir;
	off_t ret;

	dir = smbw_dir(fd);
	if (!dir) {
		errno = EBADF;
		return -1;
	}

	switch (whence) {
	case SEEK_SET:
		dir->offset = offset/DIRP_SIZE;
		break;
	case SEEK_CUR:
		dir->offset += offset/DIRP_SIZE;
		break;
	case SEEK_END:
		dir->offset = (dir->count * DIRP_SIZE) + offset;
		dir->offset /= DIRP_SIZE;
		break;
	}

	ret = dir->offset * DIRP_SIZE;

	DEBUG(4,("   -> %d\n", (int)ret));

	return ret;
}


/***************************************************** 
a wrapper for mkdir()
*******************************************************/
int smbw_mkdir(const char *fname, mode_t mode)
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

	if (!cli_mkdir(&srv->cli, path)) {
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
a wrapper for rmdir()
*******************************************************/
int smbw_rmdir(const char *fname)
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

	if (!cli_rmdir(&srv->cli, path)) {
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
a wrapper for getcwd()
*******************************************************/
char *smbw_getcwd(char *buf, size_t size)
{
	smbw_init();

	if (smbw_busy) {
		return (char *)real_getcwd(buf, size);
	}

	smbw_busy++;

	if (!buf) {
		if (size <= 0) size = strlen(smbw_cwd)+1;
		buf = SMB_MALLOC_ARRAY(char, size);
		if (!buf) {
			errno = ENOMEM;
			smbw_busy--;
			return NULL;
		}
	}

	if (strlen(smbw_cwd) > size-1) {
		errno = ERANGE;
		smbw_busy--;
		return NULL;
	}

	safe_strcpy(buf, smbw_cwd, size);

	smbw_busy--;
	return buf;
}

/***************************************************** 
a wrapper for fchdir()
*******************************************************/
int smbw_fchdir(unsigned int fd)
{
	struct smbw_dir *dir;
	int ret;

	smbw_busy++;

	dir = smbw_dir(fd);
	if (dir) {
		smbw_busy--;
		return chdir(dir->path);
	}	

	ret = real_fchdir(fd);
	if (ret == 0) {
		sys_getwd(smbw_cwd);		
	}

	smbw_busy--;
	return ret;
}

/***************************************************** 
open a directory on the server
*******************************************************/
DIR *smbw_opendir(const char *fname)
{
	int fd;

	smbw_busy++;

	fd = smbw_dir_open(fname);

	if (fd == -1) {
		smbw_busy--;
		return NULL;
	}

	smbw_busy--;

	return (DIR *)smbw_dir(fd);
}

/***************************************************** 
read one entry from a directory
*******************************************************/
struct dirent *smbw_readdir(DIR *dirp)
{
	struct smbw_dir *d = (struct smbw_dir *)dirp;
	static union {
		char buf[DIRP_SIZE];
		struct dirent de;
	} dbuf;

	if (smbw_getdents(d->fd, &dbuf.de, DIRP_SIZE) > 0) 
		return &dbuf.de;

	return NULL;
}

/***************************************************** 
close a DIR*
*******************************************************/
int smbw_closedir(DIR *dirp)
{
	struct smbw_dir *d = (struct smbw_dir *)dirp;
	return smbw_close(d->fd);
}

/***************************************************** 
seek in a directory
*******************************************************/
void smbw_seekdir(DIR *dirp, off_t offset)
{
	struct smbw_dir *d = (struct smbw_dir *)dirp;
	smbw_dir_lseek(d->fd,offset, SEEK_SET);
}

/***************************************************** 
current loc in a directory
*******************************************************/
off_t smbw_telldir(DIR *dirp)
{
	struct smbw_dir *d = (struct smbw_dir *)dirp;
	return smbw_dir_lseek(d->fd,0,SEEK_CUR);
}
