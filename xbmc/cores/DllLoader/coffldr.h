#ifndef __COFFLDR_H_
#define __COFFLDR_H_
#pragma message("including coffldr.h")
#include "coff.h"
#include <stdio.h>
//#define DUMPING_DATA 1

class CoffLoader {
protected:

// Allocated structures...
    COFF_FileHeader_t       *CoffFileHeader;
    OptionHeader_t          *OptionHeader;
    WindowsHeader_t         *WindowsHeader;
    Image_Data_Directory_t  *Directory;
    SectionHeader_t         *SectionHeader;
    SymbolTable_t           *SymTable;
    char                    *StringTable;
    char                    **SectionData;

// Unnecessary data
//  This is data that is used only during linking and is not necessary
//  while the program is running in general


    int                     NumberOfSymbols;
    int                     SizeOfStringTable;
    int                     NumOfDirectories;
    int                     NumOfSections;

#ifdef DUMPING_DATA
// Members for printing the structures

    void PrintFileHeader(COFF_FileHeader_t *FileHeader);
    void PrintWindowsHeader(WindowsHeader_t *WinHdr);
    void PrintOptionHeader(OptionHeader_t *OptHdr);
    void PrintSection(SectionHeader_t *ScnHdr, char *data);
    void PrintStringTable(void);
    void PrintSymbolTable(void);
#endif

// Members for Loading the Different structures

    int LoadCoffFileHeader(FILE *fp);
    int LoadSymTable(FILE *fp);
    int LoadStringTable(FILE *fp);
    int LoadOptionHeaders(FILE *fp);
    int LoadDirectories(FILE *fp);
    int LoadSectionHeaders(FILE *fp);

// Members for access some of the Data

    int RVA2Section(unsigned long RVA);
    char *GetStringTblIndex(int index);
    char *GetStringTblOff(int Offset);
    char *GetSymbolName(SymbolTable_t *sym);
    char *GetSymbolName(int index);
    
    void PerformFixups(void);

public:
    int ParseCoff(FILE *fp);
    CoffLoader();
    ~CoffLoader();

};

#endif