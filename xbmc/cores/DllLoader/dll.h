#ifndef __DLL_H_
#define __DLL_H_
//#pragma message("including dll.h")
#include <stdlib.h>
#include <xtl.h>

#include "coffldr.h"
#include "exp2dll.h"

//#define DUMPING_DATA

class DllLoader : public CoffLoader {
protected:
// Just pointers; dont' delete...
	ImportDirTable_t	*ImportDirTable;
	ExportDirTable_t	*ExportDirTable;
	char	*Dll;
	int		refcount;
	typedef struct _ExpList {
		Exp2Dll* exp;
		_ExpList* next;
	} ExpList;
	ExpList				*Exports;

#ifdef DUMPING_DATA
	void    PrintImportLookupTable(unsigned long ImportLookupTable_RVA);
	void    PrintImportTable(ImportDirTable_t *ImportDirTable);
#endif
	void    PrintExportTable(ExportDirTable_t *ExportDirTable);
	int     ResolveOrdinal(char*, unsigned long, void**);
	int     ResolveName(char*, char*, void **);
	int     LoadExports();
	
public:
    DllLoader(const char *dll, bool track = false);
    ~DllLoader();

	int     Parse(void);
	int     ResolveImports(void);
	int     ResolveExport(char*, void**);
	char*	GetDLLName(void);
	int		IncrRef();
	int		DecrRef();
};

typedef BOOL WINAPI EntryFunc(HINSTANCE hinstDLL,DWORD fdwReason,LPVOID lpvReserved);

#endif