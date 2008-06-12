/* 
 * Fake Perms VFS module.  Implements passthrough operation of all VFS
 * calls to disk functions, except for file permissions, which are now
 * mode 0700 for the current uid/gid.
 *
 * Copyright (C) Tim Potter, 1999-2000
 * Copyright (C) Alexander Bokovoy, 2002
 * Copyright (C) Andrew Bartlett, 2002
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

extern struct current_user current_user;

#undef DBGC_CLASS
#define DBGC_CLASS DBGC_VFS

static int fake_perms_stat(vfs_handle_struct *handle, connection_struct *conn, const char *fname, SMB_STRUCT_STAT *sbuf)
{
	int ret = -1;

	ret = SMB_VFS_NEXT_STAT(handle, conn, fname, sbuf);
	if (ret == 0) {
		if (S_ISDIR(sbuf->st_mode)) {
			sbuf->st_mode = S_IFDIR | S_IRWXU;
		} else {
			sbuf->st_mode = S_IRWXU;
		}
		sbuf->st_uid = current_user.ut.uid;
		sbuf->st_gid = current_user.ut.gid;
	}

	return ret;
}

static int fake_perms_fstat(vfs_handle_struct *handle, files_struct *fsp, int fd, SMB_STRUCT_STAT *sbuf)
{
	int ret = -1;

	ret = SMB_VFS_NEXT_FSTAT(handle, fsp, fd, sbuf);
	if (ret == 0) {
		if (S_ISDIR(sbuf->st_mode)) {
			sbuf->st_mode = S_IFDIR | S_IRWXU;
		} else {
			sbuf->st_mode = S_IRWXU;
		}
		sbuf->st_uid = current_user.ut.uid;
		sbuf->st_gid = current_user.ut.gid;
	}
	return ret;
}

/* VFS operations structure */

static vfs_op_tuple fake_perms_ops[] = {	
	{SMB_VFS_OP(fake_perms_stat),	SMB_VFS_OP_STAT,	SMB_VFS_LAYER_TRANSPARENT},
	{SMB_VFS_OP(fake_perms_fstat),	SMB_VFS_OP_FSTAT,	SMB_VFS_LAYER_TRANSPARENT},

	{SMB_VFS_OP(NULL),		SMB_VFS_OP_NOOP,	SMB_VFS_LAYER_NOOP}
};

NTSTATUS vfs_fake_perms_init(void)
{
	return smb_register_vfs(SMB_VFS_INTERFACE_VERSION, "fake_perms", fake_perms_ops);
}
