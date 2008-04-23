#pragma once

#ifdef HAS_XBOX_HARDWARE
#include "Undocumented.h"
#else // from Undocumented.h
typedef const char *PCSZ;

  // XBE header information
  typedef struct _XBE_HEADER
  {
    // 000 "XBEH"
    CHAR Magic[4];
    // 004 RSA digital signature of the entire header area
    UCHAR HeaderSignature[256];
    // 104 Base address of XBE image (must be 0x00010000?)
    PVOID BaseAddress;
    // 108 Size of all headers combined - other headers must be within this
    ULONG HeaderSize;
    // 10C Size of entire image
    ULONG ImageSize;
    // 110 Size of this header (always 0x178?)
    ULONG XbeHeaderSize;
    // 114 Image timestamp - unknown format
    ULONG Timestamp;
    // 118 Pointer to certificate data (must be within HeaderSize)
    struct _XBE_CERTIFICATE *Certificate;
    // 11C Number of sections
    DWORD NumSections;
    // 120 Pointer to section headers (must be within HeaderSize)
    struct _XBE_SECTION *Sections;
    // 124 Initialization flags
    ULONG InitFlags;
    // 128 Entry point (XOR'd; see xboxhacker.net)
    PVOID EntryPoint;
    // 12C Pointer to TLS directory
    struct _XBE_TLS_DIRECTORY *TlsDirectory;
    // 130 Stack commit size
    ULONG StackCommit;
    // 134 Heap reserve size
    ULONG HeapReserve;
    // 138 Heap commit size
    ULONG HeapCommit;
    // 13C PE base address (?)
    PVOID PeBaseAddress;
    // 140 PE image size (?)
    ULONG PeImageSize;
    // 144 PE checksum (?)
    ULONG PeChecksum;
    // 148 PE timestamp (?)
    ULONG PeTimestamp;
    // 14C PC path and filename to EXE file from which XBE is derived
    PCSZ PcExePath;
    // 150 PC filename (last part of PcExePath) from which XBE is derived
    PCSZ PcExeFilename;
    // 154 PC filename (Unicode version of PcExeFilename)
    PWSTR PcExeFilenameUnicode;
    // 158 Pointer to kernel thunk table (XOR'd; EFB1F152 debug)
    ULONG_PTR *KernelThunkTable;
    // 15C Non-kernel import table (debug only)
    PVOID DebugImportTable;
    // 160 Number of library headers
    ULONG NumLibraries;
    // 164 Pointer to library headers
    struct _XBE_LIBRARY *Libraries;
    // 168 Pointer to kernel library header
    struct _XBE_LIBRARY *KernelLibrary;
    // 16C Pointer to XAPI library
    struct _XBE_LIBRARY *XapiLibrary;
    // 170 Pointer to logo bitmap (NULL = use default of Microsoft)
    PVOID LogoBitmap;
    // 174 Size of logo bitmap
    ULONG LogoBitmapSize;
    // 178
  }
  XBE_HEADER, *PXBE_HEADER;

  // Certificate structure
  typedef struct _XBE_CERTIFICATE
  {
    // 000 Size of certificate
    ULONG Size;
    // 004 Certificate timestamp (unknown format)
    ULONG Timestamp;
    // 008 Title ID
    ULONG TitleId;
    // 00C Name of the game (Unicode)
    WCHAR TitleName[40];
    // 05C Alternate title ID's (0-terminated)
    ULONG AlternateTitleIds[16];
    // 09C Allowed media types - 1 bit match between XBE and media = boots
    ULONG MediaTypes;
    // 0A0 Allowed game regions - 1 bit match between this and XBOX = boots
    ULONG GameRegion;
    // 0A4 Allowed game ratings - 1 bit match between this and XBOX = boots
    ULONG GameRating;
    // 0A8 Disk number (?)
    ULONG DiskNumber;
    // 0AC Version (?)
    ULONG Version;
    // 0B0 LAN key for this game
    UCHAR LanKey[16];
    // 0C0 Signature key for this game
    UCHAR SignatureKey[16];
    // 0D0 Signature keys for the alternate title ID's
    UCHAR AlternateSignatureKeys[16][16];
    // 1D0
  }
  XBE_CERTIFICATE, *PXBE_CERTIFICATE;

  // Section headers
  typedef struct _XBE_SECTION
  {
    // 000 Flags
    ULONG Flags;
    // 004 Virtual address (where this section loads in RAM)
    PVOID VirtualAddress;
    // 008 Virtual size (size of section in RAM; after FileSize it's 00'd)
    ULONG VirtualSize;
    // 00C File address (where in the file from which this section comes)
    ULONG FileAddress;
    // 010 File size (size of the section in the XBE file)
    ULONG FileSize;
    // 014 Pointer to section name
    PCSZ SectionName;
    // 018 Section reference count - when >= 1, section is loaded
    LONG SectionReferenceCount;
    // 01C Pointer to head shared page reference count
    WORD *HeadReferenceCount;
    // 020 Pointer to tail shared page reference count
    WORD *TailReferenceCount;
    // 024 SHA hash.  Hash DWORD containing FileSize, then hash section.
    DWORD ShaHash[5];
    // 038
  }
  XBE_SECTION, *PXBE_SECTION;

  // TLS directory information needed later
  // Library version data needed later

  // Initialization flags
#define XBE_INIT_MOUNT_UTILITY          0x00000001
#define XBE_INIT_FORMAT_UTILITY         0x00000002
#define XBE_INIT_64M_RAM_ONLY           0x00000004
#define XBE_INIT_DONT_SETUP_HDD         0x00000008

  // Region codes
#define XBE_REGION_US_CANADA            0x00000001
#define XBE_REGION_JAPAN                0x00000002
#define XBE_REGION_ELSEWHERE            0x00000004
#define XBE_REGION_DEBUG                0x80000000

  // Media types
#define XBE_MEDIA_HDD                   0x00000001
#define XBE_MEDIA_XBOX_DVD              0x00000002
#define XBE_MEDIA_ANY_CD_OR_DVD         0x00000004
#define XBE_MEDIA_CD                    0x00000008
#define XBE_MEDIA_1LAYER_DVDROM         0x00000010
#define XBE_MEDIA_2LAYER_DVDROM         0x00000020
#define XBE_MEDIA_1LAYER_DVDR           0x00000040
#define XBE_MEDIA_2LAYER_DVDR           0x00000080
#define XBE_MEDIA_USB                   0x00000100
#define XBE_MEDIA_ALLOW_UNLOCKED_HDD    0x40000000

  // Section flags
#define XBE_SEC_WRITABLE                0x00000001
#define XBE_SEC_PRELOAD                 0x00000002
#define XBE_SEC_EXECUTABLE              0x00000004
#define XBE_SEC_INSERTED_FILE           0x00000008
#define XBE_SEC_RO_HEAD_PAGE            0x00000010
#define XBE_SEC_RO_TAIL_PAGE            0x00000020

#endif

// ******************************************************************
// * standard typedefs
// ******************************************************************
typedef signed int sint;
typedef unsigned int uint;

typedef char int08;
typedef short int16;
typedef long int32;

typedef unsigned char uint08;
typedef unsigned short uint16;
typedef unsigned long uint32;

typedef signed char sint08;
typedef signed short sint16;
typedef signed long sint32;

typedef struct _xbe_info_
{
#pragma pack( push, before_header )
#pragma pack(1)
  struct header
  {
    uint32 magic;                         // magic number [should be "XBEH"]
    uint08 digsig[256];                   // digital signature
    uint32 base;                          // base address
    uint32 sizeof_headers;                // size of headers
    uint32 sizeof_image;                  // size of image
    uint32 sizeof_image_header;           // size of image header
    uint32 timedate;                      // timedate stamp
    uint32 certificate_addr;              // certificate address
    uint32 sections;                      // number of sections
    uint32 section_headers_addr;          // section headers address

    struct init_flags
    {
uint mount_utility_drive : 1;  // mount utility drive flag
uint format_utility_drive : 1;  // format utility drive flag
uint limit_64mb : 1;  // limit development kit run time memory to 64mb flag
uint dont_setup_harddisk : 1;  // don't setup hard disk flag
uint unused : 4;  // unused (or unknown)
uint unused_b1 : 8;  // unused (or unknown)
uint unused_b2 : 8;  // unused (or unknown)
uint unused_b3 : 8;  // unused (or unknown)
    }
    init_flags;

    uint32 entry;                         // entry point address
    uint32 tls_addr;                      // thread local storage directory address
    uint32 pe_stack_commit;               // size of stack commit
    uint32 pe_heap_reserve;               // size of heap reserve
    uint32 pe_heap_commit;                // size of heap commit
    uint32 pe_base_addr;                  // original base address
    uint32 pe_sizeof_image;               // size of original image
    uint32 pe_checksum;                   // original checksum
    uint32 pe_timedate;                   // original timedate stamp
    uint32 debug_pathname_addr;           // debug pathname address
    uint32 debug_filename_addr;           // debug filename address
    uint32 debug_unicode_filename_addr;   // debug unicode filename address
    uint32 kernel_image_thunk_addr;       // kernel image thunk address
    uint32 nonkernel_import_dir_addr;     // non kernel import directory address
    uint32 library_versions;              // number of library versions
    uint32 library_versions_addr;         // library versions address
    uint32 kernel_library_version_addr;   // kernel library version address
    uint32 xapi_library_version_addr;     // xapi library version address
    uint32 logo_bitmap_addr;              // logo bitmap address
    uint32 logo_bitmap_size;              // logo bitmap size
  }
  Header;

  struct certificate
  {
    uint32 size;                          // size of certificate
    uint32 timedate;                      // timedate stamp
    uint32 titleid;                       // title id
    uint16 title_name[40];                // title name (unicode)
    uint32 alt_title_id[0x10];            // alternate title ids
    uint32 allowed_media;                 // allowed media types
    uint32 game_region;                   // game region
    uint32 game_ratings;                  // game ratings
    uint32 disk_number;                   // disk number
    uint32 version;                       // version
    uint08 lan_key[16];                   // lan key
    uint08 sig_key[16];                   // signature key
    uint08 title_alt_sig_key[16][16];     // alternate signature keys
  }
  Certificate;

  struct section_header
  {
    struct flags                            // flags
    {
uint writable : 1;    // writable flag
uint preload : 1;    // preload flag
uint executable : 1;    // executable flag
uint inserted_file : 1;    // inserted file flag
uint head_page_ro : 1;    // head page read only flag
uint tail_page_ro : 1;    // tail page read only flag
uint unused_a1 : 1;    // unused (or unknown)
uint unused_a2 : 1;    // unused (or unknown)
uint unused_b1 : 8;    // unused (or unknown)
uint unused_b2 : 8;    // unused (or unknown)
uint unused_b3 : 8;    // unused (or unknown)
    }
    Flags;

    uint32 virtual_addr;                  // virtual address
    uint32 virtual_size;                  // virtual size
    uint32 raw_addr;                      // file offset to raw data
    uint32 sizeof_raw;                    // size of raw data
    uint32 section_name_addr;             // section name addr
    uint32 section_reference_count;       // section reference count
    uint16 *head_shared_ref_count_addr;    // head shared page reference count address
    uint16 *tail_shared_ref_count_addr;    // tail shared page reference count address
    uint08 section_digest[20];            // section digest
  }
  * pSection_Header;

  struct library_version
  {
    char name[8];                       // library name
    uint16 major_version;                 // major version
    uint16 minor_version;                 // minor version
    uint16 build_version;                 // build version

    struct flags                            // flags
    {
uint16 qfe_version : 13;   // QFE Version
uint16 approved : 2;    // Approved? (0:no, 1:possibly, 2:yes)
uint16 debug_build : 1;    // Is this a debug build?
    }
    Flags;
  }
  * Library_Version, * Kernel_Version, * XAPI_Version;

  struct tls                                  // thread local storage
  {
    uint32 data_start_addr;               // raw start address
    uint32 data_end_addr;                 // raw end address
    uint32 tls_index_addr;                // tls index  address
    uint32 tls_callback_addr;             // tls callback address
    uint32 sizeof_zero_fill;              // size of zero fill
    uint32 characteristics;               // characteristics
  }
  * TLS;
#pragma pack( pop, before_header )
}
XBE_INFO, * PXBE_INFO, ** PPXBE_INFO;

// XPR stuff
#ifndef XBR_HEADER
typedef struct _XPR_File_Header_
{
  union _ImageType_
  {
    DWORD dwXPRMagic;
    WORD wBitmapMagic;
  } Type;

  DWORD dwTotalSize;
  DWORD dwHeaderSize;

  DWORD dwTextureCommon;
  DWORD dwTextureData;
  DWORD dwTextureLock;
  BYTE btTextureMisc1;
  BYTE btTextureFormat;
BYTE btTextureLevel : 4;
BYTE btTextureWidth : 4;
BYTE btTextureHeight : 4;
BYTE btTextureMisc2 : 4;
  DWORD dwTextureSize;

  DWORD dwEndOfHeader; // 0xFFFFFFFF
}
XPR_FILE_HEADER, * PXPR_FILE_HEADER, ** PPXPR_FILE_HEADER;
#endif

class CXBE
{
public:
  CXBE();
  virtual ~CXBE();
  bool ExtractIcon(const CStdString& strFilename, const CStdString& strIcon);
  uint32 ExtractGameRegion(const CStdString& strFilename);
  static int FilterRegion(int iRegion, bool bForceAllModes=false);
protected:
  XBE_INFO m_XBEInfo;
  int m_iHeaderSize;
  char * m_pHeader;
  int m_iImageSize;
  char * m_pImage;

} ;
