#ifndef _RAR_SCANTREE_
#define _RAR_SCANTREE_

enum { RECURSE_NONE=0,RECURSE_ALWAYS,RECURSE_WILDCARDS };
enum { SCAN_SKIPDIRS=0,SCAN_GETDIRS,SCAN_GETDIRSTWICE,SCAN_GETCURDIRS };
enum { SCAN_SUCCESS,SCAN_DONE,SCAN_ERROR,SCAN_NEXT };

#define MAXSCANDEPTH    (NM/2)

class CommandData;

class ScanTree
{
  private:
    bool PrepareMasks();
    int FindProc(FindData *FindData);

    FindFile *FindStack[MAXSCANDEPTH];
    int Depth;

    int SetAllMaskDepth;
    
    StringList *FileMasks;
    int Recurse;
    bool GetLinks;
    int GetDirs;
    int Errors;

    char CurMask[NM];
    wchar CurMaskW[NM];
    char OrigCurMask[NM];
    wchar OrigCurMaskW[NM];
    bool SearchAllInRoot;
    bool FastFindFile;
    int SpecPathLength;
    int SpecPathLengthW;

    char ErrArcName[NM];

    CommandData *Cmd;
  public:
  ScanTree(StringList *FileMasks,int Recurse,bool GetLinks,int GetDirs);
    ~ScanTree();
    int GetNext(FindData *FindData);
    int GetSpecPathLength() {return(SpecPathLength);};
    int GetSpecPathLengthW() {return(SpecPathLengthW);};
    int GetErrors() {return(Errors);};
    void SetErrArcName(const char *Name) {strcpy(ErrArcName,Name);}
    void SetCommandData(CommandData *Cmd) {ScanTree::Cmd=Cmd;}
};

#endif
