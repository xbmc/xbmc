/* 
   Unix SMB/CIFS implementation.
   System QUOTA function wrappers for QUOTACTL_4A
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
#ifdef HAVE_QUOTACTL_4A
#undef HAVE_QUOTACTL_4A
#endif
#endif

#ifdef HAVE_QUOTACTL_4A
/* long quotactl(int cmd, char *special, qid_t id, caddr_t addr) */
/* this is used by: HPUX,IRIX */

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifdef HAVE_ASM_TYPES_H
#include <asm/types.h>
#endif

#ifdef HAVE_SYS_QUOTA_H
#include <sys/quota.h>
#endif

#ifndef Q_SETQLIM
#define Q_SETQLIM Q_SETQUOTA
#endif

#ifndef QCMD
#define QCMD(x,y) x
#endif

#ifndef QCMD
#define QCMD(x,y) x
#endif

#ifdef GRPQUOTA
#define HAVE_GROUP_QUOTA
#endif

#ifndef QUOTABLOCK_SIZE
#define QUOTABLOCK_SIZE DEV_BSIZE
#endif

#ifdef HAVE_DQB_FSOFTLIMIT
#define dqb_isoftlimit	dqb_fsoftlimit
#define dqb_ihardlimit	dqb_fhardlimit
#define dqb_curinodes	dqb_curfiles
#endif

#ifdef INITQFNAMES
#define USERQUOTAFILE_EXTENSION ".user"
#else
#define USERQUOTAFILE_EXTENSION ""
#endif

#if !defined(QUOTAFILENAME) && defined(QFILENAME)
#define QUOTAFILENAME QFILENAME
#endif

/****************************************************************************
 Abstract out the quotactl_4A get calls.
****************************************************************************/
int sys_get_vfs_quota(const char *path, const char *bdev, enum SMB_QUOTA_TYPE qtype, unid_t id, SMB_DISK_QUOTA *dp)
{
	int ret = -1;
	uint32 qflags = 0;
	struct dqblk D;
	SMB_BIG_UINT bsize = (SMB_BIG_UINT)QUOTABLOCK_SIZE;

	ZERO_STRUCT(D);
	ZERO_STRUCT(*dp);
	dp->qtype = qtype;

	switch (qtype) {
		case SMB_USER_QUOTA_TYPE:
			DEBUG(10,("sys_get_vfs_quota: path[%s] bdev[%s] SMB_USER_QUOTA_TYPE uid[%u]\n",
				path, bdev, (unsigned)id.uid));

			if ((ret = quotactl(QCMD(Q_GETQUOTA,USRQUOTA), (caddr_t)bdev, id.uid, (void *)&D))&&errno != EDQUOT) {
				return ret;
			}

			if ((D.dqb_curblocks==0)&&
				(D.dqb_bsoftlimit==0)&&
				(D.dqb_bhardlimit==0)) {
				/* the upper layer functions don't want empty quota records...*/
				return -1;
			}

			break;
#ifdef HAVE_GROUP_QUOTA
		case SMB_GROUP_QUOTA_TYPE:
			DEBUG(10,("sys_get_vfs_quota: path[%s] bdev[%s] SMB_GROUP_QUOTA_TYPE gid[%u]\n",
				path, bdev, (unsigned)id.gid));

			if ((ret = quotactl(QCMD(Q_GETQUOTA,GRPQUOTA), (caddr_t)bdev, id.gid, (void *)&D))&&errno != EDQUOT) {
				return ret;
			}

			if ((D.dqb_curblocks==0)&&
				(D.dqb_bsoftlimit==0)&&
				(D.dqb_bhardlimit==0)) {
				/* the upper layer functions don't want empty quota records...*/
				return -1;
			}

			break;
#endif /* HAVE_GROUP_QUOTA */
		case SMB_USER_FS_QUOTA_TYPE:
			id.uid = getuid();

			DEBUG(10,("sys_get_vfs_quota: path[%s] bdev[%s] SMB_USER_FS_QUOTA_TYPE (uid[%u])\n",
				path, (caddr_t)bdev, (unsigned)id.uid));

			if ((ret = quotactl(QCMD(Q_GETQUOTA,USRQUOTA), (caddr_t)bdev, id.uid, (void *)&D))==0) {
				qflags |= QUOTAS_DENY_DISK;
			}

			ret = 0;
			break;
#ifdef HAVE_GROUP_QUOTA
		case SMB_GROUP_FS_QUOTA_TYPE:
			id.gid = getgid();

			DEBUG(10,("sys_get_vfs_quota: path[%s] bdev[%s] SMB_GROUP_FS_QUOTA_TYPE (gid[%u])\n",
				path, bdev, (unsigned)id.gid));

			if ((ret = quotactl(QCMD(Q_GETQUOTA,GRPQUOTA), (caddr_t)bdev, id.gid, (void *)&D))==0) {
				qflags |= QUOTAS_DENY_DISK;
			}

			ret = 0;
			break;
#endif /* HAVE_GROUP_QUOTA */
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
 Abstract out the quotactl_4A set calls.
****************************************************************************/
int sys_set_vfs_quota(const char *path, const char *bdev, enum SMB_QUOTA_TYPE qtype, unid_t id, SMB_DISK_QUOTA *dp)
{
	int ret = -1;
	uint32 qflags = 0;
	uint32 oldqflags = 0;
	struct dqblk D;
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
			DEBUG(10,("sys_set_vfs_quota: path[%s] bdev[%s] SMB_USER_QUOTA_TYPE uid[%u]\n",
				path, bdev, (unsigned)id.uid));

			ret = quotactl(QCMD(Q_SETQLIM,USRQUOTA), (caddr_t)bdev, id.uid, (void *)&D);
			break;
#ifdef HAVE_GROUP_QUOTA
		case SMB_GROUP_QUOTA_TYPE:
			DEBUG(10,("sys_set_vfs_quota: path[%s] bdev[%s] SMB_GROUP_QUOTA_TYPE gid[%u]\n",
				path, bdev, (unsigned)id.gid));

			ret = quotactl(QCMD(Q_SETQLIM,GRPQUOTA), (caddr_t)bdev, id.gid, (void *)&D);
			break;
#endif /* HAVE_GROUP_QUOTA */
		case SMB_USER_FS_QUOTA_TYPE:
			/* this stuff didn't work as it should:
			 * switching on/off quota via quotactl()
			 * didn't work!
			 * So we just return 0
			 * --metze
			 * 
			 * On HPUX we didn't have the mount path,
			 * we need to fix sys_path_to_bdev()
			 *
			 */
			id.uid = getuid();
			DEBUG(10,("sys_set_vfs_quota: path[%s] bdev[%s] SMB_USER_FS_QUOTA_TYPE (uid[%u])\n",
				path, bdev, (unsigned)id.uid));

#if 0
			ret = quotactl(QCMD(Q_GETQUOTA,USRQUOTA), (caddr_t)bdev, id.uid, (void *)&D);

			if ((qflags&QUOTAS_DENY_DISK)||(qflags&QUOTAS_ENABLED)) {
				if (ret == 0) {
					char *quota_file = NULL;
					
					asprintf(&quota_file,"/%s/%s%s",path, QUOTAFILENAME,USERQUOTAFILE_EXTENSION);
					if (quota_file == NULL) {
						DEBUG(0,("asprintf() failed!\n"));
						errno = ENOMEM;
						return -1;
					}
					
					ret = quotactl(QCMD(Q_QUOTAON,USRQUOTA), (caddr_t)bdev, -1,(void *)quota_file);
				} else {
					ret = 0;	
				}
			} else {
				if (ret != 0) {
					/* turn off */
					ret = quotactl(QCMD(Q_QUOTAOFF,USRQUOTA), (caddr_t)bdev, -1, (void *)0);	
				} else {
					ret = 0;
				}		
			}

			DEBUG(0,("sys_set_vfs_quota: ret(%d) errno(%d)[%s] uid(%d) bdev[%s]\n",
				ret,errno,strerror(errno),id.uid,bdev));
#else
			if ((ret = quotactl(QCMD(Q_GETQUOTA,USRQUOTA), (caddr_t)bdev, id.uid, (void *)&D))==0) {
				oldqflags |= QUOTAS_DENY_DISK;
			}

			if (oldqflags == qflags) {
				ret = 0;
			} else {
				ret = -1;
			}
#endif
			break;
#ifdef HAVE_GROUP_QUOTA
		case SMB_GROUP_FS_QUOTA_TYPE:
			/* this stuff didn't work as it should:
			 * switching on/off quota via quotactl()
			 * didn't work!
			 * So we just return 0
			 * --metze
			 * 
			 * On HPUX we didn't have the mount path,
			 * we need to fix sys_path_to_bdev()
			 *
			 */
			id.gid = getgid();
			DEBUG(10,("sys_set_vfs_quota: path[%s] bdev[%s] SMB_GROUP_FS_QUOTA_TYPE (gid[%u])\n",
				path, bdev, (unsigned)id.gid));

#if 0
			ret = quotactl(QCMD(Q_GETQUOTA,GRPQUOTA), bdev, id, (void *)&D);

			if ((qflags&QUOTAS_DENY_DISK)||(qflags&QUOTAS_ENABLED)) {
				if (ret == 0) {
					char *quota_file = NULL;
					
					asprintf(&quota_file,"/%s/%s%s",path, QUOTAFILENAME,GROUPQUOTAFILE_EXTENSION);
					if (quota_file == NULL) {
						DEBUG(0,("asprintf() failed!\n"));
						errno = ENOMEM;
						return -1;
					}
					
					ret = quotactl(QCMD(Q_QUOTAON,GRPQUOTA), (caddr_t)bdev, -1,(void *)quota_file);
				} else {
					ret = 0;	
				}
			} else {
				if (ret != 0) {
					/* turn off */
					ret = quotactl(QCMD(Q_QUOTAOFF,GRPQUOTA), (caddr_t)bdev, -1, (void *)0);	
				} else {
					ret = 0;
				}		
			}

			DEBUG(0,("sys_set_vfs_quota: ret(%d) errno(%d)[%s] uid(%d) bdev[%s]\n",
				ret,errno,strerror(errno),id.gid,bdev));
#else
			if ((ret = quotactl(QCMD(Q_GETQUOTA,GRPQUOTA), (caddr_t)bdev, id.gid, (void *)&D))==0) {
				oldqflags |= QUOTAS_DENY_DISK;
			}

			if (oldqflags == qflags) {
				ret = 0;
			} else {
				ret = -1;
			}
#endif
			break;
#endif /* HAVE_GROUP_QUOTA */
		default:
			errno = ENOSYS;
			return -1;
	}

	return ret;
}

#else /* HAVE_QUOTACTL_4A */
 void dummy_sysquotas_4A(void);

 void dummy_sysquotas_4A(void){}
#endif /* HAVE_QUOTACTL_4A */
