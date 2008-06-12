/* 
   Unix SMB/CIFS implementation.
   Samba system utilities
   Copyright (C) Andrew Tridgell 1992-1998
   Copyright (C) Jeremy Allison  1998-2005
   Copyright (C) Timur Bakeyev        2005
   
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

#ifdef HAVE_SYS_PRCTL_H
#include <sys/prctl.h>
#endif

/*
   The idea is that this file will eventually have wrappers around all
   important system calls in samba. The aims are:

   - to enable easier porting by putting OS dependent stuff in here

   - to allow for hooks into other "pseudo-filesystems"

   - to allow easier integration of things like the japanese extensions

   - to support the philosophy of Samba to expose the features of
     the OS within the SMB model. In general whatever file/printer/variable
     expansions/etc make sense to the OS should be acceptable to Samba.
*/



/*******************************************************************
 A wrapper for usleep in case we don't have one.
********************************************************************/

int sys_usleep(long usecs)
{
#ifndef HAVE_USLEEP
	struct timeval tval;
#endif

	/*
	 * We need this braindamage as the glibc usleep
	 * is not SPEC1170 complient... grumble... JRA.
	 */

	if(usecs < 0 || usecs > 1000000) {
		errno = EINVAL;
		return -1;
	}

#if HAVE_USLEEP
	usleep(usecs);
	return 0;
#else /* HAVE_USLEEP */
	/*
	 * Fake it with select...
	 */
	tval.tv_sec = 0;
	tval.tv_usec = usecs/1000;
	select(0,NULL,NULL,NULL,&tval);
	return 0;
#endif /* HAVE_USLEEP */
}

/*******************************************************************
A read wrapper that will deal with EINTR.
********************************************************************/

ssize_t sys_read(int fd, void *buf, size_t count)
{
	ssize_t ret;

	do {
#ifdef _XBOX
		// for now let's hope sys_read is only used for socket fd's
		ret = recv(fd, buf, count, 0);
#elif
		ret = read(fd, buf, count);
#endif
	} while (ret == -1 && errno == EINTR);
	return ret;
}

/*******************************************************************
A write wrapper that will deal with EINTR.
********************************************************************/

ssize_t sys_write(int fd, const void *buf, size_t count)
{
	ssize_t ret;

	do {
#ifdef _XBOX
    // for now let's hope sys_write is only used for socket fd's
		ret = send(fd, buf, count, 0);
#else
		ret = write(fd, buf, count);
#endif
	} while (ret == -1 && errno == EINTR);
	return ret;
}


/*******************************************************************
A pread wrapper that will deal with EINTR and 64-bit file offsets.
********************************************************************/

#if defined(HAVE_PREAD) || defined(HAVE_PREAD64)
ssize_t sys_pread(int fd, void *buf, size_t count, SMB_OFF_T off)
{
	ssize_t ret;

	do {
#if defined(HAVE_EXPLICIT_LARGEFILE_SUPPORT) && defined(HAVE_OFF64_T) && defined(HAVE_PREAD64)
		ret = pread64(fd, buf, count, off);
#else
		ret = pread(fd, buf, count, off);
#endif
	} while (ret == -1 && errno == EINTR);
	return ret;
}
#endif

/*******************************************************************
A write wrapper that will deal with EINTR and 64-bit file offsets.
********************************************************************/

#if defined(HAVE_PWRITE) || defined(HAVE_PWRITE64)
ssize_t sys_pwrite(int fd, const void *buf, size_t count, SMB_OFF_T off)
{
	ssize_t ret;

	do {
#if defined(HAVE_EXPLICIT_LARGEFILE_SUPPORT) && defined(HAVE_OFF64_T) && defined(HAVE_PWRITE64)
		ret = pwrite64(fd, buf, count, off);
#else
		ret = pwrite(fd, buf, count, off);
#endif
	} while (ret == -1 && errno == EINTR);
	return ret;
}
#endif

/*******************************************************************
A send wrapper that will deal with EINTR.
********************************************************************/

ssize_t sys_send(int s, const void *msg, size_t len, int flags)
{
	ssize_t ret;

	do {
		ret = send(s, msg, len, flags);
	} while (ret == -1 && errno == EINTR);
	return ret;
}

/*******************************************************************
A sendto wrapper that will deal with EINTR.
********************************************************************/

ssize_t sys_sendto(int s,  const void *msg, size_t len, int flags, const struct sockaddr *to, socklen_t tolen)
{
	ssize_t ret;

	do {
		ret = sendto(s, msg, len, flags, to, tolen);
	} while (ret == -1 && errno == EINTR);
	return ret;
}

/*******************************************************************
A recvfrom wrapper that will deal with EINTR.
********************************************************************/

ssize_t sys_recvfrom(int s, void *buf, size_t len, int flags, struct sockaddr *from, socklen_t *fromlen)
{
	ssize_t ret;

	do {
		ret = recvfrom(s, buf, len, flags, from, fromlen);
	} while (ret == -1 && errno == EINTR);
	return ret;
}

/*******************************************************************
A fcntl wrapper that will deal with EINTR.
********************************************************************/

int sys_fcntl_ptr(int fd, int cmd, void *arg)
{
	OutputDebugString("Todo: sys_fcntl_ptr\n");
#ifndef _XBOX
	int ret;

	do {
		ret = fcntl(fd, cmd, arg);
	} while (ret == -1 && errno == EINTR);
	return ret;
#endif //_XBOX
	return -1;
}

/*******************************************************************
A fcntl wrapper that will deal with EINTR.
********************************************************************/

int sys_fcntl_long(int fd, int cmd, long arg)
{
	OutputDebugString("Todo: sys_fcntl_ptr\n");
#ifndef _XBOX
	int ret;

	do {
		ret = fcntl(fd, cmd, arg);
	} while (ret == -1 && errno == EINTR);
	return ret;
#endif //_XBOX
	return -1;
}

/*******************************************************************
A stat() wrapper that will deal with 64 bit filesizes.
********************************************************************/

int sys_stat(const char *fname,SMB_STRUCT_STAT *sbuf)
{
	int ret;
#if defined(HAVE_EXPLICIT_LARGEFILE_SUPPORT) && defined(HAVE_OFF64_T) && defined(HAVE_STAT64)
	ret = stat64(fname, sbuf);
#else
	ret = stat(fname, sbuf);
#endif
	/* we always want directories to appear zero size */
	if (ret == 0 && S_ISDIR(sbuf->st_mode)) sbuf->st_size = 0;
	return ret;
}

/*******************************************************************
 An fstat() wrapper that will deal with 64 bit filesizes.
********************************************************************/

int sys_fstat(int fd,SMB_STRUCT_STAT *sbuf)
{
	int ret;
#if defined(HAVE_EXPLICIT_LARGEFILE_SUPPORT) && defined(HAVE_OFF64_T) && defined(HAVE_FSTAT64)
	ret = fstat64(fd, sbuf);
#else
	ret = fstat(fd, sbuf);
#endif
	/* we always want directories to appear zero size */
	if (ret == 0 && S_ISDIR(sbuf->st_mode)) sbuf->st_size = 0;
	return ret;
}

/*******************************************************************
 An lstat() wrapper that will deal with 64 bit filesizes.
********************************************************************/

int sys_lstat(const char *fname,SMB_STRUCT_STAT *sbuf)
{
	int ret;
#if defined(HAVE_EXPLICIT_LARGEFILE_SUPPORT) && defined(HAVE_OFF64_T) && defined(HAVE_LSTAT64)
	ret = lstat64(fname, sbuf);
#elif _XBOX
	// xbox doesn't have symbolic links, just use stat
	ret = _stat64(fname, sbuf);
#else
	ret = lstat(fname, sbuf);
#endif
	/* we always want directories to appear zero size */
	if (ret == 0 && S_ISDIR(sbuf->st_mode)) sbuf->st_size = 0;
	return ret;
}

/*******************************************************************
 An ftruncate() wrapper that will deal with 64 bit filesizes.
********************************************************************/

int sys_ftruncate(int fd, SMB_OFF_T offset)
{
#if defined(HAVE_EXPLICIT_LARGEFILE_SUPPORT) && defined(HAVE_OFF64_T) && defined(HAVE_FTRUNCATE64)
	return ftruncate64(fd, offset);
#else
	return ftruncate(fd, offset);
#endif
}

/*******************************************************************
 An lseek() wrapper that will deal with 64 bit filesizes.
********************************************************************/

SMB_OFF_T sys_lseek(int fd, SMB_OFF_T offset, int whence)
{
#if defined(HAVE_EXPLICIT_LARGEFILE_SUPPORT) && defined(HAVE_OFF64_T) && defined(HAVE_LSEEK64)
	return lseek64(fd, offset, whence);
#else
	return lseek(fd, offset, whence);
#endif
}

/*******************************************************************
 An fseek() wrapper that will deal with 64 bit filesizes.
********************************************************************/

int sys_fseek(FILE *fp, SMB_OFF_T offset, int whence)
{
#if defined(HAVE_EXPLICIT_LARGEFILE_SUPPORT) && defined(LARGE_SMB_OFF_T) && defined(HAVE_FSEEK64)
	return fseek64(fp, offset, whence);
#elif defined(HAVE_EXPLICIT_LARGEFILE_SUPPORT) && defined(LARGE_SMB_OFF_T) && defined(HAVE_FSEEKO64)
	return fseeko64(fp, offset, whence);
#else
	return fseek(fp, offset, whence);
#endif
}

/*******************************************************************
 An ftell() wrapper that will deal with 64 bit filesizes.
********************************************************************/

SMB_OFF_T sys_ftell(FILE *fp)
{
#if defined(HAVE_EXPLICIT_LARGEFILE_SUPPORT) && defined(LARGE_SMB_OFF_T) && defined(HAVE_FTELL64)
	return (SMB_OFF_T)ftell64(fp);
#elif defined(HAVE_EXPLICIT_LARGEFILE_SUPPORT) && defined(LARGE_SMB_OFF_T) && defined(HAVE_FTELLO64)
	return (SMB_OFF_T)ftello64(fp);
#else
	return (SMB_OFF_T)ftell(fp);
#endif
}

/*******************************************************************
 A creat() wrapper that will deal with 64 bit filesizes.
********************************************************************/

int sys_creat(const char *path, mode_t mode)
{
#if defined(HAVE_EXPLICIT_LARGEFILE_SUPPORT) && defined(HAVE_CREAT64)
	return creat64(path, mode);
#else
	/*
	 * If creat64 isn't defined then ensure we call a potential open64.
	 * JRA.
	 */
	return sys_open(path, O_WRONLY | O_CREAT | O_TRUNC, mode);
#endif
}

/*******************************************************************
 An open() wrapper that will deal with 64 bit filesizes.
********************************************************************/

int sys_open(const char *path, int oflag, mode_t mode)
{
#if defined(HAVE_EXPLICIT_LARGEFILE_SUPPORT) && defined(HAVE_OPEN64)
	return open64(path, oflag, mode);
#else
	return open(path, oflag, mode);
#endif
}

/*******************************************************************
 An fopen() wrapper that will deal with 64 bit filesizes.
********************************************************************/

FILE *sys_fopen(const char *path, const char *type)
{
#if defined(HAVE_EXPLICIT_LARGEFILE_SUPPORT) && defined(HAVE_FOPEN64)
	return fopen64(path, type);
#else
	return fopen(path, type);
#endif
}

/*******************************************************************
 An opendir wrapper that will deal with 64 bit filesizes.
********************************************************************/

SMB_STRUCT_DIR *sys_opendir(const char *name)
{
#if defined(HAVE_EXPLICIT_LARGEFILE_SUPPORT) && defined(HAVE_OPENDIR64)
	return opendir64(name);
#else
	return opendir(name);
#endif
}

/*******************************************************************
 A readdir wrapper that will deal with 64 bit filesizes.
********************************************************************/

SMB_STRUCT_DIRENT *sys_readdir(SMB_STRUCT_DIR *dirp)
{
#if defined(HAVE_EXPLICIT_LARGEFILE_SUPPORT) && defined(HAVE_READDIR64)
	return readdir64(dirp);
#else
	return readdir(dirp);
#endif
}

/*******************************************************************
 A seekdir wrapper that will deal with 64 bit filesizes.
********************************************************************/

void sys_seekdir(SMB_STRUCT_DIR *dirp, long offset)
{
#ifdef _XBOX
  OutputDebugString("TODO:_sys_seekdir");
#else
#if defined(HAVE_EXPLICIT_LARGEFILE_SUPPORT) && defined(HAVE_SEEKDIR64)
	seekdir64(dirp, offset);
#else
	seekdir(dirp, offset);
#endif
#endif
}

/*******************************************************************
 A telldir wrapper that will deal with 64 bit filesizes.
********************************************************************/

long sys_telldir(SMB_STRUCT_DIR *dirp)
{
#ifdef _XBOX
  OutputDebugString("TODO:_sys_seekdir");
  return 0;
#else
#if defined(HAVE_EXPLICIT_LARGEFILE_SUPPORT) && defined(HAVE_TELLDIR64)
	return (long)telldir64(dirp);
#else
	return (long)telldir(dirp);
#endif
#endif
}

/*******************************************************************
 A rewinddir wrapper that will deal with 64 bit filesizes.
********************************************************************/

void sys_rewinddir(SMB_STRUCT_DIR *dirp)
{
#if defined(HAVE_EXPLICIT_LARGEFILE_SUPPORT) && defined(HAVE_REWINDDIR64)
	rewinddir64(dirp);
#else
	rewinddir(dirp);
#endif
}

/*******************************************************************
 A close wrapper that will deal with 64 bit filesizes.
********************************************************************/

int sys_closedir(SMB_STRUCT_DIR *dirp)
{
#if defined(HAVE_EXPLICIT_LARGEFILE_SUPPORT) && defined(HAVE_CLOSEDIR64)
	return closedir64(dirp);
#else
	return closedir(dirp);
#endif
}

/*******************************************************************
 An mknod() wrapper that will deal with 64 bit filesizes.
********************************************************************/

int sys_mknod(const char *path, mode_t mode, SMB_DEV_T dev)
{
#if defined(HAVE_MKNOD) || defined(HAVE_MKNOD64)
#if defined(HAVE_EXPLICIT_LARGEFILE_SUPPORT) && defined(HAVE_MKNOD64) && defined(HAVE_DEV64_T)
	return mknod64(path, mode, dev);
#else
	return mknod(path, mode, dev);
#endif
#else
	/* No mknod system call. */
	errno = ENOSYS;
	return -1;
#endif
}

/*******************************************************************
 Wrapper for realpath.
********************************************************************/

char *sys_realpath(const char *path, char *resolved_path)
{
#if defined(HAVE_REALPATH)
	return realpath(path, resolved_path);
#else
	/* As realpath is not a system call we can't return ENOSYS. */
	errno = EINVAL;
	return NULL;
#endif
}

/*******************************************************************
The wait() calls vary between systems
********************************************************************/

int sys_waitpid(pid_t pid,int *status,int options)
{
#ifdef HAVE_WAITPID
	return waitpid(pid,status,options);
#else /* HAVE_WAITPID */
	return wait4(pid, status, options, NULL);
#endif /* HAVE_WAITPID */
}

/*******************************************************************
 System wrapper for getwd
********************************************************************/

char *sys_getwd(char *s)
{
	char *wd;
#ifdef HAVE_GETCWD
	wd = (char *)getcwd(s, sizeof (pstring));
#else
	wd = (char *)getwd(s);
#endif
	return wd;
}

/*******************************************************************
system wrapper for symlink
********************************************************************/

int sys_symlink(const char *oldpath, const char *newpath)
{
#ifndef HAVE_SYMLINK
	errno = ENOSYS;
	return -1;
#else
	return symlink(oldpath, newpath);
#endif
}

/*******************************************************************
system wrapper for readlink
********************************************************************/

int sys_readlink(const char *path, char *buf, size_t bufsiz)
{
#ifndef HAVE_READLINK
	errno = ENOSYS;
	return -1;
#else
	return readlink(path, buf, bufsiz);
#endif
}

/*******************************************************************
system wrapper for link
********************************************************************/

int sys_link(const char *oldpath, const char *newpath)
{
#ifndef HAVE_LINK
	errno = ENOSYS;
	return -1;
#else
	return link(oldpath, newpath);
#endif
}

/*******************************************************************
chown isn't used much but OS/2 doesn't have it
********************************************************************/

int sys_chown(const char *fname,uid_t uid,gid_t gid)
{
#ifndef HAVE_CHOWN
	static int done;
	if (!done) {
		DEBUG(1,("WARNING: no chown!\n"));
		done=1;
	}
	errno = ENOSYS;
	return -1;
#else
	return(chown(fname,uid,gid));
#endif
}

/*******************************************************************
os/2 also doesn't have chroot
********************************************************************/
int sys_chroot(const char *dname)
{
#ifndef HAVE_CHROOT
	static int done;
	if (!done) {
		DEBUG(1,("WARNING: no chroot!\n"));
		done=1;
	}
	errno = ENOSYS;
	return -1;
#else
	return(chroot(dname));
#endif
}

/**************************************************************************
A wrapper for gethostbyname() that tries avoids looking up hostnames 
in the root domain, which can cause dial-on-demand links to come up for no
apparent reason.
****************************************************************************/

struct hostent *sys_gethostbyname(const char *name)
{
#ifdef REDUCE_ROOT_DNS_LOOKUPS
	char query[256], hostname[256];
	char *domain;

	/* Does this name have any dots in it? If so, make no change */

	if (strchr_m(name, '.'))
		return(gethostbyname(name));

	/* Get my hostname, which should have domain name 
		attached. If not, just do the gethostname on the
		original string. 
	*/

	gethostname(hostname, sizeof(hostname) - 1);
	hostname[sizeof(hostname) - 1] = 0;
	if ((domain = strchr_m(hostname, '.')) == NULL)
		return(gethostbyname(name));

	/* Attach domain name to query and do modified query.
		If names too large, just do gethostname on the
		original string.
	*/

	if((strlen(name) + strlen(domain)) >= sizeof(query))
		return(gethostbyname(name));

	slprintf(query, sizeof(query)-1, "%s%s", name, domain);
	return(gethostbyname(query));
#else /* REDUCE_ROOT_DNS_LOOKUPS */
	return(gethostbyname(name));
#endif /* REDUCE_ROOT_DNS_LOOKUPS */
}


#if defined(HAVE_POSIX_CAPABILITIES)

#ifdef HAVE_SYS_CAPABILITY_H

#if defined(BROKEN_REDHAT_7_SYSTEM_HEADERS) && !defined(_I386_STATFS_H) && !defined(_PPC_STATFS_H)
#define _I386_STATFS_H
#define _PPC_STATFS_H
#define BROKEN_REDHAT_7_STATFS_WORKAROUND
#endif

#include <sys/capability.h>

#ifdef BROKEN_REDHAT_7_STATFS_WORKAROUND
#undef _I386_STATFS_H
#undef _PPC_STATFS_H
#undef BROKEN_REDHAT_7_STATFS_WORKAROUND
#endif

#endif /* HAVE_SYS_CAPABILITY_H */

/**************************************************************************
 Try and abstract process capabilities (for systems that have them).
****************************************************************************/

/* Set the POSIX capabilities needed for the given purpose into the effective
 * capability set of the current process. Make sure they are always removed
 * from the inheritable set, because there is no circumstance in which our
 * children should inherit our elevated privileges.
 */
static BOOL set_process_capability(enum smbd_capability capability,
				   BOOL	enable)
{
	cap_value_t cap_vals[2] = {0};
	int num_cap_vals = 0;

	cap_t cap;

#if defined(HAVE_PRCTL) && defined(PR_GET_KEEPCAPS) && defined(PR_SET_KEEPCAPS)
	/* On Linux, make sure that any capabilities we grab are sticky
	 * across UID changes. We expect that this would allow us to keep both
	 * the effective and permitted capability sets, but as of circa 2.6.16,
	 * only the permitted set is kept. It is a bug (which we work around)
	 * that the effective set is lost, but we still require the effective
	 * set to be kept.
	 */
	if (!prctl(PR_GET_KEEPCAPS)) {
		prctl(PR_SET_KEEPCAPS, 1);
	}
#endif

	cap = cap_get_proc();
	if (cap == NULL) {
		DEBUG(0,("set_process_capability: cap_get_proc failed: %s\n",
			strerror(errno)));
		return False;
	}

	switch (capability) {
		case KERNEL_OPLOCK_CAPABILITY:
#ifdef CAP_NETWORK_MGT
			/* IRIX has CAP_NETWORK_MGT for oplocks. */
			cap_vals[num_cap_vals++] = CAP_NETWORK_MGT;
#endif
			break;
		case DMAPI_ACCESS_CAPABILITY:
#ifdef CAP_DEVICE_MGT
			/* IRIX has CAP_DEVICE_MGT for DMAPI access. */
			cap_vals[num_cap_vals++] = CAP_DEVICE_MGT;
#elif CAP_MKNOD
			/* Linux has CAP_MKNOD for DMAPI access. */
			cap_vals[num_cap_vals++] = CAP_MKNOD;
#endif
			break;
	}

	SMB_ASSERT(num_cap_vals <= ARRAY_SIZE(cap_vals));

	if (num_cap_vals == 0) {
		cap_free(cap);
		return True;
	}

	cap_set_flag(cap, CAP_EFFECTIVE, num_cap_vals, cap_vals,
		enable ? CAP_SET : CAP_CLEAR);

	/* We never want to pass capabilities down to our children, so make
	 * sure they are not inherited.
	 */
	cap_set_flag(cap, CAP_INHERITABLE, num_cap_vals, cap_vals, CAP_CLEAR);

	if (cap_set_proc(cap) == -1) {
		DEBUG(0, ("set_process_capability: cap_set_proc failed: %s\n",
			strerror(errno)));
		cap_free(cap);
		return False;
	}

	cap_free(cap);
	return True;
}

#endif /* HAVE_POSIX_CAPABILITIES */

/****************************************************************************
 Gain the oplock capability from the kernel if possible.
****************************************************************************/

void set_effective_capability(enum smbd_capability capability)
{
#if defined(HAVE_POSIX_CAPABILITIES)
	set_process_capability(capability, True);
#endif /* HAVE_POSIX_CAPABILITIES */
}

void drop_effective_capability(enum smbd_capability capability)
{
#if defined(HAVE_POSIX_CAPABILITIES)
	set_process_capability(capability, False);
#endif /* HAVE_POSIX_CAPABILITIES */
}

/**************************************************************************
 Wrapper for random().
****************************************************************************/

long sys_random(void)
{
#if defined(HAVE_RANDOM)
	return (long)random();
#elif defined(HAVE_RAND)
	return (long)rand();
#else
	DEBUG(0,("Error - no random function available !\n"));
	exit(1);
#endif
}

/**************************************************************************
 Wrapper for srandom().
****************************************************************************/

void sys_srandom(unsigned int seed)
{
#if defined(HAVE_SRANDOM)
	srandom(seed);
#elif defined(HAVE_SRAND)
	srand(seed);
#else
	DEBUG(0,("Error - no srandom function available !\n"));
	exit(1);
#endif
}

/**************************************************************************
 Returns equivalent to NGROUPS_MAX - using sysconf if needed.
****************************************************************************/

int groups_max(void)
{
#if defined(SYSCONF_SC_NGROUPS_MAX)
	int ret = sysconf(_SC_NGROUPS_MAX);
	return (ret == -1) ? NGROUPS_MAX : ret;
#else
	return NGROUPS_MAX;
#endif
}

/**************************************************************************
 Wrapper for getgroups. Deals with broken (int) case.
****************************************************************************/

int sys_getgroups(int setlen, gid_t *gidset)
{
#if !defined(HAVE_BROKEN_GETGROUPS)
	return getgroups(setlen, gidset);
#else

	GID_T gid;
	GID_T *group_list;
	int i, ngroups;

	if(setlen == 0) {
		return getgroups(setlen, &gid);
	}

	/*
	 * Broken case. We need to allocate a
	 * GID_T array of size setlen.
	 */

	if(setlen < 0) {
		errno = EINVAL; 
		return -1;
	} 

	if (setlen == 0)
		setlen = groups_max();

	if((group_list = (GID_T *)malloc(setlen * sizeof(GID_T))) == NULL) {
		DEBUG(0,("sys_getgroups: Malloc fail.\n"));
		return -1;
	}

	if((ngroups = getgroups(setlen, group_list)) < 0) {
		int saved_errno = errno;
		SAFE_FREE(group_list);
		errno = saved_errno;
		return -1;
	}

	for(i = 0; i < ngroups; i++)
		gidset[i] = (gid_t)group_list[i];

	SAFE_FREE(group_list);
	return ngroups;
#endif /* HAVE_BROKEN_GETGROUPS */
}


/**************************************************************************
 Wrapper for setgroups. Deals with broken (int) case. Automatically used
 if we have broken getgroups.
****************************************************************************/

int sys_setgroups(int setlen, gid_t *gidset)
{
#if !defined(HAVE_SETGROUPS)
	errno = ENOSYS;
	return -1;
#endif /* HAVE_SETGROUPS */

#if !defined(HAVE_BROKEN_GETGROUPS)
	return setgroups(setlen, gidset);
#else

	GID_T *group_list;
	int i ; 

	if (setlen == 0)
		return 0 ;

	if (setlen < 0 || setlen > groups_max()) {
		errno = EINVAL; 
		return -1;   
	}

	/*
	 * Broken case. We need to allocate a
	 * GID_T array of size setlen.
	 */

	if((group_list = (GID_T *)malloc(setlen * sizeof(GID_T))) == NULL) {
		DEBUG(0,("sys_setgroups: Malloc fail.\n"));
		return -1;    
	}
 
	for(i = 0; i < setlen; i++) 
		group_list[i] = (GID_T) gidset[i]; 

	if(setgroups(setlen, group_list) != 0) {
		int saved_errno = errno;
		SAFE_FREE(group_list);
		errno = saved_errno;
		return -1;
	}
 
	SAFE_FREE(group_list);
	return 0 ;
#endif /* HAVE_BROKEN_GETGROUPS */
}

/**************************************************************************
 Wrappers for setpwent(), getpwent() and endpwent()
****************************************************************************/

void sys_setpwent(void)
{
	setpwent();
}

struct passwd *sys_getpwent(void)
{
	return getpwent();
}

void sys_endpwent(void)
{
	endpwent();
}

/**************************************************************************
 Wrappers for getpwnam(), getpwuid(), getgrnam(), getgrgid()
****************************************************************************/

struct passwd *sys_getpwnam(const char *name)
{
	return getpwnam(name);
}

struct passwd *sys_getpwuid(uid_t uid)
{
	return getpwuid(uid);
}

struct group *sys_getgrnam(const char *name)
{
	return getgrnam(name);
}

struct group *sys_getgrgid(gid_t gid)
{
	return getgrgid(gid);
}

#if 0 /* NOT CURRENTLY USED - JRA */
/**************************************************************************
 The following are the UNICODE versions of *all* system interface functions
 called within Samba. Ok, ok, the exceptions are the gethostbyXX calls,
 which currently are left as ascii as they are not used other than in name
 resolution.
****************************************************************************/

/**************************************************************************
 Wide stat. Just narrow and call sys_xxx.
****************************************************************************/

int wsys_stat(const smb_ucs2_t *wfname,SMB_STRUCT_STAT *sbuf)
{
	pstring fname;
	return sys_stat(unicode_to_unix(fname,wfname,sizeof(fname)), sbuf);
}

/**************************************************************************
 Wide lstat. Just narrow and call sys_xxx.
****************************************************************************/

int wsys_lstat(const smb_ucs2_t *wfname,SMB_STRUCT_STAT *sbuf)
{
	pstring fname;
	return sys_lstat(unicode_to_unix(fname,wfname,sizeof(fname)), sbuf);
}

/**************************************************************************
 Wide creat. Just narrow and call sys_xxx.
****************************************************************************/

int wsys_creat(const smb_ucs2_t *wfname, mode_t mode)
{
	pstring fname;
	return sys_creat(unicode_to_unix(fname,wfname,sizeof(fname)), mode);
}

/**************************************************************************
 Wide open. Just narrow and call sys_xxx.
****************************************************************************/

int wsys_open(const smb_ucs2_t *wfname, int oflag, mode_t mode)
{
	pstring fname;
	return sys_open(unicode_to_unix(fname,wfname,sizeof(fname)), oflag, mode);
}

/**************************************************************************
 Wide fopen. Just narrow and call sys_xxx.
****************************************************************************/

FILE *wsys_fopen(const smb_ucs2_t *wfname, const char *type)
{
	pstring fname;
	return sys_fopen(unicode_to_unix(fname,wfname,sizeof(fname)), type);
}

/**************************************************************************
 Wide opendir. Just narrow and call sys_xxx.
****************************************************************************/

SMB_STRUCT_DIR *wsys_opendir(const smb_ucs2_t *wfname)
{
	pstring fname;
	return opendir(unicode_to_unix(fname,wfname,sizeof(fname)));
}

/**************************************************************************
 Wide readdir. Return a structure pointer containing a wide filename.
****************************************************************************/

SMB_STRUCT_WDIRENT *wsys_readdir(SMB_STRUCT_DIR *dirp)
{
	static SMB_STRUCT_WDIRENT retval;
	SMB_STRUCT_DIRENT *dirval = sys_readdir(dirp);

	if(!dirval)
		return NULL;

	/*
	 * The only POSIX defined member of this struct is d_name.
	 */

	unix_to_unicode(retval.d_name,dirval->d_name,sizeof(retval.d_name));

	return &retval;
}

/**************************************************************************
 Wide getwd. Call sys_xxx and widen. Assumes s points to a wpstring.
****************************************************************************/

smb_ucs2_t *wsys_getwd(smb_ucs2_t *s)
{
	pstring fname;
	char *p = sys_getwd(fname);

	if(!p)
		return NULL;

	return unix_to_unicode(s, p, sizeof(wpstring));
}

/**************************************************************************
 Wide chown. Just narrow and call sys_xxx.
****************************************************************************/

int wsys_chown(const smb_ucs2_t *wfname, uid_t uid, gid_t gid)
{
	pstring fname;
	return chown(unicode_to_unix(fname,wfname,sizeof(fname)), uid, gid);
}

/**************************************************************************
 Wide chroot. Just narrow and call sys_xxx.
****************************************************************************/

int wsys_chroot(const smb_ucs2_t *wfname)
{
	pstring fname;
	return chroot(unicode_to_unix(fname,wfname,sizeof(fname)));
}

/**************************************************************************
 Wide getpwnam. Return a structure pointer containing wide names.
****************************************************************************/

SMB_STRUCT_WPASSWD *wsys_getpwnam(const smb_ucs2_t *wname)
{
	static SMB_STRUCT_WPASSWD retval;
	fstring name;
	struct passwd *pwret = sys_getpwnam(unicode_to_unix(name,wname,sizeof(name)));

	if(!pwret)
		return NULL;

	unix_to_unicode(retval.pw_name, pwret->pw_name, sizeof(retval.pw_name));
	retval.pw_passwd = pwret->pw_passwd;
	retval.pw_uid = pwret->pw_uid;
	retval.pw_gid = pwret->pw_gid;
	unix_to_unicode(retval.pw_gecos, pwret->pw_gecos, sizeof(retval.pw_gecos));
	unix_to_unicode(retval.pw_dir, pwret->pw_dir, sizeof(retval.pw_dir));
	unix_to_unicode(retval.pw_shell, pwret->pw_shell, sizeof(retval.pw_shell));

	return &retval;
}

/**************************************************************************
 Wide getpwuid. Return a structure pointer containing wide names.
****************************************************************************/

SMB_STRUCT_WPASSWD *wsys_getpwuid(uid_t uid)
{
	static SMB_STRUCT_WPASSWD retval;
	struct passwd *pwret = sys_getpwuid(uid);

	if(!pwret)
		return NULL;

	unix_to_unicode(retval.pw_name, pwret->pw_name, sizeof(retval.pw_name));
	retval.pw_passwd = pwret->pw_passwd;
	retval.pw_uid = pwret->pw_uid;
	retval.pw_gid = pwret->pw_gid;
	unix_to_unicode(retval.pw_gecos, pwret->pw_gecos, sizeof(retval.pw_gecos));
	unix_to_unicode(retval.pw_dir, pwret->pw_dir, sizeof(retval.pw_dir));
	unix_to_unicode(retval.pw_shell, pwret->pw_shell, sizeof(retval.pw_shell));

	return &retval;
}
#endif /* NOT CURRENTLY USED - JRA */

/**************************************************************************
 Extract a command into an arg list. Uses a static pstring for storage.
 Caller frees returned arg list (which contains pointers into the static pstring).
****************************************************************************/

static char **extract_args(const char *command)
{
	static pstring trunc_cmd;
	char *ptr;
	int argcl;
	char **argl = NULL;
	int i;

	pstrcpy(trunc_cmd, command);

	if(!(ptr = strtok(trunc_cmd, " \t"))) {
		errno = EINVAL;
		return NULL;
	}

	/*
	 * Count the args.
	 */

	for( argcl = 1; ptr; ptr = strtok(NULL, " \t"))
		argcl++;

	if((argl = (char **)SMB_MALLOC((argcl + 1) * sizeof(char *))) == NULL)
		return NULL;

	/*
	 * Now do the extraction.
	 */

	pstrcpy(trunc_cmd, command);

	ptr = strtok(trunc_cmd, " \t");
	i = 0;
	argl[i++] = ptr;

	while((ptr = strtok(NULL, " \t")) != NULL)
		argl[i++] = ptr;

	argl[i++] = NULL;
	return argl;
}

/**************************************************************************
 Wrapper for fork. Ensures that mypid is reset. Used so we can write
 a sys_getpid() that only does a system call *once*.
****************************************************************************/

static pid_t mypid = (pid_t)-1;

pid_t sys_fork(void)
{
	pid_t forkret = fork();

	if (forkret == (pid_t)0) /* Child - reset mypid so sys_getpid does a system call. */
		mypid = (pid_t) -1;

	return forkret;
}

/**************************************************************************
 Wrapper for getpid. Ensures we only do a system call *once*.
****************************************************************************/

pid_t sys_getpid(void)
{
	if (mypid == (pid_t)-1)
		mypid = getpid();

	return mypid;
}

/**************************************************************************
 Wrapper for popen. Safer as it doesn't search a path.
 Modified from the glibc sources.
 modified by tridge to return a file descriptor. We must kick our FILE* habit
****************************************************************************/

typedef struct _popen_list
{
	int fd;
	pid_t child_pid;
	struct _popen_list *next;
} popen_list;

static popen_list *popen_chain;

int sys_popen(const char *command)
{
	OutputDebugString("sys_popen not supported\n");
#ifndef _XBOX
	int parent_end, child_end;
	int pipe_fds[2];
	popen_list *entry = NULL;
	char **argl = NULL;

	if (pipe(pipe_fds) < 0)
		return -1;

	parent_end = pipe_fds[0];
	child_end = pipe_fds[1];

	if (!*command) {
		errno = EINVAL;
		goto err_exit;
	}

	if((entry = SMB_MALLOC_P(popen_list)) == NULL)
		goto err_exit;

	ZERO_STRUCTP(entry);

	/*
	 * Extract the command and args into a NULL terminated array.
	 */

	if(!(argl = extract_args(command)))
		goto err_exit;

	entry->child_pid = sys_fork();

	if (entry->child_pid == -1) {
		goto err_exit;
	}

	if (entry->child_pid == 0) {

		/*
		 * Child !
		 */

		int child_std_end = STDOUT_FILENO;
		popen_list *p;

		close(parent_end);
		if (child_end != child_std_end) {
			dup2 (child_end, child_std_end);
			close (child_end);
		}

		/*
		 * POSIX.2:  "popen() shall ensure that any streams from previous
		 * popen() calls that remain open in the parent process are closed
		 * in the new child process."
		 */

		for (p = popen_chain; p; p = p->next)
			close(p->fd);

		execv(argl[0], argl);
		_exit (127);
	}

	/*
	 * Parent.
	 */

	close (child_end);
	SAFE_FREE(argl);

	/* Link into popen_chain. */
	entry->next = popen_chain;
	popen_chain = entry;
	entry->fd = parent_end;

	return entry->fd;

err_exit:

	SAFE_FREE(entry);
	SAFE_FREE(argl);
	close(pipe_fds[0]);
	close(pipe_fds[1]);
	return -1;
#endif //_XBOX
	return -1;
}

/**************************************************************************
 Wrapper for pclose. Modified from the glibc sources.
****************************************************************************/

int sys_pclose(int fd)
{
	int wstatus;
	popen_list **ptr = &popen_chain;
	popen_list *entry = NULL;
	pid_t wait_pid;
	int status = -1;

	/* Unlink from popen_chain. */
	for ( ; *ptr != NULL; ptr = &(*ptr)->next) {
		if ((*ptr)->fd == fd) {
			entry = *ptr;
			*ptr = (*ptr)->next;
			status = 0;
			break;
		}
	}

	if (status < 0 || close(entry->fd) < 0)
		return -1;

	/*
	 * As Samba is catching and eating child process
	 * exits we don't really care about the child exit
	 * code, a -1 with errno = ECHILD will do fine for us.
	 */

	do {
		wait_pid = sys_waitpid (entry->child_pid, &wstatus, 0);
	} while (wait_pid == -1 && errno == EINTR);

	SAFE_FREE(entry);

	if (wait_pid == -1)
		return -1;
	return wstatus;
}

/**************************************************************************
 Wrappers for dlopen, dlsym, dlclose.
****************************************************************************/

void *sys_dlopen(const char *name, int flags)
{
#if defined(HAVE_DLOPEN)
	return dlopen(name, flags);
#else
	return NULL;
#endif
}

void *sys_dlsym(void *handle, const char *symbol)
{
#if defined(HAVE_DLSYM)
    return dlsym(handle, symbol);
#else
    return NULL;
#endif
}

int sys_dlclose (void *handle)
{
#if defined(HAVE_DLCLOSE)
	return dlclose(handle);
#else
	return 0;
#endif
}

const char *sys_dlerror(void)
{
#if defined(HAVE_DLERROR)
	return dlerror();
#else
	return NULL;
#endif
}

int sys_dup2(int oldfd, int newfd) 
{
#if defined(HAVE_DUP2)
	return dup2(oldfd, newfd);
#else
	errno = ENOSYS;
	return -1;
#endif
}

/**************************************************************************
 Wrapper for Admin Logs.
****************************************************************************/

 void sys_adminlog(int priority, const char *format_str, ...) 
{
	va_list ap;
	int ret;
	char *msgbuf = NULL;

	va_start( ap, format_str );
	ret = vasprintf( &msgbuf, format_str, ap );
	va_end( ap );

	if (ret == -1)
		return;

#if defined(HAVE_SYSLOG)
	syslog( priority, "%s", msgbuf );
#else
	DEBUG(0,("%s", msgbuf ));
#endif
	SAFE_FREE(msgbuf);
}

/**************************************************************************
 Wrappers for extented attribute calls. Based on the Linux package with
 support for IRIX and (Net|Free)BSD also. Expand as other systems have them.
****************************************************************************/

ssize_t sys_getxattr (const char *path, const char *name, void *value, size_t size)
{
#if defined(HAVE_GETXATTR)
	return getxattr(path, name, value, size);
#elif defined(HAVE_GETEA)
	return getea(path, name, value, size);
#elif defined(HAVE_EXTATTR_GET_FILE)
	char *s;
	ssize_t retval;
	int attrnamespace = (strncmp(name, "system", 6) == 0) ? 
		EXTATTR_NAMESPACE_SYSTEM : EXTATTR_NAMESPACE_USER;
	const char *attrname = ((s=strchr_m(name, '.')) == NULL) ? name : s + 1;
	/*
	 * The BSD implementation has a nasty habit of silently truncating
	 * the returned value to the size of the buffer, so we have to check
	 * that the buffer is large enough to fit the returned value.
	 */
	if((retval=extattr_get_file(path, attrnamespace, attrname, NULL, 0)) >= 0) {
		if(retval > size) {
			errno = ERANGE;
			return -1;
		}
		if((retval=extattr_get_file(path, attrnamespace, attrname, value, size)) >= 0)
			return retval;
	}

	DEBUG(10,("sys_getxattr: extattr_get_file() failed with: %s\n", strerror(errno)));
	return -1;
#elif defined(HAVE_ATTR_GET)
	int retval, flags = 0;
	int valuelength = (int)size;
	char *attrname = strchr(name,'.') + 1;
	
	if (strncmp(name, "system", 6) == 0) flags |= ATTR_ROOT;

	retval = attr_get(path, attrname, (char *)value, &valuelength, flags);

	return retval ? retval : valuelength;
#else
	errno = ENOSYS;
	return -1;
#endif
}

ssize_t sys_lgetxattr (const char *path, const char *name, void *value, size_t size)
{
#if defined(HAVE_LGETXATTR)
	return lgetxattr(path, name, value, size);
#elif defined(HAVE_LGETEA)
	return lgetea(path, name, value, size);
#elif defined(HAVE_EXTATTR_GET_LINK)
	char *s;
	ssize_t retval;
	int attrnamespace = (strncmp(name, "system", 6) == 0) ? 
		EXTATTR_NAMESPACE_SYSTEM : EXTATTR_NAMESPACE_USER;
	const char *attrname = ((s=strchr_m(name, '.')) == NULL) ? name : s + 1;

	if((retval=extattr_get_link(path, attrnamespace, attrname, NULL, 0)) >= 0) {
		if(retval > size) {
			errno = ERANGE;
			return -1;
		}
		if((retval=extattr_get_link(path, attrnamespace, attrname, value, size)) >= 0)
			return retval;
	}
	
	DEBUG(10,("sys_lgetxattr: extattr_get_link() failed with: %s\n", strerror(errno)));
	return -1;
#elif defined(HAVE_ATTR_GET)
	int retval, flags = ATTR_DONTFOLLOW;
	int valuelength = (int)size;
	char *attrname = strchr(name,'.') + 1;
	
	if (strncmp(name, "system", 6) == 0) flags |= ATTR_ROOT;

	retval = attr_get(path, attrname, (char *)value, &valuelength, flags);

	return retval ? retval : valuelength;
#else
	errno = ENOSYS;
	return -1;
#endif
}

ssize_t sys_fgetxattr (int filedes, const char *name, void *value, size_t size)
{
#if defined(HAVE_FGETXATTR)
	return fgetxattr(filedes, name, value, size);
#elif defined(HAVE_FGETEA)
	return fgetea(filedes, name, value, size);
#elif defined(HAVE_EXTATTR_GET_FD)
	char *s;
	ssize_t retval;
	int attrnamespace = (strncmp(name, "system", 6) == 0) ? 
		EXTATTR_NAMESPACE_SYSTEM : EXTATTR_NAMESPACE_USER;
	const char *attrname = ((s=strchr_m(name, '.')) == NULL) ? name : s + 1;

	if((retval=extattr_get_fd(filedes, attrnamespace, attrname, NULL, 0)) >= 0) {
		if(retval > size) {
			errno = ERANGE;
			return -1;
		}
		if((retval=extattr_get_fd(filedes, attrnamespace, attrname, value, size)) >= 0)
			return retval;
	}
	
	DEBUG(10,("sys_fgetxattr: extattr_get_fd() failed with: %s\n", strerror(errno)));
	return -1;
#elif defined(HAVE_ATTR_GETF)
	int retval, flags = 0;
	int valuelength = (int)size;
	char *attrname = strchr(name,'.') + 1;
	
	if (strncmp(name, "system", 6) == 0) flags |= ATTR_ROOT;

	retval = attr_getf(filedes, attrname, (char *)value, &valuelength, flags);

	return retval ? retval : valuelength;
#else
	errno = ENOSYS;
	return -1;
#endif
}

#if defined(HAVE_EXTATTR_LIST_FILE)

#define EXTATTR_PREFIX(s)	(s), (sizeof((s))-1)

static struct {
        int space;
	const char *name;
	size_t len;
} 
extattr[] = {
	{ EXTATTR_NAMESPACE_SYSTEM, EXTATTR_PREFIX("system.") },
        { EXTATTR_NAMESPACE_USER, EXTATTR_PREFIX("user.") },
};

typedef union {
	const char *path;
	int filedes;
} extattr_arg;

static ssize_t bsd_attr_list (int type, extattr_arg arg, char *list, size_t size)
{
	ssize_t list_size, total_size = 0;
	int i, t, len;
	char *buf;
	/* Iterate through extattr(2) namespaces */
	for(t = 0; t < (sizeof(extattr)/sizeof(extattr[0])); t++) {
		switch(type) {
#if defined(HAVE_EXTATTR_LIST_FILE)
			case 0:
				list_size = extattr_list_file(arg.path, extattr[t].space, list, size);
				break;
#endif
#if defined(HAVE_EXTATTR_LIST_LINK)
			case 1:
				list_size = extattr_list_link(arg.path, extattr[t].space, list, size);
				break;
#endif
#if defined(HAVE_EXTATTR_LIST_FD)
			case 2:
				list_size = extattr_list_fd(arg.filedes, extattr[t].space, list, size);
				break;
#endif
			default:
				errno = ENOSYS;
				return -1;
		}
		/* Some error happend. Errno should be set by the previous call */
		if(list_size < 0)
			return -1;
		/* No attributes */
		if(list_size == 0)
			continue;
		/* XXX: Call with an empty buffer may be used to calculate
		   necessary buffer size. Unfortunately, we can't say, how
		   many attributes were returned, so here is the potential
		   problem with the emulation.
		*/
		if(list == NULL) {
			/* Take the worse case of one char attribute names - 
			   two bytes per name plus one more for sanity.
			*/
			total_size += list_size + (list_size/2 + 1)*extattr[t].len;
			continue;
		}
		/* Count necessary offset to fit namespace prefixes */
		len = 0;
		for(i = 0; i < list_size; i += list[i] + 1)
			len += extattr[t].len;

		total_size += list_size + len;
		/* Buffer is too small to fit the results */
		if(total_size > size) {
			errno = ERANGE;
			return -1;
		}
		/* Shift results back, so we can prepend prefixes */
		buf = memmove(list + len, list, list_size);

		for(i = 0; i < list_size; i += len + 1) {
			len = buf[i];
			strncpy(list, extattr[t].name, extattr[t].len + 1);
			list += extattr[t].len;
			strncpy(list, buf + i + 1, len);
			list[len] = '\0';
			list += len + 1;
		}
		size -= total_size;
	}
	return total_size;
}

#endif

#if defined(HAVE_ATTR_LIST) && defined(HAVE_SYS_ATTRIBUTES_H)
static char attr_buffer[ATTR_MAX_VALUELEN];

static ssize_t irix_attr_list(const char *path, int filedes, char *list, size_t size, int flags)
{
	int retval = 0, index;
	attrlist_cursor_t *cursor = 0;
	int total_size = 0;
	attrlist_t * al = (attrlist_t *)attr_buffer;
	attrlist_ent_t *ae;
	size_t ent_size, left = size;
	char *bp = list;

	while (True) {
	    if (filedes)
		retval = attr_listf(filedes, attr_buffer, ATTR_MAX_VALUELEN, flags, cursor);
	    else
		retval = attr_list(path, attr_buffer, ATTR_MAX_VALUELEN, flags, cursor);
	    if (retval) break;
	    for (index = 0; index < al->al_count; index++) {
		ae = ATTR_ENTRY(attr_buffer, index);
		ent_size = strlen(ae->a_name) + sizeof("user.");
		if (left >= ent_size) {
		    strncpy(bp, "user.", sizeof("user."));
		    strncat(bp, ae->a_name, ent_size - sizeof("user."));
		    bp += ent_size;
		    left -= ent_size;
		} else if (size) {
		    errno = ERANGE;
		    retval = -1;
		    break;
		}
		total_size += ent_size;
	    }
	    if (al->al_more == 0) break;
	}
	if (retval == 0) {
	    flags |= ATTR_ROOT;
	    cursor = 0;
	    while (True) {
		if (filedes)
		    retval = attr_listf(filedes, attr_buffer, ATTR_MAX_VALUELEN, flags, cursor);
		else
		    retval = attr_list(path, attr_buffer, ATTR_MAX_VALUELEN, flags, cursor);
		if (retval) break;
		for (index = 0; index < al->al_count; index++) {
		    ae = ATTR_ENTRY(attr_buffer, index);
		    ent_size = strlen(ae->a_name) + sizeof("system.");
		    if (left >= ent_size) {
			strncpy(bp, "system.", sizeof("system."));
			strncat(bp, ae->a_name, ent_size - sizeof("system."));
			bp += ent_size;
			left -= ent_size;
		    } else if (size) {
			errno = ERANGE;
			retval = -1;
			break;
		    }
		    total_size += ent_size;
		}
		if (al->al_more == 0) break;
	    }
	}
	return (ssize_t)(retval ? retval : total_size);
}

#endif

ssize_t sys_listxattr (const char *path, char *list, size_t size)
{
#if defined(HAVE_LISTXATTR)
	return listxattr(path, list, size);
#elif defined(HAVE_LISTEA)
	return listea(path, list, size);
#elif defined(HAVE_EXTATTR_LIST_FILE)
	extattr_arg arg;
	arg.path = path;
	return bsd_attr_list(0, arg, list, size);
#elif defined(HAVE_ATTR_LIST) && defined(HAVE_SYS_ATTRIBUTES_H)
	return irix_attr_list(path, 0, list, size, 0);
#else
	errno = ENOSYS;
	return -1;
#endif
}

ssize_t sys_llistxattr (const char *path, char *list, size_t size)
{
#if defined(HAVE_LLISTXATTR)
	return llistxattr(path, list, size);
#elif defined(HAVE_LLISTEA)
	return llistea(path, list, size);
#elif defined(HAVE_EXTATTR_LIST_LINK)
	extattr_arg arg;
	arg.path = path;
	return bsd_attr_list(1, arg, list, size);
#elif defined(HAVE_ATTR_LIST) && defined(HAVE_SYS_ATTRIBUTES_H)
	return irix_attr_list(path, 0, list, size, ATTR_DONTFOLLOW);
#else
	errno = ENOSYS;
	return -1;
#endif
}

ssize_t sys_flistxattr (int filedes, char *list, size_t size)
{
#if defined(HAVE_FLISTXATTR)
	return flistxattr(filedes, list, size);
#elif defined(HAVE_FLISTEA)
	return flistea(filedes, list, size);
#elif defined(HAVE_EXTATTR_LIST_FD)
	extattr_arg arg;
	arg.filedes = filedes;
	return bsd_attr_list(2, arg, list, size);
#elif defined(HAVE_ATTR_LISTF)
	return irix_attr_list(NULL, filedes, list, size, 0);
#else
	errno = ENOSYS;
	return -1;
#endif
}

int sys_removexattr (const char *path, const char *name)
{
#if defined(HAVE_REMOVEXATTR)
	return removexattr(path, name);
#elif defined(HAVE_REMOVEEA)
	return removeea(path, name);
#elif defined(HAVE_EXTATTR_DELETE_FILE)
	char *s;
	int attrnamespace = (strncmp(name, "system", 6) == 0) ? 
		EXTATTR_NAMESPACE_SYSTEM : EXTATTR_NAMESPACE_USER;
	const char *attrname = ((s=strchr_m(name, '.')) == NULL) ? name : s + 1;

	return extattr_delete_file(path, attrnamespace, attrname);
#elif defined(HAVE_ATTR_REMOVE)
	int flags = 0;
	char *attrname = strchr(name,'.') + 1;
	
	if (strncmp(name, "system", 6) == 0) flags |= ATTR_ROOT;

	return attr_remove(path, attrname, flags);
#else
	errno = ENOSYS;
	return -1;
#endif
}

int sys_lremovexattr (const char *path, const char *name)
{
#if defined(HAVE_LREMOVEXATTR)
	return lremovexattr(path, name);
#elif defined(HAVE_LREMOVEEA)
	return lremoveea(path, name);
#elif defined(HAVE_EXTATTR_DELETE_LINK)
	char *s;
	int attrnamespace = (strncmp(name, "system", 6) == 0) ? 
		EXTATTR_NAMESPACE_SYSTEM : EXTATTR_NAMESPACE_USER;
	const char *attrname = ((s=strchr_m(name, '.')) == NULL) ? name : s + 1;

	return extattr_delete_link(path, attrnamespace, attrname);
#elif defined(HAVE_ATTR_REMOVE)
	int flags = ATTR_DONTFOLLOW;
	char *attrname = strchr(name,'.') + 1;
	
	if (strncmp(name, "system", 6) == 0) flags |= ATTR_ROOT;

	return attr_remove(path, attrname, flags);
#else
	errno = ENOSYS;
	return -1;
#endif
}

int sys_fremovexattr (int filedes, const char *name)
{
#if defined(HAVE_FREMOVEXATTR)
	return fremovexattr(filedes, name);
#elif defined(HAVE_FREMOVEEA)
	return fremoveea(filedes, name);
#elif defined(HAVE_EXTATTR_DELETE_FD)
	char *s;
	int attrnamespace = (strncmp(name, "system", 6) == 0) ? 
		EXTATTR_NAMESPACE_SYSTEM : EXTATTR_NAMESPACE_USER;
	const char *attrname = ((s=strchr_m(name, '.')) == NULL) ? name : s + 1;

	return extattr_delete_fd(filedes, attrnamespace, attrname);
#elif defined(HAVE_ATTR_REMOVEF)
	int flags = 0;
	char *attrname = strchr(name,'.') + 1;
	
	if (strncmp(name, "system", 6) == 0) flags |= ATTR_ROOT;

	return attr_removef(filedes, attrname, flags);
#else
	errno = ENOSYS;
	return -1;
#endif
}

#if !defined(HAVE_SETXATTR)
#define XATTR_CREATE  0x1       /* set value, fail if attr already exists */
#define XATTR_REPLACE 0x2       /* set value, fail if attr does not exist */
#endif

int sys_setxattr (const char *path, const char *name, const void *value, size_t size, int flags)
{
#if defined(HAVE_SETXATTR)
	return setxattr(path, name, value, size, flags);
#elif defined(HAVE_SETEA)
	return setea(path, name, value, size, flags);
#elif defined(HAVE_EXTATTR_SET_FILE)
	char *s;
	int retval = 0;
	int attrnamespace = (strncmp(name, "system", 6) == 0) ? 
		EXTATTR_NAMESPACE_SYSTEM : EXTATTR_NAMESPACE_USER;
	const char *attrname = ((s=strchr_m(name, '.')) == NULL) ? name : s + 1;
	if (flags) {
		/* Check attribute existence */
		retval = extattr_get_file(path, attrnamespace, attrname, NULL, 0);
		if (retval < 0) {
			/* REPLACE attribute, that doesn't exist */
			if (flags & XATTR_REPLACE && errno == ENOATTR) {
				errno = ENOATTR;
				return -1;
			}
			/* Ignore other errors */
		}
		else {
			/* CREATE attribute, that already exists */
			if (flags & XATTR_CREATE) {
				errno = EEXIST;
				return -1;
			}
		}
	}
	retval = extattr_set_file(path, attrnamespace, attrname, value, size);
	return (retval < 0) ? -1 : 0;
#elif defined(HAVE_ATTR_SET)
	int myflags = 0;
	char *attrname = strchr(name,'.') + 1;
	
	if (strncmp(name, "system", 6) == 0) myflags |= ATTR_ROOT;
	if (flags & XATTR_CREATE) myflags |= ATTR_CREATE;
	if (flags & XATTR_REPLACE) myflags |= ATTR_REPLACE;

	return attr_set(path, attrname, (const char *)value, size, myflags);
#else
	errno = ENOSYS;
	return -1;
#endif
}

int sys_lsetxattr (const char *path, const char *name, const void *value, size_t size, int flags)
{
#if defined(HAVE_LSETXATTR)
	return lsetxattr(path, name, value, size, flags);
#elif defined(LSETEA)
	return lsetea(path, name, value, size, flags);
#elif defined(HAVE_EXTATTR_SET_LINK)
	char *s;
	int retval = 0;
	int attrnamespace = (strncmp(name, "system", 6) == 0) ? 
		EXTATTR_NAMESPACE_SYSTEM : EXTATTR_NAMESPACE_USER;
	const char *attrname = ((s=strchr_m(name, '.')) == NULL) ? name : s + 1;
	if (flags) {
		/* Check attribute existence */
		retval = extattr_get_link(path, attrnamespace, attrname, NULL, 0);
		if (retval < 0) {
			/* REPLACE attribute, that doesn't exist */
			if (flags & XATTR_REPLACE && errno == ENOATTR) {
				errno = ENOATTR;
				return -1;
			}
			/* Ignore other errors */
		}
		else {
			/* CREATE attribute, that already exists */
			if (flags & XATTR_CREATE) {
				errno = EEXIST;
				return -1;
			}
		}
	}

	retval = extattr_set_link(path, attrnamespace, attrname, value, size);
	return (retval < 0) ? -1 : 0;
#elif defined(HAVE_ATTR_SET)
	int myflags = ATTR_DONTFOLLOW;
	char *attrname = strchr(name,'.') + 1;
	
	if (strncmp(name, "system", 6) == 0) myflags |= ATTR_ROOT;
	if (flags & XATTR_CREATE) myflags |= ATTR_CREATE;
	if (flags & XATTR_REPLACE) myflags |= ATTR_REPLACE;

	return attr_set(path, attrname, (const char *)value, size, myflags);
#else
	errno = ENOSYS;
	return -1;
#endif
}

int sys_fsetxattr (int filedes, const char *name, const void *value, size_t size, int flags)
{
#if defined(HAVE_FSETXATTR)
	return fsetxattr(filedes, name, value, size, flags);
#elif defined(HAVE_FSETEA)
	return fsetea(filedes, name, value, size, flags);
#elif defined(HAVE_EXTATTR_SET_FD)
	char *s;
	int retval = 0;
	int attrnamespace = (strncmp(name, "system", 6) == 0) ? 
		EXTATTR_NAMESPACE_SYSTEM : EXTATTR_NAMESPACE_USER;
	const char *attrname = ((s=strchr_m(name, '.')) == NULL) ? name : s + 1;
	if (flags) {
		/* Check attribute existence */
		retval = extattr_get_fd(filedes, attrnamespace, attrname, NULL, 0);
		if (retval < 0) {
			/* REPLACE attribute, that doesn't exist */
			if (flags & XATTR_REPLACE && errno == ENOATTR) {
				errno = ENOATTR;
				return -1;
			}
			/* Ignore other errors */
		}
		else {
			/* CREATE attribute, that already exists */
			if (flags & XATTR_CREATE) {
				errno = EEXIST;
				return -1;
			}
		}
	}
	retval = extattr_set_fd(filedes, attrnamespace, attrname, value, size);
	return (retval < 0) ? -1 : 0;
#elif defined(HAVE_ATTR_SETF)
	int myflags = 0;
	char *attrname = strchr(name,'.') + 1;
	
	if (strncmp(name, "system", 6) == 0) myflags |= ATTR_ROOT;
	if (flags & XATTR_CREATE) myflags |= ATTR_CREATE;
	if (flags & XATTR_REPLACE) myflags |= ATTR_REPLACE;

	return attr_setf(filedes, attrname, (const char *)value, size, myflags);
#else
	errno = ENOSYS;
	return -1;
#endif
}

/****************************************************************************
 Return the major devicenumber for UNIX extensions.
****************************************************************************/
                                                                                                                
uint32 unix_dev_major(SMB_DEV_T dev)
{
#if defined(HAVE_DEVICE_MAJOR_FN)
        return (uint32)major(dev);
#else
        return (uint32)(dev >> 8);
#endif
}
                                                                                                                
/****************************************************************************
 Return the minor devicenumber for UNIX extensions.
****************************************************************************/
                                                                                                                
uint32 unix_dev_minor(SMB_DEV_T dev)
{
#if defined(HAVE_DEVICE_MINOR_FN)
        return (uint32)minor(dev);
#else
        return (uint32)(dev & 0xff);
#endif
}

#if defined(WITH_AIO)

/*******************************************************************
 An aio_read wrapper that will deal with 64-bit sizes.
********************************************************************/
                                                                                                                                           
int sys_aio_read(SMB_STRUCT_AIOCB *aiocb)
{
#if defined(HAVE_EXPLICIT_LARGEFILE_SUPPORT) && defined(HAVE_AIOCB64) && defined(HAVE_AIO_READ64)
        return aio_read64(aiocb);
#elif defined(HAVE_AIO_READ)
        return aio_read(aiocb);
#else
	errno = ENOSYS;
	return -1;
#endif
}

/*******************************************************************
 An aio_write wrapper that will deal with 64-bit sizes.
********************************************************************/
                                                                                                                                           
int sys_aio_write(SMB_STRUCT_AIOCB *aiocb)
{
#if defined(HAVE_EXPLICIT_LARGEFILE_SUPPORT) && defined(HAVE_AIOCB64) && defined(HAVE_AIO_WRITE64)
        return aio_write64(aiocb);
#elif defined(HAVE_AIO_WRITE)
        return aio_write(aiocb);
#else
	errno = ENOSYS;
	return -1;
#endif
}

/*******************************************************************
 An aio_return wrapper that will deal with 64-bit sizes.
********************************************************************/
                                                                                                                                           
ssize_t sys_aio_return(SMB_STRUCT_AIOCB *aiocb)
{
#if defined(HAVE_EXPLICIT_LARGEFILE_SUPPORT) && defined(HAVE_AIOCB64) && defined(HAVE_AIO_RETURN64)
        return aio_return64(aiocb);
#elif defined(HAVE_AIO_RETURN)
        return aio_return(aiocb);
#else
	errno = ENOSYS;
	return -1;
#endif
}

/*******************************************************************
 An aio_cancel wrapper that will deal with 64-bit sizes.
********************************************************************/

int sys_aio_cancel(int fd, SMB_STRUCT_AIOCB *aiocb)
{
#if defined(HAVE_EXPLICIT_LARGEFILE_SUPPORT) && defined(HAVE_AIOCB64) && defined(HAVE_AIO_CANCEL64)
        return aio_cancel64(fd, aiocb);
#elif defined(HAVE_AIO_CANCEL)
        return aio_cancel(fd, aiocb);
#else
	errno = ENOSYS;
	return -1;
#endif
}

/*******************************************************************
 An aio_error wrapper that will deal with 64-bit sizes.
********************************************************************/

int sys_aio_error(const SMB_STRUCT_AIOCB *aiocb)
{
#if defined(HAVE_EXPLICIT_LARGEFILE_SUPPORT) && defined(HAVE_AIOCB64) && defined(HAVE_AIO_ERROR64)
        return aio_error64(aiocb);
#elif defined(HAVE_AIO_ERROR)
        return aio_error(aiocb);
#else
	errno = ENOSYS;
	return -1;
#endif
}

/*******************************************************************
 An aio_fsync wrapper that will deal with 64-bit sizes.
********************************************************************/

int sys_aio_fsync(int op, SMB_STRUCT_AIOCB *aiocb)
{
#if defined(HAVE_EXPLICIT_LARGEFILE_SUPPORT) && defined(HAVE_AIOCB64) && defined(HAVE_AIO_FSYNC64)
        return aio_fsync64(op, aiocb);
#elif defined(HAVE_AIO_FSYNC)
        return aio_fsync(op, aiocb);
#else
	errno = ENOSYS;
	return -1;
#endif
}

/*******************************************************************
 An aio_fsync wrapper that will deal with 64-bit sizes.
********************************************************************/

int sys_aio_suspend(const SMB_STRUCT_AIOCB * const cblist[], int n, const struct timespec *timeout)
{
#if defined(HAVE_EXPLICIT_LARGEFILE_SUPPORT) && defined(HAVE_AIOCB64) && defined(HAVE_AIO_SUSPEND64)
        return aio_suspend64(cblist, n, timeout);
#elif defined(HAVE_AIO_FSYNC)
        return aio_suspend(cblist, n, timeout);
#else
	errno = ENOSYS;
	return -1;
#endif
}
#else /* !WITH_AIO */

int sys_aio_read(SMB_STRUCT_AIOCB *aiocb)
{
	errno = ENOSYS;
	return -1;
}

int sys_aio_write(SMB_STRUCT_AIOCB *aiocb)
{
	errno = ENOSYS;
	return -1;
}

ssize_t sys_aio_return(SMB_STRUCT_AIOCB *aiocb)
{
	errno = ENOSYS;
	return -1;
}

int sys_aio_cancel(int fd, SMB_STRUCT_AIOCB *aiocb)
{
	errno = ENOSYS;
	return -1;
}

int sys_aio_error(const SMB_STRUCT_AIOCB *aiocb)
{
	errno = ENOSYS;
	return -1;
}

int sys_aio_fsync(int op, SMB_STRUCT_AIOCB *aiocb)
{
	errno = ENOSYS;
	return -1;
}

int sys_aio_suspend(const SMB_STRUCT_AIOCB * const cblist[], int n, const struct timespec *timeout)
{
	errno = ENOSYS;
	return -1;
}
#endif /* WITH_AIO */
