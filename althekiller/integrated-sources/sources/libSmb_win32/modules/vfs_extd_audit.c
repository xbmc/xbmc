/* 
 * Auditing VFS module for samba.  Log selected file operations to syslog
 * facility.
 *
 * Copyright (C) Tim Potter, 1999-2000
 * Copyright (C) Alexander Bokovoy, 2002
 * Copyright (C) John H Terpstra, 2003
 * Copyright (C) Stefan (metze) Metzmacher, 2003
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


#include "includes.h"

static int vfs_extd_audit_debug_level = DBGC_VFS;

#undef DBGC_CLASS
#define DBGC_CLASS vfs_extd_audit_debug_level

/* Function prototypes */

static int audit_connect(vfs_handle_struct *handle, connection_struct *conn, const char *svc, const char *user);
static void audit_disconnect(vfs_handle_struct *handle, connection_struct *conn);
static SMB_STRUCT_DIR *audit_opendir(vfs_handle_struct *handle, connection_struct *conn, const char *fname, const char *mask, uint32 attr);
static int audit_mkdir(vfs_handle_struct *handle, connection_struct *conn, const char *path, mode_t mode);
static int audit_rmdir(vfs_handle_struct *handle, connection_struct *conn, const char *path);
static int audit_open(vfs_handle_struct *handle, connection_struct *conn, const char *fname, int flags, mode_t mode);
static int audit_close(vfs_handle_struct *handle, files_struct *fsp, int fd);
static int audit_rename(vfs_handle_struct *handle, connection_struct *conn, const char *oldname, const char *newname);
static int audit_unlink(vfs_handle_struct *handle, connection_struct *conn, const char *path);
static int audit_chmod(vfs_handle_struct *handle, connection_struct *conn, const char *path, mode_t mode);
static int audit_chmod_acl(vfs_handle_struct *handle, connection_struct *conn, const char *name, mode_t mode);
static int audit_fchmod(vfs_handle_struct *handle, files_struct *fsp, int fd, mode_t mode);
static int audit_fchmod_acl(vfs_handle_struct *handle, files_struct *fsp, int fd, mode_t mode);

/* VFS operations */

static vfs_op_tuple audit_op_tuples[] = {
    
	/* Disk operations */

	{SMB_VFS_OP(audit_connect), 	SMB_VFS_OP_CONNECT, 	SMB_VFS_LAYER_LOGGER},
	{SMB_VFS_OP(audit_disconnect), 	SMB_VFS_OP_DISCONNECT, 	SMB_VFS_LAYER_LOGGER},

	/* Directory operations */

	{SMB_VFS_OP(audit_opendir), 	SMB_VFS_OP_OPENDIR, 	SMB_VFS_LAYER_LOGGER},
	{SMB_VFS_OP(audit_mkdir), 		SMB_VFS_OP_MKDIR, 	SMB_VFS_LAYER_LOGGER},
	{SMB_VFS_OP(audit_rmdir), 		SMB_VFS_OP_RMDIR, 	SMB_VFS_LAYER_LOGGER},

	/* File operations */

	{SMB_VFS_OP(audit_open), 		SMB_VFS_OP_OPEN, 	SMB_VFS_LAYER_LOGGER},
	{SMB_VFS_OP(audit_close), 		SMB_VFS_OP_CLOSE, 	SMB_VFS_LAYER_LOGGER},
	{SMB_VFS_OP(audit_rename), 		SMB_VFS_OP_RENAME, 	SMB_VFS_LAYER_LOGGER},
	{SMB_VFS_OP(audit_unlink), 		SMB_VFS_OP_UNLINK, 	SMB_VFS_LAYER_LOGGER},
	{SMB_VFS_OP(audit_chmod), 		SMB_VFS_OP_CHMOD, 	SMB_VFS_LAYER_LOGGER},
	{SMB_VFS_OP(audit_fchmod), 		SMB_VFS_OP_FCHMOD, 	SMB_VFS_LAYER_LOGGER},
	{SMB_VFS_OP(audit_chmod_acl), 	SMB_VFS_OP_CHMOD_ACL, 	SMB_VFS_LAYER_LOGGER},
	{SMB_VFS_OP(audit_fchmod_acl), 	SMB_VFS_OP_FCHMOD_ACL, 	SMB_VFS_LAYER_LOGGER},
	
	/* Finish VFS operations definition */
	
	{SMB_VFS_OP(NULL), 			SMB_VFS_OP_NOOP, 	SMB_VFS_LAYER_NOOP}
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

	facility = lp_parm_enum(SNUM(handle->conn), "extd_audit", "facility", enum_log_facilities, LOG_USER);

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

	priority = lp_parm_enum(SNUM(handle->conn), "extd_audit", "priority", enum_log_priorities, LOG_NOTICE);

	return priority;
}

/* Implementation of vfs_ops.  Pass everything on to the default
   operation but log event first. */

static int audit_connect(vfs_handle_struct *handle, connection_struct *conn, const char *svc, const char *user)
{
	int result;

	openlog("smbd_audit", LOG_PID, audit_syslog_facility(handle));

	syslog(audit_syslog_priority(handle), "connect to service %s by user %s\n", 
	       svc, user);
	DEBUG(10, ("Connected to service %s as user %s\n",
	       svc, user));

	result = SMB_VFS_NEXT_CONNECT(handle, conn, svc, user);

	return result;
}

static void audit_disconnect(vfs_handle_struct *handle, connection_struct *conn)
{
	syslog(audit_syslog_priority(handle), "disconnected\n");
	DEBUG(10, ("Disconnected from VFS module extd_audit\n"));
	SMB_VFS_NEXT_DISCONNECT(handle, conn);

	return;
}

static SMB_STRUCT_DIR *audit_opendir(vfs_handle_struct *handle, connection_struct *conn, const char *fname, const char *mask, uint32 attr)
{
	SMB_STRUCT_DIR *result;

	result = SMB_VFS_NEXT_OPENDIR(handle, conn, fname, mask, attr);

	syslog(audit_syslog_priority(handle), "opendir %s %s%s\n",
	       fname,
	       (result == NULL) ? "failed: " : "",
	       (result == NULL) ? strerror(errno) : "");
	DEBUG(1, ("vfs_extd_audit: opendir %s %s %s\n",
	       fname,
	       (result == NULL) ? "failed: " : "",
	       (result == NULL) ? strerror(errno) : ""));

	return result;
}

static int audit_mkdir(vfs_handle_struct *handle, connection_struct *conn, const char *path, mode_t mode)
{
	int result;
	
	result = SMB_VFS_NEXT_MKDIR(handle, conn, path, mode);
	
	syslog(audit_syslog_priority(handle), "mkdir %s %s%s\n", 
	       path,
	       (result < 0) ? "failed: " : "",
	       (result < 0) ? strerror(errno) : "");
	DEBUG(0, ("vfs_extd_audit: mkdir %s %s %s\n",
	       path,
	       (result < 0) ? "failed: " : "",
	       (result < 0) ? strerror(errno) : ""));

	return result;
}

static int audit_rmdir(vfs_handle_struct *handle, connection_struct *conn, const char *path)
{
	int result;
	
	result = SMB_VFS_NEXT_RMDIR(handle, conn, path);

	syslog(audit_syslog_priority(handle), "rmdir %s %s%s\n", 
	       path, 
	       (result < 0) ? "failed: " : "",
	       (result < 0) ? strerror(errno) : "");
	DEBUG(0, ("vfs_extd_audit: rmdir %s %s %s\n",
               path,
	       (result < 0) ? "failed: " : "",
	       (result < 0) ? strerror(errno) : ""));

	return result;
}

static int audit_open(vfs_handle_struct *handle, connection_struct *conn, const char *fname, int flags, mode_t mode)
{
	int result;
	
	result = SMB_VFS_NEXT_OPEN(handle, conn, fname, flags, mode);

	syslog(audit_syslog_priority(handle), "open %s (fd %d) %s%s%s\n", 
	       fname, result,
	       ((flags & O_WRONLY) || (flags & O_RDWR)) ? "for writing " : "", 
	       (result < 0) ? "failed: " : "",
	       (result < 0) ? strerror(errno) : "");
	DEBUG(2, ("vfs_extd_audit: open %s %s %s\n",
	       fname,
	       (result < 0) ? "failed: " : "",
	       (result < 0) ? strerror(errno) : ""));

	return result;
}

static int audit_close(vfs_handle_struct *handle, files_struct *fsp, int fd)
{
	int result;
	
	result = SMB_VFS_NEXT_CLOSE(handle, fsp, fd);

	syslog(audit_syslog_priority(handle), "close fd %d %s%s\n",
	       fd,
	       (result < 0) ? "failed: " : "",
	       (result < 0) ? strerror(errno) : "");
	DEBUG(2, ("vfs_extd_audit: close fd %d %s %s\n",
	       fd,
	       (result < 0) ? "failed: " : "",
	       (result < 0) ? strerror(errno) : ""));

	return result;
}

static int audit_rename(vfs_handle_struct *handle, connection_struct *conn, const char *oldname, const char *newname)
{
	int result;
	
	result = SMB_VFS_NEXT_RENAME(handle, conn, oldname, newname);

	syslog(audit_syslog_priority(handle), "rename %s -> %s %s%s\n",
	       oldname, newname,
	       (result < 0) ? "failed: " : "",
	       (result < 0) ? strerror(errno) : "");
	DEBUG(1, ("vfs_extd_audit: rename old: %s newname: %s  %s %s\n",
	       oldname, newname,
	       (result < 0) ? "failed: " : "",
	       (result < 0) ? strerror(errno) : ""));

	return result;    
}

static int audit_unlink(vfs_handle_struct *handle, connection_struct *conn, const char *path)
{
	int result;
	
	result = SMB_VFS_NEXT_UNLINK(handle, conn, path);

	syslog(audit_syslog_priority(handle), "unlink %s %s%s\n",
	       path,
	       (result < 0) ? "failed: " : "",
	       (result < 0) ? strerror(errno) : "");
	DEBUG(0, ("vfs_extd_audit: unlink %s %s %s\n",
	       path,
	       (result < 0) ? "failed: " : "",
	       (result < 0) ? strerror(errno) : ""));

	return result;
}

static int audit_chmod(vfs_handle_struct *handle, connection_struct *conn, const char *path, mode_t mode)
{
	int result;

	result = SMB_VFS_NEXT_CHMOD(handle, conn, path, mode);

	syslog(audit_syslog_priority(handle), "chmod %s mode 0x%x %s%s\n",
	       path, mode,
	       (result < 0) ? "failed: " : "",
	       (result < 0) ? strerror(errno) : "");
	DEBUG(1, ("vfs_extd_audit: chmod %s mode 0x%x %s %s\n",
	       path, mode,
	       (result < 0) ? "failed: " : "",
	       (result < 0) ? strerror(errno) : ""));

	return result;
}

static int audit_chmod_acl(vfs_handle_struct *handle, connection_struct *conn, const char *path, mode_t mode)
{
	int result;
	
	result = SMB_VFS_NEXT_CHMOD_ACL(handle, conn, path, mode);

	syslog(audit_syslog_priority(handle), "chmod_acl %s mode 0x%x %s%s\n",
	       path, mode,
	       (result < 0) ? "failed: " : "",
	       (result < 0) ? strerror(errno) : "");
	DEBUG(1, ("vfs_extd_audit: chmod_acl %s mode 0x%x %s %s\n",
	        path, mode,
	       (result < 0) ? "failed: " : "",
	       (result < 0) ? strerror(errno) : ""));

	return result;
}

static int audit_fchmod(vfs_handle_struct *handle, files_struct *fsp, int fd, mode_t mode)
{
	int result;
	
	result = SMB_VFS_NEXT_FCHMOD(handle, fsp, fd, mode);

	syslog(audit_syslog_priority(handle), "fchmod %s mode 0x%x %s%s\n",
	       fsp->fsp_name, mode,
	       (result < 0) ? "failed: " : "",
	       (result < 0) ? strerror(errno) : "");
	DEBUG(1, ("vfs_extd_audit: fchmod %s mode 0x%x %s %s",
	       fsp->fsp_name,  mode,
	       (result < 0) ? "failed: " : "",
	       (result < 0) ? strerror(errno) : ""));

	return result;
}

static int audit_fchmod_acl(vfs_handle_struct *handle, files_struct *fsp, int fd, mode_t mode)
{
	int result;
	
	result = SMB_VFS_NEXT_FCHMOD_ACL(handle, fsp, fd, mode);

	syslog(audit_syslog_priority(handle), "fchmod_acl %s mode 0x%x %s%s\n",
	       fsp->fsp_name, mode,
	       (result < 0) ? "failed: " : "",
	       (result < 0) ? strerror(errno) : "");
	DEBUG(1, ("vfs_extd_audit: fchmod_acl %s mode 0x%x %s %s",
	       fsp->fsp_name,  mode,
	       (result < 0) ? "failed: " : "",
	       (result < 0) ? strerror(errno) : ""));

	return result;
}

NTSTATUS vfs_extd_audit_init(void)
{
	NTSTATUS ret = smb_register_vfs(SMB_VFS_INTERFACE_VERSION, "extd_audit", audit_op_tuples);
	
	if (!NT_STATUS_IS_OK(ret))
		return ret;

	vfs_extd_audit_debug_level = debug_add_class("extd_audit");
	if (vfs_extd_audit_debug_level == -1) {
		vfs_extd_audit_debug_level = DBGC_VFS;
		DEBUG(0, ("vfs_extd_audit: Couldn't register custom debugging class!\n"));
	} else {
		DEBUG(10, ("vfs_extd_audit: Debug class number of 'extd_audit': %d\n", vfs_extd_audit_debug_level));
	}
	
	return ret;
}
