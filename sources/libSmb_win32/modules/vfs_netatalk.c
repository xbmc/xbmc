/* 
 * AppleTalk VFS module for Samba-3.x
 *
 * Copyright (C) Alexei Kotovich, 2002
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

#undef DBGC_CLASS
#define DBGC_CLASS DBGC_VFS

#define APPLEDOUBLE	".AppleDouble"
#define ADOUBLEMODE	0777

/* atalk functions */

static int atalk_build_paths(TALLOC_CTX *ctx, const char *path,
  const char *fname, char **adbl_path, char **orig_path, 
  SMB_STRUCT_STAT *adbl_info, SMB_STRUCT_STAT *orig_info);

static int atalk_unlink_file(const char *path);

static int atalk_get_path_ptr(char *path)
{
	int i   = 0;
	int ptr = 0;
	
	for (i = 0; path[i]; i ++) {
		if (path[i] == '/')
			ptr = i;
		/* get out some 'spam';) from win32's file name */
		else if (path[i] == ':') {
			path[i] = '\0';
			break;
		}
	}
	
	return ptr;
}

static int atalk_build_paths(TALLOC_CTX *ctx, const char *path, const char *fname,
                              char **adbl_path, char **orig_path,
                              SMB_STRUCT_STAT *adbl_info, SMB_STRUCT_STAT *orig_info)
{
	int ptr0 = 0;
	int ptr1 = 0;
	char *dname = 0;
	char *name  = 0;

	if (!ctx || !path || !fname || !adbl_path || !orig_path ||
		!adbl_info || !orig_info)
		return -1;
#if 0
	DEBUG(3, ("ATALK: PATH: %s[%s]\n", path, fname));
#endif
	if (strstr(path, APPLEDOUBLE) || strstr(fname, APPLEDOUBLE)) {
		DEBUG(3, ("ATALK: path %s[%s] already contains %s\n", path, fname, APPLEDOUBLE));
		return -1;
	}

	if (fname[0] == '.') ptr0 ++;
	if (fname[1] == '/') ptr0 ++;

	*orig_path = talloc_asprintf(ctx, "%s/%s", path, &fname[ptr0]);

	/* get pointer to last '/' */
	ptr1 = atalk_get_path_ptr(*orig_path);

	sys_lstat(*orig_path, orig_info);

	if (S_ISDIR(orig_info->st_mode)) {
		*adbl_path = talloc_asprintf(ctx, "%s/%s/%s/", 
		  path, &fname[ptr0], APPLEDOUBLE);
	} else {
		dname = talloc_strdup(ctx, *orig_path);
		dname[ptr1] = '\0';
		name = *orig_path;
		*adbl_path = talloc_asprintf(ctx, "%s/%s/%s", 
		  dname, APPLEDOUBLE, &name[ptr1 + 1]);
	}
#if 0
	DEBUG(3, ("ATALK: DEBUG:\n%s\n%s\n", *orig_path, *adbl_path)); 
#endif
	sys_lstat(*adbl_path, adbl_info);
	return 0;
}

static int atalk_unlink_file(const char *path)
{
	int ret = 0;

	become_root();
	ret = unlink(path);
	unbecome_root();
	
	return ret;
}

static void atalk_add_to_list(name_compare_entry **list)
{
	int i, count = 0;
	name_compare_entry *new_list = 0;
	name_compare_entry *cur_list = 0;

	cur_list = *list;

	if (cur_list) {
		for (i = 0, count = 0; cur_list[i].name; i ++, count ++) {
			if (strstr(cur_list[i].name, APPLEDOUBLE))
				return;
		}
	}

	if (!(new_list = SMB_CALLOC_ARRAY(name_compare_entry, (count == 0 ? 1 : count + 1))))
		return;

	for (i = 0; i < count; i ++) {
		new_list[i].name    = SMB_STRDUP(cur_list[i].name);
		new_list[i].is_wild = cur_list[i].is_wild;
	}

	new_list[i].name    = SMB_STRDUP(APPLEDOUBLE);
	new_list[i].is_wild = False;

	free_namearray(*list);

	*list = new_list;
	new_list = 0;
	cur_list = 0;
}

static void atalk_rrmdir(TALLOC_CTX *ctx, char *path)
{
	char *dpath;
	SMB_STRUCT_DIRENT *dent = 0;
	SMB_STRUCT_DIR *dir;

	if (!path) return;

	dir = sys_opendir(path);
	if (!dir) return;

	while (NULL != (dent = sys_readdir(dir))) {
		if (strcmp(dent->d_name, ".") == 0 ||
		    strcmp(dent->d_name, "..") == 0)
			continue;
		if (!(dpath = talloc_asprintf(ctx, "%s/%s", 
					      path, dent->d_name)))
			continue;
		atalk_unlink_file(dpath);
	}

	sys_closedir(dir);
}

/* Disk operations */

/* Directory operations */

SMB_STRUCT_DIR *atalk_opendir(struct vfs_handle_struct *handle, struct connection_struct *conn, const char *fname, const char *mask, uint32 attr)
{
	SMB_STRUCT_DIR *ret = 0;

	ret = SMB_VFS_NEXT_OPENDIR(handle, conn, fname, mask, attr);

	/*
	 * when we try to perform delete operation upon file which has fork
	 * in ./.AppleDouble and this directory wasn't hidden by Samba,
	 * MS Windows explorer causes the error: "Cannot find the specified file"
	 * There is some workaround to avoid this situation, i.e. if
	 * connection has not .AppleDouble entry in either veto or hide 
	 * list then it would be nice to add one.
	 */

	atalk_add_to_list(&conn->hide_list);
	atalk_add_to_list(&conn->veto_list);

	return ret;
}

static int atalk_rmdir(struct vfs_handle_struct *handle, struct connection_struct *conn, const char *path)
{
	BOOL add = False;
	TALLOC_CTX *ctx = 0;
	char *dpath;

	if (!conn || !conn->origpath || !path) goto exit_rmdir;

	/* due to there is no way to change bDeleteVetoFiles variable
	 * from this module, gotta use talloc stuff..
	 */

	strstr(path, APPLEDOUBLE) ? (add = False) : (add = True);

	if (!(ctx = talloc_init("remove_directory")))
		goto exit_rmdir;

	if (!(dpath = talloc_asprintf(ctx, "%s/%s%s", 
	  conn->origpath, path, add ? "/"APPLEDOUBLE : "")))
		goto exit_rmdir;

	atalk_rrmdir(ctx, dpath);

exit_rmdir:
	talloc_destroy(ctx);
	return SMB_VFS_NEXT_RMDIR(handle, conn, path);
}

/* File operations */

static int atalk_rename(struct vfs_handle_struct *handle, struct connection_struct *conn, const char *oldname, const char *newname)
{
	int ret = 0;
	char *adbl_path = 0;
	char *orig_path = 0;
	SMB_STRUCT_STAT adbl_info;
	SMB_STRUCT_STAT orig_info;
	TALLOC_CTX *ctx;

	ret = SMB_VFS_NEXT_RENAME(handle, conn, oldname, newname);

	if (!conn || !oldname) return ret;

	if (!(ctx = talloc_init("rename_file")))
		return ret;

	if (atalk_build_paths(ctx, conn->origpath, oldname, &adbl_path, &orig_path, 
	  &adbl_info, &orig_info) != 0)
		return ret;

	if (S_ISDIR(orig_info.st_mode) || S_ISREG(orig_info.st_mode)) {
		DEBUG(3, ("ATALK: %s has passed..\n", adbl_path));		
		goto exit_rename;
	}

	atalk_unlink_file(adbl_path);

exit_rename:
	talloc_destroy(ctx);
	return ret;
}

static int atalk_unlink(struct vfs_handle_struct *handle, struct connection_struct *conn, const char *path)
{
	int ret = 0, i;
	char *adbl_path = 0;
	char *orig_path = 0;
	SMB_STRUCT_STAT adbl_info;
	SMB_STRUCT_STAT orig_info;
	TALLOC_CTX *ctx;

	ret = SMB_VFS_NEXT_UNLINK(handle, conn, path);

	if (!conn || !path) return ret;

	/* no .AppleDouble sync if veto or hide list is empty,
	 * otherwise "Cannot find the specified file" error will be caused
	 */

	if (!conn->veto_list) return ret;
	if (!conn->hide_list) return ret;

	for (i = 0; conn->veto_list[i].name; i ++) {
		if (strstr(conn->veto_list[i].name, APPLEDOUBLE))
			break;
	}

	if (!conn->veto_list[i].name) {
		for (i = 0; conn->hide_list[i].name; i ++) {
			if (strstr(conn->hide_list[i].name, APPLEDOUBLE))
				break;
			else {
				DEBUG(3, ("ATALK: %s is not hidden, skipped..\n",
				  APPLEDOUBLE));		
				return ret;
			}
		}
	}

	if (!(ctx = talloc_init("unlink_file")))
		return ret;

	if (atalk_build_paths(ctx, conn->origpath, path, &adbl_path, &orig_path, 
	  &adbl_info, &orig_info) != 0)
		return ret;

	if (S_ISDIR(orig_info.st_mode) || S_ISREG(orig_info.st_mode)) {
		DEBUG(3, ("ATALK: %s has passed..\n", adbl_path));
		goto exit_unlink;
	}

	atalk_unlink_file(adbl_path);

exit_unlink:	
	talloc_destroy(ctx);
	return ret;
}

static int atalk_chmod(struct vfs_handle_struct *handle, struct connection_struct *conn, const char *path, mode_t mode)
{
	int ret = 0;
	char *adbl_path = 0;
	char *orig_path = 0;
	SMB_STRUCT_STAT adbl_info;
	SMB_STRUCT_STAT orig_info;
	TALLOC_CTX *ctx;

	ret = SMB_VFS_NEXT_CHMOD(handle, conn, path, mode);

	if (!conn || !path) return ret;

	if (!(ctx = talloc_init("chmod_file")))
		return ret;

	if (atalk_build_paths(ctx, conn->origpath, path, &adbl_path, &orig_path,
	  &adbl_info, &orig_info) != 0)
		return ret;

	if (!S_ISDIR(orig_info.st_mode) && !S_ISREG(orig_info.st_mode)) {
		DEBUG(3, ("ATALK: %s has passed..\n", orig_path));		
		goto exit_chmod;
	}

	chmod(adbl_path, ADOUBLEMODE);

exit_chmod:	
	talloc_destroy(ctx);
	return ret;
}

static int atalk_chown(struct vfs_handle_struct *handle, struct connection_struct *conn, const char *path, uid_t uid, gid_t gid)
{
	int ret = 0;
	char *adbl_path = 0;
	char *orig_path = 0;
	SMB_STRUCT_STAT adbl_info;
	SMB_STRUCT_STAT orig_info;
	TALLOC_CTX *ctx;

	ret = SMB_VFS_NEXT_CHOWN(handle, conn, path, uid, gid);

	if (!conn || !path) return ret;

	if (!(ctx = talloc_init("chown_file")))
		return ret;

	if (atalk_build_paths(ctx, conn->origpath, path, &adbl_path, &orig_path,
	  &adbl_info, &orig_info) != 0)
		return ret;

	if (!S_ISDIR(orig_info.st_mode) && !S_ISREG(orig_info.st_mode)) {
		DEBUG(3, ("ATALK: %s has passed..\n", orig_path));		
		goto exit_chown;
	}

	chown(adbl_path, uid, gid);

exit_chown:	
	talloc_destroy(ctx);
	return ret;
}

static vfs_op_tuple atalk_ops[] = {
    
	/* Directory operations */

	{SMB_VFS_OP(atalk_opendir),	 	SMB_VFS_OP_OPENDIR, 	SMB_VFS_LAYER_TRANSPARENT},
	{SMB_VFS_OP(atalk_rmdir), 		SMB_VFS_OP_RMDIR, 	SMB_VFS_LAYER_TRANSPARENT},

	/* File operations */

	{SMB_VFS_OP(atalk_rename), 		SMB_VFS_OP_RENAME, 	SMB_VFS_LAYER_TRANSPARENT},
	{SMB_VFS_OP(atalk_unlink), 		SMB_VFS_OP_UNLINK, 	SMB_VFS_LAYER_TRANSPARENT},
	{SMB_VFS_OP(atalk_chmod), 		SMB_VFS_OP_CHMOD, 	SMB_VFS_LAYER_TRANSPARENT},
	{SMB_VFS_OP(atalk_chown),		SMB_VFS_OP_CHOWN,	SMB_VFS_LAYER_TRANSPARENT},
	
	/* Finish VFS operations definition */
	
	{SMB_VFS_OP(NULL), 			SMB_VFS_OP_NOOP, 	SMB_VFS_LAYER_NOOP}
};

NTSTATUS vfs_netatalk_init(void)
{
	return smb_register_vfs(SMB_VFS_INTERFACE_VERSION, "netatalk", atalk_ops);
}
