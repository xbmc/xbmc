
#ifndef __COFFLDR_H_
#define __COFFLDR_H_ 

//#pragma message("including coffldr.h")
#include "coff.h"

#ifndef _FILE_DEFINED
struct _iobuf {
        char *_ptr;
        int   _cnt;
        char *_base;
        int   _flag;
        int   _file;
        int   _charbuf;
        int   _bufsiz;
        char *_tmpfname;
        };
typedef struct _iobuf FILE;
#define _FILE_DEFINED
#endif

class CoffLoader
{
protected:

  //Pointers to somewhere in hModule, do not free these pointers
  COFF_FileHeader_t *CoffFileHeader;
  OptionHeader_t *OptionHeader;
  WindowsHeader_t *WindowsHeader;
  Image_Data_Directory_t *Directory;
  SectionHeader_t *SectionHeader;

  // Allocated structures... hModule now hold the master Memory handle
  SymbolTable_t *SymTable;
  char *StringTable;
  char **SectionData;

  // Unnecessary data
  //  This is data that is used only during linking and is not necessary
  //  while the program is running in general

  int NumberOfSymbols;
  int SizeOfStringTable;
  int NumOfDirectories;
  int NumOfSections;
  int FileHeaderOffset;

  // Members for printing the structures
  void PrintFileHeader(COFF_FileHeader_t *FileHeader);
  void PrintWindowsHeader(WindowsHeader_t *WinHdr);
  void PrintOptionHeader(OptionHeader_t *OptHdr);
  void PrintSection(SectionHeader_t *ScnHdr, char *data);
  void PrintStringTable(void);
  void PrintSymbolTable(void);

  // Members for Loading the Different structures
  int LoadCoffHModule(FILE * fp);
  int LoadSymTable(FILE *fp);
  int LoadStringTable(FILE *fp);
  int LoadSections(FILE *fp);

  // Members for access some of the Data

  int RVA2Section(unsigned long RVA);
  char *GetStringTblIndex(int index);
  char *GetStringTblOff(int Offset);
  char *GetSymbolName(SymbolTable_t *sym);
  char *GetSymbolName(int index);

  void PerformFixups(void);

public:
  void *hModule;   //standard windows HINSTANCE handle hold the whole image
  unsigned long EntryAddress; //Initialize entry point
  int ParseCoff(FILE *fp);
  CoffLoader();
  ~CoffLoader();

};

#endif
