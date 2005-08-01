
#include "../../stdafx.h"
#include "DllLoader.h"
#include "DllLoaderContainer.h"
#include "dll_tracker.h"
#include "dll_util.h"

// is it really needed?
void* fs_seg = NULL;

DllLoader::DllLoader(const char *sDll, bool bTrack, bool bSystemDll)
{
  ImportDirTable = 0;
  m_sFileName = strdup(sDll);
  
  m_iRefCount = 1;
  m_pExports = NULL;
  m_bTrack = bTrack;
  m_bSystemDll = bSystemDll;
  
  // Initialize FS segment, important for quicktime dll's
  if (fs_seg == NULL)
  {
    CLog::Log(LOGDEBUG, "Initializing FS_SEG..");
    fs_seg = malloc(0x1000);
    RtlZeroMemory(fs_seg, 0x1000);
    __asm {
      mov eax, fs_seg;
      mov fs: [18h], eax;
      xor eax, eax
    }
    CLog::Log(LOGDEBUG, "FS segment @ 0x%x", fs_seg);
  }

  if (!m_bSystemDll) g_dlls.RegisterDll(this);
  if (m_bTrack) tracker_dll_add(this);
}

DllLoader::~DllLoader()
{
  while (m_pExports)
  {
    ExportList* entry = m_pExports;
    m_pExports = entry->pNext;
    
    if (entry->pExport->sFunctionName) free(entry->pExport->sFunctionName);
    delete entry->pExport;
    delete entry;
  }
  
  if (!m_bSystemDll) g_dlls.UnRegisterDll(this);
  if (m_bTrack) tracker_dll_free(this);

  ImportDirTable = 0;
  free(m_sFileName);
}

int DllLoader::Parse()
{
  int iResult = 0;
  FILE* fp = fopen(m_sFileName, "rb");
  
  if (fp)
  {
    if (CoffLoader::ParseCoff(fp))
    {
      // dll is loaded now, this means we also know the base address of it and its size
      // we use this for tracking (".text" is the first section in a dll)
      for (int i = 0; i < NumOfSections; ++i)
      {
        if (!memcmp(SectionHeader[i].Name, ".text", 6))
        {
          unsigned int iMinAddr = (unsigned)hModule + SectionHeader[i].VirtualAddress;
          unsigned int iMaxAddr = iMinAddr + SectionHeader[i].VirtualSize;
          tracker_dll_set_addr(this, iMinAddr, iMaxAddr);
          break;
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
  int Sctn = RVA2Section(ImportLookupTable_RVA);
  unsigned long *Table = (unsigned long*)
      (SectionData[Sctn] + (ImportLookupTable_RVA - SectionHeader[Sctn].VirtualAddress));

  while (*Table)
  {
    if (*Table & 0x80000000)
    {
      // Process Ordinal...
      CLog::Log(LOGDEBUG, "            Ordinal: %01X\n", *Table & 0x7fffffff);
    }
    else
    {
      CLog::Log(LOGDEBUG, "            Don't process Hint/Name Table yet...\n");
    }
    Table++;
  }
}

void DllLoader::PrintImportTable(ImportDirTable_t *ImportDirTable)
{
  ImportDirTable_t *Imp = ImportDirTable;
  int HavePrinted = 0;

  printf("The Coff Image contains the following imports:\n\n");
  while ( Imp->ImportLookupTable_RVA != 0 ||
          Imp->TimeStamp != 0 ||
          Imp->ForwarderChain != 0 ||
          Imp->Name_RVA != 0 ||
          Imp->ImportAddressTable_RVA != 0)
  {
    char *Name; int Sctn;
    HavePrinted = 1;

    Sctn = RVA2Section(Imp->Name_RVA);
    Name = SectionData[Sctn] + (Imp->Name_RVA - SectionHeader[Sctn].VirtualAddress);
    
    CLog::Log(LOGDEBUG, "    %s:\n", Name);
    CLog::Log(LOGDEBUG, "        ImportAddressTable:     %04X\n", Imp->ImportAddressTable_RVA);
    CLog::Log(LOGDEBUG, "        ImportLookupTable:      %04X\n", Imp->ImportLookupTable_RVA);
    CLog::Log(LOGDEBUG, "        TimeStamp:              %01X\n", Imp->TimeStamp);
    CLog::Log(LOGDEBUG, "        Forwarder Chain:        %01X\n", Imp->ForwarderChain);
    
    PrintImportLookupTable(Imp->ImportLookupTable_RVA);
    CLog::Log(LOGDEBUG, "\n");
    Imp++;
  }
  if (!HavePrinted) CLog::Log(LOGDEBUG, "None.");
}

void DllLoader::PrintExportTable(ExportDirTable_t *ExportDirTable)
{
  int Sctn = RVA2Section(ExportDirTable->Name_RVA);
  char *Name = SectionData[Sctn] + (ExportDirTable->Name_RVA - SectionHeader[Sctn].VirtualAddress);

  Sctn = RVA2Section(ExportDirTable->ExportAddressTable_RVA);
  unsigned long *ExportAddressTable = (unsigned long*)
                                      (SectionData[Sctn] + (ExportDirTable->ExportAddressTable_RVA - SectionHeader[Sctn].VirtualAddress));

  Sctn = RVA2Section(ExportDirTable->NamePointerTable_RVA);
  unsigned long *NamePointerTable = (unsigned long*)
                                    (SectionData[Sctn] + (ExportDirTable->NamePointerTable_RVA - SectionHeader[Sctn].VirtualAddress));

  Sctn = RVA2Section(ExportDirTable->OrdinalTable_RVA);
  unsigned short *OrdinalTable = (unsigned short*)
                                 (SectionData[Sctn] + (ExportDirTable->OrdinalTable_RVA - SectionHeader[Sctn].VirtualAddress));


  printf("Export Table for %s:\n", Name);

  printf("ExportFlags:    %04X\n", ExportDirTable->ExportFlags);
  printf("TimeStamp:      %04X\n", ExportDirTable->TimeStamp);
  printf("Major Ver:      %02X\n", ExportDirTable->MajorVersion);
  printf("Minor Ver:      %02X\n", ExportDirTable->MinorVersion);
  printf("Name RVA:       %04X\n", ExportDirTable->Name_RVA);
  printf("OrdinalBase     %d\n", ExportDirTable->OrdinalBase);
  printf("NumAddrTable    %d\n", ExportDirTable->NumAddrTable);
  printf("NumNamePtrs     %d\n", ExportDirTable->NumNamePtrs);
  printf("ExportAddressTable_RVA  %04X\n", ExportDirTable->ExportAddressTable_RVA);
  printf("NamePointerTable_RVA    %04X\n", ExportDirTable->NamePointerTable_RVA);
  printf("OrdinalTable_RVA        %04X\n\n", ExportDirTable->OrdinalTable_RVA);

  printf("Public Exports:\n");
  printf("    ordinal hint RVA      name\n");
  for (unsigned int i = 0; i < ExportDirTable->NumAddrTable; i++)
  {
    int Sctn = RVA2Section(NamePointerTable[i]);
    char *Name = SectionData[Sctn] + (NamePointerTable[i] - SectionHeader[Sctn].VirtualAddress);

    printf("          %d", OrdinalTable[i] + ExportDirTable->OrdinalBase);
    printf("    %d", OrdinalTable[i]);
    printf(" %08X", ExportAddressTable[OrdinalTable[i]]);
    printf(" %s\n", Name);
  }
}

int DllLoader::ResolveImports(void)
{
  int bResult = 1;
  if ( NumOfDirectories >= 2 && Directory[IMPORT_TABLE].Size > 0 )
  {
    int Sctn = RVA2Section(Directory[IMPORT_TABLE].RVA);
    ImportDirTable = (ImportDirTable_t*)
                     (SectionData[Sctn] + (Directory[IMPORT_TABLE].RVA - SectionHeader[Sctn].VirtualAddress));
                     
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
      char *Name; int Sctn;

      Sctn = RVA2Section(Imp->Name_RVA);
      Name = SectionData[Sctn] + (Imp->Name_RVA - SectionHeader[Sctn].VirtualAddress);

      int SctnTbl = RVA2Section(Imp->ImportLookupTable_RVA);
      unsigned long *Table = (unsigned long*)
                             (SectionData[SctnTbl] + (Imp->ImportLookupTable_RVA - SectionHeader[SctnTbl].VirtualAddress));

      int SctnAddr = RVA2Section(Imp->ImportAddressTable_RVA);
      unsigned long *Addr = (unsigned long*)
                            (SectionData[SctnTbl] + (Imp->ImportAddressTable_RVA - SectionHeader[SctnTbl].VirtualAddress));

      while (*Table)
      {
        if (*Table & 0x80000000)
        {
          void *Fixup;
          if ( !ResolveOrdinal(Name, *Table&0x7ffffff, &Fixup) )
          {
            bResult = 0;
            char szBuf[128];
            sprintf(szBuf, "unable to resolve ordinal %s %d\n", Name, *Table&0x7ffffff);
            OutputDebugString(szBuf);
            sprintf(szBuf, "%d", *Table&0x7ffffff);
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
          int ScnName = RVA2Section(*Table + 2);
          char *ImpName = SectionData[ScnName] + (*Table + 2 - SectionHeader[ScnName].VirtualAddress);

          void *Fixup;
          if ( !ResolveName(Name, ImpName, &Fixup) )
          {
            CLog::DebugLog("Unable to resolve %s %s", Name, ImpName);
            *Addr = create_dummy_function(Name, ImpName);
            tracker_dll_data_track(this, *Addr);
            bResult = 0;
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

int DllLoader::LoadExports()
{
  if ( NumOfDirectories >= 1 )
  {
    int Sctn = RVA2Section(Directory[EXPORT_TABLE].RVA);
    ExportDirTable = (ExportDirTable_t*)
                     (SectionData[Sctn] + (Directory[EXPORT_TABLE].RVA - SectionHeader[Sctn].VirtualAddress));
#ifdef DUMPING_DATA
    PrintExportTable(ExportDirTable);
#endif
    Sctn = RVA2Section(ExportDirTable->Name_RVA);
    char *Name = SectionData[Sctn] + (ExportDirTable->Name_RVA - SectionHeader[Sctn].VirtualAddress);

    Sctn = RVA2Section(ExportDirTable->ExportAddressTable_RVA);
    unsigned long *ExportAddressTable = (unsigned long*)
                                        (SectionData[Sctn] + (ExportDirTable->ExportAddressTable_RVA - SectionHeader[Sctn].VirtualAddress));

    Sctn = RVA2Section(ExportDirTable->NamePointerTable_RVA);
    unsigned long *NamePointerTable = (unsigned long*)
                                      (SectionData[Sctn] + (ExportDirTable->NamePointerTable_RVA - SectionHeader[Sctn].VirtualAddress));

    Sctn = RVA2Section(ExportDirTable->OrdinalTable_RVA);
    unsigned short *OrdinalTable = (unsigned short*)
                                   (SectionData[Sctn] + (ExportDirTable->OrdinalTable_RVA - SectionHeader[Sctn].VirtualAddress));

    for (unsigned int i = 0; i < ExportDirTable->NumAddrTable; i++)
    {
      int Sctn = RVA2Section(NamePointerTable[i]);
      char *Name = SectionData[Sctn] + (NamePointerTable[i] - SectionHeader[Sctn].VirtualAddress);

      unsigned long RVA = ExportAddressTable[OrdinalTable[i]];
      int FSctn = RVA2Section(RVA);
      unsigned long Addr = (unsigned long)
                           (SectionData[FSctn] + (RVA - SectionHeader[FSctn].VirtualAddress));
    
      AddExport(Name, Addr);
    }
  }
  return 0;
}

int DllLoader::ResolveExport(const char *sName, void **pAddr)
{
  if (m_bSystemDll)
  {
    // system dlls (eg kernel32.dll) have no export table in xbmc
    // so lookup directly in the export list
    Export* exp = GetExportByFunctionName(sName);
    if (exp)
    {
      *pAddr = (void*)exp->function;
      return 1;
    }
  }
  else
  {
    if ( NumOfDirectories >= 1 )
    {
      int Sctn = RVA2Section(Directory[EXPORT_TABLE].RVA);
      ExportDirTable = (ExportDirTable_t*)
                      (SectionData[Sctn] + (Directory[EXPORT_TABLE].RVA - SectionHeader[Sctn].VirtualAddress));
                       
  #ifdef DUMPING_DATA
      PrintExportTable(ExportDirTable);
  #endif

      Sctn = RVA2Section(ExportDirTable->Name_RVA);
      char *Name = SectionData[Sctn] + (ExportDirTable->Name_RVA - SectionHeader[Sctn].VirtualAddress);

      Sctn = RVA2Section(ExportDirTable->ExportAddressTable_RVA);
      unsigned long *ExportAddressTable = (unsigned long*)
          (SectionData[Sctn] + (ExportDirTable->ExportAddressTable_RVA - SectionHeader[Sctn].VirtualAddress));

      Sctn = RVA2Section(ExportDirTable->NamePointerTable_RVA);
      unsigned long *NamePointerTable = (unsigned long*)
          (SectionData[Sctn] + (ExportDirTable->NamePointerTable_RVA - SectionHeader[Sctn].VirtualAddress));

      Sctn = RVA2Section(ExportDirTable->OrdinalTable_RVA);
      unsigned short *OrdinalTable = (unsigned short*)
          (SectionData[Sctn] + (ExportDirTable->OrdinalTable_RVA - SectionHeader[Sctn].VirtualAddress));

      for (unsigned int i = 0; i < ExportDirTable->NumAddrTable; i++)
      {
        int Sctn = RVA2Section(NamePointerTable[i]);
        char *Name = SectionData[Sctn] + (NamePointerTable[i] - SectionHeader[Sctn].VirtualAddress);

        if (stricmp(Name, sName) == 0 && pAddr)
        {
          unsigned long RVA = ExportAddressTable[OrdinalTable[i]];
          int FSctn = RVA2Section(RVA);
          *pAddr = (void*)(SectionData[FSctn] + (RVA - SectionHeader[FSctn].VirtualAddress));
          return 1;
        }
      }
    }
  }
  
  char* sDllName = strrchr(m_sFileName, '\\');
  if (sDllName) sDllName += 1;
  else sDllName = m_sFileName;
  
  CLog::Log(LOGWARNING, "Unable to resolve: %s %s", sDllName, sName);
  return 0;
}

Export* DllLoader::GetExportByOrdinal(unsigned long ordinal)
{
  ExportList* it = m_pExports;
  
  while (it)
  {
    if (ordinal == it->pExport->ordinal)
    {
      return it->pExport;
    }
    it = it->pNext;
  }
  return NULL;
}

Export* DllLoader::GetExportByFunctionName(const char* sFunctionName)
{
  ExportList* it = m_pExports;
  
  while (it)
  {
    if (it->pExport->sFunctionName && strcmp(sFunctionName, it->pExport->sFunctionName) == 0)
    {
      return it->pExport;
    }
    it = it->pNext;
  }
  return NULL;
}
  
int DllLoader::ResolveOrdinal(char *sName, unsigned long ordinal, void **fixup)
{
  int iSize = g_dlls.GetNrOfModules();

  for (int i = 0; i < iSize; i++)
  {
    DllLoader* pDll = g_dlls.GetModule(i);
    if (pDll && stricmp(sName, pDll->GetName()) == 0)
    {
      Export* pExp = pDll->GetExportByOrdinal(ordinal);
      if(pExp)
      {
        if (m_bTrack && pExp->track_function)
          *fixup = (void*)(pExp->track_function);
        else
          *fixup = (void*)(pExp->function);
        return 1;
        return 1;
      }
      return 0;
    }
  }
  return 0;
}

int DllLoader::ResolveName(char *sName, char* sFunction, void **fixup)
{
  int iSize = g_dlls.GetNrOfModules();

  for (int i = 0; i < iSize; i++)
  {
    DllLoader* pDll = g_dlls.GetModule(i);
    if (pDll && stricmp(sName, pDll->GetName()) == 0)
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
      return 0;
    }
  }
  return 0;
}

char* DllLoader::GetName()
{
  if (m_sFileName)
  {
    char* sName = strrchr(m_sFileName, '\\');
    if (sName) return sName + 1;
    else return m_sFileName;
  }
  return "";
}

char* DllLoader::GetFileName()
{
  if (m_sFileName) return m_sFileName;
  return "";
}
  
int DllLoader::IncrRef()
{
  m_iRefCount++;
  return m_iRefCount;
}

int DllLoader::DecrRef()
{
  m_iRefCount--;
  return m_iRefCount;
}

void DllLoader::AddExport(unsigned long ordinal, unsigned long function, void* track_function)
{
  Export* export = new Export;
  export->function = function;
  export->ordinal = ordinal;
  export->track_function = track_function;
  export->sFunctionName = NULL;
  
  ExportList* entry = new ExportList;
  entry->pExport = export;
  entry->pNext = m_pExports;
  m_pExports = entry;
}

void DllLoader::AddExport(char* sFunctionName, unsigned long ordinal, unsigned long function, void* track_function)
{
  Export* export = new Export;
  export->function = function;
  export->ordinal = ordinal;
  export->track_function = track_function;
  export->sFunctionName = strdup(sFunctionName);
  
  ExportList* entry = new ExportList;
  entry->pExport = export;
  entry->pNext = m_pExports;
  m_pExports = entry;
}

void DllLoader::AddExport(char* sFunctionName, unsigned long function, void* track_function)
{
  Export* export = new Export;
  export->function = function;
  export->ordinal = -1;
  export->track_function = track_function;
  export->sFunctionName = strdup(sFunctionName);
  
  ExportList* entry = new ExportList;
  entry->pExport = export;
  entry->pNext = m_pExports;
  m_pExports = entry;
}
