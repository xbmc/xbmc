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

/* NOTE: This file WILL produce compiler warnings. They are unavoidable 

   Do not try and get rid of them by including other include files or
   by including includes.h or proto.h or you will break portability. 
  */

#include "config.h"
#include <sys/types.h>
#include <errno.h>
#include "realcalls.h"

#ifndef NULL
# define NULL ((void *)0)
#endif

 int open(const char *name, int flags, mode_t mode)
{
	if (smbw_path(name)) {
		return smbw_open(name, flags, mode);
	}

	return real_open(name, flags, mode);
}

#ifdef HAVE__OPEN
 int _open(const char *name, int flags, mode_t mode) 
{
	return open(name, flags, mode);
}
#elif HAVE___OPEN
 int __open(char *name, int flags, mode_t mode) 
{
	return open(name, flags, mode);
}
#endif


#ifdef HAVE_OPEN64
 int open64(const char *name, int flags, mode_t mode)
{
	if (smbw_path(name)) {
		return smbw_open(name, flags, mode);
	}

	return real_open64(name, flags, mode);
}
#endif

#ifndef NO_OPEN64_ALIAS
#ifdef HAVE__OPEN64
 int _open64(const char *name, int flags, mode_t mode) 
{
	return open64(name, flags, mode);
}
#elif HAVE___OPEN64
 int __open64(const char *name, int flags, mode_t mode) 
{
	return open64(name, flags, mode);
}
#endif
#endif

#ifdef HAVE_PREAD
 ssize_t pread(int fd, void *buf, size_t size, off_t ofs)
{
	if (smbw_fd(fd)) {
		return smbw_pread(fd, buf, size, ofs);
	}

	return real_pread(fd, buf, size, ofs);
}
#endif

#if 0 && defined(HAVE_PREAD64) && defined(HAVE_EXPLICIT_LARGEFILE_SUPPORT)
 ssize_t pread64(int fd, void *buf, size_t size, off64_t ofs)
{
	if (smbw_fd(fd)) {
		return smbw_pread(fd, buf, size, ofs);
	}

	return real_pread64(fd, buf, size, ofs);
}
#endif

#ifdef HAVE_PWRITE
 ssize_t pwrite(int fd, const void *buf, size_t size, off_t ofs)
{
	if (smbw_fd(fd)) {
		return smbw_pwrite(fd, buf, size, ofs);
	}

	return real_pwrite(fd, buf, size, ofs);
}
#endif

#if 0 && defined(HAVE_PWRITE64) && defined(HAVE_EXPLICIT_LARGEFILE_SUPPORT)
 ssize_t pwrite64(int fd, const void *buf, size_t size, off64_t ofs)
{
	if (smbw_fd(fd)) {
		return smbw_pwrite(fd, buf, size, ofs);
	}

	return real_pwrite64(fd, buf, size, ofs);
}
#endif


 int chdir(const char *name)
{
	return smbw_chdir(name);
}

#ifdef HAVE___CHDIR
 int __chdir(const char *name)
{
	return chdir(name);
}
#elif HAVE__CHDIR
 int _chdir(const char *name)
{
	return chdir(name);
}
#endif


 int close(int fd)
{
	if (smbw_fd(fd)) {
		return smbw_close(fd);
	}
	if (smbw_local_fd(fd)) {
		errno = EBADF;
		return -1;
	}

	return real_close(fd);
}

#ifdef HAVE___CLOSE
 int __close(int fd)
{
	return close(fd);
}
#elif HAVE__CLOSE
 int _close(int fd)
{
	return close(fd);
}
#endif


 int fchdir(int fd)
{
	return smbw_fchdir(fd);
}

#ifdef HAVE___FCHDIR
 int __fchdir(int fd)
{
	return fchdir(fd);
}
#elif HAVE__FCHDIR
 int _fchdir(int fd)
{
	return fchdir(fd);
}
#endif


 int fcntl(int fd, int cmd, long arg)
{
	if (smbw_fd(fd)) {
		return smbw_fcntl(fd, cmd, arg);
	}

	return real_fcntl(fd, cmd, arg);
}


#ifdef HAVE___FCNTL
 int __fcntl(int fd, int cmd, long arg)
{
	return fcntl(fd, cmd, arg);
}
#elif HAVE__FCNTL
 int _fcntl(int fd, int cmd, long arg)
{
	return fcntl(fd, cmd, arg);
}
#endif



#ifdef real_getdents
 int getdents(int fd, void *dirp, unsigned int count)
{
	if (smbw_fd(fd)) {
		return smbw_getdents(fd, dirp, count);
	}

	return real_getdents(fd, dirp, count);
}
#endif

#ifdef HAVE___GETDENTS
 int __getdents(int fd, void *dirp, unsigned int count)
{
	return getdents(fd, dirp, count);
}
#elif HAVE__GETDENTS
 int _getdents(int fd, void *dirp, unsigned int count)
{
	return getdents(fd, dirp, count);
}
#endif


 off_t lseek(int fd, off_t offset, int whence)
{
	if (smbw_fd(fd)) {
		return smbw_lseek(fd, offset, whence);
	}

	return real_lseek(fd, offset, whence);
}

#ifdef HAVE___LSEEK
 off_t __lseek(int fd, off_t offset, int whence)
{
	return lseek(fd, offset, whence);
}
#elif HAVE__LSEEK
 off_t _lseek(int fd, off_t offset, int whence)
{
	return lseek(fd, offset, whence);
}
#endif


 ssize_t read(int fd, void *buf, size_t count)
{
	if (smbw_fd(fd)) {
		return smbw_read(fd, buf, count);
	}

	return real_read(fd, buf, count);
}

#ifdef HAVE___READ
 ssize_t __read(int fd, void *buf, size_t count)
{
	return read(fd, buf, count);
}
#elif HAVE__READ
 ssize_t _read(int fd, void *buf, size_t count)
{
	return read(fd, buf, count);
}
#endif


 ssize_t write(int fd, const void *buf, size_t count)
{
	if (smbw_fd(fd)) {
		return smbw_write(fd, buf, count);
	}

	return real_write(fd, buf, count);
}

#ifdef HAVE___WRITE
 ssize_t __write(int fd, const void *buf, size_t count)
{
	return write(fd, buf, count);
}
#elif HAVE__WRITE
 ssize_t _write(int fd, const void *buf, size_t count)
{
	return write(fd, buf, count);
}
#endif



 int access(const char *name, int mode)
{
	if (smbw_path(name)) {
		return smbw_access(name, mode);
	}

	return real_access(name, mode);
}



 int chmod(const char *name,mode_t mode)
{
	if (smbw_path(name)) {
		return smbw_chmod(name, mode);
	}

	return real_chmod(name, mode);
}



 int chown(const char *name,uid_t owner, gid_t group)
{
	if (smbw_path(name)) {
		return smbw_chown(name, owner, group);
	}

	return real_chown(name, owner, group);
}


 char *getcwd(char *buf, size_t size)
{
	return (char *)smbw_getcwd(buf, size);
}




 int mkdir(const char *name, mode_t mode)
{
	if (smbw_path(name)) {
		return smbw_mkdir(name, mode);
	}

	return real_mkdir(name, mode);
}


#if HAVE___FXSTAT
 int __fxstat(int vers, int fd, void *st)
{
	double xx[32];
	int ret;

	if (smbw_fd(fd)) {
		return smbw_fstat(fd, st);
	}

	ret = real_fstat(fd, xx);
	xstat_convert(vers, st, xx);
	return ret;
}
#endif

#if HAVE___XSTAT
 int __xstat(int vers, const char *name, void *st)
{
	double xx[32];
	int ret;

	if (smbw_path(name)) {
		return smbw_stat(name, st);
	}

	ret = real_stat(name, xx);
	xstat_convert(vers, st, xx);
	return ret;
}
#endif


#if HAVE___LXSTAT
 int __lxstat(int vers, const char *name, void *st)
{
	double xx[32];
	int ret;

	if (smbw_path(name)) {
		return smbw_stat(name, st);
	}

	ret = real_lstat(name, xx);
	xstat_convert(vers, st, xx);
	return ret;
}
#endif


 int stat(const char *name, void *st)
{
#if HAVE___XSTAT
	return __xstat(0, name, st);
#else
	if (smbw_path(name)) {
		return smbw_stat(name, st);
	}
	return real_stat(name, st);
#endif
}

 int lstat(const char *name, void *st)
{
#if HAVE___LXSTAT
	return __lxstat(0, name, st);
#else
	if (smbw_path(name)) {
		return smbw_stat(name, st);
	}
	return real_lstat(name, st);
#endif
}

 int fstat(int fd, void *st)
{
#if HAVE___LXSTAT
	return __fxstat(0, fd, st);
#else
	if (smbw_fd(fd)) {
		return smbw_fstat(fd, st);
	}
	return real_fstat(fd, st);
#endif
}


 int unlink(const char *name)
{
	if (smbw_path(name)) {
		return smbw_unlink(name);
	}

	return real_unlink(name);
}


#ifdef HAVE_UTIME
 int utime(const char *name,void *tvp)
{
	if (smbw_path(name)) {
		return smbw_utime(name, tvp);
	}

	return real_utime(name, tvp);
}
#endif

#ifdef HAVE_UTIMES
 int utimes(const char *name, const struct timeval *tvp)
{
	if (smbw_path(name)) {
		return smbw_utimes(name, tvp);
	}

	return real_utimes(name, tvp);
}
#endif

 int readlink(const char *path, char *buf, size_t bufsize)
{
	if (smbw_path(path)) {
		return smbw_readlink(path, buf, bufsize);
	}

	return real_readlink(path, buf, bufsize);
}


 int rename(const char *oldname,const char *newname)
{
	int p1, p2;
	p1 = smbw_path(oldname); 
	p2 = smbw_path(newname); 
	if (p1 ^ p2) {
		/* can't cross filesystem boundaries */
		errno = EXDEV;
		return -1;
	}
	if (p1 && p2) {
		return smbw_rename(oldname, newname);
	}

	return real_rename(oldname, newname);
}

 int rmdir(const char *name)
{
	if (smbw_path(name)) {
		return smbw_rmdir(name);
	}

	return real_rmdir(name);
}


 int symlink(const char *topath,const char *frompath)
{
	int p1, p2;
	p1 = smbw_path(topath); 
	p2 = smbw_path(frompath); 
	if (p1 || p2) {
		/* can't handle symlinks */
		errno = EPERM;
		return -1;
	}

	return real_symlink(topath, frompath);
}

 int dup(int fd)
{
	if (smbw_fd(fd)) {
		return smbw_dup(fd);
	}

	return real_dup(fd);
}

 int dup2(int oldfd, int newfd)
{
	if (smbw_fd(newfd)) {
		close(newfd);
	}

	if (smbw_fd(oldfd)) {
		return smbw_dup2(oldfd, newfd);
	}

	return real_dup2(oldfd, newfd);
}

#ifdef real_opendir
 void *opendir(const char *name)
{
	if (smbw_path(name)) {
		return (void *)smbw_opendir(name);
	}

	return (void *)real_opendir(name);
}
#endif

#ifdef real_readdir
 void *readdir(void *dir)
{
	if (smbw_dirp(dir)) {
		return (void *)smbw_readdir(dir);
	}

	return (void *)real_readdir(dir);
}
#endif

#ifdef real_closedir
 int closedir(void *dir)
{
	if (smbw_dirp(dir)) {
		return smbw_closedir(dir);
	}

	return real_closedir(dir);
}
#endif

#ifdef real_telldir
 off_t telldir(void *dir)
{
	if (smbw_dirp(dir)) {
		return smbw_telldir(dir);
	}

	return real_telldir(dir);
}
#endif

#ifdef real_seekdir
 int seekdir(void *dir, off_t offset)
{
	if (smbw_dirp(dir)) {
		smbw_seekdir(dir, offset);
		return 0;
	}

	real_seekdir(dir, offset);
	return 0;
}
#endif


#ifndef NO_ACL_WRAPPER
 int  acl(char  *pathp,  int  cmd,  int  nentries, void *aclbufp)
{
	if (smbw_path(pathp)) {
		return smbw_acl(pathp, cmd, nentries, aclbufp);
	}

	return real_acl(pathp, cmd, nentries, aclbufp);
}
#endif

#ifndef NO_FACL_WRAPPER
 int  facl(int fd,  int  cmd,  int  nentries, void *aclbufp)
{
	if (smbw_fd(fd)) {
		return smbw_facl(fd, cmd, nentries, aclbufp);
	}

	return real_facl(fd, cmd, nentries, aclbufp);
}
#endif

 int creat(const char *path, mode_t mode)
{
	extern int creat_bits;
	return open(path, creat_bits, mode);
}

#ifdef HAVE_CREAT64
 int creat64(const char *path, mode_t mode)
{
	extern int creat_bits;
	return open64(path, creat_bits, mode);
}
#endif

#ifdef HAVE_STAT64
  int stat64(const char *name, void *st64)
{
	if (smbw_path(name)) {
		double xx[32];
		int ret = stat(name, xx);
		stat64_convert(xx, st64);
		return ret;
	}
	return real_stat64(name, st64);
}

  int fstat64(int fd, void *st64)
{
	if (smbw_fd(fd)) {
		double xx[32];
		int ret = fstat(fd, xx);
		stat64_convert(xx, st64);
		return ret;
	}
	return real_fstat64(fd, st64);
}

  int lstat64(const char *name, void *st64)
{
	if (smbw_path(name)) {
		double xx[32];
		int ret = lstat(name, xx);
		stat64_convert(xx, st64);
		return ret;
	}
	return real_lstat64(name, st64);
}
#endif

#ifdef HAVE_LLSEEK
  offset_t llseek(int fd, offset_t ofs, int whence)
{
	if (smbw_fd(fd)) {
		return lseek(fd, ofs, whence);
	}
	return real_llseek(fd, ofs, whence);
}
#endif

#ifdef HAVE_READDIR64
 void *readdir64(void *dir)
{
	if (smbw_dirp(dir)) {
		static double xx[70];
		void *d;
		d = (void *)readdir(dir);
		if (!d) return NULL;
		dirent64_convert(d, xx);
		return xx;
	}
	return (void *)real_readdir64(dir);
}
#endif

 int fork(void)
{
	return smbw_fork();
}

