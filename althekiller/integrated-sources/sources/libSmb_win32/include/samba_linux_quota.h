#ifndef _SAMBA_LINUX_QUOTA_H_
#define _SAMBA_LINUX_QUOTA_H_
/*
   Unix SMB/CIFS implementation.
   Copyright (C) Andrew Tridgell 1994-2002

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

/*
	This file is needed because Quota support on Linux has
	been broken since Linus kernel 2.4.x. It will only get
	better (and this file be removed) when all the distributions
	ship a glibc with a working quota.h file. This is very
	bad. JRA.

	Original file came from Christoph Hellwig <hch@infradead.org>.
	Massaged into one nasty include file (to stop us having to
	add multiple files into Samba just for Linux braindamage)
	by JRA.
*/

#undef QUOTABLOCK_SIZE

#ifndef _QUOTAIO_LINUX_V1
#define _QUOTAIO_LINUX_V1

/*
 *	Headerfile for old quotafile format
 */

#include <sys/types.h>

#define V1_DQBLK_SIZE_BITS 10
#define V1_DQBLK_SIZE (1 << V1_DQBLK_SIZE_BITS)	/* Size of one quota block in bytes in old format */

#define V1_DQOFF(__id) ((loff_t) ((__id) * sizeof(struct v1_disk_dqblk)))

/* Structure of quota on disk */
struct v1_disk_dqblk {
	u_int32_t dqb_bhardlimit;	/* absolute limit on disk blks alloc */
	u_int32_t dqb_bsoftlimit;	/* preferred limit on disk blks */
	u_int32_t dqb_curblocks;	/* current block count */
	u_int32_t dqb_ihardlimit;	/* maximum # allocated inodes */
	u_int32_t dqb_isoftlimit;	/* preferred limit on inodes */
	u_int32_t dqb_curinodes;	/* current # allocated inodes */
	time_t dqb_btime;	/* time limit for excessive disk use */
	time_t dqb_itime;	/* time limit for excessive files */
} __attribute__ ((packed));

/* Structure used for communication with kernel */
struct v1_kern_dqblk {
	u_int32_t dqb_bhardlimit;	/* absolute limit on disk blks alloc */
	u_int32_t dqb_bsoftlimit;	/* preferred limit on disk blks */
	u_int32_t dqb_curblocks;	/* current block count */
	u_int32_t dqb_ihardlimit;	/* maximum # allocated inodes */
	u_int32_t dqb_isoftlimit;	/* preferred inode limit */
	u_int32_t dqb_curinodes;	/* current # allocated inodes */
	time_t dqb_btime;	/* time limit for excessive disk use */
	time_t dqb_itime;	/* time limit for excessive files */
};

struct v1_dqstats {
	u_int32_t lookups;
	u_int32_t drops;
	u_int32_t reads;
	u_int32_t writes;
	u_int32_t cache_hits;
	u_int32_t allocated_dquots;
	u_int32_t free_dquots;
	u_int32_t syncs;
};                                                                               

#ifndef Q_V1_GETQUOTA
#define Q_V1_GETQUOTA  0x300
#endif
#ifndef Q_V1_SETQUOTA
#define Q_V1_SETQUOTA  0x400
#endif

#endif /* _QUOTAIO_LINUX_V1 */

/*
 *
 *	Header file for disk format of new quotafile format
 *
 */

#ifndef _QUOTAIO_LINUX_V2
#define _QUOTAIO_LINUX_V2

#include <sys/types.h>

#ifndef _QUOTA_LINUX
#define _QUOTA_LINUX

#include <sys/types.h>

typedef u_int32_t qid_t;	/* Type in which we store ids in memory */
typedef u_int64_t qsize_t;	/* Type in which we store size limitations */

#define MAXQUOTAS 2
#define USRQUOTA  0		/* element used for user quotas */
#define GRPQUOTA  1		/* element used for group quotas */

/*
 * Definitions for the default names of the quotas files.
 */
#define INITQFNAMES { \
	"user",    /* USRQUOTA */ \
	"group",   /* GRPQUOTA */ \
	"undefined", \
}

/*
 * Definitions of magics and versions of current quota files
 */
#define INITQMAGICS {\
	0xd9c01f11,	/* USRQUOTA */\
	0xd9c01927	/* GRPQUOTA */\
}

/* Size of blocks in which are counted size limits in generic utility parts */
#define QUOTABLOCK_BITS 10
#define QUOTABLOCK_SIZE (1 << QUOTABLOCK_BITS)

/* Conversion routines from and to quota blocks */
#define qb2kb(x) ((x) << (QUOTABLOCK_BITS-10))
#define kb2qb(x) ((x) >> (QUOTABLOCK_BITS-10))
#define toqb(x) (((x) + QUOTABLOCK_SIZE - 1) >> QUOTABLOCK_BITS)

/*
 * Command definitions for the 'quotactl' system call.
 * The commands are broken into a main command defined below
 * and a subcommand that is used to convey the type of
 * quota that is being manipulated (see above).
 */
#define SUBCMDMASK  0x00ff
#define SUBCMDSHIFT 8
#define QCMD(cmd, type)  (((cmd) << SUBCMDSHIFT) | ((type) & SUBCMDMASK))

#define Q_6_5_QUOTAON  0x0100	/* enable quotas */
#define Q_6_5_QUOTAOFF 0x0200	/* disable quotas */
#define Q_6_5_SYNC     0x0600	/* sync disk copy of a filesystems quotas */

#define Q_SYNC     0x800001	/* sync disk copy of a filesystems quotas */
#define Q_QUOTAON  0x800002	/* turn quotas on */
#define Q_QUOTAOFF 0x800003	/* turn quotas off */
#define Q_GETFMT   0x800004	/* get quota format used on given filesystem */
#define Q_GETINFO  0x800005	/* get information about quota files */
#define Q_SETINFO  0x800006	/* set information about quota files */
#define Q_GETQUOTA 0x800007	/* get user quota structure */
#define Q_SETQUOTA 0x800008	/* set user quota structure */

/*
 * Quota structure used for communication with userspace via quotactl
 * Following flags are used to specify which fields are valid
 */
#define QIF_BLIMITS	1
#define QIF_SPACE	2
#define QIF_ILIMITS	4
#define QIF_INODES	8
#define QIF_BTIME	16
#define QIF_ITIME	32
#define QIF_LIMITS	(QIF_BLIMITS | QIF_ILIMITS)
#define QIF_USAGE	(QIF_SPACE | QIF_INODES)
#define QIF_TIMES	(QIF_BTIME | QIF_ITIME)
#define QIF_ALL		(QIF_LIMITS | QIF_USAGE | QIF_TIMES)

struct if_dqblk {
	u_int64_t dqb_bhardlimit;
	u_int64_t dqb_bsoftlimit;
	u_int64_t dqb_curspace;
	u_int64_t dqb_ihardlimit;
	u_int64_t dqb_isoftlimit;
	u_int64_t dqb_curinodes;
	u_int64_t dqb_btime;
	u_int64_t dqb_itime;
	u_int32_t dqb_valid;
};

/*
 * Structure used for setting quota information about file via quotactl
 * Following flags are used to specify which fields are valid
 */
#define IIF_BGRACE	1
#define IIF_IGRACE	2
#define IIF_FLAGS	4
#define IIF_ALL		(IIF_BGRACE | IIF_IGRACE | IIF_FLAGS)

struct if_dqinfo {
	u_int64_t dqi_bgrace;
	u_int64_t dqi_igrace;
	u_int32_t dqi_flags;
	u_int32_t dqi_valid;
};

/* Quota format identifiers */
#define QFMT_VFS_OLD 1
#define QFMT_VFS_V0  2

/* Flags supported by kernel */
#define V1_DQF_RSQUASH 1

/* Ioctl for getting quota size */
#include <sys/ioctl.h>
#ifndef FIOQSIZE
	#if defined(__alpha__) || defined(__powerpc__) || defined(__sh__) || defined(__sparc__) || defined(__sparc64__)
		#define FIOQSIZE _IOR('f', 128, loff_t)
	#elif defined(__arm__) || defined(__mc68000__) || defined(__s390__)
		#define FIOQSIZE 0x545E
        #elif defined(__i386__) || defined(__i486__) || defined(__i586__) || defined(__ia64__) || defined(__parisc__) || defined(__cris__) || defined(__hppa__)
		#define FIOQSIZE 0x5460
	#elif defined(__mips__) || defined(__mips64__)
		#define FIOQSIZE 0x6667
	#endif
#endif

long quotactl __P((int, const char *, qid_t, caddr_t));

#endif /* _QUOTA_LINUX */

#define V2_DQINFOOFF	sizeof(struct v2_disk_dqheader)	/* Offset of info header in file */
#define V2_DQBLKSIZE_BITS	10
#define V2_DQBLKSIZE	(1 << V2_DQBLKSIZE_BITS)	/* Size of block with quota structures */
#define V2_DQTREEOFF	1	/* Offset of tree in file in blOcks */
#define V2_DQTREEDEPTH	4	/* Depth of quota tree */
#define V2_DQSTRINBLK	((V2_DQBLKSIZE - sizeof(struct v2_disk_dqdbheader)) / sizeof(struct v2_disk_dqblk))	/* Number of entries in one blocks */
#define V2_GETIDINDEX(id, depth) (((id) >> ((V2_DQTREEDEPTH-(depth)-1)*8)) & 0xff)
#define V2_GETENTRIES(buf) ((struct v2_disk_dqblk *)(((char *)(buf)) + sizeof(struct v2_disk_dqdbheader)))
#define INIT_V2_VERSIONS { 0, 0}

struct v2_disk_dqheader {
	u_int32_t dqh_magic;	/* Magic number identifying file */
	u_int32_t dqh_version;	/* File version */
} __attribute__ ((packed));

/* Flags for version specific files */
#define V2_DQF_MASK  0x0000	/* Mask for all valid ondisk flags */

/* Header with type and version specific information */
struct v2_disk_dqinfo {
	u_int32_t dqi_bgrace;	/* Time before block soft limit becomes hard limit */
	u_int32_t dqi_igrace;	/* Time before inode soft limit becomes hard limit */
	u_int32_t dqi_flags;	/* Flags for quotafile (DQF_*) */
	u_int32_t dqi_blocks;	/* Number of blocks in file */
	u_int32_t dqi_free_blk;	/* Number of first free block in the list */
	u_int32_t dqi_free_entry;	/* Number of block with at least one free entry */
} __attribute__ ((packed));

/*
 *  Structure of header of block with quota structures. It is padded to 16 bytes so
 *  there will be space for exactly 18 quota-entries in a block
 */
struct v2_disk_dqdbheader {
	u_int32_t dqdh_next_free;	/* Number of next block with free entry */
	u_int32_t dqdh_prev_free;	/* Number of previous block with free entry */
	u_int16_t dqdh_entries;	/* Number of valid entries in block */
	u_int16_t dqdh_pad1;
	u_int32_t dqdh_pad2;
} __attribute__ ((packed));

/* Structure of quota for one user on disk */
struct v2_disk_dqblk {
	u_int32_t dqb_id;	/* id this quota applies to */
	u_int32_t dqb_ihardlimit;	/* absolute limit on allocated inodes */
	u_int32_t dqb_isoftlimit;	/* preferred inode limit */
	u_int32_t dqb_curinodes;	/* current # allocated inodes */
	u_int32_t dqb_bhardlimit;	/* absolute limit on disk space (in QUOTABLOCK_SIZE) */
	u_int32_t dqb_bsoftlimit;	/* preferred limit on disk space (in QUOTABLOCK_SIZE) */
	u_int64_t dqb_curspace;	/* current space occupied (in bytes) */
	u_int64_t dqb_btime;	/* time limit for excessive disk use */
	u_int64_t dqb_itime;	/* time limit for excessive inode use */
} __attribute__ ((packed));

/* Structure of quota for communication with kernel */
struct v2_kern_dqblk {
	unsigned int dqb_ihardlimit;
	unsigned int dqb_isoftlimit;
	unsigned int dqb_curinodes;
	unsigned int dqb_bhardlimit;
	unsigned int dqb_bsoftlimit;
	qsize_t dqb_curspace;
	time_t dqb_btime;
	time_t dqb_itime;
};

/* Structure of quotafile info for communication with kernel */
struct v2_kern_dqinfo {
	unsigned int dqi_bgrace;
	unsigned int dqi_igrace;
	unsigned int dqi_flags;
	unsigned int dqi_blocks;
	unsigned int dqi_free_blk;
	unsigned int dqi_free_entry;
};

/* Structure with gathered statistics from kernel */
struct v2_dqstats {
	u_int32_t lookups;
	u_int32_t drops;
	u_int32_t reads;
	u_int32_t writes;
	u_int32_t cache_hits;
	u_int32_t allocated_dquots;
	u_int32_t free_dquots;
	u_int32_t syncs;
	u_int32_t version;
};

#ifndef Q_V2_GETQUOTA
#define Q_V2_GETQUOTA  0x0D00
#endif
#ifndef Q_V2_SETQUOTA
#define Q_V2_SETQUOTA  0x0E00
#endif

#endif /* _QUOTAIO_LINUX_V2 */

#ifndef QUOTABLOCK_SIZE
#define QUOTABLOCK_SIZE        1024
#endif

#endif /* _SAMBA_LINUX_QUOTA_H_ */
