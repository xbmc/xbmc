/* 
   Unix SMB/CIFS implementation.
   functions to calculate the free disk space
   Copyright (C) Andrew Tridgell 1998-2000
   
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


/* Return the number of TOSIZE-byte blocks used by
   BLOCKS FROMSIZE-byte blocks, rounding away from zero.
*/
static SMB_BIG_UINT adjust_blocks(SMB_BIG_UINT blocks, SMB_BIG_UINT fromsize, SMB_BIG_UINT tosize)
{
	if (fromsize == tosize)	{ /* e.g., from 512 to 512 */
		return blocks;
	} else if (fromsize > tosize) { /* e.g., from 2048 to 512 */
		return blocks * (fromsize / tosize);
	} else { /* e.g., from 256 to 512 */
		/* Protect against broken filesystems... */
		if (fromsize == 0) {
			fromsize = tosize;
		}
		return (blocks + 1) / (tosize / fromsize);
	}
}

/* this does all of the system specific guff to get the free disk space.
   It is derived from code in the GNU fileutils package, but has been
   considerably mangled for use here 

   results are returned in *dfree and *dsize, in 512 byte units
*/
int sys_fsusage(const char *path, SMB_BIG_UINT *dfree, SMB_BIG_UINT *dsize)
{
#ifdef STAT_STATFS3_OSF1
#define CONVERT_BLOCKS(B) adjust_blocks ((SMB_BIG_UINT)(B), (SMB_BIG_UINT)fsd.f_fsize, (SMB_BIG_UINT)512)
	struct statfs fsd;

	if (statfs (path, &fsd, sizeof (struct statfs)) != 0)
		return -1;
#endif /* STAT_STATFS3_OSF1 */
	
#ifdef STAT_STATFS2_FS_DATA	/* Ultrix */
#define CONVERT_BLOCKS(B) adjust_blocks ((SMB_BIG_UINT)(B), (SMB_BIG_UINT)1024, (SMB_BIG_UINT)512)	
	struct fs_data fsd;
	
	if (statfs (path, &fsd) != 1)
		return -1;
	
	(*dsize) = CONVERT_BLOCKS (fsd.fd_req.btot);
	(*dfree) = CONVERT_BLOCKS (fsd.fd_req.bfreen);
#endif /* STAT_STATFS2_FS_DATA */
	
#ifdef STAT_STATFS2_BSIZE	/* 4.3BSD, SunOS 4, HP-UX, AIX */
#define CONVERT_BLOCKS(B) adjust_blocks ((SMB_BIG_UINT)(B), (SMB_BIG_UINT)fsd.f_bsize, (SMB_BIG_UINT)512)
	struct statfs fsd;
	
	if (statfs (path, &fsd) < 0)
		return -1;
	
#ifdef STATFS_TRUNCATES_BLOCK_COUNTS
	/* In SunOS 4.1.2, 4.1.3, and 4.1.3_U1, the block counts in the
	   struct statfs are truncated to 2GB.  These conditions detect that
	   truncation, presumably without botching the 4.1.1 case, in which
	   the values are not truncated.  The correct counts are stored in
	   undocumented spare fields.  */
	if (fsd.f_blocks == 0x1fffff && fsd.f_spare[0] > 0) {
		fsd.f_blocks = fsd.f_spare[0];
		fsd.f_bfree = fsd.f_spare[1];
		fsd.f_bavail = fsd.f_spare[2];
	}
#endif /* STATFS_TRUNCATES_BLOCK_COUNTS */
#endif /* STAT_STATFS2_BSIZE */
	

#ifdef STAT_STATFS2_FSIZE	/* 4.4BSD */
#define CONVERT_BLOCKS(B) adjust_blocks ((SMB_BIG_UINT)(B), (SMB_BIG_UINT)fsd.f_fsize, (SMB_BIG_UINT)512)
	
	struct statfs fsd;
	
	if (statfs (path, &fsd) < 0)
		return -1;
#endif /* STAT_STATFS2_FSIZE */
	
#ifdef STAT_STATFS4		/* SVR3, Dynix, Irix, AIX */
# if _AIX || defined(_CRAY)
#  define CONVERT_BLOCKS(B) adjust_blocks ((SMB_BIG_UINT)(B), (SMB_BIG_UINT)fsd.f_bsize, (SMB_BIG_UINT)512)
#  ifdef _CRAY
#   define f_bavail f_bfree
#  endif
# else
#  define CONVERT_BLOCKS(B) ((SMB_BIG_UINT)B)
#  ifndef _SEQUENT_		/* _SEQUENT_ is DYNIX/ptx */
#   ifndef DOLPHIN		/* DOLPHIN 3.8.alfa/7.18 has f_bavail */
#    define f_bavail f_bfree
#   endif
#  endif
# endif
	
	struct statfs fsd;

	if (statfs (path, &fsd, sizeof fsd, 0) < 0)
		return -1;
	/* Empirically, the block counts on most SVR3 and SVR3-derived
	   systems seem to always be in terms of 512-byte blocks,
	   no matter what value f_bsize has.  */

#endif /* STAT_STATFS4 */

#if defined(STAT_STATVFS) || defined(STAT_STATVFS64)		/* SVR4 */
# define CONVERT_BLOCKS(B) \
	adjust_blocks ((SMB_BIG_UINT)(B), fsd.f_frsize ? (SMB_BIG_UINT)fsd.f_frsize : (SMB_BIG_UINT)fsd.f_bsize, (SMB_BIG_UINT)512)

#ifdef STAT_STATVFS64
	struct statvfs64 fsd;
	if (statvfs64(path, &fsd) < 0) return -1;
#else
	struct statvfs fsd;
	if (statvfs(path, &fsd) < 0) return -1;
#endif

	/* f_frsize isn't guaranteed to be supported.  */

#endif /* STAT_STATVFS */

#ifndef CONVERT_BLOCKS
	/* we don't have any dfree code! */
	return -1;
#else
#if !defined(STAT_STATFS2_FS_DATA)
	/* !Ultrix */
	(*dsize) = CONVERT_BLOCKS (fsd.f_blocks);
	(*dfree) = CONVERT_BLOCKS (fsd.f_bavail);
#endif /* not STAT_STATFS2_FS_DATA */
#endif

	return 0;
}
