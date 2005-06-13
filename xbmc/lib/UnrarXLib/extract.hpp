#ifndef _RAR_EXTRACT_
#define _RAR_EXTRACT_

enum EXTRACT_ARC_CODE {EXTRACT_ARC_NEXT,EXTRACT_ARC_REPEAT};

class CmdExtract
{
  private:
    ComprDataIO DataIO;
    Unpack *Unp;
    long TotalFileCount;

    long FileCount;
    long MatchedArgs;
    bool FirstFile;
    bool AllMatchesExact;
    bool ReconstructDone;

    char ArcName[NM];
    wchar ArcNameW[NM];

    char Password[MAXPASSWORD];
    bool PasswordAll;
    bool PrevExtracted;
    bool SignatureFound;
    char DestFileName[NM];
    wchar DestFileNameW[NM];
  public:
    CmdExtract();
    ~CmdExtract();
    void DoExtract(CommandData *Cmd);
    void ExtractArchiveInit(CommandData *Cmd,Archive &Arc);
    EXTRACT_ARC_CODE ExtractArchive(CommandData *Cmd);
    bool ExtractCurrentFile(CommandData *Cmd,Archive &Arc,int HeaderSize,
                            bool &Repeat);
    static void UnstoreFile(ComprDataIO &DataIO,Int64 DestUnpSize);
#ifdef _XBOX
    ComprDataIO &GetDataIO() {return DataIO;}
#endif
};

#endif