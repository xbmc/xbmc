/* 
 * Auditing VFS module for samba.  Log selected file operations to syslog
 * facility.
 *
 * Copyright (C) Tim Potter, 1999-2000
 * Copyright (C) Alexander Bokovoy, 2002
 * Copyright (C) John H Terpstra, 2003
 * Copyright (C) Stefan (metze) Metzmacher, 2003
 * Copyright (C) Volker Lendecke, 2004
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *  
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *  
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/*
 * This module implements parseable logging for all Samba VFS operations.
 *
 * You use it as follows:
 *
 * [tmp]
 * path = /tmp
 * vfs objects = full_audit
 * full_audit:prefix = %u|%I
 * full_audit:success = open opendir
 * full_audit:failure = all
 *
 * vfs op can be "all" which means log all operations.
 * vfs op can be "none" which means no logging.
 *
 * This leads to syslog entries of the form:
 * smbd_audit: nobody|192.168.234.1|opendir|ok|.
 * smbd_audit: nobody|192.168.234.1|open|fail (File not found)|r|x.txt
 *
 * where "nobody" is the connected username and "192.168.234.1" is the
 * client's IP address. 
 *
 * Options:
 *
 * prefix: A macro expansion template prepended to the syslog entry.
 *
 * success: A list of VFS operations for which a successful completion should
 * be logged. Defaults to no logging at all. The special operation "all" logs
 * - you guessed it - everything.
 *
 * failure: A list of VFS operations for which failure to complete should be
 * logged. Defaults to logging everything.
 */


#include "includes.h"

static int vfs_full_audit_debug_level = DBGC_VFS;

struct vfs_full_audit_private_data {
	struct bitmap *success_ops;
	struct bitmap *failure_ops;
};

#undef DBGC_CLASS
#define DBGC_CLASS vfs_full_audit_debug_level

/* Function prototypes */

static int smb_full_audit_connect(vfs_handle_struct *handle, connection_struct *conn,
			 const char *svc, const char *user);
static void smb_full_audit_disconnect(vfs_handle_struct *handle,
			     connection_struct *conn);
static SMB_BIG_UINT smb_full_audit_disk_free(vfs_handle_struct *handle,
				    connection_struct *conn, const char *path,
				    BOOL small_query, SMB_BIG_UINT *bsize, 
				    SMB_BIG_UINT *dfree, SMB_BIG_UINT *dsize);
static int smb_full_audit_get_quota(struct vfs_handle_struct *handle,
			   struct connection_struct *conn,
			   enum SMB_QUOTA_TYPE qtype, unid_t id,
			   SMB_DISK_QUOTA *qt);
static int smb_full_audit_set_quota(struct vfs_handle_struct *handle,
			   struct connection_struct *conn,
			   enum SMB_QUOTA_TYPE qtype, unid_t id,
			   SMB_DISK_QUOTA *qt);
static int smb_full_audit_get_shadow_copy_data(struct vfs_handle_struct *handle,
                                struct files_struct *fsp,
                                SHADOW_COPY_DATA *shadow_copy_data, BOOL labels);
static int smb_full_audit_statvfs(struct vfs_handle_struct *handle,
				struct connection_struct *conn,
				const char *path,
				struct vfs_statvfs_struct *statbuf);

static SMB_STRUCT_DIR *smb_full_audit_opendir(vfs_handle_struct *handle, connection_struct *conn,
			  const char *fname, const char *mask, uint32 attr);
static SMB_STRUCT_DIRENT *smb_full_audit_readdir(vfs_handle_struct *handle,
				    connection_struct *conn, SMB_STRUCT_DIR *dirp);
static void smb_full_audit_seekdir(vfs_handle_struct *handle, connection_struct *conn,
			SMB_STRUCT_DIR *dirp, long offset);
static long smb_full_audit_telldir(vfs_handle_struct *handle, connection_struct *conn,
			SMB_STRUCT_DIR *dirp);
static void smb_full_audit_rewinddir(vfs_handle_struct *handle, connection_struct *conn,
			SMB_STRUCT_DIR *dirp);
static int smb_full_audit_mkdir(vfs_handle_struct *handle, connection_struct *conn,
		       const char *path, mode_t mode);
static int smb_full_audit_rmdir(vfs_handle_struct *handle, connection_struct *conn,
		       const char *path);
static int smb_full_audit_closedir(vfs_handle_struct *handle, connection_struct *conn,
			  SMB_STRUCT_DIR *dirp);
static int smb_full_audit_open(vfs_handle_struct *handle, connection_struct *conn,
		      const char *fname, int flags, mode_t mode);
static int smb_full_audit_close(vfs_handle_struct *handle, files_struct *fsp, int fd);
static ssize_t smb_full_audit_read(vfs_handle_struct *handle, files_struct *fsp,
			  int fd, void *data, size_t n);
static ssize_t smb_full_audit_pread(vfs_handle_struct *handle, files_struct *fsp,
			   int fd, void *data, size_t n, SMB_OFF_T offset);
static ssize_t smb_full_audit_write(vfs_handle_struct *handle, files_struct *fsp,
			   int fd, const void *data, size_t n);
static ssize_t smb_full_audit_pwrite(vfs_handle_struct *handle, files_struct *fsp,
			    int fd, const void *data, size_t n,
			    SMB_OFF_T offset);
static SMB_OFF_T smb_full_audit_lseek(vfs_handle_struct *handle, files_struct *fsp,
			     int filedes, SMB_OFF_T offset, int whence);
static ssize_t smb_full_audit_sendfile(vfs_handle_struct *handle, int tofd,
			      files_struct *fsp, int fromfd,
			      const DATA_BLOB *hdr, SMB_OFF_T offset,
			      size_t n);
static int smb_full_audit_rename(vfs_handle_struct *handle, connection_struct *conn,
			const char *oldname, const char *newname);
static int smb_full_audit_fsync(vfs_handle_struct *handle, files_struct *fsp, int fd);
static int smb_full_audit_stat(vfs_handle_struct *handle, connection_struct *conn,
		      const char *fname, SMB_STRUCT_STAT *sbuf);
static int smb_full_audit_fstat(vfs_handle_struct *handle, files_struct *fsp, int fd,
		       SMB_STRUCT_STAT *sbuf);
static int smb_full_audit_lstat(vfs_handle_struct *handle, connection_struct *conn,
		       const char *path, SMB_STRUCT_STAT *sbuf);
static int smb_full_audit_unlink(vfs_handle_struct *handle, connection_struct *conn,
			const char *path);
static int smb_full_audit_chmod(vfs_handle_struct *handle, connection_struct *conn,
		       const char *path, mode_t mode);
static int smb_full_audit_fchmod(vfs_handle_struct *handle, files_struct *fsp, int fd,
			mode_t mode);
static int smb_full_audit_chown(vfs_handle_struct *handle, connection_struct *conn,
		       const char *path, uid_t uid, gid_t gid);
static int smb_full_audit_fchown(vfs_handle_struct *handle, files_struct *fsp, int fd,
			uid_t uid, gid_t gid);
static int smb_full_audit_chdir(vfs_handle_struct *handle, connection_struct *conn,
		       const char *path);
static char *smb_full_audit_getwd(vfs_handle_struct *handle, connection_struct *conn,
			 char *path);
static int smb_full_audit_utime(vfs_handle_struct *handle, connection_struct *conn,
		       const char *path, struct utimbuf *times);
static int smb_full_audit_ftruncate(vfs_handle_struct *handle, files_struct *fsp,
			   int fd, SMB_OFF_T len);
static BOOL smb_full_audit_lock(vfs_handle_struct *handle, files_struct *fsp, int fd,
		       int op, SMB_OFF_T offset, SMB_OFF_T count, int type);
static BOOL smb_full_audit_getlock(vfs_handle_struct *handle, files_struct *fsp, int fd,
		       SMB_OFF_T *poffset, SMB_OFF_T *pcount, int *ptype, pid_t *ppid);
static int smb_full_audit_symlink(vfs_handle_struct *handle, connection_struct *conn,
			 const char *oldpath, const char *newpath);
static int smb_full_audit_readlink(vfs_handle_struct *handle, connection_struct *conn,
			  const char *path, char *buf, size_t bufsiz);
static int smb_full_audit_link(vfs_handle_struct *handle, connection_struct *conn,
		      const char *oldpath, const char *newpath);
static int smb_full_audit_mknod(vfs_handle_struct *handle, connection_struct *conn,
		       const char *pathname, mode_t mode, SMB_DEV_T dev);
static char *smb_full_audit_realpath(vfs_handle_struct *handle, connection_struct *conn,
			    const char *path, char *resolved_path);
static size_t smb_full_audit_fget_nt_acl(vfs_handle_struct *handle, files_struct *fsp,
				int fd, uint32 security_info,
				SEC_DESC **ppdesc);
static size_t smb_full_audit_get_nt_acl(vfs_handle_struct *handle, files_struct *fsp,
			       const char *name, uint32 security_info,
			       SEC_DESC **ppdesc);
static BOOL smb_full_audit_fset_nt_acl(vfs_handle_struct *handle, files_struct *fsp,
			      int fd, uint32 security_info_sent,
			      SEC_DESC *psd);
static BOOL smb_full_audit_set_nt_acl(vfs_handle_struct *handle, files_struct *fsp,
			     const char *name, uint32 security_info_sent,
			     SEC_DESC *psd);
static int smb_full_audit_chmod_acl(vfs_handle_struct *handle, connection_struct *conn,
			   const char *path, mode_t mode);
static int smb_full_audit_fchmod_acl(vfs_handle_struct *handle, files_struct *fsp,
			    int fd, mode_t mode);
static int smb_full_audit_sys_acl_get_entry(vfs_handle_struct *handle,
				   connection_struct *conn,
				   SMB_ACL_T theacl, int entry_id,
				   SMB_ACL_ENTRY_T *entry_p);
static int smb_full_audit_sys_acl_get_tag_type(vfs_handle_struct *handle,
				      connection_struct *conn,
				      SMB_ACL_ENTRY_T entry_d,
				      SMB_ACL_TAG_T *tag_type_p);
static int smb_full_audit_sys_acl_get_permset(vfs_handle_struct *handle,
				     connection_struct *conn,
				     SMB_ACL_ENTRY_T entry_d,
				     SMB_ACL_PERMSET_T *permset_p);
static void * smb_full_audit_sys_acl_get_qualifier(vfs_handle_struct *handle,
					  connection_struct *conn,
					  SMB_ACL_ENTRY_T entry_d);
static SMB_ACL_T smb_full_audit_sys_acl_get_file(vfs_handle_struct *handle,
					connection_struct *conn,
					const char *path_p,
					SMB_ACL_TYPE_T type);
static SMB_ACL_T smb_full_audit_sys_acl_get_fd(vfs_handle_struct *handle,
				      files_struct *fsp,
				      int fd);
static int smb_full_audit_sys_acl_clear_perms(vfs_handle_struct *handle,
				     connection_struct *conn,
				     SMB_ACL_PERMSET_T permset);
static int smb_full_audit_sys_acl_add_perm(vfs_handle_struct *handle,
				  connection_struct *conn,
				  SMB_ACL_PERMSET_T permset,
				  SMB_ACL_PERM_T perm);
static char * smb_full_audit_sys_acl_to_text(vfs_handle_struct *handle,
				    connection_struct *conn, SMB_ACL_T theacl,
				    ssize_t *plen);
static SMB_ACL_T smb_full_audit_sys_acl_init(vfs_handle_struct *handle,
				    connection_struct *conn,
				    int count);
static int smb_full_audit_sys_acl_create_entry(vfs_handle_struct *handle,
				      connection_struct *conn, SMB_ACL_T *pacl,
				      SMB_ACL_ENTRY_T *pentry);
static int smb_full_audit_sys_acl_set_tag_type(vfs_handle_struct *handle,
				      connection_struct *conn,
				      SMB_ACL_ENTRY_T entry,
				      SMB_ACL_TAG_T tagtype);
static int smb_full_audit_sys_acl_set_qualifier(vfs_handle_struct *handle,
				       connection_struct *conn,
				       SMB_ACL_ENTRY_T entry,
				       void *qual);
static int smb_full_audit_sys_acl_set_permset(vfs_handle_struct *handle,
				     connection_struct *conn,
				     SMB_ACL_ENTRY_T entry,
				     SMB_ACL_PERMSET_T permset);
static int smb_full_audit_sys_acl_valid(vfs_handle_struct *handle,
			       connection_struct *conn,
			       SMB_ACL_T theacl );
static int smb_full_audit_sys_acl_set_file(vfs_handle_struct *handle,
				  connection_struct *conn,
				  const char *name, SMB_ACL_TYPE_T acltype,
				  SMB_ACL_T theacl);
static int smb_full_audit_sys_acl_set_fd(vfs_handle_struct *handle, files_struct *fsp,
				int fd, SMB_ACL_T theacl);
static int smb_full_audit_sys_acl_delete_def_file(vfs_handle_struct *handle,
					 connection_struct *conn,
					 const char *path);
static int smb_full_audit_sys_acl_get_perm(vfs_handle_struct *handle,
				  connection_struct *conn,
				  SMB_ACL_PERMSET_T permset,
				  SMB_ACL_PERM_T perm);
static int smb_full_audit_sys_acl_free_text(vfs_handle_struct *handle,
				   connection_struct *conn,
				   char *text);
static int smb_full_audit_sys_acl_free_acl(vfs_handle_struct *handle,
				  connection_struct *conn,
				  SMB_ACL_T posix_acl);
static int smb_full_audit_sys_acl_free_qualifier(vfs_handle_struct *handle,
					connection_struct *conn,
					void *qualifier,
					SMB_ACL_TAG_T tagtype);
static ssize_t smb_full_audit_getxattr(struct vfs_handle_struct *handle,
			      struct connection_struct *conn, const char *path,
			      const char *name, void *value, size_t size);
static ssize_t smb_full_audit_lgetxattr(struct vfs_handle_struct *handle,
			       struct connection_struct *conn,
			       const char *path, const char *name,
			       void *value, size_t size);
static ssize_t smb_full_audit_fgetxattr(struct vfs_handle_struct *handle,
			       struct files_struct *fsp, int fd,
			       const char *name, void *value, size_t size);
static ssize_t smb_full_audit_listxattr(struct vfs_handle_struct *handle,
			       struct connection_struct *conn,
			       const char *path, char *list, size_t size);
static ssize_t smb_full_audit_llistxattr(struct vfs_handle_struct *handle,
				struct connection_struct *conn,
				const char *path, char *list, size_t size);
static ssize_t smb_full_audit_flistxattr(struct vfs_handle_struct *handle,
				struct files_struct *fsp, int fd, char *list,
				size_t size);
static int smb_full_audit_removexattr(struct vfs_handle_struct *handle,
			     struct connection_struct *conn, const char *path,
			     const char *name);
static int smb_full_audit_lremovexattr(struct vfs_handle_struct *handle,
			      struct connection_struct *conn, const char *path,
			      const char *name);
static int smb_full_audit_fremovexattr(struct vfs_handle_struct *handle,
			      struct files_struct *fsp, int fd,
			      const char *name);
static int smb_full_audit_setxattr(struct vfs_handle_struct *handle,
			  struct connection_struct *conn, const char *path,
			  const char *name, const void *value, size_t size,
			  int flags);
static int smb_full_audit_lsetxattr(struct vfs_handle_struct *handle,
			   struct connection_struct *conn, const char *path,
			   const char *name, const void *value, size_t size,
			   int flags);
static int smb_full_audit_fsetxattr(struct vfs_handle_struct *handle,
			   struct files_struct *fsp, int fd, const char *name,
			   const void *value, size_t size, int flags);

static int smb_full_audit_aio_read(struct vfs_handle_struct *handle, struct files_struct *fsp, SMB_STRUCT_AIOCB *aiocb);
static int smb_full_audit_aio_write(struct vfs_handle_struct *handle, struct files_struct *fsp, SMB_STRUCT_AIOCB *aiocb);
static ssize_t smb_full_audit_aio_return(struct vfs_handle_struct *handle, struct files_struct *fsp, SMB_STRUCT_AIOCB *aiocb);
static int smb_full_audit_aio_cancel(struct vfs_handle_struct *handle, struct files_struct *fsp, int fd, SMB_STRUCT_AIOCB *aiocb);
static int smb_full_audit_aio_error(struct vfs_handle_struct *handle, struct files_struct *fsp, SMB_STRUCT_AIOCB *aiocb);
static int smb_full_audit_aio_fsync(struct vfs_handle_struct *handle, struct files_struct *fsp, int op, SMB_STRUCT_AIOCB *aiocb);
static int smb_full_audit_aio_suspend(struct vfs_handle_struct *handle, struct files_struct *fsp, const SMB_STRUCT_AIOCB * const aiocb[], int n, const struct timespec *ts);

/* VFS operations */

static vfs_op_tuple audit_op_tuples[] = {
    
	/* Disk operations */

	{SMB_VFS_OP(smb_full_audit_connect),	SMB_VFS_OP_CONNECT,
	 SMB_VFS_LAYER_LOGGER},
	{SMB_VFS_OP(smb_full_audit_disconnect),	SMB_VFS_OP_DISCONNECT,
	 SMB_VFS_LAYER_LOGGER},
	{SMB_VFS_OP(smb_full_audit_disk_free),	SMB_VFS_OP_DISK_FREE,
	 SMB_VFS_LAYER_LOGGER},
	{SMB_VFS_OP(smb_full_audit_get_quota),	SMB_VFS_OP_GET_QUOTA,
	 SMB_VFS_LAYER_LOGGER},
	{SMB_VFS_OP(smb_full_audit_set_quota),	SMB_VFS_OP_SET_QUOTA,
	 SMB_VFS_LAYER_LOGGER},
	{SMB_VFS_OP(smb_full_audit_get_shadow_copy_data), SMB_VFS_OP_GET_SHADOW_COPY_DATA,
	 SMB_VFS_LAYER_LOGGER},
	{SMB_VFS_OP(smb_full_audit_statvfs),	SMB_VFS_OP_STATVFS,
	 SMB_VFS_LAYER_LOGGER},

	/* Directory operations */

	{SMB_VFS_OP(smb_full_audit_opendir),	SMB_VFS_OP_OPENDIR,
	 SMB_VFS_LAYER_LOGGER},
	{SMB_VFS_OP(smb_full_audit_readdir),	SMB_VFS_OP_READDIR,
	 SMB_VFS_LAYER_LOGGER},
	{SMB_VFS_OP(smb_full_audit_seekdir),	SMB_VFS_OP_SEEKDIR,
	 SMB_VFS_LAYER_LOGGER},
	{SMB_VFS_OP(smb_full_audit_telldir),	SMB_VFS_OP_TELLDIR,
	 SMB_VFS_LAYER_LOGGER},
	{SMB_VFS_OP(smb_full_audit_rewinddir),	SMB_VFS_OP_REWINDDIR,
	 SMB_VFS_LAYER_LOGGER},
	{SMB_VFS_OP(smb_full_audit_mkdir),	SMB_VFS_OP_MKDIR,
	 SMB_VFS_LAYER_LOGGER},
	{SMB_VFS_OP(smb_full_audit_rmdir),	SMB_VFS_OP_RMDIR,
	 SMB_VFS_LAYER_LOGGER},
	{SMB_VFS_OP(smb_full_audit_closedir),	SMB_VFS_OP_CLOSEDIR,
	 SMB_VFS_LAYER_LOGGER},

	/* File operations */

	{SMB_VFS_OP(smb_full_audit_open),	SMB_VFS_OP_OPEN,
	 SMB_VFS_LAYER_LOGGER},
	{SMB_VFS_OP(smb_full_audit_close),	SMB_VFS_OP_CLOSE,
	 SMB_VFS_LAYER_LOGGER},
	{SMB_VFS_OP(smb_full_audit_read),	SMB_VFS_OP_READ,
	 SMB_VFS_LAYER_LOGGER},
	{SMB_VFS_OP(smb_full_audit_pread),	SMB_VFS_OP_PREAD,
	 SMB_VFS_LAYER_LOGGER},
	{SMB_VFS_OP(smb_full_audit_write),	SMB_VFS_OP_WRITE,
	 SMB_VFS_LAYER_LOGGER},
	{SMB_VFS_OP(smb_full_audit_pwrite),	SMB_VFS_OP_PWRITE,
	 SMB_VFS_LAYER_LOGGER},
	{SMB_VFS_OP(smb_full_audit_lseek),	SMB_VFS_OP_LSEEK,
	 SMB_VFS_LAYER_LOGGER},
	{SMB_VFS_OP(smb_full_audit_sendfile),	SMB_VFS_OP_SENDFILE,
	 SMB_VFS_LAYER_LOGGER},
	{SMB_VFS_OP(smb_full_audit_rename),	SMB_VFS_OP_RENAME,
	 SMB_VFS_LAYER_LOGGER},
	{SMB_VFS_OP(smb_full_audit_fsync),	SMB_VFS_OP_FSYNC,
	 SMB_VFS_LAYER_LOGGER},
	{SMB_VFS_OP(smb_full_audit_stat),	SMB_VFS_OP_STAT,
	 SMB_VFS_LAYER_LOGGER},
	{SMB_VFS_OP(smb_full_audit_fstat),	SMB_VFS_OP_FSTAT,
	 SMB_VFS_LAYER_LOGGER},
	{SMB_VFS_OP(smb_full_audit_lstat),	SMB_VFS_OP_LSTAT,
	 SMB_VFS_LAYER_LOGGER},
	{SMB_VFS_OP(smb_full_audit_unlink),	SMB_VFS_OP_UNLINK,
	 SMB_VFS_LAYER_LOGGER},
	{SMB_VFS_OP(smb_full_audit_chmod),	SMB_VFS_OP_CHMOD,
	 SMB_VFS_LAYER_LOGGER},
	{SMB_VFS_OP(smb_full_audit_fchmod),	SMB_VFS_OP_FCHMOD,
	 SMB_VFS_LAYER_LOGGER},
	{SMB_VFS_OP(smb_full_audit_chown),	SMB_VFS_OP_CHOWN,
	 SMB_VFS_LAYER_LOGGER},
	{SMB_VFS_OP(smb_full_audit_fchown),	SMB_VFS_OP_FCHOWN,
	 SMB_VFS_LAYER_LOGGER},
	{SMB_VFS_OP(smb_full_audit_chdir),	SMB_VFS_OP_CHDIR,
	 SMB_VFS_LAYER_LOGGER},
	{SMB_VFS_OP(smb_full_audit_getwd),	SMB_VFS_OP_GETWD,
	 SMB_VFS_LAYER_LOGGER},
	{SMB_VFS_OP(smb_full_audit_utime),	SMB_VFS_OP_UTIME,
	 SMB_VFS_LAYER_LOGGER},
	{SMB_VFS_OP(smb_full_audit_ftruncate),	SMB_VFS_OP_FTRUNCATE,
	 SMB_VFS_LAYER_LOGGER},
	{SMB_VFS_OP(smb_full_audit_lock),	SMB_VFS_OP_LOCK,
	 SMB_VFS_LAYER_LOGGER},
	{SMB_VFS_OP(smb_full_audit_getlock),	SMB_VFS_OP_GETLOCK,
	 SMB_VFS_LAYER_LOGGER},
	{SMB_VFS_OP(smb_full_audit_symlink),	SMB_VFS_OP_SYMLINK,
	 SMB_VFS_LAYER_LOGGER},
	{SMB_VFS_OP(smb_full_audit_readlink),	SMB_VFS_OP_READLINK,
	 SMB_VFS_LAYER_LOGGER},
	{SMB_VFS_OP(smb_full_audit_link),	SMB_VFS_OP_LINK,
	 SMB_VFS_LAYER_LOGGER},
	{SMB_VFS_OP(smb_full_audit_mknod),	SMB_VFS_OP_MKNOD,
	 SMB_VFS_LAYER_LOGGER},
	{SMB_VFS_OP(smb_full_audit_realpath),	SMB_VFS_OP_REALPATH,
	 SMB_VFS_LAYER_LOGGER},

	/* NT ACL operations. */

	{SMB_VFS_OP(smb_full_audit_fget_nt_acl),	SMB_VFS_OP_FGET_NT_ACL,
	 SMB_VFS_LAYER_LOGGER},
	{SMB_VFS_OP(smb_full_audit_get_nt_acl),	SMB_VFS_OP_GET_NT_ACL,
	 SMB_VFS_LAYER_LOGGER},
	{SMB_VFS_OP(smb_full_audit_fset_nt_acl),	SMB_VFS_OP_FSET_NT_ACL,
	 SMB_VFS_LAYER_LOGGER},
	{SMB_VFS_OP(smb_full_audit_set_nt_acl),	SMB_VFS_OP_SET_NT_ACL,
	 SMB_VFS_LAYER_LOGGER},

	/* POSIX ACL operations. */

	{SMB_VFS_OP(smb_full_audit_chmod_acl),	SMB_VFS_OP_CHMOD_ACL,
	 SMB_VFS_LAYER_LOGGER},
	{SMB_VFS_OP(smb_full_audit_fchmod_acl),	SMB_VFS_OP_FCHMOD_ACL,
	 SMB_VFS_LAYER_LOGGER},
	{SMB_VFS_OP(smb_full_audit_sys_acl_get_entry),	SMB_VFS_OP_SYS_ACL_GET_ENTRY,
	 SMB_VFS_LAYER_LOGGER},
	{SMB_VFS_OP(smb_full_audit_sys_acl_get_tag_type),	SMB_VFS_OP_SYS_ACL_GET_TAG_TYPE,
	 SMB_VFS_LAYER_LOGGER},
	{SMB_VFS_OP(smb_full_audit_sys_acl_get_permset),	SMB_VFS_OP_SYS_ACL_GET_PERMSET,
	 SMB_VFS_LAYER_LOGGER},
	{SMB_VFS_OP(smb_full_audit_sys_acl_get_qualifier),	SMB_VFS_OP_SYS_ACL_GET_QUALIFIER,
	 SMB_VFS_LAYER_LOGGER},
	{SMB_VFS_OP(smb_full_audit_sys_acl_get_file),	SMB_VFS_OP_SYS_ACL_GET_FILE,
	 SMB_VFS_LAYER_LOGGER},
	{SMB_VFS_OP(smb_full_audit_sys_acl_get_fd),	SMB_VFS_OP_SYS_ACL_GET_FD,
	 SMB_VFS_LAYER_LOGGER},
	{SMB_VFS_OP(smb_full_audit_sys_acl_clear_perms),	SMB_VFS_OP_SYS_ACL_CLEAR_PERMS,
	 SMB_VFS_LAYER_LOGGER},
	{SMB_VFS_OP(smb_full_audit_sys_acl_add_perm),	SMB_VFS_OP_SYS_ACL_ADD_PERM,
	 SMB_VFS_LAYER_LOGGER},
	{SMB_VFS_OP(smb_full_audit_sys_acl_to_text),	SMB_VFS_OP_SYS_ACL_TO_TEXT,
	 SMB_VFS_LAYER_LOGGER},
	{SMB_VFS_OP(smb_full_audit_sys_acl_init),	SMB_VFS_OP_SYS_ACL_INIT,
	 SMB_VFS_LAYER_LOGGER},
	{SMB_VFS_OP(smb_full_audit_sys_acl_create_entry),	SMB_VFS_OP_SYS_ACL_CREATE_ENTRY,
	 SMB_VFS_LAYER_LOGGER},
	{SMB_VFS_OP(smb_full_audit_sys_acl_set_tag_type),	SMB_VFS_OP_SYS_ACL_SET_TAG_TYPE,
	 SMB_VFS_LAYER_LOGGER},
	{SMB_VFS_OP(smb_full_audit_sys_acl_set_qualifier),	SMB_VFS_OP_SYS_ACL_SET_QUALIFIER,
	 SMB_VFS_LAYER_LOGGER},
	{SMB_VFS_OP(smb_full_audit_sys_acl_set_permset),	SMB_VFS_OP_SYS_ACL_SET_PERMSET,
	 SMB_VFS_LAYER_LOGGER},
	{SMB_VFS_OP(smb_full_audit_sys_acl_valid),	SMB_VFS_OP_SYS_ACL_VALID,
	 SMB_VFS_LAYER_LOGGER},
	{SMB_VFS_OP(smb_full_audit_sys_acl_set_file),	SMB_VFS_OP_SYS_ACL_SET_FILE,
	 SMB_VFS_LAYER_LOGGER},
	{SMB_VFS_OP(smb_full_audit_sys_acl_set_fd),	SMB_VFS_OP_SYS_ACL_SET_FD,
	 SMB_VFS_LAYER_LOGGER},
	{SMB_VFS_OP(smb_full_audit_sys_acl_delete_def_file),	SMB_VFS_OP_SYS_ACL_DELETE_DEF_FILE,
	 SMB_VFS_LAYER_LOGGER},
	{SMB_VFS_OP(smb_full_audit_sys_acl_get_perm),	SMB_VFS_OP_SYS_ACL_GET_PERM,
	 SMB_VFS_LAYER_LOGGER},
	{SMB_VFS_OP(smb_full_audit_sys_acl_free_text),	SMB_VFS_OP_SYS_ACL_FREE_TEXT,
	 SMB_VFS_LAYER_LOGGER},
	{SMB_VFS_OP(smb_full_audit_sys_acl_free_acl),	SMB_VFS_OP_SYS_ACL_FREE_ACL,
	 SMB_VFS_LAYER_LOGGER},
	{SMB_VFS_OP(smb_full_audit_sys_acl_free_qualifier),	SMB_VFS_OP_SYS_ACL_FREE_QUALIFIER,
	 SMB_VFS_LAYER_LOGGER},
	
	/* EA operations. */

	{SMB_VFS_OP(smb_full_audit_getxattr),	SMB_VFS_OP_GETXATTR,
	 SMB_VFS_LAYER_LOGGER},
	{SMB_VFS_OP(smb_full_audit_lgetxattr),	SMB_VFS_OP_LGETXATTR,
	 SMB_VFS_LAYER_LOGGER},
	{SMB_VFS_OP(smb_full_audit_fgetxattr),	SMB_VFS_OP_FGETXATTR,
	 SMB_VFS_LAYER_LOGGER},
	{SMB_VFS_OP(smb_full_audit_listxattr),	SMB_VFS_OP_LISTXATTR,
	 SMB_VFS_LAYER_LOGGER},
	{SMB_VFS_OP(smb_full_audit_llistxattr),	SMB_VFS_OP_LLISTXATTR,
	 SMB_VFS_LAYER_LOGGER},
	{SMB_VFS_OP(smb_full_audit_flistxattr),	SMB_VFS_OP_FLISTXATTR,
	 SMB_VFS_LAYER_LOGGER},
	{SMB_VFS_OP(smb_full_audit_removexattr),	SMB_VFS_OP_REMOVEXATTR,
	 SMB_VFS_LAYER_LOGGER},
	{SMB_VFS_OP(smb_full_audit_lremovexattr),	SMB_VFS_OP_LREMOVEXATTR,
	 SMB_VFS_LAYER_LOGGER},
	{SMB_VFS_OP(smb_full_audit_fremovexattr),	SMB_VFS_OP_FREMOVEXATTR,
	 SMB_VFS_LAYER_LOGGER},
	{SMB_VFS_OP(smb_full_audit_setxattr),	SMB_VFS_OP_SETXATTR,
	 SMB_VFS_LAYER_LOGGER},
	{SMB_VFS_OP(smb_full_audit_lsetxattr),	SMB_VFS_OP_LSETXATTR,
	 SMB_VFS_LAYER_LOGGER},
	{SMB_VFS_OP(smb_full_audit_fsetxattr),	SMB_VFS_OP_FSETXATTR,
	 SMB_VFS_LAYER_LOGGER},
	
	{SMB_VFS_OP(smb_full_audit_aio_read),	SMB_VFS_OP_AIO_READ,
	 SMB_VFS_LAYER_LOGGER},
	{SMB_VFS_OP(smb_full_audit_aio_write),	SMB_VFS_OP_AIO_WRITE,
	 SMB_VFS_LAYER_LOGGER},
	{SMB_VFS_OP(smb_full_audit_aio_return),	SMB_VFS_OP_AIO_RETURN,
	 SMB_VFS_LAYER_LOGGER},
	{SMB_VFS_OP(smb_full_audit_aio_cancel), SMB_VFS_OP_AIO_CANCEL,
	 SMB_VFS_LAYER_LOGGER},
	{SMB_VFS_OP(smb_full_audit_aio_error),	SMB_VFS_OP_AIO_ERROR,
	 SMB_VFS_LAYER_LOGGER},
	{SMB_VFS_OP(smb_full_audit_aio_fsync),	SMB_VFS_OP_AIO_FSYNC,
	 SMB_VFS_LAYER_LOGGER},
	{SMB_VFS_OP(smb_full_audit_aio_suspend),SMB_VFS_OP_AIO_SUSPEND,
	 SMB_VFS_LAYER_LOGGER},

	/* Finish VFS operations definition */
	
	{SMB_VFS_OP(NULL),		SMB_VFS_OP_NOOP,
	 SMB_VFS_LAYER_NOOP}
};

/* The following array *must* be in the same order as defined in vfs.h */

static struct {
	vfs_op_type type;
	const char *name;
} vfs_op_names[] = {
	{ SMB_VFS_OP_CONNECT,	"connect" },
	{ SMB_VFS_OP_DISCONNECT,	"disconnect" },
	{ SMB_VFS_OP_DISK_FREE,	"disk_free" },
	{ SMB_VFS_OP_GET_QUOTA,	"get_quota" },
	{ SMB_VFS_OP_SET_QUOTA,	"set_quota" },
	{ SMB_VFS_OP_GET_SHADOW_COPY_DATA,	"get_shadow_copy_data" },
	{ SMB_VFS_OP_STATVFS,	"statvfs" },
	{ SMB_VFS_OP_OPENDIR,	"opendir" },
	{ SMB_VFS_OP_READDIR,	"readdir" },
	{ SMB_VFS_OP_SEEKDIR,   "seekdir" },
	{ SMB_VFS_OP_TELLDIR,   "telldir" },
	{ SMB_VFS_OP_REWINDDIR, "rewinddir" },
	{ SMB_VFS_OP_MKDIR,	"mkdir" },
	{ SMB_VFS_OP_RMDIR,	"rmdir" },
	{ SMB_VFS_OP_CLOSEDIR,	"closedir" },
	{ SMB_VFS_OP_OPEN,	"open" },
	{ SMB_VFS_OP_CLOSE,	"close" },
	{ SMB_VFS_OP_READ,	"read" },
	{ SMB_VFS_OP_PREAD,	"pread" },
	{ SMB_VFS_OP_WRITE,	"write" },
	{ SMB_VFS_OP_PWRITE,	"pwrite" },
	{ SMB_VFS_OP_LSEEK,	"lseek" },
	{ SMB_VFS_OP_SENDFILE,	"sendfile" },
	{ SMB_VFS_OP_RENAME,	"rename" },
	{ SMB_VFS_OP_FSYNC,	"fsync" },
	{ SMB_VFS_OP_STAT,	"stat" },
	{ SMB_VFS_OP_FSTAT,	"fstat" },
	{ SMB_VFS_OP_LSTAT,	"lstat" },
	{ SMB_VFS_OP_UNLINK,	"unlink" },
	{ SMB_VFS_OP_CHMOD,	"chmod" },
	{ SMB_VFS_OP_FCHMOD,	"fchmod" },
	{ SMB_VFS_OP_CHOWN,	"chown" },
	{ SMB_VFS_OP_FCHOWN,	"fchown" },
	{ SMB_VFS_OP_CHDIR,	"chdir" },
	{ SMB_VFS_OP_GETWD,	"getwd" },
	{ SMB_VFS_OP_UTIME,	"utime" },
	{ SMB_VFS_OP_FTRUNCATE,	"ftruncate" },
	{ SMB_VFS_OP_LOCK,	"lock" },
	{ SMB_VFS_OP_GETLOCK,	"getlock" },
	{ SMB_VFS_OP_SYMLINK,	"symlink" },
	{ SMB_VFS_OP_READLINK,	"readlink" },
	{ SMB_VFS_OP_LINK,	"link" },
	{ SMB_VFS_OP_MKNOD,	"mknod" },
	{ SMB_VFS_OP_REALPATH,	"realpath" },
	{ SMB_VFS_OP_FGET_NT_ACL,	"fget_nt_acl" },
	{ SMB_VFS_OP_GET_NT_ACL,	"get_nt_acl" },
	{ SMB_VFS_OP_FSET_NT_ACL,	"fset_nt_acl" },
	{ SMB_VFS_OP_SET_NT_ACL,	"set_nt_acl" },
	{ SMB_VFS_OP_CHMOD_ACL,	"chmod_acl" },
	{ SMB_VFS_OP_FCHMOD_ACL,	"fchmod_acl" },
	{ SMB_VFS_OP_SYS_ACL_GET_ENTRY,	"sys_acl_get_entry" },
	{ SMB_VFS_OP_SYS_ACL_GET_TAG_TYPE,	"sys_acl_get_tag_type" },
	{ SMB_VFS_OP_SYS_ACL_GET_PERMSET,	"sys_acl_get_permset" },
	{ SMB_VFS_OP_SYS_ACL_GET_QUALIFIER,	"sys_acl_get_qualifier" },
	{ SMB_VFS_OP_SYS_ACL_GET_FILE,	"sys_acl_get_file" },
	{ SMB_VFS_OP_SYS_ACL_GET_FD,	"sys_acl_get_fd" },
	{ SMB_VFS_OP_SYS_ACL_CLEAR_PERMS,	"sys_acl_clear_perms" },
	{ SMB_VFS_OP_SYS_ACL_ADD_PERM,	"sys_acl_add_perm" },
	{ SMB_VFS_OP_SYS_ACL_TO_TEXT,	"sys_acl_to_text" },
	{ SMB_VFS_OP_SYS_ACL_INIT,	"sys_acl_init" },
	{ SMB_VFS_OP_SYS_ACL_CREATE_ENTRY,	"sys_acl_create_entry" },
	{ SMB_VFS_OP_SYS_ACL_SET_TAG_TYPE,	"sys_acl_set_tag_type" },
	{ SMB_VFS_OP_SYS_ACL_SET_QUALIFIER,	"sys_acl_set_qualifier" },
	{ SMB_VFS_OP_SYS_ACL_SET_PERMSET,	"sys_acl_set_permset" },
	{ SMB_VFS_OP_SYS_ACL_VALID,	"sys_acl_valid" },
	{ SMB_VFS_OP_SYS_ACL_SET_FILE,	"sys_acl_set_file" },
	{ SMB_VFS_OP_SYS_ACL_SET_FD,	"sys_acl_set_fd" },
	{ SMB_VFS_OP_SYS_ACL_DELETE_DEF_FILE,	"sys_acl_delete_def_file" },
	{ SMB_VFS_OP_SYS_ACL_GET_PERM,	"sys_acl_get_perm" },
	{ SMB_VFS_OP_SYS_ACL_FREE_TEXT,	"sys_acl_free_text" },
	{ SMB_VFS_OP_SYS_ACL_FREE_ACL,	"sys_acl_free_acl" },
	{ SMB_VFS_OP_SYS_ACL_FREE_QUALIFIER,	"sys_acl_free_qualifier" },
	{ SMB_VFS_OP_GETXATTR,	"getxattr" },
	{ SMB_VFS_OP_LGETXATTR,	"lgetxattr" },
	{ SMB_VFS_OP_FGETXATTR,	"fgetxattr" },
	{ SMB_VFS_OP_LISTXATTR,	"listxattr" },
	{ SMB_VFS_OP_LLISTXATTR,	"llistxattr" },
	{ SMB_VFS_OP_FLISTXATTR,	"flistxattr" },
	{ SMB_VFS_OP_REMOVEXATTR,	"removexattr" },
	{ SMB_VFS_OP_LREMOVEXATTR,	"lremovexattr" },
	{ SMB_VFS_OP_FREMOVEXATTR,	"fremovexattr" },
	{ SMB_VFS_OP_SETXATTR,	"setxattr" },
	{ SMB_VFS_OP_LSETXATTR,	"lsetxattr" },
	{ SMB_VFS_OP_FSETXATTR,	"fsetxattr" },
	{ SMB_VFS_OP_AIO_READ,	"aio_read" },
	{ SMB_VFS_OP_AIO_WRITE,	"aio_write" },
	{ SMB_VFS_OP_AIO_RETURN,"aio_return" },
	{ SMB_VFS_OP_AIO_CANCEL,"aio_cancel" },
	{ SMB_VFS_OP_AIO_ERROR,	"aio_error" },
	{ SMB_VFS_OP_AIO_FSYNC,	"aio_fsync" },
	{ SMB_VFS_OP_AIO_SUSPEND,"aio_suspend" },
	{ SMB_VFS_OP_LAST, NULL }
};	

static int audit_syslog_facility(vfs_handle_struct *handle)
{
	static const struct enum_list enum_log_facilities[] = {
		{ LOG_USER, "USER" },
		{ LOG_LOCAL0, "LOCAL0" },
		{ LOG_LOCAL1, "LOCAL1" },
		{ LOG_LOCAL2, "LOCAL2" },
		{ LOG_LOCAL3, "LOCAL3" },
		{ LOG_LOCAL4, "LOCAL4" },
		{ LOG_LOCAL5, "LOCAL5" },
		{ LOG_LOCAL6, "LOCAL6" },
		{ LOG_LOCAL7, "LOCAL7" }
	};

	int facility;

	facility = lp_parm_enum(SNUM(handle->conn), "full_audit", "facility", enum_log_facilities, LOG_USER);

	return facility;
}

static int audit_syslog_priority(vfs_handle_struct *handle)
{
	static const struct enum_list enum_log_priorities[] = {
		{ LOG_EMERG, "EMERG" },
		{ LOG_ALERT, "ALERT" },
		{ LOG_CRIT, "CRIT" },
		{ LOG_ERR, "ERR" },
		{ LOG_WARNING, "WARNING" },
		{ LOG_NOTICE, "NOTICE" },
		{ LOG_INFO, "INFO" },
		{ LOG_DEBUG, "DEBUG" }
	};

	int priority;

	priority = lp_parm_enum(SNUM(handle->conn), "full_audit", "priority", enum_log_priorities, LOG_NOTICE);

	return priority;
}

static char *audit_prefix(connection_struct *conn)
{
	static pstring prefix;

	pstrcpy(prefix, lp_parm_const_string(SNUM(conn), "full_audit",
					     "prefix", "%u|%I"));
	standard_sub_snum(SNUM(conn), prefix, sizeof(prefix)-1);
	return prefix;
}

static BOOL log_success(vfs_handle_struct *handle, vfs_op_type op)
{
	struct vfs_full_audit_private_data *pd = NULL;

	SMB_VFS_HANDLE_GET_DATA(handle, pd,
		struct vfs_full_audit_private_data,
		return True);

	if (pd->success_ops == NULL) {
		return True;
	}

	return bitmap_query(pd->success_ops, op);
}

static BOOL log_failure(vfs_handle_struct *handle, vfs_op_type op)
{
	struct vfs_full_audit_private_data *pd = NULL;

	SMB_VFS_HANDLE_GET_DATA(handle, pd,
		struct vfs_full_audit_private_data,
		return True);

	if (pd->failure_ops == NULL)
		return True;

	return bitmap_query(pd->failure_ops, op);
}

static void init_bitmap(struct bitmap **bm, const char **ops)
{
	BOOL log_all = False;

	if (*bm != NULL)
		return;

	*bm = bitmap_allocate(SMB_VFS_OP_LAST);

	if (*bm == NULL) {
		DEBUG(0, ("Could not alloc bitmap -- "
			  "defaulting to logging everything\n"));
		return;
	}

	while (*ops != NULL) {
		int i;
		BOOL found = False;

		if (strequal(*ops, "all")) {
			log_all = True;
			break;
		}

		if (strequal(*ops, "none")) {
			break;
		}

		for (i=0; i<SMB_VFS_OP_LAST; i++) {
			if (vfs_op_names[i].name == NULL) {
				smb_panic("vfs_full_audit.c: name table not "
					  "in sync with vfs.h\n");
			}

			if (strequal(*ops, vfs_op_names[i].name)) {
				bitmap_set(*bm, i);
				found = True;
			}
		}
		if (!found) {
			DEBUG(0, ("Could not find opname %s, logging all\n",
				  *ops));
			log_all = True;
			break;
		}
		ops += 1;
	}

	if (log_all) {
		/* The query functions default to True */
		bitmap_free(*bm);
		*bm = NULL;
	}
}

static const char *audit_opname(vfs_op_type op)
{
	if (op >= SMB_VFS_OP_LAST)
		return "INVALID VFS OP";
	return vfs_op_names[op].name;
}

static void do_log(vfs_op_type op, BOOL success, vfs_handle_struct *handle,
		   const char *format, ...)
{
	fstring err_msg;
	pstring op_msg;
	va_list ap;

	if (success && (!log_success(handle, op)))
		return;

	if (!success && (!log_failure(handle, op)))
		return;

	if (success)
		fstrcpy(err_msg, "ok");
	else
		fstr_sprintf(err_msg, "fail (%s)", strerror(errno));

	va_start(ap, format);
	vsnprintf(op_msg, sizeof(op_msg), format, ap);
	va_end(ap);

	syslog(audit_syslog_priority(handle), "%s|%s|%s|%s\n",
	       audit_prefix(handle->conn), audit_opname(op), err_msg, op_msg);

	return;
}

/* Free function for the private data. */

static void free_private_data(void **p_data)
{
	struct vfs_full_audit_private_data *pd = *(struct vfs_full_audit_private_data **)p_data;

	if (pd->success_ops) {
		bitmap_free(pd->success_ops);
	}
	if (pd->failure_ops) {
		bitmap_free(pd->failure_ops);
	}
	SAFE_FREE(pd);
	*p_data = NULL;
}

/* Implementation of vfs_ops.  Pass everything on to the default
   operation but log event first. */

static int smb_full_audit_connect(vfs_handle_struct *handle, connection_struct *conn,
			 const char *svc, const char *user)
{
	int result;
	struct vfs_full_audit_private_data *pd = NULL;
	const char *none[] = { NULL };
	const char *all [] = { "all" };

	if (!handle) {
		return -1;
	}

	pd = SMB_MALLOC_P(struct vfs_full_audit_private_data);
	if (!pd) {
		return -1;
	}
	ZERO_STRUCTP(pd);

	openlog("smbd_audit", 0, audit_syslog_facility(handle));

	init_bitmap(&pd->success_ops,
		    lp_parm_string_list(SNUM(conn), "full_audit", "success",
					none));
	init_bitmap(&pd->failure_ops,
		    lp_parm_string_list(SNUM(conn), "full_audit", "failure",
					all));

	/* Store the private data. */
	SMB_VFS_HANDLE_SET_DATA(handle, pd, free_private_data,
				struct vfs_full_audit_private_data, return -1);

	result = SMB_VFS_NEXT_CONNECT(handle, conn, svc, user);

	do_log(SMB_VFS_OP_CONNECT, True, handle,
	       "%s", svc);

	return result;
}

static void smb_full_audit_disconnect(vfs_handle_struct *handle,
			     connection_struct *conn)
{
	SMB_VFS_NEXT_DISCONNECT(handle, conn);

	do_log(SMB_VFS_OP_DISCONNECT, True, handle,
	       "%s", lp_servicename(SNUM(conn)));

	/* The bitmaps will be disconnected when the private
	   data is deleted. */

	return;
}

static SMB_BIG_UINT smb_full_audit_disk_free(vfs_handle_struct *handle,
				    connection_struct *conn, const char *path,
				    BOOL small_query, SMB_BIG_UINT *bsize, 
				    SMB_BIG_UINT *dfree, SMB_BIG_UINT *dsize)
{
	SMB_BIG_UINT result;

	result = SMB_VFS_NEXT_DISK_FREE(handle, conn, path, small_query, bsize,
					dfree, dsize);

	/* Don't have a reasonable notion of failure here */

	do_log(SMB_VFS_OP_DISK_FREE, True, handle, "%s", path);

	return result;
}

static int smb_full_audit_get_quota(struct vfs_handle_struct *handle,
			   struct connection_struct *conn,
			   enum SMB_QUOTA_TYPE qtype, unid_t id,
			   SMB_DISK_QUOTA *qt)
{
	int result;

	result = SMB_VFS_NEXT_GET_QUOTA(handle, conn, qtype, id, qt);

	do_log(SMB_VFS_OP_GET_QUOTA, (result >= 0), handle, "");

	return result;
}

	
static int smb_full_audit_set_quota(struct vfs_handle_struct *handle,
			   struct connection_struct *conn,
			   enum SMB_QUOTA_TYPE qtype, unid_t id,
			   SMB_DISK_QUOTA *qt)
{
	int result;

	result = SMB_VFS_NEXT_SET_QUOTA(handle, conn, qtype, id, qt);

	do_log(SMB_VFS_OP_SET_QUOTA, (result >= 0), handle, "");

	return result;
}

static int smb_full_audit_get_shadow_copy_data(struct vfs_handle_struct *handle,
				struct files_struct *fsp,
				SHADOW_COPY_DATA *shadow_copy_data, BOOL labels)
{
	int result;

	result = SMB_VFS_NEXT_GET_SHADOW_COPY_DATA(handle, fsp, shadow_copy_data, labels);

	do_log(SMB_VFS_OP_GET_SHADOW_COPY_DATA, (result >= 0), handle, "");

	return result;
}

static int smb_full_audit_statvfs(struct vfs_handle_struct *handle,
				struct connection_struct *conn,
				const char *path,
				struct vfs_statvfs_struct *statbuf)
{
	int result;

	result = SMB_VFS_NEXT_STATVFS(handle, conn, path, statbuf);

	do_log(SMB_VFS_OP_STATVFS, (result >= 0), handle, "");

	return result;
}

static SMB_STRUCT_DIR *smb_full_audit_opendir(vfs_handle_struct *handle, connection_struct *conn,
			  const char *fname, const char *mask, uint32 attr)
{
	SMB_STRUCT_DIR *result;

	result = SMB_VFS_NEXT_OPENDIR(handle, conn, fname, mask, attr);

	do_log(SMB_VFS_OP_OPENDIR, (result != NULL), handle, "%s", fname);

	return result;
}

static SMB_STRUCT_DIRENT *smb_full_audit_readdir(vfs_handle_struct *handle,
				    connection_struct *conn, SMB_STRUCT_DIR *dirp)
{
	SMB_STRUCT_DIRENT *result;

	result = SMB_VFS_NEXT_READDIR(handle, conn, dirp);

	/* This operation has no reasonable error condition
	 * (End of dir is also failure), so always succeed.
	 */
	do_log(SMB_VFS_OP_READDIR, True, handle, "");

	return result;
}

static void smb_full_audit_seekdir(vfs_handle_struct *handle, connection_struct *conn,
			SMB_STRUCT_DIR *dirp, long offset)
{
	SMB_VFS_NEXT_SEEKDIR(handle, conn, dirp, offset);

	do_log(SMB_VFS_OP_SEEKDIR, True, handle, "");
	return;
}

static long smb_full_audit_telldir(vfs_handle_struct *handle, connection_struct *conn,
			SMB_STRUCT_DIR *dirp)
{
	long result;

	result = SMB_VFS_NEXT_TELLDIR(handle, conn, dirp);

	do_log(SMB_VFS_OP_TELLDIR, True, handle, "");

	return result;
}

static void smb_full_audit_rewinddir(vfs_handle_struct *handle, connection_struct *conn,
			SMB_STRUCT_DIR *dirp)
{
	SMB_VFS_NEXT_REWINDDIR(handle, conn, dirp);

	do_log(SMB_VFS_OP_REWINDDIR, True, handle, "");
	return;
}

static int smb_full_audit_mkdir(vfs_handle_struct *handle, connection_struct *conn,
		       const char *path, mode_t mode)
{
	int result;
	
	result = SMB_VFS_NEXT_MKDIR(handle, conn, path, mode);
	
	do_log(SMB_VFS_OP_MKDIR, (result >= 0), handle, "%s", path);

	return result;
}

static int smb_full_audit_rmdir(vfs_handle_struct *handle, connection_struct *conn,
		       const char *path)
{
	int result;
	
	result = SMB_VFS_NEXT_RMDIR(handle, conn, path);

	do_log(SMB_VFS_OP_RMDIR, (result >= 0), handle, "%s", path);

	return result;
}

static int smb_full_audit_closedir(vfs_handle_struct *handle, connection_struct *conn,
			  SMB_STRUCT_DIR *dirp)
{
	int result;

	result = SMB_VFS_NEXT_CLOSEDIR(handle, conn, dirp);
	
	do_log(SMB_VFS_OP_CLOSEDIR, (result >= 0), handle, "");

	return result;
}

static int smb_full_audit_open(vfs_handle_struct *handle, connection_struct *conn,
		      const char *fname, int flags, mode_t mode)
{
	int result;
	
	result = SMB_VFS_NEXT_OPEN(handle, conn, fname, flags, mode);

	do_log(SMB_VFS_OP_OPEN, (result >= 0), handle, "%s|%s",
	       ((flags & O_WRONLY) || (flags & O_RDWR))?"w":"r",
	       fname);

	return result;
}

static int smb_full_audit_close(vfs_handle_struct *handle, files_struct *fsp, int fd)
{
	int result;
	
	result = SMB_VFS_NEXT_CLOSE(handle, fsp, fd);

	do_log(SMB_VFS_OP_CLOSE, (result >= 0), handle, "%s", fsp->fsp_name);

	return result;
}

static ssize_t smb_full_audit_read(vfs_handle_struct *handle, files_struct *fsp,
			  int fd, void *data, size_t n)
{
	ssize_t result;

	result = SMB_VFS_NEXT_READ(handle, fsp, fd, data, n);

	do_log(SMB_VFS_OP_READ, (result >= 0), handle, "%s", fsp->fsp_name);

	return result;
}

static ssize_t smb_full_audit_pread(vfs_handle_struct *handle, files_struct *fsp,
			   int fd, void *data, size_t n, SMB_OFF_T offset)
{
	ssize_t result;

	result = SMB_VFS_NEXT_PREAD(handle, fsp, fd, data, n, offset);

	do_log(SMB_VFS_OP_PREAD, (result >= 0), handle, "%s", fsp->fsp_name);

	return result;
}

static ssize_t smb_full_audit_write(vfs_handle_struct *handle, files_struct *fsp,
			   int fd, const void *data, size_t n)
{
	ssize_t result;

	result = SMB_VFS_NEXT_WRITE(handle, fsp, fd, data, n);

	do_log(SMB_VFS_OP_WRITE, (result >= 0), handle, "%s", fsp->fsp_name);

	return result;
}

static ssize_t smb_full_audit_pwrite(vfs_handle_struct *handle, files_struct *fsp,
			    int fd, const void *data, size_t n,
			    SMB_OFF_T offset)
{
	ssize_t result;

	result = SMB_VFS_NEXT_PWRITE(handle, fsp, fd, data, n, offset);

	do_log(SMB_VFS_OP_PWRITE, (result >= 0), handle, "%s", fsp->fsp_name);

	return result;
}

static SMB_OFF_T smb_full_audit_lseek(vfs_handle_struct *handle, files_struct *fsp,
			     int filedes, SMB_OFF_T offset, int whence)
{
	ssize_t result;

	result = SMB_VFS_NEXT_LSEEK(handle, fsp, filedes, offset, whence);

	do_log(SMB_VFS_OP_LSEEK, (result != (ssize_t)-1), handle,
	       "%s", fsp->fsp_name);

	return result;
}

static ssize_t smb_full_audit_sendfile(vfs_handle_struct *handle, int tofd,
			      files_struct *fsp, int fromfd,
			      const DATA_BLOB *hdr, SMB_OFF_T offset,
			      size_t n)
{
	ssize_t result;

	result = SMB_VFS_NEXT_SENDFILE(handle, tofd, fsp, fromfd, hdr,
				       offset, n);

	do_log(SMB_VFS_OP_SENDFILE, (result >= 0), handle,
	       "%s", fsp->fsp_name);

	return result;
}

static int smb_full_audit_rename(vfs_handle_struct *handle, connection_struct *conn,
			const char *oldname, const char *newname)
{
	int result;
	
	result = SMB_VFS_NEXT_RENAME(handle, conn, oldname, newname);

	do_log(SMB_VFS_OP_RENAME, (result >= 0), handle, "%s|%s", oldname, newname);

	return result;    
}

static int smb_full_audit_fsync(vfs_handle_struct *handle, files_struct *fsp, int fd)
{
	int result;
	
	result = SMB_VFS_NEXT_FSYNC(handle, fsp, fd);

	do_log(SMB_VFS_OP_FSYNC, (result >= 0), handle, "%s", fsp->fsp_name);

	return result;    
}

static int smb_full_audit_stat(vfs_handle_struct *handle, connection_struct *conn,
		      const char *fname, SMB_STRUCT_STAT *sbuf)
{
	int result;
	
	result = SMB_VFS_NEXT_STAT(handle, conn, fname, sbuf);

	do_log(SMB_VFS_OP_STAT, (result >= 0), handle, "%s", fname);

	return result;    
}

static int smb_full_audit_fstat(vfs_handle_struct *handle, files_struct *fsp, int fd,
		       SMB_STRUCT_STAT *sbuf)
{
	int result;
	
	result = SMB_VFS_NEXT_FSTAT(handle, fsp, fd, sbuf);

	do_log(SMB_VFS_OP_FSTAT, (result >= 0), handle, "%s", fsp->fsp_name);

	return result;
}

static int smb_full_audit_lstat(vfs_handle_struct *handle, connection_struct *conn,
		       const char *path, SMB_STRUCT_STAT *sbuf)
{
	int result;
	
	result = SMB_VFS_NEXT_LSTAT(handle, conn, path, sbuf);

	do_log(SMB_VFS_OP_LSTAT, (result >= 0), handle, "%s", path);

	return result;    
}

static int smb_full_audit_unlink(vfs_handle_struct *handle, connection_struct *conn,
			const char *path)
{
	int result;
	
	result = SMB_VFS_NEXT_UNLINK(handle, conn, path);

	do_log(SMB_VFS_OP_UNLINK, (result >= 0), handle, "%s", path);

	return result;
}

static int smb_full_audit_chmod(vfs_handle_struct *handle, connection_struct *conn,
		       const char *path, mode_t mode)
{
	int result;

	result = SMB_VFS_NEXT_CHMOD(handle, conn, path, mode);

	do_log(SMB_VFS_OP_CHMOD, (result >= 0), handle, "%s|%o", path, mode);

	return result;
}

static int smb_full_audit_fchmod(vfs_handle_struct *handle, files_struct *fsp, int fd,
			mode_t mode)
{
	int result;
	
	result = SMB_VFS_NEXT_FCHMOD(handle, fsp, fd, mode);

	do_log(SMB_VFS_OP_FCHMOD, (result >= 0), handle,
	       "%s|%o", fsp->fsp_name, mode);

	return result;
}

static int smb_full_audit_chown(vfs_handle_struct *handle, connection_struct *conn,
		       const char *path, uid_t uid, gid_t gid)
{
	int result;

	result = SMB_VFS_NEXT_CHOWN(handle, conn, path, uid, gid);

	do_log(SMB_VFS_OP_CHOWN, (result >= 0), handle, "%s|%ld|%ld",
	       path, (long int)uid, (long int)gid);

	return result;
}

static int smb_full_audit_fchown(vfs_handle_struct *handle, files_struct *fsp, int fd,
			uid_t uid, gid_t gid)
{
	int result;

	result = SMB_VFS_NEXT_FCHOWN(handle, fsp, fd, uid, gid);

	do_log(SMB_VFS_OP_FCHOWN, (result >= 0), handle, "%s|%ld|%ld",
	       fsp->fsp_name, (long int)uid, (long int)gid);

	return result;
}

static int smb_full_audit_chdir(vfs_handle_struct *handle, connection_struct *conn,
		       const char *path)
{
	int result;

	result = SMB_VFS_NEXT_CHDIR(handle, conn, path);

	do_log(SMB_VFS_OP_CHDIR, (result >= 0), handle, "chdir|%s", path);

	return result;
}

static char *smb_full_audit_getwd(vfs_handle_struct *handle, connection_struct *conn,
			 char *path)
{
	char *result;

	result = SMB_VFS_NEXT_GETWD(handle, conn, path);
	
	do_log(SMB_VFS_OP_GETWD, (result != NULL), handle, "%s", path);

	return result;
}

static int smb_full_audit_utime(vfs_handle_struct *handle, connection_struct *conn,
		       const char *path, struct utimbuf *times)
{
	int result;

	result = SMB_VFS_NEXT_UTIME(handle, conn, path, times);

	do_log(SMB_VFS_OP_UTIME, (result >= 0), handle, "%s", path);

	return result;
}

static int smb_full_audit_ftruncate(vfs_handle_struct *handle, files_struct *fsp,
			   int fd, SMB_OFF_T len)
{
	int result;

	result = SMB_VFS_NEXT_FTRUNCATE(handle, fsp, fd, len);

	do_log(SMB_VFS_OP_FTRUNCATE, (result >= 0), handle,
	       "%s", fsp->fsp_name);

	return result;
}

static BOOL smb_full_audit_lock(vfs_handle_struct *handle, files_struct *fsp, int fd,
		       int op, SMB_OFF_T offset, SMB_OFF_T count, int type)
{
	BOOL result;

	result = SMB_VFS_NEXT_LOCK(handle, fsp, fd, op, offset, count, type);

	do_log(SMB_VFS_OP_LOCK, (result >= 0), handle, "%s", fsp->fsp_name);

	return result;
}

static BOOL smb_full_audit_getlock(vfs_handle_struct *handle, files_struct *fsp, int fd,
		       SMB_OFF_T *poffset, SMB_OFF_T *pcount, int *ptype, pid_t *ppid)
{
	BOOL result;

	result = SMB_VFS_NEXT_GETLOCK(handle, fsp, fd, poffset, pcount, ptype, ppid);

	do_log(SMB_VFS_OP_GETLOCK, (result >= 0), handle, "%s", fsp->fsp_name);

	return result;
}

static int smb_full_audit_symlink(vfs_handle_struct *handle, connection_struct *conn,
			 const char *oldpath, const char *newpath)
{
	int result;

	result = SMB_VFS_NEXT_SYMLINK(handle, conn, oldpath, newpath);

	do_log(SMB_VFS_OP_SYMLINK, (result >= 0), handle,
	       "%s|%s", oldpath, newpath);

	return result;
}

static int smb_full_audit_readlink(vfs_handle_struct *handle, connection_struct *conn,
			  const char *path, char *buf, size_t bufsiz)
{
	int result;

	result = SMB_VFS_NEXT_READLINK(handle, conn, path, buf, bufsiz);

	do_log(SMB_VFS_OP_READLINK, (result >= 0), handle, "%s", path);

	return result;
}

static int smb_full_audit_link(vfs_handle_struct *handle, connection_struct *conn,
		      const char *oldpath, const char *newpath)
{
	int result;

	result = SMB_VFS_NEXT_LINK(handle, conn, oldpath, newpath);

	do_log(SMB_VFS_OP_LINK, (result >= 0), handle,
	       "%s|%s", oldpath, newpath);

	return result;
}

static int smb_full_audit_mknod(vfs_handle_struct *handle, connection_struct *conn,
		       const char *pathname, mode_t mode, SMB_DEV_T dev)
{
	int result;

	result = SMB_VFS_NEXT_MKNOD(handle, conn, pathname, mode, dev);

	do_log(SMB_VFS_OP_MKNOD, (result >= 0), handle, "%s", pathname);

	return result;
}

static char *smb_full_audit_realpath(vfs_handle_struct *handle, connection_struct *conn,
			    const char *path, char *resolved_path)
{
	char *result;

	result = SMB_VFS_NEXT_REALPATH(handle, conn, path, resolved_path);

	do_log(SMB_VFS_OP_REALPATH, (result != NULL), handle, "%s", path);

	return result;
}

static size_t smb_full_audit_fget_nt_acl(vfs_handle_struct *handle, files_struct *fsp,
				int fd, uint32 security_info,
				SEC_DESC **ppdesc)
{
	size_t result;

	result = SMB_VFS_NEXT_FGET_NT_ACL(handle, fsp, fd, security_info,
					  ppdesc);

	do_log(SMB_VFS_OP_FGET_NT_ACL, (result > 0), handle,
	       "%s", fsp->fsp_name);

	return result;
}

static size_t smb_full_audit_get_nt_acl(vfs_handle_struct *handle, files_struct *fsp,
			       const char *name, uint32 security_info,
			       SEC_DESC **ppdesc)
{
	size_t result;

	result = SMB_VFS_NEXT_GET_NT_ACL(handle, fsp, name, security_info,
					 ppdesc);

	do_log(SMB_VFS_OP_GET_NT_ACL, (result > 0), handle,
	       "%s", fsp->fsp_name);

	return result;
}

static BOOL smb_full_audit_fset_nt_acl(vfs_handle_struct *handle, files_struct *fsp,
			      int fd, uint32 security_info_sent,
			      SEC_DESC *psd)
{
	BOOL result;

	result = SMB_VFS_NEXT_FSET_NT_ACL(handle, fsp, fd, security_info_sent,
					  psd);

	do_log(SMB_VFS_OP_FSET_NT_ACL, result, handle, "%s", fsp->fsp_name);

	return result;
}

static BOOL smb_full_audit_set_nt_acl(vfs_handle_struct *handle, files_struct *fsp,
			     const char *name, uint32 security_info_sent,
			     SEC_DESC *psd)
{
	BOOL result;

	result = SMB_VFS_NEXT_SET_NT_ACL(handle, fsp, name, security_info_sent,
					 psd);

	do_log(SMB_VFS_OP_SET_NT_ACL, result, handle, "%s", fsp->fsp_name);

	return result;
}

static int smb_full_audit_chmod_acl(vfs_handle_struct *handle, connection_struct *conn,
			   const char *path, mode_t mode)
{
	int result;
	
	result = SMB_VFS_NEXT_CHMOD_ACL(handle, conn, path, mode);

	do_log(SMB_VFS_OP_CHMOD_ACL, (result >= 0), handle,
	       "%s|%o", path, mode);

	return result;
}

static int smb_full_audit_fchmod_acl(vfs_handle_struct *handle, files_struct *fsp,
			    int fd, mode_t mode)
{
	int result;
	
	result = SMB_VFS_NEXT_FCHMOD_ACL(handle, fsp, fd, mode);

	do_log(SMB_VFS_OP_FCHMOD_ACL, (result >= 0), handle,
	       "%s|%o", fsp->fsp_name, mode);

	return result;
}

static int smb_full_audit_sys_acl_get_entry(vfs_handle_struct *handle,
				   connection_struct *conn,
				   SMB_ACL_T theacl, int entry_id,
				   SMB_ACL_ENTRY_T *entry_p)
{
	int result;

	result = SMB_VFS_NEXT_SYS_ACL_GET_ENTRY(handle, conn, theacl, entry_id,
						entry_p);

	do_log(SMB_VFS_OP_SYS_ACL_GET_ENTRY, (result >= 0), handle,
	       "");

	return result;
}

static int smb_full_audit_sys_acl_get_tag_type(vfs_handle_struct *handle,
				      connection_struct *conn,
				      SMB_ACL_ENTRY_T entry_d,
				      SMB_ACL_TAG_T *tag_type_p)
{
	int result;

	result = SMB_VFS_NEXT_SYS_ACL_GET_TAG_TYPE(handle, conn, entry_d,
						   tag_type_p);

	do_log(SMB_VFS_OP_SYS_ACL_GET_TAG_TYPE, (result >= 0), handle,
	       "");

	return result;
}

static int smb_full_audit_sys_acl_get_permset(vfs_handle_struct *handle,
				     connection_struct *conn,
				     SMB_ACL_ENTRY_T entry_d,
				     SMB_ACL_PERMSET_T *permset_p)
{
	int result;

	result = SMB_VFS_NEXT_SYS_ACL_GET_PERMSET(handle, conn, entry_d,
						  permset_p);

	do_log(SMB_VFS_OP_SYS_ACL_GET_PERMSET, (result >= 0), handle,
	       "");

	return result;
}

static void * smb_full_audit_sys_acl_get_qualifier(vfs_handle_struct *handle,
					  connection_struct *conn,
					  SMB_ACL_ENTRY_T entry_d)
{
	void *result;

	result = SMB_VFS_NEXT_SYS_ACL_GET_QUALIFIER(handle, conn, entry_d);

	do_log(SMB_VFS_OP_SYS_ACL_GET_QUALIFIER, (result != NULL), handle,
	       "");

	return result;
}

static SMB_ACL_T smb_full_audit_sys_acl_get_file(vfs_handle_struct *handle,
					connection_struct *conn,
					const char *path_p,
					SMB_ACL_TYPE_T type)
{
	SMB_ACL_T result;

	result = SMB_VFS_NEXT_SYS_ACL_GET_FILE(handle, conn, path_p, type);

	do_log(SMB_VFS_OP_SYS_ACL_GET_FILE, (result != NULL), handle,
	       "%s", path_p);

	return result;
}

static SMB_ACL_T smb_full_audit_sys_acl_get_fd(vfs_handle_struct *handle,
				      files_struct *fsp, int fd)
{
	SMB_ACL_T result;

	result = SMB_VFS_NEXT_SYS_ACL_GET_FD(handle, fsp, fd);

	do_log(SMB_VFS_OP_SYS_ACL_GET_FD, (result != NULL), handle,
	       "%s", fsp->fsp_name);

	return result;
}

static int smb_full_audit_sys_acl_clear_perms(vfs_handle_struct *handle,
				     connection_struct *conn,
				     SMB_ACL_PERMSET_T permset)
{
	int result;

	result = SMB_VFS_NEXT_SYS_ACL_CLEAR_PERMS(handle, conn, permset);

	do_log(SMB_VFS_OP_SYS_ACL_CLEAR_PERMS, (result >= 0), handle,
	       "");

	return result;
}

static int smb_full_audit_sys_acl_add_perm(vfs_handle_struct *handle,
				  connection_struct *conn,
				  SMB_ACL_PERMSET_T permset,
				  SMB_ACL_PERM_T perm)
{
	int result;

	result = SMB_VFS_NEXT_SYS_ACL_ADD_PERM(handle, conn, permset, perm);

	do_log(SMB_VFS_OP_SYS_ACL_ADD_PERM, (result >= 0), handle,
	       "");

	return result;
}

static char * smb_full_audit_sys_acl_to_text(vfs_handle_struct *handle,
				    connection_struct *conn, SMB_ACL_T theacl,
				    ssize_t *plen)
{
	char * result;

	result = SMB_VFS_NEXT_SYS_ACL_TO_TEXT(handle, conn, theacl, plen);

	do_log(SMB_VFS_OP_SYS_ACL_TO_TEXT, (result != NULL), handle,
	       "");

	return result;
}

static SMB_ACL_T smb_full_audit_sys_acl_init(vfs_handle_struct *handle,
				    connection_struct *conn,
				    int count)
{
	SMB_ACL_T result;

	result = SMB_VFS_NEXT_SYS_ACL_INIT(handle, conn, count);

	do_log(SMB_VFS_OP_SYS_ACL_INIT, (result != NULL), handle,
	       "");

	return result;
}

static int smb_full_audit_sys_acl_create_entry(vfs_handle_struct *handle,
				      connection_struct *conn, SMB_ACL_T *pacl,
				      SMB_ACL_ENTRY_T *pentry)
{
	int result;

	result = SMB_VFS_NEXT_SYS_ACL_CREATE_ENTRY(handle, conn, pacl, pentry);

	do_log(SMB_VFS_OP_SYS_ACL_CREATE_ENTRY, (result >= 0), handle,
	       "");

	return result;
}

static int smb_full_audit_sys_acl_set_tag_type(vfs_handle_struct *handle,
				      connection_struct *conn,
				      SMB_ACL_ENTRY_T entry,
				      SMB_ACL_TAG_T tagtype)
{
	int result;

	result = SMB_VFS_NEXT_SYS_ACL_SET_TAG_TYPE(handle, conn, entry,
						   tagtype);

	do_log(SMB_VFS_OP_SYS_ACL_SET_TAG_TYPE, (result >= 0), handle,
	       "");

	return result;
}

static int smb_full_audit_sys_acl_set_qualifier(vfs_handle_struct *handle,
				       connection_struct *conn,
				       SMB_ACL_ENTRY_T entry,
				       void *qual)
{
	int result;

	result = SMB_VFS_NEXT_SYS_ACL_SET_QUALIFIER(handle, conn, entry, qual);

	do_log(SMB_VFS_OP_SYS_ACL_SET_QUALIFIER, (result >= 0), handle,
	       "");

	return result;
}

static int smb_full_audit_sys_acl_set_permset(vfs_handle_struct *handle,
				     connection_struct *conn,
				     SMB_ACL_ENTRY_T entry,
				     SMB_ACL_PERMSET_T permset)
{
	int result;

	result = SMB_VFS_NEXT_SYS_ACL_SET_PERMSET(handle, conn, entry, permset);

	do_log(SMB_VFS_OP_SYS_ACL_SET_PERMSET, (result >= 0), handle,
	       "");

	return result;
}

static int smb_full_audit_sys_acl_valid(vfs_handle_struct *handle,
			       connection_struct *conn,
			       SMB_ACL_T theacl )
{
	int result;

	result = SMB_VFS_NEXT_SYS_ACL_VALID(handle, conn, theacl);

	do_log(SMB_VFS_OP_SYS_ACL_VALID, (result >= 0), handle,
	       "");

	return result;
}

static int smb_full_audit_sys_acl_set_file(vfs_handle_struct *handle,
				  connection_struct *conn,
				  const char *name, SMB_ACL_TYPE_T acltype,
				  SMB_ACL_T theacl)
{
	int result;

	result = SMB_VFS_NEXT_SYS_ACL_SET_FILE(handle, conn, name, acltype,
					       theacl);

	do_log(SMB_VFS_OP_SYS_ACL_SET_FILE, (result >= 0), handle,
	       "%s", name);

	return result;
}

static int smb_full_audit_sys_acl_set_fd(vfs_handle_struct *handle, files_struct *fsp,
				int fd, SMB_ACL_T theacl)
{
	int result;

	result = SMB_VFS_NEXT_SYS_ACL_SET_FD(handle, fsp, fd, theacl);

	do_log(SMB_VFS_OP_SYS_ACL_SET_FD, (result >= 0), handle,
	       "%s", fsp->fsp_name);

	return result;
}

static int smb_full_audit_sys_acl_delete_def_file(vfs_handle_struct *handle,
					 connection_struct *conn,
					 const char *path)
{
	int result;

	result = SMB_VFS_NEXT_SYS_ACL_DELETE_DEF_FILE(handle, conn, path);

	do_log(SMB_VFS_OP_SYS_ACL_DELETE_DEF_FILE, (result >= 0), handle,
	       "%s", path);

	return result;
}

static int smb_full_audit_sys_acl_get_perm(vfs_handle_struct *handle,
				  connection_struct *conn,
				  SMB_ACL_PERMSET_T permset,
				  SMB_ACL_PERM_T perm)
{
	int result;

	result = SMB_VFS_NEXT_SYS_ACL_GET_PERM(handle, conn, permset, perm);

	do_log(SMB_VFS_OP_SYS_ACL_GET_PERM, (result >= 0), handle,
	       "");

	return result;
}

static int smb_full_audit_sys_acl_free_text(vfs_handle_struct *handle,
				   connection_struct *conn,
				   char *text)
{
	int result;

	result = SMB_VFS_NEXT_SYS_ACL_FREE_TEXT(handle, conn, text);

	do_log(SMB_VFS_OP_SYS_ACL_FREE_TEXT, (result >= 0), handle,
	       "");

	return result;
}

static int smb_full_audit_sys_acl_free_acl(vfs_handle_struct *handle,
				  connection_struct *conn,
				  SMB_ACL_T posix_acl)
{
	int result;

	result = SMB_VFS_NEXT_SYS_ACL_FREE_ACL(handle, conn, posix_acl);

	do_log(SMB_VFS_OP_SYS_ACL_FREE_ACL, (result >= 0), handle,
	       "");

	return result;
}

static int smb_full_audit_sys_acl_free_qualifier(vfs_handle_struct *handle,
					connection_struct *conn,
					void *qualifier,
					SMB_ACL_TAG_T tagtype)
{
	int result;

	result = SMB_VFS_NEXT_SYS_ACL_FREE_QUALIFIER(handle, conn, qualifier,
						     tagtype);

	do_log(SMB_VFS_OP_SYS_ACL_FREE_QUALIFIER, (result >= 0), handle,
	       "");

	return result;
}

static ssize_t smb_full_audit_getxattr(struct vfs_handle_struct *handle,
			      struct connection_struct *conn, const char *path,
			      const char *name, void *value, size_t size)
{
	ssize_t result;

	result = SMB_VFS_NEXT_GETXATTR(handle, conn, path, name, value, size);

	do_log(SMB_VFS_OP_GETXATTR, (result >= 0), handle,
	       "%s|%s", path, name);

	return result;
}

static ssize_t smb_full_audit_lgetxattr(struct vfs_handle_struct *handle,
			       struct connection_struct *conn,
			       const char *path, const char *name,
			       void *value, size_t size)
{
	ssize_t result;

	result = SMB_VFS_NEXT_LGETXATTR(handle, conn, path, name, value, size);

	do_log(SMB_VFS_OP_LGETXATTR, (result >= 0), handle,
	       "%s|%s", path, name);

	return result;
}

static ssize_t smb_full_audit_fgetxattr(struct vfs_handle_struct *handle,
			       struct files_struct *fsp, int fd,
			       const char *name, void *value, size_t size)
{
	ssize_t result;

	result = SMB_VFS_NEXT_FGETXATTR(handle, fsp, fd, name, value, size);

	do_log(SMB_VFS_OP_FGETXATTR, (result >= 0), handle,
	       "%s|%s", fsp->fsp_name, name);

	return result;
}

static ssize_t smb_full_audit_listxattr(struct vfs_handle_struct *handle,
			       struct connection_struct *conn,
			       const char *path, char *list, size_t size)
{
	ssize_t result;

	result = SMB_VFS_NEXT_LISTXATTR(handle, conn, path, list, size);

	do_log(SMB_VFS_OP_LISTXATTR, (result >= 0), handle, "%s", path);

	return result;
}

static ssize_t smb_full_audit_llistxattr(struct vfs_handle_struct *handle,
				struct connection_struct *conn,
				const char *path, char *list, size_t size)
{
	ssize_t result;

	result = SMB_VFS_NEXT_LLISTXATTR(handle, conn, path, list, size);

	do_log(SMB_VFS_OP_LLISTXATTR, (result >= 0), handle, "%s", path);

	return result;
}

static ssize_t smb_full_audit_flistxattr(struct vfs_handle_struct *handle,
				struct files_struct *fsp, int fd, char *list,
				size_t size)
{
	ssize_t result;

	result = SMB_VFS_NEXT_FLISTXATTR(handle, fsp, fd, list, size);

	do_log(SMB_VFS_OP_FLISTXATTR, (result >= 0), handle,
	       "%s", fsp->fsp_name);

	return result;
}

static int smb_full_audit_removexattr(struct vfs_handle_struct *handle,
			     struct connection_struct *conn, const char *path,
			     const char *name)
{
	int result;

	result = SMB_VFS_NEXT_REMOVEXATTR(handle, conn, path, name);

	do_log(SMB_VFS_OP_REMOVEXATTR, (result >= 0), handle,
	       "%s|%s", path, name);

	return result;
}

static int smb_full_audit_lremovexattr(struct vfs_handle_struct *handle,
			      struct connection_struct *conn, const char *path,
			      const char *name)
{
	int result;

	result = SMB_VFS_NEXT_LREMOVEXATTR(handle, conn, path, name);

	do_log(SMB_VFS_OP_LREMOVEXATTR, (result >= 0), handle,
	       "%s|%s", path, name);

	return result;
}

static int smb_full_audit_fremovexattr(struct vfs_handle_struct *handle,
			      struct files_struct *fsp, int fd,
			      const char *name)
{
	int result;

	result = SMB_VFS_NEXT_FREMOVEXATTR(handle, fsp, fd, name);

	do_log(SMB_VFS_OP_FREMOVEXATTR, (result >= 0), handle,
	       "%s|%s", fsp->fsp_name, name);

	return result;
}

static int smb_full_audit_setxattr(struct vfs_handle_struct *handle,
			  struct connection_struct *conn, const char *path,
			  const char *name, const void *value, size_t size,
			  int flags)
{
	int result;

	result = SMB_VFS_NEXT_SETXATTR(handle, conn, path, name, value, size,
				       flags);

	do_log(SMB_VFS_OP_SETXATTR, (result >= 0), handle,
	       "%s|%s", path, name);

	return result;
}

static int smb_full_audit_lsetxattr(struct vfs_handle_struct *handle,
			   struct connection_struct *conn, const char *path,
			   const char *name, const void *value, size_t size,
			   int flags)
{
	int result;

	result = SMB_VFS_NEXT_LSETXATTR(handle, conn, path, name, value, size,
					flags);

	do_log(SMB_VFS_OP_LSETXATTR, (result >= 0), handle,
	       "%s|%s", path, name);

	return result;
}

static int smb_full_audit_fsetxattr(struct vfs_handle_struct *handle,
			   struct files_struct *fsp, int fd, const char *name,
			   const void *value, size_t size, int flags)
{
	int result;

	result = SMB_VFS_NEXT_FSETXATTR(handle, fsp, fd, name, value, size,
					flags);

	do_log(SMB_VFS_OP_FSETXATTR, (result >= 0), handle,
	       "%s|%s", fsp->fsp_name, name);

	return result;
}

static int smb_full_audit_aio_read(struct vfs_handle_struct *handle, struct files_struct *fsp, SMB_STRUCT_AIOCB *aiocb)
{
	int result;

	result = SMB_VFS_NEXT_AIO_READ(handle, fsp, aiocb);
	do_log(SMB_VFS_OP_AIO_READ, (result >= 0), handle,
		"%s", fsp->fsp_name);

	return result;
}

static int smb_full_audit_aio_write(struct vfs_handle_struct *handle, struct files_struct *fsp, SMB_STRUCT_AIOCB *aiocb)
{
	int result;

	result = SMB_VFS_NEXT_AIO_WRITE(handle, fsp, aiocb);
	do_log(SMB_VFS_OP_AIO_WRITE, (result >= 0), handle,
		"%s", fsp->fsp_name);

	return result;
}

static ssize_t smb_full_audit_aio_return(struct vfs_handle_struct *handle, struct files_struct *fsp, SMB_STRUCT_AIOCB *aiocb)
{
	int result;

	result = SMB_VFS_NEXT_AIO_RETURN(handle, fsp, aiocb);
	do_log(SMB_VFS_OP_AIO_RETURN, (result >= 0), handle,
		"%s", fsp->fsp_name);

	return result;
}

static int smb_full_audit_aio_cancel(struct vfs_handle_struct *handle, struct files_struct *fsp, int fd, SMB_STRUCT_AIOCB *aiocb)
{
	int result;

	result = SMB_VFS_NEXT_AIO_CANCEL(handle, fsp, fd, aiocb);
	do_log(SMB_VFS_OP_AIO_CANCEL, (result >= 0), handle,
		"%s", fsp->fsp_name);

	return result;
}

static int smb_full_audit_aio_error(struct vfs_handle_struct *handle, struct files_struct *fsp, SMB_STRUCT_AIOCB *aiocb)
{
	int result;

	result = SMB_VFS_NEXT_AIO_ERROR(handle, fsp, aiocb);
	do_log(SMB_VFS_OP_AIO_ERROR, (result >= 0), handle,
		"%s", fsp->fsp_name);

	return result;
}

static int smb_full_audit_aio_fsync(struct vfs_handle_struct *handle, struct files_struct *fsp, int op, SMB_STRUCT_AIOCB *aiocb)
{
	int result;

	result = SMB_VFS_NEXT_AIO_FSYNC(handle, fsp, op, aiocb);
	do_log(SMB_VFS_OP_AIO_FSYNC, (result >= 0), handle,
		"%s", fsp->fsp_name);

	return result;
}

static int smb_full_audit_aio_suspend(struct vfs_handle_struct *handle, struct files_struct *fsp, const SMB_STRUCT_AIOCB * const aiocb[], int n, const struct timespec *ts)
{
	int result;

	result = SMB_VFS_NEXT_AIO_SUSPEND(handle, fsp, aiocb, n, ts);
	do_log(SMB_VFS_OP_AIO_SUSPEND, (result >= 0), handle,
		"%s", fsp->fsp_name);

	return result;
}


NTSTATUS vfs_full_audit_init(void)
{
	NTSTATUS ret = smb_register_vfs(SMB_VFS_INTERFACE_VERSION,
					"full_audit", audit_op_tuples);
	
	if (!NT_STATUS_IS_OK(ret))
		return ret;

	vfs_full_audit_debug_level = debug_add_class("full_audit");
	if (vfs_full_audit_debug_level == -1) {
		vfs_full_audit_debug_level = DBGC_VFS;
		DEBUG(0, ("vfs_full_audit: Couldn't register custom debugging "
			  "class!\n"));
	} else {
		DEBUG(10, ("vfs_full_audit: Debug class number of "
			   "'full_audit': %d\n", vfs_full_audit_debug_level));
	}
	
	return ret;
}
