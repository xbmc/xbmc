#include "stdafx.h"
#include <stdlib.h>

#include <stdio.h>
#include <memory>
#include <map>
#include <list>

#include <string.h>
#include "dll.h"
#include "exp2dll.h"
using namespace std;
Exp2Dll *Head = 0;
extern "C" int dummy_Unresolved(void);
extern "C" int dummy_Kernel32Unresolved(void);
extern "C" int dummy_User32Unresolved(void);
extern "C" int dummy_OLE32Unresolved(void);
extern "C" int dummy_MSVCRTUnresolved(void);
extern "C" int dummy_MSVCR71Unresolved(void);
extern "C" int dummy_DX8Unresolved(void);
static int ResolveName(char *Name, char* Function, void **Fixup);

//hack to Free pe image, global variable.
DllLoader * wmaDMOdll;
DllLoader * wmvDMOdll;
DllLoader * wmsDMOdll;


// Allocation tracking for vis modules that leak

struct DllTrackInfo {
	DllLoader* pDll;
	unsigned MinAddr;
	unsigned MaxAddr;
	std::map<unsigned, unsigned> AllocList;
};

std::list<DllTrackInfo> TrackedDlls;
typedef std::list<DllTrackInfo>::iterator TrackedDllsIter;

inline std::map<unsigned,unsigned>* get_track_list(unsigned addr)
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

extern "C" void* __cdecl track_malloc(size_t s)
{
	unsigned loc;
	__asm mov eax,[ebp+4]
	__asm mov loc,eax

	std::map<unsigned,unsigned>* pList = get_track_list(loc);

	void* p = malloc(s);
	if (pList)
		(*pList)[(unsigned)p] = s;
	return p;
}

extern "C" void* __cdecl track_calloc(size_t n, size_t s)
{
	unsigned loc;
	__asm mov eax,[ebp+4]
	__asm mov loc,eax

	std::map<unsigned,unsigned>* pList = get_track_list(loc);

	void* p = calloc(n, s);
	if (pList)
		(*pList)[(unsigned)p] = n * s;
	return p;
}

extern "C" void* __cdecl track_realloc(void* p, size_t s)
{
	unsigned loc;
	__asm mov eax,[ebp+4]
	__asm mov loc,eax

	std::map<unsigned,unsigned>* pList = get_track_list(loc);

	void* q = realloc(p, s);
	if (pList)
	{
		if (p != q)
			pList->erase((unsigned)p);
		(*pList)[(unsigned)q] = s;
	}
	return q;
}

extern "C" void __cdecl track_free(void* p)
{
	unsigned loc;
	__asm mov eax,[ebp+4]
	__asm mov loc,eax

	std::map<unsigned,unsigned>* pList = get_track_list(loc);

	if (pList)
		pList->erase((unsigned)p);
	free(p);
}


#ifdef DUMPING_DATA

void DllLoader::PrintImportLookupTable(unsigned long ImportLookupTable_RVA)
{
	int Sctn = RVA2Section(ImportLookupTable_RVA);
	unsigned long *Table = (unsigned long*)
		(SectionData[Sctn] + (ImportLookupTable_RVA - SectionHeader[Sctn].VirtualAddress));

	while(*Table)
	{
		if( *Table & 0x80000000)
		{
			// Process Ordinal...
			printf("            Ordinal: %01X\n", *Table & 0x7fffffff);
		}
		else
		{
			printf("            Don't process Hint/Name Table yet...\n");
		}
		Table++;
	}
}

void DllLoader::PrintImportTable(ImportDirTable_t *ImportDirTable)
{
	ImportDirTable_t *Imp = ImportDirTable;
	int HavePrinted = 0;

	printf("The Coff Image contains the following imports:\n\n");
	while(  Imp->ImportLookupTable_RVA !=0 ||
		Imp->TimeStamp != 0 ||
		Imp->ForwarderChain != 0 ||
		Imp->Name_RVA != 0 ||
		Imp->ImportAddressTable_RVA != 0)
	{
		char *Name; int Sctn;
		HavePrinted = 1;

		Sctn = RVA2Section(Imp->Name_RVA);
		Name = SectionData[Sctn] + (Imp->Name_RVA - SectionHeader[Sctn].VirtualAddress);
		printf("    %s:\n", Name);
		printf("        ImportAddressTable:     %04X\n", Imp->ImportAddressTable_RVA);
		printf("        ImportLookupTable:      %04X\n", Imp->ImportLookupTable_RVA);
		printf("        TimeStamp:              %01X\n", Imp->TimeStamp);
		printf("        Forwarder Chain:        %01X\n", Imp->ForwarderChain);
		PrintImportLookupTable(Imp->ImportLookupTable_RVA);                   
		printf("\n");
		Imp++;
	}
	if( !HavePrinted )
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
	int bResult=1;
	if( NumOfDirectories >= 2 && Directory[IMPORT_TABLE].Size > 0 )
	{
		int Sctn = RVA2Section(Directory[IMPORT_TABLE].RVA);
		ImportDirTable = (ImportDirTable_t*)
			(SectionData[Sctn] + (Directory[IMPORT_TABLE].RVA - SectionHeader[Sctn].VirtualAddress));
#ifdef DUMPING_DATA
		PrintImportTable(ImportDirTable);
#endif
		ImportDirTable_t *Imp = ImportDirTable;

		std::map<unsigned, unsigned>* pList = NULL;
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

		while(  Imp->ImportLookupTable_RVA !=0 ||
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

			while(*Table)
			{
				if (*Table & 0x80000000)
				{
					void *Fixup;
					if( !ResolveOrdinal(Name, *Table&0x7ffffff, &Fixup) )
					{
						bResult=0;
						char szBuf[128];
						sprintf(szBuf,"unable to resolve ordinal %s %d\n",Name,*Table&0x7ffffff);
						OutputDebugString(szBuf);
						*Addr = (unsigned long) dummy_Unresolved;
					} else { 
					*Addr = (unsigned long)Fixup;  //woohoo!!
					}
				}     
				else
				{
					// We don't handle Hint/Name tables yet!!!
					int ScnName = RVA2Section(*Table + 2);
					char *ImpName = SectionData[ScnName] + (*Table + 2 - SectionHeader[ScnName].VirtualAddress);

					void *Fixup;
					if( !ResolveName(Name, ImpName, &Fixup) )
					{
						char szBuf[128];
						sprintf(szBuf,"unable to resolve %s %s\n",Name,ImpName);
						OutputDebugString(szBuf);
            if (strstr(Name,"KERNEL32")  || strstr(Name,"kernel32") )
						  *Addr = (unsigned long) dummy_Kernel32Unresolved;
            else if (strstr(Name,"USER32")  || strstr(Name,"user32") )
						  *Addr = (unsigned long) dummy_User32Unresolved;
            else if (strstr(Name,"ole32")  || strstr(Name,"ole32") )
						  *Addr = (unsigned long) dummy_OLE32Unresolved;
            else if (strstr(Name,"MSVCRT")  || strstr(Name,"msvcrt") )
						  *Addr = (unsigned long) dummy_MSVCRTUnresolved;
            else if (strstr(Name,"MSVCR71")  || strstr(Name,"msvcr71") )
						  *Addr = (unsigned long) dummy_MSVCR71Unresolved;
            else if (strstr(Name,"xbox_dx8")  || strstr(Name,"XBOX_DX8") )
						  *Addr = (unsigned long) dummy_DX8Unresolved;
            else
						  *Addr = (unsigned long) dummy_Unresolved;
						bResult=0;
					} else {
						if (pList)
						{
							// alter fixup for memory tracking
							if (!strcmp(ImpName, "malloc"))
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
							else if (!strcmp(ImpName, "free"))
							{
								Fixup = track_free;
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
	if( NumOfDirectories >= 1 )
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
			if( slash )
				exp = new Exp2Dll(slash+1, Name, Addr);
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
	if( NumOfDirectories >= 1 )
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
	return 0;
}

int DllLoader::ResolveOrdinal(char *Name, unsigned long Ord, void **Fixup)
{
	Exp2Dll* curr = Head;

	while( curr )
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

	while( curr )
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
	Dll = new char[strlen(_dll)+1];
	strcpy(Dll, _dll);
	Exports = 0;

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
	while( Exports )
	{
		ExpList* entry = Exports;
		Exports = entry->next;
		delete entry->exp;
		delete entry;
	}/*
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
				char temp[128];
				sprintf(temp, "%s: Detected memory leaks: %d leaks\n", Dll, it->AllocList.size());
				OutputDebugString(temp);
				unsigned total = 0;
				for (std::map<unsigned, unsigned>::iterator p = it->AllocList.begin(); p != it->AllocList.end(); ++p)
				{
					total += p->second;
					free((void*)p->first);
				}
				sprintf(temp, "%s: Total bytes leaked: %d\n", Dll, total);
				OutputDebugString(temp);
			}
			TrackedDlls.erase(it);
			break;
		}
	}

	ImportDirTable = 0;
	delete [] Dll;

}

int DllLoader::Parse()
{                                  
	FILE *fp;

	fp = fopen(Dll, "rb");
	if( fp == NULL )
		return 0;

	if( !CoffLoader::ParseCoff(fp) )
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
	ObjectName = new char[strlen(ObjName)+1];      
	FunctionName = NULL;
	Ordinal = Ord;
	strcpy(ObjectName, ObjName);
	Function = Func;
	Next = 0;

	curr = &Head;
	while( *curr ) curr = &((*curr)->Next);
	*curr = this;
}                

Exp2Dll::Exp2Dll(char* ObjName, char* FuncName, unsigned long Ord,unsigned long Func)
{                   
	Exp2Dll** curr;
	ObjectName   = new char[strlen(ObjName)+1];      
	FunctionName = new char[strlen(FuncName)+1];
	Ordinal = Ord;
	strcpy(ObjectName, ObjName);
	strcpy(FunctionName, FuncName);
	Function = Func;
	Next = 0;

	curr = &Head;
	while( *curr ) curr = &((*curr)->Next);
	*curr = this;
} 

Exp2Dll::Exp2Dll(char* ObjName, char* FuncName, unsigned long Func)
{                   
	Exp2Dll** curr;
	ObjectName   = new char[strlen(ObjName)+1];      
	FunctionName = new char[strlen(FuncName)+1];
	Ordinal = (unsigned long)(-1);
	strcpy(ObjectName, ObjName);
	strcpy(FunctionName, FuncName);
	Function = Func;
	Next = 0;

	curr = &Head;
	while( *curr ) curr = &((*curr)->Next);
	*curr = this;
}                
 
Exp2Dll::~Exp2Dll()
{

	if ( ObjectName) delete [] ObjectName;
	ObjectName=NULL;
	if ( FunctionName) delete [] FunctionName;
	FunctionName=NULL;

	Exp2Dll** curr;

	curr = &Head;
	while( *curr && *curr!=this ) curr = &((*curr)->Next);
	if( *curr )
	{
		*curr = Next;
		Next = 0;
	}
}           

#define DEFAULT_DLLPATH "Q:\\mplayer\\codecs"
#define MODULE_HANDLE_kernel32 0xf3f30120			//kernel32.dll magic number

extern "C" HMODULE __stdcall dllLoadLibraryA(LPCSTR libname) 
{
	
	if (strcmp(libname, "kernel32.dll") == 0 || strcmp(libname, "kernel32") == 0 ||
		  strcmp(libname, "KERNEL32.DLL") == 0 || strcmp(libname, "KERNEL32") == 0 )
		return (HMODULE)MODULE_HANDLE_kernel32;
	
	char* plibname = new char[strlen(libname)+120]; 
	sprintf(plibname, "%s\\%s", DEFAULT_DLLPATH ,(char *)libname);
	DllLoader * dllhandle = new DllLoader(plibname);
	delete[] plibname;

	int hr = dllhandle->Parse();
	if ( hr == 0 ) 
	{
		char szBuf[128];
		sprintf(szBuf,"Failed to open %s\\%s, check codecs.conf and file existence.\n",DEFAULT_DLLPATH, libname);
		OutputDebugString(szBuf);	
		delete dllhandle;
		return NULL;
	}

	dllhandle->ResolveImports();
	
	//log bad guys who do not call Freelibrary 
	if (strcmp(libname, "wmadmod.dll") == 0 || strcmp(libname, "WMADMOD.DLL") == 0 )
		wmaDMOdll = dllhandle;
	if (strcmp(libname, "wmvdmod.dll") == 0 || strcmp(libname, "WMVDMOD.DLL") == 0 )
		wmvDMOdll = dllhandle;
	if (strcmp(libname, "wmsdmod.dll") == 0 || strcmp(libname, "WMSDMOD.DLL") == 0 )
		wmsDMOdll = dllhandle;


	void * address = NULL;
	dllhandle->ResolveExport("DllMain", &address);
	if (address)
		dllhandle->EntryAddress = (unsigned long)address;
    EntryFunc * initdll = (EntryFunc *)dllhandle->EntryAddress;
	(*initdll)( (HINSTANCE) dllhandle->hModule, 1 ,0);	//call "DllMian" with DLL_PROCESS_ATTACH
	
	//#define DLL_PROCESS_ATTACH   1    
	//#define DLL_THREAD_ATTACH    2    
	//#define DLL_THREAD_DETACH    3    
	//#define DLL_PROCESS_DETACH   0    
	//#define DLL_PROCESS_VERIFIER 4   

	return (HMODULE) dllhandle;
}

extern "C" BOOL __stdcall dllFreeLibrary(HINSTANCE hLibModule)
{
	if ( hLibModule == (HMODULE)MODULE_HANDLE_kernel32 )
		return 1;
	
	DllLoader * dllhandle = (DllLoader *)hLibModule;

	EntryFunc * initdll = (EntryFunc *)dllhandle->EntryAddress;
	(*initdll)( (HINSTANCE) dllhandle->hModule, 0 ,0);	//call "DllMian" with DLL_PROCESS_DETACH

	if ( dllhandle )
		delete dllhandle;
	return 1;
}

extern "C" FARPROC __stdcall dllGetProcAddress( HMODULE hModule, LPCSTR function )
{
	void * address = NULL;
	if ( hModule == (HMODULE)MODULE_HANDLE_kernel32 )
		{
		if ( ResolveName("kernel32.dll", (char *)function, &address)||
			 ResolveName("KERNEL32.DLL", (char *)function, &address) )
			 return (FARPROC) address;
		else
			return (FARPROC) NULL;
		}

	DllLoader * dllhandle = (DllLoader *)hModule;
	dllhandle->ResolveExport((char *)function, &address);
	return (FARPROC) address;
}
//dummy functions used to catch unresolved function calls
extern "C" int dummy_Unresolved(void) {
		static int Count = 0;
		DWORD rtn_addr;
		char szBuf[128];
		__asm { mov eax, [ebp+4] }
		__asm { mov rtn_addr, eax }
    sprintf(szBuf,"unresolved function called from 0x%08X, Count number %d\n",rtn_addr,Count++);
		OutputDebugString(szBuf);
		return 1;
}
//dummy functions used to catch unresolved function calls
extern "C" int dummy_Kernel32Unresolved(void) {
		static int Count = 0;
		DWORD rtn_addr;
		char szBuf[128];
		__asm { mov eax, [ebp+4] }
		__asm { mov rtn_addr, eax }
    sprintf(szBuf,"kernel32:unresolved function called from 0x%08X, Count number %d\n",rtn_addr,Count++);
		OutputDebugString(szBuf);
		return 1;
}


//dummy functions used to catch unresolved function calls
extern "C" int dummy_User32Unresolved(void) {
		static int Count = 0;
		DWORD rtn_addr;
		char szBuf[128];
		__asm { mov eax, [ebp+4] }
		__asm { mov rtn_addr, eax }
    sprintf(szBuf,"user32:unresolved function called from 0x%08X, Count number %d\n",rtn_addr,Count++);
		OutputDebugString(szBuf);
		return 1;
}

//dummy functions used to catch unresolved function calls
extern "C" int dummy_OLE32Unresolved(void) {
		static int Count = 0;
		DWORD rtn_addr;
		char szBuf[128];
		__asm { mov eax, [ebp+4] }
		__asm { mov rtn_addr, eax }
    sprintf(szBuf,"ole32:unresolved function called from 0x%08X, Count number %d\n",rtn_addr,Count++);
		OutputDebugString(szBuf);
		return 1;
}

//dummy functions used to catch unresolved function calls
extern "C" int dummy_MSVCRTUnresolved(void) {
		static int Count = 0;
		DWORD rtn_addr;
		char szBuf[128];
		__asm { mov eax, [ebp+4] }
		__asm { mov rtn_addr, eax }
    sprintf(szBuf,"ole32:unresolved function called from 0x%08X, Count number %d\n",rtn_addr,Count++);
		OutputDebugString(szBuf);
		return 1;
}


//dummy functions used to catch unresolved function calls
extern "C" int dummy_MSVCR71Unresolved(void) {
		static int Count = 0;
		DWORD rtn_addr;
		char szBuf[128];
		__asm { mov eax, [ebp+4] }
		__asm { mov rtn_addr, eax }
    sprintf(szBuf,"msvcr71.dll:unresolved function called from 0x%08X, Count number %d\n",rtn_addr,Count++);
		OutputDebugString(szBuf);
		return 1;
}


//dummy functions used to catch unresolved function calls
extern "C" int dummy_DX8Unresolved(void) {
		static int Count = 0;
		DWORD rtn_addr;
		char szBuf[128];
		__asm { mov eax, [ebp+4] }
		__asm { mov rtn_addr, eax }
    sprintf(szBuf,"xbox_dx8.dll:unresolved function called from 0x%08X, Count number %d\n",rtn_addr,Count++);
		OutputDebugString(szBuf);
		return 1;
}

static int ResolveName(char *Name, char* Function, void **Fixup)
{
	Exp2Dll* curr = Head;

	while( curr )
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