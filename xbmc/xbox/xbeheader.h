#pragma once

// ******************************************************************
// * standard typedefs
// ******************************************************************
typedef signed int     sint;
typedef unsigned int   uint;

typedef char           int08;
typedef short          int16;
typedef long           int32;

typedef unsigned char  uint08;
typedef unsigned short uint16;
typedef unsigned long  uint32;

typedef signed char    sint08;
typedef signed short   sint16;
typedef signed long    sint32;

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
     uint mount_utility_drive    : 1;  // mount utility drive flag
     uint format_utility_drive   : 1;  // format utility drive flag
     uint limit_64mb             : 1;  // limit development kit run time memory to 64mb flag
     uint dont_setup_harddisk    : 1;  // don't setup hard disk flag
     uint unused                 : 4;  // unused (or unknown)
     uint unused_b1              : 8;  // unused (or unknown)
     uint unused_b2              : 8;  // unused (or unknown)
     uint unused_b3              : 8;  // unused (or unknown)
   } init_flags;

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
 } Header;

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
 } Certificate;

 struct section_header
 {
   struct flags                            // flags
   {
     uint writable             : 1;    // writable flag
     uint preload              : 1;    // preload flag
     uint executable           : 1;    // executable flag
     uint inserted_file        : 1;    // inserted file flag
     uint head_page_ro         : 1;    // head page read only flag
     uint tail_page_ro         : 1;    // tail page read only flag
     uint unused_a1            : 1;    // unused (or unknown)
     uint unused_a2            : 1;    // unused (or unknown)
     uint unused_b1            : 8;    // unused (or unknown)
     uint unused_b2            : 8;    // unused (or unknown)
     uint unused_b3            : 8;    // unused (or unknown)
   } Flags;

   uint32  virtual_addr;                  // virtual address
   uint32  virtual_size;                  // virtual size
   uint32  raw_addr;                      // file offset to raw data
   uint32  sizeof_raw;                    // size of raw data
   uint32  section_name_addr;             // section name addr
   uint32  section_reference_count;       // section reference count
   uint16 *head_shared_ref_count_addr;    // head shared page reference count address
   uint16 *tail_shared_ref_count_addr;    // tail shared page reference count address
   uint08  section_digest[20];            // section digest
 } * pSection_Header;

 struct library_version
 {
   char   name[8];                       // library name
   uint16 major_version;                 // major version
   uint16 minor_version;                 // minor version
   uint16 build_version;                 // build version

   struct flags                            // flags
   {
     uint16 qfe_version        : 13;   // QFE Version
     uint16 approved           : 2;    // Approved? (0:no, 1:possibly, 2:yes)
     uint16 debug_build        : 1;    // Is this a debug build?
   } Flags;
 } * Library_Version, * Kernel_Version, * XAPI_Version;

 struct tls                                  // thread local storage
 {
   uint32 data_start_addr;               // raw start address
   uint32 data_end_addr;                 // raw end address
   uint32 tls_index_addr;                // tls index  address
   uint32 tls_callback_addr;             // tls callback address
   uint32 sizeof_zero_fill;              // size of zero fill
   uint32 characteristics;               // characteristics
 } * TLS;
#pragma pack( pop, before_header )
} XBE_INFO, * PXBE_INFO, ** PPXBE_INFO;

// XPR stuff
#ifndef XBR_HEADER
typedef struct _XPR_File_Header_
{
    union _ImageType_
      {
      DWORD     dwXPRMagic;
      WORD      wBitmapMagic;
      } Type;

    DWORD       dwTotalSize;
    DWORD       dwHeaderSize;

    DWORD       dwTextureCommon;
    DWORD       dwTextureData;
    DWORD       dwTextureLock;
    BYTE        btTextureMisc1;
    BYTE        btTextureFormat;
    BYTE        btTextureLevel  : 4;
    BYTE        btTextureWidth  : 4;
    BYTE        btTextureHeight : 4;
    BYTE        btTextureMisc2  : 4;
    DWORD       dwTextureSize;

    DWORD       dwEndOfHeader; // 0xFFFFFFFF
} XPR_FILE_HEADER, * PXPR_FILE_HEADER, ** PPXPR_FILE_HEADER;
#endif

class CXBE
{
public:
  CXBE();
  virtual ~CXBE();
  bool ExtractIcon(const CStdString& strFilename, const CStdString& strIcon);
protected:
  XBE_INFO   m_XBEInfo;
  int        m_iHeaderSize;
  char     * m_pHeader;
  int        m_iImageSize;
  char *     m_pImage;

} ;