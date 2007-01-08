/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1990-1999  Microsoft Corporation

Module Name:

ntddstor.h

Abstract:

This is the include file that defines all common constants and types
accessing the storage class drivers

Author:

Peter Wieland 19-Jun-1996

Revision History:

--*/


//
// Interface GUIDs
//
// need these GUIDs outside conditional includes so that user can
//   #include <ntddstor.h> in precompiled header
//   #include <initguid.h> in a single source file
//   #include <ntddstor.h> in that source file a second time to instantiate the GUIDs
//
#ifdef DEFINE_GUID 
//
// Make sure FAR is defined...
//
#ifndef FAR
#ifdef _WIN32
#define FAR
#else
#define FAR _far
#endif
#endif

// begin_wioctlguids
DEFINE_GUID(DiskClassGuid, 0x53f56307L, 0xb6bf, 0x11d0, 0x94, 0xf2, 0x00, 0xa0, 0xc9, 0x1e, 0xfb, 0x8b);
DEFINE_GUID(CdRomClassGuid, 0x53f56308L, 0xb6bf, 0x11d0, 0x94, 0xf2, 0x00, 0xa0, 0xc9, 0x1e, 0xfb, 0x8b);
DEFINE_GUID(PartitionClassGuid, 0x53f5630aL, 0xb6bf, 0x11d0, 0x94, 0xf2, 0x00, 0xa0, 0xc9, 0x1e, 0xfb, 0x8b);
DEFINE_GUID(TapeClassGuid, 0x53f5630bL, 0xb6bf, 0x11d0, 0x94, 0xf2, 0x00, 0xa0, 0xc9, 0x1e, 0xfb, 0x8b);
DEFINE_GUID(WriteOnceDiskClassGuid, 0x53f5630cL, 0xb6bf, 0x11d0, 0x94, 0xf2, 0x00, 0xa0, 0xc9, 0x1e, 0xfb, 0x8b);
DEFINE_GUID(VolumeClassGuid, 0x53f5630dL, 0xb6bf, 0x11d0, 0x94, 0xf2, 0x00, 0xa0, 0xc9, 0x1e, 0xfb, 0x8b);
DEFINE_GUID(MediumChangerClassGuid, 0x53f56310L, 0xb6bf, 0x11d0, 0x94, 0xf2, 0x00, 0xa0, 0xc9, 0x1e, 0xfb, 0x8b);
DEFINE_GUID(FloppyClassGuid, 0x53f56311L, 0xb6bf, 0x11d0, 0x94, 0xf2, 0x00, 0xa0, 0xc9, 0x1e, 0xfb, 0x8b);
DEFINE_GUID(CdChangerClassGuid, 0x53f56312L, 0xb6bf, 0x11d0, 0x94, 0xf2, 0x00, 0xa0, 0xc9, 0x1e, 0xfb, 0x8b);
DEFINE_GUID(StoragePortClassGuid, 0x2accfe60L, 0xc130, 0x11d2, 0xb0, 0x82, 0x00, 0xa0, 0xc9, 0x1e, 0xfb, 0x8b);
// end_wioctlguids
#endif

// begin_winioctl

#ifndef _NTDDSTOR_H_
#define _NTDDSTOR_H_

#ifdef __cplusplus
extern "C"
{
#endif

  //
  // IoControlCode values for storage devices
  //

#define IOCTL_STORAGE_BASE FILE_DEVICE_MASS_STORAGE

  //
  // The following device control codes are common for all class drivers.  They
  // should be used in place of the older IOCTL_DISK, IOCTL_CDROM and IOCTL_TAPE
  // common codes
  //

#define IOCTL_STORAGE_CHECK_VERIFY     CTL_CODE(IOCTL_STORAGE_BASE, 0x0200, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_STORAGE_CHECK_VERIFY2    CTL_CODE(IOCTL_STORAGE_BASE, 0x0200, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_STORAGE_MEDIA_REMOVAL    CTL_CODE(IOCTL_STORAGE_BASE, 0x0201, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_STORAGE_EJECT_MEDIA      CTL_CODE(IOCTL_STORAGE_BASE, 0x0202, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_STORAGE_LOAD_MEDIA       CTL_CODE(IOCTL_STORAGE_BASE, 0x0203, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_STORAGE_LOAD_MEDIA2      CTL_CODE(IOCTL_STORAGE_BASE, 0x0203, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_STORAGE_RESERVE          CTL_CODE(IOCTL_STORAGE_BASE, 0x0204, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_STORAGE_RELEASE          CTL_CODE(IOCTL_STORAGE_BASE, 0x0205, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_STORAGE_FIND_NEW_DEVICES CTL_CODE(IOCTL_STORAGE_BASE, 0x0206, METHOD_BUFFERED, FILE_READ_ACCESS)

#define IOCTL_STORAGE_EJECTION_CONTROL CTL_CODE(IOCTL_STORAGE_BASE, 0x0250, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_STORAGE_MCN_CONTROL      CTL_CODE(IOCTL_STORAGE_BASE, 0x0251, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_STORAGE_GET_MEDIA_TYPES  CTL_CODE(IOCTL_STORAGE_BASE, 0x0300, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_STORAGE_GET_MEDIA_TYPES_EX CTL_CODE(IOCTL_STORAGE_BASE, 0x0301, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_STORAGE_RESET_BUS        CTL_CODE(IOCTL_STORAGE_BASE, 0x0400, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_STORAGE_RESET_DEVICE     CTL_CODE(IOCTL_STORAGE_BASE, 0x0401, METHOD_BUFFERED, FILE_READ_ACCESS)

#define IOCTL_STORAGE_GET_DEVICE_NUMBER CTL_CODE(IOCTL_STORAGE_BASE, 0x0420, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_STORAGE_PREDICT_FAILURE CTL_CODE(IOCTL_STORAGE_BASE, 0x0440, METHOD_BUFFERED, FILE_ANY_ACCESS)

  // end_winioctl


#define IOCTL_STORAGE_QUERY_PROPERTY   CTL_CODE(IOCTL_STORAGE_BASE, 0x0500, METHOD_BUFFERED, FILE_ANY_ACCESS)


  // begin_winioctl

  //
  // These ioctl codes are obsolete.  They are defined here to avoid resuing them
  // and to allow class drivers to respond to them more easily.
  //

#define OBSOLETE_IOCTL_STORAGE_RESET_BUS        CTL_CODE(IOCTL_STORAGE_BASE, 0x0400, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define OBSOLETE_IOCTL_STORAGE_RESET_DEVICE     CTL_CODE(IOCTL_STORAGE_BASE, 0x0401, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)

  //
  // IOCTL_STORAGE_GET_DEVICE_NUMBER
  //
  // input - none
  //
  // output - STORAGE_DEVICE_NUMBER structure
  //          The values in the STORAGE_DEVICE_NUMBER structure are guaranteed
  //          to remain unchanged until the system is rebooted.  They are not
  //          guaranteed to be persistant across boots.
  //

  typedef struct _STORAGE_DEVICE_NUMBER
  {

    //
    // The FILE_DEVICE_XXX type for this device.
    //

    DEVICE_TYPE DeviceType;

    //
    // The number of this device
    //

    ULONG DeviceNumber;

    //
    // If the device is partitionable, the partition number of the device.
    // Otherwise -1
    //

    ULONG PartitionNumber;
  }
  STORAGE_DEVICE_NUMBER, *PSTORAGE_DEVICE_NUMBER;

  //
  // Define the structures for scsi resets
  //

  typedef struct _STORAGE_BUS_RESET_REQUEST
  {
    UCHAR PathId;
  }
  STORAGE_BUS_RESET_REQUEST, *PSTORAGE_BUS_RESET_REQUEST;

  //
  // IOCTL_STORAGE_MEDIA_REMOVAL disables the mechanism
  // on a storage device that ejects media. This function
  // may or may not be supported on storage devices that
  // support removable media.
  //
  // TRUE means prevent media from being removed.
  // FALSE means allow media removal.
  //

  typedef struct _PREVENT_MEDIA_REMOVAL
  {
    BOOLEAN PreventMediaRemoval;
  }
  PREVENT_MEDIA_REMOVAL, *PPREVENT_MEDIA_REMOVAL;

  // begin_ntminitape


  typedef struct _TAPE_STATISTICS
  {
    ULONG Version;
    ULONG Flags;
    LARGE_INTEGER RecoveredWrites;
    LARGE_INTEGER UnrecoveredWrites;
    LARGE_INTEGER RecoveredReads;
    LARGE_INTEGER UnrecoveredReads;
    UCHAR CompressionRatioReads;
    UCHAR CompressionRatioWrites;
  }
  TAPE_STATISTICS, *PTAPE_STATISTICS;

#define RECOVERED_WRITES_VALID   0x00000001
#define UNRECOVERED_WRITES_VALID 0x00000002
#define RECOVERED_READS_VALID    0x00000004
#define UNRECOVERED_READS_VALID  0x00000008
#define WRITE_COMPRESSION_INFO_VALID  0x00000010
#define READ_COMPRESSION_INFO_VALID   0x00000020

  typedef struct _TAPE_GET_STATISTICS
  {
    ULONG Operation;
  }
  TAPE_GET_STATISTICS, *PTAPE_GET_STATISTICS;

#define TAPE_RETURN_STATISTICS 0L
#define TAPE_RETURN_ENV_INFO   1L
#define TAPE_RESET_STATISTICS  2L

  //
  // IOCTL_STORAGE_GET_MEDIA_TYPES_EX will return an array of DEVICE_MEDIA_INFO
  // structures, one per supported type, embedded in the GET_MEDIA_TYPES struct.
  //

  typedef enum _STORAGE_MEDIA_TYPE {
    //
    // Following are defined in ntdddisk.h in the MEDIA_TYPE enum
    //
    // Unknown,                // Format is unknown
    // F5_1Pt2_512,            // 5.25", 1.2MB,  512 bytes/sector
    // F3_1Pt44_512,           // 3.5",  1.44MB, 512 bytes/sector
    // F3_2Pt88_512,           // 3.5",  2.88MB, 512 bytes/sector
    // F3_20Pt8_512,           // 3.5",  20.8MB, 512 bytes/sector
    // F3_720_512,             // 3.5",  720KB,  512 bytes/sector
    // F5_360_512,             // 5.25", 360KB,  512 bytes/sector
    // F5_320_512,             // 5.25", 320KB,  512 bytes/sector
    // F5_320_1024,            // 5.25", 320KB,  1024 bytes/sector
    // F5_180_512,             // 5.25", 180KB,  512 bytes/sector
    // F5_160_512,             // 5.25", 160KB,  512 bytes/sector
    // RemovableMedia,         // Removable media other than floppy
    // FixedMedia,             // Fixed hard disk media
    // F3_120M_512,            // 3.5", 120M Floppy
    // F3_640_512,             // 3.5" ,  640KB,  512 bytes/sector
    // F5_640_512,             // 5.25",  640KB,  512 bytes/sector
    // F5_720_512,             // 5.25",  720KB,  512 bytes/sector
    // F3_1Pt2_512,            // 3.5" ,  1.2Mb,  512 bytes/sector
    // F3_1Pt23_1024,          // 3.5" ,  1.23Mb, 1024 bytes/sector
    // F5_1Pt23_1024,          // 5.25",  1.23MB, 1024 bytes/sector
    // F3_128Mb_512,           // 3.5" MO 128Mb   512 bytes/sector
    // F3_230Mb_512,           // 3.5" MO 230Mb   512 bytes/sector
    // F8_256_128,             // 8",     256KB,  128 bytes/sector
    //

    DDS_4mm = 0x20,             // Tape - DAT DDS1,2,... (all vendors)
    MiniQic,                    // Tape - miniQIC Tape
    Travan,                     // Tape - Travan TR-1,2,3,...
    QIC,                        // Tape - QIC
    MP_8mm,                     // Tape - 8mm Exabyte Metal Particle
    AME_8mm,                    // Tape - 8mm Exabyte Advanced Metal Evap
    AIT1_8mm,                   // Tape - 8mm Sony AIT1
    DLT,                        // Tape - DLT Compact IIIxt, IV
    NCTP,                       // Tape - Philips NCTP
    IBM_3480,                   // Tape - IBM 3480
    IBM_3490E,                  // Tape - IBM 3490E
    IBM_Magstar_3590,           // Tape - IBM Magstar 3590
    IBM_Magstar_MP,             // Tape - IBM Magstar MP
    STK_DATA_D3,                // Tape - STK Data D3
    SONY_DTF,                   // Tape - Sony DTF
    DV_6mm,                     // Tape - 6mm Digital Video
    DMI,                        // Tape - Exabyte DMI and compatibles
    SONY_D2,                    // Tape - Sony D2S and D2L
    CLEANER_CARTRIDGE,          // Cleaner - All Drive types that support Drive Cleaners
    CD_ROM,                     // Opt_Disk - CD
    CD_R,                       // Opt_Disk - CD-Recordable (Write Once)
    CD_RW,                      // Opt_Disk - CD-Rewriteable
    DVD_ROM,                    // Opt_Disk - DVD-ROM
    DVD_R,                      // Opt_Disk - DVD-Recordable (Write Once)
    DVD_RW,                     // Opt_Disk - DVD-Rewriteable
    MO_3_RW,                    // Opt_Disk - 3.5" Rewriteable MO Disk
    MO_5_WO,                    // Opt_Disk - MO 5.25" Write Once
    MO_5_RW,                    // Opt_Disk - MO 5.25" Rewriteable (not LIMDOW)
    MO_5_LIMDOW,                // Opt_Disk - MO 5.25" Rewriteable (LIMDOW)
    PC_5_WO,                    // Opt_Disk - Phase Change 5.25" Write Once Optical
    PC_5_RW,                    // Opt_Disk - Phase Change 5.25" Rewriteable
    PD_5_RW,                    // Opt_Disk - PhaseChange Dual Rewriteable
    ABL_5_WO,                   // Opt_Disk - Ablative 5.25" Write Once Optical
    PINNACLE_APEX_5_RW,         // Opt_Disk - Pinnacle Apex 4.6GB Rewriteable Optical
    SONY_12_WO,                 // Opt_Disk - Sony 12" Write Once
    PHILIPS_12_WO,              // Opt_Disk - Philips/LMS 12" Write Once
    HITACHI_12_WO,              // Opt_Disk - Hitachi 12" Write Once
    CYGNET_12_WO,               // Opt_Disk - Cygnet/ATG 12" Write Once
    KODAK_14_WO,                // Opt_Disk - Kodak 14" Write Once
    MO_NFR_525,                 // Opt_Disk - Near Field Recording (Terastor)
    NIKON_12_RW,                // Opt_Disk - Nikon 12" Rewriteable
    IOMEGA_ZIP,                 // Mag_Disk - Iomega Zip
    IOMEGA_JAZ,                 // Mag_Disk - Iomega Jaz
    SYQUEST_EZ135,              // Mag_Disk - Syquest EZ135
    SYQUEST_EZFLYER,            // Mag_Disk - Syquest EzFlyer
    SYQUEST_SYJET,              // Mag_Disk - Syquest SyJet
    AVATAR_F2,                  // Mag_Disk - 2.5" Floppy
    MP2_8mm,                    // Tape - 8mm Hitachi
    DST_S,                      // Ampex DST Small Tapes
    DST_M,                      // Ampex DST Medium Tapes
    DST_L,                      // Ampex DST Large Tapes
    VXATape_1,                  // Ecrix 8mm Tape
    VXATape_2,                  // Ecrix 8mm Tape
    STK_EAGLE,                  // STK Eagle
    LTO_Ultrium,                // IBM, HP, Seagate LTO Ultrium
    LTO_Accelis                // IBM, HP, Seagate LTO Accelis
  } STORAGE_MEDIA_TYPE, *PSTORAGE_MEDIA_TYPE;

#define MEDIA_ERASEABLE         0x00000001
#define MEDIA_WRITE_ONCE        0x00000002
#define MEDIA_READ_ONLY         0x00000004
#define MEDIA_READ_WRITE        0x00000008

#define MEDIA_WRITE_PROTECTED   0x00000100
#define MEDIA_CURRENTLY_MOUNTED 0x80000000

  //
  // Define the different storage bus types
  // Bus types below 128 (0x80) are reserved for Microsoft use
  //

  typedef enum _STORAGE_BUS_TYPE {
    BusTypeUnknown = 0x00,
    BusTypeScsi,
    BusTypeAtapi,
    BusTypeAta,
    BusType1394,
    BusTypeSsa,
    BusTypeFibre,
    BusTypeUsb,
    BusTypeRAID,
    BusTypeMaxReserved = 0x7F
  } STORAGE_BUS_TYPE, *PSTORAGE_BUS_TYPE;

  typedef struct _DEVICE_MEDIA_INFO
  {
    union {
      struct
      {
        LARGE_INTEGER Cylinders;
        STORAGE_MEDIA_TYPE MediaType;
        ULONG TracksPerCylinder;
        ULONG SectorsPerTrack;
        ULONG BytesPerSector;
        ULONG NumberMediaSides;
        ULONG MediaCharacteristics; // Bitmask of MEDIA_XXX values.
      }
      DiskInfo;

      struct
      {
        LARGE_INTEGER Cylinders;
        STORAGE_MEDIA_TYPE MediaType;
        ULONG TracksPerCylinder;
        ULONG SectorsPerTrack;
        ULONG BytesPerSector;
        ULONG NumberMediaSides;
        ULONG MediaCharacteristics; // Bitmask of MEDIA_XXX values.
      }
      RemovableDiskInfo;

      struct
      {
        STORAGE_MEDIA_TYPE MediaType;
        ULONG MediaCharacteristics; // Bitmask of MEDIA_XXX values.
        ULONG CurrentBlockSize;
        STORAGE_BUS_TYPE BusType;

        //
        // Bus specific information describing the medium supported.
        //

        union {
          struct
          {
            UCHAR MediumType;
            UCHAR DensityCode;
          }
          ScsiInformation;
        } BusSpecificData;

      }
      TapeInfo;
    } DeviceSpecific;
  }
  DEVICE_MEDIA_INFO, *PDEVICE_MEDIA_INFO;

  typedef struct _GET_MEDIA_TYPES
  {
    ULONG DeviceType;              // FILE_DEVICE_XXX values
    ULONG MediaInfoCount;
    DEVICE_MEDIA_INFO MediaInfo[1];
  }
  GET_MEDIA_TYPES, *PGET_MEDIA_TYPES;


  //
  // IOCTL_STORAGE_PREDICT_FAILURE
  //
  // input - none
  //
  // output - STORAGE_PREDICT_FAILURE structure
  //          PredictFailure returns zero if no failure predicted and non zero
  //                         if a failure is predicted.
  //
  //          VendorSpecific returns 512 bytes of vendor specific information
  //                         if a failure is predicted
  //
  typedef struct _STORAGE_PREDICT_FAILURE
  {
    ULONG PredictFailure;
    UCHAR VendorSpecific[512];
  }
  STORAGE_PREDICT_FAILURE, *PSTORAGE_PREDICT_FAILURE;

  // end_ntminitape
  // end_winioctl

  //
  // Property Query Structures
  //

  //
  // IOCTL_STORAGE_QUERY_PROPERTY
  //
  // Input Buffer:
  //      a STORAGE_PROPERTY_QUERY structure which describes what type of query
  //      is being done, what property is being queried for, and any additional
  //      parameters which a particular property query requires.
  //
  //  Output Buffer:
  //      Contains a buffer to place the results of the query into.  Since all
  //      property descriptors can be cast into a STORAGE_DESCRIPTOR_HEADER,
  //      the IOCTL can be called once with a small buffer then again using
  //      a buffer as large as the header reports is necessary.
  //


  //
  // Types of queries
  //

  typedef enum _STORAGE_QUERY_TYPE {
    PropertyStandardQuery = 0,           // Retrieves the descriptor
    PropertyExistsQuery,                 // Used to test whether the descriptor is supported
    PropertyMaskQuery,                   // Used to retrieve a mask of writeable fields in the descriptor
    PropertyQueryMaxDefined     // use to validate the value
  } STORAGE_QUERY_TYPE, *PSTORAGE_QUERY_TYPE;

  //
  // define some initial property id's
  //

  typedef enum _STORAGE_PROPERTY_ID {
    StorageDeviceProperty = 0,
    StorageAdapterProperty
  } STORAGE_PROPERTY_ID, *PSTORAGE_PROPERTY_ID;

  //
  // Query structure - additional parameters for specific queries can follow
  // the header
  //

  typedef struct _STORAGE_PROPERTY_QUERY
  {

    //
    // ID of the property being retrieved
    //

    STORAGE_PROPERTY_ID PropertyId;

    //
    // Flags indicating the type of query being performed
    //

    STORAGE_QUERY_TYPE QueryType;

    //
    // Space for additional parameters if necessary
    //

    UCHAR AdditionalParameters[1];

  }
  STORAGE_PROPERTY_QUERY, *PSTORAGE_PROPERTY_QUERY;

  //
  // Standard property descriptor header.  All property pages should use this
  // as their first element or should contain these two elements
  //

  typedef struct _STORAGE_DESCRIPTOR_HEADER
  {

    ULONG Version;

    ULONG Size;

  }
  STORAGE_DESCRIPTOR_HEADER, *PSTORAGE_DESCRIPTOR_HEADER;

  //
  // Device property descriptor - this is really just a rehash of the inquiry
  // data retrieved from a scsi device
  //
  // This may only be retrieved from a target device.  Sending this to the bus
  // will result in an error
  //

  typedef struct _STORAGE_DEVICE_DESCRIPTOR
  {

    //
    // Sizeof(STORAGE_DEVICE_DESCRIPTOR)
    //

    ULONG Version;

    //
    // Total size of the descriptor, including the space for additional
    // data and id strings
    //

    ULONG Size;

    //
    // The SCSI-2 device type
    //

    UCHAR DeviceType;

    //
    // The SCSI-2 device type modifier (if any) - this may be zero
    //

    UCHAR DeviceTypeModifier;

    //
    // Flag indicating whether the device's media (if any) is removable.  This
    // field should be ignored for media-less devices
    //

    BOOLEAN RemovableMedia;

    //
    // Flag indicating whether the device can support mulitple outstanding
    // commands.  The actual synchronization in this case is the responsibility
    // of the port driver.
    //

    BOOLEAN CommandQueueing;

    //
    // Byte offset to the zero-terminated ascii string containing the device's
    // vendor id string.  For devices with no such ID this will be zero
    //

    ULONG VendorIdOffset;

    //
    // Byte offset to the zero-terminated ascii string containing the device's
    // product id string.  For devices with no such ID this will be zero
    //

    ULONG ProductIdOffset;

    //
    // Byte offset to the zero-terminated ascii string containing the device's
    // product revision string.  For devices with no such string this will be
    // zero
    //

    ULONG ProductRevisionOffset;

    //
    // Byte offset to the zero-terminated ascii string containing the device's
    // serial number.  For devices with no serial number this will be zero
    //

    ULONG SerialNumberOffset;

    //
    // Contains the bus type (as defined above) of the device.  It should be
    // used to interpret the raw device properties at the end of this structure
    // (if any)
    //

    STORAGE_BUS_TYPE BusType;

    //
    // The number of bytes of bus-specific data which have been appended to
    // this descriptor
    //

    ULONG RawPropertiesLength;

    //
    // Place holder for the first byte of the bus specific property data
    //

    UCHAR RawDeviceProperties[1];

  }
  STORAGE_DEVICE_DESCRIPTOR, *PSTORAGE_DEVICE_DESCRIPTOR;


  //
  // Adapter properties
  //
  // This descriptor can be retrieved from a target device object of from the
  // device object for the bus.  Retrieving from the target device object will
  // forward the request to the underlying bus
  //

  typedef struct _STORAGE_ADAPTER_DESCRIPTOR
  {

    ULONG Version;

    ULONG Size;

    ULONG MaximumTransferLength;

    ULONG MaximumPhysicalPages;

    ULONG AlignmentMask;

    BOOLEAN AdapterUsesPio;

    BOOLEAN AdapterScansDown;

    BOOLEAN CommandQueueing;

    BOOLEAN AcceleratedTransfer;

    BOOLEAN BusType;

    USHORT BusMajorVersion;

    USHORT BusMinorVersion;

  }
  STORAGE_ADAPTER_DESCRIPTOR, *PSTORAGE_ADAPTER_DESCRIPTOR;

  // begin_winioctl

#ifdef __cplusplus
}
#endif

#endif // _NTDDSTOR_H_ 
// end_winioctl

