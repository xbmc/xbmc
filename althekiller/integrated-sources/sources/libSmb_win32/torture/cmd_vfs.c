/* 
   Unix SMB/CIFS implementation.
   VFS module functions

   Copyright (C) Simo Sorce 2002
   Copyright (C) Eric Lorimer 2002

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
#include "vfstest.h"

static const char *null_string = "";

static NTSTATUS cmd_load_module(struct vfs_state *vfs, TALLOC_CTX *mem_ctx, int argc, const char **argv)
{
	int i;
	
	if (argc < 2) {
		printf("Usage: load <modules>\n");
		return NT_STATUS_OK;
	}

	for (i=argc-1;i>0;i--) {
		if (!vfs_init_custom(vfs->conn, argv[i])) {
			DEBUG(0, ("load: (vfs_init_custom failed for %s)\n", argv[i]));
			return NT_STATUS_UNSUCCESSFUL;
		}
	}
	printf("load: ok\n");
	return NT_STATUS_OK;
}

static NTSTATUS cmd_populate(struct vfs_state *vfs, TALLOC_CTX *mem_ctx, int argc, const char **argv)
{
	char c;
	size_t size;
	if (argc != 3) {
		printf("Usage: populate <char> <size>\n");
		return NT_STATUS_OK;
	}
	c = argv[1][0];
	size = atoi(argv[2]);
	vfs->data = TALLOC_ARRAY(mem_ctx, char, size);
	if (vfs->data == NULL) {
		printf("populate: error=-1 (not enough memory)");
		return NT_STATUS_UNSUCCESSFUL;
	}
	memset(vfs->data, c, size);
	vfs->data_size = size;
	return NT_STATUS_OK;
}

static NTSTATUS cmd_show_data(struct vfs_state *vfs, TALLOC_CTX *mem_ctx, int argc, const char **argv)
{
	size_t offset;
	size_t len;
	if (argc != 1 && argc != 3) {
		printf("Usage: showdata [<offset> <len>]\n");
		return NT_STATUS_OK;
	}
	if (vfs->data == NULL || vfs->data_size == 0) {
		printf("show_data: error=-1 (buffer empty)\n");
		return NT_STATUS_UNSUCCESSFUL;
	}

	if (argc == 3) {
		offset = atoi(argv[1]);
		len = atoi(argv[2]);
	} else {
		offset = 0;
		len = vfs->data_size;
	}
	if ((offset + len) > vfs->data_size) {
		printf("show_data: error=-1 (not enough data in buffer)\n");
		return NT_STATUS_UNSUCCESSFUL;
	}
	dump_data(0, (char *)(vfs->data) + offset, len);
	return NT_STATUS_OK;
}

static NTSTATUS cmd_connect(struct vfs_state *vfs, TALLOC_CTX *mem_ctx, int argc, const char **argv)
{
	SMB_VFS_CONNECT(vfs->conn, lp_servicename(vfs->conn->service), "vfstest");
	return NT_STATUS_OK;
}

static NTSTATUS cmd_disconnect(struct vfs_state *vfs, TALLOC_CTX *mem_ctx, int argc, const char **argv)
{
	SMB_VFS_DISCONNECT(vfs->conn);
	return NT_STATUS_OK;
}

static NTSTATUS cmd_disk_free(struct vfs_state *vfs, TALLOC_CTX *mem_ctx, int argc, const char **argv)
{
	SMB_BIG_UINT diskfree, bsize, dfree, dsize;
	if (argc != 2) {
		printf("Usage: disk_free <path>\n");
		return NT_STATUS_OK;
	}

	diskfree = SMB_VFS_DISK_FREE(vfs->conn, argv[1], False, &bsize, &dfree, &dsize);
	printf("disk_free: %lu, bsize = %lu, dfree = %lu, dsize = %lu\n",
			(unsigned long)diskfree,
			(unsigned long)bsize,
			(unsigned long)dfree,
			(unsigned long)dsize);
	return NT_STATUS_OK;
}


static NTSTATUS cmd_opendir(struct vfs_state *vfs, TALLOC_CTX *mem_ctx, int argc, const char **argv)
{
	if (argc != 2) {
		printf("Usage: opendir <fname>\n");
		return NT_STATUS_OK;
	}

	vfs->currentdir = SMB_VFS_OPENDIR(vfs->conn, argv[1], NULL, 0);
	if (vfs->currentdir == NULL) {
		printf("opendir error=%d (%s)\n", errno, strerror(errno));
		return NT_STATUS_UNSUCCESSFUL;
	}

	printf("opendir: ok\n");
	return NT_STATUS_OK;
}


static NTSTATUS cmd_readdir(struct vfs_state *vfs, TALLOC_CTX *mem_ctx, int argc, const char **argv)
{
	SMB_STRUCT_DIRENT *dent;

	if (vfs->currentdir == NULL) {
		printf("readdir: error=-1 (no open directory)\n");
		return NT_STATUS_UNSUCCESSFUL;
	}

	dent = SMB_VFS_READDIR(vfs->conn, vfs->currentdir);
	if (dent == NULL) {
		printf("readdir: NULL\n");
		return NT_STATUS_OK;
	}

	printf("readdir: %s\n", dent->d_name);
	return NT_STATUS_OK;
}


static NTSTATUS cmd_mkdir(struct vfs_state *vfs, TALLOC_CTX *mem_ctx, int argc, const char **argv)
{
	if (argc != 2) {
		printf("Usage: mkdir <path>\n");
		return NT_STATUS_OK;
	}

	if (SMB_VFS_MKDIR(vfs->conn, argv[1], 00755) == -1) {
		printf("mkdir error=%d (%s)\n", errno, strerror(errno));
		return NT_STATUS_UNSUCCESSFUL;
	}
	
	printf("mkdir: ok\n");
	return NT_STATUS_OK;
}


static NTSTATUS cmd_closedir(struct vfs_state *vfs, TALLOC_CTX *mem_ctx, int argc, const char **argv)
{
	int ret;
	
	if (vfs->currentdir == NULL) {
		printf("closedir: failure (no directory open)\n");
		return NT_STATUS_UNSUCCESSFUL;
	}

	ret = SMB_VFS_CLOSEDIR(vfs->conn, vfs->currentdir);
	if (ret == -1) {
		printf("closedir failure: %s\n", strerror(errno));
		return NT_STATUS_UNSUCCESSFUL;
	}

	printf("closedir: ok\n");
	vfs->currentdir = NULL;
	return NT_STATUS_OK;
}


static NTSTATUS cmd_open(struct vfs_state *vfs, TALLOC_CTX *mem_ctx, int argc, const char **argv)
{
	int flags, fd;
	mode_t mode;
	const char *flagstr;

	mode = 00400;

	if (argc < 3 || argc > 5) {
		printf("Usage: open <filename> <flags> <mode>\n");
		printf("  flags: O = O_RDONLY\n");
		printf("         R = O_RDWR\n");
		printf("         W = O_WRONLY\n");
		printf("         C = O_CREAT\n");
	       	printf("         E = O_EXCL\n");
	       	printf("         T = O_TRUNC\n");
	       	printf("         A = O_APPEND\n");
	       	printf("         N = O_NONBLOCK/O_NDELAY\n");
#ifdef O_SYNC
	       	printf("         S = O_SYNC\n");
#endif
#ifdef O_NOFOLLOW
	       	printf("         F = O_NOFOLLOW\n");
#endif
		printf("  mode: see open.2\n");
		printf("        mode is ignored if C flag not present\n");
		printf("        mode defaults to 00400\n");
		return NT_STATUS_OK;
	}
	flags = 0;
	flagstr = argv[2];
	while (*flagstr) {
		switch (*flagstr) {
		case 'O':
			flags |= O_RDONLY;
			break;
		case 'R':
			flags |= O_RDWR;
			break;
		case 'W':
			flags |= O_WRONLY;
			break;
		case 'C':
			flags |= O_CREAT;
			break;
		case 'E':
			flags |= O_EXCL;
			break;
		case 'T':
			flags |= O_TRUNC;
			break;
		case 'A':
			flags |= O_APPEND;
			break;
		case 'N':
			flags |= O_NONBLOCK;
			break;
#ifdef O_SYNC
		case 'S':
			flags |= O_SYNC;
			break;
#endif
#ifdef O_NOFOLLOW
		case 'F':
			flags |= O_NOFOLLOW;
			break;
#endif
		default:
			printf("open: error=-1 (invalid flag!)\n");
			return NT_STATUS_UNSUCCESSFUL;
		}
		flagstr++;
	}
	if ((flags & O_CREAT) && argc == 4) {
		if (sscanf(argv[3], "%o", &mode) == 0) {
			printf("open: error=-1 (invalid mode!)\n");
			return NT_STATUS_UNSUCCESSFUL;
		}
	}

	fd = SMB_VFS_OPEN(vfs->conn, argv[1], flags, mode);
	if (fd == -1) {
		printf("open: error=%d (%s)\n", errno, strerror(errno));
		return NT_STATUS_UNSUCCESSFUL;
	}

	vfs->files[fd] = SMB_MALLOC_P(struct files_struct);
	vfs->files[fd]->fsp_name = SMB_STRDUP(argv[1]);
	vfs->files[fd]->fh = SMB_MALLOC_P(struct fd_handle);
	vfs->files[fd]->fh->fd = fd;
	vfs->files[fd]->conn = vfs->conn;
	printf("open: fd=%d\n", fd);
	return NT_STATUS_OK;
}


static NTSTATUS cmd_pathfunc(struct vfs_state *vfs, TALLOC_CTX *mem_ctx, int argc, const char **argv)
{
	int ret = -1;

	if (argc != 2) {
		printf("Usage: %s <path>\n", argv[0]);
		return NT_STATUS_OK;
	}

	if (strcmp("rmdir", argv[0]) == 0 ) {
		ret = SMB_VFS_RMDIR(vfs->conn, argv[1]);
	} else if (strcmp("unlink", argv[0]) == 0 ) {
		ret = SMB_VFS_UNLINK(vfs->conn, argv[1]);
	} else if (strcmp("chdir", argv[0]) == 0 ) {
		ret = SMB_VFS_CHDIR(vfs->conn, argv[1]);
	} else {
		printf("%s: error=%d (invalid function name!)\n", argv[0], errno);
		return NT_STATUS_UNSUCCESSFUL;
	}

	if (ret == -1) {
		printf("%s: error=%d (%s)\n", argv[0], errno, strerror(errno));
		return NT_STATUS_UNSUCCESSFUL;
	}

	printf("%s: ok\n", argv[0]);
	return NT_STATUS_OK;
}


static NTSTATUS cmd_close(struct vfs_state *vfs, TALLOC_CTX *mem_ctx, int argc, const char **argv)
{
	int fd, ret;

	if (argc != 2) {
		printf("Usage: close <fd>\n");
		return NT_STATUS_OK;
	}

	fd = atoi(argv[1]);
	if (vfs->files[fd] == NULL) {
		printf("close: error=-1 (invalid file descriptor)\n");
		return NT_STATUS_OK;
	}

	ret = SMB_VFS_CLOSE(vfs->files[fd], fd);
	if (ret == -1 )
		printf("close: error=%d (%s)\n", errno, strerror(errno));
	else
		printf("close: ok\n");

	SAFE_FREE(vfs->files[fd]->fsp_name);
	SAFE_FREE(vfs->files[fd]->fh);
	SAFE_FREE(vfs->files[fd]);
	vfs->files[fd] = NULL;
	return NT_STATUS_OK;
}


static NTSTATUS cmd_read(struct vfs_state *vfs, TALLOC_CTX *mem_ctx, int argc, const char **argv)
{
	int fd;
	size_t size, rsize;

	if (argc != 3) {
		printf("Usage: read <fd> <size>\n");
		return NT_STATUS_OK;
	}

	/* do some error checking on these */
	fd = atoi(argv[1]);
	size = atoi(argv[2]);
	vfs->data = TALLOC_ARRAY(mem_ctx, char, size);
	if (vfs->data == NULL) {
		printf("read: error=-1 (not enough memory)");
		return NT_STATUS_UNSUCCESSFUL;
	}
	vfs->data_size = size;
	
	rsize = SMB_VFS_READ(vfs->files[fd], fd, vfs->data, size);
	if (rsize == -1) {
		printf("read: error=%d (%s)\n", errno, strerror(errno));
		return NT_STATUS_UNSUCCESSFUL;
	}

	printf("read: ok\n");
	return NT_STATUS_OK;
}


static NTSTATUS cmd_write(struct vfs_state *vfs, TALLOC_CTX *mem_ctx, int argc, const char **argv)
{
	int fd, size, wsize;

	if (argc != 3) {
		printf("Usage: write <fd> <size>\n");
		return NT_STATUS_OK;
	}

	/* some error checking should go here */
	fd = atoi(argv[1]);
	size = atoi(argv[2]);
	if (vfs->data == NULL) {
		printf("write: error=-1 (buffer empty, please populate it before writing)");
		return NT_STATUS_UNSUCCESSFUL;
	}

	if (vfs->data_size < size) {
		printf("write: error=-1 (buffer too small, please put some more data in)");
		return NT_STATUS_UNSUCCESSFUL;
	}

	wsize = SMB_VFS_WRITE(vfs->files[fd], fd, vfs->data, size);

	if (wsize == -1) {
		printf("write: error=%d (%s)\n", errno, strerror(errno));
		return NT_STATUS_UNSUCCESSFUL;
	}

	printf("write: ok\n");
	return NT_STATUS_OK;
}


static NTSTATUS cmd_lseek(struct vfs_state *vfs, TALLOC_CTX *mem_ctx, int argc, const char **argv)
{
	int fd, offset, whence;
	SMB_OFF_T pos;

	if (argc != 4) {
		printf("Usage: lseek <fd> <offset> <whence>\n...where whence is 1 => SEEK_SET, 2 => SEEK_CUR, 3 => SEEK_END\n");
		return NT_STATUS_OK;
	}

	fd = atoi(argv[1]);
	offset = atoi(argv[2]);
	whence = atoi(argv[3]);
	switch (whence) {
		case 1:		whence = SEEK_SET; break;
		case 2:		whence = SEEK_CUR; break;
		default:	whence = SEEK_END;
	}

	pos = SMB_VFS_LSEEK(vfs->files[fd], fd, offset, whence);
	if (pos == (SMB_OFF_T)-1) {
		printf("lseek: error=%d (%s)\n", errno, strerror(errno));
		return NT_STATUS_UNSUCCESSFUL;
	}

	printf("lseek: ok\n");
	return NT_STATUS_OK;
}


static NTSTATUS cmd_rename(struct vfs_state *vfs, TALLOC_CTX *mem_ctx, int argc, const char **argv)
{
	int ret;
	if (argc != 3) {
		printf("Usage: rename <old> <new>\n");
		return NT_STATUS_OK;
	}

	ret = SMB_VFS_RENAME(vfs->conn, argv[1], argv[2]);
	if (ret == -1) {
		printf("rename: error=%d (%s)\n", errno, strerror(errno));
		return NT_STATUS_UNSUCCESSFUL;
	}

	printf("rename: ok\n");
	return NT_STATUS_OK;
}


static NTSTATUS cmd_fsync(struct vfs_state *vfs, TALLOC_CTX *mem_ctx, int argc, const char **argv)
{
	int ret, fd;
	if (argc != 2) {
		printf("Usage: fsync <fd>\n");
		return NT_STATUS_OK;
	}

	fd = atoi(argv[1]);
	ret = SMB_VFS_FSYNC(vfs->files[fd], fd);
	if (ret == -1) {
		printf("fsync: error=%d (%s)\n", errno, strerror(errno));
		return NT_STATUS_UNSUCCESSFUL;
	}

	printf("fsync: ok\n");
	return NT_STATUS_OK;
}


static NTSTATUS cmd_stat(struct vfs_state *vfs, TALLOC_CTX *mem_ctx, int argc, const char **argv)
{
	int ret;
	const char *user;
	const char *group;
	struct passwd *pwd = NULL;
	struct group *grp = NULL;
	SMB_STRUCT_STAT st;

	if (argc != 2) {
		printf("Usage: stat <fname>\n");
		return NT_STATUS_OK;
	}

	ret = SMB_VFS_STAT(vfs->conn, argv[1], &st);
	if (ret == -1) {
		printf("stat: error=%d (%s)\n", errno, strerror(errno));
		return NT_STATUS_UNSUCCESSFUL;
	}

	pwd = sys_getpwuid(st.st_uid);
	if (pwd != NULL) user = pwd->pw_name;
	else user = null_string;
	grp = sys_getgrgid(st.st_gid);
	if (grp != NULL) group = grp->gr_name;
	else group = null_string;

	printf("stat: ok\n");
	printf("  File: %s", argv[1]);
	if (S_ISREG(st.st_mode)) printf("  Regular File\n");
	else if (S_ISDIR(st.st_mode)) printf("  Directory\n");
	else if (S_ISCHR(st.st_mode)) printf("  Character Device\n");
	else if (S_ISBLK(st.st_mode)) printf("  Block Device\n");
	else if (S_ISFIFO(st.st_mode)) printf("  Fifo\n");
	else if (S_ISLNK(st.st_mode)) printf("  Symbolic Link\n");
	else if (S_ISSOCK(st.st_mode)) printf("  Socket\n");
	printf("  Size: %10u", (unsigned int)st.st_size);
#ifdef HAVE_STAT_ST_BLOCKS
	printf(" Blocks: %9u", (unsigned int)st.st_blocks);
#endif
#ifdef HAVE_STAT_ST_BLKSIZE
	printf(" IO Block: %u\n", (unsigned int)st.st_blksize);
#endif
	printf("  Device: 0x%10x", (unsigned int)st.st_dev);
	printf(" Inode: %10u", (unsigned int)st.st_ino);
	printf(" Links: %10u\n", (unsigned int)st.st_nlink);
	printf("  Access: %05o", (st.st_mode) & 007777);
	printf(" Uid: %5lu/%.16s Gid: %5lu/%.16s\n", (unsigned long)st.st_uid, user, 
	       (unsigned long)st.st_gid, group);
	printf("  Access: %s", ctime(&(st.st_atime)));
	printf("  Modify: %s", ctime(&(st.st_mtime)));
	printf("  Change: %s", ctime(&(st.st_ctime)));

	return NT_STATUS_OK;
}


static NTSTATUS cmd_fstat(struct vfs_state *vfs, TALLOC_CTX *mem_ctx, int argc, const char **argv)
{
	int fd;
	const char *user;
	const char *group;
	struct passwd *pwd = NULL;
	struct group *grp = NULL;
	SMB_STRUCT_STAT st;

	if (argc != 2) {
		printf("Usage: fstat <fd>\n");
		return NT_STATUS_OK;
	}

	fd = atoi(argv[1]);
	if (fd < 0 || fd > 1024) {
		printf("fstat: error=%d (file descriptor out of range)\n", EBADF);
		return NT_STATUS_OK;
	}

	if (vfs->files[fd] == NULL) {
		printf("fstat: error=%d (invalid file descriptor)\n", EBADF);
		return NT_STATUS_OK;
	}

	if (SMB_VFS_FSTAT(vfs->files[fd], fd, &st) == -1) {
		printf("fstat: error=%d (%s)\n", errno, strerror(errno));
		return NT_STATUS_UNSUCCESSFUL;
	}

	pwd = sys_getpwuid(st.st_uid);
	if (pwd != NULL) user = pwd->pw_name;
	else user = null_string;
	grp = sys_getgrgid(st.st_gid);
	if (grp != NULL) group = grp->gr_name;
	else group = null_string;

	printf("fstat: ok\n");
	if (S_ISREG(st.st_mode)) printf("  Regular File\n");
	else if (S_ISDIR(st.st_mode)) printf("  Directory\n");
	else if (S_ISCHR(st.st_mode)) printf("  Character Device\n");
	else if (S_ISBLK(st.st_mode)) printf("  Block Device\n");
	else if (S_ISFIFO(st.st_mode)) printf("  Fifo\n");
	else if (S_ISLNK(st.st_mode)) printf("  Symbolic Link\n");
	else if (S_ISSOCK(st.st_mode)) printf("  Socket\n");
	printf("  Size: %10u", (unsigned int)st.st_size);
#ifdef HAVE_STAT_ST_BLOCKS
	printf(" Blocks: %9u", (unsigned int)st.st_blocks);
#endif
#ifdef HAVE_STAT_ST_BLKSIZE
	printf(" IO Block: %u\n", (unsigned int)st.st_blksize);
#endif
	printf("  Device: 0x%10x", (unsigned int)st.st_dev);
	printf(" Inode: %10u", (unsigned int)st.st_ino);
	printf(" Links: %10u\n", (unsigned int)st.st_nlink);
	printf("  Access: %05o", (st.st_mode) & 007777);
	printf(" Uid: %5lu/%.16s Gid: %5lu/%.16s\n", (unsigned long)st.st_uid, user, 
	       (unsigned long)st.st_gid, group);
	printf("  Access: %s", ctime(&(st.st_atime)));
	printf("  Modify: %s", ctime(&(st.st_mtime)));
	printf("  Change: %s", ctime(&(st.st_ctime)));

	return NT_STATUS_OK;
}


static NTSTATUS cmd_lstat(struct vfs_state *vfs, TALLOC_CTX *mem_ctx, int argc, const char **argv)
{
	const char *user;
	const char *group;
	struct passwd *pwd = NULL;
	struct group *grp = NULL;
	SMB_STRUCT_STAT st;

	if (argc != 2) {
		printf("Usage: lstat <path>\n");
		return NT_STATUS_OK;
	}

	if (SMB_VFS_LSTAT(vfs->conn, argv[1], &st) == -1) {
		printf("lstat: error=%d (%s)\n", errno, strerror(errno));
		return NT_STATUS_UNSUCCESSFUL;
	}

	pwd = sys_getpwuid(st.st_uid);
	if (pwd != NULL) user = pwd->pw_name;
	else user = null_string;
	grp = sys_getgrgid(st.st_gid);
	if (grp != NULL) group = grp->gr_name;
	else group = null_string;

	printf("lstat: ok\n");
	if (S_ISREG(st.st_mode)) printf("  Regular File\n");
	else if (S_ISDIR(st.st_mode)) printf("  Directory\n");
	else if (S_ISCHR(st.st_mode)) printf("  Character Device\n");
	else if (S_ISBLK(st.st_mode)) printf("  Block Device\n");
	else if (S_ISFIFO(st.st_mode)) printf("  Fifo\n");
	else if (S_ISLNK(st.st_mode)) printf("  Symbolic Link\n");
	else if (S_ISSOCK(st.st_mode)) printf("  Socket\n");
	printf("  Size: %10u", (unsigned int)st.st_size);
#ifdef HAVE_STAT_ST_BLOCKS
	printf(" Blocks: %9u", (unsigned int)st.st_blocks);
#endif
#ifdef HAVE_STAT_ST_BLKSIZE
	printf(" IO Block: %u\n", (unsigned int)st.st_blksize);
#endif
	printf("  Device: 0x%10x", (unsigned int)st.st_dev);
	printf(" Inode: %10u", (unsigned int)st.st_ino);
	printf(" Links: %10u\n", (unsigned int)st.st_nlink);
	printf("  Access: %05o", (st.st_mode) & 007777);
	printf(" Uid: %5lu/%.16s Gid: %5lu/%.16s\n", (unsigned long)st.st_uid, user, 
	       (unsigned long)st.st_gid, group);
	printf("  Access: %s", ctime(&(st.st_atime)));
	printf("  Modify: %s", ctime(&(st.st_mtime)));
	printf("  Change: %s", ctime(&(st.st_ctime)));
	
	return NT_STATUS_OK;
}


static NTSTATUS cmd_chmod(struct vfs_state *vfs, TALLOC_CTX *mem_ctx, int argc, const char **argv)
{
	mode_t mode;
	if (argc != 3) {
		printf("Usage: chmod <path> <mode>\n");
		return NT_STATUS_OK;
	}

	mode = atoi(argv[2]);
	if (SMB_VFS_CHMOD(vfs->conn, argv[1], mode) == -1) {
		printf("chmod: error=%d (%s)\n", errno, strerror(errno));
		return NT_STATUS_UNSUCCESSFUL;
	}

	printf("chmod: ok\n");
	return NT_STATUS_OK;
}


static NTSTATUS cmd_fchmod(struct vfs_state *vfs, TALLOC_CTX *mem_ctx, int argc, const char **argv)
{
	int fd;
	mode_t mode;
	if (argc != 3) {
		printf("Usage: fchmod <fd> <mode>\n");
		return NT_STATUS_OK;
	}

	fd = atoi(argv[1]);
	mode = atoi(argv[2]);
	if (fd < 0 || fd > 1024) {
		printf("fchmod: error=%d (file descriptor out of range)\n", EBADF);
		return NT_STATUS_OK;
	}
	if (vfs->files[fd] == NULL) {
		printf("fchmod: error=%d (invalid file descriptor)\n", EBADF);
		return NT_STATUS_OK;
	}

	if (SMB_VFS_FCHMOD(vfs->files[fd], fd, mode) == -1) {
		printf("fchmod: error=%d (%s)\n", errno, strerror(errno));
		return NT_STATUS_UNSUCCESSFUL;
	}

	printf("fchmod: ok\n");
	return NT_STATUS_OK;
}


static NTSTATUS cmd_chown(struct vfs_state *vfs, TALLOC_CTX *mem_ctx, int argc, const char **argv)
{
	uid_t uid;
	gid_t gid;
	if (argc != 4) {
		printf("Usage: chown <path> <uid> <gid>\n");
		return NT_STATUS_OK;
	}

	uid = atoi(argv[2]);
	gid = atoi(argv[3]);
	if (SMB_VFS_CHOWN(vfs->conn, argv[1], uid, gid) == -1) {
		printf("chown: error=%d (%s)\n", errno, strerror(errno));
		return NT_STATUS_UNSUCCESSFUL;
	}

	printf("chown: ok\n");
	return NT_STATUS_OK;
}


static NTSTATUS cmd_fchown(struct vfs_state *vfs, TALLOC_CTX *mem_ctx, int argc, const char **argv)
{
	uid_t uid;
	gid_t gid;
	int fd;
	if (argc != 4) {
		printf("Usage: fchown <fd> <uid> <gid>\n");
		return NT_STATUS_OK;
	}

	uid = atoi(argv[2]);
	gid = atoi(argv[3]);
	fd = atoi(argv[1]);
	if (fd < 0 || fd > 1024) {
		printf("fchown: faliure=%d (file descriptor out of range)\n", EBADF);
		return NT_STATUS_OK;
	}
	if (vfs->files[fd] == NULL) {
		printf("fchown: error=%d (invalid file descriptor)\n", EBADF);
		return NT_STATUS_OK;
	}
	if (SMB_VFS_FCHOWN(vfs->files[fd], fd, uid, gid) == -1) {
		printf("fchown error=%d (%s)\n", errno, strerror(errno));
		return NT_STATUS_UNSUCCESSFUL;
	}

	printf("fchown: ok\n");
	return NT_STATUS_OK;
}


static NTSTATUS cmd_getwd(struct vfs_state *vfs, TALLOC_CTX *mem_ctx, int argc, const char **argv)
{
	char buf[PATH_MAX];
	if (SMB_VFS_GETWD(vfs->conn, buf) == NULL) {
		printf("getwd: error=%d (%s)\n", errno, strerror(errno));
		return NT_STATUS_UNSUCCESSFUL;
	}

	printf("getwd: %s\n", buf);
	return NT_STATUS_OK;
}

static NTSTATUS cmd_utime(struct vfs_state *vfs, TALLOC_CTX *mem_ctx, int argc, const char **argv)
{
	struct utimbuf times;
	if (argc != 4) {
		printf("Usage: utime <path> <access> <modify>\n");
		return NT_STATUS_OK;
	}
	times.actime = atoi(argv[2]);
	times.modtime = atoi(argv[3]);
	if (SMB_VFS_UTIME(vfs->conn, argv[1], &times) != 0) {
		printf("utime: error=%d (%s)\n", errno, strerror(errno));
		return NT_STATUS_UNSUCCESSFUL;
	}

	printf("utime: ok\n");
	return NT_STATUS_OK;
}

static NTSTATUS cmd_ftruncate(struct vfs_state *vfs, TALLOC_CTX *mem_ctx, int argc, const char **argv)
{
	int fd;
	SMB_OFF_T off;
	if (argc != 3) {
		printf("Usage: ftruncate <fd> <length>\n");
		return NT_STATUS_OK;
	}

	fd = atoi(argv[1]);
	off = atoi(argv[2]);
	if (fd < 0 || fd > 1024) {
		printf("ftruncate: error=%d (file descriptor out of range)\n", EBADF);
		return NT_STATUS_OK;
	}
	if (vfs->files[fd] == NULL) {
		printf("ftruncate: error=%d (invalid file descriptor)\n", EBADF);
		return NT_STATUS_OK;
	}

	if (SMB_VFS_FTRUNCATE(vfs->files[fd], fd, off) == -1) {
		printf("ftruncate: error=%d (%s)\n", errno, strerror(errno));
		return NT_STATUS_UNSUCCESSFUL;
	}

	printf("ftruncate: ok\n");
	return NT_STATUS_OK;
}

static NTSTATUS cmd_lock(struct vfs_state *vfs, TALLOC_CTX *mem_ctx, int argc, const char **argv)
{
	BOOL ret;
	int fd;
	int op;
	long offset;
	long count;
	int type;
	const char *typestr;
	
	if (argc != 6) {
		printf("Usage: lock <fd> <op> <offset> <count> <type>\n");
                printf("  ops: G = F_GETLK\n");
                printf("       S = F_SETLK\n");
                printf("       W = F_SETLKW\n");
                printf("  type: R = F_RDLCK\n");
                printf("        W = F_WRLCK\n");
                printf("        U = F_UNLCK\n");
		return NT_STATUS_OK;
	}

	if (sscanf(argv[1], "%d", &fd) == 0) {
		printf("lock: error=-1 (error parsing fd)\n");
		return NT_STATUS_UNSUCCESSFUL;
	}

	op = 0;
	switch (*argv[2]) {
	case 'G':
		op = F_GETLK;
		break;
	case 'S':
		op = F_SETLK;
		break;
	case 'W':
		op = F_SETLKW;
		break;
	default:
		printf("lock: error=-1 (invalid op flag!)\n");
		return NT_STATUS_UNSUCCESSFUL;
	}

	if (sscanf(argv[3], "%ld", &offset) == 0) {
		printf("lock: error=-1 (error parsing fd)\n");
		return NT_STATUS_UNSUCCESSFUL;
	}

	if (sscanf(argv[4], "%ld", &count) == 0) {
		printf("lock: error=-1 (error parsing fd)\n");
		return NT_STATUS_UNSUCCESSFUL;
	}

	type = 0;
	typestr = argv[5];
	while(*typestr) {
		switch (*typestr) {
		case 'R':
			type |= F_RDLCK;
			break;
		case 'W':
			type |= F_WRLCK;
			break;
		case 'U':
			type |= F_UNLCK;
			break;
		default:
			printf("lock: error=-1 (invalid type flag!)\n");
			return NT_STATUS_UNSUCCESSFUL;
		}
		typestr++;
	}

	printf("lock: debug lock(fd=%d, op=%d, offset=%ld, count=%ld, type=%d))\n", fd, op, offset, count, type);

	if ((ret = SMB_VFS_LOCK(vfs->files[fd], fd, op, offset, count, type)) == False) {
		printf("lock: error=%d (%s)\n", errno, strerror(errno));
		return NT_STATUS_UNSUCCESSFUL;
	}

	printf("lock: ok\n");
	return NT_STATUS_OK;
}

static NTSTATUS cmd_symlink(struct vfs_state *vfs, TALLOC_CTX *mem_ctx, int argc, const char **argv)
{
	if (argc != 3) {
		printf("Usage: symlink <path> <link>\n");
		return NT_STATUS_OK;
	}

	if (SMB_VFS_SYMLINK(vfs->conn, argv[1], argv[2]) == -1) {
		printf("symlink: error=%d (%s)\n", errno, strerror(errno));
		return NT_STATUS_UNSUCCESSFUL;
	}

	printf("symlink: ok\n");
	return NT_STATUS_OK;
}


static NTSTATUS cmd_readlink(struct vfs_state *vfs, TALLOC_CTX *mem_ctx, int argc, const char **argv)
{
	char buffer[PATH_MAX];
	int size;

	if (argc != 2) {
		printf("Usage: readlink <path>\n");
		return NT_STATUS_OK;
	}

	if ((size = SMB_VFS_READLINK(vfs->conn, argv[1], buffer, PATH_MAX)) == -1) {
		printf("readlink: error=%d (%s)\n", errno, strerror(errno));
		return NT_STATUS_UNSUCCESSFUL;
	}

	buffer[size] = '\0';
	printf("readlink: %s\n", buffer);
	return NT_STATUS_OK;
}


static NTSTATUS cmd_link(struct vfs_state *vfs, TALLOC_CTX *mem_ctx, int argc, const char **argv)
{
	if (argc != 3) {
		printf("Usage: link <path> <link>\n");
		return NT_STATUS_OK;
	}

	if (SMB_VFS_LINK(vfs->conn, argv[1], argv[2]) == -1) {
		printf("link: error=%d (%s)\n", errno, strerror(errno));
		return NT_STATUS_UNSUCCESSFUL;
	}

	printf("link: ok\n");
	return NT_STATUS_OK;
}

static NTSTATUS cmd_mknod(struct vfs_state *vfs, TALLOC_CTX *mem_ctx, int argc, const char **argv)
{
	mode_t mode;
	unsigned int dev_val;
	SMB_DEV_T dev;
	
	if (argc != 4) {
		printf("Usage: mknod <path> <mode> <dev>\n");
		printf("  mode is octal\n");
		printf("  dev is hex\n");
		return NT_STATUS_OK;
	}

	if (sscanf(argv[2], "%o", &mode) == 0) {
		printf("open: error=-1 (invalid mode!)\n");
		return NT_STATUS_UNSUCCESSFUL;
	}

	if (sscanf(argv[3], "%x", &dev_val) == 0) {
		printf("open: error=-1 (invalid dev!)\n");
		return NT_STATUS_UNSUCCESSFUL;
	}
	dev = (SMB_DEV_T)dev_val;

	if (SMB_VFS_MKNOD(vfs->conn, argv[1], mode, dev) == -1) {
		printf("mknod: error=%d (%s)\n", errno, strerror(errno));
		return NT_STATUS_UNSUCCESSFUL;
	}

	printf("mknod: ok\n");
	return NT_STATUS_OK;
}

static NTSTATUS cmd_realpath(struct vfs_state *vfs, TALLOC_CTX *mem_ctx, int argc, const char **argv)
{
	char respath[PATH_MAX];
	
	if (argc != 2) {
		printf("Usage: realpath <path>\n");
		return NT_STATUS_OK;
	}

	if (SMB_VFS_REALPATH(vfs->conn, argv[1], respath) == NULL) {
		printf("realpath: error=%d (%s)\n", errno, strerror(errno));
		return NT_STATUS_UNSUCCESSFUL;
	}

	printf("realpath: ok\n");
	return NT_STATUS_OK;
}

struct cmd_set vfs_commands[] = {

	{ "VFS Commands" },

	{ "load", cmd_load_module, "Load a module", "load <module.so>" },
	{ "populate", cmd_populate, "Populate a data buffer", "populate <char> <size>" },
	{ "showdata", cmd_show_data, "Show data currently in data buffer", "show_data [<offset> <len>]"},
	{ "connect",   cmd_connect,   "VFS connect()",    "connect" },
	{ "disconnect",   cmd_disconnect,   "VFS disconnect()",    "disconnect" },
	{ "disk_free",   cmd_disk_free,   "VFS disk_free()",    "disk_free <path>" },
	{ "opendir",   cmd_opendir,   "VFS opendir()",    "opendir <fname>" },
	{ "readdir",   cmd_readdir,   "VFS readdir()",    "readdir" },
	{ "mkdir",   cmd_mkdir,   "VFS mkdir()",    "mkdir <path>" },
	{ "rmdir",   cmd_pathfunc,   "VFS rmdir()",    "rmdir <path>" },
	{ "closedir",   cmd_closedir,   "VFS closedir()",    "closedir" },
	{ "open",   cmd_open,   "VFS open()",    "open <fname>" },
	{ "close",   cmd_close,   "VFS close()",    "close <fd>" },
	{ "read",   cmd_read,   "VFS read()",    "read <fd> <size>" },
	{ "write",   cmd_write,   "VFS write()",    "write <fd> <size>" },
	{ "lseek",   cmd_lseek,   "VFS lseek()",    "lseek <fd> <offset> <whence>" },
	{ "rename",   cmd_rename,   "VFS rename()",    "rename <old> <new>" },
	{ "fsync",   cmd_fsync,   "VFS fsync()",    "fsync <fd>" },
	{ "stat",   cmd_stat,   "VFS stat()",    "stat <fname>" },
	{ "fstat",   cmd_fstat,   "VFS fstat()",    "fstat <fd>" },
	{ "lstat",   cmd_lstat,   "VFS lstat()",    "lstat <fname>" },
	{ "unlink",   cmd_pathfunc,   "VFS unlink()",    "unlink <fname>" },
	{ "chmod",   cmd_chmod,   "VFS chmod()",    "chmod <path> <mode>" },
	{ "fchmod",   cmd_fchmod,   "VFS fchmod()",    "fchmod <fd> <mode>" },
	{ "chown",   cmd_chown,   "VFS chown()",    "chown <path> <uid> <gid>" },
	{ "fchown",   cmd_fchown,   "VFS fchown()",    "fchown <fd> <uid> <gid>" },
	{ "chdir",   cmd_pathfunc,   "VFS chdir()",    "chdir <path>" },
	{ "getwd",   cmd_getwd,   "VFS getwd()",    "getwd" },
	{ "utime",   cmd_utime,   "VFS utime()",    "utime <path> <access> <modify>" },
	{ "ftruncate",   cmd_ftruncate,   "VFS ftruncate()",    "ftruncate <fd> <length>" },
	{ "lock",   cmd_lock,   "VFS lock()",    "lock <f> <op> <offset> <count> <type>" },
	{ "symlink",   cmd_symlink,   "VFS symlink()",    "symlink <old> <new>" },
	{ "readlink",   cmd_readlink,   "VFS readlink()",    "readlink <path>" },
	{ "link",   cmd_link,   "VFS link()",    "link <oldpath> <newpath>" },
	{ "mknod",   cmd_mknod,   "VFS mknod()",    "mknod <path> <mode> <dev>" },
	{ "realpath",   cmd_realpath,   "VFS realpath()",    "realpath <path>" },
	{ NULL }
};
