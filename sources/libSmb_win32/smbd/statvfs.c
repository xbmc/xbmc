/* 
   Unix SMB/CIFS implementation.
   VFS API's statvfs abstraction
   Copyright (C) Alexander Bokovoy			2005
   Copyright (C) Steve French				2005
   
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

#if defined(LINUX)
static int linux_statvfs(const char *path, vfs_statvfs_struct *statbuf)
{
	struct statvfs statvfs_buf;
	int result;

	result = statvfs(path, &statvfs_buf);

	if (!result) {
		statbuf->OptimalTransferSize = statvfs_buf.f_frsize;
		statbuf->BlockSize = statvfs_buf.f_bsize;
		statbuf->TotalBlocks = statvfs_buf.f_blocks;
		statbuf->BlocksAvail = statvfs_buf.f_bfree;
		statbuf->UserBlocksAvail = statvfs_buf.f_bavail;
		statbuf->TotalFileNodes = statvfs_buf.f_files;
		statbuf->FreeFileNodes = statvfs_buf.f_ffree;
		statbuf->FsIdentifier = statvfs_buf.f_fsid;
	}
	return result;
}
#endif

/* 
 sys_statvfs() is an abstraction layer over system-dependent statvfs()/statfs()
 for particular POSIX systems. Due to controversy of what is considered more important
 between LSB and FreeBSD/POSIX.1 (IEEE Std 1003.1-2001) we need to abstract the interface
 so that particular OS would use its preffered interface.
*/
int sys_statvfs(const char *path, vfs_statvfs_struct *statbuf)
{
#if defined(LINUX)
	return linux_statvfs(path, statbuf);
#else
	/* BB change this to return invalid level */
#ifdef EOPNOTSUPP
	return EOPNOTSUPP;
#else
	return -1;
#endif /* EOPNOTSUPP */
#endif /* LINUX */

}
