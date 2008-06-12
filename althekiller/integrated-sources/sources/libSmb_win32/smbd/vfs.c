/*
   Unix SMB/Netbios implementation.
   Version 1.9.
   VFS initialisation and support functions
   Copyright (C) Tim Potter 1999
   Copyright (C) Alexander Bokovoy 2002

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

   This work was sponsored by Optifacio Software Services, Inc.
*/

#include "includes.h"

#undef DBGC_CLASS
#define DBGC_CLASS DBGC_VFS

static_decl_vfs;

struct vfs_init_function_entry {
	char *name;
 	vfs_op_tuple *vfs_op_tuples;
	struct vfs_init_function_entry *prev, *next;
};

static struct vfs_init_function_entry *backends = NULL;

/* Some structures to help us initialise the vfs operations table */

struct vfs_syminfo {
	char *name;
	void *fptr;
};

/* Default vfs hooks.  WARNING: The order of these initialisers is
   very important.  They must be in the same order as defined in
   vfs.h.  Change at your own peril. */

static struct vfs_ops default_vfs = {

	{
		/* Disk operations */
	
		vfswrap_dummy_connect,
		vfswrap_dummy_disconnect,
		vfswrap_disk_free,
		vfswrap_get_quota,
		vfswrap_set_quota,
		vfswrap_get_shadow_copy_data,
		vfswrap_statvfs,
	
		/* Directory operations */
	
		vfswrap_opendir,
		vfswrap_readdir,
		vfswrap_seekdir,
		vfswrap_telldir,
		vfswrap_rewinddir,
		vfswrap_mkdir,
		vfswrap_rmdir,
		vfswrap_closedir,
	
		/* File operations */
	
		vfswrap_open,
		vfswrap_close,
		vfswrap_read,
		vfswrap_pread,
		vfswrap_write,
		vfswrap_pwrite,
		vfswrap_lseek,
		vfswrap_sendfile,
		vfswrap_rename,
		vfswrap_fsync,
		vfswrap_stat,
		vfswrap_fstat,
		vfswrap_lstat,
		vfswrap_unlink,
		vfswrap_chmod,
		vfswrap_fchmod,
		vfswrap_chown,
		vfswrap_fchown,
		vfswrap_chdir,
		vfswrap_getwd,
		vfswrap_utime,
		vfswrap_ftruncate,
		vfswrap_lock,
		vfswrap_getlock,
		vfswrap_symlink,
		vfswrap_readlink,
		vfswrap_link,
		vfswrap_mknod,
		vfswrap_realpath,
	
		/* Windows ACL operations. */
		vfswrap_fget_nt_acl,
		vfswrap_get_nt_acl,
		vfswrap_fset_nt_acl,
		vfswrap_set_nt_acl,
	
		/* POSIX ACL operations. */
		vfswrap_chmod_acl,
		vfswrap_fchmod_acl,

		vfswrap_sys_acl_get_entry,
		vfswrap_sys_acl_get_tag_type,
		vfswrap_sys_acl_get_permset,
		vfswrap_sys_acl_get_qualifier,
		vfswrap_sys_acl_get_file,
		vfswrap_sys_acl_get_fd,
		vfswrap_sys_acl_clear_perms,
		vfswrap_sys_acl_add_perm,
		vfswrap_sys_acl_to_text,
		vfswrap_sys_acl_init,
		vfswrap_sys_acl_create_entry,
		vfswrap_sys_acl_set_tag_type,
		vfswrap_sys_acl_set_qualifier,
		vfswrap_sys_acl_set_permset,
		vfswrap_sys_acl_valid,
		vfswrap_sys_acl_set_file,
		vfswrap_sys_acl_set_fd,
		vfswrap_sys_acl_delete_def_file,
		vfswrap_sys_acl_get_perm,
		vfswrap_sys_acl_free_text,
		vfswrap_sys_acl_free_acl,
		vfswrap_sys_acl_free_qualifier,

		/* EA operations. */
		vfswrap_getxattr,
		vfswrap_lgetxattr,
		vfswrap_fgetxattr,
		vfswrap_listxattr,
		vfswrap_llistxattr,
		vfswrap_flistxattr,
		vfswrap_removexattr,
		vfswrap_lremovexattr,
		vfswrap_fremovexattr,
		vfswrap_setxattr,
		vfswrap_lsetxattr,
		vfswrap_fsetxattr,

		/* AIO operations. */
		vfswrap_aio_read,
		vfswrap_aio_write,
		vfswrap_aio_return,
		vfswrap_aio_cancel,
		vfswrap_aio_error,
		vfswrap_aio_fsync,
		vfswrap_aio_suspend
	}
};

/****************************************************************************
    maintain the list of available backends
****************************************************************************/

static struct vfs_init_function_entry *vfs_find_backend_entry(const char *name)
{
	struct vfs_init_function_entry *entry = backends;
 
	while(entry) {
		if (strcmp(entry->name, name)==0) return entry;
		entry = entry->next;
	}

	return NULL;
}

NTSTATUS smb_register_vfs(int version, const char *name, vfs_op_tuple *vfs_op_tuples)
{
	struct vfs_init_function_entry *entry = backends;

 	if ((version != SMB_VFS_INTERFACE_VERSION)) {
		DEBUG(0, ("Failed to register vfs module.\n"
		          "The module was compiled against SMB_VFS_INTERFACE_VERSION %d,\n"
		          "current SMB_VFS_INTERFACE_VERSION is %d.\n"
		          "Please recompile against the current Samba Version!\n",  
			  version, SMB_VFS_INTERFACE_VERSION));
		return NT_STATUS_OBJECT_TYPE_MISMATCH;
  	}

	if (!name || !name[0] || !vfs_op_tuples) {
		DEBUG(0,("smb_register_vfs() called with NULL pointer or empty name!\n"));
		return NT_STATUS_INVALID_PARAMETER;
	}

	if (vfs_find_backend_entry(name)) {
		DEBUG(0,("VFS module %s already loaded!\n", name));
		return NT_STATUS_OBJECT_NAME_COLLISION;
	}

	entry = SMB_XMALLOC_P(struct vfs_init_function_entry);
	entry->name = smb_xstrdup(name);
	entry->vfs_op_tuples = vfs_op_tuples;

	DLIST_ADD(backends, entry);
	DEBUG(5, ("Successfully added vfs backend '%s'\n", name));
	return NT_STATUS_OK;
}

/****************************************************************************
  initialise default vfs hooks
****************************************************************************/

static void vfs_init_default(connection_struct *conn)
{
	DEBUG(3, ("Initialising default vfs hooks\n"));

	memcpy(&conn->vfs.ops, &default_vfs.ops, sizeof(default_vfs.ops));
	memcpy(&conn->vfs_opaque.ops, &default_vfs.ops, sizeof(default_vfs.ops));
}

/****************************************************************************
  initialise custom vfs hooks
 ****************************************************************************/

BOOL vfs_init_custom(connection_struct *conn, const char *vfs_object)
{
	vfs_op_tuple *ops;
	char *module_name = NULL;
	char *module_param = NULL, *p;
	int i;
	vfs_handle_struct *handle;
	struct vfs_init_function_entry *entry;
	
	if (!conn||!vfs_object||!vfs_object[0]) {
		DEBUG(0,("vfs_init_custon() called with NULL pointer or emtpy vfs_object!\n"));
		return False;
	}

	if(!backends) {
		static_init_vfs;
	}

	DEBUG(3, ("Initialising custom vfs hooks from [%s]\n", vfs_object));

	module_name = smb_xstrdup(vfs_object);

	p = strchr_m(module_name, ':');

	if (p) {
		*p = 0;
		module_param = p+1;
		trim_char(module_param, ' ', ' ');
	}

	trim_char(module_name, ' ', ' ');

	/* First, try to load the module with the new module system */
	if((entry = vfs_find_backend_entry(module_name)) || 
	   (NT_STATUS_IS_OK(smb_probe_module("vfs", module_name)) && 
		(entry = vfs_find_backend_entry(module_name)))) {

		DEBUGADD(5,("Successfully loaded vfs module [%s] with the new modules system\n", vfs_object));
		
	 	if ((ops = entry->vfs_op_tuples) == NULL) {
	 		DEBUG(0, ("entry->vfs_op_tuples==NULL for [%s] failed\n", vfs_object));
	 		SAFE_FREE(module_name);
	 		return False;
	 	}
	} else {
		DEBUG(0,("Can't find a vfs module [%s]\n",vfs_object));
		SAFE_FREE(module_name);
		return False;
	}

	handle = TALLOC_ZERO_P(conn->mem_ctx,vfs_handle_struct);
	if (!handle) {
		DEBUG(0,("talloc_zero() failed!\n"));
		SAFE_FREE(module_name);
		return False;
	}
	memcpy(&handle->vfs_next, &conn->vfs, sizeof(struct vfs_ops));
	handle->conn = conn;
	if (module_param) {
		handle->param = talloc_strdup(conn->mem_ctx, module_param);
	}
	DLIST_ADD(conn->vfs_handles, handle);

 	for(i=0; ops[i].op != NULL; i++) {
		DEBUG(5, ("Checking operation #%d (type %d, layer %d)\n", i, ops[i].type, ops[i].layer));
		if(ops[i].layer == SMB_VFS_LAYER_OPAQUE) {
			/* Check whether this operation was already made opaque by different module */
			if(((void**)&conn->vfs_opaque.ops)[ops[i].type] == ((void**)&default_vfs.ops)[ops[i].type]) {
				/* No, it isn't overloaded yet. Overload. */
				DEBUGADD(5, ("Making operation type %d opaque [module %s]\n", ops[i].type, vfs_object));
				((void**)&conn->vfs_opaque.ops)[ops[i].type] = ops[i].op;
				((vfs_handle_struct **)&conn->vfs_opaque.handles)[ops[i].type] = handle;
			}
		}
		/* Change current VFS disposition*/
		DEBUGADD(5, ("Accepting operation type %d from module %s\n", ops[i].type, vfs_object));
		((void**)&conn->vfs.ops)[ops[i].type] = ops[i].op;
		((vfs_handle_struct **)&conn->vfs.handles)[ops[i].type] = handle;
	}

	SAFE_FREE(module_name);
	return True;
}

/*****************************************************************
 Generic VFS init.
******************************************************************/

BOOL smbd_vfs_init(connection_struct *conn)
{
	const char **vfs_objects;
	unsigned int i = 0;
	int j = 0;
	
	/* Normal share - initialise with disk access functions */
	vfs_init_default(conn);
	vfs_objects = lp_vfs_objects(SNUM(conn));

	/* Override VFS functions if 'vfs object' was not specified*/
	if (!vfs_objects || !vfs_objects[0])
		return True;
	
	for (i=0; vfs_objects[i] ;) {
		i++;
	}

	for (j=i-1; j >= 0; j--) {
		if (!vfs_init_custom(conn, vfs_objects[j])) {
			DEBUG(0, ("smbd_vfs_init: vfs_init_custom failed for %s\n", vfs_objects[j]));
			return False;
		}
	}
	return True;
}

/*******************************************************************
 Check if directory exists.
********************************************************************/

BOOL vfs_directory_exist(connection_struct *conn, const char *dname, SMB_STRUCT_STAT *st)
{
	SMB_STRUCT_STAT st2;
	BOOL ret;

	if (!st)
		st = &st2;

	if (SMB_VFS_STAT(conn,dname,st) != 0)
		return(False);

	ret = S_ISDIR(st->st_mode);
	if(!ret)
		errno = ENOTDIR;

	return ret;
}

/*******************************************************************
 vfs mkdir wrapper 
********************************************************************/

int vfs_MkDir(connection_struct *conn, const char *name, mode_t mode)
{
	int ret;
	SMB_STRUCT_STAT sbuf;

	if(!(ret=SMB_VFS_MKDIR(conn, name, mode))) {

		inherit_access_acl(conn, name, mode);

		/*
		 * Check if high bits should have been set,
		 * then (if bits are missing): add them.
		 * Consider bits automagically set by UNIX, i.e. SGID bit from parent dir.
		 */
		if(mode & ~(S_IRWXU|S_IRWXG|S_IRWXO) &&
				!SMB_VFS_STAT(conn,name,&sbuf) && (mode & ~sbuf.st_mode))
			SMB_VFS_CHMOD(conn,name,sbuf.st_mode | (mode & ~sbuf.st_mode));
	}
	return ret;
}

/*******************************************************************
 Check if an object exists in the vfs.
********************************************************************/

BOOL vfs_object_exist(connection_struct *conn,const char *fname,SMB_STRUCT_STAT *sbuf)
{
	SMB_STRUCT_STAT st;

	if (!sbuf)
		sbuf = &st;

	ZERO_STRUCTP(sbuf);

	if (SMB_VFS_STAT(conn,fname,sbuf) == -1)
		return(False);
	return True;
}

/*******************************************************************
 Check if a file exists in the vfs.
********************************************************************/

BOOL vfs_file_exist(connection_struct *conn, const char *fname,SMB_STRUCT_STAT *sbuf)
{
	SMB_STRUCT_STAT st;

	if (!sbuf)
		sbuf = &st;

	ZERO_STRUCTP(sbuf);

	if (SMB_VFS_STAT(conn,fname,sbuf) == -1)
		return False;
	return(S_ISREG(sbuf->st_mode));
}

/****************************************************************************
 Read data from fsp on the vfs. (note: EINTR re-read differs from vfs_write_data)
****************************************************************************/

ssize_t vfs_read_data(files_struct *fsp, char *buf, size_t byte_count)
{
	size_t total=0;

	while (total < byte_count)
	{
		ssize_t ret = SMB_VFS_READ(fsp, fsp->fh->fd, buf + total,
					byte_count - total);

		if (ret == 0) return total;
		if (ret == -1) {
			if (errno == EINTR)
				continue;
			else
				return -1;
		}
		total += ret;
	}
	return (ssize_t)total;
}

ssize_t vfs_pread_data(files_struct *fsp, char *buf,
                size_t byte_count, SMB_OFF_T offset)
{
	size_t total=0;

	while (total < byte_count)
	{
		ssize_t ret = SMB_VFS_PREAD(fsp, fsp->fh->fd, buf + total,
					byte_count - total, offset + total);

		if (ret == 0) return total;
		if (ret == -1) {
			if (errno == EINTR)
				continue;
			else
				return -1;
		}
		total += ret;
	}
	return (ssize_t)total;
}

/****************************************************************************
 Write data to a fd on the vfs.
****************************************************************************/

ssize_t vfs_write_data(files_struct *fsp,const char *buffer,size_t N)
{
	size_t total=0;
	ssize_t ret;

	while (total < N) {
		ret = SMB_VFS_WRITE(fsp,fsp->fh->fd,buffer + total,N - total);

		if (ret == -1)
			return -1;
		if (ret == 0)
			return total;

		total += ret;
	}
	return (ssize_t)total;
}

ssize_t vfs_pwrite_data(files_struct *fsp,const char *buffer,
                size_t N, SMB_OFF_T offset)
{
	size_t total=0;
	ssize_t ret;

	while (total < N) {
		ret = SMB_VFS_PWRITE(fsp, fsp->fh->fd, buffer + total,
                                N - total, offset + total);

		if (ret == -1)
			return -1;
		if (ret == 0)
			return total;

		total += ret;
	}
	return (ssize_t)total;
}
/****************************************************************************
 An allocate file space call using the vfs interface.
 Allocates space for a file from a filedescriptor.
 Returns 0 on success, -1 on failure.
****************************************************************************/

int vfs_allocate_file_space(files_struct *fsp, SMB_BIG_UINT len)
{
	int ret;
	SMB_STRUCT_STAT st;
	connection_struct *conn = fsp->conn;
	SMB_BIG_UINT space_avail;
	SMB_BIG_UINT bsize,dfree,dsize;

	release_level_2_oplocks_on_change(fsp);

	/*
	 * Actually try and commit the space on disk....
	 */

	DEBUG(10,("vfs_allocate_file_space: file %s, len %.0f\n", fsp->fsp_name, (double)len ));

	if (((SMB_OFF_T)len) < 0) {
		DEBUG(0,("vfs_allocate_file_space: %s negative len requested.\n", fsp->fsp_name ));
		return -1;
	}

	ret = SMB_VFS_FSTAT(fsp,fsp->fh->fd,&st);
	if (ret == -1)
		return ret;

	if (len == (SMB_BIG_UINT)st.st_size)
		return 0;

	if (len < (SMB_BIG_UINT)st.st_size) {
		/* Shrink - use ftruncate. */

		DEBUG(10,("vfs_allocate_file_space: file %s, shrink. Current size %.0f\n",
				fsp->fsp_name, (double)st.st_size ));

		flush_write_cache(fsp, SIZECHANGE_FLUSH);
		if ((ret = SMB_VFS_FTRUNCATE(fsp, fsp->fh->fd, (SMB_OFF_T)len)) != -1) {
			set_filelen_write_cache(fsp, len);
		}
		return ret;
	}

	/* Grow - we need to test if we have enough space. */

	if (!lp_strict_allocate(SNUM(fsp->conn)))
		return 0;

	len -= st.st_size;
	len /= 1024; /* Len is now number of 1k blocks needed. */
	space_avail = get_dfree_info(conn,fsp->fsp_name,False,&bsize,&dfree,&dsize);
	if (space_avail == (SMB_BIG_UINT)-1) {
		return -1;
	}

	DEBUG(10,("vfs_allocate_file_space: file %s, grow. Current size %.0f, needed blocks = %.0f, space avail = %.0f\n",
			fsp->fsp_name, (double)st.st_size, (double)len, (double)space_avail ));

	if (len > space_avail) {
		errno = ENOSPC;
		return -1;
	}

	return 0;
}

/****************************************************************************
 A vfs set_filelen call.
 set the length of a file from a filedescriptor.
 Returns 0 on success, -1 on failure.
****************************************************************************/

int vfs_set_filelen(files_struct *fsp, SMB_OFF_T len)
{
	int ret;

	release_level_2_oplocks_on_change(fsp);
	DEBUG(10,("vfs_set_filelen: ftruncate %s to len %.0f\n", fsp->fsp_name, (double)len));
	flush_write_cache(fsp, SIZECHANGE_FLUSH);
	if ((ret = SMB_VFS_FTRUNCATE(fsp, fsp->fh->fd, len)) != -1)
		set_filelen_write_cache(fsp, len);

	return ret;
}

/****************************************************************************
 A vfs fill sparse call.
 Writes zeros from the end of file to len, if len is greater than EOF.
 Used only by strict_sync.
 Returns 0 on success, -1 on failure.
****************************************************************************/

static char *sparse_buf;
#define SPARSE_BUF_WRITE_SIZE (32*1024)

int vfs_fill_sparse(files_struct *fsp, SMB_OFF_T len)
{
	int ret;
	SMB_STRUCT_STAT st;
	SMB_OFF_T offset;
	size_t total;
	size_t num_to_write;
	ssize_t pwrite_ret;

	release_level_2_oplocks_on_change(fsp);
	ret = SMB_VFS_FSTAT(fsp,fsp->fh->fd,&st);
	if (ret == -1) {
		return ret;
	}

	if (len <= st.st_size) {
		return 0;
	}

	DEBUG(10,("vfs_fill_sparse: write zeros in file %s from len %.0f to len %.0f (%.0f bytes)\n",
		fsp->fsp_name, (double)st.st_size, (double)len, (double)(len - st.st_size)));

	flush_write_cache(fsp, SIZECHANGE_FLUSH);

	if (!sparse_buf) {
		sparse_buf = SMB_CALLOC_ARRAY(char, SPARSE_BUF_WRITE_SIZE);
		if (!sparse_buf) {
			errno = ENOMEM;
			return -1;
		}
	}

	offset = st.st_size;
	num_to_write = len - st.st_size;
	total = 0;

	while (total < num_to_write) {
		size_t curr_write_size = MIN(SPARSE_BUF_WRITE_SIZE, (num_to_write - total));

		pwrite_ret = SMB_VFS_PWRITE(fsp, fsp->fh->fd, sparse_buf, curr_write_size, offset + total);
		if (pwrite_ret == -1) {
			DEBUG(10,("vfs_fill_sparse: SMB_VFS_PWRITE for file %s failed with error %s\n",
				fsp->fsp_name, strerror(errno) ));
			return -1;
		}
		if (pwrite_ret == 0) {
			return 0;
		}

		total += pwrite_ret;
	}

	set_filelen_write_cache(fsp, len);
	return 0;
}

/****************************************************************************
 Transfer some data (n bytes) between two file_struct's.
****************************************************************************/

static files_struct *in_fsp;
static files_struct *out_fsp;

static ssize_t read_fn(int fd, void *buf, size_t len)
{
	return SMB_VFS_READ(in_fsp, fd, buf, len);
}

static ssize_t write_fn(int fd, const void *buf, size_t len)
{
	return SMB_VFS_WRITE(out_fsp, fd, buf, len);
}

SMB_OFF_T vfs_transfer_file(files_struct *in, files_struct *out, SMB_OFF_T n)
{
	in_fsp = in;
	out_fsp = out;

	return transfer_file_internal(in_fsp->fh->fd, out_fsp->fh->fd, n, read_fn, write_fn);
}

/*******************************************************************
 A vfs_readdir wrapper which just returns the file name.
********************************************************************/

char *vfs_readdirname(connection_struct *conn, void *p)
{
	SMB_STRUCT_DIRENT *ptr= NULL;
	char *dname;

	if (!p)
		return(NULL);

	ptr = SMB_VFS_READDIR(conn,p);
	if (!ptr)
		return(NULL);

	dname = ptr->d_name;

#ifdef NEXT2
	if (telldir(p) < 0)
		return(NULL);
#endif

#ifdef HAVE_BROKEN_READDIR_NAME
	/* using /usr/ucb/cc is BAD */
	dname = dname - 2;
#endif

	return(dname);
}

/*******************************************************************
 A wrapper for vfs_chdir().
********************************************************************/

int vfs_ChDir(connection_struct *conn, const char *path)
{
	int res;
	static pstring LastDir="";

	if (strcsequal(path,"."))
		return(0);

	if (*path == '/' && strcsequal(LastDir,path))
		return(0);

	DEBUG(4,("vfs_ChDir to %s\n",path));

	res = SMB_VFS_CHDIR(conn,path);
	if (!res)
		pstrcpy(LastDir,path);
	return(res);
}

/* number of list structures for a caching GetWd function. */
#define MAX_GETWDCACHE (50)

static struct {
	SMB_DEV_T dev; /* These *must* be compatible with the types returned in a stat() call. */
	SMB_INO_T inode; /* These *must* be compatible with the types returned in a stat() call. */
	char *dos_path; /* The pathname in DOS format. */
	BOOL valid;
} ino_list[MAX_GETWDCACHE];

extern BOOL use_getwd_cache;

/****************************************************************************
 Prompte a ptr (to make it recently used)
****************************************************************************/

static void array_promote(char *array,int elsize,int element)
{
	char *p;
	if (element == 0)
		return;

	p = (char *)SMB_MALLOC(elsize);

	if (!p) {
		DEBUG(5,("array_promote: malloc fail\n"));
		return;
	}

	memcpy(p,array + element * elsize, elsize);
	memmove(array + elsize,array,elsize*element);
	memcpy(array,p,elsize);
	SAFE_FREE(p);
}

/*******************************************************************
 Return the absolute current directory path - given a UNIX pathname.
 Note that this path is returned in DOS format, not UNIX
 format. Note this can be called with conn == NULL.
********************************************************************/

char *vfs_GetWd(connection_struct *conn, char *path)
{
	pstring s;
	static BOOL getwd_cache_init = False;
	SMB_STRUCT_STAT st, st2;
	int i;

	*s = 0;

	if (!use_getwd_cache)
		return(SMB_VFS_GETWD(conn,path));

	/* init the cache */
	if (!getwd_cache_init) {
		getwd_cache_init = True;
		for (i=0;i<MAX_GETWDCACHE;i++) {
			string_set(&ino_list[i].dos_path,"");
			ino_list[i].valid = False;
		}
	}

	/*  Get the inode of the current directory, if this doesn't work we're
		in trouble :-) */

	if (SMB_VFS_STAT(conn, ".",&st) == -1) {
		/* Known to fail for root: the directory may be
		 * NFS-mounted and exported with root_squash (so has no root access). */
		DEBUG(1,("vfs_GetWd: couldn't stat \".\" path=%s error %s (NFS problem ?)\n", path, strerror(errno) ));
		return(SMB_VFS_GETWD(conn,path));
	}


	for (i=0; i<MAX_GETWDCACHE; i++) {
		if (ino_list[i].valid) {

			/*  If we have found an entry with a matching inode and dev number
				then find the inode number for the directory in the cached string.
				If this agrees with that returned by the stat for the current
				directory then all is o.k. (but make sure it is a directory all
				the same...) */

			if (st.st_ino == ino_list[i].inode && st.st_dev == ino_list[i].dev) {
				if (SMB_VFS_STAT(conn,ino_list[i].dos_path,&st2) == 0) {
					if (st.st_ino == st2.st_ino && st.st_dev == st2.st_dev &&
							(st2.st_mode & S_IFMT) == S_IFDIR) {
						pstrcpy (path, ino_list[i].dos_path);

						/* promote it for future use */
						array_promote((char *)&ino_list[0],sizeof(ino_list[0]),i);
						return (path);
					} else {
						/*  If the inode is different then something's changed,
							scrub the entry and start from scratch. */
						ino_list[i].valid = False;
					}
				}
			}
		}
	}

	/*  We don't have the information to hand so rely on traditional methods.
		The very slow getcwd, which spawns a process on some systems, or the
		not quite so bad getwd. */

	if (!SMB_VFS_GETWD(conn,s)) {
		DEBUG(0,("vfs_GetWd: SMB_VFS_GETWD call failed, errno %s\n",strerror(errno)));
		return (NULL);
	}

	pstrcpy(path,s);

	DEBUG(5,("vfs_GetWd %s, inode %.0f, dev %.0f\n",s,(double)st.st_ino,(double)st.st_dev));

	/* add it to the cache */
	i = MAX_GETWDCACHE - 1;
	string_set(&ino_list[i].dos_path,s);
	ino_list[i].dev = st.st_dev;
	ino_list[i].inode = st.st_ino;
	ino_list[i].valid = True;

	/* put it at the top of the list */
	array_promote((char *)&ino_list[0],sizeof(ino_list[0]),i);

	return (path);
}

BOOL canonicalize_path(connection_struct *conn, pstring path)
{
#ifdef REALPATH_TAKES_NULL
	char *resolved_name = SMB_VFS_REALPATH(conn,path,NULL);
	if (!resolved_name) {
		return False;
	}
	pstrcpy(path, resolved_name);
	SAFE_FREE(resolved_name);
	return True;
#else
#ifdef PATH_MAX
        char resolved_name_buf[PATH_MAX+1];
#else
        pstring resolved_name_buf;
#endif
	char *resolved_name = SMB_VFS_REALPATH(conn,path,resolved_name_buf);
	if (!resolved_name) {
		return False;
	}
	pstrcpy(path, resolved_name);
	return True;
#endif /* REALPATH_TAKES_NULL */
}

/*******************************************************************
 Reduce a file name, removing .. elements and checking that
 it is below dir in the heirachy. This uses realpath.
********************************************************************/

BOOL reduce_name(connection_struct *conn, const pstring fname)
{
#ifdef REALPATH_TAKES_NULL
	BOOL free_resolved_name = True;
#else
#ifdef PATH_MAX
        char resolved_name_buf[PATH_MAX+1];
#else
        pstring resolved_name_buf;
#endif
	BOOL free_resolved_name = False;
#endif
	char *resolved_name = NULL;
	size_t con_path_len = strlen(conn->connectpath);
	char *p = NULL;
	int saved_errno = errno;

	DEBUG(3,("reduce_name [%s] [%s]\n", fname, conn->connectpath));

#ifdef REALPATH_TAKES_NULL
	resolved_name = SMB_VFS_REALPATH(conn,fname,NULL);
#else
	resolved_name = SMB_VFS_REALPATH(conn,fname,resolved_name_buf);
#endif

	if (!resolved_name) {
		switch (errno) {
			case ENOTDIR:
				DEBUG(3,("reduce_name: Component not a directory in getting realpath for %s\n", fname));
				errno = saved_errno;
				return False;
			case ENOENT:
			{
				pstring tmp_fname;
				fstring last_component;
				/* Last component didn't exist. Remove it and try and canonicalise the directory. */

				pstrcpy(tmp_fname, fname);
				p = strrchr_m(tmp_fname, '/');
				if (p) {
					*p++ = '\0';
					fstrcpy(last_component, p);
				} else {
					fstrcpy(last_component, tmp_fname);
					pstrcpy(tmp_fname, ".");
				}

#ifdef REALPATH_TAKES_NULL
				resolved_name = SMB_VFS_REALPATH(conn,tmp_fname,NULL);
#else
				resolved_name = SMB_VFS_REALPATH(conn,tmp_fname,resolved_name_buf);
#endif
				if (!resolved_name) {
					DEBUG(3,("reduce_name: couldn't get realpath for %s\n", fname));
					errno = saved_errno;
					return False;
				}
				pstrcpy(tmp_fname, resolved_name);
				pstrcat(tmp_fname, "/");
				pstrcat(tmp_fname, last_component);
#ifdef REALPATH_TAKES_NULL
				SAFE_FREE(resolved_name);
				resolved_name = SMB_STRDUP(tmp_fname);
				if (!resolved_name) {
					DEBUG(0,("reduce_name: malloc fail for %s\n", tmp_fname));
					errno = saved_errno;
					return False;
				}
#else
#ifdef PATH_MAX
				safe_strcpy(resolved_name_buf, tmp_fname, PATH_MAX);
#else
				pstrcpy(resolved_name_buf, tmp_fname);
#endif
				resolved_name = resolved_name_buf;
#endif
				break;
			}
			default:
				DEBUG(1,("reduce_name: couldn't get realpath for %s\n", fname));
				/* Don't restore the saved errno. We need to return the error that
				   realpath caused here as it was not one of the cases we handle. JRA. */
				return False;
		}
	}

	DEBUG(10,("reduce_name realpath [%s] -> [%s]\n", fname, resolved_name));

	if (*resolved_name != '/') {
		DEBUG(0,("reduce_name: realpath doesn't return absolute paths !\n"));
		if (free_resolved_name)
			SAFE_FREE(resolved_name);
		errno = saved_errno;
		return False;
	}

	/* Check for widelinks allowed. */
	if (!lp_widelinks(SNUM(conn)) && (strncmp(conn->connectpath, resolved_name, con_path_len) != 0)) {
		DEBUG(2, ("reduce_name: Bad access attempt: %s is a symlink outside the share path", fname));
		if (free_resolved_name)
			SAFE_FREE(resolved_name);
		errno = EACCES;
		return False;
	}

        /* Check if we are allowing users to follow symlinks */
        /* Patch from David Clerc <David.Clerc@cui.unige.ch>
                University of Geneva */
                                                                                                                                                    
#ifdef S_ISLNK
        if (!lp_symlinks(SNUM(conn))) {
                SMB_STRUCT_STAT statbuf;
                if ( (SMB_VFS_LSTAT(conn,fname,&statbuf) != -1) &&
                                (S_ISLNK(statbuf.st_mode)) ) {
			if (free_resolved_name)
				SAFE_FREE(resolved_name);
                        DEBUG(3,("reduce_name: denied: file path name %s is a symlink\n",resolved_name));
                        errno = EACCES;
			return False;
                }
        }
#endif

	DEBUG(3,("reduce_name: %s reduced to %s\n", fname, resolved_name));
	if (free_resolved_name)
		SAFE_FREE(resolved_name);
	errno = saved_errno;
	return(True);
}
