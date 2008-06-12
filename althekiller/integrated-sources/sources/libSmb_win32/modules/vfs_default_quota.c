/* 
 * Store default Quotas in a specified quota record
 *
 * Copyright (C) Stefan (metze) Metzmacher 2003
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
 * This module allows the default quota values,
 * in the windows explorer GUI, to be stored on a samba server.
 * The problem is that linux filesystems only store quotas
 * for users and groups, but no default quotas.
 *
 * Samba returns NO_LIMIT as the default quotas by default
 * and refuses to update them.
 *
 * With this module you can store the default quotas that are reported to
 * a windows client, in the quota record of a user. By default the root user
 * is taken because quota limits for root are typically not enforced.
 *
 * This module takes 2 parametric parameters in smb.conf:
 * (the default prefix for them is 'default_quota',
 *  it can be overwrittem when you load the module in
 *  the 'vfs objects' parameter like this:
 *  vfs objects = default_quota:myprefix)
 * 
 * "<myprefix>:uid" parameter takes a integer argument,
 *     it specifies the uid of the quota record, that will be taken for
 *     storing the default USER-quotas.
 *
 *     - default value: '0' (for root user)
 *     - e.g.: default_quota:uid = 65534
 *
 * "<myprefix>:uid nolimit" parameter takes a boolean argument,
 *     it specifies if we should report the stored default quota values,
 *     also for the user record, or if you should just report NO_LIMIT
 *     to the windows client for the user specified by the "<prefix>:uid" parameter.
 *     
 *     - default value: yes (that means to report NO_LIMIT)
 *     - e.g.: default_quota:uid nolimit = no
 * 
 * "<myprefix>:gid" parameter takes a integer argument,
 *     it's just like "<prefix>:uid" but for group quotas.
 *     (NOTE: group quotas are not supported from the windows explorer!)
 *
 *     - default value: '0' (for root group)
 *     - e.g.: default_quota:gid = 65534
 *
 * "<myprefix>:gid nolimit" parameter takes a boolean argument,
 *     it's just like "<prefix>:uid nolimit" but for group quotas.
 *     (NOTE: group quotas are not supported from the windows explorer!)
 *     
 *     - default value: yes (that means to report NO_LIMIT)
 *     - e.g.: default_quota:uid nolimit = no
 *
 */

#include "includes.h"

#undef DBGC_CLASS
#define DBGC_CLASS DBGC_QUOTA

#define DEFAULT_QUOTA_NAME "default_quota"

#define DEFAULT_QUOTA_UID_DEFAULT		0
#define DEFAULT_QUOTA_UID_NOLIMIT_DEFAULT	True
#define DEFAULT_QUOTA_GID_DEFAULT		0
#define DEFAULT_QUOTA_GID_NOLIMIT_DEFAULT	True

#define DEFAULT_QUOTA_UID(handle) \
	(uid_t)lp_parm_int(SNUM((handle)->conn),DEFAULT_QUOTA_NAME,"uid",DEFAULT_QUOTA_UID_DEFAULT)

#define DEFAULT_QUOTA_UID_NOLIMIT(handle) \
	lp_parm_bool(SNUM((handle)->conn),DEFAULT_QUOTA_NAME,"uid nolimit",DEFAULT_QUOTA_UID_NOLIMIT_DEFAULT)

#define DEFAULT_QUOTA_GID(handle) \
	(gid_t)lp_parm_int(SNUM((handle)->conn),DEFAULT_QUOTA_NAME,"gid",DEFAULT_QUOTA_GID_DEFAULT)

#define DEFAULT_QUOTA_GID_NOLIMIT(handle) \
	lp_parm_bool(SNUM((handle)->conn),DEFAULT_QUOTA_NAME,"gid nolimit",DEFAULT_QUOTA_GID_NOLIMIT_DEFAULT)

static int default_quota_get_quota(vfs_handle_struct *handle, connection_struct *conn, enum SMB_QUOTA_TYPE qtype, unid_t id, SMB_DISK_QUOTA *dq)
{
	int ret = -1;

	if ((ret=SMB_VFS_NEXT_GET_QUOTA(handle, conn, qtype, id, dq))!=0) {
		return ret;
	}

	switch (qtype) {
		case SMB_USER_QUOTA_TYPE:
			/* we use id.uid == 0 for default quotas */
			if ((id.uid==DEFAULT_QUOTA_UID(handle)) &&
				DEFAULT_QUOTA_UID_NOLIMIT(handle)) {
				SMB_QUOTAS_SET_NO_LIMIT(dq);
			}
			break;
#ifdef HAVE_GROUP_QUOTA
		case SMB_GROUP_QUOTA_TYPE:
			/* we use id.gid == 0 for default quotas */
			if ((id.gid==DEFAULT_QUOTA_GID(handle)) &&
				DEFAULT_QUOTA_GID_NOLIMIT(handle)) {
				SMB_QUOTAS_SET_NO_LIMIT(dq);
			}
			break;
#endif /* HAVE_GROUP_QUOTA */
		case SMB_USER_FS_QUOTA_TYPE:
			{
				unid_t qid;
				uint32 qflags = dq->qflags;
				qid.uid = DEFAULT_QUOTA_UID(handle);
				SMB_VFS_NEXT_GET_QUOTA(handle, conn, SMB_USER_QUOTA_TYPE, qid, dq);
				dq->qflags = qflags;
			}
			break;
#ifdef HAVE_GROUP_QUOTA
		case SMB_GROUP_FS_QUOTA_TYPE:
			{
				unid_t qid;
				uint32 qflags = dq->qflags;
				qid.gid = DEFAULT_QUOTA_GID(handle);
				SMB_VFS_NEXT_GET_QUOTA(handle, conn, SMB_GROUP_QUOTA_TYPE, qid, dq);
				dq->qflags = qflags;
			}
			break;
#endif /* HAVE_GROUP_QUOTA */
		default:
			errno = ENOSYS;
			return -1;
			break;
	}

	return ret;
}

static int default_quota_set_quota(vfs_handle_struct *handle, connection_struct *conn, enum SMB_QUOTA_TYPE qtype, unid_t id, SMB_DISK_QUOTA *dq)
{
	int ret = -1;

	switch (qtype) {
		case SMB_USER_QUOTA_TYPE:
			/* we use id.uid == 0 for default quotas */
			if ((id.uid==DEFAULT_QUOTA_UID(handle)) &&
				DEFAULT_QUOTA_UID_NOLIMIT(handle)) {
				return -1;
			}
			break;
#ifdef HAVE_GROUP_QUOTA
		case SMB_GROUP_QUOTA_TYPE:
			/* we use id.gid == 0 for default quotas */
			if ((id.gid==DEFAULT_QUOTA_GID(handle)) &&
				DEFAULT_QUOTA_GID_NOLIMIT(handle)) {
				return -1;
			}
			break;
#endif /* HAVE_GROUP_QUOTA */
		case SMB_USER_FS_QUOTA_TYPE:
			break;
#ifdef HAVE_GROUP_QUOTA
		case SMB_GROUP_FS_QUOTA_TYPE:
			break;
#endif /* HAVE_GROUP_QUOTA */
		default:
			errno = ENOSYS;
			return -1;
			break;
	}

	if ((ret=SMB_VFS_NEXT_SET_QUOTA(handle, conn, qtype, id, dq))!=0) {
		return ret;
	}

	switch (qtype) {
		case SMB_USER_QUOTA_TYPE:
			break;
#ifdef HAVE_GROUP_QUOTA
		case SMB_GROUP_QUOTA_TYPE:
			break;
#endif /* HAVE_GROUP_QUOTA */
		case SMB_USER_FS_QUOTA_TYPE:
			{
				unid_t qid;
				qid.uid = DEFAULT_QUOTA_UID(handle);
				ret = SMB_VFS_NEXT_SET_QUOTA(handle, conn, SMB_USER_QUOTA_TYPE, qid, dq);
			}
			break;
#ifdef HAVE_GROUP_QUOTA
		case SMB_GROUP_FS_QUOTA_TYPE:
			{
				unid_t qid;
				qid.gid = DEFAULT_QUOTA_GID(handle);
				ret = SMB_VFS_NEXT_SET_QUOTA(handle, conn, SMB_GROUP_QUOTA_TYPE, qid, dq);
			}
			break;
#endif /* HAVE_GROUP_QUOTA */
		default:
			errno = ENOSYS;
			return -1;
			break;
	}

	return ret;
}

/* VFS operations structure */

static vfs_op_tuple default_quota_ops[] = {	
	{SMB_VFS_OP(default_quota_get_quota),	SMB_VFS_OP_GET_QUOTA,	SMB_VFS_LAYER_TRANSPARENT},
	{SMB_VFS_OP(default_quota_set_quota),	SMB_VFS_OP_SET_QUOTA,	SMB_VFS_LAYER_TRANSPARENT},

	{SMB_VFS_OP(NULL),			SMB_VFS_OP_NOOP,	SMB_VFS_LAYER_NOOP}
};

NTSTATUS vfs_default_quota_init(void)
{
	return smb_register_vfs(SMB_VFS_INTERFACE_VERSION, DEFAULT_QUOTA_NAME, default_quota_ops);
}
