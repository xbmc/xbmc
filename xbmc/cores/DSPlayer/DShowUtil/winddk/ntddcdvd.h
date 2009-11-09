/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    ntddcdvd.h

Abstract:

    This module contains structures and definitions
    associated with DVD ioctls.

    This module is used in conjunction with ntddcdrm.h which contains the
    cdrom specific ioctls which will work on CDVD drives

Author:

    Peter Wieland

Revision History:

--*/

// begin_winioctl

#ifndef _NTDDCDVD_
#define _NTDDCDVD_

#if _MSC_VER > 1000
#pragma once
#endif

#include <pshpack1.h>


//
// NtDeviceIoControlFile IoControlCode values for this device.
//
// Warning:  Remember that the low two bits of the code specify how the
//           buffers are passed to the driver!
//

#define IOCTL_DVD_BASE                 FILE_DEVICE_DVD

//
// CDVD Device Control Functions
//
// Warning: Ioctls from 200 through 300 are used for the old common class
// driver ioctls and should not be used for device specific functionality
//

//
// Copyright ioctls
//

#define IOCTL_DVD_START_SESSION     CTL_CODE(IOCTL_DVD_BASE, 0x0400, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_DVD_READ_KEY          CTL_CODE(IOCTL_DVD_BASE, 0x0401, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_DVD_SEND_KEY          CTL_CODE(IOCTL_DVD_BASE, 0x0402, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_DVD_END_SESSION       CTL_CODE(IOCTL_DVD_BASE, 0x0403, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_DVD_SET_READ_AHEAD    CTL_CODE(IOCTL_DVD_BASE, 0x0404, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_DVD_GET_REGION        CTL_CODE(IOCTL_DVD_BASE, 0x0405, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_DVD_SEND_KEY2         CTL_CODE(IOCTL_DVD_BASE, 0x0406, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)

//
// DVD Structure queries
//

#define IOCTL_DVD_READ_STRUCTURE    CTL_CODE(IOCTL_DVD_BASE, 0x0450, METHOD_BUFFERED, FILE_READ_ACCESS)

//
// The following file contains the IOCTL_STORAGE class ioctl definitions
//

#define IOCTL_STORAGE_SET_READ_AHEAD        CTL_CODE(IOCTL_STORAGE_BASE, 0x0100, METHOD_BUFFERED, FILE_READ_ACCESS)

// end_winioctl

//#include <ntddstor.h>
#include "ntddstor.h"

// begin_winioctl


#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    DvdChallengeKey = 0x01,
    DvdBusKey1,
    DvdBusKey2,
    DvdTitleKey,
    DvdAsf,
    DvdSetRpcKey = 0x6,
    DvdGetRpcKey = 0x8,
    DvdDiskKey = 0x80,
    DvdInvalidateAGID = 0x3f
} DVD_KEY_TYPE;

typedef ULONG DVD_SESSION_ID, *PDVD_SESSION_ID;

typedef struct _DVD_COPY_PROTECT_KEY {
    ULONG KeyLength;
    DVD_SESSION_ID SessionId;
    DVD_KEY_TYPE KeyType;
    ULONG KeyFlags;
    union {
        HANDLE FileHandle;
        LARGE_INTEGER TitleOffset;
    } Parameters;
    UCHAR KeyData[0];
} DVD_COPY_PROTECT_KEY, *PDVD_COPY_PROTECT_KEY;

//
// Predefined (Mt. Fuji) key sizes
// Add sizeof(DVD_COPY_PROTECT_KEY) to get allocation size for
// the full key structure
//

#define DVD_CHALLENGE_KEY_LENGTH    (12 + sizeof(DVD_COPY_PROTECT_KEY))
#define DVD_BUS_KEY_LENGTH          (8 + sizeof(DVD_COPY_PROTECT_KEY))
#define DVD_TITLE_KEY_LENGTH        (8 + sizeof(DVD_COPY_PROTECT_KEY))
#define DVD_DISK_KEY_LENGTH         (2048 + sizeof(DVD_COPY_PROTECT_KEY))
#define DVD_RPC_KEY_LENGTH          (sizeof(DVD_RPC_KEY) + sizeof(DVD_COPY_PROTECT_KEY))
#define DVD_SET_RPC_KEY_LENGTH      (sizeof(DVD_SET_RPC_KEY) + sizeof(DVD_COPY_PROTECT_KEY))
#define DVD_ASF_LENGTH              (sizeof(DVD_ASF) + sizeof(DVD_COPY_PROTECT_KEY))

//
// Used with IOCTL_DVD_END_SESSION to end all DVD sessions at once
//

#define DVD_END_ALL_SESSIONS ((DVD_SESSION_ID) 0xffffffff)

//
// CGMS Copy Protection Flags
//

#define DVD_CGMS_RESERVED_MASK      0x00000078

#define DVD_CGMS_COPY_PROTECT_MASK  0x00000018
#define DVD_CGMS_COPY_PERMITTED     0x00000000
#define DVD_CGMS_COPY_ONCE          0x00000010
#define DVD_CGMS_NO_COPY            0x00000018

#define DVD_COPYRIGHT_MASK          0x00000040
#define DVD_NOT_COPYRIGHTED         0x00000000
#define DVD_COPYRIGHTED             0x00000040

#define DVD_SECTOR_PROTECT_MASK     0x00000020
#define DVD_SECTOR_NOT_PROTECTED    0x00000000
#define DVD_SECTOR_PROTECTED        0x00000020

/*++

IOCTL_STORAGE_SET_READ_AHEAD

Requests that the storage device skip to TargetAddress once it has run across
TriggerAddress during the course of it's read-ahead caching operations.

Input:

    a STORAGE_SET_READ_AHEAD structure which contains:
        * the trigger address
        * the target address

Output:

    none

--*/

typedef struct _STORAGE_SET_READ_AHEAD {
    LARGE_INTEGER TriggerAddress;
    LARGE_INTEGER TargetAddress;
} STORAGE_SET_READ_AHEAD, *PSTORAGE_SET_READ_AHEAD;

/*++

IOCTL_DVD_READ_STRUCTURE

Issues a READ_DVD_STRUCTURE command to the drive.

Input:

    a DVD_READ_STRUCTURE describing what information is requested

Output:

    a DVD Layer Descriptor as defined below

--*/

typedef enum DVD_STRUCTURE_FORMAT {
    DvdPhysicalDescriptor,
    DvdCopyrightDescriptor,
    DvdDiskKeyDescriptor,
    DvdBCADescriptor,
    DvdManufacturerDescriptor,
    DvdMaxDescriptor
} DVD_STRUCTURE_FORMAT, *PDVD_STRUCTURE_FORMAT;

typedef struct DVD_READ_STRUCTURE {
    LARGE_INTEGER BlockByteOffset;
    DVD_STRUCTURE_FORMAT Format;
    DVD_SESSION_ID SessionId;
    UCHAR LayerNumber;
} DVD_READ_STRUCTURE, *PDVD_READ_STRUCTURE;

typedef struct _DVD_DESCRIPTOR_HEADER {
    USHORT Length;
    UCHAR Reserved[2];
    UCHAR Data[0];
} DVD_DESCRIPTOR_HEADER, *PDVD_DESCRIPTOR_HEADER;

typedef struct _DVD_LAYER_DESCRIPTOR {
    UCHAR BookVersion : 4;
    UCHAR BookType : 4;
    UCHAR MinimumRate : 4;
    UCHAR DiskSize : 4;
    UCHAR LayerType : 4;
    UCHAR TrackPath : 1;
    UCHAR NumberOfLayers : 2;
    UCHAR Reserved1 : 1;
    UCHAR TrackDensity : 4;
    UCHAR LinearDensity : 4;
    ULONG StartingDataSector;
    ULONG EndDataSector;
    ULONG EndLayerZeroSector;
    UCHAR Reserved5 : 7;
    UCHAR BCAFlag : 1;
    UCHAR Reserved6;
} DVD_LAYER_DESCRIPTOR, *PDVD_LAYER_DESCRIPTOR;

typedef struct _DVD_COPYRIGHT_DESCRIPTOR {
    UCHAR CopyrightProtectionType;
    UCHAR RegionManagementInformation;
    USHORT Reserved;
} DVD_COPYRIGHT_DESCRIPTOR, *PDVD_COPYRIGHT_DESCRIPTOR;

typedef struct _DVD_DISK_KEY_DESCRIPTOR {
    UCHAR DiskKeyData[2048];
} DVD_DISK_KEY_DESCRIPTOR, *PDVD_DISK_KEY_DESCRIPTOR;

typedef struct _DVD_BCA_DESCRIPTOR {
    UCHAR BCAInformation[0];
} DVD_BCA_DESCRIPTOR, *PDVD_BCA_DESCRIPTOR;

typedef struct _DVD_MANUFACTURER_DESCRIPTOR {
    UCHAR ManufacturingInformation[2048];
} DVD_MANUFACTURER_DESCRIPTOR, *PDVD_MANUFACTURER_DESCRIPTOR;

typedef struct _DVD_RPC_KEY {
    UCHAR UserResetsAvailable:3;
    UCHAR ManufacturerResetsAvailable:3;
    UCHAR TypeCode:2;
    UCHAR RegionMask;
    UCHAR RpcScheme;
    UCHAR Reserved2[1];
} DVD_RPC_KEY, * PDVD_RPC_KEY;

typedef struct _DVD_SET_RPC_KEY {
    UCHAR PreferredDriveRegionCode;
    UCHAR Reserved[3];
} DVD_SET_RPC_KEY, * PDVD_SET_RPC_KEY;

typedef struct _DVD_ASF {
    UCHAR Reserved0[3];
    UCHAR SuccessFlag:1;
    UCHAR Reserved1:7;
} DVD_ASF, * PDVD_ASF;

#if 0
typedef struct _DVD_REGION {
     UCHAR CopySystem;
     UCHAR RegionData;                      // current media region (not playable when set)
     UCHAR SystemRegion;                    // current drive region (playable when set)
     UCHAR ResetCount;                      // number of resets available
} DVD_REGION, *PDVD_REGION;
#endif

#ifdef __cplusplus
}
#endif


#include <poppack.h>


#endif  // _NTDDCDVD_

// end_winioctl


