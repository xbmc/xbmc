/* 
   Unix SMB/CIFS implementation.
   System QUOTA function wrappers for LINUX
   Copyright (C) Stefan (metze) Metzmacher	2003
   
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
#define DBGC_CLASS DBGC_QUOTA

#ifndef HAVE_SYS_QUOTAS
#ifdef HAVE_QUOTACTL_LINUX
#undef HAVE_QUOTACTL_LINUX
#endif
#endif

#ifdef HAVE_QUOTACTL_LINUX 

#include "samba_linux_quota.h"

/****************************************************************************
 Abstract out the v1 Linux quota get calls.
****************************************************************************/
static int sys_get_linux_v1_quota(const char *path, const char *bdev, enum SMB_QUOTA_TYPE qtype, unid_t id, SMB_DISK_QUOTA *dp)
{
	int ret = -1;
	uint32 qflags = 0;
	struct v1_kern_dqblk D;
	SMB_BIG_UINT bsize = (SMB_BIG_UINT)QUOTABLOCK_SIZE;

	ZERO_STRUCT(D);

	switch (qtype) {
		case SMB_USER_QUOTA_TYPE:
			DEBUG(10,("sys_get_linux_v1_quota: path[%s] bdev[%s] SMB_USER_QUOTA_TYPE uid[%u]\n",
				path, bdev, (unsigned)id.uid));

			if ((ret = quotactl(QCMD(Q_V1_GETQUOTA,USRQUOTA), bdev, id.uid, (caddr_t)&D))&&errno != EDQUOT) {
				return ret;
			}

			break;
		case SMB_GROUP_QUOTA_TYPE:
			DEBUG(10,("sys_get_linux_v1_quota: path[%s] bdev[%s] SMB_GROUP_QUOTA_TYPE gid[%u]\n",
				path, bdev, (unsigned)id.gid));

			if ((ret = quotactl(QCMD(Q_V1_GETQUOTA,GRPQUOTA), bdev, id.gid, (caddr_t)&D))&&errno != EDQUOT) {
				return ret;
			}

			break;
		case SMB_USER_FS_QUOTA_TYPE:
			DEBUG(10,("sys_get_linux_v1_quota: path[%s] bdev[%s] SMB_USER_FS_QUOTA_TYPE (uid[%u])\n",
				path, bdev, (unsigned)id.uid));

			if ((ret = quotactl(QCMD(Q_V1_GETQUOTA,USRQUOTA), bdev, id.uid, (caddr_t)&D))==0) {
				qflags |= QUOTAS_DENY_DISK;
			}

			break;
		case SMB_GROUP_FS_QUOTA_TYPE:
			DEBUG(10,("sys_get_linux_v1_quota: path[%s] bdev[%s] SMB_GROUP_FS_QUOTA_TYPE (gid[%u])\n",
				path, bdev, (unsigned)id.gid));

			if ((ret = quotactl(QCMD(Q_V1_GETQUOTA,GRPQUOTA), bdev, id.gid, (caddr_t)&D))==0) {
				qflags |= QUOTAS_DENY_DISK;
			}

			break;
		default:
			errno = ENOSYS;
			return -1;
	}

	dp->bsize = bsize;
	dp->softlimit = (SMB_BIG_UINT)D.dqb_bsoftlimit;
	dp->hardlimit = (SMB_BIG_UINT)D.dqb_bhardlimit;
	dp->ihardlimit = (SMB_BIG_UINT)D.dqb_ihardlimit;
	dp->isoftlimit = (SMB_BIG_UINT)D.dqb_isoftlimit;
	dp->curinodes = (SMB_BIG_UINT)D.dqb_curinodes;
	dp->curblocks = (SMB_BIG_UINT)D.dqb_curblocks;


	dp->qflags = qflags;

	return ret;
}

/****************************************************************************
 Abstract out the v1 Linux quota set calls.
****************************************************************************/
static int sys_set_linux_v1_quota(const char *path, const char *bdev, enum SMB_QUOTA_TYPE qtype, unid_t id, SMB_DISK_QUOTA *dp)
{
	int ret = -1;
	uint32 qflags = 0;
	uint32 oldqflags = 0;
	struct v1_kern_dqblk D;
	SMB_BIG_UINT bsize = (SMB_BIG_UINT)QUOTABLOCK_SIZE;

	ZERO_STRUCT(D);

	if (bsize == dp->bsize) {
		D.dqb_bsoftlimit = dp->softlimit;
		D.dqb_bhardlimit = dp->hardlimit;
		D.dqb_ihardlimit = dp->ihardlimit;
		D.dqb_isoftlimit = dp->isoftlimit;
	} else {
		D.dqb_bsoftlimit = (dp->softlimit*dp->bsize)/bsize;
		D.dqb_bhardlimit = (dp->hardlimit*dp->bsize)/bsize;
		D.dqb_ihardlimit = (dp->ihardlimit*dp->bsize)/bsize;
		D.dqb_isoftlimit = (dp->isoftlimit*dp->bsize)/bsize;
	}

	qflags = dp->qflags;

	switch (qtype) {
		case SMB_USER_QUOTA_TYPE:
			DEBUG(10,("sys_set_linux_v1_quota: path[%s] bdev[%s] SMB_USER_QUOTA_TYPE uid[%u]\n",
				path, bdev, (unsigned)id.uid));

			ret = quotactl(QCMD(Q_V1_SETQUOTA,USRQUOTA), bdev, id.uid, (caddr_t)&D);
			break;
		case SMB_GROUP_QUOTA_TYPE:
			DEBUG(10,("sys_set_linux_v1_quota: path[%s] bdev[%s] SMB_GROUP_QUOTA_TYPE gid[%u]\n",
				path, bdev, (unsigned)id.gid));

			ret = quotactl(QCMD(Q_V1_SETQUOTA,GRPQUOTA), bdev, id.gid, (caddr_t)&D);
			break;
		case SMB_USER_FS_QUOTA_TYPE:
			DEBUG(10,("sys_set_linux_v1_quota: path[%s] bdev[%s] SMB_USER_FS_QUOTA_TYPE (uid[%u])\n",
				path, bdev, (unsigned)id.uid));

			if ((ret = quotactl(QCMD(Q_V1_GETQUOTA,USRQUOTA), bdev, id.uid, (caddr_t)&D))==0) {
				oldqflags |= QUOTAS_DENY_DISK;
			}

			break;
		case SMB_GROUP_FS_QUOTA_TYPE:
			DEBUG(10,("sys_set_linux_v1_quota: path[%s] bdev[%s] SMB_GROUP_FS_QUOTA_TYPE (gid[%u])\n",
				path, bdev, (unsigned)id.gid));

			if ((ret = quotactl(QCMD(Q_V1_GETQUOTA,GRPQUOTA), bdev, id.gid, (caddr_t)&D))==0) {
				oldqflags |= QUOTAS_DENY_DISK;
			}

			break;
		default:
			errno = ENOSYS;
			return -1;
	}

	return ret;
}

/****************************************************************************
 Abstract out the v2 Linux quota get calls.
****************************************************************************/
static int sys_get_linux_v2_quota(const char *path, const char *bdev, enum SMB_QUOTA_TYPE qtype, unid_t id, SMB_DISK_QUOTA *dp)
{
	int ret = -1;
	uint32 qflags = 0;
	struct v2_kern_dqblk D;
	SMB_BIG_UINT bsize = (SMB_BIG_UINT)QUOTABLOCK_SIZE;

	ZERO_STRUCT(D);

	switch (qtype) {
		case SMB_USER_QUOTA_TYPE:
			DEBUG(10,("sys_get_linux_v2_quota: path[%s] bdev[%s] SMB_USER_QUOTA_TYPE uid[%u]\n",
				path, bdev, (unsigned)id.uid));

			if ((ret = quotactl(QCMD(Q_V2_GETQUOTA,USRQUOTA), bdev, id.uid, (caddr_t)&D))&&errno != EDQUOT) {
				return ret;
			}

			break;
		case SMB_GROUP_QUOTA_TYPE:
			DEBUG(10,("sys_get_linux_v2_quota: path[%s] bdev[%s] SMB_GROUP_QUOTA_TYPE gid[%u]\n",
				path, bdev, (unsigned)id.gid));

			if ((ret = quotactl(QCMD(Q_V2_GETQUOTA,GRPQUOTA), bdev, id.gid, (caddr_t)&D))&&errno != EDQUOT) {
				return ret;
			}

			break;
		case SMB_USER_FS_QUOTA_TYPE:
			DEBUG(10,("sys_get_linux_v2_quota: path[%s] bdev[%s] SMB_USER_FS_QUOTA_TYPE (uid[%u])\n",
				path, bdev, (unsigned)id.uid));

			if ((ret = quotactl(QCMD(Q_V2_GETQUOTA,USRQUOTA), bdev, id.uid, (caddr_t)&D))==0) {
				qflags |= QUOTAS_DENY_DISK;
			}

			break;
		case SMB_GROUP_FS_QUOTA_TYPE:
			DEBUG(10,("sys_get_linux_v2_quota: path[%s] bdev[%s] SMB_GROUP_FS_QUOTA_TYPE (gid[%u])\n",
				path, bdev, (unsigned)id.gid));

			if ((ret = quotactl(QCMD(Q_V2_GETQUOTA,GRPQUOTA), bdev, id.gid, (caddr_t)&D))==0) {
				qflags |= QUOTAS_DENY_DISK;
			}

			break;
		default:
			errno = ENOSYS;
			return -1;
	}

	dp->bsize = bsize;
	dp->softlimit = (SMB_BIG_UINT)D.dqb_bsoftlimit;
	dp->hardlimit = (SMB_BIG_UINT)D.dqb_bhardlimit;
	dp->ihardlimit = (SMB_BIG_UINT)D.dqb_ihardlimit;
	dp->isoftlimit = (SMB_BIG_UINT)D.dqb_isoftlimit;
	dp->curinodes = (SMB_BIG_UINT)D.dqb_curinodes;
	dp->curblocks = (SMB_BIG_UINT)D.dqb_curspace/bsize;


	dp->qflags = qflags;

	return ret;
}

/****************************************************************************
 Abstract out the v2 Linux quota set calls.
****************************************************************************/
static int sys_set_linux_v2_quota(const char *path, const char *bdev, enum SMB_QUOTA_TYPE qtype, unid_t id, SMB_DISK_QUOTA *dp)
{
	int ret = -1;
	uint32 qflags = 0;
	uint32 oldqflags = 0;
	struct v2_kern_dqblk D;
	SMB_BIG_UINT bsize = (SMB_BIG_UINT)QUOTABLOCK_SIZE;

	ZERO_STRUCT(D);

	if (bsize == dp->bsize) {
		D.dqb_bsoftlimit = dp->softlimit;
		D.dqb_bhardlimit = dp->hardlimit;
		D.dqb_ihardlimit = dp->ihardlimit;
		D.dqb_isoftlimit = dp->isoftlimit;
	} else {
		D.dqb_bsoftlimit = (dp->softlimit*dp->bsize)/bsize;
		D.dqb_bhardlimit = (dp->hardlimit*dp->bsize)/bsize;
		D.dqb_ihardlimit = (dp->ihardlimit*dp->bsize)/bsize;
		D.dqb_isoftlimit = (dp->isoftlimit*dp->bsize)/bsize;
	}

	qflags = dp->qflags;

	switch (qtype) {
		case SMB_USER_QUOTA_TYPE:
			DEBUG(10,("sys_set_linux_v2_quota: path[%s] bdev[%s] SMB_USER_QUOTA_TYPE uid[%u]\n",
				path, bdev, (unsigned)id.uid));

			ret = quotactl(QCMD(Q_V2_SETQUOTA,USRQUOTA), bdev, id.uid, (caddr_t)&D);
			break;
		case SMB_GROUP_QUOTA_TYPE:
			DEBUG(10,("sys_set_linux_v2_quota: path[%s] bdev[%s] SMB_GROUP_QUOTA_TYPE gid[%u]\n",
				path, bdev, (unsigned)id.gid));

			ret = quotactl(QCMD(Q_V2_SETQUOTA,GRPQUOTA), bdev, id.gid, (caddr_t)&D);
			break;
		case SMB_USER_FS_QUOTA_TYPE:
			DEBUG(10,("sys_set_linux_v2_quota: path[%s] bdev[%s] SMB_USER_FS_QUOTA_TYPE (uid[%u])\n",
				path, bdev, (unsigned)id.uid));

			if ((ret = quotactl(QCMD(Q_V2_GETQUOTA,USRQUOTA), bdev, id.uid, (caddr_t)&D))==0) {
				oldqflags |= QUOTAS_DENY_DISK;
			}

			break;
		case SMB_GROUP_FS_QUOTA_TYPE:
			DEBUG(10,("sys_set_linux_v2_quota: path[%s] bdev[%s] SMB_GROUP_FS_QUOTA_TYPE (gid[%u])\n",
				path, bdev, (unsigned)id.gid));

			if ((ret = quotactl(QCMD(Q_V2_GETQUOTA,GRPQUOTA), bdev, id.gid, (caddr_t)&D))==0) {
				oldqflags |= QUOTAS_DENY_DISK;
			}

			break;
		default:
			errno = ENOSYS;
			return -1;
	}

	return ret;
}

/****************************************************************************
 Abstract out the generic Linux quota get calls.
****************************************************************************/
static int sys_get_linux_gen_quota(const char *path, const char *bdev, enum SMB_QUOTA_TYPE qtype, unid_t id, SMB_DISK_QUOTA *dp)
{
	int ret = -1;
	uint32 qflags = 0;
	struct if_dqblk D;
	SMB_BIG_UINT bsize = (SMB_BIG_UINT)QUOTABLOCK_SIZE;

	ZERO_STRUCT(D);

	switch (qtype) {
		case SMB_USER_QUOTA_TYPE:
			DEBUG(10,("sys_get_linux_gen_quota: path[%s] bdev[%s] SMB_USER_QUOTA_TYPE uid[%u]\n",
				path, bdev, (unsigned)id.uid));

			if ((ret = quotactl(QCMD(Q_GETQUOTA,USRQUOTA), bdev, id.uid, (caddr_t)&D))&&errno != EDQUOT) {
				return ret;
			}

			break;
		case SMB_GROUP_QUOTA_TYPE:
			DEBUG(10,("sys_get_linux_gen_quota: path[%s] bdev[%s] SMB_GROUP_QUOTA_TYPE gid[%u]\n",
				path, bdev, (unsigned)id.gid));

			if ((ret = quotactl(QCMD(Q_GETQUOTA,GRPQUOTA), bdev, id.gid, (caddr_t)&D))&&errno != EDQUOT) {
				return ret;
			}

			break;
		case SMB_USER_FS_QUOTA_TYPE:
			DEBUG(10,("sys_get_linux_gen_quota: path[%s] bdev[%s] SMB_USER_FS_QUOTA_TYPE (uid[%u])\n",
				path, bdev, (unsigned)id.uid));

			if ((ret = quotactl(QCMD(Q_GETQUOTA,USRQUOTA), bdev, id.uid, (caddr_t)&D))==0) {
				qflags |= QUOTAS_DENY_DISK;
			}

			break;
		case SMB_GROUP_FS_QUOTA_TYPE:
			DEBUG(10,("sys_get_linux_gen_quota: path[%s] bdev[%s] SMB_GROUP_FS_QUOTA_TYPE (gid[%u])\n",
				path, bdev, (unsigned)id.gid));

			if ((ret = quotactl(QCMD(Q_GETQUOTA,GRPQUOTA), bdev, id.gid, (caddr_t)&D))==0) {
				qflags |= QUOTAS_DENY_DISK;
			}

			break;
		default:
			errno = ENOSYS;
			return -1;
	}

	dp->bsize = bsize;
	dp->softlimit = (SMB_BIG_UINT)D.dqb_bsoftlimit;
	dp->hardlimit = (SMB_BIG_UINT)D.dqb_bhardlimit;
	dp->ihardlimit = (SMB_BIG_UINT)D.dqb_ihardlimit;
	dp->isoftlimit = (SMB_BIG_UINT)D.dqb_isoftlimit;
	dp->curinodes = (SMB_BIG_UINT)D.dqb_curinodes;
	dp->curblocks = (SMB_BIG_UINT)D.dqb_curspace/bsize;


	dp->qflags = qflags;

	return ret;
}

/****************************************************************************
 Abstract out the gen Linux quota set calls.
****************************************************************************/
static int sys_set_linux_gen_quota(const char *path, const char *bdev, enum SMB_QUOTA_TYPE qtype, unid_t id, SMB_DISK_QUOTA *dp)
{
	int ret = -1;
	uint32 qflags = 0;
	uint32 oldqflags = 0;
	struct if_dqblk D;
	SMB_BIG_UINT bsize = (SMB_BIG_UINT)QUOTABLOCK_SIZE;

	ZERO_STRUCT(D);

	if (bsize == dp->bsize) {
		D.dqb_bsoftlimit = dp->softlimit;
		D.dqb_bhardlimit = dp->hardlimit;
		D.dqb_ihardlimit = dp->ihardlimit;
		D.dqb_isoftlimit = dp->isoftlimit;
	} else {
		D.dqb_bsoftlimit = (dp->softlimit*dp->bsize)/bsize;
		D.dqb_bhardlimit = (dp->hardlimit*dp->bsize)/bsize;
		D.dqb_ihardlimit = (dp->ihardlimit*dp->bsize)/bsize;
		D.dqb_isoftlimit = (dp->isoftlimit*dp->bsize)/bsize;
	}
	D.dqb_valid = QIF_LIMITS;

	qflags = dp->qflags;

	switch (qtype) {
		case SMB_USER_QUOTA_TYPE:
			DEBUG(10,("sys_set_linux_gen_quota: path[%s] bdev[%s] SMB_USER_QUOTA_TYPE uid[%u]\n",
				path, bdev, (unsigned)id.uid));

			ret = quotactl(QCMD(Q_SETQUOTA,USRQUOTA), bdev, id.uid, (caddr_t)&D);
			break;
		case SMB_GROUP_QUOTA_TYPE:
			DEBUG(10,("sys_set_linux_gen_quota: path[%s] bdev[%s] SMB_GROUP_QUOTA_TYPE gid[%u]\n",
				path, bdev, (unsigned)id.gid));

			ret = quotactl(QCMD(Q_SETQUOTA,GRPQUOTA), bdev, id.gid, (caddr_t)&D);
			break;
		case SMB_USER_FS_QUOTA_TYPE:
			DEBUG(10,("sys_set_linux_gen_quota: path[%s] bdev[%s] SMB_USER_FS_QUOTA_TYPE (uid[%u])\n",
				path, bdev, (unsigned)id.uid));

			if ((ret = quotactl(QCMD(Q_GETQUOTA,USRQUOTA), bdev, id.uid, (caddr_t)&D))==0) {
				oldqflags |= QUOTAS_DENY_DISK;
			}

			break;
		case SMB_GROUP_FS_QUOTA_TYPE:
			DEBUG(10,("sys_set_linux_gen_quota: path[%s] bdev[%s] SMB_GROUP_FS_QUOTA_TYPE (gid[%u])\n",
				path, bdev, (unsigned)id.gid));

			if ((ret = quotactl(QCMD(Q_GETQUOTA,GRPQUOTA), bdev, id.gid, (caddr_t)&D))==0) {
				oldqflags |= QUOTAS_DENY_DISK;
			}

			break;
		default:
			errno = ENOSYS;
			return -1;
	}

	return ret;
}

/****************************************************************************
 Abstract out the Linux quota get calls.
****************************************************************************/
int sys_get_vfs_quota(const char *path, const char *bdev, enum SMB_QUOTA_TYPE qtype, unid_t id, SMB_DISK_QUOTA *dp)
{
	int ret = -1;

	if (!path||!bdev||!dp)
		smb_panic("sys_set_vfs_quota: called with NULL pointer");

	ZERO_STRUCT(*dp);
	dp->qtype = qtype;

	switch (qtype) {
		case SMB_USER_QUOTA_TYPE:
		case SMB_GROUP_QUOTA_TYPE:
			if ((ret=sys_get_linux_gen_quota(path, bdev, qtype, id, dp))&&errno != EDQUOT) {
				if ((ret=sys_get_linux_v2_quota(path, bdev, qtype, id, dp))&&errno != EDQUOT) {
					if ((ret=sys_get_linux_v1_quota(path, bdev, qtype, id, dp))&&errno != EDQUOT) {
						return ret;
					}
				}
			}

			if ((dp->curblocks==0)&&
				(dp->softlimit==0)&&
				(dp->hardlimit==0)) {
				/* the upper layer functions don't want empty quota records...*/
				return -1;
			}

			break;
		case SMB_USER_FS_QUOTA_TYPE:
			id.uid = getuid();

			if ((ret=sys_get_linux_gen_quota(path, bdev, qtype, id, dp))&&errno != EDQUOT) {
				if ((ret=sys_get_linux_v2_quota(path, bdev, qtype, id, dp))&&errno != EDQUOT) {
					ret=sys_get_linux_v1_quota(path, bdev, qtype, id, dp);
				}
			}

			ret = 0;
			break;
		case SMB_GROUP_FS_QUOTA_TYPE:
			id.gid = getgid();

			if ((ret=sys_get_linux_gen_quota(path, bdev, qtype, id, dp))&&errno != EDQUOT) {
				if ((ret=sys_get_linux_v2_quota(path, bdev, qtype, id, dp))&&errno != EDQUOT) {
					ret=sys_get_linux_v1_quota(path, bdev, qtype, id, dp);
				}
			}

			ret = 0;
			break;
		default:
			errno = ENOSYS;
			return -1;
	}

	return ret;
}

/****************************************************************************
 Abstract out the Linux quota set calls.
****************************************************************************/
int sys_set_vfs_quota(const char *path, const char *bdev, enum SMB_QUOTA_TYPE qtype, unid_t id, SMB_DISK_QUOTA *dp)
{
	int ret = -1;
	uint32 oldqflags = 0;

	if (!path||!bdev||!dp)
		smb_panic("sys_set_vfs_quota: called with NULL pointer");

	oldqflags = dp->qflags;

	switch (qtype) {
		case SMB_USER_QUOTA_TYPE:
		case SMB_GROUP_QUOTA_TYPE:
			if ((ret=sys_set_linux_gen_quota(path, bdev, qtype, id, dp))) {
				if ((ret=sys_set_linux_v2_quota(path, bdev, qtype, id, dp))) {
					if ((ret=sys_set_linux_v1_quota(path, bdev, qtype, id, dp))) {
						return ret;
					}
				}
			}
			break;
		case SMB_USER_FS_QUOTA_TYPE:
			id.uid = getuid();

			if ((ret=sys_get_linux_gen_quota(path, bdev, qtype, id, dp))) {
				if ((ret=sys_get_linux_v2_quota(path, bdev, qtype, id, dp))) {
					ret=sys_get_linux_v1_quota(path, bdev, qtype, id, dp);
				}
			}

			if (oldqflags == dp->qflags) {
				ret = 0;
			} else {
				ret = -1;
			}
			break;
		case SMB_GROUP_FS_QUOTA_TYPE:
			id.gid = getgid();

			if ((ret=sys_get_linux_gen_quota(path, bdev, qtype, id, dp))) {
				if ((ret=sys_get_linux_v2_quota(path, bdev, qtype, id, dp))) {
					ret=sys_get_linux_v1_quota(path, bdev, qtype, id, dp);
				}
			}

			if (oldqflags == dp->qflags) {
				ret = 0;
			} else {
				ret = -1;
			}

			break;
		default:
			errno = ENOSYS;
			return -1;
	}

	return ret;
}

#else /* HAVE_QUOTACTL_LINUX */
 void dummy_sysquotas_linux(void);

 void dummy_sysquotas_linux(void){}
#endif /* HAVE_QUOTACTL_LINUX */
