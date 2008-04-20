/* 
   Unix SMB/CIFS implementation.
   status reporting
   Copyright (C) Andrew Tridgell 1994-1998
   
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

   Revision History:

   12 aug 96: Erik.Devriendt@te6.siemens.be
   added support for shared memory implementation of share mode locking

   21-Jul-1998: rsharpe@ns.aus.com (Richard Sharpe)
   Added -L (locks only) -S (shares only) flags and code

*/

/*
 * This program reports current SMB connections
 */

#include "includes.h"

#define SMB_MAXPIDS		2048
static uid_t 		Ucrit_uid = 0;               /* added by OH */
static pid_t		Ucrit_pid[SMB_MAXPIDS];  /* Ugly !!! */   /* added by OH */
static int		Ucrit_MaxPid=0;                    /* added by OH */
static unsigned int	Ucrit_IsActive = 0;                /* added by OH */

static int verbose, brief;
static int            shares_only = 0;            /* Added by RJS */
static int            locks_only  = 0;            /* Added by RJS */
static BOOL processes_only=False;
static int show_brl;
static BOOL numeric_only = False;

const char *username = NULL;

/* added by OH */
static void Ucrit_addUid(uid_t uid)
{
	Ucrit_uid = uid;
	Ucrit_IsActive = 1;
}

static unsigned int Ucrit_checkUid(uid_t uid)
{
	if ( !Ucrit_IsActive ) 
		return 1;
	
	if ( uid == Ucrit_uid ) 
		return 1;
	
	return 0;
}

static unsigned int Ucrit_checkPid(pid_t pid)
{
	int i;
	
	if ( !Ucrit_IsActive ) 
		return 1;
	
	for (i=0;i<Ucrit_MaxPid;i++) {
		if( pid == Ucrit_pid[i] ) 
			return 1;
	}
	
	return 0;
}

static BOOL Ucrit_addPid( pid_t pid )
{
	if ( !Ucrit_IsActive )
		return True;

	if ( Ucrit_MaxPid >= SMB_MAXPIDS ) {
		d_printf("ERROR: More than %d pids for user %s!\n",
			 SMB_MAXPIDS, uidtoname(Ucrit_uid));

		return False;
	}

	Ucrit_pid[Ucrit_MaxPid++] = pid;
	
	return True;
}

static void print_share_mode(const struct share_mode_entry *e, const char *sharepath, const char *fname)
{
	static int count;

	if (!is_valid_share_mode_entry(e)) {
		return;
	}

	if (count==0) {
		d_printf("Locked files:\n");
		d_printf("Pid          Uid        DenyMode   Access      R/W        Oplock           SharePath   Name   Time\n");
		d_printf("--------------------------------------------------------------------------------------------------\n");
	}
	count++;

	if (Ucrit_checkPid(procid_to_pid(&e->pid))) {
		d_printf("%-11s  ",procid_str_static(&e->pid));
		d_printf("%-9u  ", (unsigned int)e->uid);
		switch (map_share_mode_to_deny_mode(e->share_access,
						    e->private_options)) {
			case DENY_NONE: d_printf("DENY_NONE  "); break;
			case DENY_ALL:  d_printf("DENY_ALL   "); break;
			case DENY_DOS:  d_printf("DENY_DOS   "); break;
			case DENY_READ: d_printf("DENY_READ  "); break;
			case DENY_WRITE:printf("DENY_WRITE "); break;
			case DENY_FCB:  d_printf("DENY_FCB "); break;
			default: {
				d_printf("unknown-please report ! "
					 "e->share_access = 0x%x, "
					 "e->private_options = 0x%x\n",
					 (unsigned int)e->share_access,
					 (unsigned int)e->private_options );
				break;
			}
		}
		d_printf("0x%-8x  ",(unsigned int)e->access_mask);
		if ((e->access_mask & (FILE_READ_DATA|FILE_WRITE_DATA))==
				(FILE_READ_DATA|FILE_WRITE_DATA)) {
			d_printf("RDWR       ");
		} else if (e->access_mask & FILE_WRITE_DATA) {
			d_printf("WRONLY     ");
		} else {
			d_printf("RDONLY     ");
		}

		if((e->op_type & (EXCLUSIVE_OPLOCK|BATCH_OPLOCK)) == 
					(EXCLUSIVE_OPLOCK|BATCH_OPLOCK)) {
			d_printf("EXCLUSIVE+BATCH ");
		} else if (e->op_type & EXCLUSIVE_OPLOCK) {
			d_printf("EXCLUSIVE       ");
		} else if (e->op_type & BATCH_OPLOCK) {
			d_printf("BATCH           ");
		} else if (e->op_type & LEVEL_II_OPLOCK) {
			d_printf("LEVEL_II        ");
		} else {
			d_printf("NONE            ");
		}

		d_printf(" %s   %s   %s",sharepath, fname, time_to_asc((time_t *)&e->time.tv_sec));
	}
}

static void print_brl(SMB_DEV_T dev,
			SMB_INO_T ino,
			struct process_id pid, 
			enum brl_type lock_type,
			enum brl_flavour lock_flav,
			br_off start,
			br_off size)
{
	static int count;
	if (count==0) {
		d_printf("Byte range locks:\n");
		d_printf("   Pid     dev:inode  R/W      start        size\n");
		d_printf("------------------------------------------------\n");
	}
	count++;

	d_printf("%8s   %05x:%05x    %s  %9.0f   %9.0f\n", 
	       procid_str_static(&pid), (int)dev, (int)ino, 
	       lock_type==READ_LOCK?"R":"W",
	       (double)start, (double)size);
}


/*******************************************************************
 dump the elements of the profile structure
  ******************************************************************/
static int profile_dump(void)
{
#ifdef WITH_PROFILE
	if (!profile_setup(True)) {
		fprintf(stderr,"Failed to initialise profile memory\n");
		return -1;
	}

	d_printf("smb_count:                      %u\n", profile_p->smb_count);
	d_printf("uid_changes:                    %u\n", profile_p->uid_changes);
	d_printf("************************ System Calls ****************************\n");
	d_printf("opendir_count:                  %u\n", profile_p->syscall_opendir_count);
	d_printf("opendir_time:                   %u\n", profile_p->syscall_opendir_time);
	d_printf("readdir_count:                  %u\n", profile_p->syscall_readdir_count);
	d_printf("readdir_time:                   %u\n", profile_p->syscall_readdir_time);
	d_printf("mkdir_count:                    %u\n", profile_p->syscall_mkdir_count);
	d_printf("mkdir_time:                     %u\n", profile_p->syscall_mkdir_time);
	d_printf("rmdir_count:                    %u\n", profile_p->syscall_rmdir_count);
	d_printf("rmdir_time:                     %u\n", profile_p->syscall_rmdir_time);
	d_printf("closedir_count:                 %u\n", profile_p->syscall_closedir_count);
	d_printf("closedir_time:                  %u\n", profile_p->syscall_closedir_time);
	d_printf("open_count:                     %u\n", profile_p->syscall_open_count);
	d_printf("open_time:                      %u\n", profile_p->syscall_open_time);
	d_printf("close_count:                    %u\n", profile_p->syscall_close_count);
	d_printf("close_time:                     %u\n", profile_p->syscall_close_time);
	d_printf("read_count:                     %u\n", profile_p->syscall_read_count);
	d_printf("read_time:                      %u\n", profile_p->syscall_read_time);
	d_printf("read_bytes:                     %u\n", profile_p->syscall_read_bytes);
	d_printf("write_count:                    %u\n", profile_p->syscall_write_count);
	d_printf("write_time:                     %u\n", profile_p->syscall_write_time);
	d_printf("write_bytes:                    %u\n", profile_p->syscall_write_bytes);
	d_printf("pread_count:                    %u\n", profile_p->syscall_pread_count);
	d_printf("pread_time:                     %u\n", profile_p->syscall_pread_time);
	d_printf("pread_bytes:                    %u\n", profile_p->syscall_pread_bytes);
	d_printf("pwrite_count:                   %u\n", profile_p->syscall_pwrite_count);
	d_printf("pwrite_time:                    %u\n", profile_p->syscall_pwrite_time);
	d_printf("pwrite_bytes:                   %u\n", profile_p->syscall_pwrite_bytes);
#ifdef WITH_SENDFILE
	d_printf("sendfile_count:                 %u\n", profile_p->syscall_sendfile_count);
	d_printf("sendfile_time:                  %u\n", profile_p->syscall_sendfile_time);
	d_printf("sendfile_bytes:                 %u\n", profile_p->syscall_sendfile_bytes);
#endif
	d_printf("lseek_count:                    %u\n", profile_p->syscall_lseek_count);
	d_printf("lseek_time:                     %u\n", profile_p->syscall_lseek_time);
	d_printf("rename_count:                   %u\n", profile_p->syscall_rename_count);
	d_printf("rename_time:                    %u\n", profile_p->syscall_rename_time);
	d_printf("fsync_count:                    %u\n", profile_p->syscall_fsync_count);
	d_printf("fsync_time:                     %u\n", profile_p->syscall_fsync_time);
	d_printf("stat_count:                     %u\n", profile_p->syscall_stat_count);
	d_printf("stat_time:                      %u\n", profile_p->syscall_stat_time);
	d_printf("fstat_count:                    %u\n", profile_p->syscall_fstat_count);
	d_printf("fstat_time:                     %u\n", profile_p->syscall_fstat_time);
	d_printf("lstat_count:                    %u\n", profile_p->syscall_lstat_count);
	d_printf("lstat_time:                     %u\n", profile_p->syscall_lstat_time);
	d_printf("unlink_count:                   %u\n", profile_p->syscall_unlink_count);
	d_printf("unlink_time:                    %u\n", profile_p->syscall_unlink_time);
	d_printf("chmod_count:                    %u\n", profile_p->syscall_chmod_count);
	d_printf("chmod_time:                     %u\n", profile_p->syscall_chmod_time);
	d_printf("fchmod_count:                   %u\n", profile_p->syscall_fchmod_count);
	d_printf("fchmod_time:                    %u\n", profile_p->syscall_fchmod_time);
	d_printf("chown_count:                    %u\n", profile_p->syscall_chown_count);
	d_printf("chown_time:                     %u\n", profile_p->syscall_chown_time);
	d_printf("fchown_count:                   %u\n", profile_p->syscall_fchown_count);
	d_printf("fchown_time:                    %u\n", profile_p->syscall_fchown_time);
	d_printf("chdir_count:                    %u\n", profile_p->syscall_chdir_count);
	d_printf("chdir_time:                     %u\n", profile_p->syscall_chdir_time);
	d_printf("getwd_count:                    %u\n", profile_p->syscall_getwd_count);
	d_printf("getwd_time:                     %u\n", profile_p->syscall_getwd_time);
	d_printf("utime_count:                    %u\n", profile_p->syscall_utime_count);
	d_printf("utime_time:                     %u\n", profile_p->syscall_utime_time);
	d_printf("ftruncate_count:                %u\n", profile_p->syscall_ftruncate_count);
	d_printf("ftruncate_time:                 %u\n", profile_p->syscall_ftruncate_time);
	d_printf("fcntl_lock_count:               %u\n", profile_p->syscall_fcntl_lock_count);
	d_printf("fcntl_lock_time:                %u\n", profile_p->syscall_fcntl_lock_time);
	d_printf("readlink_count:                 %u\n", profile_p->syscall_readlink_count);
	d_printf("readlink_time:                  %u\n", profile_p->syscall_readlink_time);
	d_printf("symlink_count:                  %u\n", profile_p->syscall_symlink_count);
	d_printf("symlink_time:                   %u\n", profile_p->syscall_symlink_time);
	d_printf("************************ Statcache *******************************\n");
	d_printf("lookups:                        %u\n", profile_p->statcache_lookups);
	d_printf("misses:                         %u\n", profile_p->statcache_misses);
	d_printf("hits:                           %u\n", profile_p->statcache_hits);
	d_printf("************************ Writecache ******************************\n");
	d_printf("read_hits:                      %u\n", profile_p->writecache_read_hits);
	d_printf("abutted_writes:                 %u\n", profile_p->writecache_abutted_writes);
	d_printf("total_writes:                   %u\n", profile_p->writecache_total_writes);
	d_printf("non_oplock_writes:              %u\n", profile_p->writecache_non_oplock_writes);
	d_printf("direct_writes:                  %u\n", profile_p->writecache_direct_writes);
	d_printf("init_writes:                    %u\n", profile_p->writecache_init_writes);
	d_printf("flushed_writes[SEEK]:           %u\n", profile_p->writecache_flushed_writes[SEEK_FLUSH]);
	d_printf("flushed_writes[READ]:           %u\n", profile_p->writecache_flushed_writes[READ_FLUSH]);
	d_printf("flushed_writes[WRITE]:          %u\n", profile_p->writecache_flushed_writes[WRITE_FLUSH]);
	d_printf("flushed_writes[READRAW]:        %u\n", profile_p->writecache_flushed_writes[READRAW_FLUSH]);
	d_printf("flushed_writes[OPLOCK_RELEASE]: %u\n", profile_p->writecache_flushed_writes[OPLOCK_RELEASE_FLUSH]);
	d_printf("flushed_writes[CLOSE]:          %u\n", profile_p->writecache_flushed_writes[CLOSE_FLUSH]);
	d_printf("flushed_writes[SYNC]:           %u\n", profile_p->writecache_flushed_writes[SYNC_FLUSH]);
	d_printf("flushed_writes[SIZECHANGE]:     %u\n", profile_p->writecache_flushed_writes[SIZECHANGE_FLUSH]);
	d_printf("num_perfect_writes:             %u\n", profile_p->writecache_num_perfect_writes);
	d_printf("num_write_caches:               %u\n", profile_p->writecache_num_write_caches);
	d_printf("allocated_write_caches:         %u\n", profile_p->writecache_allocated_write_caches);
	d_printf("************************ SMB Calls *******************************\n");
	d_printf("mkdir_count:                    %u\n", profile_p->SMBmkdir_count);
	d_printf("mkdir_time:                     %u\n", profile_p->SMBmkdir_time);
	d_printf("rmdir_count:                    %u\n", profile_p->SMBrmdir_count);
	d_printf("rmdir_time:                     %u\n", profile_p->SMBrmdir_time);
	d_printf("open_count:                     %u\n", profile_p->SMBopen_count);
	d_printf("open_time:                      %u\n", profile_p->SMBopen_time);
	d_printf("create_count:                   %u\n", profile_p->SMBcreate_count);
	d_printf("create_time:                    %u\n", profile_p->SMBcreate_time);
	d_printf("close_count:                    %u\n", profile_p->SMBclose_count);
	d_printf("close_time:                     %u\n", profile_p->SMBclose_time);
	d_printf("flush_count:                    %u\n", profile_p->SMBflush_count);
	d_printf("flush_time:                     %u\n", profile_p->SMBflush_time);
	d_printf("unlink_count:                   %u\n", profile_p->SMBunlink_count);
	d_printf("unlink_time:                    %u\n", profile_p->SMBunlink_time);
	d_printf("mv_count:                       %u\n", profile_p->SMBmv_count);
	d_printf("mv_time:                        %u\n", profile_p->SMBmv_time);
	d_printf("getatr_count:                   %u\n", profile_p->SMBgetatr_count);
	d_printf("getatr_time:                    %u\n", profile_p->SMBgetatr_time);
	d_printf("setatr_count:                   %u\n", profile_p->SMBsetatr_count);
	d_printf("setatr_time:                    %u\n", profile_p->SMBsetatr_time);
	d_printf("read_count:                     %u\n", profile_p->SMBread_count);
	d_printf("read_time:                      %u\n", profile_p->SMBread_time);
	d_printf("write_count:                    %u\n", profile_p->SMBwrite_count);
	d_printf("write_time:                     %u\n", profile_p->SMBwrite_time);
	d_printf("lock_count:                     %u\n", profile_p->SMBlock_count);
	d_printf("lock_time:                      %u\n", profile_p->SMBlock_time);
	d_printf("unlock_count:                   %u\n", profile_p->SMBunlock_count);
	d_printf("unlock_time:                    %u\n", profile_p->SMBunlock_time);
	d_printf("ctemp_count:                    %u\n", profile_p->SMBctemp_count);
	d_printf("ctemp_time:                     %u\n", profile_p->SMBctemp_time);
	d_printf("mknew_count:                    %u\n", profile_p->SMBmknew_count);
	d_printf("mknew_time:                     %u\n", profile_p->SMBmknew_time);
	d_printf("chkpth_count:                   %u\n", profile_p->SMBchkpth_count);
	d_printf("chkpth_time:                    %u\n", profile_p->SMBchkpth_time);
	d_printf("exit_count:                     %u\n", profile_p->SMBexit_count);
	d_printf("exit_time:                      %u\n", profile_p->SMBexit_time);
	d_printf("lseek_count:                    %u\n", profile_p->SMBlseek_count);
	d_printf("lseek_time:                     %u\n", profile_p->SMBlseek_time);
	d_printf("lockread_count:                 %u\n", profile_p->SMBlockread_count);
	d_printf("lockread_time:                  %u\n", profile_p->SMBlockread_time);
	d_printf("writeunlock_count:              %u\n", profile_p->SMBwriteunlock_count);
	d_printf("writeunlock_time:               %u\n", profile_p->SMBwriteunlock_time);
	d_printf("readbraw_count:                 %u\n", profile_p->SMBreadbraw_count);
	d_printf("readbraw_time:                  %u\n", profile_p->SMBreadbraw_time);
	d_printf("readBmpx_count:                 %u\n", profile_p->SMBreadBmpx_count);
	d_printf("readBmpx_time:                  %u\n", profile_p->SMBreadBmpx_time);
	d_printf("readBs_count:                   %u\n", profile_p->SMBreadBs_count);
	d_printf("readBs_time:                    %u\n", profile_p->SMBreadBs_time);
	d_printf("writebraw_count:                %u\n", profile_p->SMBwritebraw_count);
	d_printf("writebraw_time:                 %u\n", profile_p->SMBwritebraw_time);
	d_printf("writeBmpx_count:                %u\n", profile_p->SMBwriteBmpx_count);
	d_printf("writeBmpx_time:                 %u\n", profile_p->SMBwriteBmpx_time);
	d_printf("writeBs_count:                  %u\n", profile_p->SMBwriteBs_count);
	d_printf("writeBs_time:                   %u\n", profile_p->SMBwriteBs_time);
	d_printf("writec_count:                   %u\n", profile_p->SMBwritec_count);
	d_printf("writec_time:                    %u\n", profile_p->SMBwritec_time);
	d_printf("setattrE_count:                 %u\n", profile_p->SMBsetattrE_count);
	d_printf("setattrE_time:                  %u\n", profile_p->SMBsetattrE_time);
	d_printf("getattrE_count:                 %u\n", profile_p->SMBgetattrE_count);
	d_printf("getattrE_time:                  %u\n", profile_p->SMBgetattrE_time);
	d_printf("lockingX_count:                 %u\n", profile_p->SMBlockingX_count);
	d_printf("lockingX_time:                  %u\n", profile_p->SMBlockingX_time);
	d_printf("trans_count:                    %u\n", profile_p->SMBtrans_count);
	d_printf("trans_time:                     %u\n", profile_p->SMBtrans_time);
	d_printf("transs_count:                   %u\n", profile_p->SMBtranss_count);
	d_printf("transs_time:                    %u\n", profile_p->SMBtranss_time);
	d_printf("ioctl_count:                    %u\n", profile_p->SMBioctl_count);
	d_printf("ioctl_time:                     %u\n", profile_p->SMBioctl_time);
	d_printf("ioctls_count:                   %u\n", profile_p->SMBioctls_count);
	d_printf("ioctls_time:                    %u\n", profile_p->SMBioctls_time);
	d_printf("copy_count:                     %u\n", profile_p->SMBcopy_count);
	d_printf("copy_time:                      %u\n", profile_p->SMBcopy_time);
	d_printf("move_count:                     %u\n", profile_p->SMBmove_count);
	d_printf("move_time:                      %u\n", profile_p->SMBmove_time);
	d_printf("echo_count:                     %u\n", profile_p->SMBecho_count);
	d_printf("echo_time:                      %u\n", profile_p->SMBecho_time);
	d_printf("writeclose_count:               %u\n", profile_p->SMBwriteclose_count);
	d_printf("writeclose_time:                %u\n", profile_p->SMBwriteclose_time);
	d_printf("openX_count:                    %u\n", profile_p->SMBopenX_count);
	d_printf("openX_time:                     %u\n", profile_p->SMBopenX_time);
	d_printf("readX_count:                    %u\n", profile_p->SMBreadX_count);
	d_printf("readX_time:                     %u\n", profile_p->SMBreadX_time);
	d_printf("writeX_count:                   %u\n", profile_p->SMBwriteX_count);
	d_printf("writeX_time:                    %u\n", profile_p->SMBwriteX_time);
	d_printf("trans2_count:                   %u\n", profile_p->SMBtrans2_count);
	d_printf("trans2_time:                    %u\n", profile_p->SMBtrans2_time);
	d_printf("transs2_count:                  %u\n", profile_p->SMBtranss2_count);
	d_printf("transs2_time:                   %u\n", profile_p->SMBtranss2_time);
	d_printf("findclose_count:                %u\n", profile_p->SMBfindclose_count);
	d_printf("findclose_time:                 %u\n", profile_p->SMBfindclose_time);
	d_printf("findnclose_count:               %u\n", profile_p->SMBfindnclose_count);
	d_printf("findnclose_time:                %u\n", profile_p->SMBfindnclose_time);
	d_printf("tcon_count:                     %u\n", profile_p->SMBtcon_count);
	d_printf("tcon_time:                      %u\n", profile_p->SMBtcon_time);
	d_printf("tdis_count:                     %u\n", profile_p->SMBtdis_count);
	d_printf("tdis_time:                      %u\n", profile_p->SMBtdis_time);
	d_printf("negprot_count:                  %u\n", profile_p->SMBnegprot_count);
	d_printf("negprot_time:                   %u\n", profile_p->SMBnegprot_time);
	d_printf("sesssetupX_count:               %u\n", profile_p->SMBsesssetupX_count);
	d_printf("sesssetupX_time:                %u\n", profile_p->SMBsesssetupX_time);
	d_printf("ulogoffX_count:                 %u\n", profile_p->SMBulogoffX_count);
	d_printf("ulogoffX_time:                  %u\n", profile_p->SMBulogoffX_time);
	d_printf("tconX_count:                    %u\n", profile_p->SMBtconX_count);
	d_printf("tconX_time:                     %u\n", profile_p->SMBtconX_time);
	d_printf("dskattr_count:                  %u\n", profile_p->SMBdskattr_count);
	d_printf("dskattr_time:                   %u\n", profile_p->SMBdskattr_time);
	d_printf("search_count:                   %u\n", profile_p->SMBsearch_count);
	d_printf("search_time:                    %u\n", profile_p->SMBsearch_time);
	d_printf("ffirst_count:                   %u\n", profile_p->SMBffirst_count);
	d_printf("ffirst_time:                    %u\n", profile_p->SMBffirst_time);
	d_printf("funique_count:                  %u\n", profile_p->SMBfunique_count);
	d_printf("funique_time:                   %u\n", profile_p->SMBfunique_time);
	d_printf("fclose_count:                   %u\n", profile_p->SMBfclose_count);
	d_printf("fclose_time:                    %u\n", profile_p->SMBfclose_time);
	d_printf("nttrans_count:                  %u\n", profile_p->SMBnttrans_count);
	d_printf("nttrans_time:                   %u\n", profile_p->SMBnttrans_time);
	d_printf("nttranss_count:                 %u\n", profile_p->SMBnttranss_count);
	d_printf("nttranss_time:                  %u\n", profile_p->SMBnttranss_time);
	d_printf("ntcreateX_count:                %u\n", profile_p->SMBntcreateX_count);
	d_printf("ntcreateX_time:                 %u\n", profile_p->SMBntcreateX_time);
	d_printf("ntcancel_count:                 %u\n", profile_p->SMBntcancel_count);
	d_printf("ntcancel_time:                  %u\n", profile_p->SMBntcancel_time);
	d_printf("splopen_count:                  %u\n", profile_p->SMBsplopen_count);
	d_printf("splopen_time:                   %u\n", profile_p->SMBsplopen_time);
	d_printf("splwr_count:                    %u\n", profile_p->SMBsplwr_count);
	d_printf("splwr_time:                     %u\n", profile_p->SMBsplwr_time);
	d_printf("splclose_count:                 %u\n", profile_p->SMBsplclose_count);
	d_printf("splclose_time:                  %u\n", profile_p->SMBsplclose_time);
	d_printf("splretq_count:                  %u\n", profile_p->SMBsplretq_count);
	d_printf("splretq_time:                   %u\n", profile_p->SMBsplretq_time);
	d_printf("sends_count:                    %u\n", profile_p->SMBsends_count);
	d_printf("sends_time:                     %u\n", profile_p->SMBsends_time);
	d_printf("sendb_count:                    %u\n", profile_p->SMBsendb_count);
	d_printf("sendb_time:                     %u\n", profile_p->SMBsendb_time);
	d_printf("fwdname_count:                  %u\n", profile_p->SMBfwdname_count);
	d_printf("fwdname_time:                   %u\n", profile_p->SMBfwdname_time);
	d_printf("cancelf_count:                  %u\n", profile_p->SMBcancelf_count);
	d_printf("cancelf_time:                   %u\n", profile_p->SMBcancelf_time);
	d_printf("getmac_count:                   %u\n", profile_p->SMBgetmac_count);
	d_printf("getmac_time:                    %u\n", profile_p->SMBgetmac_time);
	d_printf("sendstrt_count:                 %u\n", profile_p->SMBsendstrt_count);
	d_printf("sendstrt_time:                  %u\n", profile_p->SMBsendstrt_time);
	d_printf("sendend_count:                  %u\n", profile_p->SMBsendend_count);
	d_printf("sendend_time:                   %u\n", profile_p->SMBsendend_time);
	d_printf("sendtxt_count:                  %u\n", profile_p->SMBsendtxt_count);
	d_printf("sendtxt_time:                   %u\n", profile_p->SMBsendtxt_time);
	d_printf("invalid_count:                  %u\n", profile_p->SMBinvalid_count);
	d_printf("invalid_time:                   %u\n", profile_p->SMBinvalid_time);
	d_printf("************************ Pathworks Calls *************************\n");
	d_printf("setdir_count:                   %u\n", profile_p->pathworks_setdir_count);
	d_printf("setdir_time:                    %u\n", profile_p->pathworks_setdir_time);
	d_printf("************************ Trans2 Calls ****************************\n");
	d_printf("open_count:                     %u\n", profile_p->Trans2_open_count);
	d_printf("open_time:                      %u\n", profile_p->Trans2_open_time);
	d_printf("findfirst_count:                %u\n", profile_p->Trans2_findfirst_count);
	d_printf("findfirst_time:                 %u\n", profile_p->Trans2_findfirst_time);
	d_printf("findnext_count:                 %u\n", profile_p->Trans2_findnext_count);
	d_printf("findnext_time:                  %u\n", profile_p->Trans2_findnext_time);
	d_printf("qfsinfo_count:                  %u\n", profile_p->Trans2_qfsinfo_count);
	d_printf("qfsinfo_time:                   %u\n", profile_p->Trans2_qfsinfo_time);
	d_printf("setfsinfo_count:                %u\n", profile_p->Trans2_setfsinfo_count);
	d_printf("setfsinfo_time:                 %u\n", profile_p->Trans2_setfsinfo_time);
	d_printf("qpathinfo_count:                %u\n", profile_p->Trans2_qpathinfo_count);
	d_printf("qpathinfo_time:                 %u\n", profile_p->Trans2_qpathinfo_time);
	d_printf("setpathinfo_count:              %u\n", profile_p->Trans2_setpathinfo_count);
	d_printf("setpathinfo_time:               %u\n", profile_p->Trans2_setpathinfo_time);
	d_printf("qfileinfo_count:                %u\n", profile_p->Trans2_qfileinfo_count);
	d_printf("qfileinfo_time:                 %u\n", profile_p->Trans2_qfileinfo_time);
	d_printf("setfileinfo_count:              %u\n", profile_p->Trans2_setfileinfo_count);
	d_printf("setfileinfo_time:               %u\n", profile_p->Trans2_setfileinfo_time);
	d_printf("fsctl_count:                    %u\n", profile_p->Trans2_fsctl_count);
	d_printf("fsctl_time:                     %u\n", profile_p->Trans2_fsctl_time);
	d_printf("ioctl_count:                    %u\n", profile_p->Trans2_ioctl_count);
	d_printf("ioctl_time:                     %u\n", profile_p->Trans2_ioctl_time);
	d_printf("findnotifyfirst_count:          %u\n", profile_p->Trans2_findnotifyfirst_count);
	d_printf("findnotifyfirst_time:           %u\n", profile_p->Trans2_findnotifyfirst_time);
	d_printf("findnotifynext_count:           %u\n", profile_p->Trans2_findnotifynext_count);
	d_printf("findnotifynext_time:            %u\n", profile_p->Trans2_findnotifynext_time);
	d_printf("mkdir_count:                    %u\n", profile_p->Trans2_mkdir_count);
	d_printf("mkdir_time:                     %u\n", profile_p->Trans2_mkdir_time);
	d_printf("session_setup_count:            %u\n", profile_p->Trans2_session_setup_count);
	d_printf("session_setup_time:             %u\n", profile_p->Trans2_session_setup_time);
	d_printf("get_dfs_referral_count:         %u\n", profile_p->Trans2_get_dfs_referral_count);
	d_printf("get_dfs_referral_time:          %u\n", profile_p->Trans2_get_dfs_referral_time);
	d_printf("report_dfs_inconsistancy_count: %u\n", profile_p->Trans2_report_dfs_inconsistancy_count);
	d_printf("report_dfs_inconsistancy_time:  %u\n", profile_p->Trans2_report_dfs_inconsistancy_time);
	d_printf("************************ NT Transact Calls ***********************\n");
	d_printf("create_count:                   %u\n", profile_p->NT_transact_create_count);
	d_printf("create_time:                    %u\n", profile_p->NT_transact_create_time);
	d_printf("ioctl_count:                    %u\n", profile_p->NT_transact_ioctl_count);
	d_printf("ioctl_time:                     %u\n", profile_p->NT_transact_ioctl_time);
	d_printf("set_security_desc_count:        %u\n", profile_p->NT_transact_set_security_desc_count);
	d_printf("set_security_desc_time:         %u\n", profile_p->NT_transact_set_security_desc_time);
	d_printf("notify_change_count:            %u\n", profile_p->NT_transact_notify_change_count);
	d_printf("notify_change_time:             %u\n", profile_p->NT_transact_notify_change_time);
	d_printf("rename_count:                   %u\n", profile_p->NT_transact_rename_count);
	d_printf("rename_time:                    %u\n", profile_p->NT_transact_rename_time);
	d_printf("query_security_desc_count:      %u\n", profile_p->NT_transact_query_security_desc_count);
	d_printf("query_security_desc_time:       %u\n", profile_p->NT_transact_query_security_desc_time);
	d_printf("************************ ACL Calls *******************************\n");
	d_printf("get_nt_acl_count:               %u\n", profile_p->get_nt_acl_count);
	d_printf("get_nt_acl_time:                %u\n", profile_p->get_nt_acl_time);
	d_printf("fget_nt_acl_count:              %u\n", profile_p->fget_nt_acl_count);
	d_printf("fget_nt_acl_time:               %u\n", profile_p->fget_nt_acl_time);
	d_printf("set_nt_acl_count:               %u\n", profile_p->set_nt_acl_count);
	d_printf("set_nt_acl_time:                %u\n", profile_p->set_nt_acl_time);
	d_printf("fset_nt_acl_count:              %u\n", profile_p->fset_nt_acl_count);
	d_printf("fset_nt_acl_time:               %u\n", profile_p->fset_nt_acl_time);
	d_printf("chmod_acl_count:                %u\n", profile_p->chmod_acl_count);
	d_printf("chmod_acl_time:                 %u\n", profile_p->chmod_acl_time);
	d_printf("fchmod_acl_count:               %u\n", profile_p->fchmod_acl_count);
	d_printf("fchmod_acl_time:                %u\n", profile_p->fchmod_acl_time);
	d_printf("************************ NMBD Calls ****************************\n");
	d_printf("name_release_count:             %u\n", profile_p->name_release_count);
	d_printf("name_release_time:              %u\n", profile_p->name_release_time);
	d_printf("name_refresh_count:             %u\n", profile_p->name_refresh_count);
	d_printf("name_refresh_time:              %u\n", profile_p->name_refresh_time);
	d_printf("name_registration_count:        %u\n", profile_p->name_registration_count);
	d_printf("name_registration_time:         %u\n", profile_p->name_registration_time);
	d_printf("node_status_count:              %u\n", profile_p->node_status_count);
	d_printf("node_status_time:               %u\n", profile_p->node_status_time);
	d_printf("name_query_count:               %u\n", profile_p->name_query_count);
	d_printf("name_query_time:                %u\n", profile_p->name_query_time);
	d_printf("host_announce_count:            %u\n", profile_p->host_announce_count);
	d_printf("host_announce_time:             %u\n", profile_p->host_announce_time);
	d_printf("workgroup_announce_count:       %u\n", profile_p->workgroup_announce_count);
	d_printf("workgroup_announce_time:        %u\n", profile_p->workgroup_announce_time);
	d_printf("local_master_announce_count:    %u\n", profile_p->local_master_announce_count);
	d_printf("local_master_announce_time:     %u\n", profile_p->local_master_announce_time);
	d_printf("master_browser_announce_count:  %u\n", profile_p->master_browser_announce_count);
	d_printf("master_browser_announce_time:   %u\n", profile_p->master_browser_announce_time);
	d_printf("lm_host_announce_count:         %u\n", profile_p->lm_host_announce_count);
	d_printf("lm_host_announce_time:          %u\n", profile_p->lm_host_announce_time);
	d_printf("get_backup_list_count:          %u\n", profile_p->get_backup_list_count);
	d_printf("get_backup_list_time:           %u\n", profile_p->get_backup_list_time);
	d_printf("reset_browser_count:            %u\n", profile_p->reset_browser_count);
	d_printf("reset_browser_time:             %u\n", profile_p->reset_browser_time);
	d_printf("announce_request_count:         %u\n", profile_p->announce_request_count);
	d_printf("announce_request_time:          %u\n", profile_p->announce_request_time);
	d_printf("lm_announce_request_count:      %u\n", profile_p->lm_announce_request_count);
	d_printf("lm_announce_request_time:       %u\n", profile_p->lm_announce_request_time);
	d_printf("domain_logon_count:             %u\n", profile_p->domain_logon_count);
	d_printf("domain_logon_time:              %u\n", profile_p->domain_logon_time);
	d_printf("sync_browse_lists_count:        %u\n", profile_p->sync_browse_lists_count);
	d_printf("sync_browse_lists_time:         %u\n", profile_p->sync_browse_lists_time);
	d_printf("run_elections_count:            %u\n", profile_p->run_elections_count);
	d_printf("run_elections_time:             %u\n", profile_p->run_elections_time);
	d_printf("election_count:                 %u\n", profile_p->election_count);
	d_printf("election_time:                  %u\n", profile_p->election_time);
#else /* WITH_PROFILE */
	fprintf(stderr, "Profile data unavailable\n");
#endif /* WITH_PROFILE */

	return 0;
}


static int traverse_fn1(TDB_CONTEXT *tdb, TDB_DATA kbuf, TDB_DATA dbuf, void *state)
{
	struct connections_data crec;

	if (dbuf.dsize != sizeof(crec))
		return 0;

	memcpy(&crec, dbuf.dptr, sizeof(crec));

	if (crec.cnum == -1)
		return 0;

	if (!process_exists(crec.pid) || !Ucrit_checkUid(crec.uid)) {
		return 0;
	}

	d_printf("%-10s   %s   %-12s  %s",
	       crec.name,procid_str_static(&crec.pid),
	       crec.machine,
	       time_to_asc(&crec.start));

	return 0;
}

static int traverse_sessionid(TDB_CONTEXT *tdb, TDB_DATA kbuf, TDB_DATA dbuf, void *state)
{
	struct sessionid sessionid;
	fstring uid_str, gid_str;

	if (dbuf.dsize != sizeof(sessionid))
		return 0;

	memcpy(&sessionid, dbuf.dptr, sizeof(sessionid));

	if (!process_exists_by_pid(sessionid.pid) || !Ucrit_checkUid(sessionid.uid)) {
		return 0;
	}

	Ucrit_addPid( sessionid.pid );

	fstr_sprintf(uid_str, "%d", sessionid.uid);
	fstr_sprintf(gid_str, "%d", sessionid.gid);

	d_printf("%5d   %-12s  %-12s  %-12s (%s)\n",
		 (int)sessionid.pid,
		 numeric_only ? uid_str : uidtoname(sessionid.uid),
		 numeric_only ? gid_str : gidtoname(sessionid.gid), 
		 sessionid.remote_machine, sessionid.hostname);
	
	return 0;
}




 int main(int argc, char *argv[])
{
	int c;
	static int profile_only = 0;
	TDB_CONTEXT *tdb;
	BOOL show_processes, show_locks, show_shares;
	poptContext pc;
	struct poptOption long_options[] = {
		POPT_AUTOHELP
		{"processes",	'p', POPT_ARG_NONE,	&processes_only, 'p', "Show processes only" },
		{"verbose",	'v', POPT_ARG_NONE, &verbose, 'v', "Be verbose" },
		{"locks",	'L', POPT_ARG_NONE,	&locks_only, 'L', "Show locks only" },
		{"shares",	'S', POPT_ARG_NONE,	&shares_only, 'S', "Show shares only" },
		{"user", 	'u', POPT_ARG_STRING,	&username, 'u', "Switch to user" },
		{"brief",	'b', POPT_ARG_NONE, 	&brief, 'b', "Be brief" },
#ifdef WITH_PROFILE
		{"profile",	'P', POPT_ARG_NONE,	&profile_only, 'P', "Do profiling" },
#endif /* WITH_PROFILE */
		{"byterange",	'B', POPT_ARG_NONE,	&show_brl, 'B', "Include byte range locks"},
		{"numeric",	'n', POPT_ARG_NONE,	&numeric_only, 'n', "Numeric uid/gid"},
		POPT_COMMON_SAMBA
		POPT_TABLEEND
	};

	sec_init();
	load_case_tables();

	setup_logging(argv[0],True);
	
	dbf = x_stderr;
	
	if (getuid() != geteuid()) {
		d_printf("smbstatus should not be run setuid\n");
		return(1);
	}

	pc = poptGetContext(NULL, argc, (const char **) argv, long_options, 
			    POPT_CONTEXT_KEEP_FIRST);
	
	while ((c = poptGetNextOpt(pc)) != -1) {
		switch (c) {
		case 'u':                                      
			Ucrit_addUid(nametouid(poptGetOptArg(pc)));
			break;
		}
	}

	/* setup the flags based on the possible combincations */

	show_processes = !(shares_only || locks_only || profile_only) || processes_only;
	show_locks     = !(shares_only || processes_only || profile_only) || locks_only;
	show_shares    = !(processes_only || locks_only || profile_only) || shares_only;

	if ( username )
		Ucrit_addUid( nametouid(username) );

	if (verbose) {
		d_printf("using configfile = %s\n", dyn_CONFIGFILE);
	}

	if (!lp_load(dyn_CONFIGFILE,False,False,False,True)) {
		fprintf(stderr, "Can't load %s - run testparm to debug it\n", dyn_CONFIGFILE);
		return (-1);
	}
	
	if (profile_only) {
		return profile_dump();
	}
	
	if ( show_processes ) {
		tdb = tdb_open_log(lock_path("sessionid.tdb"), 0, TDB_DEFAULT, O_RDONLY, 0);
		if (!tdb) {
			d_printf("sessionid.tdb not initialised\n");
		} else {
			d_printf("\nSamba version %s\n",SAMBA_VERSION_STRING);
			d_printf("PID     Username      Group         Machine                        \n");
			d_printf("-------------------------------------------------------------------\n");

			tdb_traverse(tdb, traverse_sessionid, NULL);
			tdb_close(tdb);
		}

		if (processes_only) 
			exit(0);	
	}
  
	if ( show_shares ) {
		tdb = tdb_open_log(lock_path("connections.tdb"), 0, TDB_DEFAULT, O_RDONLY, 0);
		if (!tdb) {
			d_printf("%s not initialised\n", lock_path("connections.tdb"));
			d_printf("This is normal if an SMB client has never connected to your server.\n");
		}  else  {
			if (verbose) {
				d_printf("Opened %s\n", lock_path("connections.tdb"));
			}

			if (brief) 
				exit(0);
		
			d_printf("\nService      pid     machine       Connected at\n");
			d_printf("-------------------------------------------------------\n");
	
			tdb_traverse(tdb, traverse_fn1, NULL);
			tdb_close(tdb);

			d_printf("\n");
		}

		if ( shares_only )
			exit(0);
	}

	if ( show_locks ) {
		int ret;

		if (!locking_init(1)) {
			d_printf("Can't initialise locking module - exiting\n");
			exit(1);
		}
		
		ret = share_mode_forall(print_share_mode);

		if (ret == 0) {
			d_printf("No locked files\n");
		} else if (ret == -1) {
			d_printf("locked file list truncated\n");
		}
		
		d_printf("\n");

		if (show_brl) {
			brl_forall(print_brl);
		}
		
		locking_end();
	}

	return (0);
}
