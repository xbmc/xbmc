/* 
   Unix SMB/CIFS implementation.
   SMB transaction2 handling
   Copyright (C) Jeremy Allison 1994-2002.

   Extensively modified by Andrew Tridgell, 1995

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

#ifndef _TRANS2_H_
#define _TRANS2_H_

/* Define the structures needed for the trans2 calls. */

/*******************************************************
 For DosFindFirst/DosFindNext - level 1

MAXFILENAMELEN = 255;
FDATE == uint16
FTIME == uint16
ULONG == uint32
USHORT == uint16

typedef struct _FILEFINDBUF {
Byte offset   Type     name                description
-------------+-------+-------------------+--------------
0             FDATE    fdateCreation;
2             FTIME    ftimeCreation;
4             FDATE    fdateLastAccess;
6             FTIME    ftimeLastAccess;
8             FDATE    fdateLastWrite;
10            FTIME    ftimeLastWrite;
12            ULONG    cbFile               file length in bytes
16            ULONG    cbFileAlloc          size of file allocation unit
20            USHORT   attrFile
22            UCHAR    cchName              length of name to follow (not including zero)
23            UCHAR    achName[MAXFILENAMELEN]; Null terminated name
} FILEFINDBUF;
*********************************************************/

#define l1_fdateCreation 0
#define l1_fdateLastAccess 4
#define l1_fdateLastWrite 8
#define l1_cbFile 12
#define l1_cbFileAlloc 16
#define l1_attrFile 20
#define l1_cchName 22
#define l1_achName 23

/**********************************************************
For DosFindFirst/DosFindNext - level 2

typedef struct _FILEFINDBUF2 {
Byte offset   Type     name                description
-------------+-------+-------------------+--------------
0             FDATE    fdateCreation;
2             FTIME    ftimeCreation;
4             FDATE    fdateLastAccess;
6             FTIME    ftimeLastAccess;
8             FDATE    fdateLastWrite;
10            FTIME    ftimeLastWrite;
12            ULONG    cbFile               file length in bytes
16            ULONG    cbFileAlloc          size of file allocation unit
20            USHORT   attrFile
22            ULONG    cbList               Extended attribute list (always 0)
26            UCHAR    cchName              length of name to follow (not including zero)
27            UCHAR    achName[MAXFILENAMELEN]; Null terminated name
} FILEFINDBUF2;
*************************************************************/

#define l2_fdateCreation 0
#define l2_fdateLastAccess 4
#define l2_fdateLastWrite 8
#define l2_cbFile 12
#define l2_cbFileAlloc 16
#define l2_attrFile 20
#define l2_cbList 22
#define l2_cchName 26
#define l2_achName 27


/**********************************************************
For DosFindFirst/DosFindNext - level 260

typedef struct _FILEFINDBUF260 {
Byte offset   Type     name                description
-------------+-------+-------------------+--------------
0              ULONG  NextEntryOffset;
4              ULONG  FileIndex;
8              LARGE_INTEGER CreationTime;
16             LARGE_INTEGER LastAccessTime;
24             LARGE_INTEGER LastWriteTime;
32             LARGE_INTEGER ChangeTime;
40             LARGE_INTEGER EndOfFile;
48             LARGE_INTEGER AllocationSize;
56             ULONG FileAttributes;
60             ULONG FileNameLength;
64             ULONG EaSize;
68             CHAR ShortNameLength;
70             UNICODE ShortName[12];
94             UNICODE FileName[];
*************************************************************/

#define l260_achName 94


/**********************************************************
For DosQueryPathInfo/DosQueryFileInfo/DosSetPathInfo/
DosSetFileInfo - level 1

typedef struct _FILESTATUS {
Byte offset   Type     name                description
-------------+-------+-------------------+--------------
0             FDATE    fdateCreation;
2             FTIME    ftimeCreation;
4             FDATE    fdateLastAccess;
6             FTIME    ftimeLastAccess;
8             FDATE    fdateLastWrite;
10            FTIME    ftimeLastWrite;
12            ULONG    cbFile               file length in bytes
16            ULONG    cbFileAlloc          size of file allocation unit
20            USHORT   attrFile
} FILESTATUS;
*************************************************************/

/* Use the l1_ defines from DosFindFirst */

/**********************************************************
For DosQueryPathInfo/DosQueryFileInfo/DosSetPathInfo/
DosSetFileInfo - level 2

typedef struct _FILESTATUS2 {
Byte offset   Type     name                description
-------------+-------+-------------------+--------------
0             FDATE    fdateCreation;
2             FTIME    ftimeCreation;
4             FDATE    fdateLastAccess;
6             FTIME    ftimeLastAccess;
8             FDATE    fdateLastWrite;
10            FTIME    ftimeLastWrite;
12            ULONG    cbFile               file length in bytes
16            ULONG    cbFileAlloc          size of file allocation unit
20            USHORT   attrFile
22            ULONG    cbList               Length of EA's (0)
} FILESTATUS2;
*************************************************************/

/* Use the l2_ #defines from DosFindFirst */

/**********************************************************
For DosQFSInfo/DosSetFSInfo - level 1

typedef struct _FSALLOCATE {
Byte offset   Type     name                description
-------------+-------+-------------------+--------------
0             ULONG    idFileSystem       id of file system
4             ULONG    cSectorUnit        number of sectors per allocation unit
8             ULONG    cUnit              number of allocation units
12            ULONG    cUnitAvail         Available allocation units
16            USHORT   cbSector           bytes per sector
} FSALLOCATE;
*************************************************************/

#define l1_idFileSystem 0
#define l1_cSectorUnit 4
#define l1_cUnit 8
#define l1_cUnitAvail 12
#define l1_cbSector 16

/**********************************************************
For DosQFSInfo/DosSetFSInfo - level 2

typedef struct _FSINFO {
Byte offset   Type     name                description
-------------+-------+-------------------+--------------
0             FDATE   vol_fdateCreation
2             FTIME   vol_ftimeCreation
4             UCHAR   vol_cch             length of volume name (excluding NULL)
5             UCHAR   vol_szVolLabel[12]  volume name
} FSINFO;
*************************************************************/

#define SMB_INFO_STANDARD               1  /* FILESTATUS3 struct */
#define SMB_INFO_SET_EA                 2  /* EAOP2 struct, only valid on set not query */
#define SMB_INFO_QUERY_EA_SIZE          2  /* FILESTATUS4 struct, only valid on query not set */
#define SMB_INFO_QUERY_EAS_FROM_LIST    3  /* only valid on query not set */
#define SMB_INFO_QUERY_ALL_EAS          4  /* only valid on query not set */
#define SMB_INFO_IS_NAME_VALID          6
#define SMB_INFO_STANDARD_LONG          11  /* similar to level 1, ie struct FileStatus3 */
#define SMB_QUERY_EA_SIZE_LONG          12  /* similar to level 2, ie struct FileStatus4 */
#define SMB_QUERY_FS_LABEL_INFO         0x101
#define SMB_QUERY_FS_VOLUME_INFO        0x102
#define SMB_QUERY_FS_SIZE_INFO          0x103
#define SMB_QUERY_FS_DEVICE_INFO        0x104
#define SMB_QUERY_FS_ATTRIBUTE_INFO     0x105
#if 0
#define SMB_QUERY_FS_QUOTA_INFO		
#endif

#define l2_vol_fdateCreation 0
#define l2_vol_cch 4
#define l2_vol_szVolLabel 5


#define SMB_QUERY_FILE_BASIC_INFO	0x101
#define SMB_QUERY_FILE_STANDARD_INFO	0x102
#define SMB_QUERY_FILE_EA_INFO		0x103
#define SMB_QUERY_FILE_NAME_INFO	0x104
#define SMB_QUERY_FILE_ALLOCATION_INFO	0x105
#define SMB_QUERY_FILE_END_OF_FILEINFO	0x106
#define SMB_QUERY_FILE_ALL_INFO		0x107
#define SMB_QUERY_FILE_ALT_NAME_INFO	0x108
#define SMB_QUERY_FILE_STREAM_INFO	0x109
#define SMB_QUERY_COMPRESSION_INFO	0x10b

#define SMB_FIND_INFO_STANDARD			1
#define SMB_FIND_EA_SIZE			2
#define SMB_FIND_EA_LIST			3
#define SMB_FIND_FILE_DIRECTORY_INFO		0x101
#define SMB_FIND_FILE_FULL_DIRECTORY_INFO	0x102
#define SMB_FIND_FILE_NAMES_INFO		0x103
#define SMB_FIND_FILE_BOTH_DIRECTORY_INFO	0x104
#define SMB_FIND_ID_FULL_DIRECTORY_INFO		0x105
#define SMB_FIND_ID_BOTH_DIRECTORY_INFO		0x106

#define SMB_SET_FILE_BASIC_INFO		0x101
#define SMB_SET_FILE_DISPOSITION_INFO	0x102
#define SMB_SET_FILE_ALLOCATION_INFO	0x103
#define SMB_SET_FILE_END_OF_FILE_INFO	0x104

/* Query FS info. */
#define SMB_INFO_ALLOCATION		1
#define SMB_INFO_VOLUME			2

/*
 * Thursby MAC extensions....
 */

/*
 * MAC CIFS Extensions have the range 0x300 - 0x2FF reserved.
 * Supposedly Microsoft have agreed to this.
 */

#define MIN_MAC_INFO_LEVEL 0x300
#define MAX_MAC_INFO_LEVEL 0x3FF

#define SMB_MAC_QUERY_FS_INFO           0x301

#define DIRLEN_GUESS (45+MAX(l1_achName,l2_achName))

/*
 * DeviceType and Characteristics returned in a
 * SMB_QUERY_FS_DEVICE_INFO call.
 */

#define DEVICETYPE_CD_ROM		0x2
#define DEVICETYPE_CD_ROM_FILE_SYSTEM	0x3
#define DEVICETYPE_DISK			0x7
#define DEVICETYPE_DISK_FILE_SYSTEM	0x8
#define DEVICETYPE_FILE_SYSTEM		0x9

/* Characteristics. */
#define TYPE_REMOVABLE_MEDIA		0x1
#define TYPE_READ_ONLY_DEVICE		0x2
#define TYPE_FLOPPY			0x4
#define TYPE_WORM			0x8
#define TYPE_REMOTE			0x10
#define TYPE_MOUNTED			0x20
#define TYPE_VIRTUAL			0x40

/* NT passthrough levels... */

#define SMB_FILE_DIRECTORY_INFORMATION			1001
#define SMB_FILE_FULL_DIRECTORY_INFORMATION		1002
#define SMB_FILE_BOTH_DIRECTORY_INFORMATION		1003
#define SMB_FILE_BASIC_INFORMATION			1004
#define SMB_FILE_STANDARD_INFORMATION			1005
#define SMB_FILE_INTERNAL_INFORMATION			1006
#define SMB_FILE_EA_INFORMATION				1007
#define SMB_FILE_ACCESS_INFORMATION			1008
#define SMB_FILE_NAME_INFORMATION			1009
#define SMB_FILE_RENAME_INFORMATION			1010
#define SMB_FILE_LINK_INFORMATION			1011
#define SMB_FILE_NAMES_INFORMATION			1012
#define SMB_FILE_DISPOSITION_INFORMATION		1013
#define SMB_FILE_POSITION_INFORMATION			1014
#define SMB_FILE_FULL_EA_INFORMATION			1015
#define SMB_FILE_MODE_INFORMATION			1016
#define SMB_FILE_ALIGNMENT_INFORMATION			1017
#define SMB_FILE_ALL_INFORMATION			1018
#define SMB_FILE_ALLOCATION_INFORMATION			1019
#define SMB_FILE_END_OF_FILE_INFORMATION		1020
#define SMB_FILE_ALTERNATE_NAME_INFORMATION		1021
#define SMB_FILE_STREAM_INFORMATION			1022
#define SMB_FILE_PIPE_INFORMATION			1023
#define SMB_FILE_PIPE_LOCAL_INFORMATION			1024
#define SMB_FILE_PIPE_REMOTE_INFORMATION		1025
#define SMB_FILE_MAILSLOT_QUERY_INFORMATION		1026
#define SMB_FILE_MAILSLOT_SET_INFORMATION		1027
#define SMB_FILE_COMPRESSION_INFORMATION		1028
#define SMB_FILE_OBJECTID_INFORMATION			1029
#define SMB_FILE_COMPLETION_INFORMATION			1030
#define SMB_FILE_MOVE_CLUSTER_INFORMATION		1031
#define SMB_FILE_QUOTA_INFORMATION			1032
#define SMB_FILE_REPARSEPOINT_INFORMATION		1033
#define SMB_FILE_NETWORK_OPEN_INFORMATION		1034
#define SMB_FILE_ATTRIBUTE_TAG_INFORMATION		1035
#define SMB_FILE_TRACKING_INFORMATION			1036
#define SMB_FILE_MAXIMUM_INFORMATION			1037

/* NT passthough levels for qfsinfo. */

#define SMB_FS_VOLUME_INFORMATION			1001
#define SMB_FS_LABEL_INFORMATION			1002
#define SMB_FS_SIZE_INFORMATION				1003
#define SMB_FS_DEVICE_INFORMATION			1004
#define SMB_FS_ATTRIBUTE_INFORMATION			1005
#define SMB_FS_QUOTA_INFORMATION			1006
#define SMB_FS_FULL_SIZE_INFORMATION			1007
#define SMB_FS_OBJECTID_INFORMATION			1008

/* flags on trans2 findfirst/findnext that control search */
#define FLAG_TRANS2_FIND_CLOSE          0x1
#define FLAG_TRANS2_FIND_CLOSE_IF_END   0x2
#define FLAG_TRANS2_FIND_REQUIRE_RESUME 0x4
#define FLAG_TRANS2_FIND_CONTINUE       0x8
#define FLAG_TRANS2_FIND_BACKUP_INTENT  0x10

/* UNIX CIFS Extensions - created by HP */
/*
 * UNIX CIFS Extensions have the range 0x200 - 0x2FF reserved.
 * Supposedly Microsoft have agreed to this.
 */

#define MIN_UNIX_INFO_LEVEL 0x200
#define MAX_UNIX_INFO_LEVEL 0x2FF

#define INFO_LEVEL_IS_UNIX(level) (((level) >= MIN_UNIX_INFO_LEVEL) && ((level) <= MAX_UNIX_INFO_LEVEL))

#define SMB_QUERY_FILE_UNIX_BASIC      0x200   /* UNIX File Info*/
#define SMB_SET_FILE_UNIX_BASIC        0x200

#define SMB_MODE_NO_CHANGE                 0xFFFFFFFF     /* file mode value which */
                                              /* means "don't change it" */
#define SMB_UID_NO_CHANGE                  0xFFFFFFFF
#define SMB_GID_NO_CHANGE                  0xFFFFFFFF

#define SMB_SIZE_NO_CHANGE_LO              0xFFFFFFFF
#define SMB_SIZE_NO_CHANGE_HI              0xFFFFFFFF
 
#define SMB_TIME_NO_CHANGE_LO              0xFFFFFFFF
#define SMB_TIME_NO_CHANGE_HI              0xFFFFFFFF

/*
Offset Size         Name
0      LARGE_INTEGER EndOfFile                File size
8      LARGE_INTEGER Blocks                   Number of bytes used on disk (st_blocks).
16     LARGE_INTEGER CreationTime             Creation time
24     LARGE_INTEGER LastAccessTime           Last access time
32     LARGE_INTEGER LastModificationTime     Last modification time
40     LARGE_INTEGER Uid                      Numeric user id for the owner
48     LARGE_INTEGER Gid                      Numeric group id of owner
56     ULONG Type                             Enumeration specifying the pathname type:
                                              0 -- File
                                              1 -- Directory
                                              2 -- Symbolic link
                                              3 -- Character device
                                              4 -- Block device
                                              5 -- FIFO (named pipe)
                                              6 -- Unix domain socket

60     LARGE_INTEGER devmajor                 Major device number if type is device
68     LARGE_INTEGER devminor                 Minor device number if type is device
76     LARGE_INTEGER uniqueid                 This is a server-assigned unique id for the file. The client
                                              will typically map this onto an inode number. The scope of
                                              uniqueness is the share.
84     LARGE_INTEGER permissions              Standard UNIX file permissions  - see below.
92     LARGE_INTEGER nlinks                   The number of directory entries that map to this entry
                                              (number of hard links)

100 - end.
*/

/* UNIX filetype mappings. */

#define UNIX_TYPE_FILE 0
#define UNIX_TYPE_DIR 1
#define UNIX_TYPE_SYMLINK 2
#define UNIX_TYPE_CHARDEV 3
#define UNIX_TYPE_BLKDEV 4
#define UNIX_TYPE_FIFO 5
#define UNIX_TYPE_SOCKET 6
#define UNIX_TYPE_UNKNOWN 0xFFFFFFFF

/*
 * Oh this is fun. "Standard UNIX permissions" has no
 * meaning in POSIX. We need to define the mapping onto
 * and off the wire as this was not done in the original HP
 * spec. JRA.
 */

#define UNIX_X_OTH			0000001
#define UNIX_W_OTH			0000002
#define UNIX_R_OTH			0000004
#define UNIX_X_GRP			0000010
#define UNIX_W_GRP                      0000020
#define UNIX_R_GRP                      0000040
#define UNIX_X_USR                      0000100
#define UNIX_W_USR                      0000200
#define UNIX_R_USR                      0000400
#define UNIX_STICKY                     0001000
#define UNIX_SET_GID                    0002000
#define UNIX_SET_UID                    0004000

/* Masks for the above */
#define UNIX_OTH_MASK                   0000007
#define UNIX_GRP_MASK                   0000070
#define UNIX_USR_MASK                   0000700
#define UNIX_PERM_MASK                  0000777
#define UNIX_EXTRA_MASK                 0007000
#define UNIX_ALL_MASK                   0007777

#define SMB_QUERY_FILE_UNIX_LINK       0x201
#define SMB_SET_FILE_UNIX_LINK         0x201
#define SMB_SET_FILE_UNIX_HLINK        0x203
/* SMB_QUERY_POSIX_ACL 0x204 see below */
#define SMB_QUERY_XATTR                0x205 /* need for non-user XATTRs */
#define SMB_QUERY_ATTR_FLAGS           0x206 /* chflags, chattr */
#define SMB_SET_ATTR_FLAGS             0x206 
#define SMB_QUERY_POSIX_PERMISSION     0x207
/* Only valid for qfileinfo */
#define SMB_QUERY_POSIX_LOCK	       0x208
/* Only valid for setfileinfo */
#define SMB_SET_POSIX_LOCK	       0x208

/* Transact 2 Find First levels */
#define SMB_FIND_FILE_UNIX             0x202

/*
 Info level for TRANS2_QFSINFO - returns version of CIFS UNIX extensions, plus
 64-bits worth of capability fun :-).
 Use the same info level for TRANS2_SETFSINFO
*/

#define SMB_QUERY_CIFS_UNIX_INFO      0x200
#define SMB_SET_CIFS_UNIX_INFO        0x200

/* Returns or sets the following.

  UINT16             major version number
  UINT16             minor version number
  LARGE_INTEGER      capability bitfield

*/

#define CIFS_UNIX_MAJOR_VERSION 1
#define CIFS_UNIX_MINOR_VERSION 0

#define CIFS_UNIX_FCNTL_LOCKS_CAP           0x1
#define CIFS_UNIX_POSIX_ACLS_CAP            0x2
#define CIFS_UNIX_XATTTR_CAP	            0x4 /* for support of other xattr
						namespaces such as system,
						security and trusted */
#define CIFS_UNIX_EXTATTR_CAP		    0x8 /* for support of chattr
						(chflags) and lsattr */
#define CIFS_UNIX_POSIX_PATHNAMES_CAP	   0x10 /* Use POSIX pathnames on the wire. */


#define SMB_QUERY_POSIX_FS_INFO     0x201

/* Returns FILE_SYSTEM_POSIX_INFO struct as follows
      (NB   For undefined values return -1 in that field) 
   le32 OptimalTransferSize;    bsize on some os, iosize on other os, This 
				is a hint to the client about best size. Server
				can return -1 if no preference, ie if SMB 
				negotiated size is adequate for optimal
				read/write performance
   le32 BlockSize; (often 512 bytes) NB: BlockSize * TotalBlocks = disk space
   le64 TotalBlocks;  redundant with other infolevels but easy to ret here
   le64 BlocksAvail;  although redundant, easy to return
   le64 UserBlocksAvail;      bavail 
   le64 TotalFileNodes;
   le64 FreeFileNodes;
   le64 FileSysIdentifier;    fsid 
   (NB statfs field Namelen comes from FILE_SYSTEM_ATTRIBUTE_INFO call) 
   (NB statfs field flags can come from FILE_SYSTEM_DEVICE_INFO call)  
*/

/* ... more as we think of them :-). */

/* SMB POSIX ACL definitions. */
/* Wire format is (all little endian) :

[2 bytes]              -     Version number.
[2 bytes]              -     Number of ACE entries to follow.
[2 bytes]              -     Number of default ACE entries to follow.
-------------------------------------
^
|
ACE entries
|
v
-------------------------------------
^
|
Default ACE entries
|
v
-------------------------------------

Where an ACE entry looks like :

[1 byte]           - Entry type.

Entry types are :

ACL_USER_OBJ            0x01
ACL_USER                0x02
ACL_GROUP_OBJ           0x04
ACL_GROUP               0x08
ACL_MASK                0x10
ACL_OTHER               0x20

[1 byte]          - permissions (perm_t)

perm_t types are :

ACL_READ                0x04
ACL_WRITE               0x02
ACL_EXECUTE             0x01

[8 bytes]         - uid/gid to apply this permission to.

In the same format as the uid/gid fields in the other
UNIX extensions definitions. Use 0xFFFFFFFFFFFFFFFF for
the MASK and OTHER entry types.

If the Number of ACE entries for either file or default ACE's
is set to 0xFFFF this means ignore this kind of ACE (and the
number of entries sent will be zero.

*/

/* The query/set info levels for POSIX ACLs. */
#define SMB_QUERY_POSIX_ACL  0x204
#define SMB_SET_POSIX_ACL  0x204

/* Current on the wire ACL version. */
#define SMB_POSIX_ACL_VERSION 1

/* ACE entry type. */
#define SMB_POSIX_ACL_USER_OBJ            0x01
#define SMB_POSIX_ACL_USER                0x02
#define SMB_POSIX_ACL_GROUP_OBJ           0x04
#define SMB_POSIX_ACL_GROUP               0x08
#define SMB_POSIX_ACL_MASK                0x10
#define SMB_POSIX_ACL_OTHER               0x20

/* perm_t types. */
#define SMB_POSIX_ACL_READ                0x04
#define SMB_POSIX_ACL_WRITE               0x02
#define SMB_POSIX_ACL_EXECUTE             0x01

#define SMB_POSIX_ACL_HEADER_SIZE         6
#define SMB_POSIX_ACL_ENTRY_SIZE         10

#define SMB_POSIX_IGNORE_ACE_ENTRIES	0xFFFF

/* Definition of SMB_SET_POSIX_LOCK */
/*
  [2 bytes] lock_type - 0 = Read, 1 = Write, 2 = Unlock
  [2 bytes] lock_flags - 1 = Wait (only valid for setlock)
  [4 bytes] pid = locking context.
  [8 bytes] start = unsigned 64 bits.
  [8 bytes] length = unsigned 64 bits.
*/

#define POSIX_LOCK_TYPE_OFFSET 0
#define POSIX_LOCK_FLAGS_OFFSET 2
#define POSIX_LOCK_PID_OFFSET 4
#define POSIX_LOCK_START_OFFSET 8
#define POSIX_LOCK_LEN_OFFSET 16
#define POSIX_LOCK_DATA_SIZE 24

#define POSIX_LOCK_FLAG_NOWAIT 0
#define POSIX_LOCK_FLAG_WAIT 1

#define POSIX_LOCK_TYPE_READ 0
#define POSIX_LOCK_TYPE_WRITE 1
#define POSIX_LOCK_TYPE_UNLOCK 2

#endif
