#ifndef __COFF_H_
#define __COFF_H_
#pragma once

#include "system.h"

/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

//#pragma message("including coff.h")
//
//      COFF -- Common Object File Format
//          Used commonly by Un*x and is imbedded in Windows PE
//          file format.
//

// These structures must be packed
#pragma pack(1)


/*
 *  Some general purpose MACROs
 */

#define VERSION_MAJOR(x)    ((unsigned int)((x)& 0xff))
#define VERSION_MINOR(x)    ((unsigned int)(((x)>8) &0xff))

#define BIGVERSION_MAJOR(x)    ((unsigned int)((x)& 0xffff))
#define BIGVERSION_MINOR(x)    ((unsigned int)(((x)>16) &0xffff))

/*
 *      COFF File Header (Object & Image)
 *          Spec section 3.3
 */

typedef struct
{
  unsigned short MachineType;            /* magic type               */
  unsigned short NumberOfSections;       /* number of sections       */
  unsigned long TimeDateStamp;          /* time & date stamp        */
  unsigned long PointerToSymbolTable;   /* file pointer to symtab   */
  unsigned long NumberOfSymbols;        /* number of symtab entries */
  unsigned short SizeOfOptionHeader;     /* sizeof(optional hdr)     */
  unsigned short Characteristics;        /* flags                    */
}
COFF_FileHeader_t;

/*
 *      Machine Types
 *          Spec section 3.3.1
 *              (only i386 relevant for us)
 */

#if 1

#ifndef IMAGE_FILE_MACHINE_I386
#define IMAGE_FILE_MACHINE_I386     0x14c
#endif



#define IMAGE_FILE_RELOCS_STRIPPED                  0x0001
#define IMAGE_FILE_EXECUTABLE_IMAGE                 0x0002
#define IMAGE_FILE_LINE_NUMS_STRIPPED               0x0004
#define IMAGE_FILE_LOCAL_SYMS_STRIPPED              0x0008
#define IMAGE_FILE_AGGRESSIVE_WS_TRIM               0x0010
#define IMAGE_FILE_LARGE_ADDRESS_AWARE              0x0020
#define IMAGE_FILE_16BIT_MACHINE                    0x0040
#define IMAGE_FILE_BYTES_REVERSED_LO                0x0080
#define IMAGE_FILE_32BIT_MACHINE                    0x0100
#define IMAGE_FILE_DEBUG_STRIPPED                   0x0200
#define IMAGE_FILE_REMOVABLE_RUN_FROM_SWAP          0x0400
#define IMAGE_FILE_SYSTEM                           0x1000
#define IMAGE_FILE_DLL                              0x2000
#define IMAGE_FILE_UP_SYSTEM_ONLY                   0x4000
#define IMAGE_FILE_BYTES_REVERSED_HI                0x8000

#endif



#define OPTMAGIC_PE32   0x010b
#define OPTMAGIC_PE32P  0x020b

#define OPTHDR_SIZE     28
#define OPTHDR_SIZEP    24
#define WINHDR_SIZE     68
#define WINHDR_SIZEP    88

/*
 *      Optional Header Standard Fields (Image Only)
 *          Spec section 3.4.1
 */

typedef struct
{
  unsigned short Magic;
  unsigned short LinkVersion;
  unsigned long CodeSize;
  unsigned long DataSize;
  unsigned long BssSize;
  unsigned long Entry;
  unsigned long CodeBase;
  unsigned long DataBase;
}
OptionHeader_t;

typedef struct
{
  unsigned short Magic;
  unsigned short LinkVersion;
  unsigned long CodeSize;
  unsigned long DataSize;
  unsigned long BssSize;
  unsigned long Entry;
  unsigned long CodeBase;
}
OptionHeaderPlus_t;

/*
 *      Optional Header Windows NT-Specific Fields (Image Only)
 *          Spec section 3.4.2
 */

typedef struct
{
  unsigned long ImageBase;
  unsigned long SectionAlignment;
  unsigned long FileAlignment;
  unsigned long OSVer;
  unsigned long ImgVer;
  unsigned long SubSysVer;
  unsigned long Reserved;
  unsigned long SizeOfImage;
  unsigned long SizeOfHeaders;
  unsigned long CheckSum;
  unsigned short Subsystem;
  unsigned short DLLFlags;
  unsigned long SizeOfStackReserve;
  unsigned long SizeOfStackCommit;
  unsigned long SizeOfHeapReserve;
  unsigned long SizeOfHeapCommit;
  unsigned long LoaderFlags;
  unsigned long NumDirectories;
}
WindowsHeader_t;

typedef struct
{
  unsigned __int64 ImageBase;
  unsigned long SectionAlignment;
  unsigned long FileAlignment;
  unsigned long OSVer;
  unsigned long ImgVer;
  unsigned long SubSysVer;
  unsigned long Reserved;
  unsigned long SizeOfImage;
  unsigned long SizeOfHeaders;
  unsigned long CheckSum;
  unsigned short Subsystem;
  unsigned short DLLFlags;
  unsigned __int64 SizeOfStackReserve;
  unsigned __int64 SizeOfStackCommit;
  unsigned __int64 SizeOfHeapReserve;
  unsigned __int64 SizeOfHeapCommit;
  unsigned long LoaderFlags;
  unsigned long NumDirectories;
}
WindowsHeaderPlus_t;

/*
#define IMAGE_SUBSYSTEM_UNKNOWN                     0
#define IMAGE_SUBSYSTEM_NATIVE                      1
#define IMAGE_SUBSYSTEM_WINDOWS_GUI                 2
#define IMAGE_SUBSYSTEM_WINDOWS_CUI                 3
#define IMAGE_SUBSYSTEM_POSIX_CUI                   7
#define IMAGE_SUBSYSTEM_WINDOWS_CE_GUI              9
#define IMAGE_SUBSYSTEM_EFI_APPLICATION             10
#define IMAGE_SUBSYSTEM_EFI_BOOT_SERVICE_DRIVER     11
#define IMAGE_SUBSYSTEM_EFI_RUNTIME_DRIVER          12

#define IMAGE_DLLCHARACTERISTICS_NO_BIND            0x0800
#define IMAGE_DLLCHARACTERISTICS_WDM_DRIVER         0x2000
#define IMAGE_DLLCHARACTERISTICS_TERMINAL_SERVER_AWARE  0X8000
*/

/*
 *      Optional Header Data Directories (Image Only)
 *          Spec section 3.4.3
 */

typedef struct
{
  unsigned long RVA;
  unsigned long Size;
}
Image_Data_Directory_t;

enum Directory_Items {
  EXPORT_TABLE = 0,
  IMPORT_TABLE,
  RESOURCE_TABLE,
  EXCEPTION_TABLE,
  CERTIFICATE_TABLE,
  BASE_RELOCATION_TABLE,
  DEBUG_,
  ARCHITECTURE,
  GLOBAL_PTR,
  TLS_TABLE,
  LOAD_CONFIG_TABLE,
  BOUND_IMPORT,
  IAT,
  DELAY_IMPORT_DESCRIPTOR,
  COM_RUNTIME_HEADER,
  RESERVED
};

/*
 *      Section Table (Section Headers)
 *          Spec section 4.
 */


typedef struct
{
  unsigned char Name[8];
  unsigned long VirtualSize;
  unsigned long VirtualAddress;
  unsigned long SizeOfRawData;
  unsigned long PtrToRawData;
  unsigned long PtrToRelocations;
  unsigned long PtrToLineNums;
  unsigned short NumRelocations;
  unsigned short NumLineNumbers;
  unsigned long Characteristics;
}
SectionHeader_t;

/*
 *      Section Flags (Characteristics)
 *          Spec section 4.1
 */

#define IMAGE_SCN_CNT_CODE          0x00000020
#define IMAGE_SCN_CNT_DATA          0x00000040
#define IMAGE_SCN_CNT_BSS           0x00000080
#define IMAGE_SCN_LNK_INFO          0x00000200
#define IMAGE_SCN_LNK_REMOVE        0x00000800
#define IMAGE_SCN_LNK_COMDAT        0x00001000
#define IMAGE_SCN_ALIGN_1BYTES      0x00100000
#define IMAGE_SCN_ALIGN_2BYTES      0x00200000
#define IMAGE_SCN_ALIGN_4BYTES      0x00300000
#define IMAGE_SCN_ALIGN_8BYTES      0x00400000
#define IMAGE_SCN_ALIGN_16BYTES     0x00500000
#define IMAGE_SCN_ALIGN_32BYTES     0x00600000
#define IMAGE_SCN_ALIGN_64BYTES     0x00700000
#define IMAGE_SCN_ALIGN_128BYTES    0x00800000
#define IMAGE_SCN_ALIGN_256BYTES    0x00900000
#define IMAGE_SCN_ALIGN_512BYTES    0x00A00000
#define IMAGE_SCN_ALIGN_1024BYTES   0x00B00000
#define IMAGE_SCN_ALIGN_2048BYTES   0x00C00000
#define IMAGE_SCN_ALIGN_4096BYTES   0x00D00000
#define IMAGE_SCN_ALIGN_8192BYTES   0x00E00000
#define IMAGE_SCN_ALIGN_MASK        0x00F00000
#define IMAGE_SCN_LNK_NRELOC_OVFL   0x01000000
#define IMAGE_SCN_MEM_DISCARDABLE   0x02000000
#define IMAGE_SCN_MEM_NOT_CACHED    0x04000000
#define IMAGE_SCN_MEM_NOT_PAGED     0x08000000
#define IMAGE_SCN_MEM_SHARED        0x10000000
#define IMAGE_SCN_MEM_EXECUTE       0x20000000
#define IMAGE_SCN_MEM_READ          0x40000000
#define IMAGE_SCN_MEM_WRITE         0x80000000

/*
 *      COFF Relocations (Object Only)
 *          Spec section 5.2
 */

typedef struct
{
  unsigned long VirtualAddress;
  unsigned long SymTableIndex;
  unsigned short Type;
}
ObjReloc_t;

/*
 *      COFF Relocation Type Indicators
 *          Spec section 5.2.1
 */

#define IMAGE_REL_I386_ABSOLUTE     0x0000
#define IMAGE_REL_I386_DIR16        0x0001
#define IMAGE_REL_I386_REL16        0x0002
#define IMAGE_REL_I386_DIR32        0x0006
#define IMAGE_REL_I386_DIR32NB      0x0007
#define IMAGE_REL_I386_SEG12        0x0009
#define IMAGE_REL_I386_SECTION      0x000A
#define IMAGE_REL_I386_SECREL       0x000B
#define IMAGE_REL_I386_REL32        0x0014

/*
 *      COFF Line Numbers
 *          Spec section 5.3
 */

typedef struct
{
  union {
    unsigned long SymbolTableIndex;
    unsigned long VirtualAddress;
  } Type;
  unsigned short LineNum;
}
LineNumbers_t;

/*
 *      COFF Symbol Table
 *          Spec section 5.4
 */

typedef struct
{
  union {
    unsigned char ShortName[8];
    unsigned __int64 Offset;
  } Name;
  unsigned long Value;
  unsigned short SectionNumber;
  unsigned short Type;
  unsigned char StorageClass;
  unsigned char NumberOfAuxSymbols;
}
SymbolTable_t;

#if !defined(_WIN32)

#define IMAGE_SYM_UNDEFINED     0
#define IMAGE_SYM_ABSOLUTE      0xFFFF
#define IMAGE_SYM_DEBUG         0xFFFE


#define IMAGE_SYM_TYPE_NULL         0
#define IMAGE_SYM_TYPE_VOID         1
#define IMAGE_SYM_TYPE_CHAR         2
#define IMAGE_SYM_TYPE_SHORT        3
#define IMAGE_SYM_TYPE_INT          4
#define IMAGE_SYM_TYPE_LONG         5
#define IMAGE_SYM_TYPE_FLOAT        6
#define IMAGE_SYM_TYPE_DOUBLE       7
#define IMAGE_SYM_TYPE_STRUCT       8
#define IMAGE_SYM_TYPE_UNION        9
#define IMAGE_SYM_TYPE_ENUM         10
#define IMAGE_SYM_TYPE_MOE          11
#define IMAGE_SYM_TYPE_BYTE         12
#define IMAGE_SYM_TYPE_WORD         13
#define IMAGE_SYM_TYPE_UINT         14
#define IMAGE_SYM_TYPE_DWORD        15

#define IMAGE_SYM_DWORD_NULL        0
#define IMAGE_SYM_DWORD_POINTER     1
#define IMAGE_SYM_DWORD_FUNCTION    2
#define IMAGE_SYM_DWORD_ARRAY       3


#define IMAGE_SYM_CLASS_END_OF_FUNCTION         0xFF
#define IMAGE_SYM_CLASS_NULL                    0
#define IMAGE_SYM_CLASS_AUTOMATIC               1
#define IMAGE_SYM_CLASS_EXTERNAL                2
#define IMAGE_SYM_CLASS_STATIC                  3
#define IMAGE_SYM_CLASS_REGISTER                4
#define IMAGE_SYM_CLASS_EXTERNAL_DEF            5
#define IMAGE_SYM_CLASS_LABEL                   6
#define IMAGE_SYM_CLASS_UNDEFINED_LABEL         7
#define IMAGE_SYM_CLASS_MEMBER_OF_STRUCT        8
#define IMAGE_SYM_CLASS_ARGUMENT                9
#define IMAGE_SYM_CLASS_STRUCT_TAG              10
#define IMAGE_SYM_CLASS_MEMBER_OF_UNION         11
#define IMAGE_SYM_CLASS_UNION_TAG               12
#define IMAGE_SYM_CLASS_TYPE_DEFINITION         13
#define IMAGE_SYM_CLASS_UNDEFINED_STATIC        14
#define IMAGE_SYM_CLASS_ENUM_TAG                15
#define IMAGE_SYM_CLASS_MEMBER_OF_ENUM          16
#define IMAGE_SYM_CLASS_REGISTER_PARAM          17
#define IMAGE_SYM_CLASS_BIT_FIELD               18
#define IMAGE_SYM_CLASS_BLOCK                   100
#define IMAGE_SYM_CLASS_FUNCTION                101
#define IMAGE_SYM_CLASS_END_OF_STRUCT           102
#define IMAGE_SYM_CLASS_FILE                    103
#define IMAGE_SYM_CLASS_SECTION                 104
#define IMAGE_SYM_CLASS_WEAK_EXTERNAL           105
#endif

typedef struct
{
  unsigned long TagIndex;
  unsigned long TotalSize;
  unsigned long PtrToLineNumber;
  unsigned long PtrToNextFunc;
  unsigned short unused;
}
AuxFuncDef_t;

/*
 *      Symbol Auxiliary Record: .bf and .ef
 *          Spec section 5.5.2
 */

typedef struct
{
  unsigned long unused;
  unsigned short LineNumber;
  unsigned long unused1;
  unsigned short unused2;
  unsigned long PtrToNextFunc;
  unsigned char unused3;
}
AuxBfEf_t;

/*
 *      Export Section (Directory)
 *          Spec section 6.3
 */

/*
 *      Export Directory Table
 *          Spec section 6.3.1
 */

typedef struct
{
  unsigned long ExportFlags;
  unsigned long TimeStamp;
  unsigned short MajorVersion;
  unsigned short MinorVersion;
  unsigned long Name_RVA;
  unsigned long OrdinalBase;
  unsigned long NumAddrTable;
  unsigned long NumNamePtrs;
  unsigned long ExportAddressTable_RVA;
  unsigned long NamePointerTable_RVA;
  unsigned long OrdinalTable_RVA;
}
ExportDirTable_t;


/*
 *      Import Section (Directory)
 *          Spec section 6.4
 */

/*
 *      Import Directory Table
 *          Spec Section 6.4.1
 */

typedef struct
{
  unsigned long ImportLookupTable_RVA;
  unsigned long TimeStamp;
  unsigned long ForwarderChain;
  unsigned long Name_RVA;
  unsigned long ImportAddressTable_RVA;
}
ImportDirTable_t;

/*
 *      .reloc Relocation types
 *          spec section 6.6
 */

#if 1
#define IMAGE_REL_BASED_ABSOLUTE        0
#define IMAGE_REL_BASED_HIGH            1
#define IMAGE_REL_BASED_LOW             2
#define IMAGE_REL_BASED_HIGHLOW         3
#define IMAGE_REL_BASED_HIGHADJ         4
#define IMAGE_REL_BASED_MIPS_JMPADDR    5
#define IMAGE_REL_BASED_SECTION         6
#define IMAGE_REL_BASED_REL32           7
#define IMAGE_REL_BASED_MIPS_JMPADDR16  9
#define IMAGE_REL_BASED_DIR64           10
#define IMAGE_REL_BASED_HIGH3ADJ        11
#endif




#pragma pack()

#endif // __COFF_H_
