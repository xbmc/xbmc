
#pragma once

#include "coffldr.h"
#include "LibraryLoader.h"

class DllLoader;


typedef struct Export
{
  const char*   name;
  unsigned long ordinal;
  void*         function;
  void*         track_function;
} Export;

typedef struct ExportEntry
{
  Export exp;
  ExportEntry* next;
} ExportEntry;

typedef struct _LoadedList
{
  DllLoader* pDll;
  _LoadedList* pNext;
} LoadedList;
  
class DllLoader : public CoffLoader, public LibraryLoader
{
public:
  DllLoader(const char *dll, bool track = false, bool bSystemDll = false, bool bLoadSymbols = false, Export* exports = NULL);
  virtual ~DllLoader();

  virtual bool Load();
  virtual void Unload();

  virtual int ResolveExport(const char*, void** ptr);
  virtual int ResolveExport(unsigned long ordinal, void** ptr);
  virtual bool HasSymbols() { return m_bLoadSymbols && !m_bUnloadSymbols; }
  virtual bool IsSystemDll() { return m_bSystemDll; }
  virtual HMODULE GetHModule() { return (HMODULE)hModule; }  
  
protected:  
  int Parse();
  int ResolveImports();

  Export* GetExportByOrdinal(unsigned long ordinal);
  Export* GetExportByFunctionName(const char* sFunctionName);
  
  void AddExport(unsigned long ordinal, void* function, void* track_function = NULL);
  void AddExport(char* sFunctionName, unsigned long ordinal, void* function, void* track_function = NULL);
  void AddExport(char* sFunctionName, void* function, void* track_function = NULL);
  void SetExports(Export* exports) { m_pStaticExports = exports; }

private:
  // Just pointers; dont' delete...
  ImportDirTable_t *ImportDirTable;
  ExportDirTable_t *ExportDirTable;
  bool m_bTrack;
  bool m_bSystemDll; // true if this dll should not be removed
  bool m_bLoadSymbols; // when true this dll should not be removed
  bool m_bUnloadSymbols;
  ExportEntry* m_pExportHead;
  Export* m_pStaticExports;
  LoadedList* m_pDlls;

  void PrintImportLookupTable(unsigned long ImportLookupTable_RVA);
  void PrintImportTable(ImportDirTable_t *ImportDirTable);
  void PrintExportTable(ExportDirTable_t *ExportDirTable);
  
  int ResolveOrdinal(char*, unsigned long, void**);
  int ResolveName(char*, char*, void **);
  char* ResolveReferencedDll(char* dll);
  int LoadExports();
  void LoadSymbols();
  void UnloadSymbols();
};
