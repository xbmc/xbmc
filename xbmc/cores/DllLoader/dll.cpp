#include "stdafx.h"
#include <stdlib.h>

#include <stdio.h>
#include <memory>
#include <map>
#include <list>
#include <vector>

#include <string.h>
#include "dll.h"
#include "exp2dll.h"

#include "../../utils/log.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define DEFAULT_DLLPATH "Q:\\system\\players\\mplayer\\codecs"

#define MODULE_HANDLES_START     0xf3f30120
#define MODULE_HANDLE_kernel32  0xf3f30120   //kernel32.dll magic number
#define MODULE_HANDLE_user32    0xf3f30130   //USER32.dll magic number
#define MODULE_HANDLE_ddraw     0xf3f30140   //ddraw.dll magic number
#define MODULE_HANDLE_wininet   0xf3f30150   //WININET.dll magic number
#define MODULE_HANDLE_advapi32  0xf3f30160   //ADVAPI32.dll magic number
#define MODULE_HANDLE_ws2_32    0xf3f30170   //WS2_32.dll magic number// winsock2
#define MODULE_HANDLES_END       0xf3f30170

#define MODULE_HANDLE_IS_DLLFILE(h) (!((HMODULE)h >= (HMODULE)MODULE_HANDLES_START && \
                                       (HMODULE)h <= (HMODULE)MODULE_HANDLES_END))

using namespace std;
Exp2Dll *Head = 0;

void* fs_seg = NULL;
vector<DllLoader *> m_vecDlls;

// Allocation tracking for vis modules that leak

struct AllocLenCaller
{
  unsigned size;
  unsigned calleraddr;
};

struct MSizeCount
{
  unsigned size;
  unsigned count;
};

std::map<unsigned, MSizeCount> Calleradrmap;

struct DllTrackInfo
{
  DllLoader* pDll;
  unsigned MinAddr;
  unsigned MaxAddr;
  std::map<unsigned, AllocLenCaller> AllocList;

  // list with dll's that are loaded by this dll
  std::list<HMODULE> DllList;
};

int ResolveName(char*, char*, void **);
extern "C" HMODULE __stdcall dllLoadLibraryExtended(LPCSTR file, LPCSTR sourcedll);
extern "C" BOOL __stdcall dllFreeLibrary(HINSTANCE hLibModule);

std::list<DllTrackInfo> TrackedDlls;
typedef std::list<DllTrackInfo>::iterator TrackedDllsIter;

// temporarily hack to free all mplayer dll's
// not used currently, but it can be uncommented if mplayer is not freeing
// all dll's it has used (this could be possible, but I didn't noticed it yet).
// basicly it removes all dll's which have the name 'mplayer' in it. After that
// all reference to this dll are deleted from the tracked dll lists
void tracker_free_mplayer_dlls()
{ /*
    char strDllName[MAX_PATH + 2];
    
    // find mplayer dll's first
    vector<DllLoader *>::iterator it = m_vecDlls.begin();
    while (it != m_vecDlls.end())
    {
      if (MODULE_HANDLE_IS_DLLFILE(*it))
      {
        DllLoader* pDll = (DllLoader*)*it;
        
        strcpy(strDllName, pDll->GetDLLName());
        
        // lower
        for (unsigned int i = 0; i < strlen(strDllName); i++)
        {
          strDllName[i] = tolower(strDllName[i]);
        }
        // ony take the dll's which have mplayer in it, for ex. (q:\\mplayer\\wmamod.dll)
        if (strstr(strDllName, "mplayer") != NULL)
        {
          // remove it from the dll list first
          it = m_vecDlls.erase(it);
                
          // free this library until refcount == 1
          while(dllFreeLibrary((HMODULE)*it) < 1);
          
          // time to remove all references from all tracked dll's now
          for (TrackedDllsIter itt = TrackedDlls.begin(); itt != TrackedDlls.end(); ++itt)
         {
           std::list<HMODULE>::iterator p = itt->DllList.begin();
           while (p != itt->DllList.end())
           {
             if (pDll == (DllLoader*)*p) p = itt->DllList.erase(p);
             else p++;
           }
         } 
        }
        else it++;
      }
      else it++;
    }*/
}

inline std::map<unsigned, AllocLenCaller>* get_track_list(unsigned addr)
{
  for (TrackedDllsIter it = TrackedDlls.begin(); it != TrackedDlls.end(); ++it)
  {
    if (addr >= it->MinAddr && addr <= it->MaxAddr)
    {
      return &it->AllocList;
    }
  }
  return NULL;
}

inline std::list<HMODULE>* get_track_list_dll(unsigned addr)
{
  for (TrackedDllsIter it = TrackedDlls.begin(); it != TrackedDlls.end(); ++it)
  {
    if (addr >= it->MinAddr && addr <= it->MaxAddr)
    {
      return &it->DllList;
    }
  }
  return NULL;
}

inline char* getpath(char *buf, const char *full)
{
  char* pos;
  if (pos = strrchr(full, '\\'))
  {
    strncpy(buf, full, pos - full);
    buf[pos - full] = 0;
    return buf;
  }
  else
    return NULL;
}

inline char* get_track_dll_path(unsigned addr)
{
  for (TrackedDllsIter it = TrackedDlls.begin(); it != TrackedDlls.end(); ++it)
  {
    if (addr >= it->MinAddr && addr <= it->MaxAddr)
    {
      return it->pDll->GetDLLName();
    }
  }
  return NULL;
}


extern "C" void* __cdecl track_malloc(size_t s)
{
  unsigned loc;
  __asm mov eax, [ebp + 4]
  __asm mov loc, eax

  std::map<unsigned, AllocLenCaller>* pList = get_track_list(loc);

  void* p = malloc(s);
  if (!p) 
  {    
    CLog::Log(LOGDEBUG, "DLL: %s : malloc failed, crash imminent", get_track_dll_path(loc));
    return NULL;
  }
  if (pList)
  {
    AllocLenCaller temp;
    temp.size = s;
    temp.calleraddr = loc;
    (*pList)[(unsigned)p] = temp;
  }
  return p;
}

extern "C" void* __cdecl track_calloc(size_t n, size_t s)
{
  unsigned loc;
  __asm mov eax, [ebp + 4]
  __asm mov loc, eax

  std::map<unsigned, AllocLenCaller>* pList = get_track_list(loc);

  void* p = calloc(n, s);
  if (!p) 
  {    
    CLog::Log(LOGDEBUG, "DLL: %s : calloc failed, crash imminent", get_track_dll_path(loc));
    return NULL;
  }
  if (pList)
  {
    AllocLenCaller temp;
    temp.size = n * s;
    temp.calleraddr = loc;
    (*pList)[(unsigned)p] = temp;
  }
  return p;
}

extern "C" void* __cdecl track_realloc(void* p, size_t s)
{
  unsigned loc;
  __asm mov eax, [ebp + 4]
  __asm mov loc, eax

  std::map<unsigned, AllocLenCaller>* pList = get_track_list(loc);

  void* q = realloc(p, s);
  if (!q) 
  {
    //  a dll may realloc with a size of 0, so NULL is the correct return value is this case
    if (s>0) CLog::Log(LOGDEBUG, "DLL: %s : realloc failed, crash imminent", get_track_dll_path(loc));
    return NULL;
  }
  if (pList)
  {
    if (p != q) pList->erase((unsigned)p);
    AllocLenCaller temp;
    temp.size = s;
    temp.calleraddr = loc;
    (*pList)[(unsigned)q] = temp;
  }
  return q;
}

extern "C" void __cdecl track_free(void* p)
{
  unsigned loc;
  __asm mov eax, [ebp + 4]
  __asm mov loc, eax

  std::map<unsigned, AllocLenCaller>* pList = get_track_list(loc);

  if (!pList || pList->erase((unsigned)p) < 1)
  {
    // unable to delete the pointer from one of the trackers, but track_free is called!!
    // This will happen when memory is freed by another dll then the one which allocated the memory.
    // We, have to search every map for this memory pointer.
    // Yes, it's slow todo, but if we are freeing already freed pointers when unloading a dll
    // xbmc will crash.
    std::map<unsigned, AllocLenCaller>* pList;

    for (TrackedDllsIter it = TrackedDlls.begin(); it != TrackedDlls.end(); ++it)
    {
      pList = &it->AllocList;
      // try to free the pointer from this list, and break if success
      if (pList->erase((unsigned)p) > 0) break;
    }
  }

  free(p);
}

extern "C" char* __cdecl track_strdup( const char* str)
{
  unsigned loc;
  __asm mov eax, [ebp + 4]
  __asm mov loc, eax

  std::map<unsigned, AllocLenCaller>* pList = get_track_list(loc);

  char* pdup;
  pdup = strdup(str);

  if (pdup && pList)
  {
    AllocLenCaller temp;
    temp.size = 0;
    temp.calleraddr = loc;
    (*pList)[(unsigned)pdup] = temp;
  }

  return pdup;
}

extern "C" HMODULE __stdcall track_LoadLibraryA(LPCSTR file)
{
  unsigned loc;
  __asm mov eax, [ebp + 4]
  __asm mov loc, eax

  std::list<HMODULE>* pList = get_track_list_dll(loc);
  HMODULE handle = dllLoadLibraryExtended(file, get_track_dll_path(loc));

  if (pList && handle) pList->push_back(handle);
  return handle;
}

extern "C" BOOL __stdcall track_FreeLibrary(HINSTANCE hLibModule)
{
  unsigned loc;
  __asm mov eax, [ebp + 4]
  __asm mov loc, eax

  std::list<HMODULE>* pList = get_track_list_dll(loc);

  if (pList)
  {
    for (std::list<HMODULE>::iterator it = pList->begin(); it != pList->end(); ++it)
    {
      if (*it == hLibModule)
      {
        pList->erase(it);
        break;
      }
    }
  }

  return dllFreeLibrary(hLibModule);
}

std::list<unsigned long*> AllocatedFunctionList;
int iDllDummyOutputCall = 0;
extern "C" void dll_dummy_output(char* dllname, char* funcname)
{
  CLog::Log(LOGERROR, "%s: Unresolved function called (%s), Count number %d", dllname, funcname, ++iDllDummyOutputCall);
}

// this piece of asm code only calls dll_dummy_output(s, s) and will return NULL
unsigned char dummy_func[] = {
                               0x55,                            // push        ebp
                               0x8b, 0xec,                      // mov         ebp,esp
                               0xa1, 0, 0, 0, 0,                // mov         eax,dword ptr [0 0 0 0]
                               0x50,                            // push        eax
                               0xa1, 0, 0, 0, 0,                // mov         eax,dword ptr [0 0 0 0]
                               0x50,                            // push        eax
                               0xff, 0x15, 0, 0, 0, 0,          // call        dword ptr[dll_dummy_output]
                               0x83, 0xc4, 0x08,                // add         esp,8
                               0x33, 0xc0,                      // xor         eax,eax       // return NULL
                               0x5d,                            // pop         ebp
                               0xc3                            // ret
                             };

/* Create a new callable function
 * This allocates a few bytes with the next content
 * 
 * 1 function in assembly code (dummy_func)
 * 2 datapointer               (pointer to dll string)
 * 3 datapointer               (pointer to function string)
 * 4 datapointer               (pointer to function string)
 * 5 string                    (string of chars representing dll name)
 * 6 string                    (string of chars representing function name)
 */
void MakeDummyFunction(unsigned long* Addr, const char* strDllName, const char* strFunctionName)
{
  unsigned int iFunctionSize = sizeof(dummy_func);
  unsigned int iDllNameSize = strlen(strDllName) + 1;
  unsigned int iFunctionNameSize = strlen(strFunctionName) + 1;

  // allocate memory for function + strings + 3 x 4 bytes for three datapointers
  char* pData = new char[iFunctionSize + 12 + iDllNameSize + iFunctionNameSize];

  char* offDataPointer1 = pData + iFunctionSize;
  char* offDataPointer2 = pData + iFunctionSize + 4;
  char* offDataPointer3 = pData + iFunctionSize + 8;
  char* offStringDll = pData + iFunctionSize + 12;
  char* offStringFunc = pData + iFunctionSize + 12 + iDllNameSize;

  // 1 copy assembly code
  memcpy(pData, dummy_func, iFunctionSize);

  // insert pointers to datapointers into assembly code (fills 0x00000000 in dummy_func)
  *(int*)(pData + 4) = (int)offDataPointer1;
  *(int*)(pData + 10) = (int)offDataPointer2;
  *(int*)(pData + 17) = (int)offDataPointer3;

  // 2 fill datapointer with pointer to 5 (string)
  *(int*)offDataPointer1 = (int)offStringFunc;
  // 3 fill datapointer with pointer to 6 (string)
  *(int*)offDataPointer2 = (int)offStringDll;
  // 4 fill datapointer with pointer to dll_dummy_output
  *(int*)offDataPointer3 = (int)dll_dummy_output;

  // copy arguments to 5 (string) and 6 (string)
  memcpy(offStringDll, strDllName, iDllNameSize);
  memcpy(offStringFunc, strFunctionName, iFunctionNameSize);

  // bind new function to Addr
  *Addr = (unsigned long)pData;

  // add it to AllocatedFunctionList so we can free the just created functions if we want
  AllocatedFunctionList.push_back((unsigned long*)pData);
}

#ifdef DUMPING_DATA

void DllLoader::PrintImportLookupTable(unsigned long ImportLookupTable_RVA)
{
  int Sctn = RVA2Section(ImportLookupTable_RVA);
  unsigned long *Table = (unsigned long*)
                         (SectionData[Sctn] + (ImportLookupTable_RVA - SectionHeader[Sctn].VirtualAddress));

  while (*Table)
  {
    if ( *Table & 0x80000000)
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
  if ( !HavePrinted )
    printf("None.\n");
}

#endif

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

    std::map<unsigned, AllocLenCaller>* pList = NULL;
    for (TrackedDllsIter it = TrackedDlls.begin(); it != TrackedDlls.end(); ++it)
    {
      if (it->pDll == this)
      {
        pList = &it->AllocList;
        for (int i = 0; i < NumOfSections; ++i)
        {
          if (!memcmp(SectionHeader[i].Name, ".text", 6))
          {
            it->MinAddr = (unsigned)hModule + SectionHeader[i].VirtualAddress;
            it->MaxAddr = it->MinAddr + SectionHeader[i].VirtualSize;
            break;
          }
        }
        break;
      }
    }

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
            MakeDummyFunction(Addr, Name, szBuf);
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
            MakeDummyFunction(Addr, Name, ImpName);
            bResult = 0;
          }
          else
          {
            if (pList)
            {
              // alter fixup for memory tracking
              if (!strcmp(ImpName, "malloc") || !strcmp(ImpName, "??2@YAPAXI@Z"))
              {
                Fixup = track_malloc;
              }
              else if (!strcmp(ImpName, "calloc"))
              {
                Fixup = track_calloc;
              }
              else if (!strcmp(ImpName, "realloc"))
              {
                Fixup = track_realloc;
              }
              else if (!strcmp(ImpName, "free") || !strcmp(ImpName, "??3@YAXPAX@Z") || !strcmp(ImpName, "??_V@YAXPAX@Z"))
              {
                Fixup = track_free;
              }
              else if (!strcmp(ImpName, "_strdup"))
              {
                Fixup = track_strdup;
              }
              else if (!strcmp(ImpName, "LoadLibraryA"))
              {
                Fixup = track_LoadLibraryA;
              }
              else if (!strcmp(ImpName, "FreeLibrary"))
              {
                Fixup = track_FreeLibrary;
              }
            }
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

      Exp2Dll* exp;
      char* slash = strrchr(Dll, '\\');
      if ( slash )
        exp = new Exp2Dll(slash + 1, Name, Addr);
      else
        exp = new Exp2Dll(Dll, Name, Addr);
      ExpList* entry = new ExpList;
      entry->exp = exp;
      entry->next = Exports;
      Exports = entry;
    }
  }
  return 0;
}

int DllLoader::ResolveExport(char *_Name, void **Addr)
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

      if (stricmp(Name, _Name) == 0 && Addr)
      {
        unsigned long RVA = ExportAddressTable[OrdinalTable[i]];
        int FSctn = RVA2Section(RVA);
        *Addr = (void *)
                (SectionData[FSctn] + (RVA - SectionHeader[FSctn].VirtualAddress));
        return 1;
      }
    }
  }
  char szBuf[250];
  sprintf(szBuf, "Unable to resolve: %s %s\n", strrchr(Dll, '\\') + 1, _Name);
  OutputDebugString(szBuf);
  return 0;
}

int DllLoader::ResolveOrdinal(char *Name, unsigned long Ord, void **Fixup)
{
  Exp2Dll* curr = Head;

  while ( curr )
  {
    if (stricmp(Name, curr->ObjectName) == 0 &&
        Ord == curr->Ordinal)
    {
      *Fixup = (void*)(curr->Function);
      return 1;
    }
    curr = curr->Next;
  }
  return 0;
}

int DllLoader::ResolveName(char *Name, char* Function, void **Fixup)
{
  Exp2Dll* curr = Head;

  while ( curr )
  {
    if (stricmp(Name, curr->ObjectName) == 0 &&
        strcmp(Function, curr->FunctionName) == 0)
    {
      *Fixup = (void*)(curr->Function);
      return 1;
    }
    curr = curr->Next;
  }
  return 0;
}

DllLoader::DllLoader(const char *_dll, bool track)
{
  ImportDirTable = 0;
  Dll = new char[strlen(_dll) + 1];
  refcount = 1;
  strcpy(Dll, _dll);
  Exports = 0;

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

  if (track)
  {
    DllTrackInfo TrackInfo;
    TrackInfo.pDll = this;
    TrackInfo.MinAddr = TrackInfo.MaxAddr = 0;
    TrackedDlls.push_back(TrackInfo);
  }
}

DllLoader::~DllLoader()
{
  while ( Exports )
  {
    ExpList* entry = Exports;
    Exports = entry->next;
    delete entry->exp;
    delete entry;
  } /*
    while(Head )
    {
    Exp2Dll* entry = Head;
    Head = entry->Next;
    delete entry;
    }
    Head=NULL;*/
  for (TrackedDllsIter it = TrackedDlls.begin(); it != TrackedDlls.end(); ++it)
  {
    if (it->pDll == this)
    {
      if (!it->AllocList.empty())
      {
        CLog::DebugLog("%s: Detected memory leaks: %d leaks", Dll, it->AllocList.size());
        unsigned total = 0;
        std::map<unsigned, MSizeCount>::iterator itt;
        for (std::map<unsigned, AllocLenCaller>::iterator p = it->AllocList.begin(); p != it->AllocList.end(); ++p)
        {
          total += (p->second).size;
          free((void*)p->first);
          if ( ( itt = Calleradrmap.find((p->second).calleraddr) ) != Calleradrmap.end() )
          {
            (itt->second).size += (p->second).size;
            (itt->second).count++;
          }
          else
          {
            MSizeCount temp;
            temp.size = (p->second).size;
            temp.count = 1;
            Calleradrmap.insert( make_pair( (p->second).calleraddr, temp ) );
          }
        }
        for ( itt = Calleradrmap.begin(); itt != Calleradrmap.end();++itt )
        {
          CLog::DebugLog("leak caller address %8x, size %8i, counter %4i", itt->first, (itt->second).size, (itt->second).count);
        }
        CLog::DebugLog("%s: Total bytes leaked: %d", Dll, total);
        Calleradrmap.erase(Calleradrmap.begin(), Calleradrmap.end());
      }

      // unloading unloaded dll's
      if (!it->DllList.empty())
      {
        CLog::DebugLog("%s: Detected %d unloaded dll's", Dll, it->DllList.size());
        for (std::list<HMODULE>::iterator p = it->DllList.begin(); p != it->DllList.end(); ++p)
        {
          DllLoader* pDll = (DllLoader*) * p;
          if (pDll != NULL &&
              pDll != (DllLoader*)MODULE_HANDLE_kernel32 &&
              pDll != (DllLoader*)MODULE_HANDLE_user32 &&
              pDll != (DllLoader*)MODULE_HANDLE_ddraw &&
              pDll != (DllLoader*)MODULE_HANDLE_wininet &&
              pDll != (DllLoader*)MODULE_HANDLE_advapi32 &&
              pDll != (DllLoader*)MODULE_HANDLE_ws2_32)
          {
            if (strlen(pDll->GetDLLName()) > 0) CLog::DebugLog("  : %s", pDll->GetDLLName());
          }
        }
        for (std::list<HMODULE>::iterator p = it->DllList.begin(); p != it->DllList.end(); ++p)
        {
          if (*p) dllFreeLibrary(*p);
        }
      }

      it = TrackedDlls.erase(it);
      break;
    }
  }

  ImportDirTable = 0;
  delete [] Dll;

  // free all functions which where created at the time we loaded the dll
  std::list<unsigned long*>::iterator unIt = AllocatedFunctionList.begin();
while (unIt != AllocatedFunctionList.end()) { delete[](*unIt); unIt++; }
  AllocatedFunctionList.clear();
}

char* DllLoader::GetDLLName()
{
  return Dll ? Dll : "";
}

int DllLoader::IncrRef()
{
  refcount++;
  return refcount;
}

int DllLoader::DecrRef()
{
  refcount--;
  return refcount;
}

int DllLoader::Parse()
{
  FILE *fp;

  fp = fopen(Dll, "rb");
  if ( fp == NULL )
    return 0;

  if ( !CoffLoader::ParseCoff(fp) )
  {
    fclose(fp);
    return 0;
  }

  fclose(fp);
  LoadExports();

  return 1;
}



Exp2Dll::Exp2Dll(char* ObjName, unsigned long Ord, unsigned long Func)
{
  Exp2Dll** curr;
  ObjectName = new char[strlen(ObjName) + 1];
  FunctionName = NULL;
  Ordinal = Ord;
  strcpy(ObjectName, ObjName);
  Function = Func;
  Next = 0;

  curr = &Head;
  while ( *curr ) curr = &((*curr)->Next);
  *curr = this;
}

Exp2Dll::Exp2Dll(char* ObjName, char* FuncName, unsigned long Ord, unsigned long Func)
{
  Exp2Dll** curr;
  ObjectName = new char[strlen(ObjName) + 1];
  FunctionName = new char[strlen(FuncName) + 1];
  Ordinal = Ord;
  strcpy(ObjectName, ObjName);
  strcpy(FunctionName, FuncName);
  Function = Func;
  Next = 0;

  curr = &Head;
  while ( *curr ) curr = &((*curr)->Next);
  *curr = this;
}

Exp2Dll::Exp2Dll(char* ObjName, char* FuncName, unsigned long Func)
{
  Exp2Dll** curr;
  ObjectName = new char[strlen(ObjName) + 1];
  FunctionName = new char[strlen(FuncName) + 1];
  Ordinal = (unsigned long)( -1);
  strcpy(ObjectName, ObjName);
  strcpy(FunctionName, FuncName);
  Function = Func;
  Next = 0;

  curr = &Head;
  while ( *curr ) curr = &((*curr)->Next);
  *curr = this;
}

Exp2Dll::~Exp2Dll()
{

  if ( ObjectName) delete [] ObjectName;
  ObjectName = NULL;
  if ( FunctionName) delete [] FunctionName;
  FunctionName = NULL;

  Exp2Dll** curr;

  curr = &Head;
  while ( *curr && *curr != this ) curr = &((*curr)->Next);
  if ( *curr )
  {
    *curr = Next;
    Next = 0;
  }
}

void RemoveAllDllImports()
{
  Exp2Dll* curr = Head;

  Exp2Dll* next;
  while ( curr )
  {
    next=curr->Next;
    delete curr;
    curr = next;
  }
}

extern "C" HMODULE __stdcall dllLoadLibraryExtended(LPCSTR file, LPCSTR sourcedll)
{
  // we skip to the last backslash
  // this is effectively eliminating weird characters in
  // the text output windows

  char* libname = strrchr(file, '\\');
  if (libname) libname++;
  else libname = (char*)file;

  char llibname[MAX_PATH + 1]; // lower case
  strcpy(llibname, libname);
  strlwr(llibname);

  CLog::Log(LOGDEBUG, "LoadLibraryA('%s')", libname);
  if (strcmp(llibname, "kernel32.dll") == 0) return (HMODULE)MODULE_HANDLE_kernel32;
  if (strcmp(llibname, "user32.dll") == 0) return (HMODULE)MODULE_HANDLE_user32;
  if (strcmp(llibname, "ddraw.dll") == 0) return (HMODULE)MODULE_HANDLE_ddraw;
  if (strcmp(llibname, "wininet.dll") == 0) return (HMODULE)MODULE_HANDLE_wininet;
  if (strcmp(llibname, "advapi32.dll") == 0) return (HMODULE)MODULE_HANDLE_advapi32;
  if (strcmp(llibname, "ws2_32.dll") == 0 || strcmp(llibname, "ws2_32") == 0) return (HMODULE)MODULE_HANDLE_ws2_32;

  char pfile[MAX_PATH + 1];
  if (strlen(file) > 1 && file[1] == ':')
    sprintf(pfile, "%s", (char *)file);
  else
  {
    if (sourcedll)
    {
      //Use calling dll's path as base address for this call
      char path[MAX_PATH + 1];
      getpath(path, sourcedll);

      //Handle mplayer case specially
      //it has all it's dlls in a codecs subdirectory
      if (strstr(sourcedll, "mplayer.dll"))
        sprintf(pfile, "%s\\codecs\\%s", path, (char *)libname);
      else
        sprintf(pfile, "%s\\%s", path, (char *)libname);
    }
    else
      sprintf(pfile, "%s\\%s", DEFAULT_DLLPATH , (char *)libname);
  }

  // Check m_vecDlls vector if dll is already loaded and return its handle
  for (unsigned int i = 0; i < m_vecDlls.size(); i++)
  {
    DllLoader* dll = m_vecDlls[i];
    if (strncmp(pfile, dll->GetDLLName(), strlen(pfile)) == 0)
    {
      CLog::Log(LOGDEBUG, "%s already loaded -> 0x%x", dll->GetDLLName(), dll);
      dll->IncrRef();
      return (HMODULE) dll;
    }
  }

  // enable memory tracking by default
  DllLoader * dllhandle = new DllLoader(pfile, true);

  int hr = dllhandle->Parse();
  if (hr == 0)
  {
    CLog::Log(LOGERROR, "Failed to open %s, check codecs.conf and file existence.\n", pfile);
    delete dllhandle;
    return NULL;
  }

  dllhandle->ResolveImports();

  // only execute DllMain if no EntryPoint is found
  if (!dllhandle->EntryAddress)
  {
    void* address = NULL;
    dllhandle->ResolveExport("DllMain", &address);
    if (address) dllhandle->EntryAddress = (unsigned long)address;
  }
  else
  {
    CLog::Log(LOGDEBUG, "Executing EntryPoint at: 0x%x - Dll: %s", dllhandle->EntryAddress, libname);
  }

  // patch some unwanted calls in memory
  if (strstr(libname, "QuickTime.qts") && dllhandle)
  {
    int i;
    DWORD dispatch_addr;
    DWORD imagebase_addr;
    DWORD dispatch_rva;

    dllhandle->ResolveExport("theQuickTimeDispatcher", (void **)&dispatch_addr);
    imagebase_addr = (DWORD)dllhandle->hModule;
    CLog::Log(LOGDEBUG, "Virtual Address of theQuickTimeDispatcher = 0x%x", dispatch_addr);
    CLog::Log(LOGDEBUG, "ImageBase of %s = 0x%x", libname, imagebase_addr);

    dispatch_rva = dispatch_addr - imagebase_addr;

    CLog::Log(LOGDEBUG, "Relative Virtual Address of theQuickTimeDispatcher = %p", dispatch_rva);

    DWORD base = imagebase_addr;
    if (dispatch_rva == 0x124C30)
    {
      CLog::Log(LOGINFO, "QuickTime5 DLLs found\n");
      for (i = 0;i < 5;i++) ((BYTE*)base + 0x19e842)[i] = 0x90; // make_new_region ?
      for (i = 0;i < 28;i++) ((BYTE*)base + 0x19e86d)[i] = 0x90; // call__call_CreateCompatibleDC ?
      for (i = 0;i < 5;i++) ((BYTE*)base + 0x19e898)[i] = 0x90; // jmp_to_call_loadbitmap ?
      for (i = 0;i < 9;i++) ((BYTE*)base + 0x19e8ac)[i] = 0x90; // call__calls_OLE_shit ?
      for (i = 0;i < 106;i++) ((BYTE*)base + 0x261B10)[i] = 0x90; // disable threads
    }
    else if (dispatch_rva == 0x13B330)
    {
      CLog::Log(LOGINFO, "QuickTime6 DLLs found\n");
      for (i = 0;i < 5;i++) ((BYTE*)base + 0x2730CC)[i] = 0x90; // make_new_region
      for (i = 0;i < 28;i++) ((BYTE*)base + 0x2730f7)[i] = 0x90; // call__call_CreateCompatibleDC
      for (i = 0;i < 5;i++) ((BYTE*)base + 0x273122)[i] = 0x90; // jmp_to_call_loadbitmap
      for (i = 0;i < 9;i++) ((BYTE*)base + 0x273131)[i] = 0x90; // call__calls_OLE_shit
      for (i = 0;i < 96;i++) ((BYTE*)base + 0x2AC852)[i] = 0x90; // disable threads
    }
    else if (dispatch_rva == 0x13C3E0)
    {
      CLog::Log(LOGINFO, "QuickTime6.3 DLLs found\n");
      for (i = 0;i < 5;i++) ((BYTE*)base + 0x268F6C)[i] = 0x90; // make_new_region
      for (i = 0;i < 28;i++) ((BYTE*)base + 0x268F97)[i] = 0x90; // call__call_CreateCompatibleDC
      for (i = 0;i < 5;i++) ((BYTE*)base + 0x268FC2)[i] = 0x90; // jmp_to_call_loadbitmap
      for (i = 0;i < 9;i++) ((BYTE*)base + 0x268FD1)[i] = 0x90; // call__calls_OLE_shit
      for (i = 0;i < 96;i++) ((BYTE*)base + 0x2B4722)[i] = 0x90; // disable threads
    }
    else
    {
      CLog::Log(LOGERROR, "Unsupported QuickTime version");
      //return 0;
    }

    CLog::Log(LOGINFO, "QuickTime.qts patched!!!\n");
  }

  EntryFunc* initdll = (EntryFunc *)dllhandle->EntryAddress;
  (*initdll)((HINSTANCE) dllhandle, 1 , 0); //call "DllMain" with DLL_PROCESS_ATTACH

  //#define DLL_PROCESS_ATTACH   1
  //#define DLL_THREAD_ATTACH    2
  //#define DLL_THREAD_DETACH    3
  //#define DLL_PROCESS_DETACH   0
  //#define DLL_PROCESS_VERIFIER 4

  CLog::Log(LOGDEBUG, "LoadLibrary('%s') returning: 0x%x", libname, dllhandle);

  // Add dll to m_vecDlls
  m_vecDlls.push_back(dllhandle);
  return (HMODULE) dllhandle;
}

extern "C" HMODULE __stdcall dllLoadLibraryA(LPCSTR file)
{
  unsigned loc;
  __asm mov eax, [ebp + 4]
  __asm mov loc, eax

  return dllLoadLibraryExtended(file, get_track_dll_path(loc));
}

#define DONT_RESOLVE_DLL_REFERENCES   0x00000001
#define LOAD_LIBRARY_AS_DATAFILE      0x00000002
#define LOAD_WITH_ALTERED_SEARCH_PATH 0x00000008
#define LOAD_IGNORE_CODE_AUTHZ_LEVEL  0x00000010

extern "C" HMODULE dllLoadLibraryExA(LPCSTR lpLibFileName, HANDLE hFile, DWORD dwFlags)
{
  char strFlags[1024];
  strFlags[0] = '\0';

  if (dwFlags & DONT_RESOLVE_DLL_REFERENCES) strcat(strFlags, "\n - DONT_RESOLVE_DLL_REFERENCES");
  if (dwFlags & LOAD_IGNORE_CODE_AUTHZ_LEVEL) strcat(strFlags, "\n - LOAD_IGNORE_CODE_AUTHZ_LEVEL");
  if (dwFlags & LOAD_LIBRARY_AS_DATAFILE) strcat(strFlags, "\n - LOAD_LIBRARY_AS_DATAFILE");
  if (dwFlags & LOAD_WITH_ALTERED_SEARCH_PATH) strcat(strFlags, "\n - LOAD_WITH_ALTERED_SEARCH_PATH");

  CLog::Log(LOGDEBUG, "LoadLibraryExA called with flags: %s", strFlags);
  return dllLoadLibraryA(lpLibFileName);
}

extern "C" BOOL __stdcall dllFreeLibrary(HINSTANCE hLibModule)
{
  CLog::Log(LOGDEBUG, "FreeLibrary(0x%x)", hLibModule);
  if (hLibModule == (HMODULE)MODULE_HANDLE_kernel32 ||
      hLibModule == (HMODULE)MODULE_HANDLE_user32 ||
      hLibModule == (HMODULE)MODULE_HANDLE_ddraw ||
      hLibModule == (HMODULE)MODULE_HANDLE_wininet ||
      hLibModule == (HMODULE)MODULE_HANDLE_advapi32 ||
      hLibModule == (HMODULE)MODULE_HANDLE_ws2_32)
    return 1;

  DllLoader * dllhandle = (DllLoader *)hLibModule;
  if (dllhandle->DecrRef() > 0)
  {
    CLog::Log(LOGDEBUG, "Cannot FreeLibrary(%s), refcount > 0", dllhandle->GetDLLName());
    return 0;
  }

  EntryFunc * initdll = (EntryFunc *)dllhandle->EntryAddress;
  (*initdll)((HINSTANCE) dllhandle->hModule, DLL_PROCESS_DETACH , 0); //call "DllMain" with DLL_PROCESS_DETACH

  //Remove dll from m_vecDlls
  for (vector<DllLoader*>::iterator iDll = m_vecDlls.begin(); iDll != m_vecDlls.end(); ++iDll)
  {
    if ((*iDll) == dllhandle)
    {
      m_vecDlls.erase(iDll);
      break;
    }
  }

  if (dllhandle) delete dllhandle;
  return 1;
}

extern "C" FARPROC __stdcall dllGetProcAddress( HMODULE hModule, LPCSTR function )
{
  void * address = NULL;
  if ( hModule == (HMODULE)MODULE_HANDLE_kernel32 )
  {
    if ( ResolveName("kernel32.dll", (char *)function, &address) ||
         ResolveName("KERNEL32.DLL", (char *)function, &address) )
    {
      CLog::Log(LOGDEBUG, "KERNEL32!GetProcAddress(0x%x, '%s') => 0x%x", hModule, function, address);
      return (FARPROC) address;
    }
    else
    {
      CLog::Log(LOGDEBUG, "KERNEL32!GetProcAddress(0x%x, '%s') => 0x%x", hModule, function, NULL);
      return (FARPROC) NULL;
    }
  }

  if ( hModule == (HMODULE)MODULE_HANDLE_user32 )
  {
    if ( ResolveName("user32.dll", (char *)function, &address) ||
         ResolveName("USER32.DLL", (char *)function, &address) )
    {
      CLog::Log(LOGDEBUG, "KERNEL32!GetProcAddress(0x%x, '%s') => 0x%x", hModule, function, address);
      return (FARPROC) address;
    }
    else
    {
      CLog::Log(LOGDEBUG, "KERNEL32!GetProcAddress(0x%x, '%s') => 0x%x", hModule, function, NULL);
      return (FARPROC) NULL;
    }
  }

  if ( hModule == (HMODULE)MODULE_HANDLE_ddraw )
  {
    if ( ResolveName("ddraw.dll", (char *)function, &address) ||
         ResolveName("DDRAW.DLL", (char *)function, &address) )
    {
      CLog::Log(LOGDEBUG, "KERNEL32!GetProcAddress(0x%x, '%s') => 0x%x", hModule, function, address);
      return (FARPROC) address;
    }
    else
    {
      CLog::Log(LOGDEBUG, "KERNEL32!GetProcAddress(0x%x, '%s') => 0x%x", hModule, function, NULL);
      return (FARPROC) NULL;
    }
  }

  if ( hModule == (HMODULE)MODULE_HANDLE_wininet )
  {
    if ( ResolveName("wininet.dll", (char *)function, &address) ||
         ResolveName("WININET.DLL", (char *)function, &address) )
    {
      CLog::Log(LOGDEBUG, "KERNEL32!GetProcAddress (0x%x, '%s') => 0x%x", hModule, function, address);
      return (FARPROC) address;
    }
    else
    {
      CLog::Log(LOGDEBUG, "KERNEL32!GetProcAddress(0x%x, '%s') => 0x%x", hModule, function, NULL);
      return (FARPROC) NULL;
    }
  }

  if ( hModule == (HMODULE)MODULE_HANDLE_advapi32 )
  {
    if ( ResolveName("advapi32.dll", (char *)function, &address) ||
         ResolveName("ADVAPI32.DLL", (char *)function, &address) )
    {
      CLog::Log(LOGDEBUG, "KERNEL32!GetProcAddress (0x%x, '%s') => 0x%x", hModule, function, address);
      return (FARPROC) address;
    }
    else
    {
      CLog::Log(LOGDEBUG, "KERNEL32!GetProcAddress(0x%x, '%s') => 0x%x", hModule, function, NULL);
      return (FARPROC) NULL;
    }
  }

  if ( hModule == (HMODULE)MODULE_HANDLE_ws2_32 )
  {
    if (ResolveName("ws2_32.dll", (char *)function, &address) ||
        ResolveName("WS2_32.DLL", (char *)function, &address))
    {
      CLog::Log(LOGDEBUG, "KERNEL32!GetProcAddress (0x%x, '%s') => 0x%x", hModule, function, address);
      return (FARPROC) address;
    }
    else
    {
      CLog::Log(LOGDEBUG, "KERNEL32!GetProcAddress(0x%x, '%s') => 0x%x", hModule, function, NULL);
      return (FARPROC) NULL;
    }
  }


  DllLoader * dllhandle = (DllLoader *)hModule;
  dllhandle->ResolveExport((char *)function, &address);
  CLog::Log(LOGDEBUG, "KERNEL32!GetProcAddress(0x%x, '%s') => 0x%x", hModule, function, address);
  return (FARPROC) address;
}

extern "C" DWORD WINAPI dllGetModuleFileNameA(HMODULE hModule, LPSTR lpFilename, DWORD nSize)
{
  DWORD result = 0;
  if (NULL == hModule)
  {
    strncpy(lpFilename, "xbmc.xbe", nSize);
    CLog::Log(LOGDEBUG, "GetModuleFileNameA(0x%x, 0x%x, %d) => '%s'\n",
              hModule, lpFilename, nSize, lpFilename);
    return 1;
  }

  // Lookup dll handle in m_vecDlls
  for (unsigned int i = 0; i < m_vecDlls.size(); i++)
  {
    DllLoader * dll = m_vecDlls[i];
    if (hModule == (HMODULE) dll)
    {
      strncpy(lpFilename, dll->GetDLLName(), nSize);
      return 1;
    }
  }

  return NULL;
}

extern "C" HMODULE WINAPI dllGetModuleHandleA(LPCSTR lpModuleName)
{
  /*
  If the file name extension is omitted, the default library extension .dll is appended.
  The file name string can include a trailing point character (.) to indicate that the module name has no extension.
  The string does not have to specify a path. When specifying a path, be sure to use backslashes (\), not forward slashes (/).
  The name is compared (case independently)
  If this parameter is NULL, GetModuleHandle returns a handle to the file used to create the calling process (.exe file).
  */
  char* strModuleName = new char[strlen(lpModuleName) + 5];
  strcpy(strModuleName, lpModuleName);

  if (strrchr(strModuleName, '.') == 0) strcat(strModuleName, ".dll");

  if (stricmp(strModuleName, "kernel32.dll") == 0)
  {
    CLog::Log(LOGDEBUG, "KERNEL32!GetModuleHandleA('%s') => 0x%x", lpModuleName, MODULE_HANDLE_kernel32);
    return (HMODULE)MODULE_HANDLE_kernel32;
  }
  if (stricmp(strModuleName, "user32.dll") == 0)
  {
    CLog::Log(LOGDEBUG, "KERNEL32!GetModuleHandleA('%s') => 0x%x", lpModuleName, MODULE_HANDLE_user32);
    return (HMODULE)MODULE_HANDLE_user32;
  }

  if (stricmp(strModuleName, "ddraw.dll") == 0)
  {
    CLog::Log(LOGDEBUG, "KERNEL32!GetModuleHandleA('%s') => 0x%x", lpModuleName, MODULE_HANDLE_ddraw);
    return (HMODULE)MODULE_HANDLE_ddraw;
  }
  if (stricmp(strModuleName, "wininet.dll") == 0)
  {
    CLog::Log(LOGDEBUG, "KERNEL32!GetModuleHandleA('%s') => 0x%x", lpModuleName, MODULE_HANDLE_wininet);
    return (HMODULE)MODULE_HANDLE_wininet;
  }
  if (stricmp(strModuleName, "advapi32.dll") == 0)
  {
    CLog::Log(LOGDEBUG, "KERNEL32!GetModuleHandleA('%s') => 0x%x", lpModuleName, MODULE_HANDLE_advapi32);
    return (HMODULE)MODULE_HANDLE_advapi32;
  }
  if (stricmp(strModuleName, "ws2_32.dll") == 0)
  {
    CLog::Log(LOGDEBUG, "KERNEL32!GetModuleHandleA('%s') => 0x%x", lpModuleName, MODULE_HANDLE_ws2_32);
    return (HMODULE)MODULE_HANDLE_ws2_32;
  }

  delete []strModuleName;

  CLog::Log(LOGDEBUG, "GetModuleHandleA(%s) .. looking up", lpModuleName);

  // Lookup module handle for lpModuleName
  for (unsigned int i = 0; i < m_vecDlls.size(); i++)
  {
    DllLoader * dll = m_vecDlls[i];
    char *lastbc;
    char *dllname;

    dllname = dll->GetDLLName();

    //get filename only
    lastbc = strrchr(dllname, '\\');
    if (lastbc)
    {
      lastbc++;
      dllname = lastbc;
    }
    if (strncmp(lpModuleName, dllname, lstrlen(lpModuleName)) == 0)
    {
      CLog::Log(LOGDEBUG, "GetModuleHandleA(%s) => 0x%x", lpModuleName, dll);
      return (HMODULE)dll;
    }
  }

  return NULL;
}

static int ResolveName(char *Name, char* Function, void **Fixup)
{
  Exp2Dll* curr = Head;

  while (curr)
  {
    if (stricmp(Name, curr->ObjectName) == 0 &&
        strcmp(Function, curr->FunctionName) == 0)
    {
      *Fixup = (void*)(curr->Function);
      return 1;
    }
    curr = curr->Next;
  }
  *Fixup = NULL;
  return 0;
}

