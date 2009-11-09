/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    ntddcdrm.h

Abstract:

    This module contains structures and definitions
    associated with CDROM IOCTls.

Author:

    Mike Glass

Revision History:

--*/

// begin_winioctl

#ifndef _NTDDCDRM_
#define _NTDDCDRM_

#if _MSC_VER >= 1200
#pragma warning(push)
#endif

#if _MSC_VER > 1000
#pragma once
#endif

//
// remove some level 4 warnings for this header file:
#pragma warning(disable:4200) // array[0]
#pragma warning(disable:4201) // nameless struct/unions
#pragma warning(disable:4214) // bit fields other than int

#ifdef __cplusplus
extern "C" {
#endif

//
// NtDeviceIoControlFile IoControlCode values for this device.
//
// Warning:  Remember that the low two bits of the code specify how the
//           buffers are passed to the driver!
//

#define IOCTL_CDROM_BASE                 FILE_DEVICE_CD_ROM

#define IOCTL_CDROM_UNLOAD_DRIVER        CTL_CODE(IOCTL_CDROM_BASE, 0x0402, METHOD_BUFFERED, FILE_READ_ACCESS)

//
// CDROM Audio Device Control Functions
//

#define IOCTL_CDROM_READ_TOC         CTL_CODE(IOCTL_CDROM_BASE, 0x0000, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_CDROM_SEEK_AUDIO_MSF   CTL_CODE(IOCTL_CDROM_BASE, 0x0001, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_CDROM_STOP_AUDIO       CTL_CODE(IOCTL_CDROM_BASE, 0x0002, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_CDROM_PAUSE_AUDIO      CTL_CODE(IOCTL_CDROM_BASE, 0x0003, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_CDROM_RESUME_AUDIO     CTL_CODE(IOCTL_CDROM_BASE, 0x0004, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_CDROM_GET_VOLUME       CTL_CODE(IOCTL_CDROM_BASE, 0x0005, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_CDROM_PLAY_AUDIO_MSF   CTL_CODE(IOCTL_CDROM_BASE, 0x0006, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_CDROM_SET_VOLUME       CTL_CODE(IOCTL_CDROM_BASE, 0x000A, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_CDROM_READ_Q_CHANNEL   CTL_CODE(IOCTL_CDROM_BASE, 0x000B, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_CDROM_GET_CONTROL      CTL_CODE(IOCTL_CDROM_BASE, 0x000D, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_CDROM_GET_LAST_SESSION CTL_CODE(IOCTL_CDROM_BASE, 0x000E, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_CDROM_RAW_READ         CTL_CODE(IOCTL_CDROM_BASE, 0x000F, METHOD_OUT_DIRECT,  FILE_READ_ACCESS)
#define IOCTL_CDROM_DISK_TYPE        CTL_CODE(IOCTL_CDROM_BASE, 0x0010, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_CDROM_GET_DRIVE_GEOMETRY    CTL_CODE(IOCTL_CDROM_BASE, 0x0013, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_CDROM_GET_DRIVE_GEOMETRY_EX CTL_CODE(IOCTL_CDROM_BASE, 0x0014, METHOD_BUFFERED, FILE_READ_ACCESS)

#define IOCTL_CDROM_READ_TOC_EX       CTL_CODE(IOCTL_CDROM_BASE, 0x0015, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_CDROM_GET_CONFIGURATION CTL_CODE(IOCTL_CDROM_BASE, 0x0016, METHOD_BUFFERED, FILE_READ_ACCESS)

// end_winioctl

//
// The following device control codes are common for all class drivers.  The
// functions codes defined here must match all of the other class drivers.
//
// Warning: these codes will be replaced in the future with the IOCTL_STORAGE
// codes included below
//

#define IOCTL_CDROM_CHECK_VERIFY    CTL_CODE(IOCTL_CDROM_BASE, 0x0200, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_CDROM_MEDIA_REMOVAL   CTL_CODE(IOCTL_CDROM_BASE, 0x0201, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_CDROM_EJECT_MEDIA     CTL_CODE(IOCTL_CDROM_BASE, 0x0202, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_CDROM_LOAD_MEDIA      CTL_CODE(IOCTL_CDROM_BASE, 0x0203, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_CDROM_RESERVE         CTL_CODE(IOCTL_CDROM_BASE, 0x0204, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_CDROM_RELEASE         CTL_CODE(IOCTL_CDROM_BASE, 0x0205, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_CDROM_FIND_NEW_DEVICES CTL_CODE(IOCTL_CDROM_BASE, 0x0206, METHOD_BUFFERED, FILE_READ_ACCESS)

//
// The following file contains the IOCTL_STORAGE class ioctl definitions
//

#include "ntddstor.h"

// begin_winioctl

//
// The following device control code is for the SIMBAD simulated bad
// sector facility. See SIMBAD.H in this directory for related structures.
//

#define IOCTL_CDROM_SIMBAD        CTL_CODE(IOCTL_CDROM_BASE, 0x1003, METHOD_BUFFERED, FILE_READ_ACCESS)

//
// Maximum CD Rom size
//

#define MAXIMUM_NUMBER_TRACKS 100
#define MAXIMUM_CDROM_SIZE 804
#define MINIMUM_CDROM_READ_TOC_EX_SIZE 2  // two bytes min transferred

//
// READ_TOC_EX structure
//
typedef struct _CDROM_READ_TOC_EX {
    UCHAR Format    : 4;
    UCHAR Reserved1 : 3; // future expansion
    UCHAR Msf       : 1;
    UCHAR SessionTrack;
    UCHAR Reserved2;     // future expansion
    UCHAR Reserved3;     // future expansion
} CDROM_READ_TOC_EX, *PCDROM_READ_TOC_EX;

#define CDROM_READ_TOC_EX_FORMAT_TOC      0x00
#define CDROM_READ_TOC_EX_FORMAT_SESSION  0x01
#define CDROM_READ_TOC_EX_FORMAT_FULL_TOC 0x02
#define CDROM_READ_TOC_EX_FORMAT_PMA      0x03
#define CDROM_READ_TOC_EX_FORMAT_ATIP     0x04
#define CDROM_READ_TOC_EX_FORMAT_CDTEXT   0x05

//
// CD ROM Table OF Contents (TOC)
// Format 0 - Get table of contents
//

typedef struct _TRACK_DATA {
    UCHAR Reserved;
    UCHAR Control : 4;
    UCHAR Adr : 4;
    UCHAR TrackNumber;
    UCHAR Reserved1;
    UCHAR Address[4];
} TRACK_DATA, *PTRACK_DATA;

typedef struct _CDROM_TOC {

    //
    // Header
    //

    UCHAR Length[2];  // add two bytes for this field
    UCHAR FirstTrack;
    UCHAR LastTrack;

    //
    // Track data
    //

    TRACK_DATA TrackData[MAXIMUM_NUMBER_TRACKS];
} CDROM_TOC, *PCDROM_TOC;

#define CDROM_TOC_SIZE sizeof(CDROM_TOC)

//
// CD ROM Table OF Contents
// Format 1 - Session Information
//

typedef struct _CDROM_TOC_SESSION_DATA {
    
    //
    // Header
    //

    UCHAR Length[2];  // add two bytes for this field
    UCHAR FirstCompleteSession;
    UCHAR LastCompleteSession;

    //
    // One track, representing the first track
    // of the last finished session
    //

    TRACK_DATA TrackData[1];

} CDROM_TOC_SESSION_DATA, *PCDROM_TOC_SESSION_DATA;


//
// CD ROM Table OF Contents
// Format 2 - Full TOC
//

typedef struct _CDROM_TOC_FULL_TOC_DATA_BLOCK {
    UCHAR SessionNumber;
    UCHAR Control      : 4;
    UCHAR Adr          : 4;
    UCHAR Reserved1;
    UCHAR Point;
    UCHAR MsfExtra[3];
    UCHAR Zero;
    UCHAR Msf[3];
} CDROM_TOC_FULL_TOC_DATA_BLOCK, *PCDROM_TOC_FULL_TOC_DATA_BLOCK;

typedef struct _CDROM_TOC_FULL_TOC_DATA {
    
    //
    // Header
    //

    UCHAR Length[2];  // add two bytes for this field
    UCHAR FirstCompleteSession;
    UCHAR LastCompleteSession;

    //
    // one to N descriptors included
    //

    CDROM_TOC_FULL_TOC_DATA_BLOCK Descriptors[0];

} CDROM_TOC_FULL_TOC_DATA, *PCDROM_TOC_FULL_TOC_DATA;

//
// CD ROM Table OF Contents
// Format 3 - Program Memory Area
//
typedef struct _CDROM_TOC_PMA_DATA {
    
    //
    // Header
    //

    UCHAR Length[2];  // add two bytes for this field
    UCHAR Reserved1;
    UCHAR Reserved2;

    //
    // one to N descriptors included
    //

    CDROM_TOC_FULL_TOC_DATA_BLOCK Descriptors[0];

} CDROM_TOC_PMA_DATA, *PCDROM_TOC_PMA_DATA;

//
// CD ROM Table OF Contents
// Format 4 - Absolute Time In Pregroove
//

typedef struct _CDROM_TOC_ATIP_DATA_BLOCK {

    UCHAR CdrwReferenceSpeed : 3;
    UCHAR Reserved3          : 1;
    UCHAR WritePower         : 3;
    UCHAR True1              : 1;
    UCHAR Reserved4       : 6;
    UCHAR UnrestrictedUse : 1;
    UCHAR Reserved5       : 1;
    UCHAR A3Valid     : 1;
    UCHAR A2Valid     : 1;
    UCHAR A1Valid     : 1;
    UCHAR DiscSubType : 3;
    UCHAR IsCdrw      : 1;
    UCHAR True2       : 1;
    UCHAR Reserved7;
    
    UCHAR LeadInMsf[3];
    UCHAR Reserved8;
    
    UCHAR LeadOutMsf[3];
    UCHAR Reserved9;
    
    UCHAR A1Values[3];
    UCHAR Reserved10;
    
    UCHAR A2Values[3];
    UCHAR Reserved11;

    UCHAR A3Values[3];
    UCHAR Reserved12;

} CDROM_TOC_ATIP_DATA_BLOCK, *PCDROM_TOC_ATIP_DATA_BLOCK;

typedef struct _CDROM_TOC_ATIP_DATA {
    
    //
    // Header
    //

    UCHAR Length[2];  // add two bytes for this field
    UCHAR Reserved1;
    UCHAR Reserved2;

    //
    // zero? to N descriptors included.
    //

    CDROM_TOC_ATIP_DATA_BLOCK Descriptors[0];

} CDROM_TOC_ATIP_DATA, *PCDROM_TOC_ATIP_DATA;

//
// CD ROM Table OF Contents
// Format 5 - CD Text Info
//
typedef struct _CDROM_TOC_CD_TEXT_DATA_BLOCK {
    UCHAR PackType;
    UCHAR TrackNumber       : 7;
    UCHAR ExtensionFlag     : 1;  // should be zero!
    UCHAR SequenceNumber;
    UCHAR CharacterPosition : 4;
    UCHAR BlockNumber       : 3;
    UCHAR Unicode           : 1;
    union {
        UCHAR Text[12];
        WCHAR WText[6];
    };
    UCHAR CRC[2];
} CDROM_TOC_CD_TEXT_DATA_BLOCK, *PCDROM_TOC_CD_TEXT_DATA_BLOCK;

typedef struct _CDROM_TOC_CD_TEXT_DATA {
    
    //
    // Header
    //

    UCHAR Length[2];  // add two bytes for this field
    UCHAR Reserved1;
    UCHAR Reserved2;
    
    //
    // the text info comes in discrete blocks of
    // a heavily-overloaded structure
    //
    
    CDROM_TOC_CD_TEXT_DATA_BLOCK Descriptors[0];

} CDROM_TOC_CD_TEXT_DATA, *PCDROM_TOC_CD_TEXT_DATA;

//
// These are the types used for PackType field in CDROM_TOC_CD_TEXT_DATA_BLOCK
// and also for requesting specific info from IOCTL_CDROM_READ_CD_TEXT
//
#define CDROM_CD_TEXT_PACK_ALBUM_NAME 0x80
#define CDROM_CD_TEXT_PACK_PERFORMER  0x81
#define CDROM_CD_TEXT_PACK_SONGWRITER 0x82
#define CDROM_CD_TEXT_PACK_COMPOSER   0x83
#define CDROM_CD_TEXT_PACK_ARRANGER   0x84
#define CDROM_CD_TEXT_PACK_MESSAGES   0x85
#define CDROM_CD_TEXT_PACK_DISC_ID    0x86
#define CDROM_CD_TEXT_PACK_GENRE      0x87
#define CDROM_CD_TEXT_PACK_TOC_INFO   0x88
#define CDROM_CD_TEXT_PACK_TOC_INFO2  0x89
// 0x8a - 0x8d are reserved....
#define CDROM_CD_TEXT_PACK_UPC_EAN    0x8e
#define CDROM_CD_TEXT_PACK_SIZE_INFO  0x8f

//
// Play audio starting at MSF and ending at MSF
//

typedef struct _CDROM_PLAY_AUDIO_MSF {
    UCHAR StartingM;
    UCHAR StartingS;
    UCHAR StartingF;
    UCHAR EndingM;
    UCHAR EndingS;
    UCHAR EndingF;
} CDROM_PLAY_AUDIO_MSF, *PCDROM_PLAY_AUDIO_MSF;

//
// Seek to MSF
//

typedef struct _CDROM_SEEK_AUDIO_MSF {
    UCHAR M;
    UCHAR S;
    UCHAR F;
} CDROM_SEEK_AUDIO_MSF, *PCDROM_SEEK_AUDIO_MSF;


//
//  Flags for the disk type
//

typedef struct _CDROM_DISK_DATA {

    ULONG DiskData;

} CDROM_DISK_DATA, *PCDROM_DISK_DATA;

#define CDROM_DISK_AUDIO_TRACK      (0x00000001)
#define CDROM_DISK_DATA_TRACK       (0x00000002)

//
// CD ROM Data Mode Codes, used with IOCTL_CDROM_READ_Q_CHANNEL
//

#define IOCTL_CDROM_SUB_Q_CHANNEL    0x00
#define IOCTL_CDROM_CURRENT_POSITION 0x01
#define IOCTL_CDROM_MEDIA_CATALOG    0x02
#define IOCTL_CDROM_TRACK_ISRC       0x03

typedef struct _CDROM_SUB_Q_DATA_FORMAT {
    UCHAR Format;
    UCHAR Track;
} CDROM_SUB_Q_DATA_FORMAT, *PCDROM_SUB_Q_DATA_FORMAT;


//
// CD ROM Sub-Q Channel Data Format
//

typedef struct _SUB_Q_HEADER {
    UCHAR Reserved;
    UCHAR AudioStatus;
    UCHAR DataLength[2];
} SUB_Q_HEADER, *PSUB_Q_HEADER;

typedef struct _SUB_Q_CURRENT_POSITION {
    SUB_Q_HEADER Header;
    UCHAR FormatCode;
    UCHAR Control : 4;
    UCHAR ADR : 4;
    UCHAR TrackNumber;
    UCHAR IndexNumber;
    UCHAR AbsoluteAddress[4];
    UCHAR TrackRelativeAddress[4];
} SUB_Q_CURRENT_POSITION, *PSUB_Q_CURRENT_POSITION;

typedef struct _SUB_Q_MEDIA_CATALOG_NUMBER {
    SUB_Q_HEADER Header;
    UCHAR FormatCode;
    UCHAR Reserved[3];
    UCHAR Reserved1 : 7;
    UCHAR Mcval : 1;
    UCHAR MediaCatalog[15];
} SUB_Q_MEDIA_CATALOG_NUMBER, *PSUB_Q_MEDIA_CATALOG_NUMBER;

typedef struct _SUB_Q_TRACK_ISRC {
    SUB_Q_HEADER Header;
    UCHAR FormatCode;
    UCHAR Reserved0;
    UCHAR Track;
    UCHAR Reserved1;
    UCHAR Reserved2 : 7;
    UCHAR Tcval : 1;
    UCHAR TrackIsrc[15];
} SUB_Q_TRACK_ISRC, *PSUB_Q_TRACK_ISRC;

typedef union _SUB_Q_CHANNEL_DATA {
    SUB_Q_CURRENT_POSITION CurrentPosition;
    SUB_Q_MEDIA_CATALOG_NUMBER MediaCatalog;
    SUB_Q_TRACK_ISRC TrackIsrc;
} SUB_Q_CHANNEL_DATA, *PSUB_Q_CHANNEL_DATA;

//
// Audio Status Codes
//

#define AUDIO_STATUS_NOT_SUPPORTED  0x00
#define AUDIO_STATUS_IN_PROGRESS    0x11
#define AUDIO_STATUS_PAUSED         0x12
#define AUDIO_STATUS_PLAY_COMPLETE  0x13
#define AUDIO_STATUS_PLAY_ERROR     0x14
#define AUDIO_STATUS_NO_STATUS      0x15

//
// ADR Sub-channel Q Field
//

#define ADR_NO_MODE_INFORMATION     0x0
#define ADR_ENCODES_CURRENT_POSITION 0x1
#define ADR_ENCODES_MEDIA_CATALOG   0x2
#define ADR_ENCODES_ISRC            0x3

//
// Sub-channel Q Control Bits
//

#define AUDIO_WITH_PREEMPHASIS      0x1
#define DIGITAL_COPY_PERMITTED      0x2
#define AUDIO_DATA_TRACK            0x4
#define TWO_FOUR_CHANNEL_AUDIO      0x8

//
// Get Audio control parameters
//

typedef struct _CDROM_AUDIO_CONTROL {
    UCHAR LbaFormat;
    USHORT LogicalBlocksPerSecond;
} CDROM_AUDIO_CONTROL, *PCDROM_AUDIO_CONTROL;

//
// Volume control - Volume takes a value between 1 and 0xFF.
// SCSI-II CDROM audio suppports up to 4 audio ports with
// Independent volume control.
//

typedef struct _VOLUME_CONTROL {
    UCHAR PortVolume[4];
} VOLUME_CONTROL, *PVOLUME_CONTROL;

typedef enum _TRACK_MODE_TYPE {
    YellowMode2,
    XAForm2,
    CDDA
} TRACK_MODE_TYPE, *PTRACK_MODE_TYPE;

//
// Passed to cdrom to describe the raw read, ie. Mode 2, Form 2, CDDA...
//

typedef struct __RAW_READ_INFO {
    LARGE_INTEGER DiskOffset;
    ULONG    SectorCount;
    TRACK_MODE_TYPE TrackMode;
} RAW_READ_INFO, *PRAW_READ_INFO;

#ifdef __cplusplus
}
#endif


#if _MSC_VER >= 1200
#pragma warning(pop)          // un-sets any local warning changes
#else
#pragma warning(default:4200) // array[0] is not a warning for this file
#pragma warning(default:4201) // nameless struct/unions
#pragma warning(default:4214) // bit fields other than int
#endif


#endif  // _NTDDCDRM_

// end_winioctl


