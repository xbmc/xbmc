/*
**********************************
**********************************
**      BROUGHT TO YOU BY:      **
**********************************
**********************************
**                              **
**       [TEAM ASSEMBLY]        **
**                              **
**     www.team-assembly.com    **
**                              **
******************************************************************************************************
* This is free software; you can redistribute it and/or modify
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
******************************************************************************************************


********************************************************************************************************
**      XKEXPORTS.H - XBOX Kernel Exports Header
********************************************************************************************************
**
** This Header containts various helpful XBOX Kernel exports and other #defines..
** This is pretty much a work in progress and will contrinue to grow in future..
**
********************************************************************************************************

UPDATE LOG:
--------------------------------------------------------------------------------------------------------
Date: 02/18/2003
By: UNDEAD [team-assembly]
Reason: Prepared 0.2 for Public Release
--------------------------------------------------------------------------------------------------------

*/

#pragma once
#if defined (_XBOX)
//This complete file is only supported for XBOX..
#pragma message ("Compiling for XBOX: " __FILE__)



#include <xtl.h>
#ifdef _DEBUG
 #define OUTPUT_DEBUG_STRING(s) OutputDebugStringA(s)
#else
 #define OUTPUT_DEBUG_STRING(s) (VOID)(s)
#endif

#define XB_SUCCESS(Status) ((LONG)(Status) >= 0)

//Defines for Symbolic Links...
#define DriveC "\\??\\C:"
#define DeviceC "\\Device\\Harddisk0\\Partition2"
#define DriveD "\\??\\D:"
#define CdRom "\\Device\\Cdrom0"
#define DriveE "\\??\\E:"
#define DeviceE "\\Device\\Harddisk0\\Partition1"
#define DriveF "\\??\\F:"
#define DeviceF "\\Device\\Harddisk0\\Partition6"

typedef CONST SHORT CSHORT;
/*
//Unicode STRING
typedef struct _STRING
{
 USHORT Length;
 USHORT MaximumLength;
 PSTR Buffer;
} UNICODE_STRING, *PUNICODE_STRING;//, ANSI_STRING, *PANSI_STRING;

//for use with IOCTL
typedef struct _IO_STATUS_BLOCK {
    union
 {
        LONG Status;
        LPVOID Pointer;
    };

    LPLONG Information;
} IO_STATUS_BLOCK, *PIO_STATUS_BLOCK;

//for use with IOCTL
typedef struct _OBJECT_ATTRIBUTES
{
    HANDLE   RootDirectory;
    PANSI_STRING ObjectName;
    ULONG   Attributes;
} OBJECT_ATTRIBUTES, *POBJECT_ATTRIBUTES;
*/
// APC routine
//typedef VOID (NTAPI *PIO_APC_ROUTINE) (IN PVOID ApcContext, IN PIO_STATUS_BLOCK IoStatusBlock, IN ULONG Reserved);


// Flags for OBJECT_ATTRIBUTES::Attributes
#define OBJ_INHERIT             0x00000002L
#define OBJ_PERMANENT           0x00000010L
#define OBJ_EXCLUSIVE           0x00000020L
#define OBJ_CASE_INSENSITIVE    0x00000040L
#define OBJ_OPENIF              0x00000080L
#define OBJ_OPENLINK            0x00000100L
#define OBJ_KERNEL_HANDLE       0x00000200L
#define OBJ_VALID_ATTRIBUTES    0x000003F2L

// Differences from NT: SECURITY_DESCRIPTOR support is gone.
#define InitializeObjectAttributes( p, n, a, r ) {  \
    (p)->RootDirectory = r;                             \
    (p)->Attributes = a;                                \
    (p)->ObjectName = n;                                \
    }


// CreateDisposition values for NtCreateFile()
#define FILE_SUPERSEDE                  0x00000000
#define FILE_OPEN                       0x00000001
#define FILE_CREATE                     0x00000002
#define FILE_OPEN_IF                    0x00000003
#define FILE_OVERWRITE                  0x00000004
#define FILE_OVERWRITE_IF               0x00000005
#define FILE_MAXIMUM_DISPOSITION        0x00000005


// CreateOption values for NtCreateFile()
// FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT is what CreateFile
// uses for most things when translating to NtCreateFile.
#define FILE_DIRECTORY_FILE                     0x00000001
#define FILE_WRITE_THROUGH                      0x00000002
#define FILE_SEQUENTIAL_ONLY                    0x00000004
#define FILE_NO_INTERMEDIATE_BUFFERING          0x00000008
#define FILE_SYNCHRONOUS_IO_ALERT               0x00000010
#define FILE_SYNCHRONOUS_IO_NONALERT            0x00000020
#define FILE_NON_DIRECTORY_FILE                 0x00000040
#define FILE_CREATE_TREE_CONNECTION             0x00000080
#define FILE_COMPLETE_IF_OPLOCKED               0x00000100
#define FILE_NO_EA_KNOWLEDGE                    0x00000200
#define FILE_OPEN_FOR_RECOVERY                  0x00000400
#define FILE_RANDOM_ACCESS                      0x00000800
#define FILE_DELETE_ON_CLOSE                    0x00001000
#define FILE_OPEN_BY_FILE_ID                    0x00002000
#define FILE_OPEN_FOR_BACKUP_INTENT             0x00004000
#define FILE_NO_COMPRESSION                     0x00008000
#define FILE_RESERVE_OPFILTER                   0x00100000
#define FILE_OPEN_REPARSE_POINT                 0x00200000
#define FILE_OPEN_NO_RECALL                     0x00400000
#define FILE_OPEN_FOR_FREE_SPACE_QUERY          0x00800000
#define FILE_COPY_STRUCTURED_STORAGE            0x00000041
#define FILE_STRUCTURED_STORAGE                 0x00000441
#define FILE_VALID_OPTION_FLAGS                 0x00ffffff
#define FILE_VALID_PIPE_OPTION_FLAGS            0x00000032
#define FILE_VALID_MAILSLOT_OPTION_FLAGS        0x00000032
#define FILE_VALID_SET_FLAGS                    0x00000036


#define FILE_DEVICE_CONTROLLER          0x00000004
#define IOCTL_SCSI_BASE                 FILE_DEVICE_CONTROLLER


// Access types
#define FILE_ANY_ACCESS                 0
#define FILE_READ_ACCESS          ( 0x0001 )    // file & pipe
#define FILE_WRITE_ACCESS         ( 0x0002 )    // file & pipe

// Method types
#define METHOD_BUFFERED                 0
#define METHOD_IN_DIRECT                1
#define METHOD_OUT_DIRECT               2
#define METHOD_NEITHER                  3

// The all-important CTL_CODE
#define CTL_CODE( DeviceType, Function, Method, Access ) (                 \
    ((DeviceType) << 16) | ((Access) << 14) | ((Function) << 2) | (Method) \
)

//Kernel Exports for doing all the cool stuff !!
extern "C"
{
  // XBOXAPI LONG WINAPI RtlInitAnsiString(OUT PANSI_STRING DestinationString, IN LPCSTR SourceString);

  //XBOXAPI LONG WINAPI HalWriteSMBusValue(UCHAR devddress, UCHAR offset, UCHAR writedw, DWORD data);
  XBOXAPI LONG WINAPI HalReadSMBusValue(UCHAR devddress, UCHAR offset, UCHAR readdw, LPBYTE pdata);

  // XBOXAPI LONG WINAPI IoCreateSymbolicLink(IN PUNICODE_STRING SymbolicLinkName,IN PUNICODE_STRING DeviceName);
  //XBOXAPI LONG WINAPI IoDeleteSymbolicLink(IN PUNICODE_STRING SymbolicLinkName);

  // XBOXAPI LONG WINAPI NtCreateFile(OUT PHANDLE FileHandle, IN ACCESS_MASK DesiredAccess, IN POBJECT_ATTRIBUTES ObjectAttributes,  OUT PIO_STATUS_BLOCK IoStatusBlock, IN PLARGE_INTEGER AllocationSize OPTIONAL, IN ULONG FileAttributes, IN ULONG ShareAccess, IN ULONG CreateDisposition, IN ULONG CreateOptions);

  //XBOXAPI LONG WINAPI NtDeviceIoControlFile(IN HANDLE FileHandle, IN HANDLE hEvent OPTIONAL, IN PIO_APC_ROUTINE ApcRoutine OPTIONAL, IN PVOID ApcContext OPTIONAL, OUT PIO_STATUS_BLOCK IoStatusBlock, IN ULONG IoControlCode, IN PVOID InputBuffer OPTIONAL, IN ULONG InputBufferLength, OUT PVOID OutputBuffer OPTIONAL, IN ULONG OutputBufferLength);

}

//Some important Kernel exported Variables
extern "C" XBOXAPI LPVOID XepDefaultImagePath;
extern "C" XBOXAPI LPVOID XepLoaderLock;
extern "C" XBOXAPI LPVOID XboxHardwareInfo;
extern "C" XBOXAPI LPVOID KeHasQuickBooted;
extern "C" XBOXAPI LPVOID ObpDosDevicesDriveLetterMap;
extern "C" XBOXAPI LPVOID XepDataTableEntry;
extern "C" XBOXAPI LPVOID FactorySettingsInfo;
extern "C" XBOXAPI LPVOID UserSettingsInfo;
extern "C" XBOXAPI LPVOID ExpCdRomBootROMString;
extern "C" XBOXAPI LPVOID ExpHardDiskBootROMString;
extern "C" XBOXAPI LPVOID ExpHDXbdmDLL;
extern "C" XBOXAPI LPVOID ExpDVDXbdmDLL;
extern "C" XBOXAPI LPVOID XepDashboardRedirectionPath;
extern "C" XBOXAPI LPVOID XepDashboardImagePath;
extern "C" XBOXAPI LPVOID ObpIoDevicesDirectoryObject;
extern "C" XBOXAPI LPVOID ExpCdRomBootROMStringBuffer; // IdexCdRomDeviceNameBuffer;
//extern "C" XBOXAPI LPVOID LaunchDataPage;
//extern "C" XBOXAPI LPVOID XboxBootFlags;
//extern "C" XBOXAPI LPVOID XeImageFileName;
extern "C" XBOXAPI LPVOID XboxEEPROMKey;
extern "C" XBOXAPI LPVOID XboxHDKey;

#endif
