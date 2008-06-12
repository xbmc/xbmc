/* 
   Unix SMB/CIFS implementation.
   System QUOTA function wrappers
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

#ifdef HAVE_SYS_QUOTAS

#if defined(HAVE_QUOTACTL_4A) 

/*#endif HAVE_QUOTACTL_4A */
#elif defined(HAVE_QUOTACTL_4B)

#error HAVE_QUOTACTL_4B not implemeted

/*#endif HAVE_QUOTACTL_4B */
#elif defined(HAVE_QUOTACTL_3)

#error HAVE_QUOTACTL_3 not implemented

/* #endif  HAVE_QUOTACTL_3 */
#else /* NO_QUOTACTL_USED */

#endif /* NO_QUOTACTL_USED */

#ifdef HAVE_MNTENT
static int sys_path_to_bdev(const char *path, char **mntpath, char **bdev, char **fs)
{
	int ret = -1;
	SMB_STRUCT_STAT S;
	FILE *fp;
	struct mntent *mnt;
	SMB_DEV_T devno;

	/* find the block device file */

	if (!path||!mntpath||!bdev||!fs)
		smb_panic("sys_path_to_bdev: called with NULL pointer");

	(*mntpath) = NULL;
	(*bdev) = NULL;
	(*fs) = NULL;
	
	if ( sys_stat(path, &S) == -1 )
		return (-1);

	devno = S.st_dev ;

	fp = setmntent(MOUNTED,"r");
	if (fp == NULL) {
		return -1;
	}
  
	while ((mnt = getmntent(fp))) {
		if ( sys_stat(mnt->mnt_dir,&S) == -1 )
			continue ;

		if (S.st_dev == devno) {
			(*mntpath) = SMB_STRDUP(mnt->mnt_dir);
			(*bdev) = SMB_STRDUP(mnt->mnt_fsname);
			(*fs)   = SMB_STRDUP(mnt->mnt_type);
			if ((*mntpath)&&(*bdev)&&(*fs)) {
				ret = 0;
			} else {
				SAFE_FREE(*mntpath);
				SAFE_FREE(*bdev);
				SAFE_FREE(*fs);
				ret = -1;
			}

			break;
		}
	}

	endmntent(fp) ;

	return ret;
}
/* #endif HAVE_MNTENT */
#elif defined(HAVE_DEVNM)

/* we have this on HPUX, ... */
static int sys_path_to_bdev(const char *path, char **mntpath, char **bdev, char **fs)
{
	int ret = -1;
	char dev_disk[256];
	SMB_STRUCT_STAT S;

	if (!path||!mntpath||!bdev||!fs)
		smb_panic("sys_path_to_bdev: called with NULL pointer");

	(*mntpath) = NULL;
	(*bdev) = NULL;
	(*fs) = NULL;
	
	/* find the block device file */

	if ((ret=sys_stat(path, &S))!=0) {
		return ret;
	}
	
	if ((ret=devnm(S_IFBLK, S.st_dev, dev_disk, 256, 1))!=0) {
		return ret;	
	}

	/* we should get the mntpath right...
	 * but I don't know how
	 * --metze
	 */
	(*mntpath) = SMB_STRDUP(path);
	(*bdev) = SMB_STRDUP(dev_disk);
	if ((*mntpath)&&(*bdev)) {
		ret = 0;
	} else {
		SAFE_FREE(*mntpath);
		SAFE_FREE(*bdev);
		ret = -1;
	}	
	
	
	return ret;	
}

/* #endif HAVE_DEVNM */
#else
/* we should fake this up...*/
static int sys_path_to_bdev(const char *path, char **mntpath, char **bdev, char **fs)
{
	int ret = -1;

	if (!path||!mntpath||!bdev||!fs)
		smb_panic("sys_path_to_bdev: called with NULL pointer");

	(*mntpath) = NULL;
	(*bdev) = NULL;
	(*fs) = NULL;
	
	(*mntpath) = SMB_STRDUP(path);
	if (*mntpath) {
		ret = 0;
	} else {
		SAFE_FREE(*mntpath);
		ret = -1;
	}

	return ret;
}
#endif

/*********************************************************************
 Now the list of all filesystem specific quota systems we have found
**********************************************************************/
static struct {
	const char *name;
	int (*get_quota)(const char *path, const char *bdev, enum SMB_QUOTA_TYPE qtype, unid_t id, SMB_DISK_QUOTA *dp);
	int (*set_quota)(const char *path, const char *bdev, enum SMB_QUOTA_TYPE qtype, unid_t id, SMB_DISK_QUOTA *dp);
} sys_quota_backends[] = {
#ifdef HAVE_XFS_QUOTAS
	{"xfs", sys_get_xfs_quota, 	sys_set_xfs_quota},
#endif /* HAVE_XFS_QUOTAS */
	{NULL, 	NULL, 			NULL}	
};

static int command_get_quota(const char *path, enum SMB_QUOTA_TYPE qtype, unid_t id, SMB_DISK_QUOTA *dp)
{
	const char *get_quota_command;
	char **lines = NULL;
	
	get_quota_command = lp_get_quota_command();
	if (get_quota_command && *get_quota_command) {
		const char *p;
		char *p2;
		pstring syscmd;
		int _id = -1;

		switch(qtype) {
			case SMB_USER_QUOTA_TYPE:
			case SMB_USER_FS_QUOTA_TYPE:
				_id = id.uid;
				break;
			case SMB_GROUP_QUOTA_TYPE:
			case SMB_GROUP_FS_QUOTA_TYPE:
				_id = id.gid;
				break;
			default:
				DEBUG(0,("invalid quota type.\n"));
				return -1;
		}

		slprintf(syscmd, sizeof(syscmd)-1, 
			"%s \"%s\" %d %d", 
			get_quota_command, path, qtype, _id);

		DEBUG (3, ("get_quota: Running command %s\n", syscmd));

		lines = file_lines_pload(syscmd, NULL);
		if (lines) {
			char *line = lines[0];

			DEBUG (3, ("Read output from get_quota, \"%s\"\n", line));

			/* we need to deal with long long unsigned here, if supported */

			dp->qflags = (enum SMB_QUOTA_TYPE)strtoul(line, &p2, 10);
			p = p2;
			while (p && *p && isspace(*p)) {
				p++;
			}

			if (p && *p) {
				dp->curblocks = STR_TO_SMB_BIG_UINT(p, &p);
			} else {
				goto invalid_param;
			}

			while (p && *p && isspace(*p)) {
				p++;
			}

			if (p && *p) {
				dp->softlimit = STR_TO_SMB_BIG_UINT(p, &p);
			} else {
				goto invalid_param;
			}

			while (p && *p && isspace(*p)) {
				p++;
			}

			if (p && *p) {
				dp->hardlimit = STR_TO_SMB_BIG_UINT(p, &p);
			} else {
				goto invalid_param;
			}

			while (p && *p && isspace(*p)) {
				p++;
			}

			if (p && *p) {
				dp->curinodes = STR_TO_SMB_BIG_UINT(p, &p);
			} else {
				goto invalid_param;
			}

			while (p && *p && isspace(*p)) {
				p++;
			}

			if (p && *p) {
				dp->isoftlimit = STR_TO_SMB_BIG_UINT(p, &p);
			} else {
				goto invalid_param;
			}

			while (p && *p && isspace(*p)) {
				p++;
			}

			if (p && *p) {
				dp->ihardlimit = STR_TO_SMB_BIG_UINT(p, &p);
			} else {
				goto invalid_param;	
			}

			while (p && *p && isspace(*p)) {
				p++;
			}

			if (p && *p) {
				dp->bsize = STR_TO_SMB_BIG_UINT(p, NULL);
			} else {
				dp->bsize = 1024;
			}

			file_lines_free(lines);
			lines = NULL;

			DEBUG (3, ("Parsed output of get_quota, ...\n"));

#ifdef LARGE_SMB_OFF_T
			DEBUGADD (5,( 
				"qflags:%u curblocks:%llu softlimit:%llu hardlimit:%llu\n"
				"curinodes:%llu isoftlimit:%llu ihardlimit:%llu bsize:%llu\n", 
				dp->qflags,(long long unsigned)dp->curblocks,
				(long long unsigned)dp->softlimit,(long long unsigned)dp->hardlimit,
				(long long unsigned)dp->curinodes,
				(long long unsigned)dp->isoftlimit,(long long unsigned)dp->ihardlimit,
				(long long unsigned)dp->bsize));
#else /* LARGE_SMB_OFF_T */
			DEBUGADD (5,( 
				"qflags:%u curblocks:%lu softlimit:%lu hardlimit:%lu\n"
				"curinodes:%lu isoftlimit:%lu ihardlimit:%lu bsize:%lu\n", 
				dp->qflags,(long unsigned)dp->curblocks,
				(long unsigned)dp->softlimit,(long unsigned)dp->hardlimit,
				(long unsigned)dp->curinodes,
				(long unsigned)dp->isoftlimit,(long unsigned)dp->ihardlimit,
				(long unsigned)dp->bsize));
#endif /* LARGE_SMB_OFF_T */
			return 0;
		}

		DEBUG (0, ("get_quota_command failed!\n"));
		return -1;
	}

	errno = ENOSYS;
	return -1;
	
invalid_param:

	file_lines_free(lines);
	DEBUG(0,("The output of get_quota_command is invalid!\n"));
	return -1;
}

static int command_set_quota(const char *path, enum SMB_QUOTA_TYPE qtype, unid_t id, SMB_DISK_QUOTA *dp)
{
	const char *set_quota_command;
	
	set_quota_command = lp_set_quota_command();
	if (set_quota_command && *set_quota_command) {
		char **lines;
		pstring syscmd;
		int _id = -1;

		switch(qtype) {
			case SMB_USER_QUOTA_TYPE:
			case SMB_USER_FS_QUOTA_TYPE:
				_id = id.uid;
				break;
			case SMB_GROUP_QUOTA_TYPE:
			case SMB_GROUP_FS_QUOTA_TYPE:
				_id = id.gid;
				break;
			default:
				return -1;
		}

#ifdef LARGE_SMB_OFF_T
		slprintf(syscmd, sizeof(syscmd)-1, 
			"%s \"%s\" %d %d "
			"%u %llu %llu "
			"%llu %llu %llu ", 
			set_quota_command, path, qtype, _id, dp->qflags,
			(long long unsigned)dp->softlimit,(long long unsigned)dp->hardlimit,
			(long long unsigned)dp->isoftlimit,(long long unsigned)dp->ihardlimit,
			(long long unsigned)dp->bsize);
#else /* LARGE_SMB_OFF_T */
		slprintf(syscmd, sizeof(syscmd)-1, 
			"%s \"%s\" %d %d "
			"%u %lu %lu "
			"%lu %lu %lu ", 
			set_quota_command, path, qtype, _id, dp->qflags,
			(long unsigned)dp->softlimit,(long unsigned)dp->hardlimit,
			(long unsigned)dp->isoftlimit,(long unsigned)dp->ihardlimit,
			(long unsigned)dp->bsize);
#endif /* LARGE_SMB_OFF_T */



		DEBUG (3, ("get_quota: Running command %s\n", syscmd));

		lines = file_lines_pload(syscmd, NULL);
		if (lines) {
			char *line = lines[0];

			DEBUG (3, ("Read output from set_quota, \"%s\"\n", line));

			file_lines_free(lines);
			
			return 0;
		}
		DEBUG (0, ("set_quota_command failed!\n"));
		return -1;
	}

	errno = ENOSYS;
	return -1;
}

int sys_get_quota(const char *path, enum SMB_QUOTA_TYPE qtype, unid_t id, SMB_DISK_QUOTA *dp)
{
	int ret = -1;
	int i;
	BOOL ready = False;
	char *mntpath = NULL;
	char *bdev = NULL;
	char *fs = NULL;

	if (!path||!dp)
		smb_panic("sys_get_quota: called with NULL pointer");

	if (command_get_quota(path, qtype, id, dp)==0) {	
		return 0;
	} else if (errno != ENOSYS) {
		return -1;
	}

	if ((ret=sys_path_to_bdev(path,&mntpath,&bdev,&fs))!=0) {
		DEBUG(0,("sys_path_to_bdev() failed for path [%s]!\n",path));
		return ret;
	}

	errno = 0;
	DEBUG(10,("sys_get_quota() uid(%u, %u)\n", (unsigned)getuid(), (unsigned)geteuid()));

	for (i=0;(fs && sys_quota_backends[i].name && sys_quota_backends[i].get_quota);i++) {
		if (strcmp(fs,sys_quota_backends[i].name)==0) {
			ret = sys_quota_backends[i].get_quota(mntpath, bdev, qtype, id, dp);
			if (ret!=0) {
				DEBUG(3,("sys_get_%s_quota() failed for mntpath[%s] bdev[%s] qtype[%d] id[%d]: %s.\n",
					fs,mntpath,bdev,qtype,(qtype==SMB_GROUP_QUOTA_TYPE?id.gid:id.uid),strerror(errno)));
			} else {
				DEBUG(10,("sys_get_%s_quota() called for mntpath[%s] bdev[%s] qtype[%d] id[%d].\n",
					fs,mntpath,bdev,qtype,(qtype==SMB_GROUP_QUOTA_TYPE?id.gid:id.uid)));
			}
			ready = True;
			break;	
		}		
	}

	if (!ready) {
		/* use the default vfs quota functions */
		ret=sys_get_vfs_quota(mntpath, bdev, qtype, id, dp);
		if (ret!=0) {
			DEBUG(3,("sys_get_%s_quota() failed for mntpath[%s] bdev[%s] qtype[%d] id[%d]: %s\n",
				"vfs",mntpath,bdev,qtype,(qtype==SMB_GROUP_QUOTA_TYPE?id.gid:id.uid),strerror(errno)));
		} else {
			DEBUG(10,("sys_get_%s_quota() called for mntpath[%s] bdev[%s] qtype[%d] id[%d].\n",
				"vfs",mntpath,bdev,qtype,(qtype==SMB_GROUP_QUOTA_TYPE?id.gid:id.uid)));
		}
	}

	SAFE_FREE(mntpath);
	SAFE_FREE(bdev);
	SAFE_FREE(fs);

	if ((ret!=0)&& (errno == EDQUOT)) {
		DEBUG(10,("sys_get_quota() warning over quota!\n"));
		return 0;
	}

	return ret;
}

int sys_set_quota(const char *path, enum SMB_QUOTA_TYPE qtype, unid_t id, SMB_DISK_QUOTA *dp)
{
	int ret = -1;
	int i;
	BOOL ready = False;
	char *mntpath = NULL;
	char *bdev = NULL;
	char *fs = NULL;

	/* find the block device file */

	if (!path||!dp)
		smb_panic("get_smb_quota: called with NULL pointer");

	if (command_set_quota(path, qtype, id, dp)==0) {	
		return 0;
	} else if (errno != ENOSYS) {
		return -1;
	}

	if ((ret=sys_path_to_bdev(path,&mntpath,&bdev,&fs))!=0) {
		DEBUG(0,("sys_path_to_bdev() failed for path [%s]!\n",path));
		return ret;
	}

	errno = 0;
	DEBUG(10,("sys_set_quota() uid(%u, %u)\n", (unsigned)getuid(), (unsigned)geteuid())); 

	for (i=0;(fs && sys_quota_backends[i].name && sys_quota_backends[i].set_quota);i++) {
		if (strcmp(fs,sys_quota_backends[i].name)==0) {
			ret = sys_quota_backends[i].set_quota(mntpath, bdev, qtype, id, dp);
			if (ret!=0) {
				DEBUG(3,("sys_set_%s_quota() failed for mntpath[%s] bdev[%s] qtype[%d] id[%d]: %s.\n",
					fs,mntpath,bdev,qtype,(qtype==SMB_GROUP_QUOTA_TYPE?id.gid:id.uid),strerror(errno)));
			} else {
				DEBUG(10,("sys_set_%s_quota() called for mntpath[%s] bdev[%s] qtype[%d] id[%d].\n",
					fs,mntpath,bdev,qtype,(qtype==SMB_GROUP_QUOTA_TYPE?id.gid:id.uid)));
			}
			ready = True;
			break;
		}		
	}

	if (!ready) {
		/* use the default vfs quota functions */
		ret=sys_set_vfs_quota(mntpath, bdev, qtype, id, dp);
		if (ret!=0) {
			DEBUG(3,("sys_set_%s_quota() failed for mntpath[%s] bdev[%s] qtype[%d] id[%d]: %s.\n",
				"vfs",mntpath,bdev,qtype,(qtype==SMB_GROUP_QUOTA_TYPE?id.gid:id.uid),strerror(errno)));
		} else {
			DEBUG(10,("sys_set_%s_quota() called for mntpath[%s] bdev[%s] qtype[%d] id[%d].\n",
				"vfs",mntpath,bdev,qtype,(qtype==SMB_GROUP_QUOTA_TYPE?id.gid:id.uid)));
		}
	}

	SAFE_FREE(mntpath);
	SAFE_FREE(bdev);
	SAFE_FREE(fs);

	if ((ret!=0)&& (errno == EDQUOT)) {
		DEBUG(10,("sys_set_quota() warning over quota!\n"));
		return 0;
	}

	return ret;		
}

#else /* HAVE_SYS_QUOTAS */
 void dummy_sysquotas_c(void);

 void dummy_sysquotas_c(void)
{
	return;
}
#endif /* HAVE_SYS_QUOTAS */

