/*
 * Recycle bin VFS module for Samba.
 *
 * Copyright (C) 2001, Brandon Stone, Amherst College, <bbstone@amherst.edu>.
 * Copyright (C) 2002, Jeremy Allison - modified to make a VFS module.
 * Copyright (C) 2002, Alexander Bokovoy - cascaded VFS adoption,
 * Copyright (C) 2002, Juergen Hasch - added some options.
 * Copyright (C) 2002, Simo Sorce
 * Copyright (C) 2002, Stefan (metze) Metzmacher
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

#define ALLOC_CHECK(ptr, label) do { if ((ptr) == NULL) { DEBUG(0, ("recycle.bin: out of memory!\n")); errno = ENOMEM; goto label; } } while(0)

static int vfs_recycle_debug_level = DBGC_VFS;

#undef DBGC_CLASS
#define DBGC_CLASS vfs_recycle_debug_level
 
static int recycle_connect(vfs_handle_struct *handle, connection_struct *conn, const char *service, const char *user);
static void recycle_disconnect(vfs_handle_struct *handle, connection_struct *conn);
static int recycle_unlink(vfs_handle_struct *handle, connection_struct *conn, const char *name);

static vfs_op_tuple recycle_ops[] = {

	/* Disk operations */
	{SMB_VFS_OP(recycle_connect),	SMB_VFS_OP_CONNECT,	SMB_VFS_LAYER_TRANSPARENT},
	{SMB_VFS_OP(recycle_disconnect),	SMB_VFS_OP_DISCONNECT,	SMB_VFS_LAYER_TRANSPARENT},

	/* File operations */
	{SMB_VFS_OP(recycle_unlink),	SMB_VFS_OP_UNLINK,	SMB_VFS_LAYER_TRANSPARENT},

	{SMB_VFS_OP(NULL),		SMB_VFS_OP_NOOP,	SMB_VFS_LAYER_NOOP}
};

static int recycle_connect(vfs_handle_struct *handle, connection_struct *conn, const char *service, const char *user)
{
	DEBUG(10,("recycle_connect() connect to service[%s] as user[%s].\n",
		service,user));

	return SMB_VFS_NEXT_CONNECT(handle, conn, service, user);		
}

static void recycle_disconnect(vfs_handle_struct *handle, connection_struct *conn)
{
	DEBUG(10,("recycle_disconnect() connect to service[%s].\n",
		lp_servicename(SNUM(conn))));

	SMB_VFS_NEXT_DISCONNECT(handle, conn);	
}

static const char *recycle_repository(vfs_handle_struct *handle)
{
	const char *tmp_str = NULL;
	

	tmp_str = lp_parm_const_string(SNUM(handle->conn), "recycle", "repository",".recycle");

	DEBUG(10, ("recycle: repository = %s\n", tmp_str));
	
	return tmp_str;
}

static BOOL recycle_keep_dir_tree(vfs_handle_struct *handle)
{
	BOOL ret;
	
	ret = lp_parm_bool(SNUM(handle->conn), "recycle", "keeptree", False);

	DEBUG(10, ("recycle_bin: keeptree = %s\n", ret?"True":"False"));
	
	return ret;
}

static BOOL recycle_versions(vfs_handle_struct *handle)
{
	BOOL ret;

	ret = lp_parm_bool(SNUM(handle->conn), "recycle", "versions", False);

	DEBUG(10, ("recycle: versions = %s\n", ret?"True":"False"));
	
	return ret;
}

static BOOL recycle_touch(vfs_handle_struct *handle)
{
	BOOL ret;

	ret = lp_parm_bool(SNUM(handle->conn), "recycle", "touch", False);

	DEBUG(10, ("recycle: touch = %s\n", ret?"True":"False"));
	
	return ret;
}

static BOOL recycle_touch_mtime(vfs_handle_struct *handle)
{
	BOOL ret;

	ret = lp_parm_bool(SNUM(handle->conn), "recycle", "touch_mtime", False);

	DEBUG(10, ("recycle: touch_mtime = %s\n", ret?"True":"False"));
	
	return ret;
}

static const char **recycle_exclude(vfs_handle_struct *handle)
{
	const char **tmp_lp;
	
	tmp_lp = lp_parm_string_list(SNUM(handle->conn), "recycle", "exclude", NULL);

	DEBUG(10, ("recycle: exclude = %s ...\n", tmp_lp?*tmp_lp:""));
	
	return tmp_lp;
}

static const char **recycle_exclude_dir(vfs_handle_struct *handle)
{
	const char **tmp_lp;
	
	tmp_lp = lp_parm_string_list(SNUM(handle->conn), "recycle", "exclude_dir", NULL);

	DEBUG(10, ("recycle: exclude_dir = %s ...\n", tmp_lp?*tmp_lp:""));
	
	return tmp_lp;
}

static const char **recycle_noversions(vfs_handle_struct *handle)
{
	const char **tmp_lp;
	
	tmp_lp = lp_parm_string_list(SNUM(handle->conn), "recycle", "noversions", NULL);

	DEBUG(10, ("recycle: noversions = %s\n", tmp_lp?*tmp_lp:""));
	
	return tmp_lp;
}

static int recycle_maxsize(vfs_handle_struct *handle)
{
	int maxsize;
	
	maxsize = lp_parm_int(SNUM(handle->conn), "recycle", "maxsize", -1);

	DEBUG(10, ("recycle: maxsize = %d\n", maxsize));
	
	return maxsize;
}

static mode_t recycle_directory_mode(vfs_handle_struct *handle)
{
	int dirmode;
	const char *buff;

	buff = lp_parm_const_string(SNUM(handle->conn), "recycle", "directory_mode", NULL);

	if (buff != NULL ) {
		sscanf(buff, "%o", &dirmode);
	} else {
		dirmode=S_IRUSR | S_IWUSR | S_IXUSR;
	}

	DEBUG(10, ("recycle: directory_mode = %o\n", dirmode));
	return (mode_t)dirmode;
}

static mode_t recycle_subdir_mode(vfs_handle_struct *handle)
{
	int dirmode;
	const char *buff;

	buff = lp_parm_const_string(SNUM(handle->conn), "recycle", "subdir_mode", NULL);

	if (buff != NULL ) {
		sscanf(buff, "%o", &dirmode);
	} else {
		dirmode=recycle_directory_mode(handle);
	}

	DEBUG(10, ("recycle: subdir_mode = %o\n", dirmode));
	return (mode_t)dirmode;
}

static BOOL recycle_directory_exist(vfs_handle_struct *handle, const char *dname)
{
	SMB_STRUCT_STAT st;

	if (SMB_VFS_NEXT_STAT(handle, handle->conn, dname, &st) == 0) {
		if (S_ISDIR(st.st_mode)) {
			return True;
		}
	}

	return False;
}

static BOOL recycle_file_exist(vfs_handle_struct *handle, const char *fname)
{
	SMB_STRUCT_STAT st;

	if (SMB_VFS_NEXT_STAT(handle, handle->conn, fname, &st) == 0) {
		if (S_ISREG(st.st_mode)) {
			return True;
		}
	}

	return False;
}

/**
 * Return file size
 * @param conn connection
 * @param fname file name
 * @return size in bytes
 **/
static SMB_OFF_T recycle_get_file_size(vfs_handle_struct *handle, const char *fname)
{
	SMB_STRUCT_STAT st;

	if (SMB_VFS_NEXT_STAT(handle, handle->conn, fname, &st) != 0) {
		DEBUG(0,("recycle: stat for %s returned %s\n", fname, strerror(errno)));
		return (SMB_OFF_T)0;
	}

	return(st.st_size);
}

/**
 * Create directory tree
 * @param conn connection
 * @param dname Directory tree to be created
 * @return Returns True for success
 **/
static BOOL recycle_create_dir(vfs_handle_struct *handle, const char *dname)
{
	size_t len;
	mode_t mode;
	char *new_dir = NULL;
	char *tmp_str = NULL;
	char *token;
	char *tok_str;
	BOOL ret = False;

	mode = recycle_directory_mode(handle);

	tmp_str = SMB_STRDUP(dname);
	ALLOC_CHECK(tmp_str, done);
	tok_str = tmp_str;

	len = strlen(dname)+1;
	new_dir = (char *)SMB_MALLOC(len + 1);
	ALLOC_CHECK(new_dir, done);
	*new_dir = '\0';
	if (dname[0] == '/') {
		/* Absolute path. */
		safe_strcat(new_dir,"/",len);
	}

	/* Create directory tree if neccessary */
	for(token = strtok(tok_str, "/"); token; token = strtok(NULL, "/")) {
		safe_strcat(new_dir, token, len);
		if (recycle_directory_exist(handle, new_dir))
			DEBUG(10, ("recycle: dir %s already exists\n", new_dir));
		else {
			DEBUG(5, ("recycle: creating new dir %s\n", new_dir));
			if (SMB_VFS_NEXT_MKDIR(handle, handle->conn, new_dir, mode) != 0) {
				DEBUG(1,("recycle: mkdir failed for %s with error: %s\n", new_dir, strerror(errno)));
				ret = False;
				goto done;
			}
		}
		safe_strcat(new_dir, "/", len);
		mode = recycle_subdir_mode(handle);
	}

	ret = True;
done:
	SAFE_FREE(tmp_str);
	SAFE_FREE(new_dir);
	return ret;
}

/**
 * Check if needle is contained exactly in haystack
 * @param haystack list of parameters separated by delimimiter character
 * @param needle string to be matched exactly to haystack
 * @return True if found
 **/
static BOOL checkparam(const char **haystack_list, const char *needle)
{
	int i;

	if (haystack_list == NULL || haystack_list[0] == NULL ||
		*haystack_list[0] == '\0' || needle == NULL || *needle == '\0') {
		return False;
	}

	for(i=0; haystack_list[i] ; i++) {
		if(strequal(haystack_list[i], needle)) {
			return True;
		}
	}

	return False;
}

/**
 * Check if needle is contained in haystack, * and ? patterns are resolved
 * @param haystack list of parameters separated by delimimiter character
 * @param needle string to be matched exectly to haystack including pattern matching
 * @return True if found
 **/
static BOOL matchparam(const char **haystack_list, const char *needle)
{
	int i;

	if (haystack_list == NULL || haystack_list[0] == NULL ||
		*haystack_list[0] == '\0' || needle == NULL || *needle == '\0') {
		return False;
	}

	for(i=0; haystack_list[i] ; i++) {
		if(unix_wild_match(haystack_list[i], needle)) {
			return True;
		}
	}

	return False;
}

/**
 * Touch access or modify date
 **/
static void recycle_do_touch(vfs_handle_struct *handle, const char *fname, BOOL touch_mtime)
{
	SMB_STRUCT_STAT st;
	struct utimbuf tb;
	time_t currtime;
	
	if (SMB_VFS_NEXT_STAT(handle, handle->conn, fname, &st) != 0) {
		DEBUG(0,("recycle: stat for %s returned %s\n", fname, strerror(errno)));
		return;
	}
	currtime = time(&currtime);
	tb.actime = currtime;
	tb.modtime = touch_mtime ? currtime : st.st_mtime;

	if (SMB_VFS_NEXT_UTIME(handle, handle->conn, fname, &tb) == -1 ) {
		DEBUG(0, ("recycle: touching %s failed, reason = %s\n", fname, strerror(errno)));
	}
}

/**
 * Check if file should be recycled
 **/
static int recycle_unlink(vfs_handle_struct *handle, connection_struct *conn, const char *file_name)
{
	char *path_name = NULL;
       	char *temp_name = NULL;
	char *final_name = NULL;
	const char *base;
	char *repository = NULL;
	int i = 1;
	int maxsize;
	SMB_OFF_T file_size; /* space_avail;	*/
	BOOL exist;
	int rc = -1;

	repository = alloc_sub_conn(conn, recycle_repository(handle));
	ALLOC_CHECK(repository, done);
	/* shouldn't we allow absolute path names here? --metze */
	/* Yes :-). JRA. */
	trim_char(repository, '\0', '/');
	
	if(!repository || *(repository) == '\0') {
		DEBUG(3, ("recycle: repository path not set, purging %s...\n", file_name));
		rc = SMB_VFS_NEXT_UNLINK(handle, conn, file_name);
		goto done;
	}

	/* we don't recycle the recycle bin... */
	if (strncmp(file_name, repository, strlen(repository)) == 0) {
		DEBUG(3, ("recycle: File is within recycling bin, unlinking ...\n"));
		rc = SMB_VFS_NEXT_UNLINK(handle, conn, file_name);
		goto done;
	}

	file_size = recycle_get_file_size(handle, file_name);
	/* it is wrong to purge filenames only because they are empty imho
	 *   --- simo
	 *
	if(fsize == 0) {
		DEBUG(3, ("recycle: File %s is empty, purging...\n", file_name));
		rc = SMB_VFS_NEXT_UNLINK(handle,conn,file_name);
		goto done;
	}
	 */

	/* FIXME: this is wrong, we should check the whole size of the recycle bin is
	 * not greater then maxsize, not the size of the single file, also it is better
	 * to remove older files
	 */
	maxsize = recycle_maxsize(handle);
	if(maxsize > 0 && file_size > maxsize) {
		DEBUG(3, ("recycle: File %s exceeds maximum recycle size, purging... \n", file_name));
		rc = SMB_VFS_NEXT_UNLINK(handle, conn, file_name);
		goto done;
	}

	/* FIXME: this is wrong: moving files with rename does not change the disk space
	 * allocation
	 *
	space_avail = SMB_VFS_NEXT_DISK_FREE(handle, conn, ".", True, &bsize, &dfree, &dsize) * 1024L;
	DEBUG(5, ("space_avail = %Lu, file_size = %Lu\n", space_avail, file_size));
	if(space_avail < file_size) {
		DEBUG(3, ("recycle: Not enough diskspace, purging file %s\n", file_name));
		rc = SMB_VFS_NEXT_UNLINK(handle, conn, file_name);
		goto done;
	}
	 */

	/* extract filename and path */
	base = strrchr(file_name, '/');
	if (base == NULL) {
		base = file_name;
		path_name = SMB_STRDUP("/");
		ALLOC_CHECK(path_name, done);
	}
	else {
		path_name = SMB_STRDUP(file_name);
		ALLOC_CHECK(path_name, done);
		path_name[base - file_name] = '\0';
		base++;
	}

	DEBUG(10, ("recycle: fname = %s\n", file_name));	/* original filename with path */
	DEBUG(10, ("recycle: fpath = %s\n", path_name));	/* original path */
	DEBUG(10, ("recycle: base = %s\n", base));		/* filename without path */

	if (matchparam(recycle_exclude(handle), base)) {
		DEBUG(3, ("recycle: file %s is excluded \n", base));
		rc = SMB_VFS_NEXT_UNLINK(handle, conn, file_name);
		goto done;
	}

	/* FIXME: this check will fail if we have more than one level of directories,
	 * we shoud check for every level 1, 1/2, 1/2/3, 1/2/3/4 .... 
	 * 	---simo
	 */
	if (checkparam(recycle_exclude_dir(handle), path_name)) {
		DEBUG(3, ("recycle: directory %s is excluded \n", path_name));
		rc = SMB_VFS_NEXT_UNLINK(handle, conn, file_name);
		goto done;
	}

	if (recycle_keep_dir_tree(handle) == True) {
		asprintf(&temp_name, "%s/%s", repository, path_name);
	} else {
		temp_name = SMB_STRDUP(repository);
	}
	ALLOC_CHECK(temp_name, done);

	exist = recycle_directory_exist(handle, temp_name);
	if (exist) {
		DEBUG(10, ("recycle: Directory already exists\n"));
	} else {
		DEBUG(10, ("recycle: Creating directory %s\n", temp_name));
		if (recycle_create_dir(handle, temp_name) == False) {
			DEBUG(3, ("recycle: Could not create directory, purging %s...\n", file_name));
			rc = SMB_VFS_NEXT_UNLINK(handle, conn, file_name);
			goto done;
		}
	}

	asprintf(&final_name, "%s/%s", temp_name, base);
	ALLOC_CHECK(final_name, done);
	DEBUG(10, ("recycle: recycled file name: %s\n", final_name));		/* new filename with path */

	/* check if we should delete file from recycle bin */
	if (recycle_file_exist(handle, final_name)) {
		if (recycle_versions(handle) == False || matchparam(recycle_noversions(handle), base) == True) {
			DEBUG(3, ("recycle: Removing old file %s from recycle bin\n", final_name));
			if (SMB_VFS_NEXT_UNLINK(handle, conn, final_name) != 0) {
				DEBUG(1, ("recycle: Error deleting old file: %s\n", strerror(errno)));
			}
		}
	}

	/* rename file we move to recycle bin */
	i = 1;
	while (recycle_file_exist(handle, final_name)) {
		SAFE_FREE(final_name);
		asprintf(&final_name, "%s/Copy #%d of %s", temp_name, i++, base);
	}

	DEBUG(10, ("recycle: Moving %s to %s\n", file_name, final_name));
	rc = SMB_VFS_NEXT_RENAME(handle, conn, file_name, final_name);
	if (rc != 0) {
		DEBUG(3, ("recycle: Move error %d (%s), purging file %s (%s)\n", errno, strerror(errno), file_name, final_name));
		rc = SMB_VFS_NEXT_UNLINK(handle, conn, file_name);
		goto done;
	}

	/* touch access date of moved file */
	if (recycle_touch(handle) == True || recycle_touch_mtime(handle))
		recycle_do_touch(handle, final_name, recycle_touch_mtime(handle));

done:
	SAFE_FREE(path_name);
	SAFE_FREE(temp_name);
	SAFE_FREE(final_name);
	SAFE_FREE(repository);
	return rc;
}

NTSTATUS vfs_recycle_init(void)
{
	NTSTATUS ret = smb_register_vfs(SMB_VFS_INTERFACE_VERSION, "recycle", recycle_ops);

	if (!NT_STATUS_IS_OK(ret))
		return ret;
	
	vfs_recycle_debug_level = debug_add_class("recycle");
	if (vfs_recycle_debug_level == -1) {
		vfs_recycle_debug_level = DBGC_VFS;
		DEBUG(0, ("vfs_recycle: Couldn't register custom debugging class!\n"));
	} else {
		DEBUG(10, ("vfs_recycle: Debug class number of 'recycle': %d\n", vfs_recycle_debug_level));
	}
	
	return ret;
}
