#include <xtl.h>
#include <stdlib.h>

#include <stdio.h>

#include <string.h>
#include "dll.h"
#include "exp2dll.h"

Exp2Dll *Head = 0;
extern "C" void dummy_Unresolved(void);

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
						printf("unable to resolve %s %x\n",Name,*Table&0x7ffffff);
						return 0;       //  ungh linker error!
					}
					*Addr = (unsigned long)Fixup;  //woohoo!!
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
						*Addr = (unsigned long) dummy_Unresolved;
						bResult=0;
					} else {
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

DllLoader::DllLoader(char *_dll)
{
	ImportDirTable = 0;        
	Dll = new char[strlen(_dll)+1];
	strcpy(Dll, _dll);
	Exports = 0;
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

#define DEFAULT_DLLPATH "Q:\\dll"

extern "C" HMODULE __stdcall dllLoadLibraryA(LPCSTR libname) 
{
	char * plibname = new char[strlen(libname)+20]; 
	sprintf(plibname, "%s\\%s", DEFAULT_DLLPATH ,(char *)libname);
	DllLoader * dllhandle = new DllLoader(plibname);
	dllhandle->Parse();
	dllhandle->ResolveImports();

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
	DllLoader * dllhandle = (DllLoader *)hLibModule;

	EntryFunc * initdll = (EntryFunc *)dllhandle->EntryAddress;
	(*initdll)( (HINSTANCE) dllhandle->hModule, 0 ,0);	//call "DllMian" with DLL_PROCESS_DETACH

	if ( dllhandle )
		delete dllhandle;
	return 1;
}

extern "C" FARPROC __stdcall dllGetProcAddress( HMODULE hModule, LPCSTR function )
{
	DllLoader * dllhandle = (DllLoader *)hModule;
	void * address = NULL;
	dllhandle->ResolveExport((char *)function, &address);
	return (FARPROC) address;
}

//dummy functions used to catch unresolved function calls
extern "C" void dummy_Unresolved(void) {
		static int Count = 0;
		char szBuf[128];
		sprintf(szBuf,"unresolved function called, Count number %d\n",Count++);
		OutputDebugString(szBuf);
}