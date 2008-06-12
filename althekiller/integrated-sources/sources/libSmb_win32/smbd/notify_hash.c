/*
   Unix SMB/CIFS implementation.
   change notify handling - hash based implementation
   Copyright (C) Jeremy Allison 1994-1998
   Copyright (C) Andrew Tridgell 2000

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

struct change_data {
	time_t last_check_time; /* time we last checked this entry */
	struct timespec modify_time; /* Info from the directory we're monitoring. */
	struct timespec status_time; /* Info from the directory we're monitoring. */
	time_t total_time; /* Total time of all directory entries - don't care if it wraps. */
	unsigned int num_entries; /* Zero or the number of files in the directory. */
	unsigned int mode_sum;
	unsigned char name_hash[16];
};


/* Compare struct timespec. */
#define TIMESTAMP_NEQ(x, y) (((x).tv_sec != (y).tv_sec) || ((x).tv_nsec != (y).tv_nsec))

/****************************************************************************
 Create the hash we will use to determine if the contents changed.
*****************************************************************************/

static BOOL notify_hash(connection_struct *conn, char *path, uint32 flags, 
			struct change_data *data, struct change_data *old_data)
{
	SMB_STRUCT_STAT st;
	pstring full_name;
	char *p;
	const char *fname;
	size_t remaining_len;
	size_t fullname_len;
	struct smb_Dir *dp;
	long offset;

	ZERO_STRUCTP(data);

	if(SMB_VFS_STAT(conn,path, &st) == -1)
		return False;

	data->modify_time = get_mtimespec(&st);
	data->status_time = get_ctimespec(&st);

	if (old_data) {
		/*
		 * Shortcut to avoid directory scan if the time
		 * has changed - we always must return true then.
		 */
		if (TIMESTAMP_NEQ(old_data->modify_time, data->modify_time) ||
		    TIMESTAMP_NEQ(old_data->status_time, data->status_time) ) {
				return True;
		}
	}
 
        if (S_ISDIR(st.st_mode) && 
            (flags & ~(FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME)) == 0)
        {
		/* This is the case of a client wanting to know only when
		 * the contents of a directory changes. Since any file
		 * creation, rename or deletion will update the directory
		 * timestamps, we don't need to create a hash.
		 */
                return True;
        }

	if (lp_change_notify_timeout(SNUM(conn)) <= 0) {
		/* It change notify timeout has been disabled, never scan the directory. */
		return True;
	}

	/*
	 * If we are to watch for changes that are only stored
	 * in inodes of files, not in the directory inode, we must
	 * scan the directory and produce a unique identifier with
	 * which we can determine if anything changed. We use the
	 * modify and change times from all the files in the
	 * directory, added together (ignoring wrapping if it's
	 * larger than the max time_t value).
	 */

	dp = OpenDir(conn, path, NULL, 0);
	if (dp == NULL)
		return False;

	data->num_entries = 0;
	
	pstrcpy(full_name, path);
	pstrcat(full_name, "/");
	
	fullname_len = strlen(full_name);
	remaining_len = sizeof(full_name) - fullname_len - 1;
	p = &full_name[fullname_len];
	
	offset = 0;
	while ((fname = ReadDirName(dp, &offset))) {
		SET_STAT_INVALID(st);
		if(strequal(fname, ".") || strequal(fname, ".."))
			continue;		

		if (!is_visible_file(conn, path, fname, &st, True))
			continue;

		data->num_entries++;
		safe_strcpy(p, fname, remaining_len);

		/*
		 * Do the stat - but ignore errors.
		 */		
		if (!VALID_STAT(st)) {
			SMB_VFS_STAT(conn,full_name, &st);
		}

		/*
		 * Always sum the times.
		 */

		data->total_time += (st.st_mtime + st.st_ctime);

		/*
		 * If requested hash the names.
		 */

		if (flags & (FILE_NOTIFY_CHANGE_DIR_NAME|FILE_NOTIFY_CHANGE_FILE_NAME|FILE_NOTIFY_CHANGE_FILE)) {
			int i;
			unsigned char tmp_hash[16];
			mdfour(tmp_hash, (const unsigned char *)fname, strlen(fname));
			for (i=0;i<16;i++)
				data->name_hash[i] ^= tmp_hash[i];
		}

		/*
		 * If requested sum the mode_t's.
		 */

		if (flags & (FILE_NOTIFY_CHANGE_ATTRIBUTES|FILE_NOTIFY_CHANGE_SECURITY))
			data->mode_sum += st.st_mode;
	}
	
	CloseDir(dp);
	
	return True;
}

/****************************************************************************
 Register a change notify request.
*****************************************************************************/

static void *hash_register_notify(connection_struct *conn, char *path, uint32 flags)
{
	struct change_data data;

	if (!notify_hash(conn, path, flags, &data, NULL))
		return NULL;

	data.last_check_time = time(NULL);

	return (void *)memdup(&data, sizeof(data));
}

/****************************************************************************
 Check if a change notify should be issued.
 A time of zero means instantaneous check - don't modify the last check time.
*****************************************************************************/

static BOOL hash_check_notify(connection_struct *conn, uint16 vuid, char *path, uint32 flags, void *datap, time_t t)
{
	struct change_data *data = (struct change_data *)datap;
	struct change_data data2;
	int cnto = lp_change_notify_timeout(SNUM(conn));

	if (t && cnto <= 0) {
		/* Change notify turned off on this share.
		 * Only scan when (t==0) - we think something changed. */
		return False;
	}

	if (t && t < data->last_check_time + cnto) {
		return False;
	}

	if (!change_to_user(conn,vuid))
		return True;
	if (!set_current_service(conn,FLAG_CASELESS_PATHNAMES,True)) {
		change_to_root_user();
		return True;
	}

	if (!notify_hash(conn, path, flags, &data2, data) ||
	    TIMESTAMP_NEQ(data2.modify_time, data->modify_time) ||
	    TIMESTAMP_NEQ(data2.status_time, data->status_time) ||
	    data2.total_time != data->total_time ||
	    data2.num_entries != data->num_entries ||
		data2.mode_sum != data->mode_sum ||
		memcmp(data2.name_hash, data->name_hash, sizeof(data2.name_hash))) {
		change_to_root_user();
		return True;
	}

	if (t) {
		data->last_check_time = t;
	}

	change_to_root_user();

	return False;
}

/****************************************************************************
 Remove a change notify data structure.
*****************************************************************************/

static void hash_remove_notify(void *datap)
{
	free(datap);
}

/****************************************************************************
 Setup hash based change notify.
****************************************************************************/

struct cnotify_fns *hash_notify_init(void) 
{
	static struct cnotify_fns cnotify;

	cnotify.register_notify = hash_register_notify;
	cnotify.check_notify = hash_check_notify;
	cnotify.remove_notify = hash_remove_notify;
	cnotify.select_time = 60; /* Start with 1 minute default. */
	cnotify.notification_fd = -1;

	return &cnotify;
}

/*
  change_notify_reply_packet(cnbp->request_buf,ERRSRV,ERRaccess);
  change_notify_reply_packet(cnbp->request_buf,0,NT_STATUS_NOTIFY_ENUM_DIR);

  chain_size = 0;
  file_chain_reset();

  uint16 vuid = (lp_security() == SEC_SHARE) ? UID_FIELD_INVALID : 
  SVAL(cnbp->request_buf,smb_uid);
*/
