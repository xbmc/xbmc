/*
*      Copyright (C) 2010 Team XBMC
*      http://www.xbmc.org
*
*  This Program is free software; you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation; either version 2, or (at your option)
*  any later version.
*
*  This Program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with XBMC; see the file COPYING.  If not, write to
*  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
*  http://www.gnu.org/copyleft/gpl.html
*
*/

#ifndef VFSPROVIDER_H
#define VFSPROVIDER_H

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/statvfs.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Function to add an entry in a readdir() operation
 *
 * @param buf the buffer passed to the readdir() operation
 * @param name the file name of the directory entry
 * @param stat file attributes, can be NULL
 * @param off offset of the next entry or zero
 * @return 1 if buffer is full, zero otherwise
 */
typedef int (*xbmc_vfs_fill_dir_t) (void *buf, const char *name,
				const struct stat *stbuf, off_t off);

/**
 * VFS operations:
 *
 */
struct xbmc_vfs_operations {

	/** Provider specific runtime data */
	void *data;

	/** Get file attributes */
	int (*stat) (const char *, struct stat *);

	/** Create a directory */
	int (*mkdir) (const char *, mode_t);

	/** Remove a directory */
	int (*rmdir) (const char *);

	/** Rename a file */
	int (*rename) (const char *, const char *);

	/** Change the size of a file */
	int (*truncate) (const char *, off_t);

	/** File open operation */
	int (*open) (const char *, struct fuse_file_info *);

	/** Read data from an open file */
	int (*read) (const char *, char *, size_t, off_t,
		     struct fuse_file_info *);

	/** Check file access permissions */
	int (*access) (const char *, int);

	/** Write data to an open file */
	int (*write) (const char *, const char *, size_t, off_t,
		      struct fuse_file_info *);

	/** Create and open a file */
	int (*create) (const char *, mode_t, struct fuse_file_info *);

	/** Change the size of an open file */
	int (*ftruncate) (const char *, off_t, struct fuse_file_info *);

	/** Get attributes from an open file */
	int (*fgetattr) (const char *, struct stat *, struct fuse_file_info *);

	/** Get file system statistics */
	int (*statfs) (const char *, struct statvfs *);

	/** Possibly flush cached data */
	int (*flush) (const char *, struct fuse_file_info *);

	/** Release an open file */
	int (*release) (const char *, struct fuse_file_info *);

	/** Synchronize file contents */
	int (*fsync) (const char *, int, struct fuse_file_info *);

	/** Set extended attributes */
	int (*setxattr) (const char *, const char *, const char *, size_t, int);

	/** Get extended attributes */
	int (*getxattr) (const char *, const char *, char *, size_t);

	/** List extended attributes */
	int (*listxattr) (const char *, char *, size_t);

	/** Remove extended attributes */
	int (*removexattr) (const char *, const char *);

	/** Open directory */
	int (*opendir) (const char *, struct fuse_file_info *);

	/** Read directory */
	int (*readdir) (const char *, void *, fuse_fill_dir_t, off_t,
			struct fuse_file_info *);

	/** Release directory */
	int (*releasedir) (const char *, struct fuse_file_info *);

};

#endif /*VFSPRIVIDER_H*/
