
#pragma once

//#include <stdlib.h>
//#include <xtl.h>

#include "coffldr.h"
#include "exp2dll.h"

//#define DUMPING_DATA

class DllLoader : public CoffLoader
{
public:
  DllLoader(const char *dll, bool track = false, bool bSystemDll = false);
  ~DllLoader();

  int Parse();
  int ResolveImports();
  int ResolveExport(const char*, void**);
  char* GetName(); // eg "mplayer.dll"
  char* GetFileName(); // "Q:\system\mplayer\players\mplayer.dll"
  int IncrRef();
  int DecrRef();
  
  Exp2Dll* GetExportByOrdinal(unsigned long ordinal);
  Exp2Dll* GetExportByFunctionName(const char* sFunctionName);
  bool IsSystemDll() { return m_bSystemDll; };
  
  void AddExport(unsigned long ordinal, unsigned long function);
  void AddExport(char* sFunctionName, unsigned long ordinal, unsigned long function);
  void AddExport(char* sFunctionName, unsigned long function);

private:
  // Just pointers; dont' delete...
  ImportDirTable_t *ImportDirTable;
  ExportDirTable_t *ExportDirTable;
  char* m_sFileName;
  int m_iRefCount;
  bool m_bTrack;
  bool m_bSystemDll; // true if this dll should not be removed
  typedef struct _ExportList
  {
    Exp2Dll* pExport;
    _ExportList* pNext;
  } ExportList;
  ExportList* m_pExports;

  void PrintImportLookupTable(unsigned long ImportLookupTable_RVA);
  void PrintImportTable(ImportDirTable_t *ImportDirTable);
  void PrintExportTable(ExportDirTable_t *ExportDirTable);
  
  int ResolveOrdinal(char*, unsigned long, void**);
  int ResolveName(char*, char*, void **);
  int LoadExports();
};
