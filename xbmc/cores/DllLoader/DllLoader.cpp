/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include <stdlib.h>
#include <algorithm>

#include "DllLoader.h"
#include "DllLoaderContainer.h"
#include "filesystem/SpecialProtocol.h"
#include "dll_tracker.h"
#include "dll_util.h"
#include <limits>
#include "utils/log.h"

#ifdef TARGET_WINDOWS
extern "C" FILE *fopen_utf8(const char *_Filename, const char *_Mode);
#else
#define fopen_utf8 fopen
#endif

#include "commons/Exception.h"

#define DLL_PROCESS_DETACH   0
#define DLL_PROCESS_ATTACH   1
#define DLL_THREAD_ATTACH    2
#define DLL_THREAD_DETACH    3
#define DLL_PROCESS_VERIFIER 4


#ifndef APIENTRY
#define APIENTRY __stdcall
#endif

//  Entry point of a dll (DllMain)
typedef int (APIENTRY *EntryFunc)(HINSTANCE hinstDLL, DWORD fdwReason, void* lpvReserved);


#ifdef TARGET_POSIX
/*
 * This is a dirty hack.
 * The win32 DLLs contain an alloca routine, that first probes the soon
 * to be allocated new memory *below* the current stack pointer in 4KByte
 * increments.  After the mem probing below the current %esp,  the stack
 * pointer is finally decremented to make room for the "alloca"ed memory.
 * Maybe the probing code is intended to extend the stack on a windows box.
 * Anyway, the linux kernel does *not* extend the stack by simply accessing
 * memory below %esp;  it segfaults.
 * The extend_stack_for_dll_alloca() routine just preallocates a big chunk
 * of memory on the stack, for use by the DLLs alloca routine.
 * Added the noinline attribute as e.g. gcc 3.2.2 inlines this function
 * in a way that breaks it.
 */
static void __attribute__((noinline)) extend_stack_for_dll_alloca(void)
{
    volatile int* mem =(volatile int*)alloca(0x20000);
    *mem=0x1234;
}
#endif


DllLoader::DllLoader(const char *sDll, bool bTrack, bool bSystemDll, bool bLoadSymbols, Export* exps) : LibraryLoader(sDll)
{
  ImportDirTable = 0;
  m_pExportHead = NULL;
  m_pStaticExports = exps;
  m_bTrack = bTrack;
  m_bSystemDll = bSystemDll;
  m_pDlls = NULL;


  if(!bSystemDll)
  {
    // Initialize FS segment, important for quicktime dll's
#if defined(USE_LDT_KEEPER)
    m_ldt_fs = Setup_LDT_Keeper();
#endif
  }

  DllLoaderContainer::RegisterDll(this);
  if (m_bTrack) tracker_dll_add(this);
  m_bLoadSymbols=bLoadSymbols;

  m_bUnloadSymbols=false;

  /* system dll's are never loaded in any way, so let's just use the pointer */
  /* to this object as their base address */
  if (m_bSystemDll)
    hModule = (HMODULE)this;

}

DllLoader::~DllLoader()
{
  while (m_pExportHead)
  {
    ExportEntry* entry = m_pExportHead;
    m_pExportHead = entry->next;

    free(entry);
  }

  while (m_pDlls)
  {
    LoadedList* entry = m_pDlls;
    m_pDlls = entry->pNext;
    LibraryLoader* lib = entry->pDll;
    if (entry->pDll) DllLoaderContainer::ReleaseModule(lib);
    delete entry;
  }

  // can't unload a system dll, as this might be happing during xbmc destruction
  if(!m_bSystemDll)
  {
    DllLoaderContainer::UnRegisterDll(this);

#ifdef USE_LDT_KEEPER
    Restore_LDT_Keeper(m_ldt_fs);
#endif
  }
  if (m_bTrack) tracker_dll_free(this);

  ImportDirTable = 0;

  // hModule points to DllLoader in this case
  if (m_bSystemDll)
    hModule = NULL;
}

int DllLoader::Parse()
{
  int iResult = 0;

  std::string strFileName= GetFileName();
  FILE* fp = fopen_utf8(CSpecialProtocol::TranslatePath(strFileName).c_str(), "rb");

  if (fp)
  {
    if (CoffLoader::ParseCoff(fp))
    {
      if(WindowsHeader)
        tracker_dll_set_addr(this, (uintptr_t)hModule,
          (uintptr_t)hModule + WindowsHeader->SizeOfImage - 1);
      else
      {
        uintptr_t iMinAddr = std::numeric_limits<uintptr_t>::max();
        uintptr_t iMaxAddr = 0;
        // dll is loaded now, this means we also know the base address of it and its size
        for (int i = 0; i < NumOfSections; ++i)
        {
          iMinAddr = std::min<uintptr_t>(iMinAddr,
                       (uintptr_t)SectionHeader[i].VirtualAddress);
          iMaxAddr = std::max<uintptr_t>(iMaxAddr,
                       (uintptr_t)(SectionHeader[i].VirtualAddress +
                                   SectionHeader[i].VirtualSize));
        }
        if(iMaxAddr > iMinAddr)
        {
          iMinAddr += (uintptr_t)hModule;
          iMaxAddr += (uintptr_t)hModule;
          tracker_dll_set_addr(this, iMinAddr, iMaxAddr - 1);
        }
      }
      LoadExports();
      iResult = 1;
    }
    fclose(fp);
  }
  if (iResult == 0)
  {
    m_bTrack = false;
  }
  return iResult;
}

void DllLoader::PrintImportLookupTable(unsigned long ImportLookupTable_RVA)
{
  unsigned long *Table = (unsigned long*)RVA2Data(ImportLookupTable_RVA);

  while (*Table)
  {
    if (*Table & 0x80000000)
    {
      // Process Ordinal...
      CLog::Log(LOGDEBUG, "            Ordinal: %01lX", *Table & 0x7fffffff);
    }
    else
    {
      CLog::Log(LOGDEBUG, "            Don't process Hint/Name Table yet...");
    }
    Table++;
  }
}

void DllLoader::PrintImportTable(ImportDirTable_t *ImportDirTable)
{
  ImportDirTable_t *Imp = ImportDirTable;
  int HavePrinted = 0;

  CLog::Log(LOGDEBUG, "The Coff Image contains the following imports:");
  while ( Imp->ImportLookupTable_RVA != 0 ||
          Imp->TimeStamp != 0 ||
          Imp->ForwarderChain != 0 ||
          Imp->Name_RVA != 0 ||
          Imp->ImportAddressTable_RVA != 0)
  {
    char *Name;
    HavePrinted = 1;

    Name = (char*)RVA2Data(Imp->Name_RVA);

    CLog::Log(LOGDEBUG, "    %s:", Name);
    CLog::Log(LOGDEBUG, "        ImportAddressTable:     %04lX", Imp->ImportAddressTable_RVA);
    CLog::Log(LOGDEBUG, "        ImportLookupTable:      %04lX", Imp->ImportLookupTable_RVA);
    CLog::Log(LOGDEBUG, "        TimeStamp:              %01lX", Imp->TimeStamp);
    CLog::Log(LOGDEBUG, "        Forwarder Chain:        %01lX", Imp->ForwarderChain);

    PrintImportLookupTable(Imp->ImportLookupTable_RVA);
    CLog::Log(LOGDEBUG, "");
    Imp++;
  }
  if (!HavePrinted) CLog::Log(LOGDEBUG, "None.");
}

void DllLoader::PrintExportTable(ExportDirTable_t *ExportDirTable)
{
  char *Name = (char*)RVA2Data(ExportDirTable->Name_RVA);

  unsigned long *ExportAddressTable = (unsigned long*)RVA2Data(ExportDirTable->ExportAddressTable_RVA);
  unsigned long *NamePointerTable = (unsigned long*)RVA2Data(ExportDirTable->NamePointerTable_RVA);
  unsigned short *OrdinalTable = (unsigned short*)RVA2Data(ExportDirTable->OrdinalTable_RVA);


  CLog::Log(LOGDEBUG, "Export Table for %s:", Name);

  CLog::Log(LOGDEBUG, "ExportFlags:    %04lX", ExportDirTable->ExportFlags);
  CLog::Log(LOGDEBUG, "TimeStamp:      %04lX", ExportDirTable->TimeStamp);
  CLog::Log(LOGDEBUG, "Major Ver:      %02X", ExportDirTable->MajorVersion);
  CLog::Log(LOGDEBUG, "Minor Ver:      %02X", ExportDirTable->MinorVersion);
  CLog::Log(LOGDEBUG, "Name RVA:       %04lX", ExportDirTable->Name_RVA);
  CLog::Log(LOGDEBUG, "OrdinalBase     %lu", ExportDirTable->OrdinalBase);
  CLog::Log(LOGDEBUG, "NumAddrTable    %lu", ExportDirTable->NumAddrTable);
  CLog::Log(LOGDEBUG, "NumNamePtrs     %lu", ExportDirTable->NumNamePtrs);
  CLog::Log(LOGDEBUG, "ExportAddressTable_RVA  %04lX", ExportDirTable->ExportAddressTable_RVA);
  CLog::Log(LOGDEBUG, "NamePointerTable_RVA    %04lX", ExportDirTable->NamePointerTable_RVA);
  CLog::Log(LOGDEBUG, "OrdinalTable_RVA        %04lX", ExportDirTable->OrdinalTable_RVA);

  CLog::Log(LOGDEBUG, "Public Exports:");
  CLog::Log(LOGDEBUG, "    ordinal hint RVA      name");
  for (unsigned int i = 0; i < ExportDirTable->NumNamePtrs; i++)
  {
    char *Name = (char*)RVA2Data(NamePointerTable[i]);

    CLog::Log(LOGDEBUG, "          %lu", OrdinalTable[i] + ExportDirTable->OrdinalBase);
    CLog::Log(LOGDEBUG, "    %d", OrdinalTable[i]);
    CLog::Log(LOGDEBUG, " %08lX", ExportAddressTable[OrdinalTable[i]]);
    CLog::Log(LOGDEBUG, " %s", Name);
  }
}

int DllLoader::ResolveImports(void)
{
  int bResult = 1;
  if ( NumOfDirectories >= 2 && Directory[IMPORT_TABLE].Size > 0 )
  {
    ImportDirTable = (ImportDirTable_t*)RVA2Data(Directory[IMPORT_TABLE].RVA);

#ifdef DUMPING_DATA
    PrintImportTable(ImportDirTable);
#endif

    ImportDirTable_t *Imp = ImportDirTable;

    while ( Imp->ImportLookupTable_RVA != 0 ||
            Imp->TimeStamp != 0 ||
            Imp->ForwarderChain != 0 ||
            Imp->Name_RVA != 0 ||
            Imp->ImportAddressTable_RVA != 0)
    {
      const char *Name = (const char*)RVA2Data(Imp->Name_RVA);

      const char* FileName=ResolveReferencedDll(Name);
      //  If possible use the dll name WITH path to resolve exports. We could have loaded
      //  a dll with the same name as another dll but from a different directory
      if (FileName) Name=FileName;

      unsigned long *Table = (unsigned long*)RVA2Data(Imp->ImportLookupTable_RVA);
      unsigned long *Addr = (unsigned long*)RVA2Data(Imp->ImportAddressTable_RVA);

      while (*Table)
      {
        if (*Table & 0x80000000)
        {
          void *Fixup;
          if ( !ResolveOrdinal(Name, *Table&0x7ffffff, &Fixup) )
          {
            bResult = 0;
            char szBuf[128];
            CLog::Log(LOGDEBUG,"Unable to resolve ordinal %s %lu", Name, *Table&0x7ffffff);
            sprintf(szBuf, "%lu", *Table&0x7ffffff);
            *Addr = create_dummy_function(Name, szBuf);
            tracker_dll_data_track(this, *Addr);
          }
          else
          {
            *Addr = (unsigned long)Fixup;  //woohoo!!
          }
        }
        else
        {
          // We don't handle Hint/Name tables yet!!!
          char *ImpName = (char*)RVA2Data(*Table + 2);

          void *Fixup;
          if ( !ResolveName(Name, ImpName, &Fixup) )
          {
            *Addr=get_win_function_address(Name, ImpName);
            if(!*Addr)
            {
              CLog::Log(LOGDEBUG,"Unable to resolve %s %s", Name, ImpName);
              *Addr = create_dummy_function(Name, ImpName);
              tracker_dll_data_track(this, *Addr);
              bResult = 0;
            }
          }
          else
          {
            *Addr = (unsigned long)Fixup;
          }
        }
        Table++;
        Addr++;
      }
      Imp++;
    }
  }
  return bResult;
}

const char* DllLoader::ResolveReferencedDll(const char* dll)
{
  DllLoader* pDll = static_cast<DllLoader*>(DllLoaderContainer::LoadModule(dll, GetPath(), m_bLoadSymbols));

  if (!pDll)
  {
    CLog::Log(LOGDEBUG, "Unable to load referenced dll %s - Dll: %s", dll, GetFileName());
    return NULL;
  }
  else if (!pDll->IsSystemDll())
  {
    LoadedList* entry=new LoadedList;
    entry->pDll=pDll;
    entry->pNext=m_pDlls;
    m_pDlls=entry;
  }

  return pDll->GetFileName();
}

int DllLoader::LoadExports()
{
  if ( NumOfDirectories > EXPORT_TABLE && Directory[EXPORT_TABLE].Size > 0 )
  {
    ExportDirTable = (ExportDirTable_t*)RVA2Data(Directory[EXPORT_TABLE].RVA);

#ifdef DUMPING_DATA
    PrintExportTable(ExportDirTable);
#endif

    //! @todo Validate all pointers are valid. Is a zero RVA valid or not? I'd guess not as it would
    //! point to the coff file header, thus not right.

    unsigned long *ExportAddressTable = (unsigned long*)RVA2Data(ExportDirTable->ExportAddressTable_RVA);
    unsigned long *NamePointerTable = (unsigned long*)RVA2Data(ExportDirTable->NamePointerTable_RVA);
    unsigned short *OrdinalTable = (unsigned short*)RVA2Data(ExportDirTable->OrdinalTable_RVA);

    for (unsigned int i = 0; i < ExportDirTable->NumNamePtrs; i++)
    {
      char *Name = (char*)RVA2Data(NamePointerTable[i]);
      void* Addr = (void*)RVA2Data(ExportAddressTable[OrdinalTable[i]]);
      AddExport(Name, OrdinalTable[i]+ExportDirTable->OrdinalBase, Addr);
    }
  }
  return 0;
}

int DllLoader::ResolveExport(const char *sName, void **pAddr, bool logging)
{
  Export* pExport=GetExportByFunctionName(sName);

  if (pExport)
  {
    if (m_bTrack && pExport->track_function)
      *pAddr=(void*)pExport->track_function;
    else
      *pAddr=(void*)pExport->function;

    return 1;
  }

  const char* sDllName = strrchr(GetFileName(), '\\');
  if (sDllName) sDllName += 1;
  else sDllName = GetFileName();

  if (logging)
    CLog::Log(LOGWARNING, "Unable to resolve: %s %s", sDllName, sName);
  return 0;
}

int DllLoader::ResolveOrdinal(unsigned long ordinal, void **pAddr)
{
  Export* pExport=GetExportByOrdinal(ordinal);

  if (pExport)
  {
    if (m_bTrack && pExport->track_function)
      *pAddr=(void*)pExport->track_function;
    else
      *pAddr=(void*)pExport->function;

    return 1;
  }

  const char* sDllName = strrchr(GetFileName(), '\\');
  if (sDllName) sDllName += 1;
  else sDllName = GetFileName();

  CLog::Log(LOGWARNING, "Unable to resolve: %s %lu", sDllName, ordinal);
  return 0;
}

Export* DllLoader::GetExportByOrdinal(unsigned long ordinal)
{
  ExportEntry* entry = m_pExportHead;

  while (entry)
  {
    if (ordinal == entry->exp.ordinal)
    {
      return &entry->exp;
    }
    entry = entry->next;
  }

  if( m_pStaticExports )
  {
    Export* exp = m_pStaticExports;
    while(exp->function || exp->track_function || exp->name)
    {
      if (ordinal == exp->ordinal)
        return exp;
      exp++;
    }
  }

  return NULL;
}

Export* DllLoader::GetExportByFunctionName(const char* sFunctionName)
{
  ExportEntry* entry = m_pExportHead;

  while (entry)
  {
    if (entry->exp.name && strcmp(sFunctionName, entry->exp.name) == 0)
    {
      return &entry->exp;
    }
    entry = entry->next;
  }

  if( m_pStaticExports )
  {
    Export* exp = m_pStaticExports;
    while(exp->function || exp->track_function || exp->name)
    {
      if (exp->name && strcmp(sFunctionName, exp->name) == 0)
        return exp;
      exp++;
    }
  }

  return NULL;
}

int DllLoader::ResolveOrdinal(const char *sName, unsigned long ordinal, void **fixup)
{
  DllLoader* pDll = static_cast<DllLoader*>(DllLoaderContainer::GetModule(sName));

  if (pDll)
  {
    Export* pExp = pDll->GetExportByOrdinal(ordinal);
    if(pExp)
    {
      if (m_bTrack && pExp->track_function)
        *fixup = (void*)(pExp->track_function);
      else
        *fixup = (void*)(pExp->function);

      return 1;
    }
  }

  return 0;
}

int DllLoader::ResolveName(const char *sName, char* sFunction, void **fixup)
{
  DllLoader* pDll = static_cast<DllLoader*>(DllLoaderContainer::GetModule(sName));

  if (pDll)
  {
    Export* pExp = pDll->GetExportByFunctionName(sFunction);
    if(pExp)
    {
      if (m_bTrack && pExp->track_function)
        *fixup = (void*)(pExp->track_function);
      else
        *fixup = (void*)(pExp->function);
      return 1;
    }
  }

  return 0;
}

void DllLoader::AddExport(unsigned long ordinal, void* function, void* track_function)
{
  ExportEntry* entry = (ExportEntry*)malloc(sizeof(ExportEntry));
  if (!entry)
    return;
  entry->exp.function = function;
  entry->exp.ordinal = ordinal;
  entry->exp.track_function = track_function;
  entry->exp.name = NULL;

  entry->next = m_pExportHead;
  m_pExportHead = entry;
}

void DllLoader::AddExport(char* sFunctionName, unsigned long ordinal, void* function, void* track_function)
{
  int len = sizeof(ExportEntry);

  ExportEntry* entry = (ExportEntry*)malloc(len + strlen(sFunctionName) + 1);
  if (!entry)
    return;
  entry->exp.function = function;
  entry->exp.ordinal = ordinal;
  entry->exp.track_function = track_function;
  entry->exp.name = ((char*)(entry)) + len;
  strcpy(const_cast<char*>(entry->exp.name), sFunctionName);

  entry->next = m_pExportHead;
  m_pExportHead = entry;
}

void DllLoader::AddExport(char* sFunctionName, void* function, void* track_function)
{
  int len = sizeof(ExportEntry);

  ExportEntry* entry = (ExportEntry*)malloc(len + strlen(sFunctionName) + 1);
  if (!entry)
    return;
  entry->exp.function = (void*)function;
  entry->exp.ordinal = -1;
  entry->exp.track_function = track_function;
  entry->exp.name = ((char*)(entry)) + len;
  strcpy(const_cast<char*>(entry->exp.name), sFunctionName);

  entry->next = m_pExportHead;
  m_pExportHead = entry;
}

bool DllLoader::Load()
{
  if (!Parse())
  {
    CLog::Log(LOGERROR, "Unable to open dll %s", GetFileName());
    return false;
  }

  ResolveImports();
  LoadSymbols();

  // only execute DllMain if no EntryPoint is found
  if (!EntryAddress)
    ResolveExport("DllMain", (void**)&EntryAddress);

#ifdef LOGALL
  CLog::Log(LOGDEBUG, "Executing EntryPoint with DLL_PROCESS_ATTACH at: 0x%x - Dll: %s", pLoader->EntryAddress, sName);
#endif

  if(EntryAddress)
  {
    EntryFunc initdll = (EntryFunc)EntryAddress;
    /* since we are handing execution over to unknown code, safeguard here */
    try
    {
#ifdef TARGET_POSIX
	extend_stack_for_dll_alloca();
#endif
      initdll((HINSTANCE)hModule, DLL_PROCESS_ATTACH , 0); //call "DllMain" with DLL_PROCESS_ATTACH

#ifdef LOGALL
      CLog::Log(LOGDEBUG, "EntryPoint with DLL_PROCESS_ATTACH called - Dll: %s", sName);
#endif

    }
    XBMCCOMMONS_HANDLE_UNCHECKED
    catch(...)
    {
      CLog::Log(LOGERROR, "%s - Unhandled exception during DLL_PROCESS_ATTACH", __FUNCTION__);

      // vp7vfw.dll throws a CUserException due to a missing export
      // but the export isn't really needed for normal operation
      // and dll works anyway, so let's ignore it

      if (StringUtils::CompareNoCase(GetName(), "vp7vfw.dll") != 0)
        return false;


      CLog::Log(LOGDEBUG, "%s - Ignoring exception during DLL_PROCESS_ATTACH", __FUNCTION__);
    }

    // init function may have fixed up the export table
    // this is what I expect should happens on PECompact2
    // dll's if export table is compressed.
    if(!m_pExportHead)
      LoadExports();
  }

  return true;
}

void DllLoader::Unload()
{
#ifdef LOGALL
    CLog::Log(LOGDEBUG, "Executing EntryPoint with DLL_PROCESS_DETACH at: 0x%x - Dll: %s", pDll->EntryAddress, pDll->GetFileName());
#endif

    //call "DllMain" with DLL_PROCESS_DETACH
    if(EntryAddress)
    {
      EntryFunc initdll = (EntryFunc)EntryAddress;
      initdll((HINSTANCE)hModule, DLL_PROCESS_DETACH , 0);
    }

#ifdef LOGALL
  CLog::Log(LOGDEBUG, "EntryPoint with DLL_PROCESS_DETACH called - Dll: %s", pDll->GetFileName());
#endif

  if (m_bUnloadSymbols)
    UnloadSymbols();
}

// This function is a hack to get symbols loaded for
// dlls. The function FFinishImageLoad internally allocates
// memory which is/can never be freed. And the dll can not be
// unloaded.
void DllLoader::LoadSymbols()
{
#ifdef ENABLE_SYMBOL_LOADING
  if (!m_bLoadSymbols ) return;

  // don't load debug symbols unless we have a debugger present
  // seems these calls break on some bioses. i suppose it could
  // be related to if the bios has debug capabilities.
  if (!DmIsDebuggerPresent())
  {
    m_bLoadSymbols=false;
    return;
  }

  LPVOID pBaseAddress=GetXbdmBaseAddress();

  if (pBaseAddress)
  {
    CoffLoader dllxbdm;
    if (dllxbdm.ParseHeaders(pBaseAddress))
    {
      int offset=GetFFinishImageLoadOffset(dllxbdm.WindowsHeader->CheckSum);

      if (offset==0)
      {
        CLog::Log(LOGDEBUG,"DllLoader: Unable to load symbols for %s. No offset for xbdm.dll with checksum 0x%08X found", GetName(), dllxbdm.WindowsHeader->CheckSum);
        return;
      }

      // Get a function pointer to the unexported function FFinishImageLoad
      fnFFinishImageLoad FFinishImageLoad=(fnFFinishImageLoad)((LPBYTE)pBaseAddress+offset);

      // Prepare parameter for the function call
      LDR_DATA_TABLE_ENTRY ldte;
      LPLDR_DATA_TABLE_ENTRY pldteout;
      ldte.DllBase=hModule; // Address where this dll is loaded into memory
      char* szName=GetName(); // Name of this dll without path

      try
      {
        // Call FFinishImageLoad to register this dll to the debugger and load its symbols.
        FFinishImageLoad(&ldte, szName, &pldteout);
      }
      catch(...)
      {
        CLog::Log(LOGDEBUG,"DllLoader: Loading symbols for %s failed with an exception.", GetName());
      }
    }
  }
  else
    CLog::Log(LOGDEBUG,"DllLoader: Can't load symbols for %s. xbdm.dll is needed and not loaded", GetName());

#ifdef ENABLE_SYMBOL_UNLOADING
  m_bUnloadSymbols=true;  // Do this to allow unloading this dll from dllloadercontainer
#endif

#else
  m_bLoadSymbols=false;
#endif
}

// This function is even more a hack
// It will remove the dll from the Debug manager
// but vs.net does not unload the symbols (don't know why)
// The dll can be loaded again after unloading.
// This function leaks memory.
void DllLoader::UnloadSymbols()
{
#ifdef ENABLE_SYMBOL_UNLOADING
  ANSI_STRING name;
  OBJECT_ATTRIBUTES attributes;
  RtlInitAnsiString(&name, GetName());
  InitializeObjectAttributes(&attributes, &name, OBJ_CASE_INSENSITIVE, NULL);

  // Try to unload the symbols from vs.net debugger
  DbgUnLoadImageSymbols(&name, (ULONG)hModule, 0xFFFFFFFF);

  LPVOID pBaseAddress=GetXbdmBaseAddress();

  if (pBaseAddress)
  {
    CoffLoader dllxbdm;
    if (dllxbdm.ParseHeaders(pBaseAddress))
    {
      int offset=GetDmiOffset(dllxbdm.WindowsHeader->CheckSum);

      if (offset==0)
      {
        CLog::Log(LOGDEBUG,"DllLoader: Unable to unload symbols for %s. No offset for xbdm.dll with checksum 0x%08X found", GetName(), dllxbdm.WindowsHeader->CheckSum);
        return;
      }

      try
      {
        std::wstring strNameW;
        g_charsetConverter.utf8ToW(GetName(), strNameW);

        // Get the address of the global struct g_dmi
        // It is located inside the xbdm.dll and
        // get the LoadedModuleList member (here the entry var)
        // of the structure.
        LPBYTE g_dmi=((LPBYTE)pBaseAddress)+offset;
        LIST_ENTRY* entry=(LIST_ENTRY*)(g_dmi+4);

        //  Search for the dll we are unloading...
        while (entry)
        {
          std::wstring baseName=(wchar_t*)((LDR_DATA_TABLE_ENTRY*)entry)->BaseDllName.Buffer;
          if (baseName == strNameW)
          {
            // ...and remove it from the LoadedModuleList and free its memory.
            LIST_ENTRY* back=entry->Blink;
            LIST_ENTRY* front=entry->Flink;
            back->Flink=front;
            front->Blink=back;
            DmFreePool(entry);
            break;
          }

          entry=entry->Flink;
        }
      }
      catch(...)
      {
        CLog::Log(LOGDEBUG,"DllLoader: Unloading symbols for %s failed with an exception.", GetName());
      }
    }
  }
  else
    CLog::Log(LOGDEBUG,"DllLoader: Can't unload symbols for %s. xbdm.dll is needed and not loaded", GetName());
#endif
}
