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

#include <stdlib.h>
#include <string.h>
#include "coff.h"
#include "coffldr.h"

//#define DUMPING_DATA 1

#ifndef __GNUC__
#pragma warning (disable:4806)
#endif

#include "utils/log.h"
#define printf(format, ...) CLog::Log(LOGDEBUG, format , ##__VA_ARGS__)

const char *DATA_DIR_NAME[16] =
  {
    "Export Table",
    "Import Table",
    "Resource Table",
    "Exception Table",
    "Certificate Table",
    "Base Relocation Table",
    "Debug",
    "Architecture",
    "Global Ptr",
    "TLS Table",
    "Load Config Table",
    "Bound Import",
    "IAT",
    "Delay Import Descriptor",
    "COM+ Runtime Header",
    "Reserved"
  };


CoffLoader::CoffLoader()
{
  CoffFileHeader = 0;
  OptionHeader = 0;
  WindowsHeader = 0;
  Directory = 0;
  SectionHeader = 0;
  SymTable = 0;
  StringTable = 0;
  SectionData = 0;

  NumberOfSymbols = 0;
  SizeOfStringTable = 0;
  NumOfDirectories = 0;
  NumOfSections = 0;
  FileHeaderOffset = 0;
  hModule = NULL;
}

CoffLoader::~CoffLoader()
{
  if ( hModule )
  {
#ifdef _LINUX
    free(hModule);
#else
    VirtualFree(hModule, 0, MEM_RELEASE);
#endif
    hModule = NULL;
  }
  if ( SymTable )
  {
    delete [] SymTable;
    SymTable = 0;
  }
  if ( StringTable )
  {
    delete [] StringTable;
    StringTable = 0;
  }
  if ( SectionData )
  {
    delete [] SectionData;
    SectionData = 0;
  }
}

// Has nothing to do with the coff loader itself
// it can be used to parse the headers of a dll
// already loaded into memory
int CoffLoader::ParseHeaders(void* hModule)
{
  if (strncmp((char*)hModule, "MZ", 2) != 0)
    return 0;

  int* Offset = (int*)((char*)hModule+0x3c);
  if (*Offset <= 0)
    return 0;

  if (strncmp((char*)hModule+*Offset, "PE\0\0", 4) != 0)
    return 0;

  FileHeaderOffset = *Offset + 4;

  CoffFileHeader = (COFF_FileHeader_t *) ( (char*)hModule + FileHeaderOffset );
  NumOfSections = CoffFileHeader->NumberOfSections;

  OptionHeader = (OptionHeader_t *) ( (char*)CoffFileHeader + sizeof(COFF_FileHeader_t) );
  WindowsHeader = (WindowsHeader_t *) ( (char*)OptionHeader + OPTHDR_SIZE );
  EntryAddress = OptionHeader->Entry;
  NumOfDirectories = WindowsHeader->NumDirectories;

  Directory = (Image_Data_Directory_t *) ( (char*)WindowsHeader + WINHDR_SIZE);
  SectionHeader = (SectionHeader_t *) ( (char*)Directory + sizeof(Image_Data_Directory_t) * NumOfDirectories);

  if (CoffFileHeader->MachineType != IMAGE_FILE_MACHINE_I386)
    return 0;

#ifdef DUMPING_DATA
  PrintFileHeader(CoffFileHeader);
#endif

  if ( CoffFileHeader->SizeOfOptionHeader == 0 ) //not an image file, object file maybe
    return 0;

  // process Option Header
  if (OptionHeader->Magic == OPTMAGIC_PE32P)
  {
    printf("PE32+ not supported\n");
    return 0;
  }
  else if (OptionHeader->Magic == OPTMAGIC_PE32)
  {

#ifdef DUMPING_DATA
    PrintOptionHeader(OptionHeader);
    PrintWindowsHeader(WindowsHeader);
#endif

  }
  else
  {
    //add error message
    return 0;
  }

#ifdef DUMPING_DATA
  for (int DirCount = 0; DirCount < NumOfDirectories; DirCount++)
  {
    printf("Data Directory %02d: %s\n", DirCount + 1, DATA_DIR_NAME[DirCount]);
    printf("                    RVA:  %08X\n", Directory[DirCount].RVA);
    printf("                    Size: %08X\n\n", Directory[DirCount].Size);
  }
#endif

  return 1;

}

int CoffLoader::LoadCoffHModule(FILE *fp)
{
  //test file signatures
  char Sig[4];
  rewind(fp);
  memset(Sig, 0, sizeof(Sig));
  if (!fread(Sig, 1, 2, fp) || strncmp(Sig, "MZ", 2) != 0)
    return 0;

  if (fseek(fp, 0x3c, SEEK_SET) != 0)
    return 0;
  
  int Offset = 0;
  if (!fread(&Offset, sizeof(int), 1, fp) || (Offset <= 0))
    return 0;

  if (fseek(fp, Offset, SEEK_SET) != 0)
    return 0;

  memset(Sig, 0, sizeof(Sig));
  if (!fread(Sig, 1, 4, fp) || strncmp(Sig, "PE\0\0", 4) != 0)
    return 0;

  Offset += 4;
  FileHeaderOffset = Offset;

  // Load and process Header
  if (fseek(fp, FileHeaderOffset + sizeof(COFF_FileHeader_t) + OPTHDR_SIZE, SEEK_SET)) //skip to winows headers
    return 0;

  WindowsHeader_t tempWindowsHeader;
  size_t readcount = fread(&tempWindowsHeader, 1, WINHDR_SIZE, fp);
  if (readcount != WINHDR_SIZE) //test file size error
    return 0;

  // alloc aligned memory
#ifdef _LINUX
  hModule = malloc(tempWindowsHeader.SizeOfImage);
#else
  hModule = VirtualAllocEx(0, (PVOID)tempWindowsHeader.ImageBase, tempWindowsHeader.SizeOfImage, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
  if (hModule == NULL)
    hModule = VirtualAlloc(0, tempWindowsHeader.SizeOfImage, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
#endif
  if (hModule == NULL)
    return 0;   //memory allocation fails

  rewind(fp);
  readcount = fread(hModule, 1, tempWindowsHeader.SizeOfHeaders, fp);
  if (readcount != tempWindowsHeader.SizeOfHeaders)   //file size error
    return 0;

  CoffFileHeader = (COFF_FileHeader_t *) ( (char*)hModule + FileHeaderOffset );
  NumOfSections = CoffFileHeader->NumberOfSections;

  OptionHeader = (OptionHeader_t *) ( (char*)CoffFileHeader + sizeof(COFF_FileHeader_t) );
  WindowsHeader = (WindowsHeader_t *) ( (char*)OptionHeader + OPTHDR_SIZE );
  EntryAddress = OptionHeader->Entry;
  NumOfDirectories = WindowsHeader->NumDirectories;

  Directory = (Image_Data_Directory_t *) ( (char*)WindowsHeader + WINHDR_SIZE);
  SectionHeader = (SectionHeader_t *) ( (char*)Directory + sizeof(Image_Data_Directory_t) * NumOfDirectories);

  if (CoffFileHeader->MachineType != IMAGE_FILE_MACHINE_I386)
    return 0;

#ifdef DUMPING_DATA
  PrintFileHeader(CoffFileHeader);
#endif

  if ( CoffFileHeader->SizeOfOptionHeader == 0 ) //not an image file, object file maybe
    return 0;

  // process Option Header
  if (OptionHeader->Magic == OPTMAGIC_PE32P)
  {
    printf("PE32+ not supported\n");
    return 0;
  }
  else if (OptionHeader->Magic == OPTMAGIC_PE32)
  {

#ifdef DUMPING_DATA
    PrintOptionHeader(OptionHeader);
    PrintWindowsHeader(WindowsHeader);
#endif

  }
  else
  {
    //add error message
    return 0;
  }

#ifdef DUMPING_DATA
  for (int DirCount = 0; DirCount < NumOfDirectories; DirCount++)
  {
    printf("Data Directory %02d: %s\n", DirCount + 1, DATA_DIR_NAME[DirCount]);
    printf("                    RVA:  %08X\n", Directory[DirCount].RVA);
    printf("                    Size: %08X\n\n", Directory[DirCount].Size);
  }
#endif

  return 1;

}

int CoffLoader::LoadSymTable(FILE *fp)
{
  int Offset = ftell(fp);
  if (Offset < 0)
    return 0;

  if ( CoffFileHeader->PointerToSymbolTable == 0 )
    return 1;

  if (fseek(fp, CoffFileHeader->PointerToSymbolTable /* + CoffBeginOffset*/, SEEK_SET) != 0)
    return 0;

  SymbolTable_t *tmp = new SymbolTable_t[CoffFileHeader->NumberOfSymbols];
  if (!tmp)
  {
    printf("Could not allocate memory for symbol table!\n");
    return 0;
  }
  if (!fread((void *)tmp, CoffFileHeader->NumberOfSymbols, sizeof(SymbolTable_t), fp))
  {
    delete[] tmp;
    return 0;
  }
  NumberOfSymbols = CoffFileHeader->NumberOfSymbols;
  SymTable = tmp;
  if (fseek(fp, Offset, SEEK_SET) != 0)
    return 0;
  return 1;
}

int CoffLoader::LoadStringTable(FILE *fp)
{
  int StringTableSize;
  char *tmp = NULL;
  
  int Offset = ftell(fp);
  if (Offset < 0)
    return 0;

  if ( CoffFileHeader->PointerToSymbolTable == 0 )
    return 1;

  if (fseek(fp, CoffFileHeader->PointerToSymbolTable +
        CoffFileHeader->NumberOfSymbols * sizeof(SymbolTable_t),
        SEEK_SET) != 0)
    return 0;

  if (!fread(&StringTableSize, 1, sizeof(int), fp))
    return 0;
  StringTableSize -= 4;
  if (StringTableSize != 0)
  {
    tmp = new char[StringTableSize];
    if (tmp == NULL)
    {
      printf("Could not allocate memory for string table\n");
      return 0;
    }
    if (!fread((void *)tmp, StringTableSize, sizeof(char), fp))
    {
      delete[] tmp;
      return 0;
    }
  }
  SizeOfStringTable = StringTableSize;
  StringTable = tmp;
  if (fseek(fp, Offset, SEEK_SET) != 0)
    return 0;
  return 1;
}

int CoffLoader::LoadSections(FILE *fp)
{
  NumOfSections = CoffFileHeader->NumberOfSections;

  SectionData = new char * [NumOfSections];
  if ( !SectionData )
    return 0;

  // Bobbin007: for debug dlls this check always fails

  //////check VMA size!!!!!
  //unsigned long vma_size = 0;
  //for (int SctnCnt = 0; SctnCnt < NumOfSections; SctnCnt++)
  //{
  //  SectionHeader_t *ScnHdr = (SectionHeader_t *)(SectionHeader + SctnCnt);
  //  vma_size = max(vma_size, ScnHdr->VirtualAddress + ScnHdr->SizeOfRawData);
  //  vma_size = max(vma_size, ScnHdr->VirtualAddress + ScnHdr->VirtualSize);
  //}

  //if (WindowsHeader->SizeOfImage < vma_size)
  //  return 0;   //something wrong with file

  for (int SctnCnt = 0; SctnCnt < NumOfSections; SctnCnt++)
  {
    SectionHeader_t *ScnHdr = (SectionHeader_t *)(SectionHeader + SctnCnt);
    SectionData[SctnCnt] = ((char*)hModule + ScnHdr->VirtualAddress);

    if (fseek(fp, ScnHdr->PtrToRawData, SEEK_SET) != 0)
      return 0;

    if (!fread(SectionData[SctnCnt], 1, ScnHdr->SizeOfRawData, fp))
      return 0;

#ifdef DUMPING_DATA
    //debug blocks
    char szBuf[128];
    char namebuf[9];
    for (int i = 0; i < 8; i++)
      namebuf[i] = ScnHdr->Name[i];
    namebuf[8] = '\0';
    sprintf(szBuf, "Load code Sections %s Memory %p,Length %x\n", namebuf,
            SectionData[SctnCnt], max(ScnHdr->VirtualSize, ScnHdr->SizeOfRawData));
    OutputDebugString(szBuf);
#endif

    if ( ScnHdr->SizeOfRawData < ScnHdr->VirtualSize )  //initialize BSS data in the end of section
    {
      memset((char*)((long)(SectionData[SctnCnt]) + ScnHdr->SizeOfRawData), 0, ScnHdr->VirtualSize - ScnHdr->SizeOfRawData);
    }

    if (ScnHdr->Characteristics & IMAGE_SCN_CNT_BSS)  //initialize whole .BSS section, pure .BSS is obsolete
    {
      memset(SectionData[SctnCnt], 0, ScnHdr->VirtualSize);
    }

#ifdef DUMPING_DATA
    PrintSection(SectionHeader + SctnCnt, SectionData[SctnCnt]);
#endif

  }
  return 1;
}

//FIXME:  Add the Free Resources functions

int CoffLoader::RVA2Section(unsigned long RVA)
{
  NumOfSections = CoffFileHeader->NumberOfSections;
  for ( int i = 0; i < NumOfSections; i++)
  {
    if ( SectionHeader[i].VirtualAddress <= RVA )
    {
      if ( i + 1 != NumOfSections )
      {
        if ( RVA < SectionHeader[i + 1].VirtualAddress )
        {
          if ( SectionHeader[i].VirtualAddress + SectionHeader[i].VirtualSize <= RVA )
            printf("Warning! Address outside of Section: %lx!\n", RVA);
          //                    else
          return i;
        }
      }
      else
      {
        if ( SectionHeader[i].VirtualAddress + SectionHeader[i].VirtualSize <= RVA )
          printf("Warning! Address outside of Section: %lx!\n", RVA);
        //                else
        return i;
      }
    }
  }
  printf("RVA2Section lookup failure!\n");
  return 0;
}

void* CoffLoader::RVA2Data(unsigned long RVA)
{
  int Sctn = RVA2Section(RVA);

  if( RVA < SectionHeader[Sctn].VirtualAddress
   || RVA >= SectionHeader[Sctn].VirtualAddress + SectionHeader[Sctn].VirtualSize)
  {
    // RVA2Section is lying, let's use base address of dll instead, only works if
    // DLL has been loaded fully into memory, wich we normally do
    return (void*)(RVA + (unsigned long)hModule);
  }
  return SectionData[Sctn] + RVA - SectionHeader[Sctn].VirtualAddress;
}

unsigned long CoffLoader::Data2RVA(void* address)
{
  for ( int i = 0; i < CoffFileHeader->NumberOfSections; i++)
  {
    if(address >= SectionData[i] && address < SectionData[i] + SectionHeader[i].VirtualSize)
      return (unsigned long)address - (unsigned long)SectionData[i] + SectionHeader[i].VirtualAddress;
  }

  // Section wasn't found, so use relative to main load of dll
  return (unsigned long)address - (unsigned long)hModule;
}

char *CoffLoader::GetStringTblIndex(int index)
{
  char *table = StringTable;

  while (index--)
    table += strlen(table) + 1;
  return table;
}

char *CoffLoader::GetStringTblOff(int Offset)
{
  return StringTable + Offset - 4;
}

char *CoffLoader::GetSymbolName(SymbolTable_t *sym)
{
  static char shortname[9];
  __int64 index = sym->Name.Offset;
  int low = (int)(index & 0xFFFFFFFF);
  int high = (int)((index >> 32) & 0xFFFFFFFF);

  if (low == 0)
  {
    return GetStringTblOff(high);
  }
  else
  {
    memset(shortname, 0, 9);
    strncpy(shortname, (char *)sym->Name.ShortName, 8);
    return shortname;
  }
}

char *CoffLoader::GetSymbolName(int index)
{
  SymbolTable_t *sym = &(SymTable[index]);
  return GetSymbolName(sym);
}

void CoffLoader::PrintStringTable(void)
{
  int size = SizeOfStringTable;
  int index = 0;
  char *table = StringTable;

  printf("\nSTRING TABLE\n");
  while (size)
  {
    printf("%2d: %s\n", index++, table);
    size -= strlen(table) + 1;
    table += strlen(table) + 1;
  }
  printf("\n");
}


void CoffLoader::PrintSymbolTable(void)
{
  int SymIndex;

  printf("COFF SYMBOL TABLE\n");
  for (SymIndex = 0; SymIndex < NumberOfSymbols; SymIndex++)
  {
    printf("%03X ", SymIndex);
    printf("%08lX ", SymTable[SymIndex].Value);

    if (SymTable[SymIndex].SectionNumber == IMAGE_SYM_ABSOLUTE)
      printf("ABS     ");
    else if (SymTable[SymIndex].SectionNumber == IMAGE_SYM_DEBUG)
      printf("DEBUG   ");
    else if (SymTable[SymIndex].SectionNumber == IMAGE_SYM_UNDEFINED)
      printf("UNDEF   ");
    else
    {
      printf("SECT%d ", SymTable[SymIndex].SectionNumber);
      if (SymTable[SymIndex].SectionNumber < 10)
        printf(" ");
      if (SymTable[SymIndex].SectionNumber < 100)
        printf(" ");
    }

    if (SymTable[SymIndex].Type == 0)
      printf("notype       ");
    else
    {
      printf("%X         ", SymTable[SymIndex].Type);
      if (SymTable[SymIndex].Type < 0x10)
        printf(" ");
      if (SymTable[SymIndex].Type < 0x100)
        printf(" ");
      if (SymTable[SymIndex].Type < 0x1000)
        printf(" ");
    }

    if (SymTable[SymIndex].StorageClass == IMAGE_SYM_CLASS_END_OF_FUNCTION)
      printf("End of Function   ");
    if (SymTable[SymIndex].StorageClass == IMAGE_SYM_CLASS_NULL)
      printf("Null              ");
    if (SymTable[SymIndex].StorageClass == IMAGE_SYM_CLASS_AUTOMATIC)
      printf("Automatic         ");
    if (SymTable[SymIndex].StorageClass == IMAGE_SYM_CLASS_EXTERNAL)
      printf("External          ");
    if (SymTable[SymIndex].StorageClass == IMAGE_SYM_CLASS_STATIC)
      printf("Static            ");
    if (SymTable[SymIndex].StorageClass == IMAGE_SYM_CLASS_REGISTER)
      printf("Register          ");
    if (SymTable[SymIndex].StorageClass == IMAGE_SYM_CLASS_EXTERNAL_DEF)
      printf("External Def      ");
    if (SymTable[SymIndex].StorageClass == IMAGE_SYM_CLASS_LABEL)
      printf("Label             ");
    if (SymTable[SymIndex].StorageClass == IMAGE_SYM_CLASS_UNDEFINED_LABEL)
      printf("Undefined Label   ");
    if (SymTable[SymIndex].StorageClass == IMAGE_SYM_CLASS_MEMBER_OF_STRUCT)
      printf("Member Of Struct  ");
    if (SymTable[SymIndex].StorageClass == IMAGE_SYM_CLASS_ARGUMENT)
      printf("Argument          ");
    if (SymTable[SymIndex].StorageClass == IMAGE_SYM_CLASS_STRUCT_TAG)
      printf("Struct Tag        ");
    if (SymTable[SymIndex].StorageClass == IMAGE_SYM_CLASS_MEMBER_OF_UNION)
      printf("Member Of Union   ");
    if (SymTable[SymIndex].StorageClass == IMAGE_SYM_CLASS_UNION_TAG)
      printf("Union Tag         ");
    if (SymTable[SymIndex].StorageClass == IMAGE_SYM_CLASS_TYPE_DEFINITION)
      printf("Type Definition  ");
    if (SymTable[SymIndex].StorageClass == IMAGE_SYM_CLASS_UNDEFINED_STATIC)
      printf("Undefined Static  ");
    if (SymTable[SymIndex].StorageClass == IMAGE_SYM_CLASS_ENUM_TAG)
      printf("Enum Tag          ");
    if (SymTable[SymIndex].StorageClass == IMAGE_SYM_CLASS_MEMBER_OF_ENUM)
      printf("Member Of Enum    ");
    if (SymTable[SymIndex].StorageClass == IMAGE_SYM_CLASS_REGISTER_PARAM)
      printf("Register Param    ");
    if (SymTable[SymIndex].StorageClass == IMAGE_SYM_CLASS_BIT_FIELD)
      printf("Bit Field         ");
    if (SymTable[SymIndex].StorageClass == IMAGE_SYM_CLASS_BLOCK)
      printf("Block             ");
    if (SymTable[SymIndex].StorageClass == IMAGE_SYM_CLASS_FUNCTION)
      printf("Function          ");
    if (SymTable[SymIndex].StorageClass == IMAGE_SYM_CLASS_END_OF_STRUCT)
      printf("End Of Struct     ");
    if (SymTable[SymIndex].StorageClass == IMAGE_SYM_CLASS_FILE)
      printf("File              ");
    if (SymTable[SymIndex].StorageClass == IMAGE_SYM_CLASS_SECTION)
      printf("Section           ");
    if (SymTable[SymIndex].StorageClass == IMAGE_SYM_CLASS_WEAK_EXTERNAL)
      printf("Weak External     ");

    printf("| %s", GetSymbolName(SymIndex));

    SymIndex += SymTable[SymIndex].NumberOfAuxSymbols;
    printf("\n");
  }
  printf("\n");

}

void CoffLoader::PrintFileHeader(COFF_FileHeader_t *FileHeader)
{
  printf("COFF Header\n");
  printf("------------------------------------------\n\n");

  printf("MachineType:            0x%04X\n", FileHeader->MachineType);
  printf("NumberOfSections:       0x%04X\n", FileHeader->NumberOfSections);
  printf("TimeDateStamp:          0x%08lX\n",FileHeader->TimeDateStamp);
  printf("PointerToSymbolTable:   0x%08lX\n",FileHeader->PointerToSymbolTable);
  printf("NumberOfSymbols:        0x%08lX\n",FileHeader->NumberOfSymbols);
  printf("SizeOfOptionHeader:     0x%04X\n", FileHeader->SizeOfOptionHeader);
  printf("Characteristics:        0x%04X\n", FileHeader->Characteristics);

  if (FileHeader->Characteristics & IMAGE_FILE_RELOCS_STRIPPED)
    printf("                        IMAGE_FILE_RELOCS_STRIPPED\n");

  if (FileHeader->Characteristics & IMAGE_FILE_EXECUTABLE_IMAGE)
    printf("                        IMAGE_FILE_EXECUTABLE_IMAGE\n");

  if (FileHeader->Characteristics & IMAGE_FILE_LINE_NUMS_STRIPPED)
    printf("                        IMAGE_FILE_LINE_NUMS_STRIPPED\n");

  if (FileHeader->Characteristics & IMAGE_FILE_LOCAL_SYMS_STRIPPED)
    printf("                        IMAGE_FILE_LOCAL_SYMS_STRIPPED\n");

  if (FileHeader->Characteristics & IMAGE_FILE_AGGRESSIVE_WS_TRIM)
    printf("                        IMAGE_FILE_AGGRESSIVE_WS_TRIM\n");

  if (FileHeader->Characteristics & IMAGE_FILE_LARGE_ADDRESS_AWARE)
    printf("                        IMAGE_FILE_LARGE_ADDRESS_AWARE\n");

  if (FileHeader->Characteristics & IMAGE_FILE_16BIT_MACHINE)
    printf("                        IMAGE_FILE_16BIT_MACHINE\n");

  if (FileHeader->Characteristics & IMAGE_FILE_BYTES_REVERSED_LO)
    printf("                        IMAGE_FILE_BYTES_REVERSED_LO\n");

  if (FileHeader->Characteristics & IMAGE_FILE_32BIT_MACHINE)
    printf("                        IMAGE_FILE_32BIT_MACHINE\n");

  if (FileHeader->Characteristics & IMAGE_FILE_DEBUG_STRIPPED)
    printf("                        IMAGE_FILE_DEBUG_STRIPPED\n");

  if (FileHeader->Characteristics & IMAGE_FILE_REMOVABLE_RUN_FROM_SWAP)
    printf("                        IMAGE_FILE_REMOVABLE_RUN_FROM_SWAP\n");

  if (FileHeader->Characteristics & IMAGE_FILE_SYSTEM)
    printf("                        IMAGE_FILE_SYSTEM\n");

  if (FileHeader->Characteristics & IMAGE_FILE_DLL)
    printf("                        IMAGE_FILE_DLL\n");

  if (FileHeader->Characteristics & IMAGE_FILE_UP_SYSTEM_ONLY)
    printf("                        IMAGE_FILE_UP_SYSTEM_ONLY\n");

  if (FileHeader->Characteristics & IMAGE_FILE_BYTES_REVERSED_HI)
    printf("                        IMAGE_FILE_BYTES_REVERSED_HI\n");

  printf("\n");
}

void CoffLoader::PrintOptionHeader(OptionHeader_t *OptHdr)
{
  printf("Option Header\n");
  printf("------------------------------------------\n\n");

  printf("Magic:              0x%04X\n", OptHdr->Magic);
  printf("Linker Major Ver:   0x%02X\n", VERSION_MAJOR(OptHdr->LinkVersion));
  printf("Linker Minor Ver:   0x%02X\n", VERSION_MINOR(OptHdr->LinkVersion));
  printf("Code Size:          0x%08lX\n", OptHdr->CodeSize);
  printf("Data Size:          0x%08lX\n", OptHdr->DataSize);
  printf("BSS Size:           0x%08lX\n", OptHdr->BssSize);
  printf("Entry:              0x%08lX\n", OptHdr->Entry);
  printf("Code Base:          0x%08lX\n", OptHdr->CodeBase);
  printf("Data Base:          0x%08lX\n", OptHdr->DataBase);
  printf("\n");
}

void CoffLoader::PrintWindowsHeader(WindowsHeader_t *WinHdr)
{
  printf("Windows Specific Option Header\n");
  printf("------------------------------------------\n\n");

  printf("Image Base:         0x%08lX\n", WinHdr->ImageBase);
  printf("Section Alignment:  0x%08lX\n", WinHdr->SectionAlignment);
  printf("File Alignment:     0x%08lX\n", WinHdr->FileAlignment);
  printf("OS Version:         %d.%08d\n", BIGVERSION_MAJOR(WinHdr->OSVer), BIGVERSION_MINOR(WinHdr->OSVer));
  printf("Image Version:      %d.%08d\n", BIGVERSION_MAJOR(WinHdr->ImgVer), BIGVERSION_MINOR(WinHdr->ImgVer));
  printf("SubSystem Version:  %d.%08d\n", BIGVERSION_MAJOR(WinHdr->SubSysVer), BIGVERSION_MINOR(WinHdr->SubSysVer));
  printf("Size of Image:      0x%08lX\n", WinHdr->SizeOfImage);
  printf("Size of Headers:    0x%08lX\n", WinHdr->SizeOfHeaders);
  printf("Checksum:           0x%08lX\n", WinHdr->CheckSum);
  printf("Subsystem:          0x%04X\n", WinHdr->Subsystem);
  printf("DLL Flags:          0x%04X\n", WinHdr->DLLFlags);
  printf("Sizeof Stack Resv:  0x%08lX\n", WinHdr->SizeOfStackReserve);
  printf("Sizeof Stack Comm:  0x%08lX\n", WinHdr->SizeOfStackCommit);
  printf("Sizeof Heap Resv:   0x%08lX\n", WinHdr->SizeOfHeapReserve);
  printf("Sizeof Heap Comm:   0x%08lX\n", WinHdr->SizeOfHeapCommit);
  printf("Loader Flags:       0x%08lX\n", WinHdr->LoaderFlags);
  printf("Num Directories:    %ld\n", WinHdr->NumDirectories);
  printf("\n");
}

void CoffLoader::PrintSection(SectionHeader_t *ScnHdr, char* data)
{
  char SectionName[9];

  strncpy(SectionName, (char *)ScnHdr->Name, 8);
  SectionName[8] = 0;
  printf("Section: %s\n", SectionName);
  printf("------------------------------------------\n\n");

  printf("Virtual Size:       0x%08lX\n", ScnHdr->VirtualSize);
  printf("Virtual Address:    0x%08lX\n", ScnHdr->VirtualAddress);
  printf("Sizeof Raw Data:    0x%08lX\n", ScnHdr->SizeOfRawData);
  printf("Ptr To Raw Data:    0x%08lX\n", ScnHdr->PtrToRawData);
  printf("Ptr To Relocations: 0x%08lX\n", ScnHdr->PtrToRelocations);
  printf("Ptr To Line Nums:   0x%08lX\n", ScnHdr->PtrToLineNums);
  printf("Num Relocations:    0x%04X\n", ScnHdr->NumRelocations);
  printf("Num Line Numbers:   0x%04X\n", ScnHdr->NumLineNumbers);
  printf("Characteristics:    0x%08lX\n", ScnHdr->Characteristics);
  if (ScnHdr->Characteristics & IMAGE_SCN_CNT_CODE)
    printf("                    IMAGE_SCN_CNT_CODE\n");
  if (ScnHdr->Characteristics & IMAGE_SCN_CNT_DATA)
    printf("                    IMAGE_SCN_CNT_DATA\n");
  if (ScnHdr->Characteristics & IMAGE_SCN_CNT_BSS)
    printf("                    IMAGE_SCN_CNT_BSS\n");
  if (ScnHdr->Characteristics & IMAGE_SCN_LNK_INFO)
    printf("                    IMAGE_SCN_LNK_INFO\n");
  if (ScnHdr->Characteristics & IMAGE_SCN_LNK_REMOVE)
    printf("                    IMAGE_SCN_LNK_REMOVE\n");
  if (ScnHdr->Characteristics & IMAGE_SCN_LNK_COMDAT)
    printf("                    IMAGE_SCN_LNK_COMDAT\n");

  if ((ScnHdr->Characteristics & IMAGE_SCN_ALIGN_MASK) == IMAGE_SCN_ALIGN_1BYTES)
    printf("                    IMAGE_SCN_ALIGN_1BYTES\n");
  if ((ScnHdr->Characteristics & IMAGE_SCN_ALIGN_MASK) == IMAGE_SCN_ALIGN_2BYTES)
    printf("                    IMAGE_SCN_ALIGN_2BYTES\n");
  if ((ScnHdr->Characteristics & IMAGE_SCN_ALIGN_MASK) == IMAGE_SCN_ALIGN_4BYTES)
    printf("                    IMAGE_SCN_ALIGN_4BYTES\n");
  if ((ScnHdr->Characteristics & IMAGE_SCN_ALIGN_MASK) == IMAGE_SCN_ALIGN_8BYTES)
    printf("                    IMAGE_SCN_ALIGN_8BYTES\n");
  if ((ScnHdr->Characteristics & IMAGE_SCN_ALIGN_MASK) == IMAGE_SCN_ALIGN_16BYTES)
    printf("                    IMAGE_SCN_ALIGN_16BYTES\n");
  if ((ScnHdr->Characteristics & IMAGE_SCN_ALIGN_MASK) == IMAGE_SCN_ALIGN_32BYTES)
    printf("                    IMAGE_SCN_ALIGN_32BYTES\n");
  if ((ScnHdr->Characteristics & IMAGE_SCN_ALIGN_MASK) == IMAGE_SCN_ALIGN_64BYTES)
    printf("                    IMAGE_SCN_ALIGN_64BYTES\n");
  if ((ScnHdr->Characteristics & IMAGE_SCN_ALIGN_MASK) == IMAGE_SCN_ALIGN_128BYTES)
    printf("                    IMAGE_SCN_ALIGN_128BYTES\n");
  if ((ScnHdr->Characteristics & IMAGE_SCN_ALIGN_MASK) == IMAGE_SCN_ALIGN_256BYTES)
    printf("                    IMAGE_SCN_ALIGN_256BYTES\n");
  if ((ScnHdr->Characteristics & IMAGE_SCN_ALIGN_MASK) == IMAGE_SCN_ALIGN_512BYTES)
    printf("                    IMAGE_SCN_ALIGN_512BYTES\n");
  if ((ScnHdr->Characteristics & IMAGE_SCN_ALIGN_MASK) == IMAGE_SCN_ALIGN_1024BYTES)
    printf("                    IMAGE_SCN_ALIGN_1024BYTES\n");
  if ((ScnHdr->Characteristics & IMAGE_SCN_ALIGN_MASK) == IMAGE_SCN_ALIGN_2048BYTES)
    printf("                    IMAGE_SCN_ALIGN_2048BYTES\n");
  if ((ScnHdr->Characteristics & IMAGE_SCN_ALIGN_MASK) == IMAGE_SCN_ALIGN_4096BYTES)
    printf("                    IMAGE_SCN_ALIGN_4096BYTES\n");
  if ((ScnHdr->Characteristics & IMAGE_SCN_ALIGN_MASK) == IMAGE_SCN_ALIGN_8192BYTES)
    printf("                    IMAGE_SCN_ALIGN_8192BYTES\n");

  if (ScnHdr->Characteristics & IMAGE_SCN_LNK_NRELOC_OVFL)
    printf("                    IMAGE_SCN_LNK_NRELOC_OVFL\n");
  if (ScnHdr->Characteristics & IMAGE_SCN_MEM_DISCARDABLE)
    printf("                    IMAGE_SCN_MEM_DISCARDABLE\n");
  if (ScnHdr->Characteristics & IMAGE_SCN_MEM_NOT_CACHED)
    printf("                    IMAGE_SCN_MEM_NOT_CACHED\n");
  if (ScnHdr->Characteristics & IMAGE_SCN_MEM_NOT_PAGED)
    printf("                    IMAGE_SCN_MEM_NOT_PAGED\n");
  if (ScnHdr->Characteristics & IMAGE_SCN_MEM_SHARED)
    printf("                    IMAGE_SCN_MEM_SHARED\n");
  if (ScnHdr->Characteristics & IMAGE_SCN_MEM_EXECUTE)
    printf("                    IMAGE_SCN_MEM_EXECUTE\n");
  if (ScnHdr->Characteristics & IMAGE_SCN_MEM_READ)
    printf("                    IMAGE_SCN_MEM_READ\n");
  if (ScnHdr->Characteristics & IMAGE_SCN_MEM_WRITE)
    printf("                    IMAGE_SCN_MEM_WRITE\n");
  printf("\n");

  // Read the section Data, Relocations, & Line Nums
  //    Save the offset

  if (ScnHdr->SizeOfRawData > 0)
  {
    unsigned int i;
    char ch;
    // Print the Raw Data

    printf("\nRAW DATA");
    for (i = 0; i < ScnHdr->VirtualSize; i++)
    {
      if ((i % 16) == 0)
        printf("\n  %08X: ", i);
      ch = data[i];
      printf("%02X ", (unsigned int)ch);
    }
    printf("\n\n");
  }

  /*
  #if 0
  if (ScnHdr->NumRelocations > 0)
  {
  // Print Section Relocations
  ObjReloc_t ObjReloc;

  fseek(fp, ScnHdr->PtrToRelocations/ * + CoffBeginOffset* /, SEEK_SET);
  printf("RELOCATIONS\n");
  printf("                     Symbol    Symbol\n");
  printf(" Offset    Type      Index     Name\n");
  printf(" --------  --------  --------  ------\n");
  for (int i = 0; i < ScnHdr->NumRelocations; i++)
  {
  fread(&ObjReloc, 1, sizeof(ObjReloc_t), fp);
  printf(" %08X  ", ObjReloc.VirtualAddress);

  if (ObjReloc.Type == IMAGE_REL_I386_ABSOLUTE)
  printf("ABSOLUTE  ");
  if (ObjReloc.Type == IMAGE_REL_I386_DIR16)
  printf("DIR16     ");
  if (ObjReloc.Type == IMAGE_REL_I386_REL16)
  printf("REL16     ");
  if (ObjReloc.Type == IMAGE_REL_I386_DIR32)
  printf("DIR32     ");
  if (ObjReloc.Type == IMAGE_REL_I386_DIR32NB)
  printf("DIR32NB   ");
  if (ObjReloc.Type == IMAGE_REL_I386_SEG12)
  printf("SEG12     ");
  if (ObjReloc.Type == IMAGE_REL_I386_SECTION)
  printf("SECTION   ");
  if (ObjReloc.Type == IMAGE_REL_I386_SECREL)
  printf("SECREL    ");
  if (ObjReloc.Type == IMAGE_REL_I386_REL32)
  printf("REL32     ");
  printf("%8X  ", ObjReloc.SymTableIndex);
  printf("%s\n", GetSymbolName(ObjReloc.SymTableIndex));
  }
  printf("\n");
  }

  if (ScnHdr->NumLineNumbers > 0)
  {
  // Print The Line Number Info
  LineNumbers_t LineNumber;
  int LineCnt = 0;
  int BaseLineNum = -1;

  fseek(fp, ScnHdr->PtrToLineNums/ * + CoffBeginOffset* /, SEEK_SET);
  printf("LINE NUMBERS");
  for (int i = 0; i < ScnHdr->NumLineNumbers; i++)
  {
  int LNOffset = ftell(fp);

  fread(&LineNumber, 1, sizeof(LineNumbers_t), fp);
  if (LineNumber.LineNum == 0)
  {
  SymbolTable_t *Sym;
  int SymIndex;

  printf("\n");
  SymIndex = LineNumber.Type.SymbolTableIndex;
  Sym = &(SymTable[SymIndex]);
  if (Sym->NumberOfAuxSymbols > 0)
  {
  Sym = &(SymTable[SymIndex+1]);
  AuxFuncDef_t *FuncDef = (AuxFuncDef_t *)Sym;

  if (FuncDef->PtrToLineNumber == LNOffset)
  {
  Sym = &(SymTable[FuncDef->TagIndex]);
  if (Sym->NumberOfAuxSymbols > 0)
  {
  Sym = &(SymTable[FuncDef->TagIndex+1]);
  AuxBfEf_t *Bf = (AuxBfEf_t *)Sym;
  BaseLineNum = Bf->LineNumber;
  }
  }
  }
  printf(" Symbol Index: %8x ", SymIndex);
  printf(" Base line number: %8d\n", BaseLineNum);
  printf(" Symbol name = %s", GetSymbolName(SymIndex));
  LineCnt = 0;
  }
  else
  {
  if ((LineCnt%4) == 0)
  {
  printf("\n ");
  LineCnt = 0;
  }
  printf("%08X(%5d)  ", LineNumber.Type.VirtualAddress,
  LineNumber.LineNum + BaseLineNum);
  LineCnt ++;
  }
  }
  printf("\n");
  }
  #endif
  */

  printf("\n");
}

int CoffLoader::ParseCoff(FILE *fp)
{
  if ( !LoadCoffHModule(fp) )
  {
    printf("Failed to load/find COFF hModule header\n");
    return 0;
  }
  if ( !LoadSymTable(fp) ||
       !LoadStringTable(fp) ||
       !LoadSections(fp) )
    return 0;

  PerformFixups();

#ifdef DUMPING_DATA
  PrintSymbolTable();
  PrintStringTable();
#endif
  return 1;
}

void CoffLoader::PerformFixups(void)
{
  int FixupDataSize;
  char *FixupData;
  char *EndData;

  EntryAddress = (unsigned long)RVA2Data(EntryAddress);

  if( (PVOID)WindowsHeader->ImageBase == hModule )
    return;

  if ( !Directory )
    return ;

  if ( NumOfDirectories <= BASE_RELOCATION_TABLE )
    return ;

  if ( !Directory[BASE_RELOCATION_TABLE].Size )
    return ;

  FixupDataSize = Directory[BASE_RELOCATION_TABLE].Size;
  FixupData = (char*)RVA2Data(Directory[BASE_RELOCATION_TABLE].RVA);
  EndData = FixupData + FixupDataSize;

  while (FixupData < EndData)
  {
    // Starting a new Fixup Block
    unsigned long PageRVA = *((unsigned long*)FixupData);
    FixupData += 4;
    unsigned long BlockSize = *((unsigned long*)FixupData);
    FixupData += 4;

    BlockSize -= 8;
    for (unsigned int i = 0; i < BlockSize / 2; i++)
    {
      unsigned short Fixup = *((unsigned short*)FixupData);
      FixupData += 2;
      int Type = (Fixup >> 12) & 0x0f;
      Fixup &= 0xfff;
      if (Type == IMAGE_REL_BASED_HIGHLOW)
      {
        unsigned long *Off = (unsigned long*)RVA2Data(Fixup + PageRVA);
        *Off = (unsigned long)RVA2Data(*Off - WindowsHeader->ImageBase);
      }
      else if (Type == IMAGE_REL_BASED_ABSOLUTE)
      {}
      else
      {
        printf("Unsupported fixup type!!\n");
      }
    }
  }
}
