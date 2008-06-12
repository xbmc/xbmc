/* 
   Unix SMB/CIFS implementation.
   Wrap disk only vfs functions to sidestep dodgy compilers.
   Copyright (C) Tim Potter 1998
   
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

#undef DBGC_CLASS
#define DBGC_CLASS DBGC_VFS


/* Check for NULL pointer parameters in vfswrap_* functions */

/* We don't want to have NULL function pointers lying around.  Someone
   is sure to try and execute them.  These stubs are used to prevent
   this possibility. */

int vfswrap_dummy_connect(vfs_handle_struct *handle, connection_struct *conn, const char *service, const char *user)
{
    return 0;    /* Return >= 0 for success */
}

void vfswrap_dummy_disconnect(vfs_handle_struct *handle, connection_struct *conn)
{
}

/* Disk operations */

SMB_BIG_UINT vfswrap_disk_free(vfs_handle_struct *handle, connection_struct *conn, const char *path, BOOL small_query, SMB_BIG_UINT *bsize, 
			       SMB_BIG_UINT *dfree, SMB_BIG_UINT *dsize)
{
	SMB_BIG_UINT result;

	result = sys_disk_free(conn, path, small_query, bsize, dfree, dsize);
	return result;
}

int vfswrap_get_quota(struct vfs_handle_struct *handle, struct connection_struct *conn, enum SMB_QUOTA_TYPE qtype, unid_t id, SMB_DISK_QUOTA *qt)
{
#ifdef HAVE_SYS_QUOTAS
	int result;

	START_PROFILE(syscall_get_quota);
	result = sys_get_quota(conn->connectpath, qtype, id, qt);
	END_PROFILE(syscall_get_quota);
	return result;
#else
	errno = ENOSYS;
	return -1;
#endif	
}

int vfswrap_set_quota(struct vfs_handle_struct *handle, struct connection_struct *conn, enum SMB_QUOTA_TYPE qtype, unid_t id, SMB_DISK_QUOTA *qt)
{
#ifdef HAVE_SYS_QUOTAS
	int result;

	START_PROFILE(syscall_set_quota);
	result = sys_set_quota(conn->connectpath, qtype, id, qt);
	END_PROFILE(syscall_set_quota);
	return result;
#else
	errno = ENOSYS;
	return -1;
#endif	
}

int vfswrap_get_shadow_copy_data(struct vfs_handle_struct *handle, struct files_struct *fsp, SHADOW_COPY_DATA *shadow_copy_data, BOOL labels)
{
	errno = ENOSYS;
	return -1;  /* Not implemented. */
}
    
int vfswrap_statvfs(struct vfs_handle_struct *handle, struct connection_struct *conn, const char *path, vfs_statvfs_struct *statbuf)
{
	return sys_statvfs(path, statbuf);
}

/* Directory operations */

SMB_STRUCT_DIR *vfswrap_opendir(vfs_handle_struct *handle, connection_struct *conn, const char *fname, const char *mask, uint32 attr)
{
	SMB_STRUCT_DIR *result;

	START_PROFILE(syscall_opendir);
	result = sys_opendir(fname);
	END_PROFILE(syscall_opendir);
	return result;
}

SMB_STRUCT_DIRENT *vfswrap_readdir(vfs_handle_struct *handle, connection_struct *conn, SMB_STRUCT_DIR *dirp)
{
	SMB_STRUCT_DIRENT *result;

	START_PROFILE(syscall_readdir);
	result = sys_readdir(dirp);
	END_PROFILE(syscall_readdir);
	return result;
}

void vfswrap_seekdir(vfs_handle_struct *handle, connection_struct *conn, SMB_STRUCT_DIR *dirp, long offset)
{
	START_PROFILE(syscall_seekdir);
	sys_seekdir(dirp, offset);
	END_PROFILE(syscall_seekdir);
}

long vfswrap_telldir(vfs_handle_struct *handle, connection_struct *conn, SMB_STRUCT_DIR *dirp)
{
	long result;
	START_PROFILE(syscall_telldir);
	result = sys_telldir(dirp);
	END_PROFILE(syscall_telldir);
	return result;
}

void vfswrap_rewinddir(vfs_handle_struct *handle, connection_struct *conn, SMB_STRUCT_DIR *dirp)
{
	START_PROFILE(syscall_rewinddir);
	sys_rewinddir(dirp);
	END_PROFILE(syscall_rewinddir);
}

int vfswrap_mkdir(vfs_handle_struct *handle, connection_struct *conn, const char *path, mode_t mode)
{
	int result;
	BOOL has_dacl = False;

	START_PROFILE(syscall_mkdir);

	if (lp_inherit_acls(SNUM(conn)) && (has_dacl = directory_has_default_acl(conn, parent_dirname(path))))
		mode = 0777;

	result = mkdir(path, mode);

	if (result == 0 && !has_dacl) {
		/*
		 * We need to do this as the default behavior of POSIX ACLs	
		 * is to set the mask to be the requested group permission
		 * bits, not the group permission bits to be the requested
		 * group permission bits. This is not what we want, as it will
		 * mess up any inherited ACL bits that were set. JRA.
		 */
		int saved_errno = errno; /* We may get ENOSYS */
		if ((SMB_VFS_CHMOD_ACL(conn, path, mode) == -1) && (errno == ENOSYS))
			errno = saved_errno;
	}

	END_PROFILE(syscall_mkdir);
	return result;
}

int vfswrap_rmdir(vfs_handle_struct *handle, connection_struct *conn, const char *path)
{
	int result;

	START_PROFILE(syscall_rmdir);
	result = rmdir(path);
	END_PROFILE(syscall_rmdir);
	return result;
}

int vfswrap_closedir(vfs_handle_struct *handle, connection_struct *conn, SMB_STRUCT_DIR *dirp)
{
	int result;

	START_PROFILE(syscall_closedir);
	result = sys_closedir(dirp);
	END_PROFILE(syscall_closedir);
	return result;
}

/* File operations */
    
int vfswrap_open(vfs_handle_struct *handle, connection_struct *conn, const char *fname, int flags, mode_t mode)
{
	int result;

	START_PROFILE(syscall_open);
	result = sys_open(fname, flags, mode);
	END_PROFILE(syscall_open);
	return result;
}

int vfswrap_close(vfs_handle_struct *handle, files_struct *fsp, int fd)
{
	int result;

	START_PROFILE(syscall_close);

	result = close(fd);
	END_PROFILE(syscall_close);
	return result;
}

ssize_t vfswrap_read(vfs_handle_struct *handle, files_struct *fsp, int fd, void *data, size_t n)
{
	ssize_t result;

	START_PROFILE_BYTES(syscall_read, n);
	result = sys_read(fd, data, n);
	END_PROFILE(syscall_read);
	return result;
}

ssize_t vfswrap_pread(vfs_handle_struct *handle, files_struct *fsp, int fd, void *data,
			size_t n, SMB_OFF_T offset)
{
	ssize_t result;

#if defined(HAVE_PREAD) || defined(HAVE_PREAD64)
	START_PROFILE_BYTES(syscall_pread, n);
	result = sys_pread(fd, data, n, offset);
	END_PROFILE(syscall_pread);
 
	if (result == -1 && errno == ESPIPE) {
		/* Maintain the fiction that pipes can be seeked (sought?) on. */
		result = SMB_VFS_READ(fsp, fd, data, n);
		fsp->fh->pos = 0;
	}

#else /* HAVE_PREAD */
	SMB_OFF_T   curr;
	int lerrno;
   
	curr = SMB_VFS_LSEEK(fsp, fd, 0, SEEK_CUR);
	if (curr == -1 && errno == ESPIPE) {
		/* Maintain the fiction that pipes can be seeked (sought?) on. */
		result = SMB_VFS_READ(fsp, fd, data, n);
		fsp->fh->pos = 0;
		return result;
	}

	if (SMB_VFS_LSEEK(fsp, fd, offset, SEEK_SET) == -1) {
		return -1;
	}

	errno = 0;
	result = SMB_VFS_READ(fsp, fd, data, n);
	lerrno = errno;

	SMB_VFS_LSEEK(fsp, fd, curr, SEEK_SET);
	errno = lerrno;

#endif /* HAVE_PREAD */

	return result;
}

ssize_t vfswrap_write(vfs_handle_struct *handle, files_struct *fsp, int fd, const void *data, size_t n)
{
	ssize_t result;

	START_PROFILE_BYTES(syscall_write, n);
	result = sys_write(fd, data, n);
	END_PROFILE(syscall_write);
	return result;
}

ssize_t vfswrap_pwrite(vfs_handle_struct *handle, files_struct *fsp, int fd, const void *data,
			size_t n, SMB_OFF_T offset)
{
	ssize_t result;

#if defined(HAVE_PWRITE) || defined(HAVE_PRWITE64)
	START_PROFILE_BYTES(syscall_pwrite, n);
	result = sys_pwrite(fd, data, n, offset);
	END_PROFILE(syscall_pwrite);

	if (result == -1 && errno == ESPIPE) {
		/* Maintain the fiction that pipes can be sought on. */
		result = SMB_VFS_WRITE(fsp, fd, data, n);
	}

#else /* HAVE_PWRITE */
	SMB_OFF_T   curr;
	int         lerrno;

	curr = SMB_VFS_LSEEK(fsp, fd, 0, SEEK_CUR);
	if (curr == -1) {
		return -1;
	}

	if (SMB_VFS_LSEEK(fsp, fd, offset, SEEK_SET) == -1) {
		return -1;
	}

	result = SMB_VFS_WRITE(fsp, fd, data, n);
	lerrno = errno;

	SMB_VFS_LSEEK(fsp, fd, curr, SEEK_SET);
	errno = lerrno;

#endif /* HAVE_PWRITE */

	return result;
}

SMB_OFF_T vfswrap_lseek(vfs_handle_struct *handle, files_struct *fsp, int filedes, SMB_OFF_T offset, int whence)
{
	SMB_OFF_T result = 0;

	START_PROFILE(syscall_lseek);

	/* Cope with 'stat' file opens. */
	if (filedes != -1)
		result = sys_lseek(filedes, offset, whence);

	/*
	 * We want to maintain the fiction that we can seek
	 * on a fifo for file system purposes. This allows
	 * people to set up UNIX fifo's that feed data to Windows
	 * applications. JRA.
	 */

	if((result == -1) && (errno == ESPIPE)) {
		result = 0;
		errno = 0;
	}

	END_PROFILE(syscall_lseek);
	return result;
}

ssize_t vfswrap_sendfile(vfs_handle_struct *handle, int tofd, files_struct *fsp, int fromfd, const DATA_BLOB *hdr,
			SMB_OFF_T offset, size_t n)
{
	ssize_t result;

	START_PROFILE_BYTES(syscall_sendfile, n);
	result = sys_sendfile(tofd, fromfd, hdr, offset, n);
	END_PROFILE(syscall_sendfile);
	return result;
}

/*********************************************************
 For rename across filesystems Patch from Warren Birnbaum
 <warrenb@hpcvscdp.cv.hp.com>
**********************************************************/

static int copy_reg(const char *source, const char *dest)
{
	SMB_STRUCT_STAT source_stats;
	int saved_errno;
	int ifd = -1;
	int ofd = -1;

	if (sys_lstat (source, &source_stats) == -1)
		return -1;

	if (!S_ISREG (source_stats.st_mode))
		return -1;

	if((ifd = sys_open (source, O_RDONLY, 0)) < 0)
		return -1;

	if (unlink (dest) && errno != ENOENT)
		return -1;

#ifdef O_NOFOLLOW
	if((ofd = sys_open (dest, O_WRONLY | O_CREAT | O_TRUNC | O_NOFOLLOW, 0600)) < 0 )
#else
	if((ofd = sys_open (dest, O_WRONLY | O_CREAT | O_TRUNC , 0600)) < 0 )
#endif
		goto err;

	if (transfer_file(ifd, ofd, (size_t)-1) == -1)
		goto err;

	/*
	 * Try to preserve ownership.  For non-root it might fail, but that's ok.
	 * But root probably wants to know, e.g. if NFS disallows it.
	 */

#ifdef HAVE_FCHOWN
	if ((fchown(ofd, source_stats.st_uid, source_stats.st_gid) == -1) && (errno != EPERM))
#else
	if ((chown(dest, source_stats.st_uid, source_stats.st_gid) == -1) && (errno != EPERM))
#endif
		goto err;

	/*
	 * fchown turns off set[ug]id bits for non-root,
	 * so do the chmod last.
	 */

#if defined(HAVE_FCHMOD)
	if (fchmod (ofd, source_stats.st_mode & 07777))
#else
	if (chmod (dest, source_stats.st_mode & 07777))
#endif
		goto err;

	if (close (ifd) == -1)
		goto err;

	if (close (ofd) == -1)
		return -1;

	/* Try to copy the old file's modtime and access time.  */
	{
		struct utimbuf tv;

		tv.actime = source_stats.st_atime;
		tv.modtime = source_stats.st_mtime;
		utime(dest, &tv);
	}

	if (unlink (source) == -1)
		return -1;

	return 0;

  err:

	saved_errno = errno;
	if (ifd != -1)
		close(ifd);
	if (ofd != -1)
		close(ofd);
	errno = saved_errno;
	return -1;
}

int vfswrap_rename(vfs_handle_struct *handle, connection_struct *conn, const char *oldname, const char *newname)
{
	int result;

	START_PROFILE(syscall_rename);
	result = rename(oldname, newname);
	if (errno == EXDEV) {
		/* Rename across filesystems needed. */
		result = copy_reg(oldname, newname);
	}

	END_PROFILE(syscall_rename);
	return result;
}

int vfswrap_fsync(vfs_handle_struct *handle, files_struct *fsp, int fd)
{
#ifdef HAVE_FSYNC
	int result;

	START_PROFILE(syscall_fsync);
	result = fsync(fd);
	END_PROFILE(syscall_fsync);
	return result;
#else
	return 0;
#endif
}

int vfswrap_stat(vfs_handle_struct *handle, connection_struct *conn, const char *fname, SMB_STRUCT_STAT *sbuf)
{
	int result;

	START_PROFILE(syscall_stat);
	result = sys_stat(fname, sbuf);
	END_PROFILE(syscall_stat);
	return result;
}

int vfswrap_fstat(vfs_handle_struct *handle, files_struct *fsp, int fd, SMB_STRUCT_STAT *sbuf)
{
	int result;

	START_PROFILE(syscall_fstat);
	result = sys_fstat(fd, sbuf);
	END_PROFILE(syscall_fstat);
	return result;
}

int vfswrap_lstat(vfs_handle_struct *handle, connection_struct *conn, const char *path, SMB_STRUCT_STAT *sbuf)
{
	int result;

	START_PROFILE(syscall_lstat);
	result = sys_lstat(path, sbuf);
	END_PROFILE(syscall_lstat);
	return result;
}

int vfswrap_unlink(vfs_handle_struct *handle, connection_struct *conn, const char *path)
{
	int result;

	START_PROFILE(syscall_unlink);
	result = unlink(path);
	END_PROFILE(syscall_unlink);
	return result;
}

int vfswrap_chmod(vfs_handle_struct *handle, connection_struct *conn, const char *path, mode_t mode)
{
	int result;

	START_PROFILE(syscall_chmod);

	/*
	 * We need to do this due to the fact that the default POSIX ACL
	 * chmod modifies the ACL *mask* for the group owner, not the
	 * group owner bits directly. JRA.
	 */

	
	{
		int saved_errno = errno; /* We might get ENOSYS */
		if ((result = SMB_VFS_CHMOD_ACL(conn, path, mode)) == 0) {
			END_PROFILE(syscall_chmod);
			return result;
		}
		/* Error - return the old errno. */
		errno = saved_errno;
	}

	result = chmod(path, mode);
	END_PROFILE(syscall_chmod);
	return result;
}

int vfswrap_fchmod(vfs_handle_struct *handle, files_struct *fsp, int fd, mode_t mode)
{
	int result;
	
	START_PROFILE(syscall_fchmod);

	/*
	 * We need to do this due to the fact that the default POSIX ACL
	 * chmod modifies the ACL *mask* for the group owner, not the
	 * group owner bits directly. JRA.
	 */
	
	{
		int saved_errno = errno; /* We might get ENOSYS */
		if ((result = SMB_VFS_FCHMOD_ACL(fsp, fd, mode)) == 0) {
			END_PROFILE(syscall_fchmod);
			return result;
		}
		/* Error - return the old errno. */
		errno = saved_errno;
	}

#if defined(HAVE_FCHMOD)
	result = fchmod(fd, mode);
#else
	result = -1;
	errno = ENOSYS;
#endif

	END_PROFILE(syscall_fchmod);
	return result;
}

int vfswrap_chown(vfs_handle_struct *handle, connection_struct *conn, const char *path, uid_t uid, gid_t gid)
{
	int result;

	START_PROFILE(syscall_chown);
	result = sys_chown(path, uid, gid);
	END_PROFILE(syscall_chown);
	return result;
}

int vfswrap_fchown(vfs_handle_struct *handle, files_struct *fsp, int fd, uid_t uid, gid_t gid)
{
#ifdef HAVE_FCHOWN
	int result;

	START_PROFILE(syscall_fchown);
	result = fchown(fd, uid, gid);
	END_PROFILE(syscall_fchown);
	return result;
#else
	errno = ENOSYS;
	return -1;
#endif
}

int vfswrap_chdir(vfs_handle_struct *handle, connection_struct *conn, const char *path)
{
	int result;

	START_PROFILE(syscall_chdir);
	result = chdir(path);
	END_PROFILE(syscall_chdir);
	return result;
}

char *vfswrap_getwd(vfs_handle_struct *handle, connection_struct *conn, char *path)
{
	char *result;

	START_PROFILE(syscall_getwd);
	result = sys_getwd(path);
	END_PROFILE(syscall_getwd);
	return result;
}

int vfswrap_utime(vfs_handle_struct *handle, connection_struct *conn, const char *path, struct utimbuf *times)
{
	int result;

	START_PROFILE(syscall_utime);
	result = utime(path, times);
	END_PROFILE(syscall_utime);
	return result;
}

/*********************************************************************
 A version of ftruncate that will write the space on disk if strict
 allocate is set.
**********************************************************************/

static int strict_allocate_ftruncate(vfs_handle_struct *handle, files_struct *fsp, int fd, SMB_OFF_T len)
{
	SMB_STRUCT_STAT st;
	SMB_OFF_T currpos = SMB_VFS_LSEEK(fsp, fd, 0, SEEK_CUR);
	unsigned char zero_space[4096];
	SMB_OFF_T space_to_write;

	if (currpos == -1)
		return -1;

	if (SMB_VFS_FSTAT(fsp, fd, &st) == -1)
		return -1;

	space_to_write = len - st.st_size;

#ifdef S_ISFIFO
	if (S_ISFIFO(st.st_mode))
		return 0;
#endif

	if (st.st_size == len)
		return 0;

	/* Shrink - just ftruncate. */
	if (st.st_size > len)
		return sys_ftruncate(fd, len);

	/* Write out the real space on disk. */
	if (SMB_VFS_LSEEK(fsp, fd, st.st_size, SEEK_SET) != st.st_size)
		return -1;

	space_to_write = len - st.st_size;

	memset(zero_space, '\0', sizeof(zero_space));
	while ( space_to_write > 0) {
		SMB_OFF_T retlen;
		SMB_OFF_T current_len_to_write = MIN(sizeof(zero_space),space_to_write);

		retlen = SMB_VFS_WRITE(fsp,fsp->fh->fd,(char *)zero_space,current_len_to_write);
		if (retlen <= 0)
			return -1;

		space_to_write -= retlen;
	}

	/* Seek to where we were */
	if (SMB_VFS_LSEEK(fsp, fd, currpos, SEEK_SET) != currpos)
		return -1;

	return 0;
}

int vfswrap_ftruncate(vfs_handle_struct *handle, files_struct *fsp, int fd, SMB_OFF_T len)
{
	int result = -1;
	SMB_STRUCT_STAT st;
	char c = 0;
	SMB_OFF_T currpos;

	START_PROFILE(syscall_ftruncate);

	if (lp_strict_allocate(SNUM(fsp->conn))) {
		result = strict_allocate_ftruncate(handle, fsp, fd, len);
		END_PROFILE(syscall_ftruncate);
		return result;
	}

	/* we used to just check HAVE_FTRUNCATE_EXTEND and only use
	   sys_ftruncate if the system supports it. Then I discovered that
	   you can have some filesystems that support ftruncate
	   expansion and some that don't! On Linux fat can't do
	   ftruncate extend but ext2 can. */

	result = sys_ftruncate(fd, len);
	if (result == 0)
		goto done;

	/* According to W. R. Stevens advanced UNIX prog. Pure 4.3 BSD cannot
	   extend a file with ftruncate. Provide alternate implementation
	   for this */
	currpos = SMB_VFS_LSEEK(fsp, fd, 0, SEEK_CUR);
	if (currpos == -1) {
		goto done;
	}

	/* Do an fstat to see if the file is longer than the requested
	   size in which case the ftruncate above should have
	   succeeded or shorter, in which case seek to len - 1 and
	   write 1 byte of zero */
	if (SMB_VFS_FSTAT(fsp, fd, &st) == -1) {
		goto done;
	}

#ifdef S_ISFIFO
	if (S_ISFIFO(st.st_mode)) {
		result = 0;
		goto done;
	}
#endif

	if (st.st_size == len) {
		result = 0;
		goto done;
	}

	if (st.st_size > len) {
		/* the sys_ftruncate should have worked */
		goto done;
	}

	if (SMB_VFS_LSEEK(fsp, fd, len-1, SEEK_SET) != len -1)
		goto done;

	if (SMB_VFS_WRITE(fsp, fd, &c, 1)!=1)
		goto done;

	/* Seek to where we were */
	if (SMB_VFS_LSEEK(fsp, fd, currpos, SEEK_SET) != currpos)
		goto done;
	result = 0;

  done:

	END_PROFILE(syscall_ftruncate);
	return result;
}

BOOL vfswrap_lock(vfs_handle_struct *handle, files_struct *fsp, int fd, int op, SMB_OFF_T offset, SMB_OFF_T count, int type)
{
	BOOL result;

	START_PROFILE(syscall_fcntl_lock);
	result =  fcntl_lock(fd, op, offset, count, type);
	END_PROFILE(syscall_fcntl_lock);
	return result;
}

BOOL vfswrap_getlock(vfs_handle_struct *handle, files_struct *fsp, int fd, SMB_OFF_T *poffset, SMB_OFF_T *pcount, int *ptype, pid_t *ppid)
{
	BOOL result;

	START_PROFILE(syscall_fcntl_getlock);
	result =  fcntl_getlock(fd, poffset, pcount, ptype, ppid);
	END_PROFILE(syscall_fcntl_getlock);
	return result;
}

int vfswrap_symlink(vfs_handle_struct *handle, connection_struct *conn, const char *oldpath, const char *newpath)
{
	int result;

	START_PROFILE(syscall_symlink);
	result = sys_symlink(oldpath, newpath);
	END_PROFILE(syscall_symlink);
	return result;
}

int vfswrap_readlink(vfs_handle_struct *handle, connection_struct *conn, const char *path, char *buf, size_t bufsiz)
{
	int result;

	START_PROFILE(syscall_readlink);
	result = sys_readlink(path, buf, bufsiz);
	END_PROFILE(syscall_readlink);
	return result;
}

int vfswrap_link(vfs_handle_struct *handle, connection_struct *conn, const char *oldpath, const char *newpath)
{
	int result;

	START_PROFILE(syscall_link);
	result = sys_link(oldpath, newpath);
	END_PROFILE(syscall_link);
	return result;
}

int vfswrap_mknod(vfs_handle_struct *handle, connection_struct *conn, const char *pathname, mode_t mode, SMB_DEV_T dev)
{
	int result;

	START_PROFILE(syscall_mknod);
	result = sys_mknod(pathname, mode, dev);
	END_PROFILE(syscall_mknod);
	return result;
}

char *vfswrap_realpath(vfs_handle_struct *handle, connection_struct *conn, const char *path, char *resolved_path)
{
	char *result;

	START_PROFILE(syscall_realpath);
	result = sys_realpath(path, resolved_path);
	END_PROFILE(syscall_realpath);
	return result;
}

size_t vfswrap_fget_nt_acl(vfs_handle_struct *handle, files_struct *fsp, int fd, uint32 security_info, SEC_DESC **ppdesc)
{
	size_t result;

	START_PROFILE(fget_nt_acl);
	result = get_nt_acl(fsp, security_info, ppdesc);
	END_PROFILE(fget_nt_acl);
	return result;
}

size_t vfswrap_get_nt_acl(vfs_handle_struct *handle, files_struct *fsp, const char *name, uint32 security_info, SEC_DESC **ppdesc)
{
	size_t result;

	START_PROFILE(get_nt_acl);
	result = get_nt_acl(fsp, security_info, ppdesc);
	END_PROFILE(get_nt_acl);
	return result;
}

BOOL vfswrap_fset_nt_acl(vfs_handle_struct *handle, files_struct *fsp, int fd, uint32 security_info_sent, SEC_DESC *psd)
{
	BOOL result;

	START_PROFILE(fset_nt_acl);
	result = set_nt_acl(fsp, security_info_sent, psd);
	END_PROFILE(fset_nt_acl);
	return result;
}

BOOL vfswrap_set_nt_acl(vfs_handle_struct *handle, files_struct *fsp, const char *name, uint32 security_info_sent, SEC_DESC *psd)
{
	BOOL result;

	START_PROFILE(set_nt_acl);
	result = set_nt_acl(fsp, security_info_sent, psd);
	END_PROFILE(set_nt_acl);
	return result;
}

int vfswrap_chmod_acl(vfs_handle_struct *handle, connection_struct *conn, const char *name, mode_t mode)
{
#ifdef HAVE_NO_ACL
	errno = ENOSYS;
	return -1;
#else
	int result;

	START_PROFILE(chmod_acl);
	result = chmod_acl(conn, name, mode);
	END_PROFILE(chmod_acl);
	return result;
#endif
}

int vfswrap_fchmod_acl(vfs_handle_struct *handle, files_struct *fsp, int fd, mode_t mode)
{
#ifdef HAVE_NO_ACL
	errno = ENOSYS;
	return -1;
#else
	int result;

	START_PROFILE(fchmod_acl);
	result = fchmod_acl(fsp, fd, mode);
	END_PROFILE(fchmod_acl);
	return result;
#endif
}

int vfswrap_sys_acl_get_entry(vfs_handle_struct *handle, connection_struct *conn, SMB_ACL_T theacl, int entry_id, SMB_ACL_ENTRY_T *entry_p)
{
	return sys_acl_get_entry(theacl, entry_id, entry_p);
}

int vfswrap_sys_acl_get_tag_type(vfs_handle_struct *handle, connection_struct *conn, SMB_ACL_ENTRY_T entry_d, SMB_ACL_TAG_T *tag_type_p)
{
	return sys_acl_get_tag_type(entry_d, tag_type_p);
}

int vfswrap_sys_acl_get_permset(vfs_handle_struct *handle, connection_struct *conn, SMB_ACL_ENTRY_T entry_d, SMB_ACL_PERMSET_T *permset_p)
{
	return sys_acl_get_permset(entry_d, permset_p);
}

void * vfswrap_sys_acl_get_qualifier(vfs_handle_struct *handle, connection_struct *conn, SMB_ACL_ENTRY_T entry_d)
{
	return sys_acl_get_qualifier(entry_d);
}

SMB_ACL_T vfswrap_sys_acl_get_file(vfs_handle_struct *handle, connection_struct *conn, const char *path_p, SMB_ACL_TYPE_T type)
{
	return sys_acl_get_file(path_p, type);
}

SMB_ACL_T vfswrap_sys_acl_get_fd(vfs_handle_struct *handle, files_struct *fsp, int fd)
{
	return sys_acl_get_fd(fd);
}

int vfswrap_sys_acl_clear_perms(vfs_handle_struct *handle, connection_struct *conn, SMB_ACL_PERMSET_T permset)
{
	return sys_acl_clear_perms(permset);
}

int vfswrap_sys_acl_add_perm(vfs_handle_struct *handle, connection_struct *conn, SMB_ACL_PERMSET_T permset, SMB_ACL_PERM_T perm)
{
	return sys_acl_add_perm(permset, perm);
}

char * vfswrap_sys_acl_to_text(vfs_handle_struct *handle, connection_struct *conn, SMB_ACL_T theacl, ssize_t *plen)
{
	return sys_acl_to_text(theacl, plen);
}

SMB_ACL_T vfswrap_sys_acl_init(vfs_handle_struct *handle, connection_struct *conn, int count)
{
	return sys_acl_init(count);
}

int vfswrap_sys_acl_create_entry(vfs_handle_struct *handle, connection_struct *conn, SMB_ACL_T *pacl, SMB_ACL_ENTRY_T *pentry)
{
	return sys_acl_create_entry(pacl, pentry);
}

int vfswrap_sys_acl_set_tag_type(vfs_handle_struct *handle, connection_struct *conn, SMB_ACL_ENTRY_T entry, SMB_ACL_TAG_T tagtype)
{
	return sys_acl_set_tag_type(entry, tagtype);
}

int vfswrap_sys_acl_set_qualifier(vfs_handle_struct *handle, connection_struct *conn, SMB_ACL_ENTRY_T entry, void *qual)
{
	return sys_acl_set_qualifier(entry, qual);
}

int vfswrap_sys_acl_set_permset(vfs_handle_struct *handle, connection_struct *conn, SMB_ACL_ENTRY_T entry, SMB_ACL_PERMSET_T permset)
{
	return sys_acl_set_permset(entry, permset);
}

int vfswrap_sys_acl_valid(vfs_handle_struct *handle, connection_struct *conn, SMB_ACL_T theacl )
{
	return sys_acl_valid(theacl );
}

int vfswrap_sys_acl_set_file(vfs_handle_struct *handle, connection_struct *conn, const char *name, SMB_ACL_TYPE_T acltype, SMB_ACL_T theacl)
{
	return sys_acl_set_file(name, acltype, theacl);
}

int vfswrap_sys_acl_set_fd(vfs_handle_struct *handle, files_struct *fsp, int fd, SMB_ACL_T theacl)
{
	return sys_acl_set_fd(fd, theacl);
}

int vfswrap_sys_acl_delete_def_file(vfs_handle_struct *handle, connection_struct *conn, const char *path)
{
	return sys_acl_delete_def_file(path);
}

int vfswrap_sys_acl_get_perm(vfs_handle_struct *handle, connection_struct *conn, SMB_ACL_PERMSET_T permset, SMB_ACL_PERM_T perm)
{
	return sys_acl_get_perm(permset, perm);
}

int vfswrap_sys_acl_free_text(vfs_handle_struct *handle, connection_struct *conn, char *text)
{
	return sys_acl_free_text(text);
}

int vfswrap_sys_acl_free_acl(vfs_handle_struct *handle, connection_struct *conn, SMB_ACL_T posix_acl)
{
	return sys_acl_free_acl(posix_acl);
}

int vfswrap_sys_acl_free_qualifier(vfs_handle_struct *handle, connection_struct *conn, void *qualifier, SMB_ACL_TAG_T tagtype)
{
	return sys_acl_free_qualifier(qualifier, tagtype);
}

/****************************************************************
 Extended attribute operations.
*****************************************************************/

ssize_t vfswrap_getxattr(struct vfs_handle_struct *handle,struct connection_struct *conn,const char *path, const char *name, void *value, size_t size)
{
	return sys_getxattr(path, name, value, size);
}

ssize_t vfswrap_lgetxattr(struct vfs_handle_struct *handle,struct connection_struct *conn,const char *path, const char *name, void *value, size_t size)
{
	return sys_lgetxattr(path, name, value, size);
}

ssize_t vfswrap_fgetxattr(struct vfs_handle_struct *handle, struct files_struct *fsp,int fd, const char *name, void *value, size_t size)
{
	return sys_fgetxattr(fd, name, value, size);
}

ssize_t vfswrap_listxattr(struct vfs_handle_struct *handle, struct connection_struct *conn,const char *path, char *list, size_t size)
{
	return sys_listxattr(path, list, size);
}

ssize_t vfswrap_llistxattr(struct vfs_handle_struct *handle, struct connection_struct *conn,const char *path, char *list, size_t size)
{
	return sys_llistxattr(path, list, size);
}

ssize_t vfswrap_flistxattr(struct vfs_handle_struct *handle, struct files_struct *fsp,int fd, char *list, size_t size)
{
	return sys_flistxattr(fd, list, size);
}

int vfswrap_removexattr(struct vfs_handle_struct *handle, struct connection_struct *conn,const char *path, const char *name)
{
	return sys_removexattr(path, name);
}

int vfswrap_lremovexattr(struct vfs_handle_struct *handle, struct connection_struct *conn,const char *path, const char *name)
{
	return sys_lremovexattr(path, name);
}

int vfswrap_fremovexattr(struct vfs_handle_struct *handle, struct files_struct *fsp,int fd, const char *name)
{
	return sys_fremovexattr(fd, name);
}

int vfswrap_setxattr(struct vfs_handle_struct *handle, struct connection_struct *conn,const char *path, const char *name, const void *value, size_t size, int flags)
{
	return sys_setxattr(path, name, value, size, flags);
}

int vfswrap_lsetxattr(struct vfs_handle_struct *handle, struct connection_struct *conn,const char *path, const char *name, const void *value, size_t size, int flags)
{
	return sys_lsetxattr(path, name, value, size, flags);
}

int vfswrap_fsetxattr(struct vfs_handle_struct *handle, struct files_struct *fsp,int fd, const char *name, const void *value, size_t size, int flags)
{
	return sys_fsetxattr(fd, name, value, size, flags);
}

int vfswrap_aio_read(struct vfs_handle_struct *handle, struct files_struct *fsp, SMB_STRUCT_AIOCB *aiocb)
{
	return sys_aio_read(aiocb);
}

int vfswrap_aio_write(struct vfs_handle_struct *handle, struct files_struct *fsp, SMB_STRUCT_AIOCB *aiocb)
{
	return sys_aio_write(aiocb);
}

ssize_t vfswrap_aio_return(struct vfs_handle_struct *handle, struct files_struct *fsp, SMB_STRUCT_AIOCB *aiocb)
{
	return sys_aio_return(aiocb);
}

int vfswrap_aio_cancel(struct vfs_handle_struct *handle, struct files_struct *fsp, int fd, SMB_STRUCT_AIOCB *aiocb)
{
	return sys_aio_cancel(fd, aiocb);
}

int vfswrap_aio_error(struct vfs_handle_struct *handle, struct files_struct *fsp, SMB_STRUCT_AIOCB *aiocb)
{
	return sys_aio_error(aiocb);
}

int vfswrap_aio_fsync(struct vfs_handle_struct *handle, struct files_struct *fsp, int op, SMB_STRUCT_AIOCB *aiocb)
{
	return sys_aio_fsync(op, aiocb);
}

int vfswrap_aio_suspend(struct vfs_handle_struct *handle, struct files_struct *fsp, const SMB_STRUCT_AIOCB * const aiocb[], int n, const struct timespec *timeout)
{
	return sys_aio_suspend(aiocb, n, timeout);
}
