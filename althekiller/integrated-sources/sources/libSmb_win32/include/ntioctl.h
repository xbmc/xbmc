/* 
   Unix SMB/CIFS implementation.
   NT ioctl code constants
   Copyright (C) Andrew Tridgell              2002
   
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
  I'm guessing we will need to support a bunch of these eventually. For now
  we only need the sparse flag
*/

#ifndef _NTIOCTL_H
#define _NTIOCTL_H

/* IOCTL information */
/* List of ioctl function codes that look to be of interest to remote clients like this. */
/* Need to do some experimentation to make sure they all work remotely.                  */
/* Some of the following such as the encryption/compression ones would be                */
/* invoked from tools via a specialized hook into the VFS rather than via the            */
/* standard vfs entry points */
#define FSCTL_REQUEST_OPLOCK_LEVEL_1 0x00090000
#define FSCTL_REQUEST_OPLOCK_LEVEL_2 0x00090004
#define FSCTL_REQUEST_BATCH_OPLOCK   0x00090008
#define FSCTL_LOCK_VOLUME            0x00090018
#define FSCTL_UNLOCK_VOLUME          0x0009001C
#define FSCTL_GET_COMPRESSION        0x0009003C
#define FSCTL_SET_COMPRESSION        0x0009C040
#define FSCTL_REQUEST_FILTER_OPLOCK  0x0009008C
#define FSCTL_FIND_FILES_BY_SID	     0x0009008F
#define FSCTL_FILESYS_GET_STATISTICS 0x00090090
#define FSCTL_SET_OBJECT_ID          0x00090098
#define FSCTL_GET_OBJECT_ID          0x0009009C
#define FSCTL_SET_REPARSE_POINT      0x000900A4
#define FSCTL_GET_REPARSE_POINT      0x000900A8
#define FSCTL_DELETE_REPARSE_POINT   0x000900AC
#define FSCTL_0x000900C0	     0x000900C0
#define FSCTL_SET_SPARSE             0x000900C4
#define FSCTL_SET_ZERO_DATA          0x000900C8
#define FSCTL_SET_ENCRYPTION         0x000900D7
#define FSCTL_ENCRYPTION_FSCTL_IO    0x000900DB
#define FSCTL_WRITE_RAW_ENCRYPTED    0x000900DF
#define FSCTL_READ_RAW_ENCRYPTED     0x000900E3
#define FSCTL_SIS_COPYFILE           0x00090100
#define FSCTL_QUERY_ALLOCATED_RANGES 0x000940CF
#define FSCTL_SIS_LINK_FILES         0x0009C104

#define FSCTL_GET_SHADOW_COPY_DATA   0x00144064   /* KJC -- Shadow Copy information */

#if 0
#define FSCTL_SECURITY_ID_CHECK
#define FSCTL_DISMOUNT_VOLUME
#define FSCTL_GET_NTFS_FILE_RECORD
#define FSCTL_ALLOW_EXTENDED_DASD_IO
#define FSCTL_RECALL_FILE

#endif

#ifndef _WIN32
#define IO_REPARSE_TAG_MOUNT_POINT   0xA0000003
#define IO_REPARSE_TAG_HSM           0xC0000004
#define IO_REPARSE_TAG_SIS           0x80000007
#endif


/* For FSCTL_GET_SHADOW_COPY_DATA ...*/
typedef char SHADOW_COPY_LABEL[25];

typedef struct shadow_copy_data {
	TALLOC_CTX *mem_ctx;
	/* Total number of shadow volumes currently mounted */
	uint32 num_volumes;
	/* Concatenated list of labels */
	SHADOW_COPY_LABEL *labels;
} SHADOW_COPY_DATA;


#endif /* _NTIOCTL_H */
