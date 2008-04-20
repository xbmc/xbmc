#ifndef _PROFILE_H_
#define _PROFILE_H_
/* 
   Unix SMB/CIFS implementation.
   store smbd profiling information in shared memory
   Copyright (C) Andrew Tridgell 1999
   
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

/* this file defines the profile structure in the profile shared
   memory area */

#define PROF_SHMEM_KEY ((key_t)0x07021999)
#define PROF_SHM_MAGIC 0x6349985
#define PROF_SHM_VERSION 10

/* time values in the following structure are in microseconds */

struct profile_stats {
/* general counters */
	unsigned smb_count; /* how many SMB packets we have processed */
	unsigned uid_changes; /* how many times we change our effective uid */
/* system call counters */
	unsigned syscall_opendir_count;
	unsigned syscall_opendir_time;
	unsigned syscall_readdir_count;
	unsigned syscall_readdir_time;
	unsigned syscall_seekdir_count;
	unsigned syscall_seekdir_time;
	unsigned syscall_telldir_count;
	unsigned syscall_telldir_time;
	unsigned syscall_rewinddir_count;
	unsigned syscall_rewinddir_time;
	unsigned syscall_mkdir_count;
	unsigned syscall_mkdir_time;
	unsigned syscall_rmdir_count;
	unsigned syscall_rmdir_time;
	unsigned syscall_closedir_count;
	unsigned syscall_closedir_time;
	unsigned syscall_open_count;
	unsigned syscall_open_time;
	unsigned syscall_close_count;
	unsigned syscall_close_time;
	unsigned syscall_read_count;
	unsigned syscall_read_time;
	unsigned syscall_read_bytes;	/* bytes read with read syscall */
	unsigned syscall_pread_count;
	unsigned syscall_pread_time;
	unsigned syscall_pread_bytes;	/* bytes read with pread syscall */
	unsigned syscall_write_count;
	unsigned syscall_write_time;
	unsigned syscall_write_bytes;	/* bytes written with write syscall */
	unsigned syscall_pwrite_count;
	unsigned syscall_pwrite_time;
	unsigned syscall_pwrite_bytes;	/* bytes written with pwrite syscall */
	unsigned syscall_lseek_count;
	unsigned syscall_lseek_time;
	unsigned syscall_sendfile_count;
	unsigned syscall_sendfile_time;
	unsigned syscall_sendfile_bytes; /* bytes read with sendfile syscall */
	unsigned syscall_rename_count;
	unsigned syscall_rename_time;
	unsigned syscall_fsync_count;
	unsigned syscall_fsync_time;
	unsigned syscall_stat_count;
	unsigned syscall_stat_time;
	unsigned syscall_fstat_count;
	unsigned syscall_fstat_time;
	unsigned syscall_lstat_count;
	unsigned syscall_lstat_time;
	unsigned syscall_unlink_count;
	unsigned syscall_unlink_time;
	unsigned syscall_chmod_count;
	unsigned syscall_chmod_time;
	unsigned syscall_fchmod_count;
	unsigned syscall_fchmod_time;
	unsigned syscall_chown_count;
	unsigned syscall_chown_time;
	unsigned syscall_fchown_count;
	unsigned syscall_fchown_time;
	unsigned syscall_chdir_count;
	unsigned syscall_chdir_time;
	unsigned syscall_getwd_count;
	unsigned syscall_getwd_time;
	unsigned syscall_utime_count;
	unsigned syscall_utime_time;
	unsigned syscall_ftruncate_count;
	unsigned syscall_ftruncate_time;
	unsigned syscall_fcntl_lock_count;
	unsigned syscall_fcntl_lock_time;
	unsigned syscall_fcntl_getlock_count;
	unsigned syscall_fcntl_getlock_time;
	unsigned syscall_readlink_count;
	unsigned syscall_readlink_time;
	unsigned syscall_symlink_count;
	unsigned syscall_symlink_time;
	unsigned syscall_link_count;
	unsigned syscall_link_time;
	unsigned syscall_mknod_count;
	unsigned syscall_mknod_time;
	unsigned syscall_realpath_count;
	unsigned syscall_realpath_time;
	unsigned syscall_get_quota_count;
	unsigned syscall_get_quota_time;
	unsigned syscall_set_quota_count;
	unsigned syscall_set_quota_time;
/* stat cache counters */
	unsigned statcache_lookups;
	unsigned statcache_misses;
	unsigned statcache_hits;
/* write cache counters */
	unsigned writecache_read_hits;
	unsigned writecache_abutted_writes;
	unsigned writecache_total_writes;
	unsigned writecache_non_oplock_writes;
	unsigned writecache_direct_writes;
	unsigned writecache_init_writes;
	unsigned writecache_flushed_writes[NUM_FLUSH_REASONS];
	unsigned writecache_num_perfect_writes;
	unsigned writecache_num_write_caches;
	unsigned writecache_allocated_write_caches;
/* counters for individual SMB types */
	unsigned SMBmkdir_count;	/* create directory */
	unsigned SMBmkdir_time;
	unsigned SMBrmdir_count;	/* delete directory */
	unsigned SMBrmdir_time;
	unsigned SMBopen_count;		/* open file */
	unsigned SMBopen_time;
	unsigned SMBcreate_count;	/* create file */
	unsigned SMBcreate_time;
	unsigned SMBclose_count;	/* close file */
	unsigned SMBclose_time;
	unsigned SMBflush_count;	/* flush file */
	unsigned SMBflush_time;
	unsigned SMBunlink_count;	/* delete file */
	unsigned SMBunlink_time;
	unsigned SMBmv_count;		/* rename file */
	unsigned SMBmv_time;
	unsigned SMBgetatr_count;	/* get file attributes */
	unsigned SMBgetatr_time;
	unsigned SMBsetatr_count;	/* set file attributes */
	unsigned SMBsetatr_time;
	unsigned SMBread_count;		/* read from file */
	unsigned SMBread_time;
	unsigned SMBwrite_count;	/* write to file */
	unsigned SMBwrite_time;
	unsigned SMBlock_count;		/* lock byte range */
	unsigned SMBlock_time;
	unsigned SMBunlock_count;	/* unlock byte range */
	unsigned SMBunlock_time;
	unsigned SMBctemp_count;	/* create temporary file */
	unsigned SMBctemp_time;
	/* SMBmknew stats are currently combined with SMBcreate */
	unsigned SMBmknew_count;	/* make new file */
	unsigned SMBmknew_time;
	unsigned SMBchkpth_count;	/* check directory path */
	unsigned SMBchkpth_time;
	unsigned SMBexit_count;		/* process exit */
	unsigned SMBexit_time;
	unsigned SMBlseek_count;	/* seek */
	unsigned SMBlseek_time;
	unsigned SMBlockread_count;	/* Lock a range and read */
	unsigned SMBlockread_time;
	unsigned SMBwriteunlock_count;	/* Unlock a range then write */
	unsigned SMBwriteunlock_time;
	unsigned SMBreadbraw_count;	/* read a block of data with no smb header */
	unsigned SMBreadbraw_time;
	unsigned SMBreadBmpx_count;	/* read block multiplexed */
	unsigned SMBreadBmpx_time;
	unsigned SMBreadBs_count;	/* read block (secondary response) */
	unsigned SMBreadBs_time;
	unsigned SMBwritebraw_count;	/* write a block of data with no smb header */
	unsigned SMBwritebraw_time;
	unsigned SMBwriteBmpx_count;	/* write block multiplexed */
	unsigned SMBwriteBmpx_time;
	unsigned SMBwriteBs_count;	/* write block (secondary request) */
	unsigned SMBwriteBs_time;
	unsigned SMBwritec_count;	/* secondary write request */
	unsigned SMBwritec_time;
	unsigned SMBsetattrE_count;	/* set file attributes expanded */
	unsigned SMBsetattrE_time;
	unsigned SMBgetattrE_count;	/* get file attributes expanded */
	unsigned SMBgetattrE_time;
	unsigned SMBlockingX_count;	/* lock/unlock byte ranges and X */
	unsigned SMBlockingX_time;
	unsigned SMBtrans_count;	/* transaction - name, bytes in/out */
	unsigned SMBtrans_time;
	unsigned SMBtranss_count;	/* transaction (secondary request/response) */
	unsigned SMBtranss_time;
	unsigned SMBioctl_count;	/* IOCTL */
	unsigned SMBioctl_time;
	unsigned SMBioctls_count;	/* IOCTL  (secondary request/response) */
	unsigned SMBioctls_time;
	unsigned SMBcopy_count;		/* copy */
	unsigned SMBcopy_time;
	unsigned SMBmove_count;		/* move */
	unsigned SMBmove_time;
	unsigned SMBecho_count;		/* echo */
	unsigned SMBecho_time;
	unsigned SMBwriteclose_count;	/* write a file then close it */
	unsigned SMBwriteclose_time;
	unsigned SMBopenX_count;	/* open and X */
	unsigned SMBopenX_time;
	unsigned SMBreadX_count;	/* read and X */
	unsigned SMBreadX_time;
	unsigned SMBwriteX_count;	/* write and X */
	unsigned SMBwriteX_time;
	unsigned SMBtrans2_count;	/* TRANS2 protocol set */
	unsigned SMBtrans2_time;
	unsigned SMBtranss2_count;	/* TRANS2 protocol set, secondary command */
	unsigned SMBtranss2_time;
	unsigned SMBfindclose_count;	/* Terminate a TRANSACT2_FINDFIRST */
	unsigned SMBfindclose_time;
	unsigned SMBfindnclose_count;	/* Terminate a TRANSACT2_FINDNOTIFYFIRST */
	unsigned SMBfindnclose_time;
	unsigned SMBtcon_count;		/* tree connect */
	unsigned SMBtcon_time;
	unsigned SMBtdis_count;		/* tree disconnect */
	unsigned SMBtdis_time;
	unsigned SMBnegprot_count;	/* negotiate protocol */
	unsigned SMBnegprot_time;
	unsigned SMBsesssetupX_count;	/* Session Set Up & X (including User Logon) */
	unsigned SMBsesssetupX_time;
	unsigned SMBulogoffX_count;	/* user logoff */
	unsigned SMBulogoffX_time;
	unsigned SMBtconX_count;	/* tree connect and X*/
	unsigned SMBtconX_time;
	unsigned SMBdskattr_count;	/* get disk attributes */
	unsigned SMBdskattr_time;
	unsigned SMBsearch_count;	/* search directory */
	unsigned SMBsearch_time;
	/* SBMffirst stats combined with SMBsearch */
	unsigned SMBffirst_count;	/* find first */
	unsigned SMBffirst_time;
	/* SBMfunique stats combined with SMBsearch */
	unsigned SMBfunique_count;	/* find unique */
	unsigned SMBfunique_time;
	unsigned SMBfclose_count;	/* find close */
	unsigned SMBfclose_time;
	unsigned SMBnttrans_count;	/* NT transact */
	unsigned SMBnttrans_time;
	unsigned SMBnttranss_count;	/* NT transact secondary */
	unsigned SMBnttranss_time;
	unsigned SMBntcreateX_count;	/* NT create and X */
	unsigned SMBntcreateX_time;
	unsigned SMBntcancel_count;	/* NT cancel */
	unsigned SMBntcancel_time;
	unsigned SMBntrename_count;	/* NT rename file */
	unsigned SMBntrename_time;
	unsigned SMBsplopen_count;	/* open print spool file */
	unsigned SMBsplopen_time;
	unsigned SMBsplwr_count;	/* write to print spool file */
	unsigned SMBsplwr_time;
	unsigned SMBsplclose_count;	/* close print spool file */
	unsigned SMBsplclose_time;
	unsigned SMBsplretq_count;	/* return print queue */
	unsigned SMBsplretq_time;
	unsigned SMBsends_count;	/* send single block message */
	unsigned SMBsends_time;
	unsigned SMBsendb_count;	/* send broadcast message */
	unsigned SMBsendb_time;
	unsigned SMBfwdname_count;	/* forward user name */
	unsigned SMBfwdname_time;
	unsigned SMBcancelf_count;	/* cancel forward */
	unsigned SMBcancelf_time;
	unsigned SMBgetmac_count;	/* get machine name */
	unsigned SMBgetmac_time;
	unsigned SMBsendstrt_count;	/* send start of multi-block message */
	unsigned SMBsendstrt_time;
	unsigned SMBsendend_count;	/* send end of multi-block message */
	unsigned SMBsendend_time;
	unsigned SMBsendtxt_count;	/* send text of multi-block message */
	unsigned SMBsendtxt_time;
	unsigned SMBinvalid_count;	/* invalid command */
	unsigned SMBinvalid_time;
/* Pathworks setdir command */
	unsigned pathworks_setdir_count;
	unsigned pathworks_setdir_time;
/* These are the TRANS2 sub commands */
	unsigned Trans2_open_count;
	unsigned Trans2_open_time;
	unsigned Trans2_findfirst_count;
	unsigned Trans2_findfirst_time;
	unsigned Trans2_findnext_count;
	unsigned Trans2_findnext_time;
	unsigned Trans2_qfsinfo_count;
	unsigned Trans2_qfsinfo_time;
	unsigned Trans2_setfsinfo_count;
	unsigned Trans2_setfsinfo_time;
	unsigned Trans2_qpathinfo_count;
	unsigned Trans2_qpathinfo_time;
	unsigned Trans2_setpathinfo_count;
	unsigned Trans2_setpathinfo_time;
	unsigned Trans2_qfileinfo_count;
	unsigned Trans2_qfileinfo_time;
	unsigned Trans2_setfileinfo_count;
	unsigned Trans2_setfileinfo_time;
	unsigned Trans2_fsctl_count;
	unsigned Trans2_fsctl_time;
	unsigned Trans2_ioctl_count;
	unsigned Trans2_ioctl_time;
	unsigned Trans2_findnotifyfirst_count;
	unsigned Trans2_findnotifyfirst_time;
	unsigned Trans2_findnotifynext_count;
	unsigned Trans2_findnotifynext_time;
	unsigned Trans2_mkdir_count;
	unsigned Trans2_mkdir_time;
	unsigned Trans2_session_setup_count;
	unsigned Trans2_session_setup_time;
	unsigned Trans2_get_dfs_referral_count;
	unsigned Trans2_get_dfs_referral_time;
	unsigned Trans2_report_dfs_inconsistancy_count;
	unsigned Trans2_report_dfs_inconsistancy_time;
/* These are the NT transact sub commands. */
	unsigned NT_transact_create_count;
	unsigned NT_transact_create_time;
	unsigned NT_transact_ioctl_count;
	unsigned NT_transact_ioctl_time;
	unsigned NT_transact_set_security_desc_count;
	unsigned NT_transact_set_security_desc_time;
	unsigned NT_transact_notify_change_count;
	unsigned NT_transact_notify_change_time;
	unsigned NT_transact_rename_count;
	unsigned NT_transact_rename_time;
	unsigned NT_transact_query_security_desc_count;
	unsigned NT_transact_query_security_desc_time;
	unsigned NT_transact_get_user_quota_count;
	unsigned NT_transact_get_user_quota_time;
	unsigned NT_transact_set_user_quota_count;
	unsigned NT_transact_set_user_quota_time;
/* These are ACL manipulation calls */
	unsigned get_nt_acl_count;
	unsigned get_nt_acl_time;
	unsigned fget_nt_acl_count;
	unsigned fget_nt_acl_time;
	unsigned set_nt_acl_count;
	unsigned set_nt_acl_time;
	unsigned fset_nt_acl_count;
	unsigned fset_nt_acl_time;
	unsigned chmod_acl_count;
	unsigned chmod_acl_time;
	unsigned fchmod_acl_count;
	unsigned fchmod_acl_time;
/* These are nmbd stats */
	unsigned name_release_count;
	unsigned name_release_time;
	unsigned name_refresh_count;
	unsigned name_refresh_time;
	unsigned name_registration_count;
	unsigned name_registration_time;
	unsigned node_status_count;
	unsigned node_status_time;
	unsigned name_query_count;
	unsigned name_query_time;
	unsigned host_announce_count;
	unsigned host_announce_time;
	unsigned workgroup_announce_count;
	unsigned workgroup_announce_time;
	unsigned local_master_announce_count;
	unsigned local_master_announce_time;
	unsigned master_browser_announce_count;
	unsigned master_browser_announce_time;
	unsigned lm_host_announce_count;
	unsigned lm_host_announce_time;
	unsigned get_backup_list_count;
	unsigned get_backup_list_time;
	unsigned reset_browser_count;
	unsigned reset_browser_time;
	unsigned announce_request_count;
	unsigned announce_request_time;
	unsigned lm_announce_request_count;
	unsigned lm_announce_request_time;
	unsigned domain_logon_count;
	unsigned domain_logon_time;
	unsigned sync_browse_lists_count;
	unsigned sync_browse_lists_time;
	unsigned run_elections_count;
	unsigned run_elections_time;
	unsigned election_count;
	unsigned election_time;
};

struct profile_header {
	int prof_shm_magic;
	int prof_shm_version;
	struct profile_stats stats;
};

extern struct profile_header *profile_h;
extern struct profile_stats *profile_p;
extern struct timeval profile_starttime;
extern struct timeval profile_endtime;
extern struct timeval profile_starttime_nested;
extern struct timeval profile_endtime_nested;
extern BOOL do_profile_flag;
extern BOOL do_profile_times;

#ifdef WITH_PROFILE

/* these are helper macros - do not call them directly in the code
 * use the DO_PROFILE_* START_PROFILE and END_PROFILE ones
 * below which test for the profile flage first
 */
#define INC_PROFILE_COUNT(x) profile_p->x++
#define DEC_PROFILE_COUNT(x) profile_p->x--
#define ADD_PROFILE_COUNT(x,y) profile_p->x += (y)

#if defined(HAVE_CLOCK_GETTIME)

extern clockid_t __profile_clock;

static inline SMB_BIG_UINT profile_timestamp(void)
{
	struct timespec ts;

	/* FIXME: On a single-CPU system, or a system where we have bound
	 * daemon threads to single CPUs (eg. using cpusets or processor
	 * affinity), it might be preferable to use CLOCK_PROCESS_CPUTIME_ID.
	 */

	clock_gettime(__profile_clock, &ts);
	return (ts.tv_sec * 1000000) + (ts.tv_nsec / 1000); /* usec */
}

#else

static inline SMB_BIG_UINT profile_timestamp(void)
{
	struct timeval tv;
	GetTimeOfDay(&tv);
	return (tv.tv_sec * 1000000) + tv.tv_usec;
}

#endif

/* end of helper macros */

#define DO_PROFILE_INC(x) \
	if (do_profile_flag) { \
		INC_PROFILE_COUNT(x); \
	}

#define DO_PROFILE_DEC(x) \
	if (do_profile_flag) { \
		DEC_PROFILE_COUNT(x); \
	}

#define DO_PROFILE_DEC_INC(x,y) \
	if (do_profile_flag) { \
		DEC_PROFILE_COUNT(x); \
		INC_PROFILE_COUNT(y); \
	}

#define DO_PROFILE_ADD(x,n) \
	if (do_profile_flag) { \
		ADD_PROFILE_COUNT(x,n); \
	}

#define START_PROFILE(x) \
	SMB_BIG_UINT __profstamp_##x = 0; \
	if (do_profile_flag) { \
		__profstamp_##x = do_profile_times ? profile_timestamp() : 0;\
		INC_PROFILE_COUNT(x##_count); \
  	}

#define START_PROFILE_BYTES(x,n) \
	SMB_BIG_UINT __profstamp_##x = 0; \
	if (do_profile_flag) { \
		__profstamp_##x = do_profile_times ? profile_timestamp() : 0;\
		INC_PROFILE_COUNT(x##_count); \
		ADD_PROFILE_COUNT(x##_bytes, n); \
  	}

#define END_PROFILE(x) \
	if (do_profile_times) { \
		ADD_PROFILE_COUNT(x##_time, \
		    profile_timestamp() - __profstamp_##x); \
	}

#define START_PROFILE_NESTED(x) START_PROFILE(x)
#define END_PROFILE_NESTED(x) END_PROFILE(x)

#else /* WITH_PROFILE */

#define DO_PROFILE_INC(x)
#define DO_PROFILE_DEC(x)
#define DO_PROFILE_DEC_INC(x,y)
#define DO_PROFILE_ADD(x,n)
#define START_PROFILE(x)
#define START_PROFILE_NESTED(x)
#define START_PROFILE_BYTES(x,n)
#define END_PROFILE(x)
#define END_PROFILE_NESTED(x)

#endif /* WITH_PROFILE */

#endif
